noinst_LIBRARIES += lib/libAS_MSG.a
lib_libAS_MSG_a_SOURCES = %D%/AS_MSG_pmesg.C %D%/AS_MSG_pmesg1.C %D%/AS_MSG_pmesg2.C

libCA_a_SOURCES += $(lib_libAS_MSG_a_SOURCES)

dist_bin_SCRIPTS += %D%/convert-fasta-to-v2.pl %D%/tracedb-to-frg.pl	\
%D%/convert-v1-to-v2.pl

bin_PROGRAMS += bin/remove_fragment bin/extractmessages
bin_remove_fragment_SOURCES = %D%/remove_fragment.C
bin_extractmessages_SOURCES = %D%/ExtractMessages.C

noinst_HEADERS += %D%/AS_MSG_types.h %D%/AS_MSG_pmesg.h	\
%D%/AS_MSG_pmesg_internal.h
