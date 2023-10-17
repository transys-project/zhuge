NR==1 {
    rttStart = 0
    fdStart = 0
    fpsStart = 0
    rttEnd = 0
    fdEnd = 0
    fpsEnd = 0
    fd_sh = 400
    fps_sh = 10
    mints = 0
    cwndState = 0
    rtt_th = 200
    window = 20000  # 20sec
    interval = 40   # 40ms = 25fps
    split(FILENAME, res, "[-./k]")
    if ("fast" == res[3]) {
        rtt_th -= 16
    }
    bwcut = int(res[6])
    ratio = int(res[5])
    lastTime = 0
}

NR==FNR {
    if ($2 == "Time" && $3 >= bwcut * interval + window && $3 <  bwcut * interval + 2 * window) {
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

/send frame/ {
    snd[$5] = $10
} 

/decode: frame/ {
    rcv[$3] = $5
    if (!mints) {
        mints = int($5/1000)
    }
    count[int($5/1000)] += 1
    maxts = int($5/1000)
}

/discard frame/ {
    rcv[$3] = $5
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

    # calculate fd deviations
    # if all frame lost ?
    lbound = bwcut - window / interval
    rbound = bwcut + window / interval
    for (var in rcv) {
        fid = int(var)
        # printf("%d, %d, %d, %d, %d\n", var, lbound, rbound, int(var) > lbound, int(var) < rbound)
        if (fid > (bwcut - window / interval) && fid < (bwcut + window / interval)) {
            if (rcv[var] - snd[var] > fd_sh) {
                if (!fdStart) {
                    fdStart = fid;
                }                
            }
            else {
                if (fdStart > 0 && (!fdEnd)) {
                    fdEnd = fid;
                    print FILENAME, "fd", (fdEnd - fdStart) * interval
                }
            }
        }
    }
    if (fdStart == 0){
        print FILENAME, "fd", 0
    }
    else if (fdStart > 0 && (!fdEnd)){
         print FILENAME, "fd", window
    }
    
    # calculate fps deviations
    for (ts = mints + 1; ts <= maxts - 1; ts++) {
        if (ts in count && ts > (bwcut * interval) / 1000 && ts < (bwcut * interval + window) / 1000) {
            if (count[ts] < fps_sh) {
                if (fpsStart == 0) {
                    fpsStart = ts;
                }
                fpsEnd = ts;
            } 
        }
    }
    if (fpsStart == 0) {
        print FILENAME, "fps", 0
    }
    else if (fpsStart > 0) {
        print FILENAME, "fps", (fpsEnd - fpsStart) * 1000
    }

}