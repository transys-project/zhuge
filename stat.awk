{
    count[$1] ++;
    if (maxrtt[$1] < $2) {
        maxrtt[$1] = $2
    }
}

END{
    for (idx=0; idx<=$1; idx++) {
        printf("%d\t%d\t%d\n", idx, count[idx], maxrtt[idx])
    }
}