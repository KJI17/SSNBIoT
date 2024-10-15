# SSNBIoT
Small Steady-state Performance in Narrowband Internet of Things (NB-IoT) Simulation

This Simulator uses an analytical model to simulated NB-IoT technology on 1 base station using backoff method for 10 minutes (T) and use 1 coverage enhancement (CE) with 200 users (M). A packet will be transmitted to 24 sub carriers (Nsc) randomly within three probabilities to generate uplink packet (Ppkt) 10%, 20%, and 30% running on a maximum transmitted power. Our assumption a Transmitted ACK (TACK) consume 2 slots while a Delivered ACK (DACK) consume 1 slot, and backoff window (W_BO) use 10 slots, where each slot has a time to transmit 5ms. In addition, the graph to be analyzed is the Ppkt sent based on the increase of user (multiple of 5 up to 200) with the resulting packet. There are several components that checked are: Normalized throughput, Average delay, and Resource efficiency.

further reading : https://ieeexplore.ieee.org/abstract/document/9042441

# USAGE
Simply run together with PCG Library C Implementation (pcg-random.org). Gcc/G++ compiler must be installed on your computer.
