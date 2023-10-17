seq 1 1 5000 | awk 'NR<'"$1"'{printf("30Mbps 15.8ms 0.00\n")} NR>='"$1"'{printf("%.2fMbps %.2fms 0.00\n", 30 / '"$2"', 16-0.2*'"$2"')}' > $3/k$2-$1.log
