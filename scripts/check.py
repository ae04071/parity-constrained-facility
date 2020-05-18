#!/usr/bin/env python3

import os
import sys

NAMES = ("output-impl3", "output")

def read_obj(file_path):
    try:
        with open(file_path) as f:
            lines = f.readlines()
            if not lines or len(lines) < 3:
                return "(Format error)"
            if lines[-1].strip() == "":
                lines = lines[:-1]
            if len(lines) < 3:
                return "(Format error)"
            return lines[-3].strip()
    except IOError:
        return "(IOError)"

def check_category(root, category, names):
    croot = os.path.join(root, category)
    for i in os.listdir(croot):
        dname = os.path.join(croot, i)
        check_dirs([os.path.join(dname, j) for j in names])

def check_dirs(dirs):
    for i in os.listdir(dirs[0]):
        obj = read_obj(os.path.join(dirs[0], i))
        for j in dirs[1:]:
            cur_obj = read_obj(os.path.join(j, i))
            if cur_obj != obj:
                print(os.path.join(j, i), cur_obj, "!=", obj)

def main(exe, root, category, *args):
    check_category(root, category, args or NAMES)

if __name__ == "__main__":
    main(*sys.argv)