#include "spread_functions.hpp"

#define _USE_MATH_DEFINES
#include <cmath>
#include <random>
#include <vector>
#include <omp.h>
#include <iostream>
#include <array>
#include <random>

#include "fires.hpp"
#include "landscape.hpp"

constexpr float PIf = 3.1415927f;  // float PI
std::mt19937 rng(123);
std::uniform_real_distribution<float> dist(0.0f, 1.0f);

const Cell& get_default_cell() {
  static Cell default_cell = {
      0.0f,
      0.0f,
      0.0f,
      0.0f,
      false,
      MATORRAL,
  };
  return default_cell;
}

inline std::array<float, 8> spread_probability(
    const Cell& burning, 
    const std::array<float, 8> elevations,
    const std::array<float, 8> vegetation_types,
    const std::array<float, 8> fwis,
    const std::array<float, 8> aspects,
    SimulationParams params,
    const float* angles,
    float distance,
    float elevation_mean,
    float elevation_sd,
    const std::array<float, 8> upper_limits
) {
  std::array<float, 8> probs;
  for (size_t n = 0; n < 8; n++) {
    float upper_limit = upper_limits[n];
    float slope_term = sinf(atanf((elevations[n] - burning.elevation) / distance));
    float wind_term = cosf(angles[n] - burning.wind_direction);
    float elev_term = (elevations[n] - elevation_mean) / elevation_sd;

    float linpred = params.independent_pred;

    if (vegetation_types[n] == SUBALPINE) {
      linpred += params.subalpine_pred;
    } else if (vegetation_types[n] == WET) {
      linpred += params.wet_pred;
    } else if (vegetation_types[n] == DRY) {
      linpred += params.dry_pred;
    }

    linpred += params.fwi_pred * fwis[n];
    linpred += params.aspect_pred * aspects[n];

    linpred += wind_term * params.wind_pred + elev_term * params.elevation_pred +
              slope_term * params.slope_pred;

    float prob = upper_limit / (1 + expf(-linpred));
    probs[n] = prob;
  }

  return probs;
}

