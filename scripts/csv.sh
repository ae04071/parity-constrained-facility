#!/bin/bash
root=res/results
methods=("${@:1}")
categories=(300-300 500-500 700-700)

(
  for m in "${methods[@]}"; do
    echo -n ",1200"
  done
  for m in "${methods[@]}"; do
    echo -n ",$m"
  done
  echo
  for c in "${categories[@]}"; do
    for m in "${methods[@]}"; do
      cat "$root/$m/$c.txt"
    done | scripts/coord.py --csv
  done
) > "$root/tmp.csv"
#  | xsel --clipboard
