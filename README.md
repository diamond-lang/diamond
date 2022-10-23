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
```

## Dependencies

diamond uses the following dependencies:
- LLD 12
- LLVM 12
- Python 3 (for build.py and test.py)
- clang (for compiling)

The only dependency that must be installed on Ubuntu is clang. This can be achieved with the following command:
```
sudo apt-get install clang
```

On the other hand, on Windows, you have to download and install [Build Tools fo Visual Studio 2022](https://visualstudio.microsoft.com/downloads/). On the installation you should mark "C++ Clang tools for Windows" under "Desktop Development with C++". And also you should install [Python 3](https://www.python.org/).

## Building on Ubuntu

To get LLVM you can run `get_llvm.py`. This script download LLVM prebuilt binaries, extract them and save them in `deps` inside `llvm`.

This script download LLVM from
[https://github.com/llvm/llvm-project/releases](https://github.com/llvm/llvm-project/releases).

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
