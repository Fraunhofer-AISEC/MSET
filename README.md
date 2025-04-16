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

For more information, please refer to the SP '25 conference paper:

> E. Vintila, P. Zieris and J. Horsch, "Evaluating the Effectiveness of Memory
> Safety Sanitizers," in 2025 IEEE Symposium on Security and Privacy (SP), San
> Francisco, CA, USA, 2025, pp. 88-88, doi:
> [10.1109/SP61157.2025.00088](https://doi.ieeecomputersociety.org/10.1109/SP61157.2025.00088).

Please note that this is a prototype implementation.

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

```xml
<setup>
  <compile_cmd>clang -fsanitize=address $SOURCE_FILE -o $GENERATED_BINARY</compile_cmd>
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

## License

MSET is distributed under the Apache License, Version 2.0; refer to LICENSE for
details.

