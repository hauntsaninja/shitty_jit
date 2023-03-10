from setuptools import Extension, find_packages, setup

setup(
    name="shitty_jit",
    version="0.1",
    packages=find_packages(),
    ext_modules=[
        Extension(
            "shitty_jit._shitty_eval", ["shitty_jit/shitty_eval.c"], extra_compile_args=["-Wall"]
        )
    ],
    python_requires=">=3.9,<3.11",
)
