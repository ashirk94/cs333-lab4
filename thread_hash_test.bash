#!/bin/bash

PROG=./thread_hash
BE_NICE=-n
TOTAL_POINTS=0
DATA_DIR=/disk/scratch/rchaney/Classes/cs333/Labs/hash_test
NICE_POINTS=3
LINE_POINTS=5
CRACKED_POINTS=17

NOLEAKS="All heap blocks were freed -- no leaks are possible"

WARN_FILE=.WARN.err
TIME_LOG=.Time.log

if [ ! -x ${PROG} ]
then
    make all

    if [ ! -x ${PROG} ]
    then
        echo "**** Program did not build. Exiting with 0 points. ****"
        exit 1
    fi
fi

> ${TIME_LOG}

# for W in 10 100 #500 #1000 #2000 3000 #4000 5000
# for W in 100 #500 #1000 #2000 3000 #4000 5000
for W in 250 500 #1000 2000 3000 #4000 5000
do
    #for T in 1 2 4 8 16 24
    #for T in 1 2 4 8 16 24
    for T in 1 2 4 8 16 24
    do
        ${PROG} -i ${DATA_DIR}/passwords${W}.txt -d ${DATA_DIR}/plain${W}.txt -t ${T} ${BE_NICE} \
                -o cracked_T${T}_W${W}.out 2> cracked_T${T}_W${W}.err > /dev/null &

        BG=$!
        echo "Started: threads = ${T}   passwords${W}.txt   pid = ${BG}"
        BEGINT=$(date +%s)
        #NICE_VAL=$(ps -efl | grep ${LOGNAME} | grep ${BG} | grep -v grep | awk '{print $8;}')
        NICE_VAL=$(ps -efl | grep ${LOGNAME} | awk -v BG=${BG} '{if ( $4 == BG ) print $8;}')
        if [ -z "${NICE_VAL}" ]
        then
            echo -e "\t*** FAILED TO FIND PROCESS. PROBABLY A FAILURE IN YOUR getopt() ***"
            echo -e "\tSkipping the rest of this iteration."
            continue
        fi
        echo -e "\tpid = ${BG}  nice = ${NICE_VAL}"
        if [ ${NICE_VAL} -gt 0 ]
        then
            ((TOTAL_POINTS+=NICE_POINTS))
            echo -e "\tThanks for being nice. That is worth ${NICE_POINTS} points."
            echo -e "\tTOTAL_POINTS = ${TOTAL_POINTS}"
        else
            ((TOTAL_POINTS-=NICE_POINTS))
            echo -e "\tWhat happened to being nice? You lost ${NICE_POINTS} points for that."
            echo -e "\tTOTAL_POINTS = ${TOTAL_POINTS}"
        fi
        
        wait ${BG}
        ENDT=$(date +%s)
        ET=$((ENDT - BEGINT))
        #ETM=$(echo "${ET} / 60.0" | bc -l)
        ETM=$(echo ${ET} | awk '{printf "%.2f", $1 / 60;}')
        #echo -e "\tBack from ${BG}  et = ${ET} sec"
        echo -e "\tProcess done:  threads = ${T}   hash count = ${W}  et = ${ET} sec   ${ETM} min"

        if [ ! -f cracked_T${T}_W${W}.out ]
        then
            echo -e "\t*** FAILED TO CREATE OUTPUT FILE. CHECK THE -o COMMAND LINE OPTION. ***"
            echo -e "\tSkipping the rest of this iteration."
            continue
        fi
        
        CRACKED=$(wc -l < cracked_T${T}_W${W}.out)
        if [ ${CRACKED} -eq ${W} ]
        then
            ((TOTAL_POINTS+=LINE_POINTS))
            echo -e "\tCracked password count matches expected:  threads = ${T}   password count = ${W}"
            echo -e "\tTOTAL_POINTS = ${TOTAL_POINTS}"
        else
            echo -e "\t*** CRACKED PASSWORD COUNT DOES NOT MATCH EXPECTED: ${CRACKED} != ${W} (expected)   threads = ${T}   password count = ${W}"
            echo -e "\tYou need to fix this before trying more tests."
            echo -e "\tSkipping the rest of this iteration."
            echo -e "FAILED hashes: ${W}\tthreads: ${T}" >> ${TIME_LOG}
            continue
        fi

        awk '{print $2;}' cracked_T${T}_W${W}.out | uniq | sort > cracked_T${T}_W${W}.cracked
        awk -F ':' '{print $1;}' ${DATA_DIR}/key${W}.txt | sort | uniq > cracked_T${T}_W${W}.plain

        diff -q cracked_T${T}_W${W}.cracked cracked_T${T}_W${W}.plain > /dev/null 2> /dev/null
        RDIFF=$?
        if [ ${RDIFF} -eq 0 ]
        then
            ((TOTAL_POINTS+=CRACKED_POINTS))
            echo -e "\tCracked passwords match." # cracked_T${T}_W${W}.cracked cracked_T${T}_W${W}.plain"
            echo -e "\tTOTAL_POINTS = ${TOTAL_POINTS}"
            if [ ${ET} -lt 100 ]
            then
                echo -e "\thashes: ${W}\tthreads: ${T}\tsec: ${ET}\t\tmin: ${ETM}" >> ${TIME_LOG}
            else
                echo -e "\thashes: ${W}\tthreads: ${T}\tsec: ${ET}\tmin: ${ETM}" >> ${TIME_LOG}
            fi
        else
            echo -e "FAILED hashes: ${W}\tthreads: ${T}" >> ${TIME_LOG}
            echo -e "\t*** CRACKED PASSWORDS DO NOT MATCH!!! Try: diff cracked_T${T}_W${W}.cracked ${DATA_DIR}/cracked_T${T}_W${W}.plain"
            diff cracked_T${T}_W${W}.cracked cracked_T${T}_W${W}.plain 2> /dev/null | head
        fi
    done
