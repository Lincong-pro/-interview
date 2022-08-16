cmake_minimum_required(VERSION 3.1)

project(MuduoLearning LANGUAGES C CXX)

set(HEADERS base/ThreadPool.h test/sharedPointer.h)
set(SOURCES base/ThreadPool.cpp main.cc)

add_executable(Muduo ${HEADERS} ${SOURCES})