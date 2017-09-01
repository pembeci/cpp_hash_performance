## Goal

To test Redis key lookup performance with the same data sets.

## Results

The output results can be found [here](results.txt). For 4M and 6M hash sizes, average lookup time is around 58-62 microseconds. 
This may be 10 times slower than `std::unordered_map` but fast enough to consider Redis as an alternative due to its many advantages.

## Dependencies

Install C++ Redis client from here: https://github.com/Cylix/cpp_redis. Instructions are [here](https://github.com/Cylix/cpp_redis/wiki/Windows-Install). Redis for Windows can be installed from [here](https://github.com/MicrosoftArchive/redis/releases/tag/win-3.0.504). 
