# LASSO

## Introduction

LASSO is a parallel machine learning system that learns a regression
model from large data.  It works in either of two modes:

  1. IPM-mode.  In this mode, you start multiple training processes
  running the `mrml-lasso/train` program on one or more computers.
  Each process learns a model from its local part of data.  After all
  processes are finished, these models are aggregated into one using
  the Iterative Parameter Mixtures (IPM) technology.

  1. MPI-mode.  In this model, you start a process running the
  `mrml-lasso/mrml-lasso` program, which will start more processes
  using MPI.  After every iteration, these processes exchange their
  opinions and update the model.  Since MPI-mode induces more data
  exchanges than IPM-mode, it is less scalable.

In either mode, LASSO learns a logistic regression model with
L1-regularization using the OWL-QN training algorithm.  For more
details about this algorithm, please refer to:

  - http://en.wikipedia.org/wiki/Limited-memory_BFGS, or
  - http://research.microsoft.com/en-us/downloads/b1eb1016-1738-4bd5-83a9-370c9d498a03/default.aspx

## Motivation

This project serves as a baseline training system for the grand
challenge in IEEE ICME 2014.  To win this challenge, you need to be
able to handle large training corpus generated from real Internet
services.  You can develop your own system, or try this one.

## Installation

LASSO was developed and tested on MacOS X and Linux.  It should be able to run on FreeBSD.

### Dependents

LASSO depends on the following thirdparty libraries:

  1. protobuf
  1. boost
  1. gflags
  1. openssl
  1. libssh2
  1. mpich2

On MacOS X, it is recommended to install these packages using Homebrew.  Homebrew makes sure that all header files come to folder `/usr/loca/include` and all libraries come to `/usr/local/lib`.  

On Linux, you can install these packages using package management systems or build your own copy from source code.  In this case, you might need to edit the `CMakeLists.txt` file to tell `cmake` where these packages are installed.  Please refer to comments in `CMakeLists.txt` as a guide on how to edit it.

To make it easy to deploy LASSO on many computers, we prefer *static linking* to above libraries and the GCC runtiem library during building.  This can be controlled by adding the following line to the `CMakeLists.txt` file:

    set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc")

With `-static-libgcc`, you should not need to worry that all computers in your cluster run the same version of GCC runtime.

Notice that above linker flags are not supported on MacOS X.   It is reasonable anyway as MacOS X is a desktop system, and it is efficient for desktop applications sharing common components as shared libraries.

### Checkout and Build

With above dependents installed, you can simply checkout the code and build it using `cmake`.

    cd ~
    git clone https://github.com/wangkuiyi/lasso
    cd /tmp
    cmake ~/lasso
    make
    make install
    
The `make install` commmand copies built software to a directory specified in `CMakeLists.txt` by the directive

    set(CMAKE_INSTALL_PREFIX "/home/public/paralgo")
    