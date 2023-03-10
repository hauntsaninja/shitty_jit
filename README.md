# shitty_jit

A quick introduction to the incredibly powerful [PEP 523 APIs](https://peps.python.org/pep-0523/).

The frame evaluation API is intended as a hook for JIT compilers. It's what
[PyTorch 2.0](https://pytorch.org/get-started/pytorch-2.0/) uses for its magical new
`torch.compile` feature.

But today, we're going to use it to...

## Turn addition into subtraction

```python
from shitty_jit import *

def simple_addition():
    a = 1
    b = 2
    print(f"{a + b = }")

print("Calling simple_addition normally...")
simple_addition()

print("Setting our PEP 523 frame evaluator...")
turn_interpreted_addition_into_subtraction()

print("Calling simple_addition again...")
simple_addition()
print("and again (note the code object is cached, so we won't hit our frame evaluator)...")
simple_addition()

print("Unsetting our PEP 523 frame evaluator...")
reset_shitty_jit()

print("Calling simple_addition one last time...")
simple_addition()
```

This will output:
```text
Calling simple_addition normally...
a + b = 3
Setting our PEP 523 frame evaluator...
Calling simple_addition again...
Hello from inside our frame evaluator! (compiling simple_addition)
a + b = -1
and again (note the code object is cached, so we won't hit our frame evaluator)...
a + b = -1
Unsetting our PEP 523 frame evaluator...
Hello from inside our frame evaluator! (compiling reset_shitty_jit)
Calling simple_addition one last time...
a + b = 3
```

## Caveats

- The CPython interpreter does constant folding when compiling to bytecode, so if you try and
  add two constants, the operation will be evaluated by the compiler, not the bytecode interpreter.
  This means that `turn_interpreted_addition_into_subtraction` will not have an opportunity to
  change the addition into subtraction.
- This is just a toy (and a dangerous one at that)! If you're looking for a real life use of this
  API, check out [TorchDynamo](https://github.com/pytorch/pytorch/tree/master/torch/_dynamo).
- This code currently only works on CPython 3.9 and 3.10.
