rootlibs:=$(shell root-config --libs)
rootflags:=$(shell root-config --cflags)

CORRECTION_INCDIR:=$(shell correction config --incdir)
CORRECTION_LIBDIR:=$(shell correction config --libdir)

OBJDIR=build/obj
SRCDIR=src

SOFLAGS       = -shared
LD = g++ -m64 -g -Wall

CXXFLAGS = -O0 -g -Wall -fmessage-length=0 $(rootflags) -fPIC -I$(SRCDIR) -I$(CORRECTION_INCDIR) -I.
DEPFLAGS = -MMD -MP -MF $(@:.o=.d) -MT $@

# ============================================================
# Source files
# ============================================================

SRCS := $(wildcard $(SRCDIR)/*.cpp) $(SRCDIR)/helpers/GenParticleHelper.cpp $(SRCDIR)/framework/RNodeTree.cpp $(SRCDIR)/analysis/BaseAnalyser.cpp $(SRCDIR)/fakefactor/FakeFactorAnalyser.cpp $(SRCDIR)/helpers/utility.cpp $(SRCDIR)/framework/NanoAODAnalyzerrdframe.cpp

# Exclude files containing main()
COMMON_SRCS := $(filter-out \
    $(SRCDIR)/apps/nanoaodrdataframe.cpp \
    $(SRCDIR)/apps/fakefactorframe.cpp, \
    $(SRCS))

HEADERS = $(wildcard $(SRCDIR)/*.h)

# Objects used by the common library
OBJS := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(COMMON_SRCS)) $(OBJDIR)/rootdict.o
DEPS := $(OBJS:.o=.d) $(OBJDIR)/apps/nanoaodrdataframe.d $(OBJDIR)/apps/fakefactorframe.d


# ============================================================
# Libraries
# ============================================================

LIBS_EXE = $(rootlibs) -lMathMore -lGenVector -lcorrectionlib -L$(CORRECTION_LIBDIR)

LIBS = $(rootlibs)


# ============================================================
# Targets
# ============================================================

TARGET = nanoaodrdataframe
FAKE_TARGET = fakefactorframe


# ============================================================
# Default target
# ============================================================

all: $(TARGET) $(FAKE_TARGET) libnanoadrdframe.so
	@echo "";
	@echo "*********************************";
	@echo "";
	@echo "     La compilation est finie :) ";
	@echo "";
	@echo "   Quel codeur!!!!!!!!!!... <3 <3 <3";
	@echo "";
	@echo "*********************************";


# ============================================================
# Clean
# ============================================================

clean:
	rm -f $(OBJS) \
	      $(OBJDIR)/apps/nanoaodrdataframe.o \
	      $(OBJDIR)/apps/fakefactorframe.o \
	      $(TARGET) \
	      $(FAKE_TARGET) \
	      libnanoadrdframe.so \
	      $(DICT_SOURCE) \
	      rootdict_rdict.pcm
	rm -rf .nfs*


# ============================================================
# ROOT dictionary
# ============================================================

DICT_HEADERS := $(SRCDIR)/NanoAODAnalyzerrdframe.h $(SRCDIR)/BaseAnalyser.h $(SRCDIR)/FakeFactorAnalyser.h $(SRCDIR)/Linkdef.h
DICTDIR := build/dict
DICT_SOURCE := $(DICTDIR)/rootdict.C
DICT_DEPS := $(DICTDIR)/rootdict_headers.d
DICT_PCM := $(DICTDIR)/rootdict_rdict.pcm

# Track headers parsed for the dictionary, including transitive includes.
$(DICT_DEPS): $(DICT_HEADERS) Makefile
	mkdir -p $(DICTDIR)
	$(CXX) $(CXXFLAGS) -D__CLING__ -MM -MP -x c++ -MT $(DICT_DEPS) -MT $(DICT_SOURCE) -MT $(DICT_PCM) $(DICT_HEADERS) > $@.tmp
	mv $@.tmp $@

# rootcling produces both outputs in one invocation (GNU Make >= 4.3).
$(DICT_SOURCE) $(DICT_PCM) &: $(DICT_HEADERS) $(DICT_DEPS)
	mkdir -p $(DICTDIR)
	rm -f $(DICT_SOURCE)
	rootcling -I$(CORRECTION_INCDIR) -I$(SRCDIR) $(DICT_SOURCE) $(DICT_HEADERS)
	test -s $(DICT_SOURCE)
	test -s $(DICT_PCM)

# Check missing or old lookup links without changing correct links.
.PHONY: check_pcm_link
check_pcm_link:

rootdict_rdict.pcm: check_pcm_link | $(DICT_PCM)
	@if [ ! -L "$@" ] || [ "$$(readlink "$@")" != "$(DICT_PCM)" ]; then \
	    ln -sfn $(DICT_PCM) "$@"; \
	fi

$(TARGET) $(FAKE_TARGET) libnanoadrdframe.so: | rootdict_rdict.pcm


# ============================================================
# Common shared library
# ============================================================

libnanoadrdframe.so: $(OBJS)
	$(LD) $(SOFLAGS) $(LIBS) -o $@ $^


# ============================================================
# ROOT dictionary object
# ============================================================

# Visit the PCM first so grouped-output recovery precedes object checks.
$(OBJDIR)/rootdict.o: $(DICT_PCM) $(DICT_SOURCE)
	mkdir -p $(@D)
	$(CXX) -c -o $@ $(CXXFLAGS) $(DEPFLAGS) $(DICT_SOURCE)


# ============================================================
# Compile all common .cpp files
# ============================================================

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(@D)
	$(CXX) -c -o $@ $(CXXFLAGS) $(DEPFLAGS) $<


# ============================================================
# Main executable
# ============================================================

$(TARGET): $(OBJS) $(OBJDIR)/apps/nanoaodrdataframe.o
	$(CXX) -o $(TARGET) $^ $(LIBS_EXE)


# ============================================================
# Fake-factor executable
# ============================================================

$(FAKE_TARGET): $(OBJS) $(OBJDIR)/apps/fakefactorframe.o
	$(CXX) -o $(FAKE_TARGET) $^ $(LIBS_EXE)

# Include generated dependencies without failing on the first build.
-include $(DEPS) $(DICT_DEPS)
