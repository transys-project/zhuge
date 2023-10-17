{
    split($1, ret, "[-/k]")
    cc = ret[1]
    aqm = ret[2]
    k = ret[4]
    resultRtt[cc"-"aqm][k] += $3
    cntRtt[cc"-"aqm][k] += 1
}

END {
    printf("k")
    for (aqm in resultRtt) {
        printf("\t%s", aqm)
    }
    printf("\n")
    for (k in resultRtt["gcc-wirtc"]) {
        printf("%d", k)
        for (aqm in resultRtt) {
            printf("\t%5f", resultRtt[aqm][k] / cntRtt[aqm][k])
        }
        printf("\n")
    }
}