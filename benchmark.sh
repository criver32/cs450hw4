#!/bin/bash

RED='\033[0;31m'
GREEN=='\033[0;32m'
NC='\033[0m' # No Color

mkfile() {
    head -c $1 </dev/urandom >$2;
}
mkhumfile() {
    yes | head -c $1 >$2;
}

bytesToHuman() {
    b=${1:-0}; d=''; s=0; S=(Bytes {K,M,G,T,P,E,Z,Y}iB)
    while ((b > 1024)); do
        d="$(printf ".%02d" $((b % 1024 * 100 / 1024)))"
        b=$((b / 1024))
        let s++
    done
    echo "$b$d ${S[$s]}"
}


benchmark_readable() {
    printf "Benchmarking with human-readable files\n"
    TESTFILE=results
    echo "size custom TCP" > $TESTFILE
    for ((i = 1; i < 100000; i=i*10)); do
        printf "Testing with a $(bytesToHuman $(($i*10240))) file\n"
        mkhumfile $(($i*10240)) testfile_$i

        printf "Starting receiver: $2 > outfile\n"
        $2 > outfile &
        receiverpid=$!

        printf "Timing the execution of $1 testfile_$i\n"
        execution_time=$({ time -p eval $1 testfile_$i; } 2>&1 | awk '/real/ { print $2 }')
        printf "Execution time: ${execution_time}\n"

        printf "Waiting for receiver to exit...\n"
        wait $receiverpid

        printf "Comparing file hashes\n"
        sent_md5=`md5sum testfile_$i | awk '{ print $1 }'`
        received_md5=`md5sum outfile | awk '{ print $1 }'`
        
        if [[ "$sent_md5" == "$received_md5" ]]; then
            printf "${GREEN} File hashes match${NC}\n"
        else
            printf "${RED} File hashes do not match, aborting benchmark ${NC}\n"
            break
        fi

        printf "Starting TCP receiver: nc -nvlkp 1234 > outfile\n"
        nc -nvlkp 1234 > outfile > outfile &
        receiverpid=$!

        printf "Timing the execution of nc -w 3 -n 127.0.0.1 1234 < testfile_$i\n"
        nc_execution_time=$({ time -p nc -w 3 -n 127.0.0.1 1234 < testfile_$i; } 2>&1 | awk '/real/ { print $2 }')
        printf "Execution time: ${nc_execution_time}\n"

        printf "Waiting for receiver to exit...\n"
        wait $receiverpid

        printf "Comparing file hashes\n"
        sent_md5=`md5sum testfile_$i | awk '{ print $1 }'`
        received_md5=`md5sum outfile | awk '{ print $1 }'`
        
        if [[ "$sent_md5" == "$received_md5" ]]; then
            printf "$i*10240 ${GREEN} File hashes match${NC}\n"
        else
            printf "${RED} File hashes do not match, aborting benchmark ${NC}\n"
            break
        fi

        echo "$(($i*10240)) $execution_time $nc_execution_time" >> $TESTFILE

        rm -f testfile_$i
        rm -f outfile
    done;

    diff=`awk 'NR>1 { udp_sum += $2; tcp_sum += $3 } END { print (udp_sum < tcp_sum) ? (1-udp_sum/tcp_sum)*100"% faster" : (1-tcp_sum/udp_sum)*100"% slower" }' $TESTFILE`

    printf "Benchmark complete. On average, your implementation ran $diff than TCP."
}


benchmark_random() {
    printf "Benchmarking with random files\n"
    TESTFILE=results
    echo "size custom TCP" > $TESTFILE
    for ((i = 1; i < 100000; i=i*10)); do
        printf "Testing with a $(bytesToHuman $(($i*10240))) file\n"
        mkfile $(($i*10240)) testfile_$i

        printf "Starting receiver: $2 > outfile\n"
        $2 > outfile &
        receiverpid=$!

        printf "Timing the execution of $1 testfile_$i\n"
        execution_time=$({ time -p eval $1 testfile_$i; } 2>&1 | awk '/real/ { print $2 }')
        printf "Execution time: ${execution_time}\n"

        printf "Waiting for receiver to exit...\n"
        wait $receiverpid

        printf "Comparing file hashes\n"
        sent_md5=`md5sum testfile_$i | awk '{ print $1 }'`
        received_md5=`md5sum outfile | awk '{ print $1 }'`
        
        if [[ "$sent_md5" == "$received_md5" ]]; then
            printf "${GREEN} File hashes match${NC}\n"
        else
            printf "${RED} File hashes do not match, aborting benchmark ${NC}\n"
            break
        fi

        printf "Starting TCP receiver: nc -nvlkp 1234 > outfile\n"
        nc -nvlkp 1234 > outfile > outfile &
        receiverpid=$!

        printf "Timing the execution of nc -w 3 -n 127.0.0.1 1234 < testfile_$i\n"
        nc_execution_time=$({ time -p nc -w 3 -n 127.0.0.1 1234 < testfile_$i; } 2>&1 | awk '/real/ { print $2 }')
        printf "Execution time: ${nc_execution_time}\n"

        printf "Waiting for receiver to exit...\n"
        wait $receiverpid

        printf "Comparing file hashes\n"
        sent_md5=`md5sum testfile_$i | awk '{ print $1 }'`
        received_md5=`md5sum outfile | awk '{ print $1 }'`
        
        if [[ "$sent_md5" == "$received_md5" ]]; then
            printf "$i*10240 ${GREEN} File hashes match${NC}\n"
        else
            printf "${RED} File hashes do not match, aborting benchmark ${NC}\n"
            break
        fi

        echo "$(($i*10240)) $execution_time $nc_execution_time" >> $TESTFILE

        rm -f testfile_$i
        rm -f outfile
    done;

    diff=`awk 'NR>1 { udp_sum += $2; tcp_sum += $3 } END { print (udp_sum < tcp_sum) ? (1-udp_sum/tcp_sum)*100"% faster" : (1-tcp_sum/udp_sum)*100"% slower" }' $TESTFILE`

    printf "Benchmark complete. On average, your implementation ran $diff than TCP."
}


plot() {
    # Usage makeplot outfile title xlabel ylabel datafile colnum

    gnuplot -e "
    set terminal 'pdfcairo';
    set output '$1';
    set title '$2';
    set xlabel '$3';
    set ylabel '$4';
    set logscale x;
    set key autotitle columnheader;
    set style data lines;
    plot for [i=2:$6] '$5' u 1:i;"
}