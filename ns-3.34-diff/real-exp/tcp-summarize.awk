BEGIN {
    conf[0]="copa-fifo"
    conf[1]="copa-codel"
    conf[2]="abc-fifo"
    conf[3]="copa-fifortc"
    conf[4]="copa-wirtc"
    conf[5]="copa-fast1"
    conf[6]="copa-fast2"
    conf[7]="copa-fast4"
    conf[8]="copa-fast8"

    trace[0]="rest-wifi"
    trace[1]="office-wifi"
    trace[2]="pitree4g"
    trace[3]="xu4g"
    trace[4]="xu5g"
    trace[5]="abc4g"
}

{
    res[$1][$2] = $3
}

END {
    for (tridx = 0; tridx < length(trace); tridx++) {
        # printf("%.4f\t", res["copa-fifo"][trace[tridx]] < res["copa-codel"][trace[tridx]] ? 
        #     res["copa-fifo"][trace[tridx]] : res["copa-codel"][trace[tridx]])
        printf("%.4f\t", tridx < 2 ? res["copa-fifo"][trace[tridx]] : res["copa-codel"][trace[tridx]])
        printf("%.4f\t", res["copa-fast2"][trace[tridx]])
        printf("%.4f\t", res["abc-fifo"][trace[tridx]])
        # printf("%.4f\t", res["copa-fifortc"][trace[tridx]] < res["copa-wirtc"][trace[tridx]] ? 
        #     res["copa-fifortc"][trace[tridx]] : res["copa-wirtc"][trace[tridx]])
        printf("%.4f\t", tridx < 2 ? res["copa-fifortc"][trace[tridx]] : res["copa-wirtc"][trace[tridx]])

        # for (confidx = 0; confidx < length(conf); confidx++) {
        #     printf("%.4f\t", res[conf[confidx]][trace[tridx]])
        # }
        printf("\n")
    }
}