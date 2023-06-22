#include "many_simulations.hpp"

#include <cmath>

Matrix<size_t> burned_amounts_per_cell(
    Landscape& landscape, std::vector<std::pair<size_t, size_t>> ignition_cells,
    SimulationParams params, float distance, float elevation_mean, float elevation_sd,
    float upper_limit, size_t n_replicates
) {

  Matrix<size_t> burned_amounts(landscape.width, landscape.height);

  for (size_t i = 0; i < n_replicates; i++) {
    Fire fire = simulate_fire(
        landscape, ignition_cells, params, distance, elevation_mean, elevation_sd, upper_limit
    );

    for (size_t row = 0; row < landscape.width; row++) {
      for (size_t col = 0; col < landscape.height; col++) {
        if (fire.burned_layer[{row, col}]) {
          burned_amounts[{row, col}] += 1;
        }
      }
    }
  }

  return burned_amounts;
}
