for k in 2 3 4 5 6 7 8 10 12 14 16 20 25 30 35 40 45 50 
do
	for cut in `seq 500 40 1300`
	do
		zsh ktrace_gen.sh ${cut} ${k} testtraces
	done
	for cut in `seq 3600 40 1300`
	do
		zsh ktrace_gen.sh ${cut} ${k} traces
	done
done
