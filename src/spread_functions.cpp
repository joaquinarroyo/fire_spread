#include "spread_functions.hpp"

#define _USE_MATH_DEFINES
#include <cmath>
#include <random>
#include <vector>
#include <omp.h>
#include <iostream>

#include "fires.hpp"
#include "landscape.hpp"

constexpr float PIf = 3.1415927f;  // float PI

inline float spread_probability(
    const Cell& burning, const Cell& neighbour, SimulationParams params, float angle,
    float distance, float elevation_mean, float elevation_sd, float upper_limit = 1.0
) {

  float slope_term = sinf(atanf((neighbour.elevation - burning.elevation) / distance));
  float wind_term = cosf(angle - burning.wind_direction);
  float elev_term = (neighbour.elevation - elevation_mean) / elevation_sd;

  float linpred = params.independent_pred;

  if (neighbour.vegetation_type == SUBALPINE) {
    linpred += params.subalpine_pred;
  } else if (neighbour.vegetation_type == WET) {
    linpred += params.wet_pred;
  } else if (neighbour.vegetation_type == DRY) {
    linpred += params.dry_pred;
  }

  linpred += params.fwi_pred * neighbour.fwi;
  linpred += params.aspect_pred * neighbour.aspect;

  linpred += wind_term * params.wind_pred + elev_term * params.elevation_pred +
             slope_term * params.slope_pred;

  float prob = upper_limit / (1 + expf(-linpred));

  return prob;
}

Fire simulate_fire(
    const Landscape& landscape, const std::vector<std::pair<size_t, size_t>>& ignition_cells,
    SimulationParams params, float distance, float elevation_mean, float elevation_sd,
    float upper_limit = 1.0
) {
  // For reproducibility of rand() function in the simulation
  srand(420);

  size_t n_row = landscape.height;
  size_t n_col = landscape.width;

  std::vector<std::pair<size_t, size_t>> burned_ids;

  size_t start = 0;
  size_t end = ignition_cells.size();

  for (size_t i = 0; i < end; i++) {
    burned_ids.push_back(ignition_cells[i]);
  }

  std::vector<size_t> burned_ids_steps;
  burned_ids_steps.push_back(end);

  size_t burning_size = end + 1;

  Matrix<bool> burned_bin = Matrix<bool>(n_col, n_row);

  for (size_t i = 0; i < end; i++) {
    size_t cell_0 = ignition_cells[i].first;
    size_t cell_1 = ignition_cells[i].second;
    burned_bin[{ cell_0, cell_1 }] = 1;
  }

  double start_time = omp_get_wtime();
  int processed_cells = 0;
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

      const Cell& burning_cell = landscape[{ burning_cell_0, burning_cell_1 }];


      int neighbors_coords[2][8];

      // TODO: Aca podemos ver de vectorizar
      for (size_t i = 0; i < 8; i++) {
        neighbors_coords[0][i] = int(burning_cell_0) + moves[i][0];
        neighbors_coords[1][i] = int(burning_cell_1) + moves[i][1];
      }
      // Note that in the case 0 - 1 we will have size_t_MAX

      // Loop over neighbors_coords of the focal burning cell
      // TODO: Aca podemos ver de vectorizar para procesar 8 celdas en paralelo
      for (size_t n = 0; n < 8; n++) {

        int neighbour_cell_0 = neighbors_coords[0][n];
        int neighbour_cell_1 = neighbors_coords[1][n];

        // Is the cell in range?
        bool out_of_range = 0 > neighbour_cell_0 || neighbour_cell_0 >= int(n_col) ||
                            0 > neighbour_cell_1 || neighbour_cell_1 >= int(n_row);

        if (out_of_range)
          continue;

        const Cell& neighbour_cell = landscape[{ neighbour_cell_0, neighbour_cell_1 }];
        
        // Add one to the number of processed cells
        processed_cells += 1;

        // Is the cell burnable?
        bool burnable_cell =
            !burned_bin[{ neighbour_cell_0, neighbour_cell_1 }] && neighbour_cell.burnable;

        if (!burnable_cell)
          continue;


        // simulate fire
        float prob = spread_probability(
            burning_cell, neighbour_cell, params, angles[n], distance, elevation_mean,
            elevation_sd, upper_limit
        );

        // Burn with probability prob (Bernoulli)
        bool burn = rand() < prob * (RAND_MAX + 1.0);

        if (burn == 0)
          continue;

        // If burned, store id of recently burned cell and set 1 in burned_bin
        end_forward += 1;
        burned_ids.push_back({ neighbour_cell_0, neighbour_cell_1 });
        burned_bin[{ neighbour_cell_0, neighbour_cell_1 }] = true;
      }
    }

    // update start and end
    start = end;
    end = end_forward;
    burning_size = end - start;
    burned_ids_steps.push_back(end);
  }
  double end_time = omp_get_wtime();
  double time_taken = end_time - start_time;
  return { n_col, n_row, processed_cells, time_taken, burned_bin, burned_ids, burned_ids_steps };
}
