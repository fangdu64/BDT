# **********************************************************************
#
# Bamtools
#
# **********************************************************************
bdt_topdir		= ../..
include $(bdt_topdir)/config/Make.rules
top_srcdir		= ../Ice/Ice-3.5.1/cpp
project_builddir= .
project_srcdir	= .

iBSLibDir	= $(BDT_Install)/bdt/lib
LIBFILENAME	= $(call mklibfilename,Bamtools,2.3.0)
SONAME		= $(call mksoname,Bamtools,23)  
LIBNAME		= $(call mklibname,Bamtools)
TARGETS		= $(call mklibtargets,$(iBSLibDir)/$(LIBFILENAME),$(iBSLibDir)/$(SONAME),$(iBSLibDir)/$(LIBNAME))

OBJS_LOCAL	=

OBJS_RELATIVE=BamAlignment.o \
			BamMultiReader.o \
			BamReader.o \
			BamWriter.o \
			BamHeader_p.o \
			BamMultiReader_p.o \
			BamRandomAccessController_p.o \
			BamReader_p.o \
			BamWriter_p.o \
			BamIndexFactory_p.o \
			BamStandardIndex_p.o \
			BamToolsIndex_p.o \
			BamDeviceFactory_p.o \
			BamFile_p.o \
			BamFtp_p.o \
			BamHttp_p.o \
			BamPipe_p.o \
			BgzfStream_p.o \
			ByteArray_p.o \
			HostAddress_p.o \
			HostInfo_p.o \
			HttpHeader_p.o \
			ILocalIODevice_p.o \
			RollingBuffer_p.o \
			TcpSocketEngine_p.o \
			TcpSocketEngine_unix_p.o \
			TcpSocket_p.o \
			SamFormatParser_p.o \
			SamFormatPrinter_p.o \
			SamHeaderValidator_p.o \
			BamException_p.o \
			SamHeader.o \
			SamProgram.o \
			SamProgramChain.o \
			SamReadGroup.o \
			SamReadGroupDictionary.o \
			SamSequence.o \
			SamSequenceDictionary.o

SRCS_RELATIVE_SHORT = api/BamAlignment.cpp \
			api/BamMultiReader.cpp \
			api/BamReader.cpp \
			api/BamWriter.cpp \
			api/internal/bam/BamHeader_p.cpp \
			api/internal/bam/BamMultiReader_p.cpp \
			api/internal/bam/BamRandomAccessController_p.cpp \
			api/internal/bam/BamReader_p.cpp \
			api/internal/bam/BamWriter_p.cpp \
			api/internal/index/BamIndexFactory_p.cpp \
			api/internal/index/BamStandardIndex_p.cpp \
			api/internal/index/BamToolsIndex_p.cpp \
			api/internal/io/BamDeviceFactory_p.cpp \
			api/internal/io/BamFile_p.cpp \
			api/internal/io/BamFtp_p.cpp \
			api/internal/io/BamHttp_p.cpp \
			api/internal/io/BamPipe_p.cpp \
			api/internal/io/BgzfStream_p.cpp \
			api/internal/io/ByteArray_p.cpp \
			api/internal/io/HostAddress_p.cpp \
			api/internal/io/HostInfo_p.cpp \
			api/internal/io/HttpHeader_p.cpp \
			api/internal/io/ILocalIODevice_p.cpp \
			api/internal/io/RollingBuffer_p.cpp \
			api/internal/io/TcpSocketEngine_p.cpp \
			api/internal/io/TcpSocketEngine_unix_p.cpp \
			api/internal/io/TcpSocket_p.cpp \
			api/internal/sam/SamFormatParser_p.cpp \
			api/internal/sam/SamFormatPrinter_p.cpp \
			api/internal/sam/SamHeaderValidator_p.cpp \
			api/internal/utils/BamException_p.cpp \
			api/SamHeader.cpp \
			api/SamProgram.cpp \
			api/SamProgramChain.cpp \
			api/SamReadGroup.cpp \
			api/SamReadGroupDictionary.cpp \
			api/SamSequence.cpp \
			api/SamSequenceDictionary.cpp

OBJS		= $(OBJS_LOCAL) $(OBJS_RELATIVE)
SRCS_LOCAL = $(OBJS_LOCAL:.o=.cpp)
SRCS_RELATIVE =$(addprefix $(project_srcdir)/,$(SRCS_RELATIVE_SHORT))
SRCS		= $(SRCS_LOCAL) $(SRCS_RELATIVE)

include $(top_srcdir)/config/Make.rules

ifeq ($(UNAME),Darwin)
	STATICLIBS = no
else
	STATICLIBS = yes
endif

CPPFLAGS	:= -I. -I$(project_srcdir) -DBAMTOOLS_API_LIBRARY -D_FILE_OFFSET_BITS=64 $(CPPFLAGS)
CXXFLAGS	:=$(subst -Werror,,$(CXXFLAGS))
LINKWITH        := -lz

$(project_builddir)/%.o: $(project_srcdir)/api/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

$(project_builddir)/%.o: $(project_srcdir)/api/internal/bam/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

$(project_builddir)/%.o: $(project_srcdir)/api/internal/index/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

$(project_builddir)/%.o: $(project_srcdir)/api/internal/io/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

$(project_builddir)/%.o: $(project_srcdir)/api/internal/sam/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

$(project_builddir)/%.o: $(project_srcdir)/api/internal/utils/%.cpp
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
	if test ! -d $(iBSLibDir) ; \
	then \
		mkdir -p $(iBSLibDir) ; \
		chmod a+rx $(iBSLibDir) ; \
	fi
	rm -f $@
	$(call mkshlib,$@,$(SONAME),$(OBJS),$(LINKWITH))

$(iBSLibDir)/$(SONAME): $(iBSLibDir)/$(LIBFILENAME)
	rm -f $@
	ln -s $(LIBFILENAME) $@

$(iBSLibDir)/$(LIBNAME): $(iBSLibDir)/$(SONAME)
	rm -f $@
	ln -s $(SONAME) $@
endif

install:: all