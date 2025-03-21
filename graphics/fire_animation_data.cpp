#include <iostream>
#include <random>
#include <fstream>

#include "fires.hpp"
#include "ignition_cells.hpp"
#include "landscape.hpp"
#include "many_simulations.hpp"
#include "spread_functions.hpp"

#define DISTANCE 30
#define ELEVATION_MEAN 1163.3
#define ELEVATION_SD 399.5
#define UPPER_LIMIT 0.75
#define HEIGHT 20000
#define WIDTH 20000
#define FILENAME "graphics/simdata/fire_animation_data.txt"

// main function reading command line arguments
int main(int argc, char* argv[]) {
  try {

    // check if the number of arguments is correct
    if (argc != 2) {
      std::cerr << "Usage: " << argv[0] << " <landscape_file_prefix>" << std::endl;
      return EXIT_FAILURE;
    }

    // read the landscape file prefix
    std::string landscape_file_prefix = argv[1];

    // read the landscape
    Landscape landscape(landscape_file_prefix + "-metadata.csv", landscape_file_prefix + "-landscape.csv", HEIGHT, WIDTH);

    // read the ignition cells
    IgnitionCells ignition_cells =
        read_ignition_cells(landscape_file_prefix + "-ignition_points.csv");

    SimulationParams params = {
      0, 0.5, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2
    };

    Fire fire = simulate_fire(
        landscape, ignition_cells, params, DISTANCE, ELEVATION_MEAN, ELEVATION_SD, UPPER_LIMIT
    );
    double time_taken = fire.time_taken;
    double metric = fire.processed_cells / time_taken; // TODO: Revisar si lo hacemos en nanosegundos o esta bien asi

    std::cout << "  SIMULATION PERFORMANCE DATA" << std::endl;
    std::cout << "* Total time taken: " << time_taken << " seconds" << std::endl;
    std::cout << "* Metric: " << metric << " cells/sec processed (N*M cells: worst case)" << std::endl;

    // Abrir el archivo de salida y crear la cadena con información
    std::ofstream outputFile(FILENAME);
    outputFile << "Landscape size: " << landscape.width << " " << landscape.height << std::endl;
    size_t step = 0;
    size_t i = 0;
    for (size_t j : fire.burned_ids_steps) {
      if (i >= j) {
        continue;
      }
      outputFile << "Step " << step << ":" << std::endl;
      for (; i < j; i++) {
        outputFile << fire.burned_ids[i].first << " " << fire.burned_ids[i].second << std::endl;
      }
      step++;
    }
    outputFile.close();
  } catch (std::runtime_error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
