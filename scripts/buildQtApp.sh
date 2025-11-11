rm -rf build && rm -rf Lunaria
mkdir build
cd build
cmake .. -G Ninja
ninja

cp ./Lunaria ../Lunaria