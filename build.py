#!/usr/bin/env python3
import os
import platform
import sys
import multiprocessing
from functools import partial

# Configuration
# -------------
name = 'diamond'
source_files = [
    'src/main.cpp',
    'src/errors.cpp',
    'src/tokens.cpp',
    'src/lexer.cpp',
    'src/ast.cpp',
    'src/utilities.cpp',
    'src/parser.cpp',
    'src/semantic/intrinsics.cpp',
    'src/semantic/semantic.cpp',
    'src/semantic/type_inference.cpp',
    'src/codegen.cpp'
]

# Platform specific constants
# ---------------------------
def get_name():
    if   platform.system() == 'Linux': return name
    elif platform.system() == 'Windows': return name + ".exe"
    else: assert False

def get_object_file_extension():
    if   platform.system() == 'Linux': return '.o'
    elif platform.system() == 'Windows': return '.obj'
    else: assert False

def get_default_llvm_config_path():
    if   platform.system() == 'Linux': return 'deps/llvm/bin/llvm-config'
    elif platform.system() == 'Windows': return 'deps\\llvm\\bin\\llvm-config.exe'
    else: assert False

def get_lld_libraries():
    if   platform.system() == 'Linux': return '-llldDriver -llldCore -llldELF -llldCommon'
    elif platform.system() == 'Windows': return '-llldDriver -llldCore -llldCOFF -llldCommon'
    else: assert False

# Helper functions
# ----------------
def need_to_recompile():
    if not os.path.exists(get_name()):
        return True

    if not os.path.exists('cache'):
        return True

    # for file in src
    for file in os.listdir('src'):
        # Check associated object file exists
        if not os.path.exists(os.path.join('cache', file.split('.')[0] + get_object_file_extension())):
            return True

    # Check if source file was edited after creation of diamond executable
    if os.path.getmtime(os.path.join('src', file)) > os.path.getmtime(get_name()):
        return True

# Build
# -----
def build_object_file(source_file, llvm_config):
    # Create file name for output object file
    object_file = os.path.basename(source_file).split('.')[0] + get_object_file_extension()
    object_file = os.path.join('cache', object_file)

    # Get llvm include path
    command =f'"{llvm_config}" --includedir'
    llvm_include_path = os.popen(command).read().strip()

    # If object file should be builded or rebuilded
    if not os.path.exists(object_file) or os.path.getmtime(source_file) > os.path.getmtime(object_file):
        # Build
        command = f'clang++ -std=c++17 -g {source_file} -c -o {object_file} -I {llvm_include_path}'
        print(command)
        os.popen(command).read()

def build_object_files(llvm_config):
    # Make cache dir if not exists
    if not os.path.exists('cache'):
        os.mkdir('cache')

    num_cores = multiprocessing.cpu_count()
    pool = multiprocessing.Pool(num_cores)
    _ = pool.map(partial(build_object_file, llvm_config=llvm_config), source_files)

def build():
    llvm_config = get_default_llvm_config_path()
    if len(sys.argv) > 1:
        llvm_config = sys.argv[1]

    # Check llvm-config exists
    if not os.path.exists(llvm_config):
        print(f'Couldn\'t found llvm-config in {os.path.dirname(llvm_config)} :(')
        return

    # Build object files
    build_object_files(llvm_config)
    objects_files = list(map(lambda file: os.path.join('cache', file), os.listdir('cache')))
    objects_files = ' '.join(objects_files)

     # Get llvm libs
    command = f'{llvm_config} --libs --link-static'
    llvm_libs = os.popen(command).read()
    llvm_libs = llvm_libs.strip()

    if platform.system() == 'Windows':
        llvm_libs = llvm_libs.split(" ")
        llvm_libs = [lib.split("\\")[-1] for lib in llvm_libs]
        llvm_libs = ["-l" + lib.split(".")[0] for lib in llvm_libs]
        llvm_libs = " ".join(llvm_libs)

    # Get libs path
    command = f'{llvm_config} --link-static --ldflags'
    libpath = os.popen(command).read().strip()

    if platform.system() == "Linux":
        libpath = libpath.split("-L")[1]

    elif platform.system() == 'Windows':
        libpath = libpath.split("-LIBPATH:")[1]

    # Get system libs
    command = f'{llvm_config} --link-static --system-libs'
    system_libs = os.popen(command).read().strip()

    if platform.system() == 'Windows':
        system_libs = system_libs.split(" ")
        system_libs = ["-l" + lib.split(".")[0] for lib in system_libs]
        system_libs = " ".join(system_libs)

    # Build diamond
    command = f'clang++ -std=c++17 {objects_files} -o {get_name()} -L {libpath} {get_lld_libraries()} {llvm_libs} {system_libs}'
    print("Linking...")
    output = os.popen(command).read()
    if output != "":
        print(output)

# Main
# ----
def main():
    if need_to_recompile():
        build()

    else:
        print("Nothing to do...")

if __name__ == "__main__":
    main()
