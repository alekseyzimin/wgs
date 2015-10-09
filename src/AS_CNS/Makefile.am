noinst_LIBRARIES += lib/libAS_CNS.a
lib_libAS_CNS_a_SOURCES = %D%/Array_CNS.C %D%/MultiAlign.C		\
%D%/MultiAlignPrint.C %D%/MultiAlignStore.C %D%/MultiAlignment_CNS.C	\
%D%/RefreshMANode.C %D%/AbacusRefine.C %D%/ApplyAlignment.C		\
%D%/BaseCall.C %D%/GetAlignmentTrace.C %D%/MergeMultiAligns.C		\
%D%/MergeRefine.C %D%/MultiAlignContig.C %D%/MultiAlignUnitig.C		\
%D%/PrintAlignment.C %D%/ReplaceEndUnitigInContig.C

libCA_a_SOURCES += $(lib_libAS_CNS_a_SOURCES)

# TODO: do we need to support SEQ_AN
bin_PROGRAMS += bin/utgcns bin/ctgcns bin/tigStore

bin_utgcns_SOURCES = %D%/utgcns.C
bin_ctgcns_SOURCES = %D%/ctgcns.C
bin_tigStore_SOURCES = %D%/tigStore.C

noinst_HEADERS += %D%/Array_CNS.h %D%/MultiAlign.h		\
%D%/MultiAlignment_CNS.h %D%/MultiAlignment_CNS_private.h	\
%D%/MultiAlignStore.h
