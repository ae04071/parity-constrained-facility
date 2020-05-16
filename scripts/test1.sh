#!/bin/bash
#scripts/test.py --input-dir res/300-300/111/input \
#  --sol-dir res/300-300/111/solution \
#  --output-dir res/300-300/111/output \
#  --abs-tol 1e-6 --rel-tol 1e-10 \
#  --command cmake-build-debug/PCFL --threads 2 \
#    --open-prior 2 --assign-prior 1 --parity-prior 0

category=$1
output_name=$2
command=("${@:3}")

in_root="../exam-data/res/$category"
out_root="res/$category"

for in_dir in $in_root/*; do
  name=`basename "$in_dir"`
  echo "==== $name ===="
  echo "$name" > /dev/tty
  scripts/test.py --command "${command[@]}" \; \
    --input-dir "$in_dir/input" \
    --sol-dir "$in_dir/output" \
    --output-dir "$out_root/$name/$output_name" \
    --abs-tol 1e-6 --rel-tol 1e-10 \
    --timeout 1200 \
     || exit $?

done