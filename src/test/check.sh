#!/bin/bash
cp ../cool.so .

# Run through each of the tests
./test_cool_value
if [ $? -eq 0 ]; then
    ./test_cool_environment
fi

# cleanup test files
rm *.tmp

# remove the library
rm cool.so
