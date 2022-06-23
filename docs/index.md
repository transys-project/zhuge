
## Abstract

Real-time communication (RTC) applications like video conferencing or cloud gaming require consistent low latency to provide a seamless interactive experience. However, wireless networks including WiFi and cellular, albeit providing a satisfactory median latency, drastically degrade at the tail due to frequent and substantial wireless bandwidth fluctuations. We observe that the control loop for the sending rate of RTC applications is inflated when congestion happens at the wireless access point (AP), resulting in untimely rate adaption to wireless dynamics. Existing solutions, however, suffer from the inflated control loop and fail to quickly adapt to bandwidth fluctuations. In this paper, we propose Zhuge, a pure wireless AP based solution that reduces the control loop of RTC applications by separating congestion feedback from congested queues. We design a Fortune Teller to precisely estimate per-packet wireless latency upon its arrival at the wireless AP. To make Zhuge deployable at scale, we also design a Feedback Updater that translates the estimated latency to comprehensible feedback messages for various protocols and immediately delivers them back to senders for rate adaption. Trace-driven and real-world evaluation shows that Zhuge reduces the ratio of large tail latency and RTC performance degradation by 17% to 95%.

## Paper

### Achieving Consistent Low Latency for Wireless Real-Time Communications with the Shortest Control Loop

Zili Meng, Yaning Guo, Chen Sun, Bo Wang, Justine Sherry, Hongqiang Harry Liu, Mingwei Xu<br>
Proceedings of the 2022 ACM SIGCOMM Conference<br>
[[PDF]](https://zilimeng.com/papers/zhuge-sigcomm22.pdf)

### Citation

```
@inproceedings{meng-sigcomm2022,
  title = {{Achieving Consistent Low Latency for Wireless Real Time Communications with the Shortest Control Loop}},
  author = {Meng, Zili and Guo, Yaning and Sun, Chen and Wang, Bo and Sherry, Justine and Liu, Hongqiang Harry and Xu, Mingwei},
  year = {2022},
  booktitle = {Proceedings of the 2022 Conference of the ACM Special Interest Group on Data Communication (SIGCOMM)},
  publisher = {ACM},
  address = {New York, NY, USA}
}
```
## Code
[GitHub](https://github.com/transys-project/zhuge/)

## Contact

For any questions, please send an email to [zilim@ieee.org](mailto:zilim@ieee.org).

<script src="//t1.extreme-dm.com/f.js" id="eXF-zilimeng-0" async defer></script>
