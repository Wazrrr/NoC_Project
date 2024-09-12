import os
import re
import pandas as pd
import matplotlib.pyplot as plt

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


# Function to plot the latency-throughput curve for multiple files
def plot_latency_throughput_for_files(file_paths):
    plt.figure(figsize=(12, 8))
    traffic = file_paths[0].split("/")[-1].split(".txt")[0]
    print(traffic)

    colors = {
        "TABLE_": "#e1bc89",
        "dor": "#4e9a4c",
        "goal": "#993524",  # Red
        "min_ad": "#2bb5b7",
    }

    for file_path in file_paths:

        title = extract_title(file_path)
        if title == "algorithm0":
            title = "TABLE_"
        (
            injection_rates,
            packet_received,
            average_latency,
            rejction_rates,
        ) = parse_file(file_path)

        temp_injection_rates = injection_rates
        # find the first element in average_latency which is larger than 100 and delete all after elements
        for i in range(len(average_latency)):
            if average_latency[i] > 200:
                average_latency = average_latency[:i]
                temp_injection_rates = injection_rates[:i]
                break

        color = colors.get(
            title, "#000000"
        )  # Default to black if the title is not found

        # Plot the data with the specified color
        plt.plot(
            temp_injection_rates,
            average_latency,
            marker="o",
            label=title,
            color=color,
            linewidth=2.5,
        )

        # plt.plot(temp_injection_rates, average_latency, marker='o', label=title)
        # plt.plot(injection_rates, rejction_rates, marker='o', label=title)
        # plt.plot(injection_rates, packet_received, marker="o", label=title)

    plt.xlabel("Injection Rates")

    plt.ylabel("Average Packet Latency")
    # plt.ylabel('Rejection Rates')
    # plt.ylabel("Packet Received")

    # i want that the end of the title is the traffic pattern
    plt.title(
        "Latency-Injectionrate Curve for four different routing algorithms in "
        + traffic
        + " traffic pattern"
    )
    # plt.title('Latency')
    # plt.title(
    #     "PacketsReceived-Injectionrate Curve for Different Traffic Patterns"
    # )
    plt.grid(True)
    plt.legend()
    # plt.savefig('latency_injectionrates_curve.png')
    plt.savefig("global_results/plot_latency_injectionrate_compare_" + traffic + ".png")
    # plt.savefig("packetsreceived_injectionrates_curve.png")
    plt.show()


list = [
    "uniform_random",
    "torus_transpose",
    "torus_tornado",
    "shuffle",
    "bit_rotation",
    "bit_reverse",
]

# List of file paths to process
# i want create a list of file paths for each traffic pattern
for traffic in list:
    file_paths = [
        "../result/Lab4/4_4_4_torus/algorithm0/" + traffic + ".txt",
        "../result/Lab4/4_4_4_torus/dor/" + traffic + ".txt",
        "../result/Lab4/4_4_4_torus/goal/" + traffic + ".txt",
        "../result/Lab4/4_4_4_torus/min_ad/" + traffic + ".txt",
        # Add more file paths as needed
    ]
    # Plot the latency-throughput curves
    plot_latency_throughput_for_files(file_paths)
