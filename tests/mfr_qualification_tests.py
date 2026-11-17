"""Regression checks for evidence acceptance; no AE/GPU/process operations."""
import importlib.util
import json
import sys
import shutil
import tempfile
import unittest
from pathlib import Path
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
sys.dont_write_bytecode=True
spec=importlib.util.spec_from_file_location("mfr_check",ROOT/"tools/check_mfr_qualification.py")
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)


def interval(identity,thread,time,begin,end):
    common=dict(event="mfr-selector",module="main",nativeBuild=132,pid=11,id=identity,thread=thread,time=time,scale=24,selector=24)
    return [dict(common,phase="begin",qpc=begin),dict(common,phase="end",qpc=end)]


class EvidenceTests(unittest.TestCase):
    def test_intervals(self):
        a=interval(1,2,1,10,30);b=interval(2,3,2,20,40)
        self.assertEqual(module.journal_overlap([a[0],b[0],a[1],b[1]]),(True,[],2))
        self.assertFalse(module.journal_overlap(a+interval(2,3,2,30,40))[0])
        self.assertFalse(module.journal_overlap(a+interval(2,2,2,20,40))[0])
        self.assertFalse(module.journal_overlap(a+interval(2,3,1,20,40))[0])
        other=interval(2,3,2,20,40)
        for row in other:row["pid"]=12
        self.assertFalse(module.journal_overlap(a+other)[0])
        self.assertTrue(module.journal_overlap([a[0]])[1])
        self.assertTrue(module.journal_overlap([a[1]])[1])
        self.assertTrue(module.journal_overlap([a[0],a[0],a[1]])[1])
        pre=interval(2,3,2,20,40)
        for row in pre:row["selector"]=23
        self.assertFalse(module.journal_overlap([a[0],pre[0],a[1],pre[1]])[0])

    def test_pixels_and_host_proof(self):
        # Keep even test scratch confined to artifacts (never user temp folders).
        scratch=ROOT/"artifacts/mfr-01-20261116/checker-tests";scratch.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch) as directory:
            run=Path(directory);serial=run/"serial";parallel=run/"parallel";serial.mkdir();parallel.mkdir()
            for i in range(48):
                for target in (serial,parallel):Image.new("RGBA",(3,2),(i,5,9,255)).save(target/f"frame_{i:05}.png")
            names=[f"{depth}bpc-{case}-cpu" for depth in (8,16,32) for case in (0,1)]
            outputs=[dict(name=names[0],parallel=False,directory=str(serial)),dict(name=names[0],parallel=True,directory=str(parallel))]
            for index,name in enumerate(names[1:]):
                for flag,source in ((False,serial),(True,parallel)):
                    target=run/f"copy-{index}-{flag}";shutil.copytree(source,target);outputs.append(dict(name=name,parallel=flag,directory=str(target)))
            report=dict(nativeBuild=132,passed=True,hostProcessId=11,hostBeginQpc=1,hostEndQpc=100,host="23.6",cases=names,cudaAvailable=False,outputs=outputs)
            report_path=run/"report.json";report_path.write_text(json.dumps(report),encoding="utf8")
            a,b=interval(1,2,1,10,30),interval(2,3,2,20,40);journal=run/"journal.jsonl"
            journal.write_text("\n".join(json.dumps(row) for row in (a[0],b[0],a[1],b[1])),encoding="utf8")
            result=module.check_run(run,journal);self.assertEqual(result["serialParallelPngFrames"],288);self.assertFalse(result["hostQualified"])
            Image.new("RGBA",(3,2),(255,0,0,255)).save(parallel/"frame_00010.png")
            with self.assertRaisesRegex(ValueError,"pixel difference"):module.check_run(run,journal)
            Image.new("RGBA",(3,2),(10,5,9,255)).save(parallel/"frame_00010.png")
            report["hostProcessId"]=12;report_path.write_text(json.dumps(report),encoding="utf8")
            with self.assertRaisesRegex(ValueError,"overlapping"):module.check_run(run,journal)
            report["hostProcessId"]=11;report["hostBeginQpc"]=50;report_path.write_text(json.dumps(report),encoding="utf8")
            with self.assertRaisesRegex(ValueError,"overlapping"):module.check_run(run,journal)
            report["hostBeginQpc"]=1;report["outputs"][0]["directory"]=str(run.parent);report_path.write_text(json.dumps(report),encoding="utf8")
            with self.assertRaisesRegex(ValueError,"escapes"):module.check_run(run,journal)


if __name__=="__main__":unittest.main()
