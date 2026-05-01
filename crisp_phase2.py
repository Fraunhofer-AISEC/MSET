#!/usr/bin/env python3
"""Crisp compile + final run helper.

This script takes a **bufferized** MLIR file (containing memref instead of tensor),
compiles it through the CRISP pipeline (lowering to LLVM dialect, then to
LLVM IR, object file, and finally an executable), and runs the executable.

Supports two configs:
  - asan:  standard ASan instrumentation
  - crisp: CRISP optimized ASan instrumentation

python crisp_phase2.py [testcases].mlir --config asan
"""

import argparse
import os
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple

# Paths
SCRIPT_DIR = Path(__file__).resolve().parent
# crisp_phase2.py may live directly in the project root or in a sub-directory.
# Auto-detect the real project root by looking for build/bin/mlir-opt.
def _find_project_root(start: Path) -> Path:
    p = start
    while p != p.parent:
        if (p / "build" / "bin" / "mlir-opt").exists():
            return p
        p = p.parent
    # Search sibling directories (e.g., MSET is inside a parent that also
    # contains torch-mlir with its own build/ directory).
    parent = start.parent
    for sibling in parent.iterdir():
        if sibling.is_dir() and (sibling / "build" / "bin" / "mlir-opt").exists():
            return sibling
    # Final fallback: only if mlir-opt really lives under SCRIPT_DIR/build
    if (SCRIPT_DIR / "build" / "bin" / "mlir-opt").exists():
        return SCRIPT_DIR
    return SCRIPT_DIR.parent

PROJECT_ROOT = _find_project_root(SCRIPT_DIR)
BUILD_DIR = PROJECT_ROOT / "build"
MLIR_OPT = BUILD_DIR / "bin" / "mlir-opt"
MLIR_TRANSLATE = BUILD_DIR / "bin" / "mlir-translate"
LLC = BUILD_DIR / "bin" / "llc"
ASAN_RT = Path("/usr/lib/llvm-22/lib/clang/22/lib/linux/libclang_rt.asan-x86_64.so")
MLIR_LIBDIR = BUILD_DIR / "lib"
SHARED_LIBS = [
    str(MLIR_LIBDIR / "libmlir_c_runner_utils.so"),
    str(MLIR_LIBDIR / "libmlir_runner_utils.so"),
]


@dataclass
class BenchConfig:
    tag: str
    asan_enabled: bool
    crisp_enabled: bool
    needs_asan_rt: bool


ASAN_CONFIG = BenchConfig(
    tag="asan", asan_enabled=True, crisp_enabled=False, needs_asan_rt=True
)

CRISP_CONFIG = BenchConfig(
    tag="crisp", asan_enabled=False, crisp_enabled=True, needs_asan_rt=True
)


def run_cmd(cmd: list, cwd=None, env=None, check=True) -> subprocess.CompletedProcess:
    """Run a shell command and optionally check for errors."""
    cmd_str = " ".join(str(c) for c in cmd)
    print(f"[cmd] {cmd_str}")
    result = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd, env=env)
    if result.stdout:
        print(f"[stdout]\n{result.stdout}")
    if result.stderr:
        print(f"[stderr]\n{result.stderr}")
    if check and result.returncode != 0:
        raise RuntimeError(f"Command failed (rc={result.returncode}): {cmd_str}")
    return result


