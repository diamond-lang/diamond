#!/usr/bin/env python3
import subprocess
from os.path import exists

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
	if not exists('test/' + file):
		print("File not found :(")
		return
	result = subprocess.run(['./diamond', 'run', 'test/' + file], stdout=subprocess.PIPE, text=True)
	if result.stdout == expected:
		print('OK')
	else:
		print('Failed :(')
		print_expected_vs_result(expected, result.stdout)
		quit()

test("floats.dm", "4.3\n" + \
                  "6\n" + \
                  "0.8\n" + \
                  "0.33333\n")

test("arithmetic_expressions.dm", "16\n" + \
                                  "-3\n" + \
                                  "-7.5\n" +
                                  "6\n" + \
                                  "30\n" + \
                                  "-2\n")

test("comparison_expressions.dm", "false\n" + \
                                  "true\n" + \
                                  "true\n" + \
                                  "true\n" + \
                                  "false\n")

test("assignments.dm", "113\n" + \
                       "true\n")

test("booleans.dm", "true\n" + \
                    "true\n" + \
                    "true\n" + \
                    "true\n" + \
                    "false\n")

test("functions.dm", "10\n" + \
                     "0.875\n" + \
                     "5\n" + \
                     "false\n")

test("integers.dm", "5\n" + \
                    "6\n" + \
                    "-13\n" + \
                    "5\n" + \
                    "1\n")
