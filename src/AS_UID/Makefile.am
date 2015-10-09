noinst_LIBRARIES += lib/libAS_UID.a
lib_libAS_UID_a_SOURCES = %D%/uidclient.C

bin_PROGRAMS += bin/uidclient bin/uidserver
bin_uidclient_SOURCES = %D%/uidclient.C
bin_uidserver_SOURCES = %D%/uidserver.C

if SITE_JCVI
  lib_libAS_UID_a_SOURCES += %D%/SYS_UIDclient_JTC.c
endif

if SITE_LOCAL
  lib_libAS_UID_a_SOURCES += %D%/SYS_UIDclient_LOCAL.C
endif

if SITE_BRI
  lib_libAS_UID_a_SOURCES += %D%/SYS_UIDclient_SERVER.c
endif

libCA_a_SOURCES += $(lib_libAS_UID_a_SOURCES)
noinst_HEADERS += %D%/SYS_UIDclient.h %D%/uidserver_common.h
