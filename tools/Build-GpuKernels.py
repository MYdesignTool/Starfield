"""Compile our CUDA PTX and embed our OpenCL source. Build inputs stay in artifacts.

NVRTC is used at build time only; no runtime/compiler DLL is distributed or loaded
by the AEX. See ADR 0026 for the pinned vendor wheel/hash and local setup command.
"""
import argparse
import ctypes as c
import hashlib
import os
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--nvrtc', required=True, type=Path)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
source = (root / 'ae_plugin/gpu/SpriteKernel.h').read_text()
expected = {'nvrtc64_120_0.dll':'3aa3cd8aa10437e212760c0e1ed730807811ec3bc330216dbfde4b26211d2243',
            'nvrtc-builtins64_124.dll':'79888dba26c51475ea21fc7b47d2b9dd5b1ffaecc8e5ea22a49fa5f5a722eb43'}
for name,digest in expected.items():
    if hashlib.sha256((args.nvrtc.resolve().parent/name).read_bytes()).hexdigest()!=digest:
        raise RuntimeError('Pinned NVRTC build input hash mismatch: '+name)
with os.add_dll_directory(str(args.nvrtc.resolve().parent)):
    builtins = c.CDLL(str(args.nvrtc.resolve().parent / 'nvrtc-builtins64_124.dll'))
    dll = c.CDLL(str(args.nvrtc.resolve()))
    program = c.c_void_p()
    dll.nvrtcCreateProgram.argtypes = [c.POINTER(c.c_void_p),c.c_char_p,c.c_char_p,c.c_int,c.c_void_p,c.c_void_p]
    dll.nvrtcCompileProgram.argtypes = [c.c_void_p,c.c_int,c.POINTER(c.c_char_p)]
    dll.nvrtcGetProgramLogSize.argtypes = [c.c_void_p,c.POINTER(c.c_size_t)]
    dll.nvrtcGetProgramLog.argtypes = [c.c_void_p,c.c_void_p]
    dll.nvrtcGetPTXSize.argtypes = [c.c_void_p,c.POINTER(c.c_size_t)]
    dll.nvrtcGetPTX.argtypes = [c.c_void_p,c.c_void_p]
    dll.nvrtcDestroyProgram.argtypes = [c.POINTER(c.c_void_p)]
    if dll.nvrtcCreateProgram(c.byref(program),('#define SF_CUDA\n'+source).encode(),b'SpriteKernel.cu',0,None,None):
        raise RuntimeError('NVRTC program creation failed')
    try:
        options=(c.c_char_p*3)(b'--gpu-architecture=compute_52',b'--fmad=false',b'--std=c++14')
        status=dll.nvrtcCompileProgram(program,len(options),options)
        size=c.c_size_t()
        dll.nvrtcGetProgramLogSize(program,c.byref(size))
        log=c.create_string_buffer(size.value)
        dll.nvrtcGetProgramLog(program,log)
        if status: raise RuntimeError(log.value.decode())
        dll.nvrtcGetPTXSize(program,c.byref(size))
        ptx=c.create_string_buffer(size.value)
        if dll.nvrtcGetPTX(program,ptx): raise RuntimeError('NVRTC PTX retrieval failed')
        # OpenCL spells standard float functions without the C 'f' suffix.
        opencl='#pragma OPENCL FP_CONTRACT OFF\n'+source.replace('unsigned long long','ulong')
        for name in ['fmin','fmax','fabs','sqrt']:
            opencl=opencl.replace(name+'f(',name+'(')
        header='// Generated from our SpriteKernel.h; do not track this build output.\n#pragma once\n'
        header+='inline constexpr char kCudaPtx[]=R"SFPTX('+ptx.value.decode()+')SFPTX";\n'
        header+='inline constexpr char kOpenClSource[]=R"SFCL('+opencl+')SFCL";\n'
        args.output.parent.mkdir(parents=True,exist_ok=True)
        if not args.output.exists() or args.output.read_text()!=header: args.output.write_text(header)
        print('GPU kernels generated:',args.output,'source SHA256',hashlib.sha256(source.encode()).hexdigest())
    finally: dll.nvrtcDestroyProgram(c.byref(program))
