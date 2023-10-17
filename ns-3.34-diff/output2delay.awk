/NetworkQueueZhuge/ && /Time/ {
    print $14, $24
    if ($24 > 65500) {
        nextfile
    }
}