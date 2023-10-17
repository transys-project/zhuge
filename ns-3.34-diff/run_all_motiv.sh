for k in 2 3 4 5 6 7 8 10 12 14 16 20 25 30 35 40 45 50
do
    for cut in `seq 3600 40 4400`
    do
        echo ${k} ${cut} "copa codel"
        ./waf --run "scratch/fec-emulate --policy=rtx --ddl=10000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --wirtc=0 --cc_create=true --cc_tcp_choice=1 --qdisc=1" > motivation-results/copa-codel-k${k}-${cut}.log 2>&1 &
        sleep 30
    done
done

for k in 2 3 4 5 6 7 8 10 12 14 16 20 25 30 35 40 45 50
do
    for cut in `seq 3600 40 4400`
    do
        echo ${k} ${cut} "copa fifo"
        ./waf --run "scratch/fec-emulate --policy=rtx --ddl=10000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --wirtc=0 --cc_create=true --cc_tcp_choice=1 --qdisc=0" > motivation-results/copa-fifo-k${k}-${cut}.log 2>&1 &
        sleep 30
    done
done

for k in 2 3 4 5 6 7 8 10 12 14 16 20 25 30 35 40 45 50
do
    for cut in `seq 3600 40 4400`
    do
        echo ${k} ${cut} "cubic codel"
        ./waf --run "scratch/fec-emulate --policy=rtx --ddl=10000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --wirtc=0 --cc_create=true --cc_tcp_choice=4 --qdisc=1" > motivation-results/cubic-codel-k${k}-${cut}.log 2>&1 &
        sleep 30
    done
done

for k in 2 3 4 5 6 7 8 10 12 14 16 20 25 30 35 40 45 50
do
    for cut in `seq 3600 40 4400`
    do
        echo ${k} ${cut} "cubic fifo"
        ./waf --run "scratch/fec-emulate --policy=rtx --ddl=10000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --wirtc=0 --cc_create=true --cc_tcp_choice=4 --qdisc=0" > motivation-results/cubic-fifo-k${k}-${cut}.log 2>&1 &
        sleep 30
    done
done

for k in 2 3 4 5 6 7 8 10 12 14 16 20 25 30 35 40 45 50
do
    for cut in `seq 3600 40 4400`
    do
        echo ${k} ${cut} "bbr codel"
        ./waf --run "scratch/fec-emulate --policy=rtx --ddl=10000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --wirtc=0 --cc_create=true --cc_tcp_choice=3 --qdisc=1" > motivation-results/bbr-codel-k${k}-${cut}.log 2>&1 &
        sleep 30
    done
done

for k in 2 3 4 5 6 7 8 10 12 14 16 20 25 30 35 40 45 50
do
    for cut in `seq 3600 40 4400`
    do
        echo ${k} ${cut} "bbr fifo"
        ./waf --run "scratch/fec-emulate --policy=rtx --ddl=10000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --wirtc=0 --cc_create=true --cc_tcp_choice=3 --qdisc=0" > motivation-results/bbr-fifo-k${k}-${cut}.log 2>&1 &
        sleep 30
    done
done

for k in 2 3 4 5 6 7 8 10 12 14 16 20 25 30 35 40 45 50
do
    for cut in `seq 3600 40 4400`
    do
        echo ${k} ${cut} "gcc codel"
        ./waf --run "scratch/fec-emulate --policy=rtx --ddl=10000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --wirtc=0 --cc_create=false --cc_tcp_choice=1 --qdisc=1" > motivation-results/gcc-codel-k${k}-${cut}.log 2>&1 &
        sleep 30
    done
done

for k in 2 3 4 5 6 7 8 10 12 14 16 20 25 30 35 40 45 50
do
    for cut in `seq 3600 40 4400`
    do
        echo ${k} ${cut} "gcc fifo"
        ./waf --run "scratch/fec-emulate --policy=rtx --ddl=10000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k${k}-${cut}.log --wirtc=0 --cc_create=false --cc_tcp_choice=1 --qdisc=0" > motivation-results/gcc-fifo-k${k}-${cut}.log 2>&1 &
        sleep 30
    done
done