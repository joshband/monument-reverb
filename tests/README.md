# Tests

Tests are run with CTest. The build must be configured with
`-DMONUMENT_ENABLE_TESTS=ON -DBUILD_TESTING=ON` (both are OFF by default,
and `enable_testing()` itself lives inside the `MONUMENT_ENABLE_TESTS`
guard) — otherwise `ctest` silently finds zero tests. After configuring and building:

```sh
cmake -S . -B build -DMONUMENT_ENABLE_TESTS=ON -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build -C Release
```

For the full QA harness (CTest + audio regression + quality gates):

```sh
./scripts/run_ci_tests.sh
```

Use `TEST_CONFIG=Debug` to target Debug builds when needed.
