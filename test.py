#!/usr/bin/env python3
import os
import re
import subprocess
import sys

def test(file, expected):
    print(f"testing {file}...  ", end='')
    if not os.path.exists(file):
        print("File not found :(")
        return
    result = subprocess.run(['./diamond', 'run', file], stdout=subprocess.PIPE, text=True)
    result = result.stdout
    result =  re.sub("\\x1b\\[.+?m", "", result) # Remove escape sequences for colored text
    if result == expected:
        print('\u001b[32mOK\u001b[0m')
    else:
        print('\u001b[31mFailed\u001b[0m')
        quit()

def get_all_files(folder):
    files = []
    for file in os.listdir(folder):
        path = os.path.join(folder, file)
        if os.path.isdir(path):
            files += get_all_files(path)
        else:
            files.append(path)

    return files

def main():
    folder = 'test'

    if len(sys.argv) > 2:
        print("Too many arguments :/")
        return

    if len(sys.argv) > 1:
        folder = sys.argv[1]

    for file in get_all_files(folder):
        content = open(file).read()
        
        try:
            expected = re.search("(?<=--- Output\n)(.|\n)*(?=---)", content).group(0)
            test(file, expected)
        except:
            pass

if __name__ == "__main__":
    main()
