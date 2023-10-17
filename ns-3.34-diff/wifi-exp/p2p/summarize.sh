cd ../../udp-static-exp/results/
ls | grep 'gcc' | xargs -i awk -f ../../wifi-exp/p2p/output-stat.awk {} {} > ../../wifi-exp/p2p/stats/gcc-net-p2p.log
cd ../../wifi-exp/p2p/
cat stats/gcc-net-p2p.log | grep "rtt" | awk -f ./summary-stat.awk > ./stats/gcc-net-rtt.result
cat stats/gcc-net-p2p.log | grep "fd" | awk -f ./summary-stat.awk > ./stats/gcc-net-fd.result
cat stats/gcc-net-p2p.log | grep "fps" | awk -f ./summary-stat.awk > ./stats/gcc-net-fps.result

# ls results/* | grep -v "gcc" | xargs -i awk -f output-stat.awk {} {} > stats/tcp-net-rtt.log
# cat stats/tcp-net-rtt.log | grep "rtt" | awk -f ./summary-stat.awk > stats/tcp-net-rtt.result