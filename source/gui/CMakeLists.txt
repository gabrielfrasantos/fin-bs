add_library(fin.gui STATIC)

set_target_properties(fin.gui PROPERTIES AUTOMOC On)

target_include_directories(fin.gui PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../../>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(fin.gui PUBLIC
    fin.present
    Qt6::Quick
    Qt6::QuickControls2
)

target_sources(fin.gui PRIVATE
    ChartSeries.cpp
    ChartSeries.hpp
    FanChartItem.cpp
    FanChartItem.hpp
    Resources.cpp
    Resources.hpp
    ResultTableModel.cpp
    ResultTableModel.hpp
    RunModel.cpp
    RunModel.hpp
    ScenarioModel.cpp
    ScenarioModel.hpp
    UnderlyingsModel.cpp
    UnderlyingsModel.hpp
    Workspace.cpp
    Workspace.hpp
)

# The window is QML, carried inside the library so that the application and the tests load the
# same files from the same place, without a path to look them up at.
qt_add_resources(fin.gui "fin_gui_qml"
    PREFIX "/fin"
    FILES
        qml/Field.qml
        qml/Main.qml
        qml/ResultsPane.qml
        qml/ScenarioPane.qml
        qml/Section.qml
        qml/TablePane.qml
        qml/UnderlyingsEditor.qml
)

add_subdirectory(test)
