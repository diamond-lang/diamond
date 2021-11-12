#!/usr/bin/env python3
import os
import io
import sys
import platform
from urllib import request
import tarfile
import time

def get_url():
	if   platform.system() == 'Linux': return 'https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.1/clang+llvm-12.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz'
	elif platform.system() == 'Windows': return 'https://ziglang.org/deps/llvm%2bclang%2blld-12.0.1-rc1-x86_64-windows-msvc-release-mt.tar.xz'
	else: assert False

def get_local_file_name():
	if   platform.system() == 'Linux': return 'clang+llvm-12.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz'
	elif platform.system() == 'Windows': return 'llvm%2bclang%2blld-12.0.1-rc1-x86_64-windows-msvc-release-mt.tar.xz'
	else: assert False

# Create deps directory if not exists
if not os.path.exists('deps'):
	os.mkdir('deps')

# Retrieve file
response = request.urlopen(get_url())
length = int(response.getheader('content-length'))
buffer = io.BytesIO()
while True:
	aux = response.read(4096)
	if not aux:
		print()
		break

	buffer.write(aux)
	percent = int((len(buffer.getvalue()) / length) * 100)
	print(f'downloading LLVM... {percent}%', end='\r')

# Write to disk
with open(os.path.join('deps', get_local_file_name()), "wb") as f:
	f.write(buffer.getbuffer())

# Extract file
def track_progress(members):
	start = time.time()
	n = 2
	for member in members:
		elapsed = time.time() - start
		if elapsed > 0.5:
			n += 1
			end = time.time()
			print(f'extracting LLVM' + (n * '.') + '  ', end='\r')
			n = n % 3
			start = time.time()
		yield member

print('\rextracting LLVM.', end='\r')
tarball = tarfile.open(os.path.join('deps', get_local_file_name()), 'r')
tarball.extractall('deps', members = track_progress(tarball))
print('\rextracting LLVM...')


# Delete downloaded file
os.remove(os.path.join('deps', get_local_file_name()))

# Rename LLVM folder-
os.rename(os.path.join('deps', os.listdir('deps')[0]), os.path.join('deps', 'llvm'))
