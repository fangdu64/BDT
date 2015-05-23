# **********************************************************************
#
# bdvd
#
# **********************************************************************
bdt_topdir		= ../../..
include $(bdt_topdir)/config/Make.rules
top_srcdir		= $(bdt_topdir)/ThirdParty/Ice/Ice-3.5.1/cpp
project_builddir=.
project_srcdir 	= $(bdt_topdir)/bdvd
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

bdvd	= bdvd

TARGETS		= $(bdvd)

OBJS_LOCAL	= BDVDFreeze.o \
			RUVFreezeMap.o

OBJS_RELATIVE=bdvd.o \
			bdvdFacetAdminImpl.o \
			bdvdGlobalVars.o \
			RUVBuilder.o \
			RUVDB.o \
			RUVFacetImpl.o \
			RUVFacetWorkItem.o \
			RUVgWorker.o \
			RUVgWorkerMgr.o \
			RUVOutputWorker.o \
			RUVOutputWorkerMgr.o \
			RUVRowANOVAWorker.o \
			RUVRowANOVAWorkerMgr.o \
			RUVsWorker.o \
			RUVsWorkerMgr.o \
			RUVsWorkItem.o \
			RUVVarDecmWorker.o \
			RUVVarDecmWorkerMgr.o

OBJS		= $(OBJS_LOCAL) $(OBJS_RELATIVE)
SRCS_LOCAL = $(OBJS_LOCAL:.o=.cpp)
SRCS_RELATIVE_SHORT = $(OBJS_RELATIVE:.o=.cpp)
SRCS_RELATIVE =$(addprefix $(project_srcdir)/,$(SRCS_RELATIVE_SHORT))
SRCS		= $(SRCS_LOCAL) $(SRCS_RELATIVE)

SLICE_SRCS_RELATIVE	=BDVDFreeze.ice

SLICE_SRCS		= $(addprefix $(iBS_slicedir)/,$(SLICE_SRCS_RELATIVE))

include $(top_srcdir)/config/Make.rules


CPPFLAGS	:= -I. -I$(project_srcdir) -I$(bdtCore_dir) -I$(bdtCore_builddir) -I$(iBS_includedir) -I$(armadillo_includedir) -I$(boost_includedir) \
 -I$(bamtools_includedir) -DARMA_USE_LAPACK -DARMA_USE_BLAS -DARMA_NO_DEBUG  $(CPPFLAGS)

CXXFLAGS	:=$(subst -Werror,,$(CXXFLAGS))

SLICE2CPPFLAGS	:= --ice -I. -I$(iBS_slicedir) $(SLICE2CPPFLAGS)

LDFLAGS	:= -Wl,-rpath,$(BDT_Install)/ThirdParty/db/lib64 $(LDFLAGS)

ifeq ($(UNAME),Darwin)
	LDFLAGS 	:= -L$(iBS_libdir) -L$(bamtools_libdir) -framework Accelerate $(LDFLAGS)
else
	LDFLAGS 	:= -L$(iBS_libdir) -L$(bamtools_libdir) -L$(netlib_lapack_libdir) $(LDFLAGS)
endif

$(project_builddir)/%.h %.cpp: $(iBS_slicedir)/%.ice $(SLICE2CPP) $(SLICEPARSERLIB)
	rm -f $(project_builddir)/$(*F).h $(*F).cpp
	$(SLICE2CPP) $(SLICE2CPPFLAGS) $<

$(project_builddir)/%.o: $(project_srcdir)/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

$(project_builddir)/%.o: $(project_srcdir)/%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $<

$(bdvd): $(OBJS)
	
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
		$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) -lFreeze -lbdtCore -lbdtUtil -lBamtools -lz -latlaslapack -latlasblas $(LIBS); \
	else \
		echo "BLAS_LAPACK from Netlib"; \
		$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) -lFreeze -lbdtCore -lbdtUtil -lBamtools -lz -llapack -lrefblas -lgfortran $(LIBS); \
	fi

	mv $@ $(iBS_bindir)/$@

RUVFreezeMap.h: RUVFreezeMap.cpp
RUVFreezeMap.cpp: $(iBS_slicedir)/BDVDFreeze.ice $(SLICE2FREEZE) $(SLICEPARSERLIB)
	rm -f RUVFreezeMap.h RUVFreezeMap.cpp
	$(SLICE2FREEZE) --ice -I. -I$(slicedir) -I$(iBS_slicedir) \
		--dict iBS::CRUVFreezeMap,int,iBS::RUVFacetInfo RUVFreezeMap \
		$(iBS_slicedir)/BDVDFreeze.ice

clean::
	rm -f *.h
	rm -f *.cpp

install:: all
