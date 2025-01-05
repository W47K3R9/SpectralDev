cmake_minimum_required(VERSION 3.20)
project(spectral_main)

add_library(cmpl_flags INTERFACE)
target_compile_features(cmpl_flags INTERFACE cxx_std_20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Ofast")
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

add_executable(${PROJECT_NAME} main.cpp
        inc/SpctDomainSpecific.h
        inc/SpctProcessingFunctions.h
        inc/SpctCircularBuffer.h
        inc/SpctExponentLUT.h
        inc/SpctOscillators.h
        inc/SpctWavetables.h
        test/SpctWTTest.h
        test/SpctBufferManagerTest.h)
target_link_libraries(${PROJECT_NAME} PUBLIC cmpl_flags)
target_include_directories(${PROJECT_NAME} PUBLIC inc)
