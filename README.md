# Wireless-Spectrum-Hypervisor
Code for a wireless spectrum hypervisor architecture that abstracts an radio frequency (RF) front-end into a configurable number of virtual RF front-ends.

Reference:
[1] Pereira de Figueiredo, F. A.; Mennes, R.; Jabandžic, I.; Jiao, X.; Moerman, I.; "A Base-Band Wireless Spectrum Hypervisor for Multiplexing Concurrent OFDM signals". Preprints 2019, 2019100115 (doi: 10.20944/preprints201910.0115.v1).

**Abstract**
The next generation of wireless and mobile networks will have to handle a significant increase in traffic load compared to the actual one. This situation calls for novel ways to increase the spectral efficiency. Therefore in this paper, we propose a wireless spectrum hypervisor architecture that abstracts a radio frequency (RF) front-end into a configurable number of virtual RF front-ends. The proposed architecture has the ability to enable flexible spectrum access in existing wireless and mobile networks, which is a challenging task due to the limited spectrum programmability, $i.e.$, the capability a system has to change the spectral properties of a given signal to fit an arbitrary frequency allocation. The main goal of the proposed approach is to improve spectral efficiency by efficiently using vacant gaps in congested spectrum-bandwidths or adopting network densification through infrastructure sharing. We demonstrate mathematically how our proposed approach works and present several simulation results proving its functionality and efficiency. Additionally, we designed and implemented an open-source and free proof of concept prototype of the proposed architecture, which can be used by researchers and developers to run experiments or extend the concept to other applications. We present several experimental results used to validate the proposed prototype. We demonstrate that the prototype can easily handle up to 12 concurrent physical layers.


# Installation steps

Install gnuradio FROM SOURCE!!!. Now only gnuradio 3.7.10.1 is verified. Newer version may cause issues.

Then:
```
git clone https://github.com/zz4fap/Wireless-Spectrum-Hypervisor.git
cd Wireless-Spectrum-Hypervisor
mkdir build
cd build
cmake ../
make
```
