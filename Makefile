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
	      $(SRCDIR)/rootdict.C \
	      rootdict_rdict.pcm
	rm -rf .nfs*


# ============================================================
# ROOT dictionary
# ============================================================

DICT_HEADERS := $(SRCDIR)/NanoAODAnalyzerrdframe.h $(SRCDIR)/BaseAnalyser.h $(SRCDIR)/FakeFactorAnalyser.h $(SRCDIR)/Linkdef.h
DICT_DEPS := $(SRCDIR)/rootdict_headers.d
DICT_PCM := $(SRCDIR)/rootdict_rdict.pcm

# Track headers parsed for the dictionary, including transitive includes.
$(DICT_DEPS): $(DICT_HEADERS) Makefile
	$(CXX) $(CXXFLAGS) -D__CLING__ -MM -MP -x c++ -MT $(DICT_DEPS) -MT $(SRCDIR)/rootdict.C -MT $(DICT_PCM) $(DICT_HEADERS) > $@.tmp
	mv $@.tmp $@

# rootcling produces both outputs in one invocation (GNU Make >= 4.3).
$(SRCDIR)/rootdict.C $(DICT_PCM) &: $(DICT_HEADERS) $(DICT_DEPS)
	rm -f $(SRCDIR)/rootdict.C
	rootcling -I$(CORRECTION_INCDIR) -I$(SRCDIR) $(SRCDIR)/rootdict.C $(DICT_HEADERS)
	test -s $(SRCDIR)/rootdict.C
	test -s $(DICT_PCM)

# Recover the lookup link without recompiling objects or relinking binaries.
rootdict_rdict.pcm: | $(DICT_PCM)
	ln -sfn $(DICT_PCM) $@

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
$(OBJDIR)/rootdict.o: $(DICT_PCM) $(SRCDIR)/rootdict.C
	mkdir -p $(@D)
	$(CXX) -c -o $@ $(CXXFLAGS) $(DEPFLAGS) $(SRCDIR)/rootdict.C


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
