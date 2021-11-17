cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
message("first not creat root project")
project(IOTmasterCtrl VERSION 1.0.1)

#link_directories(/disk4/SmartBox/output/mesonaxg_sai/target/usr/lib/)
message("add_subdirectory(src)")
add_subdirectory(src)