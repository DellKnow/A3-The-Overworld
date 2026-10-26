#===================================================================================#
#                                    Name                                           #
#===================================================================================#

Name: Mark Suchodolskyi
Hero: DellKnow

#===================================================================================#
#                                     Lab                                           #
#===================================================================================#

Lab: L07

#===================================================================================#
#                          Instructions on Compiling                                #
#===================================================================================#

Run the command below:

cmake -S . -B build -G "MinGW Makefiles" ; cmake --build build ; .\build\a3.exe

Refresh cmake:
cmake --fresh -S . -B build -G "MinGW Makefiles"

My CMake file depends on a local Machine_Setup folder. I defined its location using:

set(MACHINE_SETUP
    "${CMAKE_CURRENT_SOURCE_DIR}/../../Machine_Setup"
)

Before compiling, update this path so that it points to the location of the 
Machine_Setup folder on your system.

#===================================================================================#
#                                    Questions                                      #
#===================================================================================#

