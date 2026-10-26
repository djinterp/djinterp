# djinterp - C++ filesystem layer: reference test build
#
# Canonical build is CMake (build/cmake/config, per AGENTS-cmake.md). This
# Makefile is a portable convenience for building/running the standalone (_sa)
# suites directly.
#
#   make                          compile every _sa suite on $(STD)
#   make STD=c++20                ...on a chosen standard
#   make sweep                    compile every suite on C++98..C++23
#   make run DJINTERP_LIB=<lib>   link against your built framework and run
#   make clean
#
# `make` / `make sweep` need only headers (yours, incl. inc/djinterp/c/fs/*.h,
# c/dstring.h, config/cpp/cfg_path.h) and prove the suites compile. `run` links
# against your framework (the c/fs + core objects), which the suites exercise.

CXX      ?= g++
STD      ?= c++17
REPO     := $(abspath $(CURDIR)/..)
INC      := $(REPO)/inc
TESTDIR  := $(REPO)/src/test/cpp
BIN      := $(CURDIR)/bin
CXXFLAGS ?= -std=$(STD) -Wall -Wextra -D_GNU_SOURCE -I$(INC)
DJINTERP_LIB ?=

SRCS := $(wildcard $(TESTDIR)/*_tests_sa.cpp)
OBJS := $(patsubst $(TESTDIR)/%.cpp,$(BIN)/%.o,$(SRCS))

.PHONY: all sweep run clean
all: $(OBJS)
	@echo "compiled $(words $(OBJS)) suite(s) on $(STD)"
$(BIN)/%.o: $(TESTDIR)/%.cpp | $(BIN)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(BIN):
	@mkdir -p $(BIN)
sweep:
	@STDS="c++98 c++11 c++14 c++17 c++20 c++23" REPO="$(REPO)" CXX="$(CXX)" sh $(CURDIR)/build_sa_tests.sh
run: | $(BIN)
ifeq ($(strip $(DJINTERP_LIB)),)
	@echo "set DJINTERP_LIB=<your built framework library or c/fs+core objects> to link and run"; exit 2
else
	@for s in $(SRCS); do n=$$(basename $$s .cpp); echo "== $$n =="; \
	   $(CXX) $(CXXFLAGS) $$s $(DJINTERP_LIB) -o $(BIN)/$$n && $(BIN)/$$n || exit 1; done
endif
clean:
	@rm -rf $(BIN)
