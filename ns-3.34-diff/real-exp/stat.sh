for cc in gcc
do
    for queue in fifo codel wirtc
    do
        for trace in rest-wifi office-wifi pitree4g xu4g xu5g
        do
            conf=${cc}-${queue}-${trace}
            echo ${conf}

            # Network layer metric
            ls results/${conf}_* | xargs -i awk '/PacketSenderUdp/ {print $5}' {} | sort -n > stats/${conf}-net-rtt.log
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
        done
    done
done