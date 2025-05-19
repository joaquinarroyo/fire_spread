for N in 2048 4096 6144 8192 12288; do
  echo "Running dgemm $N x $N x $N..."
  ./time_dgemm $N $N $N >> dgemm_results.txt
done
for N in 12288 16384 20480 24576; do
  echo "Running sgemm $N x $N x $N..."
  ./time_sgemm $N $N $N >> sgemm_large_results.txt
done
