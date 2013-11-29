



#include "base/class_register.h"
#include "base/class_register_test_helper.h"
#include "base/scoped_ptr.h"
#include "gtest/gtest.h"


TEST(ClassRegister, CreateMapper) {
  scoped_ptr<Mapper> mapper;
  mapper.reset(CREATE_MAPPER(""));
  EXPECT_TRUE(mapper.get() == NULL);

  mapper.reset(CREATE_MAPPER("HelloMapper "));
  EXPECT_TRUE(mapper.get() == NULL);

  mapper.reset(CREATE_MAPPER("HelloWorldMapper"));
  EXPECT_TRUE(mapper.get() == NULL);

  mapper.reset(CREATE_MAPPER("HelloReducer"));
  EXPECT_TRUE(mapper.get() == NULL);

  mapper.reset(CREATE_MAPPER("WorldReducer"));
  EXPECT_TRUE(mapper.get() == NULL);

  mapper.reset(CREATE_MAPPER("SecondaryMapper"));
  EXPECT_TRUE(mapper.get() == NULL);

  mapper.reset(CREATE_MAPPER("HelloMapper"));
  ASSERT_TRUE(mapper.get() != NULL);
  EXPECT_EQ("HelloMapper", mapper->GetMapperName());

  mapper.reset(CREATE_MAPPER("WorldMapper"));
  ASSERT_TRUE(mapper.get() != NULL);
  EXPECT_EQ("WorldMapper", mapper->GetMapperName());
}

TEST(ClassRegister, CreateSecondaryMapper) {
  scoped_ptr<Mapper> mapper;
  mapper.reset(CREATE_SECONDARY_MAPPER(""));
  EXPECT_TRUE(mapper.get() == NULL);

  mapper.reset(CREATE_SECONDARY_MAPPER("SecondaryMapper "));
  EXPECT_TRUE(mapper.get() == NULL);

  mapper.reset(CREATE_SECONDARY_MAPPER("HelloWorldMapper"));
  EXPECT_TRUE(mapper.get() == NULL);

  mapper.reset(CREATE_SECONDARY_MAPPER("HelloReducer"));
  EXPECT_TRUE(mapper.get() == NULL);

  mapper.reset(CREATE_SECONDARY_MAPPER("WorldReducer"));
  EXPECT_TRUE(mapper.get() == NULL);

  mapper.reset(CREATE_SECONDARY_MAPPER("HelloMapper"));
  EXPECT_TRUE(mapper.get() == NULL);

  mapper.reset(CREATE_SECONDARY_MAPPER("WorldMapper"));
  EXPECT_TRUE(mapper.get() == NULL);

  mapper.reset(CREATE_SECONDARY_MAPPER("SecondaryMapper"));
  ASSERT_TRUE(mapper.get() != NULL);
  EXPECT_EQ("SecondaryMapper", mapper->GetMapperName());
}

TEST(ClassRegister, CreateReducer) {
  scoped_ptr<Reducer> reducer;
  reducer.reset(CREATE_REDUCER(""));
  EXPECT_TRUE(reducer.get() == NULL);

  reducer.reset(CREATE_REDUCER("HelloReducer "));
  EXPECT_TRUE(reducer.get() == NULL);

  reducer.reset(CREATE_REDUCER("HelloWorldReducer"));
  EXPECT_TRUE(reducer.get() == NULL);

  reducer.reset(CREATE_REDUCER("HelloMapper"));
  EXPECT_TRUE(reducer.get() == NULL);

  reducer.reset(CREATE_REDUCER("WorldMapper"));
  EXPECT_TRUE(reducer.get() == NULL);

  reducer.reset(CREATE_REDUCER("HelloReducer"));
  ASSERT_TRUE(reducer.get() != NULL);
  EXPECT_EQ("HelloReducer", reducer->GetReducerName());

  reducer.reset(CREATE_REDUCER("WorldReducer"));
  ASSERT_TRUE(reducer.get() != NULL);
  EXPECT_EQ("WorldReducer", reducer->GetReducerName());
}

TEST(ClassRegister, CreateFileImpl) {
  scoped_ptr<FileImpl> file_impl;
  file_impl.reset(CREATE_FILE_IMPL("/mem"));
  ASSERT_TRUE(file_impl.get() != NULL);
  EXPECT_EQ("MemFileImpl", file_impl->GetFileImplName());

  file_impl.reset(CREATE_FILE_IMPL("/nfs"));
  ASSERT_TRUE(file_impl.get() != NULL);
  EXPECT_EQ("NetworkFileImpl", file_impl->GetFileImplName());

  file_impl.reset(CREATE_FILE_IMPL("/local"));
  ASSERT_TRUE(file_impl.get() != NULL);
  EXPECT_EQ("LocalFileImpl", file_impl->GetFileImplName());

  file_impl.reset(CREATE_FILE_IMPL("/"));
  ASSERT_TRUE(file_impl.get() != NULL);
  EXPECT_EQ("LocalFileImpl", file_impl->GetFileImplName());

  file_impl.reset(CREATE_FILE_IMPL(""));
  ASSERT_TRUE(file_impl.get() != NULL);
  EXPECT_EQ("LocalFileImpl", file_impl->GetFileImplName());

  file_impl.reset(CREATE_FILE_IMPL("/mem2"));
  ASSERT_TRUE(file_impl.get() != NULL);
  EXPECT_EQ("LocalFileImpl", file_impl->GetFileImplName());

  file_impl.reset(CREATE_FILE_IMPL("/mem/"));
  ASSERT_TRUE(file_impl.get() != NULL);
  EXPECT_EQ("LocalFileImpl", file_impl->GetFileImplName());

  file_impl.reset(CREATE_FILE_IMPL("/nfs2/"));
  ASSERT_TRUE(file_impl.get() != NULL);
  EXPECT_EQ("LocalFileImpl", file_impl->GetFileImplName());
}
