add_executable(fin.core_test)
emil_build_for(fin.core_test BOOL FIN_BS_BUILD_TESTS)
emil_add_test(fin.core_test)

target_link_libraries(fin.core_test PUBLIC
    gmock_main
    fin.core
)

target_sources(fin.core_test PRIVATE
    TestToolboxLinkage.cpp
)
