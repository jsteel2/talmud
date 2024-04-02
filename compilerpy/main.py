#!/usr/bin/env python3

import sys
from compiler import Compiler

if __name__ == "__main__":
    with open(sys.argv[1], "wb") as o:
        with open(sys.argv[2], "r") as i:
            compiler = Compiler(i, o, sys.argv[2])
            compiler.run()
