#include "fires.hpp"

#include <fstream>

#include "landscape.hpp"
#include "matrix.hpp"

inline size_t INDEX(size_t x, size_t y, size_t width) {
  return x + y * width;
}

Fire read_fire(size_t width, size_t height, std::string filename) {

  std::ifstream burned_ids_file(filename);

  if (!burned_ids_file.is_open()) {
    throw std::runtime_error("Can't open landscape file");
  }

  CSVIterator loop(burned_ids_file);
  loop++;

  std::vector<uint8_t> burned_layer(width * height, false);

  std::vector<std::pair<size_t, size_t>> burned_ids;

  for (; loop != CSVIterator(); ++loop) {
    if (loop->size() < 2) {
      throw std::runtime_error("Invalid fire file");
    }
    size_t x = atoi((*loop)[0].data());
    size_t y = atoi((*loop)[1].data());
    if (x >= width || y >= height) {
      throw std::runtime_error("Invalid fire file");
    }
    burned_layer[INDEX(x, y, width)] = 1;
    burned_ids.push_back({ x, y });
  }

  burned_ids_file.close();

  return { width, height, 0, 0, burned_layer, burned_ids, {} };
}

Fire empty_fire(size_t width, size_t height) {
  return { width, height, 0, 0, std::vector<uint8_t>(width * height), {}, {} };
}

FireStats get_fire_stats(const Fire& fire, const Landscape& landscape) {

  FireStats stats = { 0, 0, 0, 0 };

  for (auto [x, y] : fire.burned_ids) {
    Cell cell = landscape[{ x, y }];

    if (cell.vegetation_type == SUBALPINE) {
      stats.counts_veg_subalpine++;
    } else if (cell.vegetation_type == WET) {
      stats.counts_veg_wet++;
    } else if (cell.vegetation_type == DRY) {
      stats.counts_veg_dry++;
    } else {
      stats.counts_veg_matorral++;
    }
  }

  return stats;
}
