#include <iostream>
#include <string>
#include <fstream>

#include "ignition_cells.hpp"
#include "landscape.hpp"
#include "many_simulations.hpp"
#include "spread_functions.hpp"

#define DISTANCE 30
#define ELEVATION_MEAN 1163.3
#define ELEVATION_SD 399.5
#define UPPER_LIMIT 0.5
#define SIMULATIONS 100
#define HEIGHT 20000
#define WIDTH 20000
#define FILENAME "graphics/simdata/burned_probabilities_data.txt"

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
    Landscape landscape(
        landscape_file_prefix + "-metadata.csv", landscape_file_prefix + "-landscape.csv", HEIGHT, WIDTH
    );

    // read the ignition cells
    IgnitionCells ignition_cells =
        read_ignition_cells(landscape_file_prefix + "-ignition_points.csv");

    SimulationParams params = {
      0, 0.5, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2, 0.2
    };

    Matrix<size_t> burned_amounts = burned_amounts_per_cell(
        landscape, ignition_cells, params, DISTANCE, ELEVATION_MEAN, ELEVATION_SD, UPPER_LIMIT, SIMULATIONS
    );
    // Abrir el archivo de salida y crear la cadena con información
    std::ofstream outputFile(FILENAME);
    outputFile << "Landscape size: " << landscape.width << " " << landscape.height << std::endl;
    outputFile << "Simulations: " << SIMULATIONS << std::endl;
    // Escribir los valores de burned_amounts en el archivo
    for (size_t i = 0; i < landscape.height; i++) {
        for (size_t j = 0; j < landscape.width; j++) {
            if (j != 0) {
                outputFile << " ";
            }
            outputFile << burned_amounts[{j, i}];
        }
        outputFile << std::endl;
    }
    // Cerrar archivo de salida
    outputFile.close();
  } catch (std::runtime_error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}