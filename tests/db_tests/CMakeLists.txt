add_executable(db_tests db_tests.cpp)
add_executable(db_size_test db_size_test.cpp)
target_link_libraries(db_tests crypto common lmdb zlibstatic ${Boost_LIBRARIES})
target_link_libraries(db_size_test crypto common lmdb zlibstatic ${Boost_LIBRARIES})
