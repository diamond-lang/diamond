# diamond

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

The only dependencies that must be installed are Clang and Python. And generally Python is already installed on most Linux distros.

## Building on Linux and macOS

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

## Building on Windows

Support for Windows have being removed for the time being. It was too much work to keep the Windows version working.