add_executable(fin.costs_test)
emil_build_for(fin.costs_test BOOL FIN_BS_BUILD_TESTS)
emil_add_test(fin.costs_test)

target_link_libraries(fin.costs_test PUBLIC
    gmock_main
    fin.costs
)

target_sources(fin.costs_test PRIVATE
    TestBrokerFees.cpp
    TestFeeSchedule.cpp
    TestFxModel.cpp
    TestTrade.cpp
)
