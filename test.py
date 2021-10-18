#!/usr/bin/env python3
import os
import re
import platform
import subprocess

def print_expected_vs_result(expected, result):
	expected = expected.split("\n")
	print("  Expected:")
	for line in expected:
		print("    " + line)

	result = result.split("\n")
	print("  Result:")
	for line in result:
		print("    " + line)

def test(file, expected):
	print(f"testing {file}...  ", end='')
	if not os.path.exists(file):
		print("File not found :(")
		return
	result = subprocess.run(['./diamond', 'run', file], stdout=subprocess.PIPE, text=True)
	result = result.stdout
	result =  re.sub("\\x1b\\[.+?m", "", result) # Remove escape sequences for colored text
	if result == expected:
		print('OK')
	else:
		print('Failed :(')
		print_expected_vs_result(expected, result)
		quit()

def get_all_files(folder, file_paths = []):
	for file in os.listdir(folder):
		path = os.path.join(folder, file)
		if os.path.isdir(path):
			file_paths += get_all_files(path)
		else:
			file_paths.append(path)

	return file_paths

def main():
	for file in get_all_files('test'):
		content = open(file).read()
		expected = re.search("(?<=--- Output)(.|\n|\r\n)*(?=---)", content).group(0).strip() + '\n'
		test(file, expected)

if __name__ == "__main__":
	main()
