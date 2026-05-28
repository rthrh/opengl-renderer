emcmake cmake -B build-wasm -DCMAKE_BUILD_TYPE=Release -DASSIMP_BUILD_ZLIB=ON
cmake --build build-wasm/ -j4
