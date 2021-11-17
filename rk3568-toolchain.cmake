# Creater @Sven
#

SET(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

#设置编译器
SET(CMAKE_C_COMPILER    /usr/local/arm/m3568-sdk-v1.0.0-ga/gcc-buildroot-9.3.0-2020.03-x86_64_aarch64-rockchip-linux-gnu/bin/aarch64-linux-gcc)
SET(CMAKE_CXX_COMPILER  /usr/local/arm/m3568-sdk-v1.0.0-ga/gcc-buildroot-9.3.0-2020.03-x86_64_aarch64-rockchip-linux-gnu/bin/aarch64-linux-g++)

# SET(CMAKE_FIND_ROOT_PATH  /usr/local/arm/m3568-sdk-v1.0.0-ga/gcc-buildroot-9.3.0-2020.03-x86_64_aarch64-rockchip-linux-gnu/lib)

# set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
# set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
# set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
# set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
#set(CMAKE_FIND_NO_INSTALL_PREFIX TRUE)

#设置编译器查找头文件的路径
# include_directories(BEFORE SYSTEM
# 			 /usr/include
# 			 /usr/lib/include
# 			 /usr/local/include
# )

# link_directories(/usr/local/arm/lib)