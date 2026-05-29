# =========================
# Compiler and ROOT setup
# =========================

rootlibs := $(shell root-config --libs)
rootflags := $(shell root-config --cflags)

CORRECTION_INCDIR := $(shell correction config --incdir)
CORRECTION_LIBDIR := $(shell correction config --libdir)

# =========================
# Directory structure
# =========================

SRCDIR  = src/src
INCDIR  = src/include
APPDIR  = src/app
EXTDIR  = src/external
OBJDIR  = src/obj

TARGET  = nanoaodrdataframe

# =========================
# Compiler settings
# =========================

CXX      = g++
LD       = g++ -m64 -g -Wall
SOFLAGS  = -shared

CXXFLAGS = -O0 -g -Wall -fmessage-length=0 \
$(rootflags) \
-fPIC \
-I$(INCDIR) \
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

SRCS := $(wildcard $(SRCDIR)/*.cpp)
SRCS += $(wildcard $(EXTDIR)/roccor/*.cpp)
SRCS += $(wildcard $(EXTDIR)/roccor/*.cc)

# >>> IMPORTANT: ADD APP (main executable)
APPSRCS := $(wildcard $(APPDIR)/*.cpp)

# =========================
# Object files
# =========================

OBJS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(filter %.cpp,$(SRCS)))
OBJS += $(patsubst %.cc,$(OBJDIR)/%.o,$(filter %.cc,$(SRCS)))
OBJS += $(patsubst %.cpp,$(OBJDIR)/%.o,$(filter %.cpp,$(APPSRCS)))
OBJS += $(OBJDIR)/rootdict.o

# =========================
# Create object directories
# =========================

$(shell mkdir -p $(OBJDIR))
$(shell mkdir -p $(OBJDIR)/src)
$(shell mkdir -p $(OBJDIR)/src/src)
$(shell mkdir -p $(OBJDIR)/src/app)
$(shell mkdir -p $(OBJDIR)/src/external/roccor)

# =========================
# Main targets
# =========================

all: $(TARGET) libnanoadrdframe.so

	@echo ""
	@echo "*********************************"
	@echo ""
	@echo "     Compilation finished :)"
	@echo ""
	@echo "*********************************"
	@echo ""

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

# =========================
# ROOT dictionary generation
# =========================

$(SRCDIR)/rootdict.C: \
$(INCDIR)/NanoAODAnalyzerrdframe.h \
$(INCDIR)/BaseAnalyser.h \
src/Linkdef.h

	rm -f $@

	rootcling \
	-I$(INCDIR) \
	-I$(EXTDIR) \
	-I$(EXTDIR)/json \
	-I$(EXTDIR)/roccor \
	-I$(CORRECTION_INCDIR) \
	$@ $^

	rm -f rootdict_rdict.pcm
	ln -sf $(SRCDIR)/rootdict_rdict.pcm .

# =========================
# ROOT dictionary object
# =========================

$(OBJDIR)/rootdict.o: $(SRCDIR)/rootdict.C
	@mkdir -p $(dir $@)
	$(CXX) -c -o $@ $(CXXFLAGS) $<

# =========================
# Generic compilation rules
# =========================

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c -o $@ $(CXXFLAGS) $<

$(OBJDIR)/%.o: %.cc
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
