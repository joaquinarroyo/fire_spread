#include "spread_functions.hpp"

#define _USE_MATH_DEFINES
#include <cmath>
#include <random>
#include <vector>
#include <omp.h>
#include <iostream>
#include <array>

#include "fires.hpp"
#include "landscape.hpp"

inline size_t INDEX(size_t x, size_t y, size_t width) {
  return x + y * width;
}

constexpr float PIf = 3.1415927f;  // float PI

std::array<float, 8> spread_probability(
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

Fire simulate_fire( // missed: splitting region at control altering definition _268 = operator new (prephitmp_483); missed: not vectorized: statement can throw an exception: resx 41
    const std::vector<Cell> landscape, size_t n_row, size_t n_col, const std::vector<std::pair<size_t, size_t>>& ignition_cells,
    SimulationParams params, float distance, float elevation_mean, float elevation_sd,
    float upper_limit = 1.0
) {
  const Cell* landscape_data = landscape.data();
  std::vector<std::pair<size_t, size_t>> burned_ids;

  size_t start = 0;
  size_t end = ignition_cells.size();

  for (size_t i = 0; i < end; i++) {
    burned_ids.push_back(ignition_cells[i]);
  }

  std::vector<size_t> burned_ids_steps;
  burned_ids_steps.push_back(end);

  size_t burning_size = end + 1;

  std::vector<uint8_t> burned_bin(n_col * n_row, 0);;
  for (size_t i = 0; i < end; i++) {
      size_t x = ignition_cells[i].first;
      size_t y = ignition_cells[i].second;
      burned_bin[INDEX(x, y, n_col)] = 1;
  }
  uint8_t* burned_data = burned_bin.data();

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
      size_t burning_cell_0 = burned_ids[b].first;
      size_t burning_cell_1 = burned_ids[b].second;

      const Cell& burning_cell = landscape_data[INDEX(burning_cell_0, burning_cell_1, n_col)];

      int neighbors_coords_0[8];
      int neighbors_coords_1[8];
      for (size_t i = 0; i < 8; i++) {
        neighbors_coords_0[i] = int(burning_cell_0) + moves[i][0];
        neighbors_coords_1[i] = int(burning_cell_1) + moves[i][1];
      }
      // Note that in the case 0 - 1 we will have size_t_MAX
      
      // Loop over neighbors_coords of the focal burning cell
      std::array<float, 8> elevations;
      std::array<float, 8> vegetation_types;
      std::array<float, 8> fwis;
      std::array<float, 8> aspects;
      std::array<float, 8> upper_limits;
      // TODO: Ver aca
      for (size_t n = 0; n < 8; n++) {
        int neighbour_cell_0 = neighbors_coords_0[n];
        int neighbour_cell_1 = neighbors_coords_1[n];

        // Is the cell in range?
        uint8_t out_of_range = 0 > neighbour_cell_0 || neighbour_cell_0 >= int(n_col) ||
                               0 > neighbour_cell_1 || neighbour_cell_1 >= int(n_row);
        
        // TODO: Revisar que neighbour_cell_0 y neighbour_cell_1 no sean negativos
        size_t idx = INDEX(neighbour_cell_0, neighbour_cell_1, n_col);
        const Cell& neighbour_cell = landscape_data[idx];
        uint8_t burnable_cell = !burned_data[INDEX(neighbour_cell_0, neighbour_cell_1, n_col)] && neighbour_cell.burnable;

        uint8_t mask = (!out_of_range && burnable_cell) ? 1 : 0;
        float limit = mask * upper_limit;
        
        elevations[n] = neighbour_cell.elevation;
        vegetation_types[n] = neighbour_cell.vegetation_type;
        fwis[n] = neighbour_cell.fwi;
        aspects[n] = neighbour_cell.aspect;
        upper_limits[n] = limit;

        processed_cells += !out_of_range;
      }
      
      std::array<float, 8> probs;
      probs = spread_probability(
        burning_cell, elevations, vegetation_types, fwis, aspects, params, angles, distance, elevation_mean, elevation_sd, upper_limits
      );
      
      // TODO: Ver aca
      for (size_t n = 0; n < 8; n++) {
        // Burn with probability prob (Bernoulli)
        bool burn = rand() < probs[n] * (RAND_MAX + 1.0);
        if (burn) {
          end_forward += 1;
          burned_ids.push_back({ neighbors_coords_0[n], neighbors_coords_1[n] });
          burned_data[INDEX(neighbors_coords_0[n], neighbors_coords_1[n], n_col)] = 1;
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
  return { n_col, n_row, processed_cells, time_taken, burned_bin, burned_ids, burned_ids_steps };
}
