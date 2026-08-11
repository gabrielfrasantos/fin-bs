add_library(fin.optimize STATIC)

target_include_directories(fin.optimize PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(fin.optimize PUBLIC
    fin.core
    fin.costs
    fin.sim
    fin.strategy
    fin.tax
)

target_link_libraries(fin.optimize PRIVATE
    numerical.optimization
    numerical.regularization
)

target_sources(fin.optimize PRIVATE
    Objective.cpp
    Objective.hpp
    Recommendation.cpp
    Recommendation.hpp
    RiskLimits.cpp
    RiskLimits.hpp
    StrategyObjective.cpp
    StrategyObjective.hpp
    StrategySetting.cpp
    StrategySetting.hpp
    StrikeSizingOptimizer.cpp
    StrikeSizingOptimizer.hpp
)

add_subdirectory(test)
