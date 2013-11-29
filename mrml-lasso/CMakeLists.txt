# Build protobuf
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS logistic_regression.proto)

# Build library mrml.
add_library(lasso ${PROTO_SRCS} vector_types.cc learner_states.cc learner.cc)
add_library(lasso-predict prediction_engine.cc)

# Build unittests.
set(LIBS lasso mrml sorted_buffer strutil hash base mpichcxx mpich opa ssh2 ssl crypto z dl boost_program_options boost_regex boost_filesystem boost_system protobuf gflags gtest pthread)

add_executable(sparse_vector_tmpl_test sparse_vector_tmpl_test.cc)
target_link_libraries(sparse_vector_tmpl_test gtest_main ${LIBS})

add_executable(dense_vector_tmpl_test dense_vector_tmpl_test.cc)
target_link_libraries(dense_vector_tmpl_test gtest_main ${LIBS})

add_executable(vector_types_test vector_types_test.cc)
target_link_libraries(vector_types_test gtest_main ${LIBS})

add_executable(learner_states_test learner_states_test.cc)
target_link_libraries(learner_states_test gtest_main ${LIBS})

add_executable(learner_test learner_test.cc)
target_link_libraries(learner_test gtest_main ${LIBS})

add_executable(termination_flag_test termination_flag_test.cc)
target_link_libraries(termination_flag_test ${LIBS})

# Build regression test
add_executable(train train.cc)
target_link_libraries(train ${LIBS})

# Build MapReduce binaries
add_executable(mrml-lasso mrml_mappers_and_reducers.cc mr_assign_feature_id.cc mr_convert_data_format.cc command_line_options.cc)
target_link_libraries(mrml-lasso mrml-main ${LIBS})

# Build dump_learner_states
add_executable(dump_learner_states dump_learner_states.cc)
target_link_libraries(dump_learner_states ${LIBS})

# Install library and header files
install(TARGETS mrml-lasso DESTINATION bin/mrml-lasso)
FILE(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${HEADER_FILES} DESTINATION include/mrml-lasso)