def build_phase2_pipeline(asan: bool = False, crisp: bool = False) -> str:
    """Build the MLIR pass pipeline from bufferized MLIR to LLVM dialect."""
    passes = []

    # ASan/CRISP access instrumentation (before any lowering/canonicalize
    # that might remove the operations we want to instrument).
    if crisp or asan:
        passes.append("func.func(asan-access-instrument)")

    # Lifecycle instrumentation after bufferization is stable
    if crisp or asan:
        passes.append("func.func(asan-lifecycle-instrument)")

    # Lower linalg to loops, then affine
    passes.extend([
        "func.func(convert-linalg-to-loops)",
        "func.func(lower-affine)",
        "canonicalize",
    ])

    # CRISP optimization passes (only for crisp config)
    if crisp:
        passes.extend([
            "asan-optimization",
            "asan-check-elimination",
            "canonicalize",
            "asan-hoist-check",
            "canonicalize",
            "func.func(asan-static-check)",
            "func.func(asan-writeback-elimination)",
        ])

    # Convert ASan dialect to LLVM
    if asan or crisp:
        passes.append("convert-asan-to-llvm")

    # Lower to LLVM dialect
    passes.extend([
        "convert-scf-to-cf",
        "func.func(arith-expand)",
        "func.func(convert-math-to-llvm)",
        "convert-math-to-libm",
        "expand-strided-metadata",
        "finalize-memref-to-llvm",
        "convert-bufferization-to-memref",
        "finalize-memref-to-llvm",
        "convert-vector-to-scf",
        "convert-vector-to-llvm",
        "func.func(convert-arith-to-llvm)",
        "convert-func-to-llvm",
        "convert-cf-to-llvm",
        "convert-complex-to-llvm",
    ])

    # Convert ASan globals to LLVM
    if asan or crisp:
        passes.append("convert-asan-globals-to-llvm")

    # Final cleanup
    passes.extend([
        "reconcile-unrealized-casts",
        "canonicalize",
        "cse",
    ])

    return f"builtin.module({','.join(passes)})"


def _ensure_data_layout(input_path: Path, output_dir: Path) -> Path:
    """Workaround: inject a default x86_64 data_layout if missing.
    torch-mlir's ASanToLLVM pass crashes without it."""
    content = input_path.read_text()
    if "data_layout" in content:
        return input_path
    import re
    m = re.search(r'module\s*\{', content)
    if not m:
        return input_path
    dl_attr = 'llvm.data_layout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"'
    new_content = content[:m.start()] + f'module attributes {{ {dl_attr} }} {{' + content[m.end():]
    temp_path = output_dir / f"{input_path.stem}_dl.mlir"
    temp_path.write_text(new_content)
    return temp_path


def compile_mlir_to_llvm_dialect(
    input_path: Path, output_path: Path, cfg: BenchConfig, debug_ir: bool = False
) -> float:
    """Run mlir-opt to lower bufferized MLIR to LLVM dialect."""
    pipeline = build_phase2_pipeline(
        asan=cfg.asan_enabled,
        crisp=cfg.crisp_enabled,
    )
    # Workaround for torch-mlir ASanToLLVM missing data_layout
    if cfg.asan_enabled or cfg.crisp_enabled:
        input_path = _ensure_data_layout(input_path, output_path.parent)
    cmd = [
        str(MLIR_OPT),
        str(input_path),
        f"--pass-pipeline={pipeline}",
        "-o", str(output_path),
    ]
    if debug_ir:
        cmd.append("--mlir-print-ir-after-all")

    t0 = time.perf_counter()
    run_cmd(cmd)
    elapsed = time.perf_counter() - t0
    return elapsed


def translate_to_llvmir(llvm_dialect_path: Path, llvm_ir_path: Path):
    """Translate LLVM dialect MLIR to LLVM IR (.ll)."""
    run_cmd([
        str(MLIR_TRANSLATE),
        str(llvm_dialect_path),
        "--mlir-to-llvmir",
        "-o", str(llvm_ir_path),
    ])


def compile_llvmir_to_object(llvm_ir_path: Path, obj_path: Path):
    """Compile LLVM IR to object file with llc."""
    run_cmd([
        str(LLC),
        str(llvm_ir_path),
        "-o", str(obj_path),
        "--relocation-model=pic",
        "--filetype=obj",
    ])


def link_executable(obj_path: Path, exe_path: Path, cfg: BenchConfig):
    """Link object file into an executable with clang."""
    libs = []
    rpaths = []
    for lib in SHARED_LIBS:
        libs.extend(["-l", Path(lib).stem[3:]])  # strip 'lib' prefix
        rpaths.append(f"-Wl,-rpath,{Path(lib).parent}")

    cmd = [
        "clang",
        str(obj_path),
        "-o", str(exe_path),
        f"-L{MLIR_LIBDIR}",
    ] + libs + rpaths

    # Link ASan runtime if needed.
    # NOTE: Do NOT use -fsanitize=address here because the MLIR ASan passes
    # already emit the instrumentation; we only need the shared runtime.
    if cfg.needs_asan_rt:
        cmd.extend([f"-L{ASAN_RT.parent}", f"-l:{ASAN_RT.name}"])

    run_cmd(cmd)


