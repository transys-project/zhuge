import numpy as np
from math import log2
import os
import argparse


maxDiff = 512
maxExp = 9


def processEst (fname):
    estDelay = {}
    with open (fname, 'r') as f:
        lastEstId = 0
        for line in f.readlines ():
            line = line.split ()
            estId = int (line[1])
            if (estId < lastEstId - 32768):
                break
            if int (line[0]) >= 27 and int (line[0]) <= 30:
                # The estimation has minor bugs that will concentrate on 27-30
                # We do not have time to debug it, just ignore here.
                continue
            estDelay[estId] = int (line[0])
            lastEstId = estId
    return estDelay


def processReal (fname, estDelay):
    diff = np.zeros (maxDiff + 1, dtype=int)
    mapRes = np.zeros ((maxExp + 1, maxExp + 1), dtype=int)
    with open (fname, 'r') as f:
        lastRealId = 0
        for line in f.readlines ():
            line = line.split ()
            realId = int (line[1])
            realDelay = int (line[2])
            if (realId < lastRealId):
                break
            if (realId not in estDelay.keys ()):
                continue
            diff[min (abs (max (1, estDelay[realId]) - max (1, realDelay)), maxDiff)] += 1
            mapX = min (int (log2 (max (1, realDelay))), maxExp)
            mapY = min (int (log2 (max (1, estDelay[realId]))), maxExp)
            mapRes[mapX, mapY] += 1
            lastRealId = realId
    return diff, mapRes


if __name__ == "__main__":
    parser = argparse.ArgumentParser ()
    parser.add_argument ('--tracePrefix')
    parser.add_argument ('--traceClass')
    args = parser.parse_args ()

    totalDiff = np.zeros (maxDiff + 1, dtype=int)
    totalMapRes = np.zeros ((maxExp + 1, maxExp + 1), dtype=float)
    for trace in os.listdir (args.tracePrefix):
        if not os.path.isdir (os.path.join (args.tracePrefix, trace)):
            continue
        if args.traceClass not in trace:
            continue
        print (trace)
        if not os.path.exists (os.path.join (args.tracePrefix, trace, "real.tr")):
            os.system ("cd " + os.path.join (args.tracePrefix, trace) + 
                " && awk -f ../../../../pcap2delay.awk pcap.tr > real.tr")
        if not os.path.exists (os.path.join (args.tracePrefix, trace, "est.tr")):
            os.system ("cd " + os.path.join (args.tracePrefix, trace) + 
                " && awk -f ../../../../output2delay.awk output.log > est.tr")
        estDelay = processEst (os.path.join (args.tracePrefix, trace, "est.tr"))
        diff, mapRes = processReal (os.path.join (args.tracePrefix, trace, "real.tr"), estDelay)
        totalDiff += diff
        totalMapRes += mapRes

    np.savetxt (os.path.join (args.tracePrefix, args.traceClass + "-totalDiff.log"), 
        (np.cumsum (totalDiff) / np.sum (totalDiff)).T, fmt="%.4f")
    for y in range (totalMapRes.shape[1]):
        totalMapRes[:, y] = totalMapRes[:, y] / np.sum (totalMapRes[:, y])
    np.savetxt (os.path.join (args.tracePrefix, args.traceClass + "-totalMapRes.log"), totalMapRes, fmt="%.4f")
