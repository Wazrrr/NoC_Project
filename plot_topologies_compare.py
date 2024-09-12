import os
import re
import pandas as pd
import matplotlib.pyplot as plt

# Function to parse a single file and extract data
def parse_file(file_path):
    injection_rates = []
    packet_received = []
    average_latency = []
    rejection_rates = []
    average_hops = []

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
                rejection = float(re.findall(r"[-+]?\d*\.\d+|\d+", line)[0])
                rejection_rates.append(rejection)
            elif "average_hops =" in line:
                hops = float(re.findall(r"[-+]?\d*\.\d+|\d+", line)[0])
                average_hops.append(hops)

    return (
        injection_rates,
        packet_received,
        average_latency,
        rejection_rates,
        average_hops,
    )


# Function to extract title from file name
def extract_title(file_name):
    return file_name.split("lab2_task1_")[-1].split(".txt")[0]


colors = {
    "Mesh_XY": "#4e9a4c",
    "3D Torus": "#993524",  # Red
    "Ring": "#2bb5b7",
}
# Function to plot the latency-throughput curve for multiple files
def plot_hops_for_files(file_paths):
    plt.figure(figsize=(12, 8))

    for file_path in file_paths:
        file_name = os.path.basename(file_path)
        title = extract_title(file_name)
        if file_name == "uniform_random.txt":
            title = "3D Torus"
        elif file_name == "Mesh_XY.txt":
            title = "Mesh_XY"
        else:
            title = "Ring"
        (
            injection_rates,
            packet_received,
            average_latency,
            rejection_rates,
            average_hops,
        ) = parse_file(file_path)

        color = colors[title]
        plt.plot(
            injection_rates,
            average_hops,
            marker="o",
            label=title,
            color=color,
            linewidth=2.5,
        )

    plt.xlabel("Injection rates")
    plt.ylabel("Average Hops")
    plt.title("Hops-Throughput Curve for Different Topologies")
    plt.grid(True)
    plt.legend()
    plt.savefig("hops.png")
    plt.show()


def plot_latency_for_files(file_paths):
    plt.figure(figsize=(12, 8))

    for file_path in file_paths:
        file_name = os.path.basename(file_path)
        title = extract_title(file_name)
        if file_name == "uniform_random.txt":
            title = "3D Torus"
        elif file_name == "Mesh_XY.txt":
            title = "Mesh_XY"
        else:
            title = "Ring"
        (
            injection_rates,
            packet_received,
            average_latency,
            rejection_rates,
            average_hops,
        ) = parse_file(file_path)

        color = colors[title]
        plt.plot(
            injection_rates,
            average_latency,
            marker="o",
            label=title,
            color=color,
            linewidth=2.5,
        )

    plt.xlabel("Injection rates")
    plt.ylabel("Average Latency")
    plt.title("Latency-Throughput Curve for Different Topologies")
    plt.grid(True)
    plt.legend()
    plt.savefig("latency.png")
    plt.show()


def plot_receptionrate_for_files(file_paths):
    plt.figure(figsize=(12, 8))

    for file_path in file_paths:
        file_name = os.path.basename(file_path)
        title = extract_title(file_name)
        if file_name == "uniform_random.txt":
            title = "3D Torus"
        elif file_name == "Mesh_XY.txt":
            title = "Mesh_XY"
        else:
            title = "Ring"
        (
            injection_rates,
            packet_received,
            average_latency,
            rejection_rates,
            average_hops,
        ) = parse_file(file_path)

        color = colors[title]
        reception_rates = [i / 64 / 10000 for i in packet_received]
        plt.plot(
            injection_rates,
            reception_rates,
            marker="o",
            label=title,
            color=color,
            linewidth=2.5,
        )

    plt.xlabel("Injection rates")
    plt.ylabel("Reception Rates")
    plt.title("Reception_Rates-Throughput Curve for Different Topologies")
    plt.grid(True)
    plt.legend()
    plt.savefig("reception.png")
    plt.show()


# List of file paths to process
file_paths = [
    "result/Lab4/4_4_4_torus/algorithm0/uniform_random.txt",
    "result/Lab3/Task1/Mesh_XY.txt",
    "result/Lab3/Task1/Ring.txt",
    # Add more file paths as needed
]

# Plot the latency-throughput curves
plot_hops_for_files(file_paths)
plot_latency_for_files(file_paths)
plot_receptionrate_for_files(file_paths)
