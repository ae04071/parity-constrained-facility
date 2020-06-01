#!/bin/bash

exe=cmake-build-release/PCFL
categories=(300-300 500-500 700-700)
name="$1"
command=("$exe" --validate --verbose "${@:2}")

if [ -z "$name" ]; then
  echo "too few arguments" >&2
  exit 1
fi

res_dir="res/results/$name"
log_file="pcfl.log"

mkdir -p "$res_dir" || exit $?

for category in "${categories[@]}"; do
  prefix="$res_dir/$category"
  raw="$prefix.txt"

  echo "$category" > /dev/tty
  (
    echo "==== $name ===="
    scripts/test1.sh "$category" "output-$name" "${command[@]}" || exit $?
  ) > "$raw" || exit $?
done

mv "$log_file" "$res_dir/$log_file"
