## Goal

To test Redis key lookup performance with the same data sets.

## Results

The output results can be found [here](results.txt). For 4M and 6M hash sizes, average lookup time is around 58-62 microseconds. 
This may be 10 times slower than `std::unordered_map` but fast enough to consider Redis as an alternative due to its many advantages.
