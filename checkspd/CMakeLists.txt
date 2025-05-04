cmake_minimum_required(VERSION 3.10)

set( PROJECT_NAME chkspd )
project( ${PROJECT_NAME} VERSION 1.0 LANGUAGES C CXX )

add_executable(${PROJECT_NAME} chkspd.cpp)
