#!/bin/bash

input_dir="res/exam-input"
output_dir="res/exam-output2"
solution_dir="res/exam-output"
executable="cmake-build-debug/pcfl-jiho-1"
result_file="$output_dir/result.txt"

for input_file in $input_dir/*; do
  file_name=`basename $input_file`
  echo "Testing $file_name"
  output_file="$output_dir/$file_name"
  sol_file="$solution_dir/$file_name"
  (time ( $executable < $input_file | tail -n1 | tr -d '[:space:]' | tee $output_file
   echo; cat $sol_file; echo;)) 2>&1
  echo
done | tee $result_file
