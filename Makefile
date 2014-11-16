#
# This Makefile is for development use only, it needs to be updated or generated
# from autoconf for cross-platform deployment
#

CXX := g++
SRCEXT := cpp
SRCDIR := src
TESTDIR := test
BUILDDIR := build
LIBDIR := lib
BINDIR := bin
TARGET := $(LIBDIR)/libembed.so

TARGET_INCLUDE := -I include
TARGET_CPPFLAGS := $(TARGET_INCLUDE)
TARGET_CXXFLAGS := -Wall -fPIC
TARGET_LDLIBS :=
TARGET_LDFLAGS := -L$(LIBDIR) -shared

TARGET_SOURCES := $(notdir $(shell find $(SRCDIR) -type f -name *.$(SRCEXT)))
TARGET_OBJECTS := $(addprefix $(BUILDDIR)/, $(TARGET_SOURCES:.$(SRCEXT)=.o))

VPATH = $(shell find $(SRCDIR)/* -type d) $(shell find $(TESTDIR)/* -type d)

all: embed

embed: $(TARGET)

$(TARGET): $(TARGET_OBJECTS)
	$(CXX) $(TARGET_LDFLAGS) -o $@ $(TARGET_OBJECTS) $(TARGET_LOADLIBS)

clean:
	rm -rf $(BUILDDIR)/* $(LIBDIR)/* $(BINDIR)/*

.PHONY: clean

include $(TESTDIR)/Makefile.in

tests: $(TESTS)

$(BUILDDIR)/%.o: %.$(SRCEXT)
	$(if $(findstring src/,$^), \
		$(CXX) $^ -c -o $@ $(TARGET_CPPFLAGS) $(TARGET_CXXFLAGS), \
		$(CXX) $^ -c -o $@ $(TEST_CPPFLAGS) $(TEST_CXXFLAGS))
