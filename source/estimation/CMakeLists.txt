add_library(fin.estimation STATIC)

target_include_directories(fin.estimation PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(fin.estimation PUBLIC
    fin.core
)

target_link_libraries(fin.estimation PRIVATE
    numerical.estimators.offline
    numerical.filters.passive
    numerical.math
    numerical.solver
)

target_sources(fin.estimation PRIVATE
    DriftEstimator.cpp
    DriftEstimator.hpp
    EmaVolatilityEstimator.cpp
    EmaVolatilityEstimator.hpp
    HistoricalVolatility.cpp
    HistoricalVolatility.hpp
    LineFit.cpp
    LineFit.hpp
    LogReturns.cpp
    LogReturns.hpp
    MarketParameters.cpp
    MarketParameters.hpp
    OrdinaryLeastSquaresFit.cpp
    OrdinaryLeastSquaresFit.hpp
    PriceSeries.cpp
    PriceSeries.hpp
    RlsVolatilityEstimator.cpp
    RlsVolatilityEstimator.hpp
    TotalLeastSquaresFit.cpp
    TotalLeastSquaresFit.hpp
)

add_subdirectory(test)
add_subdirectory(test_doubles)
