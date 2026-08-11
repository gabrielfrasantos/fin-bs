add_library(fin.costs STATIC)

target_include_directories(fin.costs PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(fin.costs PUBLIC
    fin.core
)

target_sources(fin.costs PRIVATE
    BrokerFees.cpp
    BrokerFees.hpp
    FeeSchedule.cpp
    FeeSchedule.hpp
    FxModel.cpp
    FxModel.hpp
    Trade.cpp
    Trade.hpp
)

add_subdirectory(test)
