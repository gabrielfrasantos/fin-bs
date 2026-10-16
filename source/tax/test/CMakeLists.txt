add_executable(fin.tax_test)
emil_build_for(fin.tax_test BOOL FIN_BS_BUILD_TESTS)
emil_add_test(fin.tax_test)

target_link_libraries(fin.tax_test PUBLIC
    gmock_main
    fin.tax
)

target_sources(fin.tax_test PRIVATE
    TestBox3TaxModel.cpp
    TestDividendTax.cpp
)
