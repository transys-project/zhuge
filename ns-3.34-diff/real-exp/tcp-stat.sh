rtt=200
fd=400
fps=10

rm stats/summary-tcp-net-rtt.log
rm stats/summary-tcp-app-delay.log
rm stats/summary-tcp-app-fps.log

for queue in copa-fifo copa-codel copa-wirtc copa-fast1 copa-fast2 copa-fast4 copa-fast8 abc-fifo copa-fifortc
do
    for trace in rest-wifi office-wifi pitree4g xu4g xu5g abc4g
    do
        conf=${queue}-${trace}
        echo ${conf}

        # Network layer metric
        ls results/${conf}_* | xargs -i awk '/PacketSenderTcp/ && /RttNet/ {print $5}' {} | sort -n > stats/${conf}-net-rtt.log
        zsh ../log2cdf.sh stats/${conf}-net-rtt.log

        # Application layer metric
        ls results/${conf}_* | xargs -i awk -f ../app_delay.awk {} | sort -n > stats/${conf}-app-delay.log
        zsh ../log2cdf.sh stats/${conf}-app-delay.log

        # The fps needs to remove the first and last second stat for accuracy
        ls results/${conf}_* | xargs -i awk -f ../app_fps.awk {} | sort -nr > stats/${conf}-app-fps.log
        zsh ../log2cdf.sh stats/${conf}-app-fps.log
        awk '{fps = $1 - 1; print $0} END {
            while (fps >= 0) {
                printf("%d\t0\n", fps)
                fps--
            }
        }' stats/${conf}-app-fps.log.result > stats/${conf}-app-fps.log.result.tmp
        mv stats/${conf}-app-fps.log.result.tmp stats/${conf}-app-fps.log.result
        
        awk '{
            if ($1 >= '"${rtt}"') {
                printf("%s\t%s\t%.4f\n", "'${queue}'", "'${trace}'", $2)
                nextfile
            }
        }' stats/${queue}-${trace}-net-rtt.log.result >> stats/summary-tcp-net-rtt.log
        awk '{
            if ($1 >= '"${fd}"') {
                printf("%s\t%s\t%.4f\n", "'${queue}'", "'${trace}'", $2)
                nextfile
            }
        }' stats/${queue}-${trace}-app-delay.log.result >> stats/summary-tcp-app-delay.log
        awk '{
            if ($1 <= '"${fps}"') {
                printf("%s\t%s\t%.4f\n", "'${queue}'", "'${trace}'", $2)
                nextfile
            }
        }' stats/${queue}-${trace}-app-fps.log.result >> stats/summary-tcp-app-fps.log
    done
done