Fire simulate_fire(
    const std::vector<Cell> landscape, size_t n_row, size_t n_col, const std::vector<std::pair<size_t, size_t>>& ignition_cells,
    SimulationParams params, float distance, float elevation_mean, float elevation_sd,
    float upper_limit = 1.0
) {
  const Cell* landscape_data = landscape.data();
  std::vector<size_t> burned_ids_0;
  std::vector<size_t> burned_ids_1;

  size_t start = 0;
  size_t end = ignition_cells.size();

  for (size_t i = 0; i < end; i++) {
    burned_ids_0.push_back(ignition_cells[i].first);
    burned_ids_1.push_back(ignition_cells[i].second);
  }

  std::vector<size_t> burned_ids_steps;
  burned_ids_steps.push_back(end);

  size_t burning_size = end + 1;

  std::vector<int> burned_bin(n_col * n_row, 0);
  for (size_t i = 0; i < end; i++) {
      size_t x = ignition_cells[i].first;
      size_t y = ignition_cells[i].second;
      burned_bin[utils::INDEX(x, y, n_col)] = 1;
  }
  int* burned_data = burned_bin.data();

  double start_time = omp_get_wtime();
  unsigned int processed_cells = 0;
  constexpr int moves[8][2] = { { -1, -1 }, { -1, 0 }, { -1, 1 }, { 0, -1 },
                                { 0, 1 }, { 1, -1 }, { 1, 0 }, { 1, 1 } };
  constexpr float angles[8] = { PIf * 3 / 4, PIf, PIf * 5 / 4, PIf / 2, 
                                PIf * 3 / 2, PIf / 4, 0, PIf * 7 / 4 };
  while (burning_size > 0) {
    size_t end_forward = end;

    // Loop over burning cells in the cycle

    // b is going to keep the position in burned_ids that have to be evaluated
    // in this burn cycle
    for (size_t b = start; b < end; b++) {
      // b is the index of the burning cell in the burned_ids vector
      size_t burning_cell_0 = burned_ids_0[b];
      size_t burning_cell_1 = burned_ids_1[b];
      const Cell& burning_cell = landscape_data[utils::INDEX(burning_cell_0, burning_cell_1, n_col)];

      // Calculate the neighbors coordinates
      int neighbors_coords_0[8];
      int neighbors_coords_1[8];
      for (size_t i = 0; i < 8; i++) {
        neighbors_coords_0[i] = int(burning_cell_0) + moves[i][0];
        neighbors_coords_1[i] = int(burning_cell_1) + moves[i][1];
      }
      
      // SoA: Estructura de Arrays para los atributos de los vecinos
      std::array<size_t, 8> indices;
      std::array<int, 8> out_of_range_flags;

      std::array<float, 8> elevations;
      std::array<float, 8> fwis;
      std::array<float, 8> aspects;
      std::array<float, 8> vegetation_types;
      std::array<float, 8> burnables;
      
      std::array<float, 8> upper_limits;

      // 1. Get the neighbors data
      std::array<const Cell*, 8> neighbors;
      for (size_t n = 0; n < 8; ++n) {
        int nc0 = neighbors_coords_0[n];
        int nc1 = neighbors_coords_1[n];
        size_t idx = utils::INDEX(nc0, nc1, n_col);
        bool out_of_range = (nc0 < 0 || nc0 >= int(n_col) || nc1 < 0 || nc1 >= int(n_row));
        if (out_of_range) {
          neighbors[n] = &get_default_cell();
          indices[n] = 0;
          out_of_range_flags[n] = 1;
        } else {
          neighbors[n] = &landscape_data[idx];
          indices[n] = idx;
          out_of_range_flags[n] = 0;
        }
      }
      
      // loop totalmente separado para acceder por campo
      for (size_t n = 0; n < 8; ++n) {
        elevations[n] = neighbors[n]->elevation;
        fwis[n] = neighbors[n]->fwi;
        aspects[n] = neighbors[n]->aspect;
        vegetation_types[n] = neighbors[n]->vegetation_type;
        burnables[n] = neighbors[n]->burnable;
      }

      // 2. Check if neighbors are in range and burnable to calculate the upper limits
      for (size_t n = 0; n < 8; ++n) {
        size_t idx = indices[n];
        int burnable = !burned_data[idx] && burnables[n];
        int mask = (!out_of_range_flags[n] && burnable);
        upper_limits[n] = mask * upper_limit;
        processed_cells += !out_of_range_flags[n];
      }
      
      // 3. Calculate the probabilities of burning
      std::array<float, 8> probs;
      probs = spread_probability(
        burning_cell, elevations, vegetation_types, fwis, aspects, params, angles, distance, elevation_mean, elevation_sd, upper_limits
      );
      
      // 4. Calculate the burn flags based on the probabilities
      std::array<uint8_t, 8> burn_flags;
      for (size_t n = 0; n < 8; ++n) {
        burn_flags[n] = dist(rng) < probs[n];
      }
      
      // 5. Update the burned data and the burned ids if the cell is going to burn
      for (size_t n = 0; n < 8; ++n) {
        if (burn_flags[n]) {
          burned_data[indices[n]] = 1;
          burned_ids_0.push_back(neighbors_coords_0[n]);
          burned_ids_1.push_back(neighbors_coords_1[n]);
          end_forward++;
        }
      }
    }

    // update start and end
    start = end;
    end = end_forward;
    burning_size = end - start;
    burned_ids_steps.push_back(end);
  }
  double end_time = omp_get_wtime(); // missed: statement clobbers memory: start_time_64 = omp_get_wtime ();
  double time_taken = end_time - start_time;
  return { n_col, n_row, processed_cells, time_taken, burned_bin, burned_ids_0, burned_ids_1, burned_ids_steps };
}
