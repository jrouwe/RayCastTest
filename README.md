# RayCastTest

A framework for testing ray casts against triangles using various tree types and triangle encodings.

To run:

- Download ISPC from http://ispc.github.io/downloads.html (tested with v1.12) and copy ispc.exe into the folder of this README.md file
- Make sure CMake 3.10 or higher is installed 
- Run cmake_vs2017.bat to create a project file
- Open Build\TestRayCast.sln
- Compile and run the Release version

To tweak the behavior, open TestRaycast.cpp and scroll to the 'Configuration' section.

- TEST_FILE specifies which file to load
- TEST_CODEC specifies which triangle encoding to use
- TEST_TYPE specifies which tree type to use
- If you define RUN_ALL_TESTS, all different triangle/tree encodings will be tested. Results will be written to 'Timings.csv'. This can take a long time!
- Define QUICK_TEST to skip the most expensive tests
- Undefine RANDOM_RAYS to cast a regular grid of rays
- NUM_RAYS_PER_AXIS specifies how many rays per axis you want to cast (total amount of rays is NUM_RAYS_PER_AXIS^2)
- Define TEST_SPLITTERS to test the various tree splitting algorithms
- Define FLUSH_CACHE_AFTER_EVERY_RAY to flush the cache after every ray instead of after each test

For more information see: [Rouwe-TriangleEncAndBVHsForRayCasts.docx](https://github.com/jrouwe/RayCastTest/blob/master/Rouwe-TriangleEncAndBVHsForRayCasts.docx?raw=true)
