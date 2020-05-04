#!/bin/bash
in_dir=input
out_dir="$2"
sol_dir="$3"
root_dir="$1"

if [ -z "$out_dir" ]; then
  out_dir=output
fi
if [ -z "$sol_dir" ]; then
  sol_dir=solution
fi

command="cmake-build-debug/PCFL --threads 2 \
  --open-prior 2 --assign-prior 1 --parity-prior 0 \;"

for sub_dir in "$root_dir"/*; do
  echo "====== $sub_dir ======"
  options=()
  options+=(--input-dir "$sub_dir/$in_dir")
  if [ -d "$sub_dir/$sol_dir" ]; then
    options+=(--sol-dir "$sub_dir/$sol_dir")
  fi
  options+=(--output-dir "$sub_dir/$out_dir")
  options+=(--abs-tol 1e-6 --rel-tol 1e-10)
  options+=(--timeout 120)
  options+=(--command $command)
  scripts/test.py "${options[@]}" || exit $?
done
