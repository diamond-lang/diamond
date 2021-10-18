#!/usr/bin/env python3
import subprocess
import os
import re

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
	print(f"testing {file}... ", end='')
	if not os.path.exists(file):
		print("File not found :(")
		return
	result = subprocess.run(['./diamond', 'run', file], stdout=subprocess.PIPE, text=True)
	if result.stdout == expected:
		print('OK')
	else:
		print('Failed :(')
		print_expected_vs_result(expected, result.stdout)
		quit()

def main():
	for file in os.listdir('test'):
		file = os.path.join('test', file)
		content = open(file).read()
		expected = re.search("(?<=--- Output)(.|\n|\r\n)*(?=---)", content).group(0).strip() + '\n'
		test(file, expected)

if __name__ == "__main__":
	main()
