#!/usr/bin/env python3
import os
import platform

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

def build_object_files(compiler, compiler_flags, llvm_path):
	if not os.path.exists('cache'):
		os.mkdir('cache')

	for i in range(len(source_files)):
		source_file = source_files[i]
		source_file_o = 'cache/' + source_file.split('/')[-1].split('.')[0] + '.o'

		if not os.path.exists(source_file_o) or os.path.getmtime(source_file) > os.path.getmtime(source_file_o):
			command = f'{compiler} {compiler_flags} {source_file} -c -o {source_file_o} -I {llvm_path}/include'
			print(command)
			output = os.popen(command).read()

def buid_on_linux():
	compiler = 'c++'
	llvm_path = '/usr/lib/llvm-12'
	compiler_flags = f'-std={cpp_version}'

	# Get llvm libs
	command = f'{llvm_path}/bin/llvm-config --libs --link-static'
	llvm_libs = os.popen(command).read()
	llvm_libs = llvm_libs.strip()

	# Build object files
	build_object_files(compiler, compiler_flags, llvm_path)
	objects_files = list(map(lambda file: 'cache/' + file, os.listdir('cache')))
	objects_files = ' '.join(objects_files)

	# Build diamond
	command = f'{compiler} {compiler_flags} {objects_files} -o {name} -I{llvm_path}/include -L{llvm_path}/lib {llvm_libs} -lrt -ldl -lpthread -lm -lz -ltinfo'
	print("Linking...")
	output = os.popen(command).read()
	print(output)

def build_on_windows():
	# Get libs
	command = '"C:\\Program Files\\LLVM\\bin\\llvm-config.exe" --libs'
	output = os.popen(command).read()
	output = output.split(' ')
	output = list(map(lambda i: output[i] + ' ' + output[i + 1], range(0, len(output) - 1, 2)))
	libs = list(map(lambda lib: '"' + lib + '"', output))

	# Get cpp flags
	command = '"C:\\Program Files\\LLVM\\bin\\llvm-config.exe" --cxxflags'
	output = os.popen(command).read()
	output = output.split(' ')
	cpp_flags = output

	# Get system libs
	command = '"C:\\Program Files\\LLVM\\bin\\llvm-config.exe" --system-libs'
	output = os.popen(command).read()
	output = output.split(' ')
	system_libs = output


	command = 'cl src/main.cpp src/parser.cpp src/semantic.cpp src/codegen.cpp src/errors.cpp src/utilities.cpp src/types.cpp /std:c++17 /I "C:\Program Files\LLVM/include" /Fe:diamond.exe /link ' + ' '.join(libs)
	print(command)
	output = os.popen(command).read()
	print(output)

def need_to_recompile():
	if not os.path.exists(name):
		return True

	files = os.listdir('src')
	for i in range(len(files)):
		if os.path.getmtime('src/' + files[i]) > os.path.getmtime(name):
			return True

	return False

if need_to_recompile():
	if platform.system() == 'Linux':
		buid_on_linux()

	elif platform.system() == 'Windows':
		build_on_windows()

else:
	print("Nothing to do...")
