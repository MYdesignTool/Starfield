"""Verify real offline journal records; these never count as AE qualification."""
import importlib.util
import json
import sys
from pathlib import Path

sys.dont_write_bytecode=True
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("mfr_check",root/"tools/check_mfr_qualification.py")
checker=importlib.util.module_from_spec(spec);spec.loader.exec_module(checker)
records=[json.loads(line) for line in (root/"artifacts/mfr-offline-handoff-20261116/journal-tests/records.jsonl").read_text(encoding="utf8").splitlines()]
overlap,errors,intervals=checker.journal_overlap(records)
assert len(records)==194 and intervals==97,(len(records),intervals)
assert overlap and not errors,(overlap,errors)
assert len({row["pid"] for row in records})==1
assert len({row["thread"] for row in records if row["selector"]==24})==4
assert sum(row["selector"]==31 for row in records)==96
assert sum(row["selector"]==23 for row in records)==2
assert max(row["active"] for row in records)>=4
assert len({row["id"] for row in records})==97
assert {row["nativeBuild"] for row in records}=={132}
print("Production journal: 194 intact records / 97 paired intervals, four render threads, overlap, nested GPU exception cleanup, unique IDs and pre-render filtering passed. Offline only; hostQualified=false.")
