#!/bin/bash

input_dir="res/barrier/input"
output_dir="res/barrier/output"
executable="cmake-build-debug/pcfl-jiho-1"
result_file="$output_dir/result.txt"

mkdir -p $output_dir

for input_file in $input_dir/*; do
  file_name=`basename "$input_file"`
  echo "Testing $file_name"
  output_file_0="$output_dir/$file_name.0"
  output_file_1="$output_dir/$file_name.1"
  output_file_diff="$output_dir/$file_name.diff"

  (time ( "$executable" --verbose < "$input_file" > "$output_file_0")) 2>&1
  echo
  (time ( "$executable" --verbose --barrier < "$input_file" > "$output_file_1")) 2>&1
  echo
  diff "$output_file_0" "$output_file_1" | tee "$output_file_diff"
#done | tee "$result_file"
done > "$result_file"
