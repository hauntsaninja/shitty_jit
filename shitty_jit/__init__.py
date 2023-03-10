import dis

from . import _shitty_eval


def reset_shitty_jit():
    _shitty_eval.set_eval_frame_func(None)


def turn_interpreted_addition_into_subtraction():
    _shitty_eval.set_eval_frame_func(_addition_into_subtraction_frame_evaluator)


def _addition_into_subtraction_frame_evaluator(frame):
    print("Hello from inside our frame evaluator!")

    # Modifying bytecode can be much more complicated this. Bytecode is also considered an
    # implementation detail of CPython, so this stuff is quite susceptible to being broken.
    code = frame.f_code
    instructions = dis.get_instructions(code)
    new_code = []
    for i in instructions:
        if i.opname == "BINARY_ADD":
            new_code.append(dis.opmap["BINARY_SUBTRACT"])
        else:
            new_code.append(i.opcode)
        new_code.append((i.arg or 0) & 0xFF)

    return code.replace(co_code=bytes(new_code))
