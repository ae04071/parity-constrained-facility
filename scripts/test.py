#!/usr/bin/env python3
import math
import os
import sys
import time
import subprocess
import traceback
import threading
import queue
import json

GRB_CORRECT = ("2",)
GRB_IMPOSSIBLE = ("3", "4", "5")
GRB_INCORRECT = ("7", "8", "9", "10", "15")


class PCFLTester:

    def __init__(self, command):
        self.command = command

        self.timeout = None
        self.abs_tol = 1e-4
        self.rel_tol = 1e-4
        self.output_extracted = True
        self.gurobi_time = False

    def test_list(self, tuples, jobs=1):
        """
        run test_one for each element of the list
        :param tuples: iterable of tuple (name, input file, output file, error file, solution file)
        :param jobs: number of jobs
        :return: iterator which will run the test
        """

        list_tuples = list(tuples)

        def p(e):
            file_name, in_file, out_file, err_file, sol_file = e
            return (file_name,) + self.test_one(in_file, out_file, err_file, sol_file)

        if jobs == 1:
            for e in list_tuples:
                yield p(e)
        else:
            remaining_jobs = threading.Semaphore(jobs)
            result_queue = queue.Queue(jobs)
            threads = []

            def thread_start(e):
                remaining_jobs.acquire()
                res = p(e)
                remaining_jobs.release()
                result_queue.put(res)

            for e in list_tuples:
                th = threading.Thread(target=thread_start, args=(e,))
                th.daemon = True
                th.start()
                threads.append(th)

            for _ in list_tuples:
                yield result_queue.get()

            for th in threads:
                th.join()

    def test_dir(
            self, in_dir, out_dir=None, err_dir=None,
            sol_dir=None, jobs=1):
        for d in (out_dir, err_dir):
            if d:
                try:
                    os.makedirs(d, exist_ok=True)
                except OSError:
                    traceback.print_exc()

        file_names = filename_sorted(os.listdir(in_dir))

        def p(file_name):
            in_file = os.path.join(in_dir, file_name)
            out_file = out_dir and os.path.join(out_dir, file_name)
            err_file = err_dir and os.path.join(err_dir, file_name)
            sol_file = sol_dir and os.path.join(sol_dir, file_name)
            return file_name, in_file, out_file, err_file, sol_file

        tuples = (p(f) for f in file_names)

        return self.test_list(tuples, jobs)

    def test_one(self, in_file, out_file=None, err_file=None, sol_file=None):
        io_error = False
        incorrect = False

        # read input
        try:
            with open(in_file) as f:
                input_str = f.read()
        except OSError:
            print("in_file: " + in_file, file=sys.stderr)
            traceback.print_exc()
            return False, True, True, math.nan

        # run program
        return_code, err_str, out_str, wct = self.__run_one(input_str)

        # interpret return code or exception
        # if return_code == -2: # Exit by SIGINT
        #     raise KeyboardInterrupt
        if isinstance(return_code, BaseException):
            try:
                raise return_code
            except subprocess.TimeoutExpired:
                return io_error, False, False, math.inf
            except subprocess.SubprocessError:
                print("in_file: " + in_file, file=sys.stderr)
                traceback.print_exc()
                return io_error, True, False, math.nan
        exception = return_code != 0 or err_str != ""

        # extract values from output string
        out_extracted, out_cost, out_time, out_status = extract_output(out_str)

        # write stdout to file
        try:
            if out_file:
                with open(out_file, "w") as f:
                    # print(self.output_extracted)
                    f.write(out_extracted if self.output_extracted
                            else out_str)
        except OSError:
            print("in_file: " + in_file, file=sys.stderr)
            traceback.print_exc()
            io_error = True

        # write stderr to file
        if err_str:
            try:
                if err_file:
                    with open(err_file, "w") as f:
                        f.write(err_str)
                else:  # if err_file is not specified, print to stderr
                    print("in_file: " + in_file, file=sys.stderr)
                    print(err_str, end="", file=sys.stderr)
            except OSError:
                print("in_file: " + in_file, file=sys.stderr)
                traceback.print_exc()
                io_error = True

        # interpret status
        if out_status in GRB_CORRECT:
            pass
        elif out_status in GRB_IMPOSSIBLE:
            exception = True
            print("in_file: " + in_file, file=sys.stderr)
            print(f"IMPOSSIBLE: {out_status}", file=sys.stderr)
        elif out_status in GRB_INCORRECT:
            incorrect = True
        else:
            exception = True
            print("in_file: " + in_file, file=sys.stderr)
            print(f"Unexpected status: {out_status}", file=sys.stderr)

        # select time
        if self.gurobi_time:
            if out_time >= 0:
                wct = out_time
            else:
                exception = True
                print("in_file: " + in_file, file=sys.stderr)
                print(f"Running time: {out_time}", file=sys.stderr)

        # compare objective value
        try:
            if sol_file:
                with open(sol_file) as f:
                    sol_str = f.read()

                _, sol_cost, _, _ = extract_output(sol_str)

                if out_cost == out_cost and sol_cost == sol_cost:
                    abs_err = abs(out_cost - sol_cost)
                    rel_err = abs(abs_err / sol_cost)

                    if abs_err > self.abs_tol and rel_err > self.rel_tol:
                        incorrect = True
                else:
                    incorrect = True
        except OSError:
            print("in_file: " + in_file, file=sys.stderr)
            traceback.print_exc()
            io_error = True

        return io_error, exception, incorrect, wct

    def __run_one(self, input_str):
        input_data = input_str.encode(errors="replace")
        try:
            start_time = time.time()
            p = subprocess.Popen(self.command, stdin=subprocess.PIPE,
                                 stderr=subprocess.PIPE, stdout=subprocess.PIPE)
            try:
                out_data, err_data = p.communicate(input_data, self.timeout)
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
    """
    raw_output will look like:
    ...non commercial only...
    ...
    ...many lines of garbage outputs...
    ...
    COST
    RUNNING TIME
    STATUS
    """
    lines = raw_output.strip().split("\n")
    if len(lines) >= 3:
        try:
            cost = float(lines[-3].strip())
        except ValueError:
            cost = math.nan
        try:
            running_time = float(lines[-2])
        except ValueError:
            running_time = math.nan
        status = lines[-1].strip()
        return "\n".join(lines[-3:]), cost, running_time, status
    else:
        return "", math.nan, math.nan, "(output error)"


