import matplotlib.pyplot as plt
import sys

FILENAME = "./graphics/simdata/{}_perf_data.txt"
DIR = "./plots/"

def read_perf_data(name):
    """
    Reads the performance data from the file

    Example input:
    m*n, min_metric, max_metric, time

    Example output:
    {
        m*n: (min_metric, max_metric, time)
    }
    """
    data = {}
    with open(FILENAME.format(name), "r") as file:
        lines = file.readlines()
        for line in lines:
            cells, min_metric, max_metric, time = line.split(",")
            if cells in data.keys():
                data[int(cells)].append((float(min_metric), float(max_metric), float(time)))
            else:
                data[int(cells)] = [(float(min_metric), float(max_metric), float(time))]
    
    for key, value in data.items():
        min_metric = sum([x[0] for x in value]) / len(value)
        max_metric = sum([x[1] for x in value]) / len(value)
        time = sum([x[2] for x in value]) / len(value)
        data[key] = (min_metric, max_metric, time)
    return data

def generate_plots(data, name):
    """
    Generates the plot for the performance data and saves it

    data structure:
    {
        m*n: (min_metric, max_metric, time)
    }
    """
    if not data:
        print("No data to plot")
        return
    data = [(key, value[0], value[1], value[2]) for key, value in data.items()]
    data = sorted(data, key=lambda x: x[0])
    # Delete duplicate key: x[0] data
    data = list(dict.fromkeys(data))
    cells = [x[0] for x in data]
    min_metric = [x[1] for x in data]
    max_metric = [x[2] for x in data]
    time = [x[3] for x in data]
    # First plot, metric performance
    plt.xscale("log")    
    plt.plot(cells, min_metric, label="Min processed cells/nanosecond")
    plt.plot(cells, max_metric, label="Max processed cells/nanosecond")
    plt.plot(cells, min_metric, "o", color="blue")
    plt.plot(cells, max_metric, "o", color="orange")
    plt.xlabel("Number of cells")
    plt.ylabel("Metric processed cells/nanosecond")
    plt.title(f"{name}: metric performance x size")
    plt.legend()
    plot_name = DIR + name + "_perf.png"
    plt.savefig(plot_name)
    print("Plot saved as: " + plot_name)
    # Second plot, time performance
    plt.clf()
    plt.xscale("log")
    plt.xlabel("Number of cells")
    plt.ylabel("Time (seconds)")
    plt.title(f"{name}: time performance x size")
    plt.plot(cells, time, label="Time (seconds)")
    plt.plot(cells, time, "o", color="blue")
    plt.legend()
    plot_name = DIR + name + "_time.png"
    plt.savefig(plot_name)
    print("Plot saved as: " + plot_name)


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 generate_plots.py burned_probabilities|fire_animation")
        return
    name = sys.argv[1]
    if name not in ["burned_probabilities", "fire_animation"]:
        print("Usage: python3 generate_plots.py burned_probabilities|fire_animation")
        return
    data = read_perf_data(name)
    generate_plots(data, name)

if __name__ == "__main__":
    main() 