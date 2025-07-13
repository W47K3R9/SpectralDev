cmake_minimum_required(VERSION 3.20)
project(spectral_main)

add_library(cmpl_flags INTERFACE)
target_compile_features(cmpl_flags INTERFACE cxx_std_23)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -O3")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

add_executable(${PROJECT_NAME} main.cpp
        inc/SpctDomainSpecific.h
        inc/SpctProcessingFunctions.h
        inc/SpctCircularBuffer.h
        inc/SpctExponentLUT.h
        inc/SpctOscillator.h
        inc/SpctWavetables.h
        test/SpctWTTest.h
        test/SpctBufferManagerTest.h
        inc/SpctOscillatorStack.h
        test/VoiceSampleArray.h
        test/SpctRealVoiceTest.h)
target_link_libraries(${PROJECT_NAME} PUBLIC cmpl_flags)
target_include_directories(${PROJECT_NAME} PUBLIC inc)
