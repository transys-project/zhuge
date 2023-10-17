for k in 30 
do
    for cut in `seq 3600 40 4400`
    do
        echo ${k} ${cut} "copa wirtc"
        ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=1 --qsize=2" > results/copa-wirtc-${cut}.log 2>&1 &
        sleep 13
    done
done

# for k in 30 
# do
#     for cut in `seq 3840 40 4400`
#     do
#         echo ${k} ${cut} "copa wirtc"
#         ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=0 --qsize=2" > results/copa-fifo-wirtc-${cut}.log 2>&1 &
#         sleep 13
#     done
# done

# sleep 1200

# for k in 30 
# do
#     for cut in `seq 3600 40 4400`
#     do
#         echo ${k} ${cut} "abc wirtc"
#         ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --node_number=10 --cc_create=true --cc_tcp_choice=2 --queue=2 --mode=1 --qdisc=0 --qsize=2" > results/abc-k30-fifo-${cut}.log 2>&1 &
#         sleep 13
#     done
# done

# sleep 1200

# for k in 30 
# do
#     for cut in `seq 3600 40 4400`
#     do
#         echo ${k} ${cut} "fastack wirtc"
#         ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=3 --mode=1 --qdisc=0 --qsize=2" > results/fast-k30-fifo-${cut}.log 2>&1 &
#         sleep 13
#     done
# done

# for k in 30 
# do
#     for cut in `seq 3600 40 4400`
#     do
#         echo ${k} ${cut} "copa nowirtc 1p"
#         ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=1 --qsize=2" > results/copa-nowirtc-${cut}.log 2>&1 &
#         sleep 13
#     done
# done

# for k in 30 
# do
#     for cut in `seq 3600 40 4400`
#     do
#         echo ${k} ${cut} "copa wirtc 100p"
#         ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=1 --qsize=2" > Copa-k30-qsize/copa-wirtc-100p-k${k}-${cut}.log 2>&1 &
#         sleep 13
#     done
# done

# for k in 30 
# do
#     for cut in `seq 3600 40 4400`
#     do
#         echo ${k} ${cut} "copa nowirtc 100p"
#         ./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=1 --qsize=2" > Copa-k30-qsize/copa-nowirtc-100p-k${k}-${cut}.log 2>&1 &
#         sleep 13
#     done
# done