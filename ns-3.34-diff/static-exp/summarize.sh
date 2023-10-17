ls results/gcc-* | xargs -i awk -f output-stat.awk {} {} > stats/gcc-net-rtt.log
cat stats/gcc-net-rtt.log | grep "rtt" | awk -f ./summary-stat.awk > stats/gcc-net-rtt.result

ls results/* | grep -v "gcc" | xargs -i awk -f output-stat.awk {} {} > stats/tcp-net-rtt.log
cat stats/tcp-net-rtt.log | grep "rtt" | awk -f ./summary-stat.awk > stats/tcp-net-rtt.result
cat stats/tcp-net-rtt.log | grep "fd" | awk -f ./summary-stat.awk > stats/tcp-app-fd.result
cat stats/tcp-net-rtt.log | grep "fps" | awk -f ./summary-stat.awk > stats/tcp-app-fps.result