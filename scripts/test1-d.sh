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
  echo "==== default ===="
  scripts/test1.sh "$category" output "${command[@]}" --impl1 || exit $?
) > "$result_file" || exit $?

(
  prefix="res/results/default/$category"
  raw="${prefix}_raw.txt"
  fix="${prefix}.txt"
  incorrect="${prefix}_incorrect_list.txt"
  scripts/check.py res "$category" output output-impl3 | tee "$incorrect"
  cp "$result_file" "$raw" || exit $?
  scripts/recalc_result.py --replace-inf=1200 < "$raw" > "$fix" || exit $?
)
