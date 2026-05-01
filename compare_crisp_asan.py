#!/usr/bin/env python3
"""
compare_crisp_asan.py

Run specific test-case keys (where CRISP and ASan overall results differ)
with both configs. Only keep logs when the two configs behave differently.
//这里的bug type是从evaluate_mlir中选出，硬编码。

Directory layout:
  crisp_asan_diff/
    <full_test_case_key>/
      <variant_name>/
        asan.log
        crisp.log

python compare_crisp_asan.py
"""

import shutil
import subprocess
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Hard-coded keys to investigate (from evaluate_mlir.py diff output)
# ---------------------------------------------------------------------------

TARGET_KEYS = [
    # Spatial
    "Inter-Object Linear OOBA Underflow Direct Write Global Global",
    "Intra-Object Linear OOBA Overflow Direct Write Global Global",
    "Non-Object Linear OOBA Underflow Direct Write Global Global",
    "Inter-Object Linear OOBA Overflow Direct Write Heap Global",
    "Inter-Object Linear OOBA Overflow Direct Write Heap Heap",
    "Intra-Object Linear OOBA Overflow Direct Write Heap Heap",
    "Non-Object Linear OOBA Underflow Direct Write Heap Heap",
    "Inter-Object Linear OOBA Overflow Direct Write Heap Stack",
    "Inter-Object Linear OOBA Underflow Direct Write Stack Stack",
    "Intra-Object Linear OOBA Overflow Direct Write Stack Stack",
    "Non-Object Linear OOBA Underflow Direct Write Stack Stack",
    # Temporal
    "Use-after-* Direct Read Heap Freed Memory",
    "Use-after-* Direct Write Heap Freed Memory",
    "Use-after-* Stdlib Read Heap Freed Memory",
    "Use-after-* Stdlib Write Heap Freed Memory",
    "Use-after-* Direct Read Stack Freed Memory",
    "Use-after-* Direct Write Stack Freed Memory",
    "Use-after-* Stdlib Read Stack Freed Memory",
    "Use-after-* Stdlib Write Stack Freed Memory",
    "Use-after-* Direct Read Heap (Re)used Memory",
    "Use-after-* Direct Write Heap (Re)used Memory",
    "Use-after-* Stdlib Read Heap (Re)used Memory",
    "Use-after-* Stdlib Write Heap (Re)used Memory",
    "Use-after-* Direct Read Stack (Re)used Memory",
    "Use-after-* Direct Write Stack (Re)used Memory",
    "Use-after-* Stdlib Read Stack (Re)used Memory",
    "Use-after-* Stdlib Write Stack (Re)used Memory",
]

# ---------------------------------------------------------------------------
# Key -> filename glob pattern
# ---------------------------------------------------------------------------

BUG_MAP = {
    "Linear OOBA": "linear_ooba",
    "Non-Linear OOBA": "non_linear_ooba",
    "Type Confusion OOBA": "type_confusion_ooba",
    "Use-after-*": "use_after_star",
    "Double-free": "double_free",
    "Misuse-of-free": "misuse_of_free",
}

REL_MAP = {
    "Non-Object": "non_object",
    "Intra-Object": "intra_object",
    "Inter-Object": "inter_object",
}

FLOW_MAP = {
    "Overflow": "overflow",
    "Underflow": "underflow",
}

LOC_MAP = {
    "Direct": "direct",
    "Stdlib": "stdlib",
}

ACT_MAP = {
    "Read": "read",
    "Write": "write",
}

REGION_MAP = {
    "Heap": "heap",
    "Stack": "stack",
    "Global": "global",
}

MEM_MAP = {
    "Freed Memory": "freed_memory",
    "(Re)used Memory": "used_memory",
}


