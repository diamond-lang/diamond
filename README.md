# diamond

A programming language that aims to make programming easier.

## Dependencies

diamond uses the following dependencies:
- meson
- llvm-12

On ubuntu meson can be installed using:
```
sudo apt-get install meson
```
And llvm-12 and following the instructions on this [link](https://apt.llvm.org/).

Namely adding llvm software repository, then adding the public key, updating the package repository, and finally installing the llvm with the following command:

```
sudo apt-get install llvm-12
```

If you installed llvm following the steps above is also necessary to create a symbolic link from `/usr/bin/llvm-config-12` to `/usr/local/bin/llvm-config`.

## Building
```
meson builddir
cd builddir
ninja
```

## Installing

To install:
```
sudo ninja install
```

To uninstall:
```
sudo ninja uninstall
```

## Usage
```
diamond [program file]      # Compiles the program
diamond run [program file]  # Runs the program
```
