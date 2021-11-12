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
	'src/parser.cpp',
	'src/semantic.cpp',
	'src/codegen.cpp',
	'src/errors.cpp',
	'src/utilities.cpp',
	'src/types.cpp'
]
cpp_version = 'c++17'

# Getters (Platform specific)
# ---------------------------
def get_name():
	if   platform.system() == 'Linux': return name
	elif platform.system() == 'Windows': return name + ".exe"
	else: assert False

def get_source_files():
	if   platform.system() == 'Linux': return source_files
	elif platform.system() == 'Windows': return list(map(lambda file: file.replace('/', '\\'), source_files))
	else: assert False

def get_compiler():
	if   platform.system() == 'Linux': return 'c++'
	elif platform.system() == 'Windows': return 'cl'
	else: assert False

def get_cpp_version():
	if   platform.system() == 'Linux': return '-std=' + cpp_version
	elif platform.system() == 'Windows': return '/std:' + cpp_version
	else: assert False

def get_object_file_extension():
	if   platform.system() == 'Linux': return '.o'
	elif platform.system() == 'Windows': return '.obj'
	else: assert False

def get_flags_to_make_object_file():
	if   platform.system() == 'Linux': return '-c -o '
	elif platform.system() == 'Windows': return '/Fo'
	else: assert False

def get_llvm_include_path(llvm_path):
	if   platform.system() == 'Linux': return llvm_path + '/include'
	elif platform.system() == 'Windows': return f'"{llvm_path}\\include"'
	else: assert False

def get_lld_libraries():
	if   platform.system() == 'Linux': return '-llldDriver -llldCore -llldELF -llldCommon'
	elif platform.system() == 'Windows': return 'lldDriver.lib lldCore.lib lldCOFF.lib lldCommon.lib'
	else: assert False

# Builders
# --------
def build_object_file(source_file, llvm_path):
	source_file_o = os.path.basename(source_file).split('.')[0] + get_object_file_extension()
	source_file_o = os.path.join('cache', source_file_o)

	if not os.path.exists(source_file_o) or os.path.getmtime(source_file) > os.path.getmtime(source_file_o):
		command = f'{get_compiler()} {get_cpp_version()} {source_file} {get_flags_to_make_object_file()}{source_file_o} -I {get_llvm_include_path(llvm_path)}'
		print(command)
		output = os.popen(command).read()

def build_object_files(llvm_path):
	# Make cache dir if not exists
	if not os.path.exists('cache'):
		os.mkdir('cache')

	num_cores = multiprocessing.cpu_count()
	pool = multiprocessing.Pool(num_cores)
	_ = pool.map(partial(build_object_file, llvm_path=llvm_path), get_source_files())

def buid_on_linux():
	llvm_path = 'deps/llvm'
	if len(sys.argv) > 1:
		llvm_path = sys.argv[1]

	# Get llvm libs
	command = f'{llvm_path}/bin/llvm-config --libs --link-static'
	llvm_libs = os.popen(command).read()
	llvm_libs = llvm_libs.strip()

	# Build object files
	build_object_files(llvm_path)
	objects_files = list(map(lambda file: os.path.join('cache', file), os.listdir('cache')))
	objects_files = ' '.join(objects_files)

	# Build diamond
	command = f'{get_compiler()} {get_cpp_version()} {objects_files} -o {name} -L{llvm_path}/lib {get_lld_libraries()} {llvm_libs} -lrt -ldl -lpthread -lm -ltinfo'
	print("Linking...")
	output = os.popen(command).read()
	print(output)

def build_on_windows():
	llvm_path = 'deps\\llvm'
	if len(sys.argv) > 1:
		llvm_path = sys.argv[1]

	# Build object files
	build_object_files(llvm_path)
	objects_files = list(map(lambda file: 'cache\\' + file, os.listdir('cache')))
	objects_files = ' '.join(objects_files)

	# Get libs
	command = f'"{llvm_path}\\bin\\llvm-config.exe" --libs'
	output = os.popen(command).read()
	output = output.split('C:\\')
	output = list(map(lambda lib: 'C:\\' + lib, output))
	output = list(map(lambda lib: os.path.basename(lib).strip(), output))
	libs = ' '.join(output)

	# Build diamond
	command = f'cl {objects_files} {get_cpp_version()} /Fe:{get_name()} /link /LIBPATH:"{llvm_path}\lib" {libs} {get_lld_libraries()}'
	print("Linking...")
	output = os.popen(command).read()
	print(output)

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

	return False

def main():
	if need_to_recompile():
		if platform.system() == 'Linux':
			buid_on_linux()

		elif platform.system() == 'Windows':
			build_on_windows()

	else:
		print("Nothing to do...")

if __name__ == "__main__":
	main()
