cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

link_directories(/usr/local/arm/lib)   # 动态库的路径一定要放在add_executable前面！！!

add_executable(IOTmasterCtrl          
        main_ctrl.c
        dev_info.c
        #my_sqlite3.c
        mr_protocol.c
)

target_include_directories(IOTmasterCtrl 
        PUBLIC
                "${IOTmasterCtrl_SOURCE_DIR}/inc"
                /usr/local/include/
                /usr/local/arm/include
        )
        
#target_link_libraries(IOTmasterCtrl  pthread zlog sqlite3)
target_link_libraries(IOTmasterCtrl  pthread)

set_target_properties(IOTmasterCtrl PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${IOTmasterCtrl_SOURCE_DIR}/bin)
