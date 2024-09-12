list=("uniform_random" "torus_transpose" "torus_tornado" "shuffle" "bit_rotation" "bit_reverse")
# list=("uniform_random")
# Loop through the list and print each number
for traffic in "${list[@]}"; do
    echo "traffic: $traffic"

    filename="./result/Lab4/4_4_4_torus/goal/$traffic.txt"
    # filename="/home/wanz/gem5/result/Lab4/4_4_4_torus/goal/test.txt"

    echo > "$filename"
    for injectionrate in $(seq 0.01 0.02 1); do
        echo "Current injection rate: $injectionrate" >> "$filename"

        # Perform your operations here

        ./build/NULL/gem5.opt \
        configs/example/garnet_synth_traffic.py \
        --network=garnet --num-cpus=64 --num-dirs=64 \
        --topology=Torus --dims="4,4,4" --mesh-rows=8 \
        --routing-algorithm=4 \
        --synthetic=$traffic \
        --sim-cycles=20000 --injectionrate=$injectionrate --vcs-per-vnet=4 --garnet-deadlock-threshold=50000

        grep "packets_injected::total" m5out/stats.txt | sed 's/system.ruby.network.packets_injected::total\s*/packets_injected = /' >> "$filename"
        grep "packets_received::total" m5out/stats.txt | sed 's/system.ruby.network.packets_received::total\s*/packets_received = /' >> "$filename"
        grep "average_packet_queueing_latency" m5out/stats.txt | sed 's/system.ruby.network.average_packet_queueing_latency\s*/average_packet_queueing_latency = /' >> "$filename"
        grep "average_packet_network_latency" m5out/stats.txt | sed 's/system.ruby.network.average_packet_network_latency\s*/average_packet_network_latency = /' >> "$filename"
        grep "average_packet_latency" m5out/stats.txt | sed 's/system.ruby.network.average_packet_latency\s*/average_packet_latency = /' >> "$filename"
        grep "average_hops" m5out/stats.txt | sed 's/system.ruby.network.average_hops\s*/average_hops = /' >> "$filename"

        packets_injected=$(grep "packets_injected::total" m5out/stats.txt | sed 's/system.ruby.network.packets_injected::total\s*/packets_injected = /' | awk '{print $3}')
        packets_received=$(grep "packets_received::total" m5out/stats.txt | sed 's/system.ruby.network.packets_received::total\s*/packets_received = /' | awk '{print $3}')

        receptionrate=$(echo "scale=4; $packets_received / $packets_injected * $injectionrate" | bc)

        # Save the packets_injected value into a file
        echo "reception_rate = $receptionrate                  (Count/Cycle)" >> "$filename"
        echo "" >> "$filename"
    done
done
