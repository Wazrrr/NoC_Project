echo "topology: Mesh_XY"
filename="./result/Lab3/Task1/Mesh_XY.txt"

echo > "$filename"
for injectionrate in $(seq 0.01 0.02 1); do
    echo "Current injection rate: $injectionrate" >> "$filename"

    # Perform your operations here

    ./build/NULL/gem5.opt \
    configs/example/garnet_synth_traffic.py \
    --network=garnet --num-cpus=64 --num-dirs=64 \
    --topology=Mesh_XY --mesh-rows=8 \
    --routing-algorithm=0 \
    --inj-vnet=0 --synthetic=uniform_random \
    --sim-cycles=20000 --injectionrate=$injectionrate \

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

echo "topology: Ring"
filename="./result/Lab3/Task1/Ring.txt"

echo > "$filename"
for injectionrate in $(seq 0.01 0.02 1); do
    echo "Current injection rate: $injectionrate" >> "$filename"

    # Perform your operations here

    ./build/NULL/gem5.opt \
    configs/example/garnet_synth_traffic.py \
    --network=garnet --num-cpus=64 --num-dirs=64 \
    --topology=Ring --mesh-rows=8 \
    --routing-algorithm=0 \
    --inj-vnet=0 --synthetic=uniform_random \
    --sim-cycles=20000 --injectionrate=$injectionrate \

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
