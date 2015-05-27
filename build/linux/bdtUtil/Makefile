# **********************************************************************
#
# bdtUtil
#
# **********************************************************************
bdt_topdir		= ../../..
include $(bdt_topdir)/config/Make.rules
top_srcdir		= ../../../ThirdParty/Ice/Ice-3.5.1/cpp
project_builddir=.
project_srcdir 	= ../../../bdtUtil

iBS_slicedir   	= ../../../common/slice/bdt
Bamtools_includedir = ../../../ThirdParty/BamTools
bamtools_libdir = $(bdt_topdir)/ThirdParty/BamTools/lib
iBS_includedir 	= ../../../common/include

iBSLibDir	= $(BDT_Install)/bdt/lib
LIBFILENAME	= $(call mklibfilename,bdtUtil,1.0.0)
SONAME		= $(call mksoname,bdtUtil,10)
LIBNAME		= $(call mklibname,bdtUtil)
TARGETS		= $(call mklibtargets,$(iBSLibDir)/$(LIBFILENAME),$(iBSLibDir)/$(SONAME),$(iBSLibDir)/$(LIBNAME))

OBJS_LOCAL	= BasicSliceDefine.o \
			FCDCentralService.o \
			SampleService.o \
			JointAMRService.o 

OBJS_RELATIVE=GenomeHelper.o \
			RowAdjustHelper.o \
			SortHelper.o \
			BSEvidenceReader.o \
			AMDTaskHelper.o

OBJS		= $(OBJS_LOCAL) $(OBJS_RELATIVE)
SRCS_LOCAL = $(OBJS_LOCAL:.o=.cpp)
SRCS_RELATIVE_SHORT = $(OBJS_RELATIVE:.o=.cpp)
SRCS_RELATIVE =$(addprefix $(project_srcdir)/,$(SRCS_RELATIVE_SHORT))
SRCS		= $(SRCS_LOCAL) $(SRCS_RELATIVE)

SLICE_SRCS_RELATIVE	=BasicSliceDefine.ice \
					FCDCentralService.ice \
					SampleService.ice \
					JointAMRService.ice

SLICE_SRCS		= $(addprefix $(iBS_slicedir)/,$(SLICE_SRCS_RELATIVE))

include $(top_srcdir)/config/Make.rules

CPPFLAGS	:= -I. -I$(project_srcdir) -I$(iBS_includedir) -I$(Bamtools_includedir) $(CPPFLAGS)
CXXFLAGS	:=$(subst -Werror,,$(CXXFLAGS))

ifeq ($(UNAME),Darwin)
	STATICLIBS = no
	CXXFLAGS	:= $(CXXFLAGS) -L$(iBSLibDir) -L$(bamtools_libdir)
	LINKWITH	:= -lBamtools
else
	STATICLIBS = yes
endif

$(project_builddir)/%.h %.cpp: $(iBS_slicedir)/%.ice $(SLICE2CPP) $(SLICEPARSERLIB)
	rm -f $(project_builddir)/$(*F).h $(*F).cpp
	$(SLICE2CPP) $(SLICE2CPPFLAGS) $<

$(project_builddir)/%.o: $(project_srcdir)/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<


ifeq ($(STATICLIBS),yes)
$(iBSLibDir)/$(LIBNAME): $(OBJS)
	if test ! -d $(iBSLibDir) ; \
	then \
		mkdir -p $(iBSLibDir) ; \
		chmod a+rx $(iBSLibDir) ; \
	fi
	rm -f $@
	$(call mklib,$@,$(OBJS))
else
$(iBSLibDir)/$(LIBFILENAME): $(OBJS)
	rm -f $@
	$(call mkshlib,$@,$(SONAME),$(OBJS),$(LINKWITH))

$(iBSLibDir)/$(SONAME): $(iBSLibDir)/$(LIBFILENAME)
	rm -f $@
	ln -s $(LIBFILENAME) $@

$(iBSLibDir)/$(LIBNAME): $(iBSLibDir)/$(SONAME)
	rm -f $@
	ln -s $(SONAME) $@
endif

clean::
	rm -f *.h
	rm -f *.cpp

install:: all
