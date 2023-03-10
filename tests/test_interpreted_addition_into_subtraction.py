import shitty_jit


def addition(a, b):
    return a + b


def test_demo():
    assert addition(1, 2) == 3

    shitty_jit.turn_interpreted_addition_into_subtraction()
    assert addition(1, 2) == -1

    shitty_jit.reset_shitty_jit()
    assert addition(1, 2) == 3
