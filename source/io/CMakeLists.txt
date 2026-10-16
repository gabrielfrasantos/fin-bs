add_library(fin.io STATIC)

target_include_directories(fin.io PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(fin.io PUBLIC
    fin.core
    fin.costs
    fin.estimation
    fin.sim
    fin.strategy
    fin.tax
)

target_link_libraries(fin.io PRIVATE
    infra.syntax
)

target_sources(fin.io PRIVATE
    BrokerConfig.cpp
    BrokerConfig.hpp
    PriceCsvLoader.cpp
    PriceCsvLoader.hpp
    ReportWriter.cpp
    ReportWriter.hpp
    ScenarioLoader.cpp
    ScenarioLoader.hpp
    TaxConfig.cpp
    TaxConfig.hpp
)

add_subdirectory(test)
