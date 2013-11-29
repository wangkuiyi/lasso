# To build the whole paralgo project, you can type the following shell
# commands:
#
#  $> mkdir build
#  $> cd build
#  $> cmake ..
#  $> make
#
# Thus you check out the paralgo project and build it in a
# subdirectory ``build''.  If you want further to install the built
# project, you can modify the default installation directory:
#
#   set(CMAKE_INSTALL_PREFIX "/home/public/paralgo")
#
# and type the command
#
#  $> make install
#
# If you want to use distcc for a distributed build, substitute above
# command
#   cmake ../paralgo
# by
#   CC=distcc cmake ../paralgo
#
project ("paralgo")

cmake_minimum_required(VERSION 2.8)  # Requires 2.8 for protobuf support.

#------------------------------------------------------------------------------
# Add protobuf compilation support
#------------------------------------------------------------------------------
include("FindProtobuf")
find_package(Protobuf REQUIRED)

#------------------------------------------------------------------------------
# Take almost all warnining;
# Take warnings as errors;
# Do not generate debug symbols;
# Optimization level 2;
#------------------------------------------------------------------------------
add_definitions(" -Wall -Wno-sign-compare -Werror -O2 ")

#------------------------------------------------------------------------------
# Declare where our project will be installed.
#------------------------------------------------------------------------------
set(CMAKE_INSTALL_PREFIX "/home/public/paralgo")

#------------------------------------------------------------------------------
# The following flags ensure that executables are statically linked
# with libraries.  This makes it easy to deploy your executable across
# a computer cluster.  However, MacOS X does not support these flags,
# so you might want to comment them out if you develop on MacOS X.
# ------------------------------------------------------------------------------
# set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc")

#------------------------------------------------------------------------------
# Declare where the third party libraries were installed.
#
# If you are building on and for Linux, it is recommended to install
# all the following dependents by yourself, using --enable-static and
# --disabel-shared flags with the configure script.  This ensures that
# your binary links statistically with all dependents and is the only
# file you need to deploy.
#
#  MRML-lasso depends on the following thridparty pacakges:
#
#  - protobuf
#  - boost
#  - gflags
#  - openssl
#  - libssh2
#  - mpich2
#
# However, if you are building on MacOS X. It would be much easier to
# install above package using Homebrew, and you will find all headers
# files at /usr/local/include and all libraries in /usr/local/lib.
# ------------------------------------------------------------------------------
# set(THIRD_PARTY_DIR "${PROJECT_SOURCE_DIR}/../thirdparty")
# set(PROTOBUF_DIR "${THIRD_PARTY_DIR}/protobuf")
# set(BOOST_DIR "${THIRD_PARTY_DIR}/boost")
# set(GFLAGS_DIR "${THIRD_PARTY_DIR}/gflags")
# set(MPICH2_DIR "${THIRD_PARTY_DIR}/mpich")
# set(OPENSSL_DIR "${THIRD_PARTY_DIR}/openssl")
# set(LIBSSH2_DIR "${THIRD_PARTY_DIR}/libssh2")

#------------------------------------------------------------------------------
# Set include paths.
# Add new lines below if you installed more 3rd-party libs.
#------------------------------------------------------------------------------
include_directories(
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}/gtest-1.7.0/include"
  # "${PROTOBUF_DIR}/include";
  # "${MPICH2_DIR}/include";
  # "${OPENSSL_DIR}/include";
  # "${LIBSSH2_DIR}/include";
  # "${BOOST_DIR}";
  # "${GFLAGS_DIR}/include";
  "/usr/local/include"
  )

#------------------------------------------------------------------------------
# Set libray paths.
# Add new lines below if you add new packages in paralgo project, or installed
# more 3rd-party libs.
#------------------------------------------------------------------------------
link_directories(
  "${PROJECT_BINARY_DIR}/base"
  "${PROJECT_BINARY_DIR}/strutil"
  "${PROJECT_BINARY_DIR}/hash"
  "${PROJECT_BINARY_DIR}/sorted_buffer"
  "${PROJECT_BINARY_DIR}/mrml"
  # "${PROTOBUF_DIR}/lib";
  # "${MPICH2_DIR}/lib";
  # "${OPENSSL_DIR}/lib";
  # "${LIBSSH2_DIR}/lib";
  # "${BOOST_DIR}/stage/lib";
  # "${GFLAGS_DIR}/lib";
  "/usr/local/lib"
  )

#------------------------------------------------------------------------------
# Declare packages in paralgo project.
# Add new lines below if you add new packages in paralgo project.
#------------------------------------------------------------------------------
add_subdirectory(gtest-1.7.0)
add_subdirectory(base)
add_subdirectory(strutil)
add_subdirectory(hash)
add_subdirectory(sorted_buffer)
add_subdirectory(mrml)
add_subdirectory(mrml-lasso)
