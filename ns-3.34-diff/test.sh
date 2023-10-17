awk -F '[ dt]' '{
    if(!exist[$107]) {
        exist[$107] = 1;
        maxid = $107;
        ts[$107] = $2
    };
} 
END{
    for(id=0; id<=maxid; id++){
        print id,ts[id]
    }
}' $1/snd-udp.tr > $1/encode.log

awk 'NR==FNR{snd[$1]=$2} NR>FNR{if(exist[$3]==0){rcv[$3]=$5/1000;exist[$3]=1;}} END{for(seq in rcv){print snd[seq], rcv[seq] - snd[seq]}}' $1/encode.log $1/decode.log | sort -nk1 > $1/rtt_wifi.log
