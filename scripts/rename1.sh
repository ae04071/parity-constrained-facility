#!/bin/bash
root=res
l1_list=(300-300 500-500 700-700)

#src_name=output
#func() { echo "$(dirname "$1")/output-default"; }

src_name=output-impl3
func() {
  l1="$(basename "$(dirname "$(dirname "$1")")")"
  l2="$(basename "$(dirname "$1")")"
  echo "../exam-data/res/$l1/$l2/output"
}

for l1_name in "${l1_list[@]}"; do
  l1_path="$root/$l1_name"
  for l2_path in "$l1_path"/*; do
    src_path="$l2_path/$src_name"
    dst_path="$(func "$src_path")"
    cp -TR --remove-destination "$src_path" "$dst_path"
  done
done
