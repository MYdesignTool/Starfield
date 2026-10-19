#pragma once
// Shared by all editor profiles so opening the standalone kernel does not
// require switching profiles. Runtime CUDA/OpenCL builds never include this
// file; the CPU test's shader namespace keeps its own per-thread function.
#if defined(__INTELLISENSE__)
#include <stddef.h>
#include <math.h>
#ifndef __global
#define __global
#endif
#ifndef __kernel
#define __kernel inline
#endif
// OpenCL built-in declaration, not a host implementation or replacement.
size_t get_global_id(unsigned int dimension);
#endif
