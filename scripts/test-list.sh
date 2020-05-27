#!/bin/bash

(
  category=700-700
  output_name=output-impl1-p
  command=(cmake-build-release/PCFL --impl1)

  in_root="../exam-data/res/$category"
  out_root="res/$category"

  dirs=("$in_root/121" "$in_root/201" "$in_root/210" "$in_root/211")

  for in_dir in "${dirs[@]}"; do
    name=`basename "$in_dir"`
    echo "==== $name ===="
    echo "$name" > /dev/tty
    scripts/test.py --command "${command[@]}" \; \
      --input-dir "$in_dir/input" \
      --sol-dir "$out_root/$name/output-impl3" \
      --output-dir "$out_root/$name/$output_name" \
      --abs-tol 1e-6 --rel-tol 1e-10 \
      --timeout 1200 \
       || exit $?

  done
) > res/results/result1.txt