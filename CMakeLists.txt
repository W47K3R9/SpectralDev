cmake_minimum_required(VERSION 3.20)
project(spectral_main)


# Interface library for compile features (good practice)
add_library(cmpl_flags INTERFACE)
target_compile_features(cmpl_flags INTERFACE cxx_std_23)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

add_library(LBTS_SpctInstanceController INTERFACE
    inc/SpctInstanceController.h 
)

target_include_directories(LBTS_SpctInstanceController  INTERFACE inc)
target_link_libraries(LBTS_SpctInstanceController INTERFACE cmpl_flags)
target_compile_options(LBTS_SpctInstanceController INTERFACE -O3)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -O3")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
add_executable(${PROJECT_NAME} main.cpp
        inc/SpctCalculationEngine.h
        inc/SpctProcessingFunctions.h
        inc/SpctOscillator.h
        inc/SpctDomainSpecific.h
        inc/SpctResyncOscsillators.h
        inc/SpctBufferManager.h
        inc/SpctCircularBuffer.h
        inc/SpctWavetables.h
        inc/SpctFxParameters.h
        inc/SpctExponentLUT.h
        inc/SpctTriggerManager.h
        test/SpctWTTest.h
        test/SpctBufferManagerTest.h
        test/VoiceSampleArray.h
        test/SpctRealVoiceTest.h)
target_link_libraries(${PROJECT_NAME} PRIVATE LBTS_SpctInstanceController cmpl_flags)
target_include_directories(${PROJECT_NAME} PRIVATE inc)
