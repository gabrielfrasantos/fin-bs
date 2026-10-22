add_library(fin.present STATIC)

target_include_directories(fin.present PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(fin.present PUBLIC
    fin.core
    fin.costs
    fin.estimation
    fin.io
    fin.optimize
    fin.sim
    fin.strategy
    fin.tax
)

target_sources(fin.present PRIVATE
    CalibrationRow.hpp
    FanChartSeries.cpp
    FanChartSeries.hpp
    PriceHistory.cpp
    PriceHistory.hpp
    ResultTable.cpp
    ResultTable.hpp
    RunController.cpp
    RunController.hpp
    ScenarioDocument.cpp
    ScenarioDocument.hpp
)

add_subdirectory(test)
