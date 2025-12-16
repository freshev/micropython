#------------------
#---- scanner -----
#------------------
start_freq = 387
stop_freq = 464
freq = start_freq

import cc1101
c1 = cc1101.cc1101(0)
c1.set_rx_bw(58)
c1.set_rx_freq(freq)

freq = start_freq
while freq < stop_freq:
    c1.set_mhz(freq); rssi = c1.get_rssi(); print("%.1f" % freq, rssi); freq = freq + 0.1

