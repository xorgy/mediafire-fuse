#!/usr/bin/env python

from __future__ import print_function

import json
import subprocess

with open("compile_commands.json", "r") as f:
    tunits = json.load(f)

result = 0
for tu in tunits:
    _,rest = tu["command"].split(" ",1)
    # iwyu does not distinguish between different outcomes of its check
    # so instead, we grep its stderr output
    # see http://code.google.com/p/include-what-you-use/issues/detail?id=157
    ret = subprocess.call("%s %s 2>&1 | grep \"has correct #\""%("iwyu", rest), shell=True)
    if ret != 0:
        result += 1

exit(result)
