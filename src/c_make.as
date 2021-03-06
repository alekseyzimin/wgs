#
###########################################################################
#
# This file is part of Celera Assembler, a software program that
# assembles whole-genome shotgun reads into contigs and scaffolds.
# Copyright (C) 1999-2004, Applera Corporation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received (LICENSE.txt) a copy of the GNU General Public
# License along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
###########################################################################

# Compiler & flags
#
# Please, please do not put defaults here.  They tend to break
# compiles on other architectures!
#
# The following sets compiler and linker flags based on the
# architecture detected.  To avoid name conflicts, we set:
#
# ARCH_CFLAGS    any flags for the compiler
# ARCH_LDFLAGS   flags for the linker
#
# ARCH_INC       any include directory paths
# ARCH_LIB       any library directory paths
#
# The final CFLAGS, CXXFLAGS and LDFLAGS are constructed from these.


# You can enable a debugging build, disabling all optimizations, by
# setting BUILDDEBUG to 1.
#
# You can enable a profiling build by setting BUILDPROFILE to 1.
#
# You can enable a line coverage build by setting BUILDCOVERAGE to 1.
# This implies a debug build with no optimization.
#
ifneq "$(origin BUILDDEBUG)" "environment"
BUILDDEBUG     = 0
endif
ifneq "$(origin BUILDPROFILE)" "environment"
BUILDPROFILE   = 0
endif
ifneq "$(origin BUILDCOVERAGE)" "environment"
BUILDCOVERAGE  = 0
endif


include $(LOCAL_WORK)/src/site_name.as
include $(LOCAL_WORK)/src/c_make.gen



ifeq ($(OSTYPE), Linux)
  ARCH_CFLAGS = -DANSI_C -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -fPIC

  ARCH_CFLAGS    += -pthread -Wall -Wno-write-strings -Wno-unused -Wno-char-subscripts
  ARCH_LDFLAGS   += -pthread -lm

  ifeq ($(BUILDDEBUG), 1)
    ARCH_CFLAGS  += -g 
#    ARCH_LDFLAGS +=
  else
    ARCH_CFLAGS  += -O2
    ARCH_LDFLAGS += -Wl,-O1
  endif

  ARCH_LIB      = /usr/X11R6/lib

  ifeq ($(MACHINETYPE), i686)
    ARCH_CFLAGS   += -march=i686 -DX86_GCC_LINUX
    ARCH_LDFLAGS  += -march=i686
  endif
  ifeq ($(MACHINETYPE), amd64)
    ARCH_CFLAGS   += -m64 -DX86_GCC_LINUX
    ARCH_LIB       = /usr/lib64 /usr/X11R6/lib64
  endif
  ifeq ($(MACHINETYPE), ia64)
    # Don't set X86_GCC_LINUX because IEEE floating point mode is not available.
  endif
endif



ifeq ($(OSTYPE), FreeBSD)
  ifeq ($(MACHINETYPE), i386)
    ARCH_LDFLAGS    += -pthread -lthr -lm
    ARCH_CFLAGS      = -pthread -Wall -Wimplicit -Wno-write-strings -Wno-unused -Wno-char-subscripts
    ARCH_CFLAGS      = -pthread       -Wimplicit -Wno-write-strings -Wno-unused -Wno-char-subscripts
  endif
  ifeq ($(MACHINETYPE), amd64)
    ARCH_LDFLAGS    += -pthread -lthr -lm
    ARCH_CFLAGS      = -pthread -Wall -Wimplicit -Wno-write-strings -Wno-unused -Wno-char-subscripts
    ARCH_CFLAGS      = -pthread       -Wimplicit -Wno-write-strings -Wno-unused -Wno-char-subscripts
  endif

  ifeq ($(BUILDCOVERAGE), 1)
    ARCH_CFLAGS   += -g -fprofile-arcs -ftest-coverage
    ARCH_LDFLAGS  += -lgcov
  else
    ifeq ($(BUILDDEBUG), 1)
      ARCH_CFLAGS   += -g
    else
      ifeq ($(BUILDPROFILE), 1)
        ARCH_CFLAGS   += -O -mtune=nocona -funroll-loops -fexpensive-optimizations -finline-functions
      else
        ARCH_CFLAGS   += -O -mtune=nocona -funroll-loops -fexpensive-optimizations -finline-functions -fomit-frame-pointer
      endif
    endif
  endif

  ARCH_INC         = /usr/local/include /usr/X11R6/include
  ARCH_LIB         = /usr/local/lib     /usr/X11R6/lib
endif



