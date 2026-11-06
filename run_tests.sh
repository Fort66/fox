#!/usr/bin/env bash
# Регрессионный прогон языка Fox.
#   ./run_tests.sh           — прогнать все тесты и сверить с эталонами
#   ./run_tests.sh --update  — перезаписать эталоны текущим выводом
set -u
cd "$(dirname "$0")"

MODE="run"
[ "${1:-}" = "--update" ] && MODE="update"

PASS=0
FAIL=0

echo "== [1/2] Юнит-тест лексера =="
if gcc -std=c11 -Wall -Wextra -Isrc tests/test_lexer.c src/lexer.c -o tests/test_lexer 2>tests/build.log; then
    if ./tests/test_lexer; then
        PASS=$((PASS+1))
    else
        echo "FAIL: юнит-тест лексера"
        FAIL=$((FAIL+1))
    fi
else
    echo "FAIL: test_lexer не собрался (см. tests/build.log)"
    FAIL=$((FAIL+1))
fi

echo "== [2/2] Регрессия примеров =="
make clean > /dev/null
if ! make > /dev/null 2>make.log; then
    echo "FAIL: компилятор не собрался (см. make.log)"
    exit 1
fi

mkdir -p examples/expected

for fox in examples/*.fox; do
    name="$(basename "$fox" .fox)"
    exp="examples/expected/${name}.txt"
    app="examples/${name}_app"

    if ! ./foxc "$fox" > /dev/null 2>"examples/expected/${name}.compile.err"; then
        echo "FAIL: $name — ошибка компиляции (см. examples/expected/${name}.compile.err)"
        FAIL=$((FAIL+1))
        continue
    fi

    out="$( ./"$app" 2>&1 )"

    if [ "$MODE" = "update" ]; then
        printf '%s\n' "$out" > "$exp"
        echo "UPDATE: $name"
        continue
    fi

    if [ ! -f "$exp" ]; then
        echo "SKIP: $name — нет эталона (создай: ./run_tests.sh --update)"
        continue
    fi

    if [ "$out" = "$(cat "$exp")" ]; then
        echo "PASS: $name"
        PASS=$((PASS+1))
    else
        echo "FAIL: $name — вывод отличается от эталона"
        diff <(printf '%s\n' "$out") "$exp" | head -20
        FAIL=$((FAIL+1))
    fi
done

echo
echo "Итог: PASS=$PASS FAIL=$FAIL"
if [ "$FAIL" -eq 0 ]; then
    echo "ВСЁ ЗЕЛЁНОЕ"
else
    echo "ЕСТЬ ПАДЕНИЯ"
fi
[ "$FAIL" -eq 0 ]