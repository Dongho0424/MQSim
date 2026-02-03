CC        := g++
LD        := g++
# Default flags (Release)
CC_FLAGS  := -std=c++11 -O3 -g -gdwarf-4

MODULES   := exec host nvm_chip nvm_chip/flash_memory sim ssd utils
SRC_DIR   := $(addprefix src/,$(MODULES)) src

# Build directories
BUILD_DIR := $(addprefix build/,$(MODULES)) build
DEBUG_DIR := $(addprefix debug_build/,$(MODULES)) debug_build

SRC       := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
# Ensure main.cpp is included and duplicates removed
SRC       := $(sort src/main.cpp $(SRC))

# Object files
OBJ       := $(patsubst src/%.cpp,build/%.o,$(SRC))
DEBUG_OBJ := $(patsubst src/%.cpp,debug_build/%.o,$(SRC))

INCLUDES  := $(addprefix -I,$(SRC_DIR))

vpath %.cpp $(SRC_DIR)

# Use $$ for variables inside define to prevent early expansion.
# This ensures $(CC_FLAGS) is evaluated when the recipe runs, not when make starts.
define make-goal
$1/%.o: %.cpp
	$$(CC) $$(CC_FLAGS) $$(INCLUDES) -c $$< -o $$@
endef


.PHONY: all checkdirs clean debug

all: checkdirs MQSim

# Debug target: Overrides CC_FLAGS to -O0
debug: CC_FLAGS := -std=c++11 -O0 -g -ggdb -gdwarf-4
debug: checkdirs_debug MQSim_debug

MQSim: $(OBJ)
	$(LD) $^ -o $@

MQSim_debug: $(DEBUG_OBJ)
	$(LD) $^ -o MQSim_debug

checkdirs: $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $@

checkdirs_debug: $(DEBUG_DIR)

$(DEBUG_DIR):
	mkdir -p $@

clean:
	rm -rf build debug_build
	rm -f MQSim MQSim_debug

# Generate rules for both release and debug builds
$(foreach bdir,$(BUILD_DIR),$(eval $(call make-goal,$(bdir))))
$(foreach ddir,$(DEBUG_DIR),$(eval $(call make-goal,$(ddir))))