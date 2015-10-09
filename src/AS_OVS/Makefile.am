noinst_LIBRARIES += lib/libAS_OVS.a
lib_libAS_OVS_a_SOURCES = %D%/AS_OVS_overlap.C		\
%D%/AS_OVS_overlapFile.C %D%/AS_OVS_overlapStore.C	\
%D%/AS_OVS_overlapStats.C

libCA_a_SOURCES += $(lib_libAS_OVS_a_SOURCES)

bin_PROGRAMS += bin/overlapStore bin/overlapStats bin/convertOverlap	\
bin/filterOverlap

bin_overlapStore_SOURCES = %D%/overlapStore.C		\
%D%/overlapStore_build.C %D%/overlapStore_merge.C	\
%D%/overlapStore_dump.C %D%/overlapStore_erates.C
bin_overlapStats_SOURCES = %D%/overlapStats.C
bin_convertOverlap_SOURCES = %D%/convertOverlap.C
bin_filterOverlap_SOURCES = %D%/filterOverlap.C

noinst_HEADERS += %D%/AS_OVS_overlapFile.h %D%/AS_OVS_overlap.h	\
%D%/AS_OVS_overlapStore.h %D%/AS_UTL_histogram.h		\
%D%/overlapStatsBoringStuff.h %D%/overlapStore.h
