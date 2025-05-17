import matplotlib.pyplot as plt
import numpy as np
import sys

FILENAME = "./graphics/simdata/{}_perf_data_{}.txt"
DIR = "./plots/"

DATA_NAMES = {
    2548: '2005_26',
    8008: '2000_8',
    13125: '2011_19W',
    14161: '2005_6',
    15912: '2009_3',
    48614: '2011_19E',
    94188: '1999_28',
    143655: '1999_27j_N',
    1483274: '1999_27j_S',
    4696595: '2021_865',
    10434109: '2015_50'
}

def read_perf_data(name, versions):
    """
    Reads the performance data from the file

    Example input:
    m*n, max_metric, time

    Example output:
    {
        m*n: (max_metric, time)
    }
    """
    data = {}
    for version in versions:
        inner_data = {}
        with open(FILENAME.format(name, version), "r") as file:
            lines = file.readlines()
            for line in lines:
                cells, max_metric, time = line.split(",")
                if cells in inner_data.keys():
                    inner_data[int(cells)].append((float(max_metric), float(time)))
                else:
                    inner_data[int(cells)] = [(float(max_metric), float(time))]
        for key, value in inner_data.items():
            max_metric = sum([x[0] for x in value]) / len(value)
            time = sum([x[1] for x in value]) / len(value)
            if key in data.keys():
                data[key].append((max_metric, time))
            else:
                data[key] = [(max_metric, time)]
    return data

def generate_barplots(data, name):
    """
    Generates bar plots for multiple metrics and times per dataset.

    Parameters:
    - data: dict with structure {id: [(value_i, time_i), ...]}
    - name: base name for the output files

    Saves:
    - {name}_metric_bar.png
    - {name}_times_bar.png
    """
    if not data:
        print("No data to plot")
        return

    ids = list(data.keys())
    num_variants = len(next(iter(data.values())))

    x_labels = [DATA_NAMES.get(i, str(i)) for i in ids]
    x = np.arange(len(ids))
    width = 0.8 / num_variants  # Ajusta ancho según cantidad de variantes

    # --- Plot de métricas ---
    fig, ax = plt.subplots(figsize=(10, 6))
    for idx in range(num_variants):
        values = [data[i][idx][0] for i in ids]
        offset = (idx - (num_variants - 1) / 2) * width
        bars = ax.bar(x + offset, values, width, label=f'Métrica {idx + 1}')
        # Añadir etiquetas de valor encima de cada barra
        for bar, value in zip(bars, values):
            ax.text(
                bar.get_x() + bar.get_width() / 2,
                bar.get_height(),
                f'{value:.2f}',
                ha='center',
                va='bottom',
                fontsize=8,
                rotation=0
            )
    ax.set_xlabel('Datasets')
    ax.set_ylabel('Valor')
    ax.set_title('Comparación Métrica')
    ax.set_xticks(x)
    ax.set_xticklabels(x_labels, rotation=45)
    ax.legend()
    plt.tight_layout()
    plt.savefig(f"{DIR}{name}_perf_bar.png")
    print(f"Saved plot to {DIR}{name}_perf_bar.png")
    plt.close()

    # --- Plot de tiempos ---
    fig, ax = plt.subplots(figsize=(10, 6))
    for idx in range(num_variants):
        times = [data[i][idx][1] for i in ids]
        offset = (idx - (num_variants - 1) / 2) * width
        bars = ax.bar(x + offset, times, width, label=f'Tiempo {idx + 1}')
        # Añadir etiquetas de valor encima de cada barra
        for bar, value in zip(bars, times):
            # Notación científica si el valor es muy grande o muy pequeño
            if abs(value) > 1000 or (0 < abs(value) < 0.01):
                label = f'{value:.2e}'
            else:
                label = f'{value:.2f}'
            ax.text(
                bar.get_x() + bar.get_width() / 2,
                bar.get_height(),
                label,
                ha='center',
                va='bottom',
                fontsize=8,
                rotation=0  # Horizontal
            )
    ax.set_yscale('log')
    ax.set_xlabel('Datasets')
    ax.set_ylabel('Tiempo (s)')
    ax.set_title('Comparación Tiempos')
    ax.set_xticks(x)
    ax.set_xticklabels(x_labels, rotation=45)
    ax.legend()
    plt.tight_layout()
    plt.savefig(f"{DIR}{name}_times_bar.png")
    print(f"Saved plot to {DIR}{name}_times_bar.png")
    plt.close()


def generate_line_plots(data, name):
    """
    Generates line plots for multiple metrics and times per dataset.

    Parameters:
    - data: dict with structure {id: [(value_i, time_i), ...]}
    - name: base name for the output files

    Saves:
    - {name}_values_line.png
    - {name}_times_line.png
    """
    if not data:
        print("No data to plot")
        return

    ids = list(data.keys())
    num_variants = len(next(iter(data.values())))

    x_labels = [DATA_NAMES.get(i, str(i)) for i in ids]
    x = np.arange(len(ids))

    # --- Plot de métricas ---
    fig, ax = plt.subplots(figsize=(10, 6))
    for idx in range(num_variants):
        values = [data[i][idx][0] for i in ids]
        ax.plot(x, values, marker='o', label=f'Métrica {idx + 1}')
    ax.set_yscale('log')
    ax.set_xlabel('Datasets')
    ax.set_ylabel('Valor')
    ax.set_title('Comparación Métricas')
    ax.set_xticks(x)
    ax.set_xticklabels(x_labels, rotation=45)
    ax.legend()
    plt.tight_layout()
    plt.savefig(f"{DIR}{name}_perf_line.png")
    print(f"Saved plot to {DIR}{name}_perf_line.png")
    plt.close()

    # --- Plot de tiempos ---
    fig, ax = plt.subplots(figsize=(10, 6))
    for idx in range(num_variants):
        times = [data[i][idx][1] for i in ids]
        ax.plot(x, times, marker='o', label=f'Tiempo {idx + 1}')
    ax.set_yscale('log')
    ax.set_xlabel('Datasets')
    ax.set_ylabel('Tiempo (s)')
    ax.set_title('Comparación Tiempos')
    ax.set_xticks(x)
    ax.set_xticklabels(x_labels, rotation=45)
    ax.legend()
    plt.tight_layout()
    plt.savefig(f"{DIR}{name}_times_line.png")
    print(f"Saved plot to {DIR}{name}_times_line.png")
    plt.close()

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 generate_plots.py burned_probabilities|fire_animation")
        return
    name = sys.argv[1]
    if name not in ["burned_probabilities", "fire_animation"]:
        print("Usage: python3 generate_plots.py burned_probabilities|fire_animation")
        return
    versions = sys.argv[2:]
    if not versions:
        print("Usage: python3 generate_plots.py burned_probabilities|fire_animation v1 v2 ...")
        return
    data = read_perf_data(name, versions)
    generate_barplots(data, name)
    generate_line_plots(data, name)

if __name__ == "__main__":
    main()