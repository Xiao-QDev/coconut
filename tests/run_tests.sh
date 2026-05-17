#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

PICO=./pico.exe
PASS=0
FAIL=0

run_test() {
    local file="$1"
    local name
    name=$(basename "$file")
    if output=$("$PICO" run "$file" 2>&1); then
        if echo "$output" | grep -q "^FAIL:"; then
            echo "FAIL $name"
            echo "$output" | grep "^FAIL:"
            FAIL=$((FAIL + 1))
        else
            echo "PASS $name"
            PASS=$((PASS + 1))
        fi
    else
        echo "ERROR $name (exit code $?)"
        echo "$output"
        FAIL=$((FAIL + 1))
    fi
}

for f in tests/test_types.pico tests/test_stdlib.pico tests/test_oop.pico tests/test_closures.pico; do
    run_test "$f"
done

# test_threads: skip on WASM, run otherwise
if [ "${WASM:-0}" != "1" ]; then
    run_test tests/test_threads.pico
fi

echo ""
echo "Results: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
