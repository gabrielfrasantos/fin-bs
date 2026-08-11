add_library(fin.strategy STATIC)

target_include_directories(fin.strategy PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(fin.strategy PUBLIC
    fin.core
    fin.costs
    fin.pricing
)

target_sources(fin.strategy PRIVATE
    CompoundingLedger.cpp
    CompoundingLedger.hpp
    Portfolio.cpp
    Portfolio.hpp
    Projection.cpp
    Projection.hpp
    ReinvestmentPolicy.cpp
    ReinvestmentPolicy.hpp
    RollRules.cpp
    RollRules.hpp
    WheelEngine.cpp
    WheelEngine.hpp
)

add_subdirectory(test)
