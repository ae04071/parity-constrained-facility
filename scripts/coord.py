#!/usr/bin/env python3

import os
import sys
import math
from functools import reduce

def input_lines():
    lines = []
    while True:
        try:
            line = input()
        except EOFError:
            break
        lines.append(line)
    return lines

def read_all(lines):
    data = {}
    ret = []
    method_names = []

    state = 0
    cur_method = ""
    cur_dir = ""
    first = True
    for line in lines:
        if state == 0:
            name = line.strip("=").strip()
            cur_method = name
            method_names.append(name)
            state = 1
        elif state in (1, 2):
            if line.startswith("="):
                if state == 2:
                    name = cur_dir
                    cur_method = name
                    method_names.append(name)
                    first = False
                name = line.strip("=").strip()
                cur_dir = name
                state = 2
            else:
                if state == 2:
                    data.setdefault(cur_dir, {})
                    if first:
                        ret.append((cur_dir, []))
                name, time = split_line(line)
                data[cur_dir].setdefault(name, {})
                data[cur_dir][name][cur_method] = time
                if first:
                    ret[-1][1].append((name, []))
                state = 1

    for i, reti in ret:
        for j, retj in reti:
            for k in method_names:
                retj.append(data[i][j][k])
    return ret

def speedup(time, def_time):
    return def_time / time
def speeddown(time, def_time):
    return time / def_time

def print_directory(data, case_func, csv):
    for i, datai in data:
        if not csv:
            print(f"==== {i} ====")
        for j, dataj in datai:
            fullname = f"{i}/{j}"
            print(f"""{fullname if csv else j}{", " if csv else ": "}{case_func(dataj)}""")

def print_speedup(data, speedup_func=speedup, csv=False):
    print_directory(data, lambda d: ", ".join(str(speedup_func(time, d[0])) for time in d[1:]), csv)

def print_plain(data, csv=False):
    print_directory(data, lambda d: ", ".join(map(str, d)), csv)

def split_line(line):
    splt = line.split(": ")
    name = ": ".join(splt[:-1])
    try:
        time = float(splt[-1].split()[0])
    except ValueError:
        time = math.nan
    return name, time

def map_all(f, data):
    def fi(x):
        i, datai = x
        return i, list(map(fj, datai))
    def fj(x):
        j, dataj = x
        return j, list(map(f, dataj))
    return map(fi, data)

def main(exe, *args):
    data = read_all(input_lines())
    for a in args:
        if a == "--replace-inf" or a.startswith("--replace-inf="):
            inf_replacement = 120.0
            if '=' in a:
                splt = a.split('=')
                if len(splt) != 2:
                    raise RuntimeError
                inf_replacement = float(splt[1])
            data = list(map_all(lambda x: inf_replacement if math.isinf(x) else x, data))

    csv = "--csv" in args
    if "--speed-up" in args:
        print_speedup(data, csv=csv)
    elif "--speed-down" in args:
        print_speedup(data, speeddown, csv=csv)
    else:
        print_plain(data, csv=csv)

if __name__ == "__main__":
    main(*sys.argv)