rm -rf build_Lunaria && rm -rf Lunaria && mkdir build_Lunaria

cd build_Lunaria

cmake ../src/ -G Ninja \
-DCMAKE_C_FLAGS="-O3 -march=native -mtune=native" \
-DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native" \
-DCMAKE_C_COMPILER_LAUNCHER=ccache \
-DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
-DCMAKE_CUDA_COMPILER_LAUNCHER=ccache \

ninja

cp ./Lunaria ../Lunaria