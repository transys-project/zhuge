{
    split($1, ret, "[-k]")
    cc = ret[2]
    aqm = ret[1]
    k = ret[4]
    
    if($2 > 0){
        resultfdelay[aqm][cc][k] += $2
        cntfdelay[aqm][cc][k] += 1
    }
    
}

END {
    
    for (aqm in resultfdelay) {
        printf("======== Frame Delay: %s, k | Cubic, Bbr, Copa, Gcc ========\n", aqm)
        for (k in resultfdelay[aqm]["abc"]) {
            printf("%2d\t%5d\n", k, resultfdelay[aqm]["abc"][k] / cntfdelay[aqm]["abc"][k])
        }
        for (k in resultfdelay[aqm]["fast"]) {
            printf("%2d\t%5d\n", k, resultfdelay[aqm]["fast"][k] / cntfdelay[aqm]["fast"][k])
        }
    }
}