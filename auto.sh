#! /bin/bash

# rm -rf build/
# mkdir build/
# cd ./build
# cmake -DCMAKE_TOOLCHAIN_FILE="../rk3568-toolchain.cmake" ..
# make clean
# make VERBOSE=1

# 交叉编译
rm -rf build/
mkdir build/
cd ./build
cmake -DCMAKE_TOOLCHAIN_FILE="../rk3568-toolchain.cmake" ..
make clean
make 
cd ..
cp ./bin/IOTmasterCtrl /home/book/nfs_rootfs/

# gcc编译
# rm -rf build/
# mkdir build/
# cd ./build
# cmake ..
# make clean
# make 
# cd ..
