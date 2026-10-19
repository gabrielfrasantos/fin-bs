function(fin_bs_add_gui_test target source)
    add_executable(${target})
    set_target_properties(${target} PROPERTIES AUTOMOC On)
    emil_build_for(${target} BOOL FIN_BS_BUILD_TESTS)
    emil_add_test(${target})

    target_link_libraries(${target} PUBLIC
        fin.gui
        Qt6::Test
    )

    target_compile_definitions(${target} PRIVATE
        FIN_BS_CONFIG_DIRECTORY="${PROJECT_SOURCE_DIR}/config"
        FIN_BS_DATA_DIRECTORY="${PROJECT_SOURCE_DIR}/data"
    )

    target_sources(${target} PRIVATE ${source})

    if (FIN_BS_BUILD_TESTS)
        set_tests_properties(${target} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
    endif()
endfunction()

fin_bs_add_gui_test(fin.gui_chart_test TestFanChartItem.cpp)
fin_bs_add_gui_test(fin.gui_scenario_test TestScenarioModel.cpp)
fin_bs_add_gui_test(fin.gui_run_test TestRunModel.cpp)
fin_bs_add_gui_test(fin.gui_window_test TestWindow.cpp)
