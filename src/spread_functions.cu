#include "spread_functions.cuh"

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

#include <cuda_runtime.h>
#include <curand_kernel.h>
#include <cuda_runtime_api.h>

struct FireKernelParams {
    const float* elevation;
    const float* fwi;
    const float* aspect;
    const float* wind_dir;
    const float* vegetation_type;
    const uint8_t* burnable;

    int* burned_bin;
    int width;
    int height;

    const int* burned_ids_0;
    const int* burned_ids_1;
    int burned_size;

    int* new_burned_ids_0;
    int* new_burned_ids_1;
    int* new_burned_count;

    float distance;
    float upper_limit;
    float elevation_mean;
    float elevation_sd;

    const SimulationParams* params;
};

constexpr float PIf = 3.1415927f;
float h_angles[8] = {
    PIf * 3 / 4, PIf, PIf * 5 / 4, PIf / 2,
    PIf * 3 / 2, PIf / 4, 0, PIf * 7 / 4
};
int h_moves[8][2] = {
    { -1, -1 }, { -1, 0 }, { -1, 1 }, { 0, -1 },
    { 0, 1 }, { 1, -1 }, { 1, 0 }, { 1, 1 }
};
__constant__ float d_angles[8];
__constant__ int d_moves[8][2];

// RNG simple por thread (XORShift32)
__device__ uint32_t xorshift32(uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

__device__ void spread_probability_device(
    float burning_elevation,
    float burning_wind_direction,
    const float* elevations,
    const float* vegetation_types,
    const float* fwis,
    const float* aspects,
    const float* upper_limits,
    const SimulationParams* params,
    float distance,
    float elevation_mean,
    float elevation_sd,
    float* probs_out // salida de 8 elementos
) {
    for (int n = 0; n < 8; n++) {
        float slope_term = __sinf(atanf((elevations[n] - burning_elevation) / distance));
        float wind_term = __cosf(d_angles[n] - burning_wind_direction);
        float elev_term = (elevations[n] - elevation_mean) / elevation_sd;

        float linpred = params->independent_pred;

        if ((int)vegetation_types[n] == SUBALPINE) {
            linpred += params->subalpine_pred;
        } else if ((int)vegetation_types[n] == WET) {
            linpred += params->wet_pred;
        } else if ((int)vegetation_types[n] == DRY) {
            linpred += params->dry_pred;
        }

        linpred += params->fwi_pred * fwis[n];
        linpred += params->aspect_pred * aspects[n];
        linpred += wind_term * params->wind_pred + elev_term * params->elevation_pred + slope_term * params->slope_pred;

        probs_out[n] = upper_limits[n] / (1.0f + __expf(-linpred));
    }
}

__global__ void fire_step_kernel(FireKernelParams args) {
    // Desempaquetar argumentos para facilitar el uso
    const float* elevation = args.elevation;
    const float* fwi = args.fwi;
    const float* aspect = args.aspect;
    const float* wind_dir = args.wind_dir;
    const float* vegetation_type = args.vegetation_type;
    const uint8_t* burnable = args.burnable;

    int* burned_bin = args.burned_bin;
    int width = args.width;
    int height = args.height;

    const int* burned_ids_0 = args.burned_ids_0;
    const int* burned_ids_1 = args.burned_ids_1;
    int burned_size = args.burned_size;

    int* new_burned_ids_0 = args.new_burned_ids_0;
    int* new_burned_ids_1 = args.new_burned_ids_1;
    int* new_burned_count = args.new_burned_count;

    float distance = args.distance;
    float upper_limit = args.upper_limit;
    float elevation_mean = args.elevation_mean;
    float elevation_sd = args.elevation_sd;

    const SimulationParams* params = args.params;

    // ---- Código original del kernel a partir de acá ----
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= burned_size) return;

    // Setup RNG con una semilla diferente por thread
    curandState state;
    curand_init(1234ULL, /* seed */
             idx,     /* sequence: hace que cada hilo tenga su propia secuencia */
             0,       /* offset */
             &state);

    int i = burned_ids_0[idx];
    int j = burned_ids_1[idx];

    int center_idx = j * width + i;

    float elev_c = elevation[center_idx];
    float wind_c = wind_dir[center_idx];

    float elevations[8], fwis[8], aspects[8], vegtypes[8], upper_limits[8];
    for (int n = 0; n < 8; ++n) {
        int ni = i + d_moves[n][0];
        int nj = j + d_moves[n][1];
        if (ni < 0 || nj < 0 || ni >= width || nj >= height) {
            upper_limits[n] = 0;
            continue;
        }

        int n_idx = nj * width + ni;

        elevations[n] = elevation[n_idx];
        fwis[n] = fwi[n_idx];
        aspects[n] = aspect[n_idx];
        vegtypes[n] = vegetation_type[n_idx];

        upper_limits[n] = (!burned_bin[n_idx] && burnable[n_idx]) ? upper_limit : 0.0f;
    }

    float probs[8];
    spread_probability_device(
        elev_c, wind_c, elevations, vegtypes, fwis, aspects, upper_limits,
        params, distance, elevation_mean, elevation_sd, probs
    );

    for (int n = 0; n < 8; ++n) {
        int ni = i + d_moves[n][0];
        int nj = j + d_moves[n][1];
        if (ni < 0 || nj < 0 || ni >= width || nj >= height) continue;
        int n_idx = nj * width + ni;

        float rand_val = curand_uniform(&state);
        if (rand_val < probs[n]) {
            if (atomicExch(&burned_bin[n_idx], 1) == 0) {
                int pos = atomicAdd(new_burned_count, 1);
                if (pos < width * height) {
                    new_burned_ids_0[pos] = ni;
                    new_burned_ids_1[pos] = nj;
                }
            }
        }
    }
}

