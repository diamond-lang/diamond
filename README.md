# diamond

A programming language that aims to make programming easier.

## Usage
```
diamond [program file]      # Compiles a program
diamond run [program file]  # Runs a program
```

## Dependencies

diamond uses the following dependencies:
- LLD 12
- LLVM 12
- Python 3 (for build.py and test.py)

## Building on ubuntu

On ubuntu LLVM 12 can be installed following the instructions on this [link](https://apt.llvm.org/).

Namely adding the LLVM software repository, then adding the public key, updating the package repository, and finally installing LLVM and LLD:

```
sudo apt-get install llvm-12-dev liblld-12-dev
```

Then to build diamond:

```
./build.py /usr/bin/llvm-12
```

(Where `/usr/bin/llvm-12` is the path where llvm is installed, by default the script assumes llvm is installed in `/usr/bin/llvm`)

## Building on Windows

The easiest way to obtain LLVM and LLD on windows is to download a prebuilt version of LLVM.

In this case we are going to use one provided by [Zig](https://ziglang.org/).

Download the following package and then extract it:

- [llvm+clang+lld-12.0.1-rc1-x86_64-windows-msvc-release-mt.tar.xz](https://ziglang.org/deps/llvm%2bclang%2blld-12.0.1-rc1-x86_64-windows-msvc-release-mt.tar.xz)

Then to build diamond:
```
.\build.py path\to\extracted\llvm\after\download
```

The command should by something like this:
```
.\build.py C:\Users\alonso\Downloads\llvm+clang+lld-12.0.1-rc1-x86_64-windows-msvc-release-mt
```
