add_executable(fin.strategy_test)
emil_build_for(fin.strategy_test BOOL FIN_BS_BUILD_TESTS)
emil_add_test(fin.strategy_test)

target_link_libraries(fin.strategy_test PUBLIC
    gmock_main
    fin.strategy
)

target_sources(fin.strategy_test PRIVATE
    TestAccount.cpp
    TestBook.cpp
    TestCollateralAllocation.cpp
    TestCompoundingLedger.cpp
    TestPortfolio.cpp
    TestProjection.cpp
    TestReinvestmentPolicy.cpp
    TestRollRules.cpp
    TestWheelEngine.cpp
)
