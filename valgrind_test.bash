#!/bin/bash

# Define the files to be used
PASSWORD_FILE="passwords10.txt"
PLAIN_FILE="plain10.txt"
THREADS=1

# Check for memory leaks with Valgrind
echo "Running Valgrind with 10 passwords..."
valgrind --leak-check=full --track-origins=yes --show-reachable=yes ./thread_hash -i $PASSWORD_FILE -d $PLAIN_FILE -t $THREADS
