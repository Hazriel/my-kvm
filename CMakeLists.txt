cmake_minimum_required (VERSION 3.10)
project (my-kvm)

set(CMAKE_C_FLAGS_DEBUG "-Wall -pedantic -std=c99 -g")
set(CMAKE_C_FLAGS_RELEASE "-Wall -pedantic -std=c99 -O2")

include_directories(inc/)

add_executable(my-kvm
  src/main.c
  src/io.c
  src/kvm_setup.c
  src/options.c
  src/kvm_cpu.c
  src/kvm.c
  src/error.c)

enable_testing()
