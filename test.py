#!/usr/bin/env python3
import os
import re
import sys
import subprocess
import multiprocessing
import functools
import platform

def get_name():
    if   platform.system() == 'Linux': return 'diamond'
    if   platform.system() == 'Darwin': return 'diamond'
    elif platform.system() == 'Windows': return 'diamond' + '.exe'
    else: assert False

def get_command():
    if   platform.system() == 'Linux': return './' + get_name()
    if   platform.system() == 'Darwin': return './' + get_name()
    elif platform.system() == 'Windows': return '.\\' + get_name()
    else: assert False


# Helper functions
# ----------------
def test(file, expected, max_file_path_len):
    print(f"testing {file}", end='')
    if not os.path.exists(file):
        print("File not found :(")
        return
    result = subprocess.run([get_command(), 'run', file], stdout=subprocess.PIPE, text=True, encoding=os.device_encoding(1))
    result = result.stdout
    result = re.sub("\\x1b\\[.+?m", "", result) # Remove escape sequences for colored text
    
    # Print result
    for _ in range(0, max_file_path_len - len(file), 1):
        print(" ", end="")
    print(" ", end="")

    if result == expected:
        print('\u001b[32mOK\u001b[0m')
        return True
    else:
        print('\u001b[31mFailed\u001b[0m')
        print()
        print(result)
        print("EXPECTED")
        print("========")
        print(expected)
        return False

def get_all_files(folder):
    files = []
    for file in os.listdir(folder):
        path = os.path.join(folder, file)
        if os.path.isdir(path):
            files += get_all_files(path)
        else:
            files.append(path)

    return files

def read_file_and_test(file, max_file_path_len):
    with open(file, encoding='utf-8') as content:
        content = content.read()

        try:
            expected = re.search("(?<=--- Output\n)(.|\n)*(?=---)", content).group(0)
            return test(file, expected, max_file_path_len)
        
        except:
            return True
        
def get_max_path_len(files):
    max_len = 0
    
    for file in files:
        if len(file) > max_len:
            max_len = len(file)

    return max_len

# Main
# ----
def main():
    folder = 'test'

    if len(sys.argv) > 2:
        print("Too many arguments :/")
        return sys.exit(1)

    if not os.path.exists(get_name()):
        print("diamond not found :(")
        return sys.exit(1)

    if len(sys.argv) > 1:
        folder = sys.argv[1]

    if os.path.isdir(folder):
        file_paths = get_all_files(folder)
        max_file_path_len = get_max_path_len(file_paths)
        num_cores = multiprocessing.cpu_count()
        with multiprocessing.Pool(num_cores) as pool:
            results = pool.map(functools.partial(read_file_and_test, max_file_path_len=max_file_path_len), file_paths)
            
            for result in results:
                if result == False: sys.exit(1)
    else:
        read_file_and_test(folder, get_max_path_len([folder]))

if __name__ == "__main__":
    main()
