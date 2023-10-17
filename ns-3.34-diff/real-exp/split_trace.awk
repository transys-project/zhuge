BEGIN {
    duration = 300
    suffix = 0
    time = 0
}

{
    if ($1 - time > duration) {
        suffix++
        time = $1
    }
    print $0 > substr(FILENAME, 1, length(FILENAME) - 13)"_"suffix".pitree-trace"
}
