#!/bin/bash
out_dir=res/test
if [ -n "$1" ]; then
  out_dir="$1"
fi
mkdir -p "$out_dir"
scripts/test.py --input-dir res/exam-input/ --sol-dir res/exam-sol/ --command cmake-build-debug/pcfl-jiho-1 --threads 2 > "$out_dir"/correct.txt
scripts/test.py --input-dir res/exam-input/ --sol-dir res/exam-output/ --abs-tol 0 --rel-tol 1e-5 --command cmake-build-debug/pcfl-jiho-1 --threads 2 > "$out_dir"/incorrect.txt
scripts/test.py --input-dir res/exam-input/ --sol-dir res/exam-sol/ --jobs 4 --command cmake-build-debug/pcfl-jiho-1 --threads 1 > "$out_dir"/j4t1.txt
scripts/test.py --input-dir res/exam-input/ --sol-dir res/exam-sol/ --jobs 4 --command cmake-build-debug/pcfl-jiho-1 --threads 2 > "$out_dir"/j4t2.txt
scripts/test.py --input-dir res/exam-input/ --sol-dir res/exam-sol/ --jobs 2 --command cmake-build-debug/pcfl-jiho-1 --threads 2 > "$out_dir"/j2t2.txt
scripts/test.py --input-dir res/exam-input/ --sol-dir res/exam-sol/ --jobs 1 --command cmake-build-debug/pcfl-jiho-1 --threads 1 > "$out_dir"/t1.txt
scripts/test.py --input-dir res/exam-input/ --sol-dir res/exam-sol/ --jobs 1 --command cmake-build-debug/pcfl-jiho-1 --threads 2 > "$out_dir"/t2.txt
scripts/test.py --input-dir res/exam-input/ --sol-dir res/exam-sol/ --jobs 1 --command cmake-build-debug/pcfl-jiho-1 --threads 3 > "$out_dir"/t3.txt
scripts/test.py --input-dir res/exam-input/ --sol-dir res/exam-sol/ --jobs 1 --command cmake-build-debug/pcfl-jiho-1 --threads 4 > "$out_dir"/t4.txt
scripts/test.py --input-dir res/exam-input/ --sol-dir res/exam-sol/ --jobs 1 --command cmake-build-debug/pcfl-jiho-1 --threads 5 > "$out_dir"/t5.txt
scripts/test.py --input-dir res/exam-input/ --sol-dir res/exam-sol/ --jobs 1 --command cmake-build-debug/pcfl-jiho-1 --threads 6 > "$out_dir"/t6.txt
scripts/test.py --input-dir res/exam-input/ --sol-dir res/exam-sol/ --jobs 1 --command cmake-build-debug/pcfl-jiho-1 --threads 7 > "$out_dir"/t7.txt
scripts/test.py --input-dir res/exam-input/ --sol-dir res/exam-sol/ --jobs 1 --command cmake-build-debug/pcfl-jiho-1 --threads 8 > "$out_dir"/t8.txt
