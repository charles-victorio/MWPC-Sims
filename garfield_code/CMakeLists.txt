cmake_minimum_required(VERSION 4.3)
project(GainSim)

find_package(ROOT REQUIRED COMPONENTS Core Graf Hist Gpad)
find_package(Garfield REQUIRED
    PATHS /home/charles/garfieldpp/garfield_source/install)


set(GARFIELD_LIBS
    Garfield::Garfield
    ROOT::Core
    ROOT::Graf
    ROOT::Hist
    ROOT::Gpad
)

add_executable(gainSim gainSim.cc)
target_link_libraries(gainSim PRIVATE ${GARFIELD_LIBS})

add_executable(mwpcMicro mwpcMicro.cc)
target_link_libraries(mwpcMicro PRIVATE ${GARFIELD_LIBS})

