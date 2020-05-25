#!/bin/bash
root=res/results
methods=(default impl3 impl3-1 impl3-p)
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
) | tee "$root/tmp.csv" \
#  | xsel --clipboard
  > /dev/null
