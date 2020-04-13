#!/usr/bin/env python3
import math
import os
import sys
import time
import subprocess
import traceback
import threading
import queue

def test_dir(command, input_dir, output_dir=None, error_dir=None, sol_dir=None,
        timeout=None, abs_tol=1e-4, rel_tol=1e-4, output_extract=True, jobs=1):
    # DEBUG
    #print(command)
    for d in (output_dir, error_dir):
        if d:
            try:
                os.makedirs(d, exist_ok=True)
            except OSError:
                traceback.print_exc()

    remaining_jobs = threading.Semaphore(jobs)
    result_queue = queue.Queue(jobs)
    threads = []

    file_names = os.listdir(input_dir)

    def p(file_name):
        in_file = os.path.join(input_dir, file_name)
        out_file = output_dir and os.path.join(output_dir, file_name)
        err_file = error_dir and os.path.join(error_dir, file_name)
        sol_file = sol_dir and os.path.join(sol_dir, file_name)
        remaining_jobs.acquire()
        res = test_one(command, in_file, out_file, err_file, sol_file,
                       timeout, abs_tol, rel_tol, output_extract)
        remaining_jobs.release()
        result_queue.put((file_name, ) + res)

    for file_name in file_names:
        th = threading.Thread(target=p, args=(file_name,))
        # DEBUG
        #th.terminal = sys.stdout;

        th.start()
        threads.append(th)

    for _ in file_names:
        yield result_queue.get()

    for th in threads:
        th.join()

def test_one(command, in_file, out_file=None, err_file=None, sol_file=None,
        timeout=None, abs_tol=1e-4, rel_tol=1e-4, output_extract=True):
    io_success = True
    correct = True
    try:
        with open(in_file) as f:
            input_str = f.read()
    except OSError:
        print("in_file: " + in_file, file=sys.stderr)
        traceback.print_exc()
        return False, True, True, math.nan
    return_code, err_str, out_str, wct = run_one(command, input_str, timeout)
    if isinstance(return_code, BaseException):
        try:
            raise return_code
        except subprocess.TimeoutExpired:
            return io_success, True, True, math.inf
        except subprocess.SubprocessError:
            print("in_file: " + in_file, file=sys.stderr)
            traceback.print_exc()
            return io_success, False, True, math.nan
    execute_success = return_code == 0 and not err_str
    out_extracted = extract_output(out_str)
    try:
        if out_file:
            with open(out_file, "w") as f:
                f.write(out_extracted if output_extract else out_str)
    except OSError:
        print("in_file: " + in_file, file=sys.stderr)
        traceback.print_exc()
        io_success = False
    try:
        if err_file:
            with open(err_file, "w") as f:
                f.write(err_str)
    except OSError:
        print("in_file: " + in_file, file=sys.stderr)
        traceback.print_exc()
        io_success = False
    try:
        if sol_file:
            with open(sol_file) as f:
                sol_str = f.readline()
            try:
                out_val = float(out_extracted)
                sol_val = float(sol_str)

                abs_err = abs(out_val - sol_val)
                rel_err = abs(abs_err / sol_val)

                if abs_err > abs_tol and rel_err > rel_tol:
                    correct = False
            except ValueError:
                print("in_file: " + in_file, file=sys.stderr)
                traceback.print_exc()
                correct = False
    except OSError:
        print("in_file: " + in_file, file=sys.stderr)
        traceback.print_exc()
        io_success = False
    return io_success, execute_success, correct, wct

def run_one(command, input_str, timeout=None):
    input_data = input_str.encode(errors="replace")
    try:
        start_time = time.time()
        p = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        try:
            out_data, err_data = p.communicate(input_data, timeout)
        finally:
            p.kill()
        end_time = time.time()
    except subprocess.SubprocessError as e:
        return e, None, None, None
    return_code = p.returncode
    wall_clock_time = end_time - start_time
    err_str = err_data.decode(errors="replace")
    out_str = out_data.decode(errors="replace")
    return return_code, err_str, out_str, wall_clock_time

def extract_output(raw_output):
    lines = raw_output.strip().split("\n")
    if lines:
        return lines[-1]
    else:
        return ""

def mean(__iterable):
    sum_value = 0
    len_value = 0
    for x in __iterable:
        sum_value += x
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

CLI_COMMAND = "CLI_COMMAND"
CLI_INPUT_DIR = "CLI_INPUT_DIR"
CLI_OUTPUT_DIR = "CLI_OUTPUT_DIR"
CLI_ERROR_DIR = "CLI_ERROR_DIR"
CLI_SOL_DIR = "CLI_SOL_DIR"
CLI_TIMEOUT = "CLI_TIMEOUT"
CLI_ABSOLUTE_ERROR_TOLERANCE = "CLI_ABSOLUTE_ERROR_TOLERANCE"
CLI_RELATIVE_ERROR_TOLERANCE = "CLI_RELATIVE_ERROR_TOLERANCE"
CLI_OUTPUT_EXTRACT = "CLI_OUTPUT_EXTRACT"
CLI_JOBS = "CLI_PARALLEL"
CLI_OPTIONS = (
    CLI_COMMAND, CLI_INPUT_DIR, CLI_OUTPUT_DIR, CLI_ERROR_DIR, CLI_SOL_DIR,
    CLI_TIMEOUT, CLI_ABSOLUTE_ERROR_TOLERANCE, CLI_RELATIVE_ERROR_TOLERANCE,
    CLI_OUTPUT_EXTRACT, CLI_JOBS
)

