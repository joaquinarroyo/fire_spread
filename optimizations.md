# Data elegida: 1999_27j_N

## -O0

```
SIMULATION PERFORMANCE DATA
* Total time taken: 8.51883 seconds
* Average time: 0.0851883 seconds
* Min metric: 7.80609e+06 cells/sec processed
* Max metric: 8.24626e+06 cells/sec processed

Performance counter stats for './graphics/burned_probabilities_data ./data/1999_27j_N':

259.437.636     cache-references                                                      
58.394.106      cache-misses # 22,51% of all cache refs         

9,431469680 seconds time elapsed

9,219312000 seconds user
0,163916000 seconds sys
```

## -O1

```
SIMULATION PERFORMANCE DATA
* Total time taken: 2.57961 seconds
* Average time: 0.0257961 seconds
* Min metric: 2.48009e+07 cells/sec processed
* Max metric: 2.7575e+07 cells/sec processed

Performance counter stats for './graphics/burned_probabilities_data ./data/1999_27j_N':

204.099.977     cache-references                                                      
27.941.014      cache-misses # 13,69% of all cache refs         

2,781539577 seconds time elapsed

2,613047000 seconds user
0,165066000 seconds sys
```

## -O2

```
SIMULATION PERFORMANCE DATA
* Total time taken: 2.56369 seconds
* Average time: 0.0256369 seconds
* Min metric: 2.61433e+07 cells/sec processed
* Max metric: 2.73291e+07 cells/sec processed

Performance counter stats for './graphics/burned_probabilities_data ./data/1999_27j_N':

194.281.175      cache-references                                                      
21.094.940      cache-misses # 10,86% of all cache refs         

2,760675543 seconds time elapsed

2,601265000 seconds user
0,159016000 seconds sys
```

## -O3

SIMULATION PERFORMANCE DATA
* Total time taken: 2.63373 seconds
* Average time: 0.0263373 seconds
* Min metric: 2.03625e+07 cells/sec processed
* Max metric: 2.75079e+07 cells/sec processed

Performance counter stats for './graphics/burned_probabilities_data ./data/1999_27j_N':

207.067.642      cache-references                                                      
29.688.910      cache-misses # 14,34% of all cache refs         

2,832369103 seconds time elapsed

2,615095000 seconds user
0,200776000 seconds sys

# Data elegida: 1999_27j_S

## -O0

```
SIMULATION PERFORMANCE DATA
* Total time taken: 53.1439 seconds
* Average time: 0.531439 seconds
* Min metric: 7.55299e+06 cells/sec processed
* Max metric: 8.41035e+06 cells/sec processed

Performance counter stats for './graphics/burned_probabilities_data ./data/1999_27j_S':

2.190.430.687      cache-references                                                      
698.068.980      cache-misses                     #   31,87% of all cache refs         
245.126.701.325     cycles                                                                
470.342.684.219      instructions                     #    1,92  insn per cycle                                                              

62,151125890 seconds time elapsed

60,775751000 seconds user
1,343773000 seconds sys
```

## -O1

```
SIMULATION PERFORMANCE DATA
* Total time taken: 16.5541 seconds
* Average time: 0.165541 seconds
* Min metric: 2.44306e+07 cells/sec processed
* Max metric: 2.66016e+07 cells/sec processed

Performance counter stats for './graphics/burned_probabilities_data ./data/1999_27j_S':

1.837.799.260      cache-references                                                      
486.131.518      cache-misses                     #   26,45% of all cache refs         
72.210.093.424      cycles                                                                
131.808.141.726      instructions                     #    1,83  insn per cycle            

18,265786539 seconds time elapsed

16,815662000 seconds user
1,377726000 seconds sys
```

## -O2

```
SIMULATION PERFORMANCE DATA
* Total time taken: 16.8908 seconds
* Average time: 0.168908 seconds
* Min metric: 2.42043e+07 cells/sec processed
* Max metric: 2.6378e+07 cells/sec processed

 Performance counter stats for './graphics/burned_probabilities_data ./data/1999_27j_S':

1.794.154.825      cache-references                                                      
455.247.045      cache-misses                     #   25,37% of all cache refs         
72.510.716.130      cycles                                                                
131.110.284.081      instructions                     #    1,81  insn per cycle            

18,506734566 seconds time elapsed

17,130769000 seconds user
1,311369000 seconds sys
```

## -O3

```
SIMULATION PERFORMANCE DATA
* Total time taken: 16.9794 seconds
* Average time: 0.169794 seconds
* Min metric: 2.37077e+07 cells/sec processed
* Max metric: 2.63917e+07 cells/sec processed

Performance counter stats for './graphics/burned_probabilities_data ./data/1999_27j_S':

1.820.208.108      cache-references                                                      
487.045.637      cache-misses                     #   26,76% of all cache refs         
72.764.830.506      cycles                                                                
122.076.859.610      instructions                     #    1,68  insn per cycle            

18,599761797 seconds time elapsed

17,063101000 seconds user
1,442600000 seconds sys
```

# -01 -ffast-math

Mejor hasta ahora

SIMULATION PERFORMANCE DATA
* Total time taken: 13.0172 seconds
* Average time: 0.130172 seconds
* Min metric: 2.95758e+07 cells/sec processed
* Max metric: 3.37678e+07 cells/sec processed

Performance counter stats for './graphics/burned_probabilities_data ./data/1999_27j_S':

1.702.118.727      cache-references                                                      
441.164.802      cache-misses                     #   25,92% of all cache refs         
58.416.867.862      cycles                                                                
108.830.030.279      instructions                     #    1,86  insn per cycle            

14,655296788 seconds time elapsed
13,269275000 seconds user
1,380924000 seconds sys

# -01 -ffast-math finline-functions -fhoist-adjacent-loads -fstore-merging -freorder-functions

Cambios despreciables.

```
SIMULATION PERFORMANCE DATA
* Total time taken: 13.0725 seconds
* Average time: 0.130725 seconds
* Min metric: 2.98832e+07 cells/sec processed
* Max metric: 3.37655e+07 cells/sec processed

Performance counter stats for './graphics/burned_probabilities_data ./data/1999_27j_S':

1.683.084.927      cache-references                                                      
426.496.167      cache-misses                     #   25,34% of all cache refs         
58.293.653.324      cycles                                                                
108.724.514.186      instructions                     #    1,87  insn per cycle            

14,812632844 seconds time elapsed
13,374500000 seconds user
1,367051000 seconds sys
```