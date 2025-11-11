rm -rf build_Lunaria && rm -rf Lunaria
mkdir build_Lunaria
cd build_Lunaria
cmake .. -G Ninja
ninja

cp ./Lunaria ../Lunaria