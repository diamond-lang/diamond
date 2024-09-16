# diamond

[![Build and test](https://github.com/diamond-lang/diamond/actions/workflows/build-and-test.yaml/badge.svg)](https://github.com/diamond-lang/diamond/actions/workflows/build-and-test.yaml)

A programming language that aims to make programming easier.

## Usage
```
diamond build [program file]
    Creates a native executable from the program.

diamond run [program file]
    Runs the program.

diamond emit [options] [program file]
    This command emits intermediary representations of
    the program. Is useful for debugging the compiler.

    The options are:
        --tokens
        --ast
        --ast-with-types
        --ast-with-concrete-types
        --llvm-ir
        --obj
```

## Dependencies

diamond uses the following dependencies:
- LLD 15
- LLVM 15
- Python 3 (for build.py and test.py)
- clang (for compiling)

The only dependencies that must be installed are Clang and Python.

### Windows

To install Clang on Windows you must install Visual Studio, follow this [steps](https://learn.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-170), and when choosing components also select Clang.

### macOs

Also, on macOS at least the prebuilt version of LLVM needs [zstd](https://formulae.brew.sh/formula/zstd) to be installed.

## Building

To get LLVM you can run `get_llvm.py`. This script download LLVM prebuilt binaries, extract them and save them in `deps` inside `llvm`.

Then to build diamond you can use `build.py`.

So to get llvm you would run:
```
./get_llvm.py
```

And to build diamond:
```
./build.py
```

Optionally you can pass to `build.py` a path to a `llvm-config` binary. This way you
can use a LLVM installation build from source, or a LLVM installation installed with
a package manager.

### Building on Windows

To have Clang availaible in the PATH you must run the commands on a *x64 Native Tools Developer Command Prompt for VS 2022* command prompt.

The you can run commands:
```
python get_llvm.py
```

And to build diamond:
```
python build.py
```
