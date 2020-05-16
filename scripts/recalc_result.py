#!/usr/bin/env python3

import os
import sys
import math
from functools import reduce

inf_replacement = 120.0

def recalc_all():
    IGNORE_START = ("  ", "INCORRECT:", "IO_ERROR:", "Avg time: ", "Med time: ", "Max time: ")
    lst = []
    while True:
        try:
            line = input()
        except EOFError:
            break
        if ".plc:" in line:
            splt = line.split(": ")
            name = ": ".join(splt[:-1])
            time = float(splt[-1].split()[0])
            lst.append(time)
            line = name + ": " + splt[-1].split()[0]
        else:
            if lst:
                recalc(lst)
                lst.clear()
        if not reduce(lambda x, y: x or line.startswith(y), (IGNORE_START), False):
            print(line)

def recalc(lst):
    print("Avg time:", mean(lst))
    print("Med time:", median(lst))
    print("Max time:", max(lst + [0]))

def mean(__iterable):
    sum_value = 0
    len_value = 0
    for x in __iterable:
        sum_value += replace_inf(x)
        len_value += 1
    return sum_value and sum_value / len_value

def median(__iterable):
    sorted_list = sorted(__iterable)
    n = len(sorted_list)
    if not sorted_list:
        return 0
    elif n % 2 == 1:
        return sorted_list[n // 2]
    else:
        return mean(sorted_list[n // 2 - 1 : n // 2 + 1])

def replace_inf(x):
    if math.isinf(x):
        return inf_replacement
    return x

def main(exe, *args):
    recalc_all()

if __name__ == "__main__":
    main(*sys.argv)