



#include "base/class_register_test_helper.h"

CLASS_REGISTER_IMPLEMENT_REGISTRY(mapper_register, Mapper);
CLASS_REGISTER_IMPLEMENT_REGISTRY(second_mapper_register, Mapper);
CLASS_REGISTER_IMPLEMENT_REGISTRY(reducer_register, Reducer);
CLASS_REGISTER_IMPLEMENT_REGISTRY(file_impl_register, FileImpl);


class HelloMapper : public Mapper {
  virtual std::string GetMapperName() const {
    return "HelloMapper";
  }
};
REGISTER_MAPPER(HelloMapper);

class WorldMapper : public Mapper {
  virtual std::string GetMapperName() const {
    return "WorldMapper";
  }
};
REGISTER_MAPPER(WorldMapper);

class SecondaryMapper : public Mapper {
  virtual std::string GetMapperName() const {
    return "SecondaryMapper";
  }
};
REGISTER_SECONDARY_MAPPER(SecondaryMapper);


class HelloReducer : public Reducer {
  virtual std::string GetReducerName() const {
    return "HelloReducer";
  }
};
REGISTER_REDUCER(HelloReducer);

class WorldReducer : public Reducer {
  virtual std::string GetReducerName() const {
    return "WorldReducer";
  }
};
REGISTER_REDUCER(WorldReducer);


class LocalFileImpl : public FileImpl {
  virtual std::string GetFileImplName() const {
    return "LocalFileImpl";
  }
};
REGISTER_DEFAULT_FILE_IMPL(LocalFileImpl);
REGISTER_FILE_IMPL("/local", LocalFileImpl);

class MemFileImpl : public FileImpl {
  virtual std::string GetFileImplName() const {
    return "MemFileImpl";
  }
};
REGISTER_FILE_IMPL("/mem", MemFileImpl);

class NetworkFileImpl : public FileImpl {
  virtual std::string GetFileImplName() const {
    return "NetworkFileImpl";
  }
};
REGISTER_FILE_IMPL("/nfs", NetworkFileImpl);
