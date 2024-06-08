#!/bin/bash

INPUT_FILE="passwords500.txt"
DICT_FILE="plain500.txt"
OUTPUT_FILE="performance_results.txt"

THREADS=(32)

echo "Testing performance with $INPUT_FILE and $DICT_FILE" > $OUTPUT_FILE

for t in "${THREADS[@]}"; do
    echo "Running with $t threads..."
    START=$(date +%s.%N)
    ./thread_hash -i $INPUT_FILE -d $DICT_FILE -t $t
    END=$(date +%s.%N)
    RUNTIME=$(echo "$END - $START" | bc)
    echo "Threads: $t, Time: $RUNTIME seconds" >> $OUTPUT_FILE
done

echo "Performance test completed. Results are in $OUTPUT_FILE"