def filename_sorted(__iterable):
    def char_class(c):
        if c.isdigit():
            return 2
        return 1

    def emit(buf, state):
        x = "".join(buf)
        if state == 2:
            return int(x), x
        else:
            return 0 if x < "0" else math.inf, x

    def restore(x):
        return x[1]

    def split(x):
        def p():
            buf = []
            state = 0
            for c in x:
                cls = char_class(c)
                if state == 0:
                    state = cls
                elif state == 1 or state != cls:
                    yield emit(buf, state)
                    state = cls
                    buf.clear()
                buf.append(c)
            yield emit(buf, state)

        return tuple(p())

    return ["".join(map(restore, x)) for x in sorted(map(split, __iterable))]


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
        return mean(sorted_list[n // 2 - 1: n // 2 + 1])


def replace_inf(inf_replacement, x):
    if math.isinf(x):
        return inf_replacement
    return x


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
CLI_USE_GUROBI_RUNTIME = "CLI_USE_GUROBI_RUNTIME"
CLI_JSON = "CLI_JSON"

JSON_NAME = "name"
JSON_INPUT = "input"
JSON_OUTPUT = "output"
JSON_ERROR = "error"
JSON_SOLUTION = "solution"


def cli_parse(argv):
    def option_handler_list(key, f=lambda x: x):
        value = []
        option_name = None

        def init(ret, state, arg):
            nonlocal option_name
            option_name = arg
            return False, handler

        def handler(ret, state, arg):
            # TODO: escaping ';' (not able to give ';' as an argument)
            if arg is not None and arg != ";":
                value.append(handle_error(f, arg, len(value), option_name))
                return False, handler
            else:
                ret[key] = tuple(value)
                return False, None

        return init

    def option_handler_tuple(key, fs):
        value = []
        i = iter(fs)
        option_name = "(bug...uninitialized)"

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

    def options_failure_func(ret, state, arg):
        raise RuntimeError("Invalid option: " + arg)

    options_failure = (options_failure_func, None, None)

    def handle_error(f, x, i, opt):
        try:
            return f(x)
        except ValueError:
            raise RuntimeError(
                f"Invalid value for parameter {i + 1} of {opt}: {x}")

    options = {
        None: (lambda _1, _2, _3: (False, None), None, None),
        "--help": (
            lambda _1, _2, _3: print_help(), "",
            "display this help and exit"),
        "--command": (
            option_handler_list(CLI_COMMAND), "<args>... ;",
            "(necessary) specify the command to execute"),
        "--input-dir": (
            option_handler_single(CLI_INPUT_DIR), "<dir>",
            "(necessary) specify the input directory\n"
            "run the tests for the files in the given directory"),
        "--output-dir": (
            option_handler_single(CLI_OUTPUT_DIR), "<dir>",
            "specify the output directory\n"
            "the output of each test will be stored in the corresponding file in this directory"),
        "--sol-dir": (
            option_handler_single(CLI_SOL_DIR), "<dir>",
            "specify the solution directory\n"
            "the output of each test will be compared to the corresponding file in this directory"),
        "--timeout": (
            option_handler_single(CLI_TIMEOUT, float), "<timeout>",
            "specify the timeout for each test\n"
            "each test will stop after the timeout if it is still running and the running time will be inf"),
        "--abs-tol": (
            option_handler_single(CLI_ABSOLUTE_ERROR_TOLERANCE, float), "<x>",
            "specify the absolute error tolerance\n"
            "if the absolute error is not greator than x, the result is considered correct\n"
            "default: 1e-4"),
        "--rel-tol": (
            option_handler_single(CLI_RELATIVE_ERROR_TOLERANCE, float), "<x>",
            "specify the relative error tolerance\n"
            "if the relative error is not greator than x, the result is considered correct\n"
            "default: 1e-4"),
        "--output-all": (
            option_handler_const(CLI_OUTPUT_EXTRACT, False), "",
            "when printing the output to file, print all the outputs including garbage outputs..."),
        "--output-extract": (
            option_handler_const(CLI_OUTPUT_EXTRACT, True), "",
            "when printing the output to file, print only the last three lines\n"
            "default"),
        "--jobs": (
            option_handler_single(CLI_JOBS, int), "<n>",
            "run at most n tests at the same time\n"
            "this options influences the running time of each test\n"
            "default: 1"),
        "--time-gurobi": (
            option_handler_const(CLI_USE_GUROBI_RUNTIME, True), "",
            "take the running time from the output and discard the measured wall clock time"),
        "--time-python": (
            option_handler_const(CLI_USE_GUROBI_RUNTIME, False), "",
            "measure the wall clock time and discard the running time result\n"
            "default"),
        "--json-input": (
            option_handler_const(CLI_JSON, True), "",
            "parse json stdin to use as testcases instead of --input-dir\n"
            f"""stdin format is [{{"{JSON_NAME}": name, "{JSON_INPUT}": input_file, "{JSON_OUTPUT}": output_file, """
            f""""{JSON_ERROR}": error_file, "{JSON_SOLUTION}": solution_file}}...]\n"""
            "output, error, solution are optional"),
    }

    def print_help():
        print(f"Usage: {argv[0]} [OPTION]...")
        print("Options:")
        left_width, align_width, indent = 2, 20, 2
        for k, v in options.items():
            if k is not None:
                indented = v[2].replace(
                    "\n", "\n" + " " * (left_width + align_width + 1 + indent))
                print(f"{'':<{left_width}}{k + ' ' + v[1]:<{align_width}} {indented}")
        exit()

    # ret = dict((x, None) for x in CLI_OPTIONS)
    # ret[CLI_ABSOLUTE_ERROR_TOLERANCE] = 1e-4
    # ret[CLI_RELATIVE_ERROR_TOLERANCE] = 1e-4
    # ret[CLI_OUTPUT_EXTRACT] = True
    # ret[CLI_JOBS] = 1
    # ret[CLI_USE_GUROBI_RUNTIME] = False
    ret = {}
    state = (False, None)
    for arg in argv[1:] + (None,):
        while True:
            if state[1]:
                state = state[1](ret, state, arg)
            else:
                state = options.get(arg, options_failure)[0](ret, state, arg)
            if not state[0]:
                break

    if not ret:
        print_help()
    if not (CLI_COMMAND in ret and (CLI_INPUT_DIR in ret or CLI_JSON in ret)):
        raise RuntimeError("Lacks necessary options")

    return ret


def parse_json(s):
    obj = json.loads(s)
    tuple_keys = (JSON_NAME, JSON_INPUT, JSON_OUTPUT, JSON_ERROR, JSON_SOLUTION)
    def p():
        for x in obj:
            if not (JSON_NAME in x and JSON_INPUT in x):
                raise RuntimeError("JSON format error")
            yield tuple(x.get(k) for k in tuple_keys)
    return list(p())


def main(*argv):
    options = cli_parse(argv)

    tester = PCFLTester(options[CLI_COMMAND])
    tester_attr_map = {
        CLI_TIMEOUT: "timeout", CLI_ABSOLUTE_ERROR_TOLERANCE: "abs_tol",
        CLI_RELATIVE_ERROR_TOLERANCE: "rel_tol",
        CLI_OUTPUT_EXTRACT: "output_extracted",
        CLI_USE_GUROBI_RUNTIME: "gurobi_time",
    }
    for k, v in options.items():
        if k in tester_attr_map:
            setattr(tester, tester_attr_map[k], v)

    # print(dict((k, getattr(tester, k)) for k in tester_attr_map.values()))

    if options[CLI_JSON]:
        args_map = {
            CLI_JOBS: "jobs"
        }
        tuples = parse_json(sys.stdin.read())
        args = dict((args_map[k], v) for (k, v) in options.items() if k in args_map)
        test_it = tester.test_list(tuples=tuples, **args)
    else:
        args_map = {
            CLI_OUTPUT_DIR: "out_dir", CLI_ERROR_DIR: "err_dir",
            CLI_SOL_DIR: "sol_dir", CLI_JOBS: "jobs"
        }
        args = dict((args_map[k], v) for (k, v) in options.items() if k in args_map)
        test_it = tester.test_dir(
            in_dir=options[CLI_INPUT_DIR],
            **args
        )

    success_time_list = []
    correct_time_list = []
    io_error_list = []
    exception_list = []
    incorrect_list = []

    for file_name, io_error, exception, incorrect, wct in test_it:
        x = [str(wct)]
        if io_error:
            x.append("IO_ERROR")
            io_error_list.append(file_name)
        if exception:
            x.append("EXCEPTION")
            exception_list.append(file_name)
        if incorrect:
            x.append("INCORRECT")
            incorrect_list.append(file_name)
        print(file_name + ": " + " ".join(x))

        if not io_error and not exception and not incorrect:
            correct_time_list.append(wct)
        if not io_error and not exception:
            success_time_list.append(wct)

        sys.stdout.flush()

    if io_error_list:
        print("IO_ERROR:")  # error while processing file
        for f in io_error_list:
            print(f"  {f}")
    if exception_list:
        print("EXCEPTION:")  # unexpected results
        for f in exception_list:
            print(f"  {f}")
    if incorrect_list:
        print("INCORRECT:")  # output not equal to solution or status is not optimal
        for f in incorrect_list:
            print(f"  {f}")
    print("Avg time:", mean(map(lambda x: replace_inf(options.get(CLI_TIMEOUT, math.inf), x), correct_time_list)))
    print("Med time:", median(correct_time_list))
    print("Max time:", max(correct_time_list + [0]))
    return 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv))
