#!/bin/sh
set -e
cmake -B test/build test/
cmake --build test/build --parallel
./test/build/scout_tests
