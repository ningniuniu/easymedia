#!/bin/bash

find . -name "*.cc" -o -name "*.h" -o -name "*.hh" | xargs clang-format-8 -style=llvm -i
