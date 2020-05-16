import math

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
