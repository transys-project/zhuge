/NodeList\/2\/DeviceList\/1\/\$ns3::PointToPointNetDevice\/MacRx/ {
    arrv[$19] = $2
    if ($19 > 65500) {
        nextfile
    }
}

/NodeList\/2\/DeviceList\/2\/\$ns3::PointToPointNetDevice\/TxQueue\/Dequeue/ {
    dept[$19] = $2
    print $2, $19, int ((dept[$19] - arrv[$19]) * 1000)
}