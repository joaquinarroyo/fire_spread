# Informe III

__Integrantes__: Arroyo Joaquin y Bolzan Francisco

__Laboratorio__: fire_spread

## Notas

Realizamos la optimización sobre la simuluación __burned_probabilities__.

Estos resultados fueron obtenidos sobre PC personal.

## Estrategias

__[1]__ Paralelizar los bucles internos que realizan los calculos sobre los vecinos de una celda quemada.

```cpp
#pragma omp for ...
for (size_t n = 0; n < 8; ++n) {
    ...
}
```

__[2]__ Paralelizar el bucle que recorre las celdas quemadas.

```cpp
#pragma omp for ...
for (size_t b = start; b < end; b++) {
    // b es el índice de la celda quemada
    size_t burning_cell_0 = burned_ids_0[b];
    size_t burning_cell_1 = burned_ids_1[b];
    ...
}
```

__[3]__ Paralelizar le ejecución secuencial de múltiples simulaciones.

```cpp
#pragma omp parallel
{ 
    // Variables locales
    #pragma omp for schedule(dynamic, 10)
    for (size_t i = 0; i < n_replicates; i++) {
        Fire fire = simulate_fire(...);
        // Métrica y tiempo
        ...
    }
    // Acumulación de métricas y probabilidades
    #pragma omp critical
    {
        ...
    }
    ...
}
```

## Resultados

### Tiempo

![](../final_plots/burned_probabilities_times_bar_3.png)

### Tiempo x Threads

Sobre dataset __1999_27j_S__

![](../final_plots/burned_probabilities_time_threads_bar_3.png)

### Métrica

![](../final_plots/burned_probabilities_perf_bar_3.png)

## Roofline

![](../final_plots/roofline_10_3.png)

## Conclusiones

- El tiempo singular de cada simulación bajó, eso explica la reducción en la métrica
- EL tiempo general de la simulación mejoró aproximadamente un 2.5x

## Potenciales mejoras

- Se podría mejorar el procesamiento de las celdas quemadas, paralelizandolo