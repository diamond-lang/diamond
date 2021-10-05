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

## Building on ubuntu

On ubuntu llvm-12 can be installed following the instructions on this [link](https://apt.llvm.org/).

Namely adding the llvm software repository, then adding the public key, updating the package repository, and finally installing llvm and lld:

```
sudo apt-get install llvm-12-dev lib-lld-12-dev
```
Also is necessary to install python package joblib. Is needed to parallelize the building and save time. This can be done:

```
pip3 install joblib
```

Finally to actually build diamond:

```
./build.py /usr/bin/llvm-12
```

(Where `/usr/bin/llvm-12` is the path where llvm is installed, by default the script assumes llvm is installed in `/usr/bin/llvm`)

## Building on Windows

The easiest way to obtain llvm and lld on windows is to download a prebuilt version of llvm.

In this case we are going to use one provided by [Zig](https://ziglang.org/).

- [llvm+clang+lld-12.0.1-rc1-x86_64-windows-msvc-release-mt.tar.xz](https://ziglang.org/deps/llvm%2bclang%2blld-12.0.1-rc1-x86_64-windows-msvc-release-mt.tar.xz)

Then install joblib:
```
pip install joblib
```

And finally to actually build diamond:
```
.\build.py path\to\extracted\llvm\after\download
```

The command could by something like this:
```
.\build.py C:\Users\alonso\Downloads\llvm+clang+lld-12.0.1-rc1-x86_64-windows-msvc-release-mt
```
