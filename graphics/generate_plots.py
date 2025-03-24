import matplotlib.pyplot as plt
import sys

FILENAME = "./graphics/simdata/{}_perf_data.txt"
DIR = "./plots/"

def read_perf_data(name):
    """
    Reads the performance data from the file
    Example input:
    m*n, min_metric, max_metric, time
    """
    data = []
    processed_cells = []
    with open(FILENAME.format(name), "r") as file:
        lines = file.readlines()
        for line in lines:
            cells, min_metric, max_metric, _ = line.split(",")
            if int(cells) in processed_cells:
                continue
            processed_cells.append(int(cells))
            data.append((int(cells), float(min_metric), float(max_metric)))
    return data

def generate_plots(data, name):
    """
    Generates the plot for the performance data and saves it
    """
    if not data:
        print("No data to plot")
        return
    data = sorted(data, key=lambda x: x[0])
    # Delete duplicate key: x[0] data
    data = list(dict.fromkeys(data))
    cells = [x[0] for x in data]
    min_metric = [x[1] for x in data]
    max_metric = [x[2] for x in data]
    plt.xlim([cells[0], max(cells)])
    plt.plot(cells, min_metric, label="Min cells/nanosecond")
    plt.plot(cells, max_metric, label="Max cells/nanosecond")
    # Save plot
    plt.xlabel("Number of cells")
    plt.ylabel("Metric (cells/nanosecond)")
    plt.title(f"{name} performance")
    plt.legend()
    plot_name = DIR + name + "_perf.png"
    plt.savefig(plot_name)
    print("Plot saved as: " + plot_name)

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 generate_plots.py burned_probabilities|fire_animation")
        return
    name = sys.argv[1]
    if name not in ["burned_probabilities", "fire_animation"]:
        print("Invalid name")
        return
    data = read_perf_data(name)
    generate_plots(data, name)

if __name__ == "__main__":
    main() 