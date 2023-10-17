echo 'queue=0 udp'
rm -rf results
mkdir results
./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k30-3760.log --node_number=10 --cc_create=false --cc_tcp_choice=1 --queue=0 --mode=1" > P2P_Emulation/gcc/output.log 2>&1
cp results/fec-emulation.tr P2P_Emulation/gcc/

echo 'queue=1 udp'
rm -rf results
mkdir results
./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k30-3760.log --node_number=10 --cc_create=false --cc_tcp_choice=1 --queue=1 --mode=1" > P2P_Emulation/gcc+wirtc/output.log 2>&1
cp results/fec-emulation.tr P2P_Emulation/gcc+wirtc/

echo 'queue=0 tcp'
rm -rf results
mkdir results
./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k30-3760.log --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=1" > P2P_Emulation/copa/output.log 2>&1
cp results/fec-emulation.tr P2P_Emulation/copa/

echo 'queue=1 tcp'
rm -rf results
mkdir results
./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k30-3760.log --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=1" > P2P_Emulation/copa+wirtc/output.log 2>&1
cp results/fec-emulation.tr P2P_Emulation/copa+wirtc/

echo 'abc tcp'
rm -rf results
mkdir results
./waf --run "scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=traces/k30-3760.log --node_number=10 --cc_create=true --cc_tcp_choice=2 --queue=2 --mode=1 --qdisc=0" > P2P_Emulation/abc/output.log 2>&1
cp results/fec-emulation.tr P2P_Emulation/abc/