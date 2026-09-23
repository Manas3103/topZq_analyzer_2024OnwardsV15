rootlibs:=$(shell root-config --libs)
rootflags:=$(shell root-config --cflags)

CORRECTION_INCDIR:=$(shell correction config --incdir)
CORRECTION_LIBDIR:=$(shell correction config --libdir)

OBJDIR=src
SRCDIR=src

SOFLAGS       = -shared
LD = g++ -m64 -g -Wall

CXXFLAGS = -O0 -g -Wall -fmessage-length=0 $(rootflags) -fPIC -I$(SRCDIR) -I$(CORRECTION_INCDIR) -I.

# ============================================================
# Source files
# ============================================================

SRCS := $(wildcard $(SRCDIR)/*.cpp)

# Exclude files containing main()
COMMON_SRCS := $(filter-out \
    $(SRCDIR)/nanoaodrdataframe.cpp \
    $(SRCDIR)/fakefactorframe.cpp, \
    $(SRCS))

HEADERS = $(wildcard $(SRCDIR)/*.h)

# Objects used by the common library
OBJS := $(patsubst %.cpp,%.o,$(COMMON_SRCS)) $(SRCDIR)/rootdict.o


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
	      $(SRCDIR)/nanoaodrdataframe.o \
	      $(SRCDIR)/fakefactorframe.o \
	      $(TARGET) \
	      $(FAKE_TARGET) \
	      libnanoadrdframe.so \
	      $(SRCDIR)/rootdict.C \
	      rootdict_rdict.pcm
	rm -rf .nfs*


# ============================================================
# ROOT dictionary
# ============================================================

$(SRCDIR)/rootdict.C: $(SRCDIR)/NanoAODAnalyzerrdframe.h \
                       $(SRCDIR)/BaseAnalyser.h \
					   $(SRCDIR)/FakeFactorAnalyser.h \
                       $(SRCDIR)/Linkdef.h

	rm -f $@
	rootcling -I$(CORRECTION_INCDIR) -I$(SRCDIR) $@ $^

	rm -f rootdict_rdict.pcm
	ln -s $(SRCDIR)/rootdict_rdict.pcm .


# ============================================================
# Temporary test dictionary
# ============================================================

$(SRCDIR)/rootdicttmp.C: $(SRCDIR)/testing.h \
                         $(SRCDIR)/test_Linkdef.h

	rm -f $@
	rootcling -I$(CORRECTION_INCDIR) -I$(SRCDIR) $@ $^


libtest.so: $(SRCDIR)/testing.o $(SRCDIR)/rootdicttmp.o
	$(LD) $(SOFLAGS) $(LIBS) -o $@ $^


$(SRCDIR)/rootdicttmp.o: $(SRCDIR)/rootdicttmp.C
	$(CXX) -c -o $@ $(CXXFLAGS) $<


# ============================================================
# Common shared library
# ============================================================

libnanoadrdframe.so: $(OBJS)
	$(LD) $(SOFLAGS) $(LIBS) -o $@ $^


# ============================================================
# RoccoR
# ============================================================

$(SRCDIR)/RoccoR.o: $(SRCDIR)/RoccoR.cpp $(SRCDIR)/RoccoR.h
	g++ -c -o $@ $(CXXFLAGS) $<


# ============================================================
# ROOT dictionary object
# ============================================================

$(SRCDIR)/rootdict.o: $(SRCDIR)/rootdict.C
	$(CXX) -c -o $@ $(CXXFLAGS) $<


# ============================================================
# Compile all common .cpp files
# ============================================================

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) $<


# ============================================================
# Main executable
# ============================================================

$(TARGET): $(OBJS) $(SRCDIR)/nanoaodrdataframe.o
	$(CXX) -o $(TARGET) $^ $(LIBS_EXE)


# ============================================================
# Fake-factor executable
# ============================================================

$(FAKE_TARGET): $(OBJS) $(SRCDIR)/fakefactorframe.o
	$(CXX) -o $(FAKE_TARGET) $^ $(LIBS_EXE)
