#!/usr/bin/env python3
import os
import platform
import re
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
	'src/semantic/type_inference.cpp'
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
	if   platform.system() == 'Linux': return '-I ' + llvm_path
	elif platform.system() == 'Windows': return '/I' + llvm_path
	else: assert False

def get_lld_libraries():
	if   platform.system() == 'Linux': return '-llldDriver -llldCore -llldELF -llldCommon'
	elif platform.system() == 'Windows': return 'lldDriver.lib lldCore.lib lldCOFF.lib lldCommon.lib'
	else: assert False

# Builders
# --------
def build_object_file(source_file, llvm_config):
	source_file_o = os.path.basename(source_file).split('.')[0] + get_object_file_extension()
	source_file_o = os.path.join('cache', source_file_o)

	# Get llvm include path
	command =f'"{llvm_config}" --includedir'
	llvm_include_path = os.popen(command).read().strip()
	llvm_include_path = get_llvm_include_path(llvm_include_path)

	if not os.path.exists(source_file_o) or os.path.getmtime(source_file) > os.path.getmtime(source_file_o):
		command = f'{get_compiler()} {get_cpp_version()} {source_file} {get_flags_to_make_object_file()}{source_file_o} {llvm_include_path}'
		print(command)
		output = os.popen(command).read()

def build_object_files(llvm_config):
	# Make cache dir if not exists
	if not os.path.exists('cache'):
		os.mkdir('cache')

	num_cores = multiprocessing.cpu_count()
	pool = multiprocessing.Pool(num_cores)
	_ = pool.map(partial(build_object_file, llvm_config=llvm_config), get_source_files())

def buid_on_linux():
	llvm_config = 'deps/llvm/bin/llvm-config'
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

	# Get libs path
	command = f'{llvm_config} --link-static --ldflags'
	libpath = os.popen(command).read().strip()

	# Get system libs
	command = f'{llvm_config} --link-static --system-libs'
	system_libs = os.popen(command).read().strip()

	# Build diamond
	command = f'{get_compiler()} {get_cpp_version()} {objects_files} -o {name} {libpath} {get_lld_libraries()} {llvm_libs} {system_libs}'
	print("Linking...")
	output = os.popen(command).read()
	if output != "":
		print(output)

def build_on_windows():
	llvm_config = 'deps\\llvm\\bin\\llvm-config.exe'
	if len(sys.argv) > 1:
		llvm_config = sys.argv[1]

	# Check llvm-config exists
	if not os.path.exists(llvm_config):
		print(f'Couldn\'t found llvm-config in {os.path.dirname(llvm_config)} :(')
		return

	# Build object files
	build_object_files(llvm_config)
	objects_files = list(map(lambda file: 'cache\\' + file, os.listdir('cache')))
	objects_files = ' '.join(objects_files)

	# Get llvm libs
	command = f'"{llvm_config}" --libs'
	output = os.popen(command).read()
	output = re.split(r'(C:\\|c:\\)', output)
	output = list(map(lambda lib: 'C:\\' + lib, output))
	output = list(map(lambda lib: os.path.basename(lib).strip(), output))
	libs = ' '.join(output)

	# Get libs path
	command = f'"{llvm_config}" --ldflags'
	libpath = os.popen(command).read().strip()

	# Build diamond
	command = f'cl {objects_files} {get_cpp_version()} /Fe:{get_name()} /link {libpath} {libs} {get_lld_libraries()}'
	print("Linking...")
	output = os.popen(command).read()
	if output != "":
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
