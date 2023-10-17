import numpy as np
from numpy.lib.function_base import place
from numpy.testing._private.utils import measure

np.random.seed(1999)

time_intvl = 40
pkt_rate = 8    # 1 1500B packet every time_intvl -> 12Mbps
min_rtt = 20

with open('trace-step.log', 'r') as f:
    downlink_queue = []
    feedback_loop = []
    frame_id = 0
    allow_adjust = 0
    dequeue_intvl = time_intvl / pkt_rate
    last_ts = 0

    while True:
        line = f.readline()
        if not line:
            break

        ts, delay = int(line.split()[0]), int(line.split()[1])
        while ts > (frame_id + 1) * time_intvl:
            # sending rate update
            pkt_rate += frame_id % 3 == 0
            while len(feedback_loop) > 0 and feedback_loop[0][0] <= ts:
                if feedback_loop[0][1] > 100 and ts >= allow_adjust:
                   pkt_rate /= 2
                   allow_adjust = ts + feedback_loop[0][1]
                feedback_loop.pop(0)

            # send new frame
            frame_id += 1
            for _ in range(pkt_rate):
                downlink_queue.append([frame_id, frame_id * time_intvl])
        
        if len(downlink_queue) > 0:
            actual_rtt = min_rtt + delay + ts - downlink_queue[0][1]
            
            # passive feedback
            effective_time = min_rtt/2 + delay + ts
            measured_rtt = actual_rtt

            # active feedback
            # dequeue_intvl += (ts - last_ts - dequeue_intvl) * 0.25
            # effective_time = min_rtt/2 + ts
            # measured_rtt = min_rtt + delay + len(downlink_queue) * int(dequeue_intvl) + ts - downlink_queue[-1][1]

            feedback_loop.append([effective_time, measured_rtt])
            print("%d %d" % (downlink_queue[0][0], actual_rtt))
            downlink_queue.pop(0)
        last_ts = ts


        # alpha = float(line.split()[0])
        # dequeue_intvl = np.random.geometric(alpha, size=time_intvl)

        # control_loop.append([time, ])
        # for control in control_loop:
