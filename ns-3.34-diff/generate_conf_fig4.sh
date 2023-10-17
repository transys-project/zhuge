fname="fig4.conf"
rm ${fname}
for trace in `ls static-exp/traces`
do
    echo '"scratch/fec-emulate --policy=rtx --ddl=10000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=static-exp/traces/'${trace}' --cc_create=false --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=0 --traceset=1",static-exp/results/gcc-fifo-'${trace}'.log' >> ${fname}
    echo '"scratch/fec-emulate --policy=rtx --ddl=10000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=static-exp/traces/'${trace}' --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=0 --traceset=1",static-exp/results/copa-fifo-'${trace}'.log' >> ${fname}
    echo '"scratch/fec-emulate --policy=rtx --ddl=10000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=static-exp/traces/'${trace}' --cc_create=true --cc_tcp_choice=3 --queue=0 --mode=1 --qdisc=0 --traceset=1",static-exp/results/bbr-fifo-'${trace}'.log' >> ${fname}
    echo '"scratch/fec-emulate --policy=rtx --ddl=10000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=static-exp/traces/'${trace}' --cc_create=true --cc_tcp_choice=4 --queue=0 --mode=1 --qdisc=0 --traceset=1",static-exp/results/cubic-fifo-'${trace}'.log' >> ${fname}
done