ifeq ($(OSTYPE), Darwin)
  CC               = gcc
  CXX              = g++
  ARCH_CFLAGS      = -D_THREAD_SAFE

  ifeq ($(MACHINETYPE), ppc)
    ifeq ($(BUILDDEBUG), 1)
      ARCH_CFLAGS   += -g
    else
      ARCH_CFLAGS   += -fast
    endif
  endif

# -mpim-altivec

  ifeq ($(MACHINETYPE), i386)
    ifeq ($(BUILDDEBUG), 1)
      ARCH_CFLAGS   += -fPIC -m64 -fmessage-length=0 -D_THREAD_SAFE -Wall -Wno-char-subscripts -g
      ARCH_LDFLAGS  += -m64 -lm
    else
      ARCH_CFLAGS   += -fPIC -m64 -fmessage-length=0 -D_THREAD_SAFE -Wall -Wno-char-subscripts -fast
      ARCH_LDFLAGS  += -m64 -lm
    endif
  endif

  ARCH_LIB         = /usr/local/lib /usr/X11R6/lib
endif



# Use "gmake SHELL=/bin/bash".
#
ifeq ($(OSTYPE), SunOS)
  ifeq ($(MACHINETYPE), i86pc)
    ARCH_CFLAGS    = -DBYTE_ORDER=LITTLE_ENDIAN -DANSI_C -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -pthreads
    ARCH_LDFLAGS  += -lm
  endif

  ifeq ($(MACHINETYPE), sparc32)
    ARCH_CFLAGS    = -DANSI_C -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -pthreads
    ARCH_LDFLAGS  += -lm -lnsl -lsocket
  endif

  ifeq ($(MACHINETYPE), sparc64)
    ARCH_CFLAGS    = -m64 -DANSI_C -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -pthreads
    ARCH_LDFLAGS  += -m64 -lm -lnsl -lsocket
  endif

  ifeq ($(BUILDDEBUG), 1)
    ARCH_CFLAGS   += -g
  else
    ARCH_CFLAGS   += -O
  endif
endif




ifeq ($(BUILDPROFILE), 1)
  ARCH_CFLAGS  += -pg
  ARCH_LDFLAGS += -pg
endif

ifdef ALL_STATIC
  ARCH_CFLAGS += -static
  ARCH_CXXFLAGS += -static-libstdc++
  ARCH_LDFLAGS = -static -static-libstdc++ -lpthread
endif

# One can argue that CXXFLAGS should be separate.  For now, we only
# add to the flags.

CFLAGS          += $(ARCH_CFLAGS)
CXXFLAGS        += $(ARCH_CFLAGS) $(ARCH_CXXFLAGS)
LDFLAGS         += $(ARCH_LDFLAGS)

INC_IMPORT_DIRS += $(LOCAL_WORK)/src $(patsubst %, $(LOCAL_WORK)/src/%, $(strip $(SUBDIRS)))
INC_IMPORT_DIRS += $(ARCH_INC)

LIB_IMPORT_DIRS += $(LOCAL_LIB)
LIB_IMPORT_DIRS += $(ARCH_LIB)

OBJ_SEARCH_PATH  = $(LOCAL_OBJ)

ifeq ($(SITE_NAME), JCVI)
  LDFLAGS += -lcurl
endif

#  The order of compilation here is very carefully chosen to be the
#  same as the order used in running an assembly.  It is extremely
#  useful if you happen to be making changes to, say, the persistent
#  stores.  Break the continuation lines after AS_GKP and you'll build
#  just gatekeeper.
#
#  It's also more-or-less telling us link relationships.  CGB doesn't
#  use REZ or CNS, etc.  It gets hairy at the end; REZ, CNS and CGW are
#  all dependent on each other.  Everything after TER isn't needed for
#  an assembly.

#CFLAGS += -I/n8/wgs/src/AS_CNS -I/n8/wgs/src/AS_CGW -I/n8/wgs/src/AS_ALN -I/n8/wgs/src/AS_REZ -I/n8/wgs/src/AS_SDB

# Broken by BPW's string UID hack: AS_CVT, AS_MPA.  AS_CVT might work,
# but its only used by AS_MPA.

# Broken by the C++ switch: AS_VWR (won't link on Linux64)

SUBDIRS = AS_RUN \
          AS_UTL \
          AS_UID \
          AS_MSG \
          AS_PER \
          AS_GKP \
          AS_OBT \
          AS_MER \
          AS_OVL \
          AS_OVS \
          AS_ALN \
          AS_CGB \
          AS_BOG \
          AS_REZ \
          AS_CNS \
          AS_LIN \
          AS_CGW \
          AS_TER \
	  AS_ENV \
	  AS_ARD \
          AS_REF
