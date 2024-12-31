#!/usr/bin/env python3
import os 

# Get current directory
current_directory = os.path.dirname(os.path.realpath(__file__))

# Create .clangd content
content = \
f""" \
CompileFlags:
    Add: [-std=c++17, -I{current_directory}/deps/llvm/include]
"""

# Write file
with open(".clangd", "w") as output:
    output.write(content)