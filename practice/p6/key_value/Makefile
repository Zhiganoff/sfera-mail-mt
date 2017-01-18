CXX = g++
CXXFLAGS = -O2 -g -Wall -std=c++0x
LDFLAGS = -lz

# Strict compiler options
# CXXFLAGS += -Wformat-security -Wignored-qualifiers -Winit-self \
# 		-Wswitch-default -Wfloat-equal -Wshadow -Wpointer-arith \
# 		-Wtype-limits -Wempty-body -Wlogical-op \
# 		-Wmissing-field-initializers -Wctor-dtor-privacy \
# 		-Wnon-virtual-dtor -Wstrict-null-sentinel -Wold-style-cast \
# 		-Woverloaded-virtual -Wsign-promo -Wextra -pedantic

# Directories with source code
SRC_DIR = src
INCLUDE_DIR = include

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin


# Add headers dirs to gcc search path
CXXFLAGS += -I $(INCLUDE_DIR) 

# Helper macros
# subst is sensitive to leading spaces in arguments.
make_path = $(addsuffix $(1), $(basename $(subst $(2), $(3), $(4))))
# Takes path list with source files and returns pathes to related objects.
src_to_obj = $(call make_path,.o, $(SRC_DIR), $(OBJ_DIR), $(1))
# Takes path list with object files and returns pathes to related dep. file.
# Dependency files will be generated with gcc -MM.
src_to_dep = $(call make_path,.d, $(SRC_DIR), $(DEP_DIR), $(1))

# All source files in our project that must be built into movable object code.
CXXFILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJFILES := $(call src_to_obj, $(CXXFILES))

# Default target (make without specified target).
.DEFAULT_GOAL := all

# Alias to make all targets.
.PHONY: all
all: bridge $(BIN_DIR)/key_value 

bridge:
	mkdir -p $(BIN_DIR) $(OBJ_DIR)

# Suppress makefile rebuilding.
Makefile: ;

# "Hack" to restrict order of actions:
# 1. Make bridge files target.
# 2. Generate and include dependency information.
ifneq ($(MAKECMDGOALS), clean)
-include bridge.touch
endif

# Rules for compiling targets
$(BIN_DIR)/key_value: $(OBJFILES)
	$(CXX) $(CXXFLAGS) $(filter %.o, $^) -o $@ $(LDFLAGS)

# Pattern for generating dependency description files (*.d)
$(DEP_DIR)/%.d: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -E -MM -MT $(call src_to_obj, $<) -MT $@ -MF $@ $<

# Pattern for compiling object files (*.o)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $(call src_to_obj, $<) $<

# Fictive target
.PHONY: clean
# Delete all temprorary and binary files
clean:
	rm -rf $(BUILD_DIR)

# If you still have "WTF?!" feeling, try reading teaching book
# by Mashechkin & Co. http://unicorn.ejudge.ru/instr.pdf
