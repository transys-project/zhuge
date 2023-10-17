cd ../../udp-static-exp/results/
ls | grep 'queue' | xargs -i awk -f ../../wifi-exp/queue/output-stat.awk {} {} > ../../wifi-exp/queue/stats/gcc-net-queue.log
cd ../../wifi-exp/queue/
cat stats/gcc-net-queue.log | grep "rtt" | awk -f ./summary-stat.awk > ./stats/gcc-net-rtt.result
cat stats/gcc-net-queue.log | grep "fd" | awk -f ./summary-stat.awk > ./stats/gcc-net-fd.result
cat stats/gcc-net-queue.log | grep "fps" | awk -f ./summary-stat.awk > ./stats/gcc-net-fps.result

# ls results/* | grep -v "gcc" | xargs -i awk -f output-stat.awk {} {} > stats/tcp-net-rtt.log
# cat stats/tcp-net-rtt.log | grep "rtt" | awk -f ./summary-stat.awk > stats/tcp-net-rtt.result