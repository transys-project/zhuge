Achieving Consistent Low Latency for Wireless Real-Time Communications with the Shortest Control Loop (SIGCOMM 2022)

Sorry for the late release of the codes.
This repo contains considerable amount of the codes from [Hairpin](https://github.com/hkust-spark/hairpin), and some later bug fixes are only put into effect in the other Hairpin project.
We apologize for the inconvenience that brings to you. 

## How to understand the codes

The main function of the codes is in the `ns-3.34/scratch/fec-emulate.cc`, and the main module for Zhuge is in `ns-3.34/src/zhuge/`.
One important thing about this code is that we provide a ns-3 simulator for real-time communications!
The simulator is in the `ns-3.34/src/bitrate-ctrl`, which can provide an end-to-end simulation for low-latency video streaming.
We used this simulator in this Zhuge project and also the Hairpin project mentioned above.
We're currently trying to separate the simulator itself from the contributions of others, but that is expected to take some time.

## Install dependency
```
sudo apt install build-essential libboost-all-dev
wget https://www.nsnam.org/releases/ns-allinone-3.34.tar.bz2
tar xjf ns-allinone-3.34.tar.bz2
mv ns-allinone-3.34/ns-3.34 .
mv ns-3.34-diff/* ns-3.34/
patch -s -p0 < ./ns-diff.patch

```

## Configure before building!
For ns-3.33 and below, to use c++14 and above features, should configure like this: (More information: C++ standard configuration in build system (#352) · Issues · nsnam / ns-3-dev · GitLab)

```
cd ns-3.34/
LDFLAGS="-lboost_filesystem -lboost_system" ./waf configure --cxx-standard=-std=c++17
```

## Figure 4
This branch contains the codes to reproduce the motivating results in Figure 4.
- Step 1: Generate the configuration file for Figure 4: `zsh generate_conf_fig4.sh`.
- Step 2: Run the codes with given configurations:
`python run_ns3.py --conf motiv-static.conf`. The results will appear in `static-exp/results/`
- Step 3: Process results to stats: 
```
cd static-exp/results
ls | xargs -i awk -f ../../../../../output-stat.awk {} {} > ../motivation-results-summary.log
awk -f ../../../../../summary-stat.awk ../motivation-results-summary.log > ../motivation-results-plot.log
``` 


## Figure 14 / 15
- Step 3: Process results to stats:
```
cd static-exp/
zsh summarize.sh
```
