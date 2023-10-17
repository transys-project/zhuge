for cut in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19
do
    ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=50 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=0 --qdisc=1 --qsize=2 --wifi_same_ap=1 --wifi_compete=1 --cut=${cut}" > results/copa-codel-q-20-${cut}.log 2>&1 &
    sleep 10
done

# ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=40 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=0 --qdisc=0 --qsize=2 --wifi_same_ap=0 --wifi_compete=1 --cut=0" > results/wirtc-fifo-c-20.log 2>&1 &
# sleep 10
# ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=40 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=0 --qdisc=1 --qsize=2 --wifi_same_ap=0 --wifi_compete=1 --cut=0" > results/copa-codel-c-20.log 2>&1 &
# sleep 10
# ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=40 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=0 --qdisc=0 --qsize=2 --wifi_same_ap=0 --wifi_compete=1 --cut=0" > results/copa-fifo-c-20.log 2>&1 &
# sleep 10
# ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=40 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=2 --queue=2 --mode=0 --qdisc=0 --qsize=2 --wifi_same_ap=0 --wifi_compete=1 --cut=0" > results/abc-fifo-c-20.log 2>&1 &
# sleep 10
# ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=40 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=3 --mode=0 --qdisc=0 --qsize=2 --wifi_same_ap=0 --wifi_compete=1 --cut=0" > results/fast-fifo-c-20.log 2>&1 &
# sleep 10
# ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=40 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=0 --qdisc=1 --qsize=2 --wifi_same_ap=1 --wifi_compete=1 --cut=0" > results/wirtc-codel-q-20.log 2>&1 &
# sleep 10
# ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=40 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=0 --qdisc=0 --qsize=2 --wifi_same_ap=1 --wifi_compete=1 --cut=0" > results/wirtc-fifo-q-20.log 2>&1 &
# sleep 10
# ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=40 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=0 --qdisc=1 --qsize=2 --wifi_same_ap=1 --wifi_compete=1 --cut=0" > results/copa-codel-q-20.log 2>&1 &
# sleep 10
# ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=40 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=0 --qdisc=0 --qsize=2 --wifi_same_ap=1 --wifi_compete=1 --cut=0" > results/copa-fifo-q-20.log 2>&1 &
# sleep 10
# ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=40 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=2 --queue=2 --mode=0 --qdisc=0 --qsize=2 --wifi_same_ap=1 --wifi_compete=1 --cut=0" > results/abc-fifo-q-20.log 2>&1 &
# sleep 10
# ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=40 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=3 --mode=0 --qdisc=0 --qsize=2 --wifi_same_ap=1 --wifi_compete=1 --cut=0" > results/fast-fifo-q-20.log 2>&1 &