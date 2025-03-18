#include "many_simulations.hpp"

#include <iostream>
#include <cmath>

Matrix<size_t> burned_amounts_per_cell(
    const Landscape& landscape, const std::vector<std::pair<size_t, size_t>>& ignition_cells,
    SimulationParams params, double distance, double elevation_mean, double elevation_sd,
    double upper_limit, size_t n_replicates
) {

  Matrix<size_t> burned_amounts(landscape.width, landscape.height);
  double median = 0;
  double total_time_taken = 0;

  for (size_t i = 0; i < n_replicates; i++) {
    Fire fire = simulate_fire(
        landscape, ignition_cells, params, distance, elevation_mean, elevation_sd, upper_limit
    );
    size_t total_burned_cells = fire.burned_ids.size();
    double metric = total_burned_cells / fire.time_taken;
    median += metric;
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
  std::cout << "* Metric (avg in " << n_replicates << " simulations): " << median / n_replicates << " cells/sec processed" << std::endl;
  return burned_amounts;
}
