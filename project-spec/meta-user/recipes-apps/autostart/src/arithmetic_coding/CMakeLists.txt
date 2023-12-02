cmake_minimum_required(VERSION 3.10)

project(ArithmeticCoding)

add_executable(test 
    test.cpp 
    ArithmeticCoder.cpp 
    BitIoStream.cpp 
    GmmTable.cpp
)



set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
