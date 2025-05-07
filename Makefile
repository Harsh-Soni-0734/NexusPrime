# Detect platform
UNAME_S := $(shell uname -s)

# Compiler and flags
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -Iinclude

ifeq ($(UNAME_S), Darwin)  # macOS specific flags
	CXXFLAGS += -stdlib=libc++ -DMACOS
endif

ifeq ($(UNAME_S), Linux)  # Linux specific flags
	CXXFLAGS += -DLINUX
endif

# Folder structure
SRCDIR = src
INCDIR = include
TESTDIR = test
OBJDIR = obj
BINDIR = bin

# Source files
SOURCES = $(TESTDIR)/main.cpp \
          $(SRCDIR)/BPlusTree.cpp \
          $(SRCDIR)/Database.cpp \
          $(SRCDIR)/SQLParser.cpp

# Object files with obj/ path
OBJECTS = $(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SOURCES)))

# Final output binary
TARGET = $(BINDIR)/NexusPrime

# Default target
all: $(TARGET)

# Link all object files to final binary
$(TARGET): $(OBJECTS)
	@mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) $(addprefix $(OBJDIR)/, $(notdir $(OBJECTS))) -o $@

# Build rule for object files from src/
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INCDIR)/%.h
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build rule for main.cpp
$(OBJDIR)/main.o: $(TESTDIR)/main.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)

.PHONY: all clean