def cli_parse(argv):
    def option_handler_list(key, f=lambda x: x):
        value = []
        option_name = None
        def init(ret, state, arg):
            nonlocal option_name
            option_name = arg
            return False, handler
        def handler(ret, state, arg):
            if arg is not None:
                value.append(handle_error(f, arg, len(value), option_name))
            else:
                ret[key] = tuple(value)
            return False, handler
        return init

    def option_handler_tuple(key, fs):
        value = []
        i = iter(fs)
        option_name = None
        def init(ret, state, arg):
            nonlocal option_name
            option_name = arg
            return False, handler
        def handler(ret, state, arg):
            try:
                value.append(handle_error(next(i), arg, len(value), option_name))
                if arg is None:
                    raise RuntimeError("Not enough parameter for " + option_name)
                return False, handler
            except StopIteration:
                ret[key] = tuple(value)
                return True, None
        return init

    def option_handler_const(key, value):
        def init(ret, state, arg):
            ret[key] = value
            return False, None
        return init

    def option_handler_single(key, f=lambda x: x):
        option_name = None
        def init(ret, state, arg):
            nonlocal option_name
            option_name = arg
            return False, handler
        def handler(ret, state, arg):
            ret[key] = handle_error(f, arg, 0, option_name)
            return False, None
        return init

    def options_failure(ret, state, arg):
        raise RuntimeError("Invalid option: " + arg)

    def handle_error(f, x, i, opt):
        try:
            return f(x)
        except ValueError:
            raise RuntimeError(
                "Invalid value for parameter {nth} of {opt}: {val}".format(
                    nth=i + 1, opt=opt, val=x
                ))

    options = {
        None: lambda _: (False, None),
        "--command": option_handler_list(CLI_COMMAND),
        "--input-dir": option_handler_single(CLI_INPUT_DIR),
        "--output-dir": option_handler_single(CLI_OUTPUT_DIR),
        "--sol-dir": option_handler_single(CLI_SOL_DIR),
        "--timeout": option_handler_single(CLI_TIMEOUT, float),
        "--abs-tol": option_handler_single(CLI_ABSOLUTE_ERROR_TOLERANCE, float),
        "--rel-tol": option_handler_single(CLI_RELATIVE_ERROR_TOLERANCE, float),
        "--output-all": option_handler_const(CLI_OUTPUT_EXTRACT, False),
        "--output-extract": option_handler_const(CLI_OUTPUT_EXTRACT, True),
        "--jobs": option_handler_single(CLI_JOBS, int),
    }
    # ret = {
    #     CLI_COMMAND: ("cmake-build-debug/pcfl-jiho-1",),
    #     CLI_INPUT_DIR: "res/exam-input",
    #     CLI_OUTPUT_DIR: "res/exam-output3",
    #     CLI_SOL_DIR: "res/exam-sol",
    # }
    ret = dict((x, None) for x in CLI_OPTIONS)
    ret[CLI_ABSOLUTE_ERROR_TOLERANCE] = 1e-4
    ret[CLI_RELATIVE_ERROR_TOLERANCE] = 1e-4
    ret[CLI_OUTPUT_EXTRACT] = True
    ret[CLI_JOBS] = 1
    state = (False, None)
    for arg in argv[1:] + (None, ):
        while True:
            if state[1]:
                state = state[1](ret, state, arg)
            else:
                state = options.get(arg, options_failure)(ret, state, arg)
            if not state[0]:
                break

    if not ret[CLI_COMMAND] or not ret[CLI_INPUT_DIR]:
        raise RuntimeError("Lacks necessary options")

    return ret

def main(*argv):
    options = cli_parse(argv)
    success_time_list = []
    correct_time_list = []
    io_error_list = []
    runtime_error_list = []
    incorrect_list = []
    print(options[CLI_COMMAND])
    for file_name, io_success, execute_success, correct, wct in test_dir(
            command=options[CLI_COMMAND],
            input_dir=options[CLI_INPUT_DIR],
            output_dir=options[CLI_OUTPUT_DIR],
            error_dir=options[CLI_ERROR_DIR],
            sol_dir=options[CLI_SOL_DIR],
            timeout=options[CLI_TIMEOUT],
            abs_tol=options[CLI_ABSOLUTE_ERROR_TOLERANCE],
            rel_tol=options[CLI_RELATIVE_ERROR_TOLERANCE],
            output_extract=options[CLI_OUTPUT_EXTRACT],
            jobs=options[CLI_JOBS],
        ):
        x = []
        x.append(str(wct))
        if not io_success:
            x.append("IO_ERROR")
        if not execute_success:
            x.append("RUNTIME_ERROR")
        if not correct:
            x.append("INCORRECT")
        print(file_name + ": " + " ".join(x))

        if io_success and execute_success and correct:
            correct_time_list.append(wct)
        if io_success and execute_success:
            success_time_list.append(wct)
        if not io_success:
            io_error_list.append(file_name)
        if not execute_success:
            runtime_error_list.append(file_name)
        if not correct:
            incorrect_list.append(file_name)

        sys.stdout.flush()

    if io_error_list:
        print("IO_ERROR:")
        for f in io_error_list:
            print("  {}".format(f))
    if runtime_error_list:
        print("RUNTIME_ERROR:")
        for f in runtime_error_list:
            print("  {}".format(f))
    if incorrect_list:
        print("INCORRECT:")
        for f in incorrect_list:
            print("  {}".format(f))
    print("Avg time:", mean(correct_time_list))
    print("Med time:", median(correct_time_list))
    print("Max time:", max(correct_time_list + [0]))
    return 0

if __name__ == "__main__":
    sys.exit(main(*sys.argv))
