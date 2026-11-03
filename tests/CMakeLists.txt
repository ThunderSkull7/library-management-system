find_package(GTest CONFIG REQUIRED)

add_executable(library_management_tests
    test_database_connection.cpp
    test_schema_manager.cpp
    test_book_model.cpp
    test_book_repository.cpp
    test_book_service.cpp
    test_book_copy_repository.cpp
    test_book_copy_service.cpp
    test_member_model.cpp
    test_member_repository.cpp
    test_member_service.cpp
    test_loan_model.cpp
    test_library_policy.cpp
    test_date_utils.cpp
    test_transaction.cpp
    test_loan_repository.cpp
    test_loan_service.cpp
)

target_link_libraries(library_management_tests
    PRIVATE
        library_management_lib
        GTest::gtest
        GTest::gtest_main
)

target_compile_features(library_management_tests PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(library_management_tests
        PRIVATE
            /W4
            /permissive-
            /EHsc
            /utf-8
    )
endif()

include(GoogleTest)
gtest_discover_tests(library_management_tests)

