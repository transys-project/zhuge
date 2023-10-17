{
    split($1, ret, "[-/k]")
    cc = ret[1]
    aqm = ret[2]
    k = ret[4]
    if ($3 >= 0) {
        if ($3 > 20000) {resultRtt[cc"-"aqm][k] += 20000}
        else {resultRtt[cc"-"aqm][k] += $3}
        cntRtt[cc"-"aqm][k] += 1
    }
}

END {
    printf("k")
    for (aqm in resultRtt) {
        printf("\t%s", aqm)
    }
    printf("\n")
    for (k in resultRtt["copa-codel"]) {
        printf("%d", k)
        for (aqm in resultRtt) {
            printf("\t%5d", resultRtt[aqm][k] / cntRtt[aqm][k])
        }
        printf("\n")
    }
}