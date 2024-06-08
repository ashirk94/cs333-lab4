#!/bin/bash

# Check if input files are provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <password_file> <dictionary_file>"
    exit 1
fi

PASSWORD_FILE=$1
DICTIONARY_FILE=$2
THREAD_COUNTS=(1 2 4 8 16 24)

echo "Running tests for input files: $PASSWORD_FILE and $DICTIONARY_FILE"
echo "Threads, Time (seconds)"

for THREADS in "${THREAD_COUNTS[@]}"; do
    START=$(date +%s.%N)
    valgrind --leak-check=full --show-leak-kinds=all ./thread_hash -i "$PASSWORD_FILE" -d "$DICTIONARY_FILE" -t "$THREADS"
    END=$(date +%s.%N)
    TIME=$(echo "$END - $START" | bc)
    echo "$THREADS, $TIME"
done
