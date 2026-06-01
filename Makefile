# =========================
# ROOT + correctionlib setup
# =========================

rootlibs := $(shell root-config --libs)
rootflags := $(shell root-config --cflags)

CORRECTION_INCDIR := $(shell correction config --incdir)
CORRECTION_LIBDIR := $(shell correction config --libdir)

# =========================
# Project structure
# =========================

SRCDIR   = src/src
INCDIR   = src/include
EXTDIR   = src/external
OBJDIR   = src/obj
APPDIR   = src/app

TARGET   = nanoaodrdataframe

# =========================
# Compiler
# =========================

CXX     = g++
LD      = g++ -m64 -g -Wall
SOFLAGS = -shared

CXXFLAGS = -O0 -g -Wall -fPIC \
    $(rootflags) \
    -I$(INCDIR) \
    -I$(INCDIR)/modules \
    -I$(EXTDIR) \
    -I$(EXTDIR)/json \
    -I$(EXTDIR)/roccor \
    -I$(CORRECTION_INCDIR) \
    -I.

LIBS_EXE = $(rootlibs) -lMathMore -lGenVector \
           -lcorrectionlib -L$(CORRECTION_LIBDIR)

LIBS = $(rootlibs)

# =========================
# Source files
# =========================

SRCS  := $(wildcard $(SRCDIR)/*.cpp)
SRCS  += $(wildcard $(SRCDIR)/modules/*.cpp)
SRCS  += $(wildcard $(EXTDIR)/roccor/*.cpp)
SRCS  += $(wildcard $(EXTDIR)/roccor/*.cc)
SRCS  += $(APPDIR)/nanoaodrdataframe.cpp

# =========================
# Object files
# =========================

OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.cc=.o)

OBJS := $(patsubst src/%, $(OBJDIR)/src/%, $(OBJS))
OBJS += $(OBJDIR)/rootdict.o

# =========================
# Create directories
# =========================

$(shell mkdir -p $(OBJDIR)/src)
$(shell mkdir -p $(OBJDIR)/src/modules)
$(shell mkdir -p $(OBJDIR)/src/external/roccor)
$(shell mkdir -p $(OBJDIR)/src/app)

# =========================
# Default target
# =========================

all: $(TARGET) libnanoadrdframe.so
	@echo ""
	@echo "================================="
	@echo " Build complete"
	@echo "================================="
	@echo ""

# =========================
# ROOT dictionary
# =========================

$(SRCDIR)/rootdict.C: \
$(INCDIR)/NanoAODAnalyzerrdframe.h \
$(INCDIR)/BaseAnalyser.h \
src/Linkdef.h

	rm -f $@
	rootcling \
	    -I$(INCDIR) \
	    -I$(INCDIR)/modules \
	    -I$(EXTDIR) \
	    -I$(EXTDIR)/json \
	    -I$(EXTDIR)/roccor \
	    -I$(CORRECTION_INCDIR) \
	    $@ $^

	rm -f rootdict_rdict.pcm
	ln -sf $(SRCDIR)/rootdict_rdict.pcm .

# =========================
# Compile rules
# =========================

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c -o $@ $(CXXFLAGS) $<

$(OBJDIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) -c -o $@ $(CXXFLAGS) $<

$(OBJDIR)/rootdict.o: $(SRCDIR)/rootdict.C
	@mkdir -p $(dir $@)
	$(CXX) -c -o $@ $(CXXFLAGS) $<

# =========================
# Shared library
# =========================

libnanoadrdframe.so: $(OBJS)
	$(LD) $(SOFLAGS) $(LIBS) -o $@ $^

# =========================
# Executable
# =========================

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS_EXE)

# =========================
# Clean
# =========================

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)
	rm -f libnanoadrdframe.so
	rm -f $(SRCDIR)/rootdict.C
	rm -f rootdict_rdict.pcm
	rm -rf .nfs*
