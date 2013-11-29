# Build protobuf
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS mrml.proto)

# Build library mrml.
add_library(mrml mrml_filesystem.cc mrml_reader.cc mrml.cc ${PROTO_SRCS} mrml_recordio.cc)
add_library(mrml-main mrml_main.cc)

# Build unittests.
set(LIBS mrml sorted_buffer strutil hash base mpichcxx mpich opa ssh2 ssl crypto z dl boost_program_options boost_regex boost_filesystem boost_system protobuf gflags gtest pthread)

add_executable(mrml_recordio_test mrml_recordio_test.cc)
target_link_libraries(mrml_recordio_test gtest_main ${LIBS})

add_executable(mrml_filesystem_test mrml_filesystem_test.cc)
target_link_libraries(mrml_filesystem_test gtest_main ${LIBS})

# Build utility codex
add_executable(codex codex.cc)
target_link_libraries(codex ${LIBS})

# Install library and header files
install(TARGETS mrml DESTINATION bin/mrml)
FILE(GLOB HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${HEADER_FILES} DESTINATION include/mrml)
