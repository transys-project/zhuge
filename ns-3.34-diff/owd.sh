grep 'NodeList/0/DeviceList/0/$ns3::PointToPointNetDevice/TxQueue/Dequeue' $1/fec-emulation.tr > $1/snd.tr
grep 'NodeList/2/DeviceList/0/$ns3::WifiNetDevice/Phy/State/RxOk' $1/third.tr | grep '10.1.1.1' > $1/rcv.tr

awk -F '[ dt]' '{if ($2<180) {print ($2, $78)}}' $1/snd.tr > $1/snd-time.log
awk -F '[ dt]' '{if ($2<180) {for (i=99; i<=NF; i=i+132) {print $2, $i}}}' $1/rcv.tr > $1/rcv-time.log
awk 'NR==FNR{snd[$2]=$1} NR>FNR{rcv[$2]=$1} END{for(seq in rcv){print snd[seq], rcv[seq] - snd[seq]}}' $1/snd-time.log $1/rcv-time.log | sort -nk1 > $1/rtt_wifi.log
# awk 'BEGIN{time=0} 
#      {if($1>time) {print($1,$2); time=$1}}' $1/rtt_wifi.log > rtt_wifi.log