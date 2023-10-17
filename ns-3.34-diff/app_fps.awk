/decode: frame/ {
    if (!mints) {
        mints = int($5/1000)
    }
    count[int($5/1000)] += 1
    maxts = int($5/1000)
} 

END {
    for (ts = mints + 1; ts <= maxts - 1; ts++) {
        if (ts in count) {
            print (count[ts])
        }
    }
}
