add_library(fin.sim STATIC)

target_include_directories(fin.sim PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(fin.sim PUBLIC
    fin.core
    fin.estimation
    fin.pricing
    fin.strategy
)

target_link_libraries(fin.sim PRIVATE
    numerical.solver
)

target_sources(fin.sim PRIVATE
    BookMonteCarlo.cpp
    BookMonteCarlo.hpp
    BookScript.cpp
    BookScript.hpp
    Calibration.cpp
    Calibration.hpp
    GbmPathGenerator.cpp
    GbmPathGenerator.hpp
    HistoricalPath.cpp
    HistoricalPath.hpp
    MonteCarlo.cpp
    MonteCarlo.hpp
    OutcomeDistribution.cpp
    OutcomeDistribution.hpp
    Rng.cpp
    Rng.hpp
    RunObserver.hpp
    WheelScript.cpp
    WheelScript.hpp
)

add_subdirectory(test)
