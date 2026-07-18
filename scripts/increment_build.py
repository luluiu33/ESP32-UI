#!/usr/bin/env python3
"""Increment FW_BUILD in main.h before each build."""
import re, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PATH = os.path.join(ROOT, "main", "main.h")

with open(PATH, "r", encoding="utf-8") as f:
    content = f.read()

def inc(m):
    return f"#define FW_BUILD {int(m.group(1)) + 1}"

new_content = re.sub(r'#define FW_BUILD (\d+)', inc, content)

if new_content != content:
    with open(PATH, "w", encoding="utf-8") as f:
        f.write(new_content)
    print(f"  [+] FW_BUILD incremented")
else:
    print(f"  [-] FW_BUILD not found, unchanged")
