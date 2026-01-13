# Tests

Tests are run with CTest. After building:

```sh
ctest --test-dir build -C Release
```

For the full QA harness (CTest + audio regression + quality gates):

```sh
./scripts/run_ci_tests.sh
```

Use `TEST_CONFIG=Debug` to target Debug builds when needed.
