add_library(fin.gui STATIC)

set_target_properties(fin.gui PROPERTIES AUTOMOC On)

target_include_directories(fin.gui PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(fin.gui PUBLIC
    fin.present
    Qt6::Widgets
)

target_sources(fin.gui PRIVATE
    FanChartWidget.cpp
    FanChartWidget.hpp
    MainWindow.cpp
    MainWindow.hpp
    ResultsPanel.cpp
    ResultsPanel.hpp
    ScenarioPanel.cpp
    ScenarioPanel.hpp
)

add_subdirectory(test)
