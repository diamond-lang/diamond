#!/usr/bin/env python3
import os
import platform
import sys
import multiprocessing
import functools
import subprocess
import shutil

# Configuration
# -------------
name = 'diamond'
c_compiler = 'clang -std=c99'
cpp_compiler = 'clang++ -std=c++17'
debug_compiler_args = '-g -fsanitize=address,undefined,null -fomit-frame-pointer -Wall -Werror -Wswitch-enum'
release_compiler_args = '-Wall -Werror -Wswitch-enum -o3'
release_debug_compiler_args = '-Wall -Werror -Wswitch-enum -g3'

# Platform specific constants
# ---------------------------
def get_name():
    if   platform.system() == 'Linux': return name
    if   platform.system() == 'Darwin': return name
    elif platform.system() == 'Windows': return name + '.exe'
    else: assert False

def get_object_file_extension():
    if   platform.system() == 'Linux': return '.o'
    if   platform.system() == 'Darwin': return '.o'
    elif platform.system() == 'Windows': return '.obj'
    else: assert False

def get_default_llvm_config_path():
    if   platform.system() == 'Linux': return 'deps/llvm/bin/llvm-config'
    if   platform.system() == 'Darwin': return 'deps/llvm/bin/llvm-config'
    elif platform.system() == 'Windows': return 'deps/llvm/bin/llvm-config'
    else: assert False

def get_lld_libraries():
    if   platform.system() == 'Linux': return '-llldELF -llldCommon'
    if   platform.system() == 'Darwin': return '-llldMachO -llldCommon'
    elif platform.system() == 'Windows': return '-llldCOFF -llldCommon'
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


def get_source_files():
    source_files = [os.path.join('src', f) for f in os.listdir('src') if f.endswith('.c') or f.endswith('.cpp')]
    return source_files

def get_compiler_args():
    compiler_args = debug_compiler_args
    if len(sys.argv) > 1:
        assert len(sys.argv) <= 2
        assert sys.argv[1] in ['debug', 'release', 'releaseDebug']
        if sys.argv[1] == 'release':
            compiler_args = release_compiler_args
        elif sys.argv[1] == 'releaseDebug':
            compiler_args = release_debug_compiler_args 
    return compiler_args

# Build
# -----
def build_object_file(source_file, llvm_include_path):
    # Create file name for output object file
    object_file = os.path.basename(source_file).split('.')[0] + get_object_file_extension()
    object_file = os.path.join('cache', object_file)

    # If object file should be builded or rebuilded
    if not os.path.exists(object_file) or os.path.getmtime(source_file) > os.path.getmtime(object_file):
        compiler = c_compiler
        if source_file.endswith('.cpp'):
            compiler = cpp_compiler
        compiler_args = get_compiler_args()

        # Build
        command = f'{compiler} {compiler_args} {source_file} -c -o {object_file} -I {llvm_include_path}'
        print(command)

        result = subprocess.run(command.split(' '), capture_output=True, text=True)
        if result.returncode == 0:
            return True
        else:
            print(result.stderr)
            return False

def build_object_files(llvm_config):
    # Source files
    source_files = get_source_files()

    # Make cache dir if not exists
    if not os.path.exists('cache'):
        os.mkdir('cache')

    # Get llvm include path
    command = f'{llvm_config} --includedir'
    llvm_include_path = subprocess.run(command.split(' '), capture_output=True, text=True).stdout.strip()

    # Build object files in parallalel
    num_cores = multiprocessing.cpu_count()
    with multiprocessing.Pool(num_cores) as pool:
        results = pool.map(functools.partial(build_object_file, llvm_include_path=llvm_include_path), source_files)

        for result in results:
            if result == False: sys.exit(1)

def build():
    llvm_config = get_default_llvm_config_path()
    if platform.system() == 'Windows':
        llvm_config += '.exe'

    # Check llvm-config exists
    if not os.path.exists(llvm_config):
        print(f'Couldn\'t found llvm-config in {os.path.dirname(llvm_config)} :(')
        sys.exit(1)

    # Build object files
    build_object_files(llvm_config)
    objects_files = list(map(lambda file: os.path.join('cache', file), os.listdir('cache')))
    objects_files = ' '.join(objects_files)

     # Get llvm libs
    command = f'{llvm_config} --libs --link-static'
    llvm_libs = subprocess.run(command.split(' '), capture_output=True, text=True).stdout.strip()
    llvm_libs = llvm_libs.strip()

    if platform.system() == 'Windows':
        llvm_libs = llvm_libs.split(' ')
        llvm_libs = [lib.split('\\')[-1] for lib in llvm_libs]
        llvm_libs = ['-l' + lib.split('.')[0] for lib in llvm_libs]
        llvm_libs = ' '.join(llvm_libs)

    # Get libs path
    command = f'{llvm_config} --link-static --ldflags'
    libpath = subprocess.run(command.split(' '), capture_output=True, text=True).stdout.strip()

    if platform.system() == 'Linux' or platform.system() == 'Darwin':
        libpath = libpath.split('-L')[1]

    elif platform.system() == 'Windows':
        libpath = libpath.split('-LIBPATH:')[1]

    # Get system libs
    command = f'{llvm_config} --link-static --system-libs'
    system_libs = subprocess.run(command.split(' '), capture_output=True, text=True).stdout.strip()

    if platform.system() == 'Darwin':
        system_libs = '-L/opt/homebrew/lib ' + system_libs

    elif platform.system() == 'Windows':
        system_libs = system_libs.split(' ')
        system_libs = ['-l' + lib.split('.')[0] for lib in system_libs]
        system_libs = ' '.join(system_libs)

    # Build diamond
    compiler_args = get_compiler_args()
    command = f'{cpp_compiler} {compiler_args} {objects_files} -o {get_name()} -L {libpath} {get_lld_libraries()} {llvm_libs} {system_libs}'
    print('Linking...')
    result = subprocess.run(command.split(' '), capture_output=True, text=True)
    if result.returncode == 0:
        return True
    else:
        print(result.stderr)
        sys.exit(1)

# Main
# ----
def main():
    if len(sys.argv) > 1:
        if os.path.exists('cache'): 
            shutil.rmtree('cache')
            os.mkdir('cache')
        build()
    else:
        if need_to_recompile():
            build()

        else:
            print('Nothing to do...')

if __name__ == '__main__':
    main()
