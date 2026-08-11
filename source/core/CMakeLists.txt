add_library(fin.core INTERFACE)

target_include_directories(fin.core INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(fin.core INTERFACE
    infra.util
    numerical.math
)

add_subdirectory(test)
