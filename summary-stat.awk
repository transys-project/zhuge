{
    split($1, ret, "[-k]")
    cc = ret[1]
    aqm = ret[2]
    k = ret[4]
    if ($2 == "rtt") {
        resultRtt[cc"-"aqm][k] += $3
        cntRtt[cc"-"aqm][k] += 1
    }
    else if ($2 == "cwnd") {
        resultCwnd[cc"-"aqm][k] += $3
        cntCwnd[cc"-"aqm][k] += 1
    }
}

END {
    print "======== Rtt ========"
    for (var in resultRtt) {
        for (k in resultRtt[var]) {
            print var, k, resultRtt[var][k] / cntRtt[var][k]
        }
    }
    print "======== cWnd ========"
    for (var in resultCwnd) {
        for (k in resultCwnd[var]) {
            print var, k, resultCwnd[var][k] / cntCwnd[var][k]
        }
    }
}