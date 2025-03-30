# Uso de benchmarks

## CPU

Buscar archivos time_dgemm.c y time_sgemm.c en Zulip

Compilarlos con 

```
gcc -o time_dgemm time_dgemm.c
gcc -o time_sgemm time_sgemm.c
```

y correr

```
./time_dgemm 8192 8192 8192
./time_sgemm 8192 8192 8192
```

Ver archivos de salida generados para la información.

## Memoria

```
git clone https://github.com/jeffhammond/STREAM.git
cd STREAM
make stream_c.exe
```

Ver output de consola para la información.