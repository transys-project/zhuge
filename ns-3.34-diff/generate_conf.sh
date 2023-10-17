# fname="gcc-exp-trace.conf"
# rm ${fname}
# for trace in `ls real-exp/traces`
# do
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/'${trace}' --node_number=10 --cc_create=false --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=0 --traceset=2",real-exp/results/gcc-fifo-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/'${trace}' --node_number=10 --cc_create=false --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=1 --traceset=2",real-exp/results/gcc-codel-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/'${trace}' --node_number=10 --cc_create=false --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=1 --traceset=2",real-exp/results/gcc-wirtc-'${trace}'.log' >> ${fname}
# done

# fname="tcp-exp-trace.conf"
# rm ${fname}
# for trace in `ls real-exp/traces`
# do
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=2 --queue=2 --mode=1 --qdisc=0 --traceset=2",real-exp/results/abc-fifo-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=1 --traceset=2",real-exp/results/copa-codel-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=1 --traceset=2",real-exp/results/copa-wirtc-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=0 --traceset=2",real-exp/results/copa-fifortc-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=0 --traceset=2 --fastrtx=1",real-exp/results/copa-fastfrtc-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=1 --traceset=2 --fastrtx=1",real-exp/results/copa-fastcrtc-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=0 --traceset=2",real-exp/results/copa-fifo-'${trace}'.log' >> ${fname}
#     for delta in 1 2 4 8
#     do
#         echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=300 --fps=25 --interval=40 --trace=real-exp/traces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=3 --mode=1 --qdisc=0 --traceset=2 --copadelta='${delta}'",real-exp/results/copa-fast'${delta}'-'${trace}'.log' >> ${fname}
#     done
# done

# fname="gcc-static-exp.conf"
# rm ${fname}
# for trace in `ls static-exp/traces`
# do
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=static-exp/traces/'${trace}' --node_number=10 --cc_create=false --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=0 --traceset=1",static-exp/results/gcc-fifo-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=static-exp/traces/'${trace}' --node_number=10 --cc_create=false --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=1 --traceset=1",static-exp/results/gcc-codel-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=static-exp/traces/'${trace}' --node_number=10 --cc_create=false --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=1 --traceset=1",static-exp/results/gcc-wirtc-'${trace}'.log' >> ${fname}
# done

# fname="tcp-static-exp.conf"
# rm ${fname}
# for trace in `ls static-exp/traces`
# do
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=static-exp/traces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=3 --mode=1 --qdisc=0 --traceset=1",static-exp/results/copa-fast-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=static-exp/traces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=2 --queue=2 --mode=1 --qdisc=0 --traceset=1",static-exp/results/abc-fifo-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=static-exp/traces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=1 --traceset=1",static-exp/results/copa-codel-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=200 --fps=25 --interval=40 --trace=static-exp/traces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=1 --traceset=1",static-exp/results/copa-wirtc-'${trace}'.log' >> ${fname}
# done

# fname="tcp-trace-exp.conf"
# rm ${fname}
# for trace in `ls static-exp/testtraces`
# do
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=72 --fps=25 --interval=40 --trace=static-exp/testtraces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=1 --traceset=1",static-exp/results/copa-codel-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=72 --fps=25 --interval=40 --trace=static-exp/testtraces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=1 --traceset=1",static-exp/results/copa-wirtc-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=72 --fps=25 --interval=40 --trace=static-exp/testtraces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=0 --traceset=1",static-exp/results/copa-fifortc-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=72 --fps=25 --interval=40 --trace=static-exp/testtraces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=0 --traceset=1 --fastrtx=1",static-exp/results/copa-fastfrtc-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=72 --fps=25 --interval=40 --trace=static-exp/testtraces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=1 --qdisc=1 --traceset=1 --fastrtx=1",static-exp/results/copa-fastcrtc-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=72 --fps=25 --interval=40 --trace=static-exp/testtraces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=2 --queue=2 --mode=1 --qdisc=0 --traceset=1",static-exp/results/abc-fifo-'${trace}'.log' >> ${fname}
#     echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=72 --fps=25 --interval=40 --trace=static-exp/testtraces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=1 --qdisc=0 --traceset=1",static-exp/results/copa-fifo-'${trace}'.log' >> ${fname}
#     for delta in 1 2 4 8
#     do
#         echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=72 --fps=25 --interval=40 --trace=static-exp/testtraces/'${trace}' --node_number=10 --cc_create=true --cc_tcp_choice=1 --queue=3 --mode=1 --qdisc=0 --traceset=1 --copadelta='${delta}'",static-exp/results/copa-fast'${delta}'-'${trace}'.log' >> ${fname}
#     done
# done

fname="tcp-wifi-exp.conf"
rm ${fname}
for trace in `ls static-exp/testtraces`
do
    echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=60 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=0 --qdisc=1 --qsize=2 --wifi_same_ap=1 --wifi_compete=1 --cut='${cut}'",tcp-wifi-exp/wirtc-codel-queue-20-'${cut}'.log' >> ${fname}
    echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=60 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=1 --mode=0 --qdisc=0 --qsize=2 --wifi_same_ap=1 --wifi_compete=1 --cut='${cut}'",tcp-wifi-exp/wirtc-fifo-queue-20-'${cut}'.log' >> ${fname}
    echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=60 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=0 --qdisc=1 --qsize=2 --wifi_same_ap=1 --wifi_compete=1 --cut='${cut}'",tcp-wifi-exp/copa-codel-queue-20-'${cut}'.log' >> ${fname}
    echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=60 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=0 --mode=0 --qdisc=0 --qsize=2 --wifi_same_ap=1 --wifi_compete=1 --cut='${cut}'",tcp-wifi-exp/copa-fifo-queue-20-'${cut}'.log' >> ${fname}
    echo '"scratch/fec-emulate --policy=rtx --ddl=1000 --vary=0 --cc=1 --duration=60 --fps=25 --interval=40 --trace=traces/k30-3600.log --node_number=20 --cc_create=true --cc_tcp_choice=1 --queue=3 --mode=0 --qdisc=0 --qsize=2 --wifi_same_ap=1 --wifi_compete=1 --cut='${cut}'",tcp-wifi-exp/fast-fifo-queue-20-'${cut}'.log' >> ${fname}
done
    
