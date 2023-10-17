awk -f ./tcp-summarize.awk stats/summary-tcp-net-rtt.log > stats/summary-tcp-net-rtt.result
awk -f ./tcp-summarize.awk stats/summary-tcp-app-delay.log > stats/summary-tcp-app-delay.result
awk -f ./tcp-summarize.awk stats/summary-tcp-app-fps.log > stats/summary-tcp-app-fps.result