#
# This Makefile is for development use only, it needs to be updated or generated
# from autoconf for cross-platform deployment
#

CXX := g++
SRCDIR := src
BUILDDIR := build
TARGET := lib/libembed.so
TESTS := 

INCLUDE := -I include
CPPFLAGS := $(INCLUDE)
CXXFLAGS := -Wall -fPIC
LDLIBS = 
LDFLAGS := -Llib -shared

SRCEXT := cpp
SOURCES := $(notdir $(shell find $(SRCDIR) -type f -name *.$(SRCEXT)))
OBJECTS := $(addprefix $(BUILDDIR)/, $(SOURCES:.$(SRCEXT)=.o))

VPATH = $(shell find $(SRCDIR)/* -type d)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) $(LOADLIBS)

$(BUILDDIR)/%.o: %.$(SRCEXT)
	$(CXX) $^ -c -o $@ $(CPPFLAGS) $(CXXFLAGS)

clean:
	rm -rf $(BUILDDIR)/* $(TARGET)

.PHONY: clean
