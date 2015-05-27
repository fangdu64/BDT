# **********************************************************************
#
# bdtCore
#
# **********************************************************************
bdt_topdir		= ../../..
include $(bdt_topdir)/config/Make.rules
top_srcdir		= $(bdt_topdir)/ThirdParty/Ice/Ice-3.5.1/cpp
project_builddir=.
project_srcdir 	= $(bdt_topdir)/bdtCore

iBS_slicedir   	= $(bdt_topdir)/common/slice/bdt
bamtools_includedir = $(bdt_topdir)/ThirdParty/BamTools
iBS_includedir 	= $(bdt_topdir)/common/include
armadillo_includedir=$(bdt_topdir)/ThirdParty/Armadillo/armadillo-4.650.4/include

ifeq ($(BOOST_TOPDIR),)
   BOOST_TOPDIR	= $(bdt_topdir)/ThirdParty/boost/boost_1_55_0
endif
boost_includedir=$(BOOST_TOPDIR)

iBSLibDir	= $(BDT_Install)/bdt/lib
LIBFILENAME	= $(call mklibfilename,bdtCore,1.0.0)
SONAME		= $(call mksoname,bdtCore,10)  
LIBNAME		= $(call mklibname,bdtCore)
TARGETS		= $(call mklibtargets,$(iBSLibDir)/$(LIBFILENAME),$(iBSLibDir)/$(SONAME),$(iBSLibDir)/$(LIBNAME))

OBJS_LOCAL	= ComputeService.o \
			FeatureDomainFreezeMap.o \
			FeatureObserverFreezeMap.o \
			FCDCentralService.o \
			BasicSliceDefine.o \
			FCDCentralFreeze.o \
			ObserverIndexFreezeMap.o \
			ObserverStatsFreezeMap.o

OBJS_RELATIVE=BigMatrixFacetImpl.o \
			BigMatrixFacetWorkItem.o \
			ComputeBuilder.o \
			ComputeServiceImpl.o \
			ComputeWorker.o \
			ComputeWorkerMgr.o \
			ComputeWorkItem.o \
			BitSetHelper.o \
			CommonHelper.o \
			FacetAdminImpl.o \
			FCDCFacetImpI.o \
			FCDCFacetWorkItem.o \
			FeatureDomainDB.o \
			FeatureObserverDB.o \
			ObserverIndexDB.o \
			FeatureValueRAM.o \
			FeatureValueStoreMgr.o \
			FeatureValueWorkItem.o \
			GlobalVars.o \
			ObserverStatsDB.o \
			ProxyCentralServiceImpl.o \
			StatisticsHelper.o \
			FCDCentralServiceImpl.o \
			FeatureValueWorker.o \
			FeatureValueWorkerMgr.o \
			GenomeBpBitset.o \
			RandomHelper.o

OBJS		= $(OBJS_LOCAL) $(OBJS_RELATIVE)
SRCS_LOCAL = $(OBJS_LOCAL:.o=.cpp)
SRCS_RELATIVE_SHORT = $(OBJS_RELATIVE:.o=.cpp)
SRCS_RELATIVE =$(addprefix $(project_srcdir)/,$(SRCS_RELATIVE_SHORT))
SRCS		= $(SRCS_LOCAL) $(SRCS_RELATIVE)

SLICE_SRCS_RELATIVE	=BasicSliceDefine.ice \
					FCDCentralService.ice \
					ComputeService.ice \
					FCDCentralFreeze.ice

SLICE_SRCS		= $(addprefix $(iBS_slicedir)/,$(SLICE_SRCS_RELATIVE))

include $(top_srcdir)/config/Make.rules


CPPFLAGS	:= -I. -I$(project_srcdir) -I$(iBS_includedir) -I$(armadillo_includedir) -I$(boost_includedir) \
 -I$(bamtools_includedir) -DARMA_USE_LAPACK -DARMA_USE_BLAS -DARMA_NO_DEBUG  $(CPPFLAGS)

CXXFLAGS	:=$(subst -Werror,,$(CXXFLAGS))

ifeq ($(UNAME),Darwin)
	STATICLIBS = no
	LINKWITH	:= -L$(iBSLibDir) -L$(bamtools_libdir) -lFreeze -lbdtUtil -lBamtools $(LIBS)
else
	STATICLIBS = yes
endif

SLICE2CPPFLAGS	:= --ice -I. -I$(iBS_slicedir) $(SLICE2CPPFLAGS)

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

# the slice2freeze rules are structured like this to avoid issues with
# parallel make
FeatureDomainFreezeMap.h: FeatureDomainFreezeMap.cpp
FeatureDomainFreezeMap.cpp: $(iBS_slicedir)/FCDCentralFreeze.ice $(SLICE2FREEZE) $(SLICEPARSERLIB)
	rm -f FeatureDomainFreezeMap.h FeatureDomainFreezeMap.cpp
	$(SLICE2FREEZE) --ice -I. -I$(slicedir) -I$(iBS_slicedir) \
		--dict iBS::CFeatureDomainFreezeMap,int,iBS::FeatureDomainInfo FeatureDomainFreezeMap \
		$(iBS_slicedir)/FCDCentralFreeze.ice

FeatureObserverFreezeMap.h: FeatureObserverFreezeMap.cpp
FeatureObserverFreezeMap.cpp: $(iBS_slicedir)/FCDCentralFreeze.ice $(SLICE2FREEZE) $(SLICEPARSERLIB)
	rm -f FeatureObserverFreezeMap.h FeatureObserverFreezeMap.cpp
	$(SLICE2FREEZE) --ice -I. -I$(slicedir) -I$(iBS_slicedir) \
		--dict iBS::CFeatureObserverFreezeMap,int,iBS::FeatureObserverInfo FeatureObserverFreezeMap \
		$(iBS_slicedir)/FCDCentralFreeze.ice

ObserverStatsFreezeMap.h: ObserverStatsFreezeMap.cpp
ObserverStatsFreezeMap.cpp: $(iBS_slicedir)/FCDCentralFreeze.ice $(SLICE2FREEZE) $(SLICEPARSERLIB)
	rm -f ObserverStatsFreezeMap.h ObserverStatsFreezeMap.cpp
	$(SLICE2FREEZE) --ice -I. -I$(slicedir) -I$(iBS_slicedir) \
		--dict iBS::CObserverStatsFreezeMap,int,iBS::ObserverStatsInfo ObserverStatsFreezeMap \
		$(iBS_slicedir)/FCDCentralFreeze.ice

ObserverIndexFreezeMap.h: ObserverIndexFreezeMap.cpp
ObserverIndexFreezeMap.cpp: $(iBS_slicedir)/FCDCentralFreeze.ice $(SLICE2FREEZE) $(SLICEPARSERLIB)
	rm -f ObserverIndexFreezeMap.h ObserverIndexFreezeMap.cpp
	$(SLICE2FREEZE) --ice -I. -I$(slicedir) -I$(iBS_slicedir) \
		--dict iBS::CObserverIndexFreezeMap,int,iBS::ObserverIndexInfo ObserverIndexFreezeMap \
		$(iBS_slicedir)/FCDCentralFreeze.ice

clean::
	rm -f *.h
	rm -f *.cpp

install:: all
