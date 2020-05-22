#!/bin/bash

exe=cmake-build-release/PCFL
category=300-300
if [ -n "$1" ]; then
  category="$1"
fi
command=("$exe" --verbose --validate)
#command=("$exe")

result_file="res/results/result1.txt"

(
  echo "default" > /dev/tty
#  echo "==== default ===="
#  scripts/test1.sh "$category" output "${command[@]}" --impl1 || exit $?
  cat "res/results/default/$category.txt" || exit $?

  echo "impl3" > /dev/tty
  echo "==== impl3 ===="
  scripts/test1.sh "$category" output-impl3-1 "${command[@]}" --impl3 --impl3-no-parity || exit $?
) > "$result_file" || exit $?

(
  prefix="res/results/impl3/$category"
  raw="${prefix}_raw.txt"
  fix="${prefix}.txt"
  csv="${prefix}.csv"
  incorrect="${prefix}_incorrect_list.txt"
  scripts/check.py res "$category" output output-impl3 | tee "$incorrect"
  cp "$result_file" "$raw" || exit $?
  scripts/recalc_result.py --replace-inf=1200 < "$raw" > "$fix" || exit $?
  scripts/coord.py --csv < "$fix" > "$csv" || exit $?
)
