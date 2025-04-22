#include "many_simulations.hpp"

#include <iostream>
#include <cmath>
#include <fstream>
#include "fires.hpp"
#define PERF_FILENAME "graphics/simdata/burned_probabilities_perf_data_v2.txt"



Matrix<size_t> burned_amounts_per_cell(
    const Landscape& landscape, const std::vector<std::pair<size_t, size_t>>& ignition_cells,
    SimulationParams params, float distance, float elevation_mean, float elevation_sd,
    float upper_limit, size_t n_replicates
) {

  Matrix<size_t> burned_amounts(landscape.width, landscape.height);
  double max_metric = 0;
  double total_time_taken = 0;
  int n_row = landscape.height;
  int n_col = landscape.width;
  Fire fire = empty_fire(n_row, n_col);
  std::vector<Cell> landscape_vec = landscape.to_flat_vector();
  for (size_t i = 0; i < n_replicates; i++) {
    Fire fire = simulate_fire(
      landscape_vec, n_row, n_col, ignition_cells, params, distance, elevation_mean, elevation_sd, upper_limit
    );
    double metric = fire.processed_cells / (fire.time_taken * 1e6);
    max_metric = std::max(max_metric, metric);
    total_time_taken += fire.time_taken;
    for (size_t col = 0; col < landscape.width; col++) {
      for (size_t row = 0; row < landscape.height; row++) {
        if (fire.burned_layer[utils::INDEX(col, row, n_col)]) {
          burned_amounts[{col, row}] += 1;
        }
      }
    }
  }

  // Guardamos data de la performance para graficar
  std::ofstream outputFile(PERF_FILENAME, std::ios::app);
  outputFile << landscape.width * landscape.height << ", " << max_metric << ", " << total_time_taken << std::endl;
  outputFile.close();

  std::cout << "  SIMULATION PERFORMANCE DATA" << std::endl;
  std::cout << "* Total time taken: " << total_time_taken << " seconds" << std::endl;
  std::cout << "* Average time: " << total_time_taken / n_replicates << " seconds" << std::endl;
  std::cout << "* Max metric: " << max_metric << " cells/nanosec processed" << std::endl;
  return burned_amounts;
}
