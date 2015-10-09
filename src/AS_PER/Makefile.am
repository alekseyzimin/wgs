noinst_LIBRARIES += lib/libAS_PER.a
lib_libAS_PER_a_SOURCES = %D%/AS_PER_genericStore.C			\
%D%/AS_PER_gkLibrary.C %D%/AS_PER_gkStore.C %D%/AS_PER_gkStore_IID.C	\
%D%/AS_PER_gkStore_PLC.C %D%/AS_PER_gkStore_UID.C			\
%D%/AS_PER_gkStore_clearRange.C %D%/AS_PER_gkStore_fragments.C		\
%D%/AS_PER_gkStore_load.C %D%/AS_PER_gkStore_partition.C		\
%D%/AS_PER_gkStream.C %D%/AS_PER_encodeSequenceQuality.C		\
%D%/AS_PER_asmStore.C

libCA_a_SOURCES += $(lib_libAS_PER_a_SOURCES)

noinst_HEADERS += %D%/AS_PER_encodeSequenceQuality.h		\
%D%/AS_PER_gkpStore.h %D%/gkStore.H %D%/gkClearRange.H		\
%D%/AS_PER_genericStore.h %D%/gkStream.H %D%/gkPlacement.H	\
%D%/gkFragment.H %D%/gkLibrary.H %D%/AS_PER_asmStore.h
