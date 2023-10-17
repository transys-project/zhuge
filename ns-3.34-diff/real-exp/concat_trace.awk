BEGIN {
    last_ts = 0
    filename = "rest-wifi.pitree-trace"
}

FNR==1 {
    diff = $1 - last_ts - 0.2
    print $1 - diff, $2 >> filename
    last_ts = $1 - diff
    print FILENAME
}

FNR>1 {
    print $1 - diff, $2 >> filename
    last_ts = $1 - diff
}