add_library(fin.pricing STATIC)

target_include_directories(fin.pricing PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(fin.pricing PUBLIC
    fin.core
)

target_link_libraries(fin.pricing PRIVATE
    numerical.math
)

target_sources(fin.pricing PRIVATE
    AssignmentModel.cpp
    AssignmentModel.hpp
    BlackScholes.cpp
    BlackScholes.hpp
    Greeks.cpp
    Greeks.hpp
    ImpliedVolatility.cpp
    ImpliedVolatility.hpp
    NormalDistribution.cpp
    NormalDistribution.hpp
    OptionParameters.hpp
)

add_subdirectory(test)