def run_executable(exe_path: Path, cfg: BenchConfig) -> int:
    """Run the compiled executable and return its exit code."""
    env = os.environ.copy()

    # Set LD_LIBRARY_PATH so shared libs are found at runtime
    existing_ld = env.get("LD_LIBRARY_PATH", "")
    lib_paths = [str(MLIR_LIBDIR)]
    if existing_ld:
        lib_paths.append(existing_ld)
    env["LD_LIBRARY_PATH"] = ":".join(lib_paths)

    # Preload ASan runtime if needed
    if cfg.needs_asan_rt:
        existing = env.get("LD_PRELOAD", "")
        if "libclang_rt.asan" not in existing:
            env["LD_PRELOAD"] = (str(ASAN_RT) + " " + existing).strip()
        env.setdefault("ASAN_OPTIONS", "detect_leaks=0")

    result = run_cmd([str(exe_path)], env=env, check=False)
    return result.returncode


def main():
    parser = argparse.ArgumentParser(
        description="Compile bufferized MLIR to executable and run it."
    )
    parser.add_argument("input_mlir", help="Input bufferized MLIR file")
    parser.add_argument(
        "output_dir",
        nargs="?",
        default=None,
        help="Optional output directory. Default: ./<input_stem>_output",
    )
    parser.add_argument(
        "--config",
        choices=["asan", "crisp"],
        default="crisp",
        help="Select compilation config: 'asan' for ASan-only, 'crisp' for ASan+CRISP.",
    )
    parser.add_argument(
        "--keep-intermediates",
        action="store_true",
        help="Keep intermediate files (LLVM dialect, LLVM IR, object file)",
    )
    parser.add_argument("--debug-ir", action="store_true", help=argparse.SUPPRESS)

    args = parser.parse_args()

    input_path = Path(args.input_mlir)
    if not input_path.exists():
        raise FileNotFoundError(f"Input file not found: {input_path}")

    if args.output_dir is None:
        output_dir = Path.cwd() / f"{input_path.stem}_output"
    else:
        output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    cfg = ASAN_CONFIG if args.config == "asan" else CRISP_CONFIG

    # Intermediate files
    llvm_dialect_path = output_dir / f"{input_path.stem}_llvm.mlir"
    llvm_ir_path = output_dir / f"{input_path.stem}.ll"
    obj_path = output_dir / f"{input_path.stem}.o"
    exe_path = output_dir / input_path.stem

    try:
        print(f"[{cfg.tag}] Compiling bufferized MLIR -> LLVM dialect ...")
        elapsed = compile_mlir_to_llvm_dialect(
            input_path, llvm_dialect_path, cfg, debug_ir=args.debug_ir
        )
        print(f"[{cfg.tag}] MLIR lowering done in {elapsed:.4f} s")

        print(f"[{cfg.tag}] Translating LLVM dialect -> LLVM IR ...")
        translate_to_llvmir(llvm_dialect_path, llvm_ir_path)

        print(f"[{cfg.tag}] Compiling LLVM IR -> object file ...")
        compile_llvmir_to_object(llvm_ir_path, obj_path)

        print(f"[{cfg.tag}] Linking executable: {exe_path}")
        link_executable(obj_path, exe_path, cfg)

        print(f"[{cfg.tag}] Executing {exe_path} ...")
        exit_code = run_executable(exe_path, cfg)
        print(f"[{cfg.tag}] Exit code: {exit_code}")

        return exit_code
    finally:
        if not args.keep_intermediates:
            print(f"[{cfg.tag}] Cleaning up intermediates in {output_dir}")
            for f in [llvm_dialect_path, llvm_ir_path, obj_path, exe_path]:
                if f.exists():
                    f.unlink()
            # Remove output directory if it's now empty
            if output_dir.exists() and not any(output_dir.iterdir()):
                output_dir.rmdir()


if __name__ == "__main__":
    sys.exit(main())
