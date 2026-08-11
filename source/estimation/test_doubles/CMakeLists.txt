add_library(fin.estimation_test_doubles STATIC)
emil_build_for(fin.estimation_test_doubles BOOL FIN_BS_BUILD_TESTS)

target_include_directories(fin.estimation_test_doubles PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../../>"
)

target_link_libraries(fin.estimation_test_doubles PUBLIC
    fin.estimation
)

target_sources(fin.estimation_test_doubles PRIVATE
    GbmSeries.cpp
    GbmSeries.hpp
)
