"""Read-only native132 smoke evidence check; never launch AE or alter preferences."""
import argparse
import json
from pathlib import Path
from PIL import Image


def journal_overlap(records, module="main"):
    starts, intervals, errors = {}, [], []
    for row in records:
        if row.get("event") != "mfr-selector" or row.get("module") != module:
            continue
        key = (row["pid"], row["module"], row["id"])
        if row["phase"] == "begin":
            if key in starts:
                errors.append("duplicate begin")
            starts[key] = row
        elif row["phase"] == "end":
            begin = starts.pop(key, None)
            if begin is None or begin["thread"] != row["thread"] or begin["qpc"] > row["qpc"]:
                errors.append("unpaired or invalid interval")
            else:
                intervals.append((begin, row))
    if starts:
        errors.append("unfinished selectors")
    # Thread IDs or active counters alone also count reentry. Require intervals
    # on different threads and frame times, in the same loaded module/process.
    overlap = False
    # May2023 AE_Effect.h; checked by mfr_binary_flags_tests.cpp against the SDK.
    render_selectors = (11, 24, 31)
    for index, (a, end_a) in enumerate(intervals):
        for b, end_b in intervals[index + 1:]:
            if (a.get("selector") in render_selectors and b.get("selector") in render_selectors
                    and a["pid"] == b["pid"] and a["thread"] != b["thread"]
                    and a["time"] * b["scale"] != b["time"] * a["scale"]
                    and max(a["qpc"], b["qpc"]) < min(end_a["qpc"], end_b["qpc"])):
                overlap = True
                break
        if overlap:
            break
    return overlap, errors, len(intervals)


def check_run(run: Path, journal: Path):
    report = json.loads((run / "report.json").read_text(encoding="utf-8-sig"))
    if report.get("nativeBuild") != 132 or not report.get("passed"):
        raise ValueError("Host render smoke did not complete")
    expected = {f"{depth}bpc-{case}-{backend}" for depth in (8,16,32) for case in (0,1)
                for backend in (("cpu","cuda") if report.get("cudaAvailable") else ("cpu",))}
    if set(report.get("cases",[])) != expected or not str(report.get("host","")).startswith("23."):
        raise ValueError("Incomplete AE2023 smoke case matrix")
    pairs = {}
    directories = set()
    for output in report["outputs"]:
        directory = Path(output["directory"]).resolve()
        if not directory.is_relative_to(run.resolve()):
            raise ValueError("Output directory escapes the test run")
        if directory in directories:
            raise ValueError("Reused smoke output directory")
        directories.add(directory)
        pairs.setdefault(output["name"], {})[output["parallel"]] = directory
    if set(pairs) != expected or len(report["outputs"]) != len(expected)*2:
        raise ValueError("Missing or duplicate smoke output pairs")
    compared = 0
    for name, pair in pairs.items():
        if set(pair) != {False, True}:
            raise ValueError(f"Missing serial/parallel pair: {name}")
        serial, parallel = sorted(pair[False].glob("*.png")), sorted(pair[True].glob("*.png"))
        if len(serial) != 48 or [p.name for p in serial] != [p.name for p in parallel]:
            raise ValueError(f"Expected matching 48-frame sequences: {name}")
        for first, second in zip(serial, parallel):
            with Image.open(first) as a, Image.open(second) as b:
                a.load(); b.load()
                if a.size != b.size or a.mode != b.mode or a.tobytes() != b.tobytes():
                    raise ValueError(f"Serial/MFR pixel difference: {name}/{first.name}")
            compared += 1
    records = [json.loads(line) for line in journal.read_text(encoding="utf-8").splitlines() if line.strip()]
    # Restrict to this host process. An offline selector fixture cannot qualify AE.
    pid = report.get("hostProcessId")
    begin_qpc, end_qpc = report.get("hostBeginQpc"), report.get("hostEndQpc")
    if not all(isinstance(value,int) for value in (pid,begin_qpc,end_qpc)) or begin_qpc>=end_qpc:
        raise ValueError("Host process ID and run bounds must be recorded by the approved launch runner")
    records = [r for r in records if r.get("pid") == pid and r.get("nativeBuild") == 132]
    scoped = [r for r in records if r.get("event")=="mfr-selector" and begin_qpc<=r.get("qpc",-1)<=end_qpc]
    overlap, errors, intervals = journal_overlap(scoped)
    if errors or not overlap:
        raise ValueError(f"No complete, overlapping host frame intervals: {errors}")
    if any(r.get("event") == "render" and r.get("result") not in (0, None) for r in records):
        raise ValueError("Host journal contains render errors/cancellation during smoke")
    return dict(nativeBuild=132,hostProcessId=pid,serialParallelPngFrames=compared,
                completeMainIntervals=intervals,overlappingFrames=True,smokePassed=True,
                hostQualified=False,remaining="Full precision export, resource families, live edits, cancellation, save/reopen")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run",type=Path)
    parser.add_argument("--journal",type=Path,default=Path("artifacts/graph-render-journal-132.jsonl"))
    args = parser.parse_args()
    print(json.dumps(check_run(args.run,args.journal),indent=2))
