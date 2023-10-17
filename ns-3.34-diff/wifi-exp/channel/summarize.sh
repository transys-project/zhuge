cd ../../udp-wifi-exp/results/
ls | grep 'channel' | xargs -i awk -f ../../wifi-exp/channel/output-stat.awk {} {} > ../../wifi-exp/channel/stats/gcc-net-channel.log
cd ../../wifi-exp/channel/
cat stats/gcc-net-channel.log | grep "rtt" | awk -f ./summary-stat.awk > ./stats/gcc-net-rtt.result
cat stats/gcc-net-channel.log | grep "fd" | awk -f ./summary-stat.awk > ./stats/gcc-net-fd.result
cat stats/gcc-net-channel.log | grep "fps" | awk -f ./summary-stat.awk > ./stats/gcc-net-fps.result

# ls results/* | grep -v "gcc" | xargs -i awk -f output-stat.awk {} {} > stats/tcp-net-rtt.log
# cat stats/tcp-net-rtt.log | grep "rtt" | awk -f ./summary-stat.awk > stats/tcp-net-rtt.result