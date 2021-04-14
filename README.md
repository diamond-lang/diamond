# diamond

A programming language that aims to make programming easier.

## Dependencies

diamond uses the following dependencies:
- meson
- llvm-12
- clang-12

On ubuntu meson can be installed using:
```
sudo apt-get install meson
```
And llvm-12 and clang-12 following the instructions on this [link](https://apt.llvm.org/).

If you installed llvm following the link above the following is necessary.
- Create a symbolic link `/usr/bin/llvm-config` pointing to `/usr/bin/llvm-config-12`
- Create a symbolic link `/usr/include/llvm` pointing to `/usr/include/llvm-12/llvm`
- Create a symbolic link `/usr/include/llvm-c` pointing to `/usr/include/llvm-12/llvm-c`

## Building

```
meson builddir
cd builddir
ninja
```

## Usage
```
./diamond path_to_source_file
```
