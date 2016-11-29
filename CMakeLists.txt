# cmake最低版本号
cmake_minimum_required(VERSION 2.8)

# 设置GXX编译参数
add_compile_options( -g -Wall -o2)

#设置项目名称
project(MAIN)


AUX_SOURCE_DIRECTORY(${MAIN_SOURCE_DIR}   MAIN_SRC)
add_executable(main ${MAIN_SRC})


