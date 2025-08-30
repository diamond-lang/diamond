#!/usr/bin/env python3
import os
import io
import platform
from urllib import request
import tarfile
import time

# Create deps directory if not exists
if not os.path.exists('deps'):
    os.mkdir('deps')

def get_url_llvm():
    if   platform.system() == 'Linux': assert False
    elif platform.system() == 'Darwin': return 'https://github.com//diamond-lang/llvm-builds/releases/download/llvm-21.1.0/llvm+lld-macos-aarch64.tar.gz'
    elif platform.system() == 'Windows': assert False
    else: assert False

def get_local_file_name_llvm():
    if   platform.system() == 'Linux': assert False
    elif platform.system() == 'Darwin': return 'llvm+lld-macos-aarch64.tar.gz'
    elif platform.system() == 'Windows': assert False
    else: assert False

def get_extracted_llvm_name():
    if   platform.system() == 'Linux': assert False
    elif platform.system() == 'Darwin': return 'llvm+lld-macos-aarch64'
    elif platform.system() == 'Windows': assert False
    else: assert False

# Retrieve llvm
response = request.urlopen(get_url_llvm())
length = int(response.getheader('content-length'))
buffer = io.BytesIO()
while True:
    aux = response.read(max(4096, length // 20))
    if not aux:
        print()
        break

    buffer.write(aux)
    percent = int((len(buffer.getvalue()) / length) * 100)
    print(f'downloading LLVM... {percent}%', end='\r')

# Write to disk
file = open(os.path.join('deps', get_local_file_name_llvm()), "wb")
file.write(buffer.getvalue())
file.close()

# Extract file
def track_progress(members):
    start = time.time()
    n = 2
    for member in members:
        elapsed = time.time() - start
        if elapsed > 0.5:
            n += 1
            print(f'extracting LLVM' + (n * '.') + '  ', end='\r')
            n = n % 3
            start = time.time()
        yield member

print('\rextracting LLVM.', end='\r')
tarball = tarfile.open(os.path.join('deps', get_local_file_name_llvm()), 'r')
tarball.extractall('deps', members = track_progress(tarball), filter='data')
tarball.close()
print('\rextracting LLVM...')

# Delete downloaded file
os.remove(os.path.join('deps', get_local_file_name_llvm()))

# Rename LLVM folder-
os.rename(os.path.join('deps', get_extracted_llvm_name()), os.path.join('deps', 'llvm'))