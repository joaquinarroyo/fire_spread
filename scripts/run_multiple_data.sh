#!/bin/bash

# Lista de data a procesar
data=("2005_6" "1999_28" "1999_27j_N" "1999_27j_S" "2021_865" "2015_50")
data=("1999_27j_S")
if [ -z "$1" ]; then
  echo "Por favor, ingresa 'burned_probabilities' o 'fire_animation' como argumento"
  exit 1
fi

if [[ "$1" != "burned_probabilities" && "$1" != "fire_animation" ]]; then
  echo "Por favor, ingresa 'burned_probabilities' o 'fire_animation' como argumento"
  exit 1
fi

# Definir el programa por defecto
program="burned_probabilities_data"
if [ "$1" == "fire_animation" ]; then
  program="fire_animation_data"
fi

# Cantidad de repeticiones
reps=1

for rep in $(seq 1 $reps); do
  echo "Repetición $rep"
  i=0
  for name in "${data[@]}"; do
    echo "$i. Ejecutando $program sobre $name (Repetición $rep)"
    sudo perf stat -e fp_ret_sse_avx_ops.all,cache-references,cache-misses,cycles,instructions \
      ./graphics/$program ./data/$name 2>&1 | tee ./stats/rep${rep}_${i}_${name}.txt
    i=$((i+1))
  done
done
