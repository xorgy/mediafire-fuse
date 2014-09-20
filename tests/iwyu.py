#!/usr/bin/env python

from __future__ import print_function

import json
import subprocess
import shlex

with open("compile_commands.json", "r") as f:
    tunits = json.load(f)

result = 0
for tu in tunits:
    _,cmd = tu["command"].split(" ",1)
    cmd = "%s %s"%("iwyu", cmd)
    cmd = shlex.split(cmd)
    # iwyu does not distinguish between different outcomes of its check
    # so instead, we grep its stderr output
    # see http://code.google.com/p/include-what-you-use/issues/detail?id=157
    ret = subprocess.Popen(cmd, stderr=subprocess.PIPE)
    _,ret = ret.communicate()
    if "has correct #" not in ret:
        result += 1
        print(ret)

exit(result)
