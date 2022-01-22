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

Also you need to have a system capable of compiling C++.

On Ubuntu this can be achieved with the following command:
```
sudo apt-get install build-essential
```

And on Windows you have to download [Build Tools fo Visual Studio 2019](https://visualstudio.microsoft.com/downloads/) and install. On the installation you should mark "Desktop Development with C++".

## Building on Ubuntu

To get LLVM you can run `get_llvm.py`. This script download LLVM prebuilt binaries, extract them and save them in `deps` inside `llvm`.

This script download LLVM from
[https://github.com/llvm/llvm-project/releases](https://github.com/llvm/llvm-project/releases).

Then to build diamond you can use `build.py`. This script assumes there is a `llvm` folder inside `deps` with the prebuilt binaries.

So to install llvm you would run:
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

To get LLVM you can run `get_llvm.py`. This script download LLVM prebuilt binaries, extract them and save them in `deps/` inside `llvm/`.

This script downloads a prebuilt version of LLVM provided by [Zig](https://ziglang.org/). So far is the easiest way I have found to use LLVM on Windows without having to built it from scratch.
- [llvm+clang+lld-12.0.1-rc1-x86_64-windows-msvc-release-mt.tar.xz](https://ziglang.org/deps/llvm%2bclang%2blld-12.0.1-rc1-x86_64-windows-msvc-release-mt.tar.xz)

Then to build diamond you can use `build.py`. This script assumes there is a `llvm` folder inside `deps` with the prebuilt binaries.

So to install llvm you would run:
```
.\get_llvm.py
```

And to build diamond:
```
.\build.py
```

Optionally you can pass to `build.py` a path to a `llvm-config` binary. This way you
can use a LLVM installation build from source.
