add_library(fin.tax STATIC)

target_include_directories(fin.tax PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(fin.tax PUBLIC
    fin.core
)

target_sources(fin.tax PRIVATE
    Box3Parameters.cpp
    Box3Parameters.hpp
    Box3TaxModel.cpp
    Box3TaxModel.hpp
    DividendTax.cpp
    DividendTax.hpp
)

add_subdirectory(test)
