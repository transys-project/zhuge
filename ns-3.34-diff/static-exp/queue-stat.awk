NR==1 {
    min_rtt = 50
    rtt_th = 200
    window = 20000  # 20sec
    interval = 40   # 40ms = 25fps
    split(FILENAME, res, "[-./k]")
    bwcut = int(res[6])
    ratio = int(res[5])
}

/QueueStat/ {
    if ($5 > 1024 * 30 / ratio / 8 * (rtt_th - min_rtt)) {
        if (!start && int($2) > bwcut * interval) {
            start = int($2)
        }
    }
    else {
        if (start && !end) {
            end = int($2)
        }
    }
} 

END {
    print FILENAME, "qdelay", end - start
}