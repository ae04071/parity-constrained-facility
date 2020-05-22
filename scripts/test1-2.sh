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
  cat "res/results/impl3/$category.txt" || exit $?

  echo "impl3-p" > /dev/tty
  echo "==== impl3-p ===="
  scripts/test1.sh "$category" output-impl3-p "${command[@]}" --impl3 || exit $?
) > "$result_file" || exit $?

(
  res_dir=res/results/impl3-p
  mkdir -p "$res_dir"
  prefix="$res_dir/$category"
#  raw="${prefix}_raw.txt"
  fix="${prefix}.txt"
  csv="${prefix}.csv"
  incorrect="${prefix}_incorrect_list.txt"
  scripts/check.py res "$category" output-impl3 output-impl3-p | tee "$incorrect"
#  cp "$result_file" "$raw" || exit $?
  cp "$result_file" "$fix" || exit $?
#  scripts/recalc_result.py --replace-inf=1200 < "$raw" > "$fix" || exit $?
  scripts/coord.py --csv < "$fix" > "$csv" || exit $?
)
