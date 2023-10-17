rtt=200
fd=400
fps=10

rm stats/summary-net-rtt.log
rm stats/summary-app-delay.log
rm stats/summary-app-fps.log
for cc in gcc
do
    for queue in fifo codel wirtc
    do
        for trace in rest-wifi office-wifi pitree4g xu4g xu5g
        do
            awk '{
                if ($1 >= '"${rtt}"') {
                    printf("%s\t%s\t%.4f\n", "'${cc}'-'${queue}'", "'${trace}'", $2)
                    nextfile
                }
            }' stats/${cc}-${queue}-${trace}-net-rtt.log.result >> stats/summary-net-rtt.log
            awk '{
                if ($1 >= '"${fd}"') {
                    printf("%s\t%s\t%.4f\n", "'${cc}'-'${queue}'", "'${trace}'", $2)
                    nextfile
                }
            }' stats/${cc}-${queue}-${trace}-app-delay.log.result >> stats/summary-app-delay.log
            awk '{
                if ($1 <= '"${fps}"') {
                    printf("%s\t%s\t%.4f\n", "'${cc}'-'${queue}'", "'${trace}'", $2)
                    nextfile
                }
            }' stats/${cc}-${queue}-${trace}-app-fps.log.result >> stats/summary-app-fps.log
        done
    done
done

awk -f ./summarize.awk stats/summary-net-rtt.log > stats/summary-net-rtt.result
awk -f ./summarize.awk stats/summary-app-delay.log > stats/summary-app-delay.result
awk -f ./summarize.awk stats/summary-app-fps.log > stats/summary-app-fps.result