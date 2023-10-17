fname="latency-trace.conf"
rm ${fname}
for trace in `ls traces`
do
    # echo '"fec-emulate --trace=traces/'${trace}' --ccas=0 --queues=1 --mode=1 --qdisc=0 --isPcapEnabled",logs/1/0/'${trace}'/output.log' >> ${fname}
    echo '"fec-emulate --trace=traces/'${trace}' --ccas=6 --queues=1 --mode=1 --qdisc=0 --isPcapEnabled",logs/1/6/'${trace}'/output.log' >> ${fname}
done
