add_executable(fin.app)

set_target_properties(fin.app PROPERTIES AUTOMOC On)

target_link_libraries(fin.app PRIVATE
    fin.gui
)

target_compile_definitions(fin.app PRIVATE
    FIN_BS_CONFIG_DIRECTORY="${PROJECT_SOURCE_DIR}/config"
)

target_sources(fin.app PRIVATE
    Demo.cpp
    Demo.hpp
    Main.cpp
)
