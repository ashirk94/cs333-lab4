#!/bin/bash

echo "Testing the -i and -d flags"
./thread_hash -i passwords10.txt -d plain10.txt -t 4

echo "Testing the -o flag"
./thread_hash -i passwords10.txt -d plain10.txt -t 4 -o output.txt
cat output.txt

echo "Testing the -t flag"
./thread_hash -i passwords10.txt -d plain10.txt -t 2
./thread_hash -i passwords10.txt -d plain10.txt -t 4

echo "Testing the -v flag"
./thread_hash -i passwords10.txt -d plain10.txt -t 4 -v

echo "Testing the -n flag"
./thread_hash -i passwords10.txt -d plain10.txt -t 4 -n

echo "Testing the -h flag"
./thread_hash -h

echo "Testing -q flag for unknown"
./thread_hash -q
