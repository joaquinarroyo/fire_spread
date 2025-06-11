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
    const int width;
    const int height;

    unsigned int* processed_cells;
    
    const SimulationParams* params;

    const float distance;
    const float upper_limit;
    const float elevation_mean;
    const float elevation_sd;

    const int rng_seed;
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

__global__ void init_rng_kernel(curandState* states, int width, int height, int seed) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    int total = width * height;
    if (tid < total) {
        int i = tid % width;
        int j = tid / width;
        unsigned long long cell_seed = seed ^ (i * 73856093) ^ (j * 19349663);
        curand_init(cell_seed, 0, 0, &states[tid]);
    }
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
    float* probs_out
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

__global__ void fire_persistent_kernel(
    FireKernelParams args,
    int* frontier_0, int* frontier_1,
    int* frontier_size,
    int* next_frontier_0, int* next_frontier_1,
    int* next_frontier_count,
    int* done_flag,
    int* in_next_frontier,
    curandState* rng_states
) {
    const float* elevation = args.elevation;
    const float* fwi = args.fwi;
    const float* aspect = args.aspect;
    const float* wind_dir = args.wind_dir;
    const float* vegetation_type = args.vegetation_type;
    const uint8_t* burnable = args.burnable;

    int* burned_bin = args.burned_bin;
    int width = args.width;
    int height = args.height;
    const SimulationParams* params = args.params;

    float distance = args.distance;
    float upper_limit = args.upper_limit;
    float elevation_mean = args.elevation_mean;
    float elevation_sd = args.elevation_sd;

    unsigned int local_processed_cells = 0;

    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    while (!*done_flag) {
        int frontier_len = *frontier_size;

        for (int idx = tid; idx < frontier_len; idx += gridDim.x * blockDim.x) {
            int i = frontier_0[idx];
            int j = frontier_1[idx];
            int center_idx = j * width + i;

            curandState local_state = rng_states[center_idx];

            float elev_c = elevation[center_idx];
            float wind_c = wind_dir[center_idx];

            int n_coords_0[8], n_coords_1[8];
            int n_indices[8];
            uint8_t n_out_flags[8];
            float n_elev[8], n_fwi[8], n_asp[8], n_veg[8], n_burn[8], n_upper[8];

            for (int n = 0; n < 8; ++n) {
                int ni = i + d_moves[n][0];
                int nj = j + d_moves[n][1];
                n_coords_0[n] = ni;
                n_coords_1[n] = nj;

                if (ni < 0 || nj < 0 || ni >= width || nj >= height) {
                    n_out_flags[n] = 1;
                    n_indices[n] = 0;
                    n_elev[n] = n_fwi[n] = n_asp[n] = n_veg[n] = 0.0f;
                    n_burn[n] = 0;
                } else {
                    int n_idx = nj * width + ni;
                    n_out_flags[n] = 0;
                    n_indices[n] = n_idx;
                    n_elev[n] = elevation[n_idx];
                    n_fwi[n] = fwi[n_idx];
                    n_asp[n] = aspect[n_idx];
                    n_veg[n] = vegetation_type[n_idx];
                    n_burn[n] = burnable[n_idx];
                    ++local_processed_cells;
                }

                uint8_t burnable_mask = (!burned_bin[n_indices[n]] && n_burn[n]);
                uint8_t valid_mask = !n_out_flags[n] && burnable_mask;
                n_upper[n] = valid_mask * upper_limit;
            }

            float n_probs[8];
            spread_probability_device(
                elev_c, wind_c,
                n_elev, n_veg, n_fwi, n_asp, n_upper,
                params, distance, elevation_mean, elevation_sd,
                n_probs
            );

            for (int n = 0; n < 8; ++n) {
                if (!burned_bin[n_indices[n]]) {
                    float rnd = curand_uniform(&local_state);
                    if (rnd < n_probs[n]) {
                        if (atomicExch(&burned_bin[n_indices[n]], 1) == 0) {
                            if (atomicCAS(&in_next_frontier[n_indices[n]], 0, 1) == 0) {
                                int pos = atomicAdd(next_frontier_count, 1);
                                next_frontier_0[pos] = n_coords_0[n];
                                next_frontier_1[pos] = n_coords_1[n];
                            }
                        }
                    }
                }
            }
            rng_states[center_idx] = local_state;
        }


        __syncthreads();

        for (int idx = tid; idx < width * height; idx += gridDim.x * blockDim.x) {
            in_next_frontier[idx] = 0;
        }

        __syncthreads();

        if (tid == 0) {
            int count = *next_frontier_count;
            *frontier_size = count;
            *next_frontier_count = 0;
            *done_flag = (count == 0);
        }

        __syncthreads();

        // Swap buffers (simple double buffer swap)
        int* tmp0 = frontier_0;
        int* tmp1 = frontier_1;
        frontier_0 = next_frontier_0;
        frontier_1 = next_frontier_1;
        next_frontier_0 = tmp0;
        next_frontier_1 = tmp1;
    }

    if (local_processed_cells)
        atomicAdd(args.processed_cells, local_processed_cells);
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
    const size_t MAX_CELLS = n_row * n_col;

    std::vector<int> h_burned_ids_0(MAX_CELLS, -1);
    std::vector<int> h_burned_ids_1(MAX_CELLS, -1);
    std::vector<int> burned_bin(MAX_CELLS, 0);

    for (size_t i = 0; i < ignition_cells.size(); ++i) {
        h_burned_ids_0[i] = ignition_cells[i].first;
        h_burned_ids_1[i] = ignition_cells[i].second;
        burned_bin[utils::INDEX(ignition_cells[i].first, ignition_cells[i].second, n_col)] = 1;
    }

    std::vector<size_t> burned_ids_steps;
    burned_ids_steps.push_back(ignition_cells.size());

    // Copy 'angles' and 'moves' to constant memory
    cudaMemcpyToSymbol(d_angles, h_angles, sizeof(h_angles));
    cudaMemcpyToSymbol(d_moves, h_moves, sizeof(h_moves));

    int *d_frontier_0, *d_frontier_1;
    int *d_next_frontier_0, *d_next_frontier_1;
    int *d_frontier_size, *d_next_frontier_count, *d_done_flag;
    int *d_burned_bin;
    unsigned int *d_processed_cells;

    // Allocate memory for frontiers, flags and counters
    cudaMalloc(&d_frontier_0, MAX_CELLS * sizeof(int));
    cudaMalloc(&d_frontier_1, MAX_CELLS * sizeof(int));
    cudaMalloc(&d_next_frontier_0, MAX_CELLS * sizeof(int));
    cudaMalloc(&d_next_frontier_1, MAX_CELLS * sizeof(int));
    cudaMalloc(&d_frontier_size, sizeof(int));
    cudaMalloc(&d_next_frontier_count, sizeof(int));
    cudaMalloc(&d_done_flag, sizeof(int));
    cudaMalloc(&d_burned_bin, MAX_CELLS * sizeof(int));
    cudaMalloc(&d_processed_cells, sizeof(unsigned int));

    int init_size = ignition_cells.size();
    cudaMemcpy(d_frontier_0, h_burned_ids_0.data(), ignition_cells.size() * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_frontier_1, h_burned_ids_1.data(), ignition_cells.size() * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_frontier_size, &init_size, sizeof(int), cudaMemcpyHostToDevice);
    cudaMemset(d_next_frontier_count, 0, sizeof(int));
    cudaMemset(d_done_flag, 0, sizeof(int));
    cudaMemcpy(d_burned_bin, burned_bin.data(), MAX_CELLS * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemset(d_processed_cells, 0, sizeof(unsigned int));

    // Copy landscape
    float *d_elevation, *d_fwi, *d_aspect, *d_wind_dir, *d_vegetation_type;
    uint8_t *d_burnable;
    cudaMalloc(&d_elevation, MAX_CELLS * sizeof(float));
    cudaMalloc(&d_fwi, MAX_CELLS * sizeof(float));
    cudaMalloc(&d_aspect, MAX_CELLS * sizeof(float));
    cudaMalloc(&d_wind_dir, MAX_CELLS * sizeof(float));
    cudaMalloc(&d_vegetation_type, MAX_CELLS * sizeof(float));
    cudaMalloc(&d_burnable, MAX_CELLS * sizeof(uint8_t));

    cudaMemcpy(d_elevation, landscape.elevation.data(), MAX_CELLS * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_fwi, landscape.fwi.data(), MAX_CELLS * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_aspect, landscape.aspect.data(), MAX_CELLS * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_wind_dir, landscape.wind_dir.data(), MAX_CELLS * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_vegetation_type, landscape.vegetation_type.data(), MAX_CELLS * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_burnable, landscape.burnable.data(), MAX_CELLS * sizeof(uint8_t), cudaMemcpyHostToDevice);

    int *d_in_next_frontier;
    cudaMalloc(&d_in_next_frontier, MAX_CELLS * sizeof(int));
    cudaMemset(d_in_next_frontier, 0, MAX_CELLS * sizeof(int));

    // Copy simulation params
    SimulationParams* d_params;
    cudaMalloc(&d_params, sizeof(SimulationParams));
    cudaMemcpy(d_params, &params, sizeof(SimulationParams), cudaMemcpyHostToDevice);

    // Prepare kernel arguments
    FireKernelParams args = {
        d_elevation, d_fwi, d_aspect, d_wind_dir, d_vegetation_type,
        d_burnable, d_burned_bin,
        static_cast<int>(n_col), static_cast<int>(n_row),
        d_processed_cells,
        d_params,
        distance, upper_limit, elevation_mean, elevation_sd,
        123 + n_replicate
    };

    // Launch the kernels
    int threads = 512;
    int blocks = (MAX_CELLS + threads - 1) / threads;

    // Initialize RNG states
    curandState* d_rng_states;
    cudaMalloc(&d_rng_states, MAX_CELLS * sizeof(curandState));
    init_rng_kernel<<<blocks, threads>>>(d_rng_states, n_col, n_row, 123 + n_replicate);

    // Initialize the fire simulation
    double start_time = omp_get_wtime();
    fire_persistent_kernel<<<blocks, threads>>>(
        args,
        d_frontier_0, d_frontier_1, d_frontier_size,
        d_next_frontier_0, d_next_frontier_1, d_next_frontier_count,
        d_done_flag,
        d_in_next_frontier,
        d_rng_states
    );
    cudaDeviceSynchronize();

    if (cudaGetLastError() != cudaSuccess) {
        std::cerr << "CUDA error: " << cudaGetErrorString(cudaGetLastError()) << std::endl;
    }

    double end_time = omp_get_wtime();
    double time_taken = end_time - start_time;

    // Copy results
    cudaMemcpy(burned_bin.data(), d_burned_bin, MAX_CELLS * sizeof(int), cudaMemcpyDeviceToHost);
    std::vector<size_t> ids_0, ids_1;
    for (size_t j = 0; j < n_row; ++j) {
        for (size_t i = 0; i < n_col; ++i) {
            if (burned_bin[utils::INDEX(i, j, n_col)]) {
                ids_0.push_back(i);
                ids_1.push_back(j);
            }
        }
    }

    // Copy processed cells count
    unsigned int processed_cells;
    cudaMemcpy(&processed_cells, d_processed_cells, sizeof(unsigned int), cudaMemcpyDeviceToHost);

    // Free device memory
    cudaFree(d_frontier_0); cudaFree(d_frontier_1);
    cudaFree(d_next_frontier_0); cudaFree(d_next_frontier_1);
    cudaFree(d_frontier_size); cudaFree(d_next_frontier_count); cudaFree(d_done_flag);
    cudaFree(d_burned_bin); cudaFree(d_processed_cells);

    cudaFree(d_elevation); cudaFree(d_fwi); cudaFree(d_aspect);
    cudaFree(d_wind_dir); cudaFree(d_vegetation_type); cudaFree(d_burnable);
    cudaFree(d_params);

    return Fire{
        n_col, n_row, processed_cells, time_taken,
        burned_bin, ids_0, ids_1, burned_ids_steps
    };
}