def key_to_glob(key: str) -> str:
    """Convert a test-case key to a glob pattern that matches .mlir filenames."""
    parts = key.split()

    # --- Spatial: relation is always the first word ---
    if parts[0] in REL_MAP:
        # Parse from the back (words are guaranteed single-token except bug name)
        origin = REGION_MAP[parts[-1]]
        target = REGION_MAP[parts[-2]]
        act = ACT_MAP[parts[-3]]
        loc = LOC_MAP[parts[-4]]
        flow = FLOW_MAP[parts[-5]]
        relation = REL_MAP[parts[0]]
        # Everything between relation and flow is the bug name
        bug = BUG_MAP[" ".join(parts[1:-5])]
        return f"{bug}_{origin}_{target}_{relation}_{flow}_{loc}_{act}_*.mlir"

    # --- Temporal: try to match memory_state from the back ---
    for mem_name, mem_code in MEM_MAP.items():
        mem_words = mem_name.split()
        if len(parts) >= len(mem_words) and parts[-len(mem_words):] == mem_words:
            remaining = parts[:-len(mem_words)]
            region = REGION_MAP[remaining[-1]]
            act = ACT_MAP[remaining[-2]]
            loc = LOC_MAP[remaining[-3]]
            bug = BUG_MAP[" ".join(remaining[:-3])]
            return f"{bug}_{mem_code}_{region}_{loc}_{act}_*.mlir"

    # Fallback
    return key + "*.mlir"


# ---------------------------------------------------------------------------
# Behavior classification
# ---------------------------------------------------------------------------

def classify_behavior(rc: int, err: str) -> str:
    if "Traceback (most recent call last):" in err:
        return "COMPILE_FAIL"
    if "ERROR: AddressSanitizer:" in err:
        return "ASAN_REPORT"
    if rc in (0, 42):
        return "OK"
    if rc == 43:
        return "PRECOND_FAIL"
    return f"OTHER:{rc}"


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    test_dir = Path("test_cases_mlir")
    out_dir = Path("crisp_asan_diff")

    kept = 0
    skipped = 0

    for key in TARGET_KEYS:
        glob_pat = key_to_glob(key)
        matches = sorted(test_dir.glob(glob_pat))
        if not matches:
            print(f"[WARN] No files match key: {key}  (pattern: {glob_pat})")
            continue

        print(f"\n=== {key} ===  ({len(matches)} variants)")

        for mlir in matches:
            base = mlir.stem
            variant_dir = out_dir / key / base

            # Run ASan
            stdout_asan = ""
            try:
                result_asan = subprocess.run(
                    [sys.executable, "crisp_phase2.py", str(mlir), "--config", "asan"],
                    capture_output=True,
                    text=True,
                    timeout=60,
                )
                rc_asan = result_asan.returncode
                stderr_asan = result_asan.stderr
                stdout_asan = result_asan.stdout
            except Exception as e:
                rc_asan = -1
                stderr_asan = str(e)

            # Run CRISP
            stdout_crisp = ""
            try:
                result_crisp = subprocess.run(
                    [sys.executable, "crisp_phase2.py", str(mlir), "--config", "crisp"],
                    capture_output=True,
                    text=True,
                    timeout=60,
                )
                rc_crisp = result_crisp.returncode
                stderr_crisp = result_crisp.stderr
                stdout_crisp = result_crisp.stdout
            except Exception as e:
                rc_crisp = -1
                stderr_crisp = str(e)

            beh_asan = classify_behavior(rc_asan, stderr_asan)
            beh_crisp = classify_behavior(rc_crisp, stderr_crisp)

            if beh_asan != beh_crisp:
                variant_dir.mkdir(parents=True, exist_ok=True)
                asan_log = (
                    f"=== ASan (rc={rc_asan}, behavior={beh_asan}) ===\n"
                    f"--- stdout ---\n{stdout_asan}\n"
                    f"--- stderr ---\n{stderr_asan}\n"
                )
                crisp_log = (
                    f"=== CRISP (rc={rc_crisp}, behavior={beh_crisp}) ===\n"
                    f"--- stdout ---\n{stdout_crisp}\n"
                    f"--- stderr ---\n{stderr_crisp}\n"
                )
                (variant_dir / "asan.log").write_text(asan_log, encoding="utf-8")
                (variant_dir / "crisp.log").write_text(crisp_log, encoding="utf-8")
                print(f"  [DIFF] {base:<70} | {beh_asan:<12} vs {beh_crisp:<12}")
                kept += 1
            else:
                print(f"  [SAME] {base:<70} | {beh_asan:<12} (rc={rc_asan} vs {rc_crisp})")
                skipped += 1

    print(f"\nDone. Kept {kept}, skipped {skipped}.")
    print(f"Differences saved to: {out_dir}/")


if __name__ == "__main__":
    main()
