import os
import re
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Function to parse a single file and extract data
def parse_file(file_path):
    injection_rates = []
    rejection_rates = []
    packet_received = []
    average_latency = []

    with open(file_path, "r") as file:
        for line in file:
            if line.startswith("Current injection rate:"):
                rate = float(re.findall(r"[-+]?\d*\.\d+|\d+", line)[0])
                injection_rates.append(rate)
            elif "packets_received =" in line:
                received = int(re.findall(r"[-+]?\d*\.\d+|\d+", line)[0])
                packet_received.append(received)
            elif "average_packet_latency =" in line:
                latency = float(re.findall(r"[-+]?\d*\.\d+|\d+", line)[0])
                average_latency.append(latency)
            elif "reception_rate =" in line:
                rejection_rate = float(
                    re.findall(r"[-+]?\d*\.\d+|\d+", line)[0]
                )
                rejection_rates.append(rejection_rate)

    return injection_rates, packet_received, average_latency, rejection_rates


# Function to extract title from file name
def extract_title(file_name):
    return file_name.split("/")[-2]


def find_the_most_reception_rate(file_path):
    (
        injection_rates,
        packet_received,
        average_latency,
        rejction_rates,
    ) = parse_file(file_path)
    reception_rates = [i / 64 / 10000 for i in packet_received]
    max_reception_rate = max(reception_rates)
    return max_reception_rate


list = [
    "uniform_random",
    "torus_transpose",
    "torus_tornado",
    "shuffle",
    "bit_rotation",
    "bit_reverse",
]

list_routing = ["algorithm0", "dor", "goal", "min_ad"]
# List of file paths to process
# i want to interate all routing algorithms and find the most packets received in each traffic pattern
algorithm0_traffic_max_reception_rates = []
dor_traffic_max_reception_rates = []
goal_traffic_max_reception_rates = []
min_ad_traffic_max_reception_rates = []

for routing_algorithm in list_routing:
    for traffic in list:
        file_path = "result/Lab4/4_4_4_torus/" + routing_algorithm + "/" + traffic + ".txt"

        if routing_algorithm == "algorithm0":
            algorithm0_traffic_max_reception_rates.append(
                (traffic, find_the_most_reception_rate(file_path))
            )
        elif routing_algorithm == "dor":
            dor_traffic_max_reception_rates.append(
                (traffic, find_the_most_reception_rate(file_path))
            )
        elif routing_algorithm == "goal":
            goal_traffic_max_reception_rates.append(
                (traffic, find_the_most_reception_rate(file_path))
            )
        elif routing_algorithm == "min_ad":
            min_ad_traffic_max_reception_rates.append(
                (traffic, find_the_most_reception_rate(file_path))
            )

# i want to plot a bar chart to compare the most reception rate between different routing algorithms

# Extract traffic patterns and max reception rates
traffic_patterns = [t[0] for t in algorithm0_traffic_max_reception_rates]
algorithm0_rates = [t[1] for t in algorithm0_traffic_max_reception_rates]
dor_rates = [t[1] for t in dor_traffic_max_reception_rates]
goal_rates = [t[1] for t in goal_traffic_max_reception_rates]
min_ad_rates = [t[1] for t in min_ad_traffic_max_reception_rates]

# Define bar width and positions
bar_width = 0.2
index = np.arange(len(traffic_patterns))

colors = {
    "TABLE_": "#b4b5b4",  # Blue
    "DOR": "#8a8b8c",  # Orange
    "GOAL": "#a21d34",  # Green
    "Min_AD": "#535453",  # Red
}

# Create the bar chart
plt.figure(figsize=(10, 6))
plt.bar(
    index, algorithm0_rates, bar_width, label="TABLE_", color=colors["TABLE_"]
)
plt.bar(
    index + bar_width, dor_rates, bar_width, label="DOR", color=colors["DOR"]
)
plt.bar(
    index + 2 * bar_width,
    min_ad_rates,
    bar_width,
    label="Min_AD",
    color=colors["Min_AD"],
)
plt.bar(
    index + 3 * bar_width,
    goal_rates,
    bar_width,
    label="GOAL",
    color=colors["GOAL"],
)
# Labeling
plt.xlabel("Traffic Patterns")
plt.ylabel("Max Reception Rate")
plt.title(
    "Comparison of Max Reception Rates Across Different Routing Algorithms"
)
plt.xticks(index + bar_width * 1.5, traffic_patterns)
plt.legend()

# Customize the grid
plt.grid(axis="x", visible=False)  # Hide vertical gridlines
plt.grid(
    axis="y", which="major", linestyle="--", color="grey"
)  # Show horizontal gridlines
plt.gca().set_axisbelow(True)  # Ensure gridlines are behind the bars

plt.tight_layout()

# Save the plot as a PNG file
plt.savefig("global_results/max_reception_rate_comparison.png")

# Show the plot
plt.show()
