#!/bin/bash

valgrind --leak-check=full --track-origins=yes ./thread_hash -i passwords10.txt -d plain10.txt -t 4
