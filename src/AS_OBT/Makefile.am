noinst_LIBRARIES += lib/libAS_OBT.a
lib_libAS_OBT_a_SOURCES = %D%/AS_OBT_acceptableOverlap.C

libCA_a_SOURCES += $(lib_libAS_OBT_a_SOURCES)

bin_PROGRAMS += bin/initialTrim bin/consolidate bin/merge-trimming	\
                bin/overlapMask bin/chimera bin/deduplicate		\
                bin/removeMateOverlap
bin_initialTrim_SOURCES = %D%/initialTrim.C %D%/trim.C
bin_consolidate_SOURCES = %D%/consolidate.C %D%/util++.C %D%/readOverlap.C
bin_merge_trimming_SOURCES = %D%/merge-trimming.C %D%/trim.C
bin_overlapMask_SOURCES = %D%/overlapMask.C %D%/util++.C %D%/readOverlap.C
bin_chimera_SOURCES = %D%/chimera.C %D%/util++.C %D%/readOverlap.C
bin_deduplicate_SOURCES = %D%/deduplicate.C %D%/util++.C %D%/readOverlap.C
bin_removeMateOverlap_SOURCES = %D%/removeMateOverlap.C %D%/util++.C %D%/readOverlap.C

noinst_HEADERS += %D%/AS_OBT_acceptableOverlap.h %D%/constants.H	\
%D%/readOverlap.H %D%/trim.H %D%/util++.H

