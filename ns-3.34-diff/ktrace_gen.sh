seq 1 1 5000 | awk 'NR<'"$1"'{printf("30Mbps 10ms 0.00\n")} NR>='"$1"'{printf("%.2fMbps 10ms 0.00\n", 30 / '"$2"')}' > k$2-$1.log
