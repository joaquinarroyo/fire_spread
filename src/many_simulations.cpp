#include "many_simulations.hpp"

#include <omp.h>
#include <iostream>
#include <cmath>
#include <fstream>
#include "fires.hpp"
#define PERF_FILENAME "graphics/simdata/burned_probabilities_perf_data_v3.txt"



Matrix<size_t> burned_amounts_per_cell(
    const Landscape& landscape, const std::vector<std::pair<size_t, size_t>>& ignition_cells,
    SimulationParams params, float distance, float elevation_mean, float elevation_sd,
    float upper_limit, size_t n_replicates
) {

  size_t n_col = landscape.width;
  size_t n_row = landscape.height;
  Matrix<size_t> burned_amounts(n_col, n_row);
  Fire fire = empty_fire(n_col, n_row);
  std::vector<Cell> landscape_vec = landscape.to_flat_vector();
  float max_metric = 0;
  float total_time_taken = 0;
  #pragma omp parallel
  { 
    Matrix<size_t> local_burned_amounts(n_col, n_row);
    float local_max_metric = 0;
    float local_total_time_taken = 0;
    #pragma omp for schedule(dynamic, 16)
    for (size_t i = 0; i < n_replicates; i++) {
      Fire fire = simulate_fire(
        landscape_vec, n_row, n_col, ignition_cells, params, distance, elevation_mean, elevation_sd, upper_limit
      );
      float metric = fire.processed_cells / (fire.time_taken * 1e6);
      local_max_metric = std::max(local_max_metric, metric);
      local_total_time_taken += fire.time_taken;
      for (size_t col = 0; col < n_col; col++) {
        for (size_t row = 0; row < n_row; row++) {
          if (fire.burned_layer[utils::INDEX(col, row, n_col)]) {
            local_burned_amounts[{col, row}] += 1;
          }
        }
      }
    }
    #pragma omp critical
    {
      for (size_t col = 0; col < n_col; col++) {
        for (size_t row = 0; row < n_row; row++) {
          burned_amounts[{col, row}] += local_burned_amounts[{col, row}];
        }
      }
      max_metric = std::max(max_metric, local_max_metric);
    }
    #pragma omp atomic
    total_time_taken += local_total_time_taken;
  }

  size_t n_threads = omp_get_max_threads();
  double avg_total_time_taken = total_time_taken / n_threads;

  // Guardamos data de la performance para graficar
  std::ofstream outputFile(PERF_FILENAME, std::ios::app);
  outputFile << n_col * n_row << ", " << max_metric << ", " << avg_total_time_taken << std::endl;
  outputFile.close();

  std::cout << "  SIMULATION PERFORMANCE DATA" << std::endl;
  std::cout << "* Total time taken: " << avg_total_time_taken << " seconds" << std::endl;
  std::cout << "* Average time: " << avg_total_time_taken / n_replicates << " seconds" << std::endl;
  std::cout << "* Max metric: " << max_metric << " cells/nanosec processed" << std::endl;
  return burned_amounts;
}