Fire simulate_fire(
    LandscapeSoA landscape,
    size_t n_row, size_t n_col,
    const std::vector<std::pair<size_t, size_t>>& ignition_cells,
    SimulationParams params,
    float distance,
    float elevation_mean,
    float elevation_sd,
    int n_replicate,
    float upper_limit = 1.0f
) {
    const size_t MAX_BURNED_CELLS = n_row * n_col;
    n_replicate += 1;

    std::vector<int> h_burned_ids_0;
    std::vector<int> h_burned_ids_1;
    h_burned_ids_0.resize(MAX_BURNED_CELLS, -1);
    h_burned_ids_1.resize(MAX_BURNED_CELLS, -1);
    std::vector<int> burned_bin(n_row * n_col, 0);

    size_t end = ignition_cells.size();
    size_t burning_size = end;

    for (size_t i = 0; i < end; i++) {
        h_burned_ids_0[i] = ignition_cells[i].first;
        h_burned_ids_1[i] = ignition_cells[i].second;
        burned_bin[utils::INDEX(ignition_cells[i].first, ignition_cells[i].second, n_col)] = 1;
    }

    std::vector<size_t> burned_ids_steps;
    burned_ids_steps.push_back(end);

    cudaMemcpyToSymbol(d_angles, h_angles, sizeof(h_angles));
    cudaMemcpyToSymbol(d_moves, h_moves, sizeof(h_moves));
    
    // Punteros GPU
    int *d_burned_ids_0, *d_burned_ids_1;
    int *d_new_burned_ids_0, *d_new_burned_ids_1;
    int *d_burned_bin, *d_new_burned_count;

    // Reservar memoria en GPU
    cudaMalloc(&d_burned_ids_0, MAX_BURNED_CELLS * sizeof(int));
    cudaMalloc(&d_burned_ids_1, MAX_BURNED_CELLS * sizeof(int));
    cudaMalloc(&d_new_burned_ids_0, MAX_BURNED_CELLS * sizeof(int));
    cudaMalloc(&d_new_burned_ids_1, MAX_BURNED_CELLS * sizeof(int));
    cudaMalloc(&d_burned_bin, n_row * n_col * sizeof(int));
    cudaMalloc(&d_new_burned_count, sizeof(int));

    // Copiar datos iniciales a GPU
    cudaMemcpy(d_burned_ids_0, h_burned_ids_0.data(), end * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_burned_ids_1, h_burned_ids_1.data(), end * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_burned_bin, burned_bin.data(), n_row * n_col * sizeof(int), cudaMemcpyHostToDevice);

    // Reservar memoria en device
    float *d_elevation, *d_fwi, *d_aspect, *d_wind_dir, *d_vegetation_type;
    uint8_t *d_burnable;

    cudaMalloc(&d_elevation, n_row * n_col * sizeof(float));
    cudaMalloc(&d_fwi, n_row * n_col * sizeof(float));
    cudaMalloc(&d_aspect, n_row * n_col * sizeof(float));
    cudaMalloc(&d_wind_dir, n_row * n_col * sizeof(float));
    cudaMalloc(&d_vegetation_type, n_row * n_col * sizeof(float));
    cudaMalloc(&d_burnable, n_row * n_col * sizeof(uint8_t));

    // Copiar datos del host al device
    cudaMemcpy(d_elevation, landscape.elevation.data(), n_row * n_col * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_fwi, landscape.fwi.data(), n_row * n_col * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_aspect, landscape.aspect.data(), n_row * n_col * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_wind_dir, landscape.wind_dir.data(), n_row * n_col * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_vegetation_type, landscape.vegetation_type.data(), n_row * n_col * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_burnable, landscape.burnable.data(), n_row * n_col * sizeof(uint8_t), cudaMemcpyHostToDevice);

    SimulationParams* d_params;
    cudaMalloc(&d_params, sizeof(SimulationParams));
    cudaMemcpy(d_params, &params, sizeof(SimulationParams), cudaMemcpyHostToDevice);

    // Comenzar simulación
    double start_time = omp_get_wtime();
    unsigned int processed_cells = 0;

    cudaMemcpy(d_burned_ids_0, h_burned_ids_0.data(), end * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_burned_ids_1, h_burned_ids_1.data(), end * sizeof(int), cudaMemcpyHostToDevice);

    int* current_burned_ids_0 = d_burned_ids_0;
    int* current_burned_ids_1 = d_burned_ids_1;
    int* next_burned_ids_0 = d_new_burned_ids_0;
    int* next_burned_ids_1 = d_new_burned_ids_1;

    while (burning_size > 0) {
        int threads = 256;
        int blocks = (burning_size + threads - 1) / threads;

        cudaMemset(d_new_burned_count, 0, sizeof(int));

        FireKernelParams args = {
            d_elevation, d_fwi, d_aspect, d_wind_dir, d_vegetation_type, d_burnable,
            d_burned_bin, static_cast<int>(n_col), static_cast<int>(n_row),
            current_burned_ids_0, current_burned_ids_1,
            static_cast<int>(burning_size),
            next_burned_ids_0, next_burned_ids_1, d_new_burned_count,
            distance, upper_limit, elevation_mean, elevation_sd,
            d_params
        };

        fire_step_kernel<<<blocks, threads>>>(args);

        cudaError_t err = cudaGetLastError();
        if (err != cudaSuccess) {
            printf("[CUDA ERROR] %s\n", cudaGetErrorString(err));
            break;
        }

        int new_count;
        cudaMemcpy(&new_count, d_new_burned_count, sizeof(int), cudaMemcpyDeviceToHost);

        // Copiar los nuevos IDs al host al final (sin memcpy hacia device)
        cudaMemcpy(h_burned_ids_0.data() + end, next_burned_ids_0, new_count * sizeof(int), cudaMemcpyDeviceToHost);
        cudaMemcpy(h_burned_ids_1.data() + end, next_burned_ids_1, new_count * sizeof(int), cudaMemcpyDeviceToHost);

        end += new_count;
        burning_size = new_count;
        processed_cells += new_count;
        burned_ids_steps.push_back(end);

        // Swap de buffers
        std::swap(current_burned_ids_0, next_burned_ids_0);
        std::swap(current_burned_ids_1, next_burned_ids_1);
    }

    double end_time = omp_get_wtime();
    double time_taken = end_time - start_time;

    cudaMemcpy(burned_bin.data(), d_burned_bin, n_row * n_col * sizeof(int), cudaMemcpyDeviceToHost);

    // Liberar memoria
    cudaFree(d_burned_ids_0);
    cudaFree(d_burned_ids_1);
    cudaFree(d_new_burned_ids_0);
    cudaFree(d_new_burned_ids_1);
    cudaFree(d_burned_bin);
    cudaFree(d_new_burned_count);

    cudaFree(d_elevation);
    cudaFree(d_fwi);
    cudaFree(d_aspect);
    cudaFree(d_wind_dir);
    cudaFree(d_vegetation_type);
    cudaFree(d_burnable);

    std::vector<size_t> ids_0_size_t(h_burned_ids_0.begin(), h_burned_ids_0.begin() + end);
    std::vector<size_t> ids_1_size_t(h_burned_ids_1.begin(), h_burned_ids_1.begin() + end);

    return Fire{
      n_col, n_row, processed_cells, time_taken,
      burned_bin, ids_0_size_t, ids_1_size_t, burned_ids_steps
    };
}
