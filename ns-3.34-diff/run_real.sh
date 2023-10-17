# for file in `ls real-exp/traces/office*`
# do
# 	filename=${file##*/}
# 	echo ${filename}
# 	./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/${filename} --node_number=10 --cc_create=false --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=1 --traceset=2" > real-exp/results/gcc-codel-${filename} 2>&1 &
# 	sleep 2
# done

# for file in `ls real-exp/traces/office*`
# do
# 	filename=${file##*/}
# 	echo ${filename}
# 	./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/${filename} --node_number=10 --cc_create=false --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=1 --traceset=2" > real-exp/results/gcc-wirtc-${filename} 2>&1 &
# 	sleep 2
# done

for file in `ls real-exp/traces/office*`
do
	filename=${file##*/}
	echo ${filename}
	./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/${filename} --node_number=10 --cc_create=false --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=0 --traceset=2" > real-exp/results/gcc-fifo-${filename} 2>&1 &
	sleep 2
done
