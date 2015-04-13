# **********************************************************************
#
# bigMat
#
# **********************************************************************
bdt_topdir		= ../../..
include $(bdt_topdir)/config/Make.rules
top_srcdir		= $(bdt_topdir)/ThirdParty/Ice/Ice-3.5.1/cpp
project_builddir=.
project_srcdir 	= $(bdt_topdir)/bigMat
iBS_slicedir   	= $(bdt_topdir)/common/slice/bdt
iBS_includedir 	= $(bdt_topdir)/common/include
bdtCore_dir 	= $(bdt_topdir)/bdtCore
bdtCore_builddir 	= ../bdtCore
iBS_libdir =$(BDT_Install)/bdt/lib
iBS_bindir =$(BDT_Install)/bdt/bin
netlib_lapack_libdir =$(bdt_topdir)/ThirdParty/LAPACK/lapack-3.5.0
bamtools_includedir = $(bdt_topdir)/ThirdParty/BamTools
bamtools_libdir = $(bdt_topdir)/ThirdParty/BamTools/lib
armadillo_includedir=$(bdt_topdir)/ThirdParty/Armadillo/armadillo-4.650.4/include

ifeq ($(BOOST_TOPDIR),)
   BOOST_TOPDIR	= $(bdt_topdir)/ThirdParty/boost/boost_1_55_0
endif
boost_includedir=$(BOOST_TOPDIR)

bigMat	= bigMat

TARGETS		= $(bigMat)

OBJS_LOCAL	= SampleService.o

OBJS_RELATIVE=BamToBinCountCreator.o \
			BigMat.o \
			SampleServiceImpl.o \
			SampleWorkItem.o

OBJS		= $(OBJS_LOCAL) $(OBJS_RELATIVE)
SRCS_LOCAL = $(OBJS_LOCAL:.o=.cpp)
SRCS_RELATIVE_SHORT = $(OBJS_RELATIVE:.o=.cpp)
SRCS_RELATIVE =$(addprefix $(project_srcdir)/,$(SRCS_RELATIVE_SHORT))
SRCS		= $(SRCS_LOCAL) $(SRCS_RELATIVE)

SLICE_SRCS_RELATIVE	=SampleService.ice

SLICE_SRCS		= $(addprefix $(iBS_slicedir)/,$(SLICE_SRCS_RELATIVE))

include $(top_srcdir)/config/Make.rules


CPPFLAGS	:= -I. -I$(project_srcdir) -I$(bdtCore_dir) -I$(bdtCore_builddir) -I$(iBS_includedir) -I$(armadillo_includedir) -I$(boost_includedir) \
 -I$(bamtools_includedir) -DARMA_USE_LAPACK -DARMA_USE_BLAS -DARMA_NO_DEBUG  $(CPPFLAGS)

CXXFLAGS	:=$(subst -Werror,,$(CXXFLAGS))

SLICE2CPPFLAGS	:= --ice -I. -I$(iBS_slicedir) $(SLICE2CPPFLAGS)

LDFLAGS	:= -Wl,-rpath,$(BDT_Install)/ThirdParty/db/lib64 $(LDFLAGS)

ifeq ($(BLAS_LAPACK_LIB),ATLAS)
	LDFLAGS 	:= -L$(iBS_libdir) -L$(bamtools_libdir) -L$(ATLAS_LIBDIR) $(LDFLAGS)
else
	LDFLAGS 	:= -L$(iBS_libdir) -L$(bamtools_libdir) -L$(netlib_lapack_libdir) $(LDFLAGS)
endif


$(project_builddir)/%.h %.cpp: $(iBS_slicedir)/%.ice $(SLICE2CPP) $(SLICEPARSERLIB)
	rm -f $(project_builddir)/$(*F).h $(*F).cpp
	$(SLICE2CPP) $(SLICE2CPPFLAGS) $<

$(project_builddir)/%.o: $(project_srcdir)/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<


$(bigMat): $(OBJS)
	
	if test ! -d $(iBS_bindir) ; \
	then \
		mkdir -p $(iBS_bindir) ; \
		chmod a+rx $(iBS_bindir) ; \
	fi
	echo "$(embedded_runpath_prefix), $(runpath_libdir)"
	rm -f $(iBS_bindir)/$@

	if [ "$(BLAS_LAPACK_LIB)" == "ATLAS" ] ; \
	then \
		echo "BLAS_LAPACK from ATLAS"; \
		$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) -lFreeze -lBamtools -lbdtCore -lz -latlaslapack -latlasblas $(LIBS); \
	else \
		echo "BLAS_LAPACK from Netlib"; \
		$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) -lFreeze -lBamtools -lbdtCore -lz -llapack -lrefblas -lgfortran $(LIBS); \
	fi

	mv $@ $(iBS_bindir)/$@

clean::
	rm -f *.h
	rm -f *.cpp

install:: all
