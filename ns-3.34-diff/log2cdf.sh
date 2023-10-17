awk -vstep=$(awk 'END{printf("%.4f\n", NR/10000)}' $1 | bc) 'BEGIN{
    cnt = 1
}
    
{
    sum += $1
    if (NR >= int(cnt*step)) {
        if ($0 != last) {
            printf("%d\t%.4f\n", $0, 1 - cnt / 10000)
            last = $0
        }
        cnt = cnt + 1 + int(NR - cnt*step)
    }
}' $1 > $1.result