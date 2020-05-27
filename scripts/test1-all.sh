#!/bin/bash

exe=cmake-build-release/PCFL
categories=(300-300 500-500 700-700)
name="$1"
command=("$exe" --validate --verbose "${@:2}")

if [ -z "$name" ]; then
  echo "too few arguments" >&2
  exit 1
fi

result_file="res/results/result1.txt"

for category in "${categories[@]}"; do
  echo "$category" > /dev/tty
  (
    echo "==== $name ===="
    scripts/test1.sh "$category" "output-$name" "${command[@]}" || exit $?
  ) > "$result_file" || exit $?

  (
    res_dir="res/results/$name"
    prefix="$res_dir/$category"
    fix="${prefix}.txt"

    mkdir -p "$res_dir" || exit $?
    cp "$result_file" "$fix" || exit $?
  )
done
