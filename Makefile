CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
	# macOS doesn't need -lrt for POSIX shared memory
	LIBS = -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-network -lsfml-system
	CXXFLAGS += -I/opt/homebrew/opt/sfml@2/include
	LDFLAGS += -L/opt/homebrew/opt/sfml@2/lib
else
	LIBS = -lsfml-graphics -lsfml-window -lsfml-audio -lsfml-network -lsfml-system -lrt
endif

TARGETS = arbiter/arbiter hip/hip asp/asp


SHARED_HEADERS = shared/game_config.h shared/game_state.h shared/sync.h \
                 shared/weapons.h shared/actions.h shared/artifacts.h \
                 shared/inventory.h shared/log_buffer.h

all: clean $(TARGETS)
	@echo Build complete.

arbiter/arbiter: arbiter/arbiter.cpp $(SHARED_HEADERS)
	$(CXX) $(CXXFLAGS) arbiter/*.cpp -o $@ $(LDFLAGS) $(LIBS)

hip/hip: hip/hip.cpp $(SHARED_HEADERS)
	$(CXX) $(CXXFLAGS) hip/*.cpp -o $@ $(LDFLAGS) $(LIBS)

asp/asp: asp/asp.cpp $(SHARED_HEADERS)
	$(CXX) $(CXXFLAGS) asp/*.cpp -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