done

T=3
W=50
# skipping being nice inside valgrind
VG_OPTS="-i ${DATA_DIR}/passwords${W}.txt -d ${DATA_DIR}/plain${W}.txt -t ${T} -o cracked_T${T}_W${W}.out"
echo -e "\nChecking with valgrind.\n\tthreads = ${T}  hashes = ${W}"
valgrind ${PROG} ${VG_OPTS} 2> V.log > /dev/null
ERR=$?

if [ ${ERR} -ne 0 ]
then
    ((TOTAL_POINTS-=5))
    echo -e "\t*** VALGRIND WAS VERY UNHAPPY WITH YOUR PROGRAM! ***"
    echo -e "\tTOTAL_POINTS = ${TOTAL_POINTS}"
else
    ((TOTAL_POINTS+=5))
    echo -e "\tA happy valgrind means more points for you."
    echo -e "\tTOTAL_POINTS = ${TOTAL_POINTS}"
fi

LEAKS=$(grep "${NOLEAKS}" V.log | wc -l)
#echo "Leak count ${LEAKS}"
if [ ${LEAKS} -eq 1 ]
then
    ((TOTAL_POINTS+=5))
    #echo -e "\n\tNo leaks found. Excellent!!!"
    echo -e "\tNo leaks means more points for you."
    echo -e "\tTOTAL_POINTS = ${TOTAL_POINTS}"
else
    OLD_POINTS=${TOTAL_POINTS}
    echo -e "\n\t*** LEAKS FOUND, A 20% deduction! ***"
    TOTAL_POINTS=$(echo "${TOTAL_POINTS} * 0.8" | bc -l)
    echo -e "\tTOTAL_POINTS = ${TOTAL_POINTS}  down from ${OLD_POINTS}"
    LEAKS_FOUND=1
fi

if [ -s ${WARN_FILE} ]
then
    OLD_POINTS=${TOTAL_POINTS}
    echo -e "\n\t*** YOU HAVE COMPILATION WARNINGS. A 20% DEDUCTION! ***"
    TOTAL_POINTS=$(echo "${TOTAL_POINTS} * 0.8" | bc -l)
    echo -e "\tTOTAL_POINTS = ${TOTAL_POINTS}  down from ${OLD_POINTS}"
    echo -e "*** Try rerunnng the Makefile test script. ***"
fi

cat <<EOF
If the performance numbers don't show a reduction in time, it 
is a 40% deduction in points.
You should see an approximate logrithmic reduction in time 
from 1-16 threads.
The values for 16 and 24 threads will look about the same.
EOF
cat ${TIME_LOG}
