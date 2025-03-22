#include "many_simulations.hpp"

#include <iostream>
#include <cmath>
#include <limits>

Matrix<size_t> burned_amounts_per_cell(
    const Landscape& landscape, const std::vector<std::pair<size_t, size_t>>& ignition_cells,
    SimulationParams params, double distance, double elevation_mean, double elevation_sd,
    double upper_limit, size_t n_replicates
) {

  Matrix<size_t> burned_amounts(landscape.width, landscape.height);
  double min_metric = std::numeric_limits<double>::infinity();
  double max_metric = 0;
  double total_time_taken = 0;

  for (size_t i = 0; i < n_replicates; i++) {
    Fire fire = simulate_fire(
        landscape, ignition_cells, params, distance, elevation_mean, elevation_sd, upper_limit
    );
    double metric = fire.processed_cells / fire.time_taken; // TODO: Revisar si lo hacemos en nanosegundos o esta bien asi
    min_metric = std::min(min_metric, metric);
    max_metric = std::max(max_metric, metric);
    total_time_taken += fire.time_taken;
    for (size_t col = 0; col < landscape.width; col++) {
      for (size_t row = 0; row < landscape.height; row++) {
        if (fire.burned_layer[{col, row}]) {
          burned_amounts[{col, row}] += 1;
        }
      }
    }
  }

  std::cout << "  SIMULATION PERFORMANCE DATA" << std::endl;
  std::cout << "* Total time taken: " << total_time_taken << " seconds" << std::endl;
  std::cout << "* Average time: " << total_time_taken / n_replicates << " seconds" << std::endl;
  std::cout << "* Min metric: " << min_metric << " cells/sec processed" << std::endl;
  std::cout << "* Max metric: " << max_metric << " cells/sec processed" << std::endl;
  return burned_amounts;
}
