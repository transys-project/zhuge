NR==1 {
    rttStart == 0
    cwndState = 0
    rtt_th = 200
    window = 20000  # 20sec
    interval = 40   # 40ms = 25fps
    split(FILENAME, res, "[-.k]")
    bwcut = int(res[5])
    ratio = int(res[4])
    lastTime = 0
}

NR==FNR {
    if ($2 == "Time" && $3 >= bwcut * interval + window && $3 < bwcut * interval + 2 * window) {
        if ($3 == lastTime) {
            next
        }
        lastTime = $3
        if ($1 == "[PacketSenderUdp]") {
            cwnd += int(($9 - cwnd) / 8)
        }
        else if ($1 == "[PacketSenderTcp]") {
            cwnd += int(($10 - cwnd) / 8)
        }
        stableRtt += int(($5 - stableRtt) / 8)
    }
    next
}

/Sender/ {
    if ($3 == lastTime) {
        next
    }
    lastTime = $3
    if ($3 > bwcut * interval - window && $3 < bwcut * interval) {
        if ($1 == "[PacketSenderUdp]") {
            cwndPrev = cwndPrev > $9 ? cwndPrev : $9;
        }
        else if ($1 == "[PacketSenderTcp]") {
            cwndPrev = cwndPrev > $10 ? cwndPrev : $10;
        }
    }
    else if ($3 >= bwcut * interval && $3 < bwcut * interval + window) {
        # calculate cwnd back to normal
        if ($1 == "[PacketSenderUdp]" && 
            ($9 < cwnd || $9 / 1.25 < cwndPrev / ratio || $9 <= 1448 * 4) && 
            cwndState == 0) {
            print FILENAME, "cwnd", $3 - bwcut * interval;
            cwndState = 1
        }
        else if ($1 == "[PacketSenderTcp]" && 
            ($10 < cwnd || $10 / 1.25 < cwndPrev / ratio || $10 <= 1448 * 4) && 
            cwndState == 0) {
            print FILENAME, "cwnd", $3 - bwcut * interval;
            cwndState = 1
        }

        # calculate RTT deviations
        if ($5 > rtt_th && rttStart == 0) {
            rttStart = $3
        }
        
        if (($5 < rtt_th) && rttStart > 0) {
            print FILENAME, "rtt", $3 - rttStart;
            rttStart = -1
        }
    }
}

END {
    if (cwndState == 0) {
        print FILENAME, "cwnd", window
    }
    
    if (rttStart == 0) {
        print FILENAME, "rtt", 0
    }
    else if (rttStart > 0) {
        print FILENAME, "rtt", window
    }
}