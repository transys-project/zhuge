NR==1 {
    duration = 100
    rttStart = 0
    fdStart = 0
    fpsStart = 0
    rttEnd = 0
    fdEnd = 0
    fpsEnd = 0
    fd_sh = 400
    fps_sh = 10
    mints = 0
    rtt_th = 200
    window = 20000  # 20sec
    interval = 40   # 40ms = 25fps
    split(FILENAME, res, "[-./k]")
    bwcut = int(res[5])
    ratio = int(res[4])
    rttcount = 0
    fdcount = 0
    fpscount = 0
    totalcount = duration * 1000 / interval
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
    if ($5 > rtt_th && $3 >=  60000 + bwcut * 1000 && $3 <  60000 + bwcut * 1000 + window){
        rttcount += 1
    }
}

END {
    # calculate rtt 
    print FILENAME, "rtt", rttcount/totalcount

    # calculate fd 
    # if all frame lost ?
    for (var in rcv) {
        if ( var > ( 60000 + bwcut * 1000  - window ) / interval && var < ( 60000 + bwcut * 1000  + window ) / interval ){
            if((rcv[var] - snd[var]) > fd_sh){
                fdcount += 1
            }
        }
    }
    print FILENAME, "fd", fdcount/totalcount
    
    # calculate fps 
    for (ts = mints + 1; ts <= maxts - 1; ts++) {
        if (ts in count && ts > ( 60000 + bwcut * 1000  - window ) / 1000 && ts < ( 60000 + bwcut * 1000  + window ) / 1000 ) {
            if( count[ts] < fps_sh ) {
                fpscount += 1
            } 
        }
    }
    print FILENAME, "fps", fpscount/duration
}