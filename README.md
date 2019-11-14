# Wireless-Spectrum-Hypervisor
Code for a wireless spectrum hypervisor architecture that abstracts an radio frequency (RF) front-end into a configurable number of virtual RF front-ends.

Reference:
[1] Pereira de Figueiredo, Mennes, R..; Jabandžic, I..; Jiao, X.; Moerman, I.. "A Base-Band Wireless Spectrum Hypervisor for Multiplexing Concurrent OFDM signals". Preprints 2019, 2019100115 (doi: 10.20944/preprints201910.0115.v1).


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
