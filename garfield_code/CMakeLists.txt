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


add_executable(gasgen gasgen.cc)
target_link_libraries(gasgen Garfield::Garfield)

add_executable(gasmerge gasmerge.cc)
target_link_libraries(gasmerge Garfield::Garfield)

add_executable(sigvtime sigvtime.cc)
target_link_libraries(sigvtime Garfield::Garfield)

add_executable(sigvtime_drift sigvtime_drift.cc)
target_link_libraries(sigvtime_drift Garfield::Garfield)

add_executable(sigvtime_drift_multitrack sigvtime_drift_multitrack.cc)
target_link_libraries(sigvtime_drift_multitrack Garfield::Garfield)

add_executable(sigvtime_sweep_tracks sigvtime_sweep_tracks.cc)
target_link_libraries(sigvtime_sweep_tracks Garfield::Garfield)