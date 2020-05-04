#!/bin/bash

exe=cmake-build-debug/PCFL
category=300-300

(
  echo "default" > /dev/tty
  echo "==== default ===="
  scripts/test1.sh "$category" output "$exe" --verbose || exit $?

  echo "lazy-parity" > /dev/tty
  echo "==== lazy-parity ===="
  scripts/test1.sh "$category" output-lp "$exe" --verbose --lazy-parity || exit $?

  echo "lazy-open" > /dev/tty
  echo "==== lazy-open ===="
  scripts/test1.sh "$category" output-lo "$exe" --verbose --lazy-open || exit $?

  echo "lazy-parity-open" > /dev/tty
  echo "==== lazy-parity-open ===="
  scripts/test1.sh "$category" output-lp-lo "$exe" --verbose --lazy-parity --lazy-open || exit $?
) > res/result1.txt
