#!/bin/bash

cd "`dirname "$0"`/.." || exit 1

# config
exe=cmake-build-debug/pcfl-jiho-1
input_dir=res/exam-input
output_dir=res/exam-output3
sol_dir=res/exam-sol

(
  for input_file in "$input_dir"/*; do
    file_name=`basename "$input_file"`
    output_file="$output_dir/$file_name"
    sol_file="$sol_dir/$file_name"
    time (
      "$exe" < "$input_file" | tail -n1 | \
        tr -d '[:space:]' | tee $output_file
      echo
      cat "$sol_file"
      echo
    ) 2>&1
  done
) > "$output_dir/result.txt"
