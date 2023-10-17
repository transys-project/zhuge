# frame delay
/send frame/ {
    snd[$5] = $10
} 

/decode: frame/ {
    rcv[$3] = $5
}

/discard frame/ {
    rcv[$3] = $5
}

END {
    for (var in rcv) {
        print var, rcv[var] - snd[var]
    }
}
