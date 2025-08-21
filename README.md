# MSET: Memory Sanitizer Evaluation Tool

MSET is an evaluation tool for memory safety sanitizers.

It generates test cases, each containing a specific type of memory bug, including:

- Linear out-of-bounds access (contiguous access)
- Non-linear out-of-bounds access (non-contiguous access)
- Out-of-bounds access due to type confusion (access after a cast to a different type)
- Double-free
- Use-after-* (use-after-return or use-after-free access)
- Misuse-of-free (passing a pointer that was not returned by `malloc` to `free`)

Each memory bug occurs in a memory region (stack, heap, or global memory) or
between two regions. The memory bugs attempt to read or write memory, either
directly in the code or by using a `stdlib` function.

For more information, please refer to the SP '25 conference paper (author's version [PDF](https://publica-rest.fraunhofer.de/server/api/core/bitstreams/2db39f21-69f6-45c1-b117-0aa5cb096eac/content)):

> E. Vintila, P. Zieris and J. Horsch, "Evaluating the Effectiveness of Memory
> Safety Sanitizers," in 2025 IEEE Symposium on Security and Privacy (SP), San
> Francisco, CA, USA, 2025, pp. 88-88, doi:
> [10.1109/SP61157.2025.00088](https://doi.ieeecomputersociety.org/10.1109/SP61157.2025.00088).

Please note that this is a prototype implementation.

## Current results

The current results, based on the latest version of MSET, are listed in the table below.
If you want your own sanitizer to be listed, please open a PR and/or contact us.

### Number and percentage of test cases detected by each sanitizer, sorted by bug type
| Sanitizer                                                                                 | Linear OOBA            | Non-linear OOBA        | Type confusion OOBA   | Use-after-*           | Double-free | Misuse-of-free |
|:------------------------------------------------------------------------------------------|:-----------------------|:-----------------------|:----------------------|:----------------------|:------------|:---------------|
| Baseline                                                                                  | 36 (40%)               | 18 (25%)               | 12 (40%)              | 0 (0%)                | 0 (0%)      | 0 (0%)         |
| [ASan](https://clang.llvm.org/docs/AddressSanitizer.html)                                 | 70 (77.8%)             | 18 (25%)               | 18 (60%)              | 8 (50%)               | 4 (100%)    | 20 (100%)      |
| [ASan--](https://www.github.com/junxzm1990/ASAN--)                                        | 70 (77.8%)             | 18 (25%)               | 18 (60%)              | 8 (50%)               | 4 (100%)    | 20 (100%)      |
| [Delta Pointers](https://www.github.com/vusec/deltapointers)                              | 60 (66.7%)             | 46 (63.9%)*            | 19 (63.3%)*           | 0 (0%)                | 0 (0%)      | 0 (0%)         |
| [Dr. Memory](https://drmemory.org/)                                                       | 44 (48.9%)             | 18 (25%)               | 14 (46.7%)            | 4 (25%)               | 4 (100%)    | 20 (100%)      |
| [EffectiveSan](https://www.github.com/GJDuck/EffectiveSan)                                | 77 (85.6%)*            | 72 (100%)              | 28 (93.3%)            | 1 (6.2%)              | 4 (100%)    | 12 (60%)       |
| [Electric Fence](https://manpages.debian.org/unstable/electric-fence/libefence.3.en.html) | 46 (51.1%)             | 18 (25%)               | 16 (53.3%)            | 4 (25%)               | 4 (100%)    | 20 (100%)      |
| [FreeGuard](https://www.github.com/UTSASRG/FreeGuard)*                                    | 38.3, σ = 1.6 (42.6%)  | 18.6, σ = 0.7 (25.8%)  | 13.6, σ = 0.5 (45.3%) | 0 (0%)                | 4 (100%)    | 8 (40%)        |
| [HWASAN](https://clang.llvm.org/docs/HardwareAssistedAddressSanitizerDesign.html)*        | 71.8, σ = 0.4 (79.8%)  | 53.7, σ = 0.5 (74.6%)  | 24, σ = 0.0 (80%)     | 15.7, σ = 0.5 (98.1%) | 4 (100%)    | 12 (60%)       |
| [LowFat](https://www.github.com/GJDuck/LowFat)                                            | 60 (66.7%)             | 54 (75%)               | 18 (60%)              | 0 (0%)                | 0 (0%)      | 12 (60%)       |
| [Memcheck](https://valgrind.org/docs/manual/mc-manual.html)                               | 48 (53.3%)             | 18 (25%)               | 16 (53.3%)            | 4 (25%)               | 4 (100%)    | 20 (100%)      |
| [QASan](https://github.com/andreafioraldi/qasan)                                          | 47 (52.2%)             | 18.4, σ = 0.8 (25.6%)* | 14 (46.7%)            | 4 (25%)               | 4 (100%)    | 20 (100%)      |
| [RedFat](https://www.github.com/GJDuck/RedFat)                                            | 48 (53.3%)             | 28 (38.9%)             | 16 (53.3%)            | 4 (25%)               | 4 (100%)    | 0 (0%)         |
| [Scudo](https://llvm.org/docs/ScudoHardenedAllocator.html)*                               | 36, σ = 0.0 (40%)      | 19.5, σ = 0.9 (27.1%)  | 12.1, σ = 0.3 (40.3%) | 0 (0%)                | 4 (100%)    | 20 (100%)      |
| [Softbound+CETS](https://www.github.com/santoshn/softboundcets-34)                        | 72 (80%)               | 54 (75%)               | 24 (80%)              | 16 (100%)             | 4 (100%)    | 20 (100%)      |
| [Softbound+CETS (rev.)](https://www.github.com/Fraunhofer-AISEC/softboundcets)            | 84 (93.3%)             | 66 (91.7%)             | 28 (93.3%)            | 16 (100%)             | 4 (100%)    | 20 (100%)      |

*results are different to those produced by v1.0. See details [here](https://github.com/Fraunhofer-AISEC/MSET/tree/main?tab=readme-ov-file#version-11).

## Building MSET on Ubuntu/Debian Linux

Building MSET requires the following packages: `cmake`, `make`. To install them,
run:

```bash
sudo apt install cmake make
```

From the project's root directory, execute the following commands. The MSET
binary will be placed in the project's root directory.

```bash
mkdir build
cd build
cmake -S ../src
cmake --build .
```

## Usage

To use MSET, run the following command from the project's build directory:

```bash
./mset --evaluate ../sanitizer_configs/<SANITIZER_CONFIG>
```

This command will read the `test_cases` directory and use the test cases to
evaluate the sanitizer specified by the configuration file `<SANITIZER_CONFIG>`.
Configuration files for most available sanitizers can be found in the
`sanitizer_configs` directory. All provided configurations assume that the
sanitizers are either installed globally (e.g., Clang) or located in
subdirectories under `sanitizers/`. Please ensure that you have installed the
sanitizers you wish to evaluate before executing MSET.

To ensure a meaningful evaluation of the sanitizer's benefits compared to the
default mitigations enabled by the compiler, make sure to specify the
`--evaluate-baseline` option. With this option, MSET will execute each test case
twice: once with the sanitizer activated and once without. To save time during
re-evaluations of a sanitizer, only enable this option when changes to the
baseline are expected (e.g., after compiler version updates).

By default, MSET will remove the binary of each test case after evaluation. To
retain the binaries, use the `--keep-binaries` option. This will cause MSET to
save each test case binary in a directory named `test_case_binaries`, located
within the `test_cases` directory.

To skip the compilation step when evaluating, use the `--evaluate-prebuilt-binaries`
option. This will cause MSET to look for binaries, instead of source files, and
use them for the evaluation. This assumes that the binaries are generated by a
previous evaluation that was done using `--keep-binaries` or by a `--compile` run.  

In addition to the standard usage of MSET, you can use the `--generate` option
to (re)generate test cases. This can be useful for developing new test cases or
modifying existing ones to test your sanitizer more specifically. By default,
`--generate` will use the `test_cases` directory. If you wish to store the test
case files in a different location, use the `--test-case-dir <TEST_CASE_DIR>`
option. If the specified directory is not empty and the `--clean-test-cases`
option is not used, the generation will be aborted. To remove previously
generated test case files before creating new ones, use the `--clean-test-cases`
option. Note that this option removes the content of the `test_cases` directory
by default, but can also be combined with `--test-case-dir <TEST_CASE_DIR>`. The
`--test-case-dir <TEST_CASE_DIR>` option can also be used in conjunction with
`--evaluate <SANITIZER_CONFIG>` to use the test case files from the alternative
location. Also, note that the `--keep-binaries` option will respect the alternative
location and create the `test_case_binaries` directory there.

To compile the test cases without evaluating them, use the `--compile` option.
This will cause MSET to compile each test case and place the binaries in a directory
named `test_case_binaries`, located within the `test_cases` directory.

By default, MSET will provide detailed results of the evaluation. To print even
more details, use the `--verbose` option.

For a detailed usage description of MSET, use the `--help` option.

### Examples

To see which memory bugs are thwarted in Clang-compiled programs by default,
assuming Clang is installed on your system, run:

```bash
./mset --evaluate ../sanitizer_configs/clang.xml
```

To evaluate ASan with respect to the baseline, again assuming Clang is installed
on your system, run:

```bash
./mset --evaluate ../sanitizer_configs/asan_clang.xml --evaluate-baseline
```

## Sanitizer configurations

To configure a sanitizer for evaluation with MSET, we use XML configuration
files. A minimal configuration containing all the mandatory tags looks like
this:

```xml
<sanitizer>
  <name>...</name>
  <setup>
    <compile_cmd>...</compile_cmd>
  </setup>
  <setup_baseline>
    <compile_cmd>...</compile_cmd>
  </setup_baseline>
  <run>...</run>
  <run_baseline>...</run_baseline>
</sanitizer>
```

Use the `<name>` tag to assign a name to the sanitizer. Within the `<setup>` tag, use the `<compile_cmd>` tag to specify
how to compile a test case. You can also include zero or more `<cmd>` tags to perform additional preparation steps for the
test case. Note that the values for the macros used by the test cases are appended automatically to the `<compile_cmd>`.
For  example, to compile a test case with ASan, a single `<compile_cmd>` tag is needed, where
its value specifies the command with the required flags. The special tokens
`$SOURCE_FILE` and `$GENERATED_BINARY` are mandatory and will be replaced by
MSET when running the test case.
A dedicated section is used for allocating the auxiliary variables that are required by the test cases,
to avoid having them unintentionally overwritten.
Our default linker script fragment `sanitizers/after_text.ld` is passed via `-Wl,-T,../../sanitizers/after_text.ld` and
should work for most sanitizers.
In our set of tested sanitizers, only RedFat requires a different linker script fragment `before_bss.ld` because placing
a section after text does not work for it.
If needed, provide your own section to ensure that the auxiliary variables are not overwritten and can be 
accessed for reading and writing.

```xml
<setup>
  <compile_cmd>clang -fsanitize=address -Wl,-T,../../sanitizers/after_text.ld $SOURCE_FILE -o $GENERATED_BINARY</compile_cmd>
</setup>
```

Use the `<setup_baseline>` tag to specify how to prepare a test case for
execution without the sanitizer enabled (refer to [Usage](#usage) for more
details on the baseline).

To specify how to run the resulting binary, use the `<run>` tag. Similarly, use
`<run_baseline>` for the baseline. In the simplest form, the run command just
executes the generated binary:

```xml
<run>$GENERATED_BINARY</run>
```

If the sanitizer requires a wrapper around the binary (e.g., QASan, Memcheck),
the `<run>` value will look like this:

```xml
<run>../sanitizers/qasan/qasan $GENERATED_BINARY</run>
```

```xml
<run>../sanitizers/memcheck/valgrind --exit-on-first-error=yes --error-exitcode=44 --undef-value-errors=no $GENERATED_BINARY</run>
```

If running the binary requires environment variables, specify them using
`<env_var>` tags under `<run_env_args>`:

```xml
<run_env_args>
  <env_var name="ASAN_OPTIONS">detect_leaks=0:handle_segv=0</env_var>
</run_env_args>
```

```xml
<env_var name="LD_PRELOAD">../sanitizers/freeguard/libfreeguard.so</env_var>
```

For sanitizers that use pointer tagging (e.g., HWASAN), use the `address_mask`
tag to allow test cases to ignore tags when comparing pointer values.

To override the default values for how the results of the test cases are interpreted,
the following tags can be used:
`bug_not_detected_exit_value`, `precondition_not_met_exit_value`, `timeout_exit_value`,
and `bug_detected_exit_values`, the last of which is a list of value tags.
To specify a custom timeout duration, use `timeout_seconds`.
For examples, refer to the `clang.xml` configuration file.

Existing sanitizer configurations can be found in the `sanitizer_configs`
directory. Please note that all provided configurations assume that the
sanitizers are either installed globally (e.g., Clang) or located in
subdirectories under `sanitizers/`.

## Limitations / TODOs

Please note that MSET has the following implementation-specific limitations:

- Some test cases, in particular those containing double-frees or
  misuses-of-free, are specific to `glibc`. We are considering adding variants
  compatible with other `libc` implementations, as well as a new exit value to
  indicate when variants are `INCOMPATIBLE`.

- The MSET test cases currently rely only on `memcpy` and `memset` for memory
  access. Other `stdlib` functions that interact with memory have not yet been
  incorporated.

- Spatial test cases use fixed sizes for both the origin and target locations.
  Exploring variable sizes may reveal limitations in the precision of sanitizers.

For more details and additional limitations that are not
implementation-specific, please refer to the paper.

## Version 1.1

v1.1 introduces several improvements and new features:

- Added an option to compile without evaluating or generating.

- Added an option to evaluate using prebuilt binaries.

- Fixed issues related to the inconsistent usage of `memcpy`/`memset`.

- Introduced additional test case variants.

- Replaced the linear OOBA variants that differed in the way the auxiliary
variables were allocated with a single variant that uses a dedicated
memory section for them.

- General code improvements and refactoring.

To reproduce the results from the paper, please use version [v1.0](https://github.com/Fraunhofer-AISEC/MSET/releases/tag/v1.0).

In some cases MSET v1.1 produces results that differ from those in the paper (produced with version v1.0).
These are explained as follows:

- Delta Pointers

  - Non-linear OOBA: the results are the same as those in the paper (Figure 5 and Section 5),
    but there is a typo in Table 3 from the appendix

  - Type confusion OOBA: one additional test case is detected because the code generation has
    been improved to further avoid optimizations. In this particular case, the `origin` is used after the bug.

- EffectiveSan

  - Linear OOBA: 3 additional test cases are not detected because they use `memset`, which is known to cause problems
    for EffectiveSan. This is described in the paper.

- QASan

  - Non-linear OOBA: The detection of such bugs is probabilistic for QASan, when used with MSET. This is because of its
    modified memory layout that can reorder `regions` across runs, making some test cases behave differently.
    For example, overflowing between 2 `regions` can only be attempted if the target is above the origin in memory.

- FreeGuard, Scudo, HWASAN

  - Sanitizers that rely on randomization give different detection rates for each run.

## License

MSET is distributed under the Apache License, Version 2.0; refer to LICENSE for
details.

