BEGIN {
    aqmidx[0] = "copa-fifo"
    aqmidx[1] = "copa-codel"
    aqmidx[2] = "copa-fast1"
    aqmidx[3] = "abc-fifo"
    aqmidx[4] = "copa-wirtc"
    aqmidx[5] = "copa-fifortc"
    aqmidx[6] = "copa-fastcrtc"
    aqmidx[7] = "copa-fastfrtc"
}

{
    split($1, ret, "[-/k]")
    cc = ret[2]
    aqm = ret[3]
    k = ret[5]
    if ($3 >= 0) {
        resultRtt[cc"-"aqm][k] += $3
        cntRtt[cc"-"aqm][k] += 1
    }
}

END {
    printf("k")
    printf("\t%s", "copa")
    printf("\t%s", "copa-fast")
    printf("\t%s", "abc")
    printf("\t%s", "copa-wirtc")
    # printf("\t%s", "copa-fastwirtc")
    
    printf("\n")
    for (k in resultRtt["copa-wirtc"]) {
        printf("%d", k)
        
        # copa
        res1 = resultRtt["copa-fifo"][k] / cntRtt["copa-fifo"][k]
        res2 = resultRtt["copa-codel"][k] / cntRtt["copa-codel"][k]
        printf("\t%5d", res1 < res2 ? res1 : res2)

        # copa-fastack
        res1 = resultRtt["copa-fast1"][k] / cntRtt["copa-fast1"][k]
        printf("\t%5d", res1)

        # abc
        res1 = resultRtt["abc-fifo"][k] / cntRtt["abc-fifo"][k]
        printf("\t%5d", res1)
 
        # copa-wirtc
        res1 = resultRtt["copa-wirtc"][k] / cntRtt["copa-wirtc"][k]
        res2 = resultRtt["copa-fifortc"][k] / cntRtt["copa-fifortc"][k]
        printf("\t%5d", res1 < res2 ? res1 : res2)

        # # copa-wirtc-fastack
        # res1 = resultRtt["copa-fastfrtc"][k] / cntRtt["copa-fastfrtc"][k]
        # res2 = resultRtt["copa-fastcrtc"][k] / cntRtt["copa-fastcrtc"][k]
        # printf("\t%5d", res1 < res2 ? res1 : res2)


        printf("\n")
    }
}