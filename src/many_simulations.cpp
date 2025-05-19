#include "many_simulations.hpp"

#include <omp.h>
#include <iostream>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <advisor-annotate.h>
#include "fires.hpp"
#define PERF_FILENAME "graphics/simdata/burned_probabilities_perf_data_"



Matrix<size_t> burned_amounts_per_cell(
    const Landscape& landscape, const std::vector<std::pair<size_t, size_t>>& ignition_cells,
    SimulationParams params, float distance, float elevation_mean, float elevation_sd,
    float upper_limit, size_t n_replicates, std::string output_filename_suffix
) {
  const char* env_threads = std::getenv("OMP_NUM_THREADS");
  if (env_threads) {
    std::cout << "OMP_NUM_THREADS desde el entorno: " << env_threads << std::endl;
  } else {
    std::cout << "OMP_NUM_THREADS no está configurado en el entorno." << std::endl;
  }
  std::cout << "Número de hilos detectados por OpenMP: " << omp_get_max_threads() << std::endl;
  size_t n_col = landscape.width;
  size_t n_row = landscape.height;

  Matrix<size_t> burned_amounts(n_col, n_row);
  Fire fire = empty_fire(n_col, n_row);
  std::vector<Cell> landscape_vec = landscape.to_flat_vector();

  float max_metric = 0.0f;
  std::vector<float> thread_total_times(omp_get_max_threads(), 0.0f);
  #pragma omp parallel
  { 
      Matrix<size_t> local_burned_amounts(n_col, n_row);
      float local_max_metric = 0.0f;
      float local_total_time_taken = 0.0f;

      ANNOTATE_SITE_BEGIN(simulation_loop);
      #pragma omp for schedule(dynamic, 10)
      for (size_t i = 0; i < n_replicates; i++) {
        ANNOTATE_ITERATION_TASK(loop1);
        Fire fire = simulate_fire(
          landscape_vec, n_row, n_col, ignition_cells, params,
          distance, elevation_mean, elevation_sd, upper_limit
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
      ANNOTATE_SITE_END();

      // Reducción de métricas y capas quemadas
      #pragma omp critical
      {
        for (size_t col = 0; col < n_col; col++) {
          for (size_t row = 0; row < n_row; row++) {
            burned_amounts[{col, row}] += local_burned_amounts[{col, row}];
          }
        }
        max_metric = std::max(max_metric, local_max_metric);
      }

      // Guardar tiempo total por thread
      int tid = omp_get_thread_num();
      thread_total_times[tid] = local_total_time_taken;
  }

  // Calcular tiempo paralelo estimado (el thread que más trabajó)
  float estimated_parallel_time = *std::max_element(
    thread_total_times.begin(), thread_total_times.end()
  );
  float total_time_taken = std::accumulate(
    thread_total_times.begin(), thread_total_times.end(), 0.0f
  );
  
  // Guardamos data de la performance para graficar
  std::ofstream outputFile(PERF_FILENAME + output_filename_suffix + ".txt", std::ios::app);
  outputFile << omp_get_max_threads() << ", " << n_col * n_row << ", " << max_metric << ", " << estimated_parallel_time << std::endl;
  outputFile.close();

  std::cout << "  SIMULATION PERFORMANCE DATA" << std::endl;
  std::cout << "* Total time taken: " << estimated_parallel_time << " seconds" << std::endl;
  std::cout << "* Average time per simulation: " << total_time_taken / n_replicates << " seconds" << std::endl;
  std::cout << "* Max metric: " << max_metric << " cells/nanosec processed" << std::endl;
  return burned_amounts;
}
