BEGIN {
    conf[0]="gcc-fifo"
    conf[1]="gcc-codel"
    conf[2]="gcc-wirtc"

    trace[0]="rest-wifi"
    trace[1]="office-wifi"
    trace[2]="pitree4g"
    trace[3]="xu4g"
    trace[4]="xu5g"
}

{
    res[$1][$2] = $3
}

END {
    for (tridx = 0; tridx < 5; tridx++) {
        for (confidx = 0; confidx < 3; confidx++) {
            printf("%.4f\t", res[conf[confidx]][trace[tridx]])
        }
        printf("\n")
    }
}