#!/bin/bash

exe=cmake-build-release/PCFL
category=300-300
if [ -n "$1" ]; then
  category="$1"
fi
command=("$exe" --verbose --validate)

(
  echo "default" > /dev/tty
  echo "==== default ===="
  scripts/test1.sh "$category" output "${command[@]}" --impl1 || exit $?

  echo "impl3" > /dev/tty
  echo "==== impl3 ===="
  scripts/test1.sh "$category" output-impl3 "${command[@]}" --impl3 || exit $?
) > res/results/result1.txt
