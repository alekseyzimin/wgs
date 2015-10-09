noinst_LIBRARIES += lib/libAS_OVL.a
lib_libAS_OVL_a_SOURCES = %D%/SharedOVL.C %D%/AS_OVL_delcher.C

libCA_a_SOURCES += $(lib_libAS_OVL_a_SOURCES)

bin_PROGRAMS += bin/overlap bin/overlap_ca bin/olap-from-seeds		\
                bin/correct-frags bin/show-corrects bin/correct-olaps	\
                bin/cat-corrects bin/cat-erates

bin_show_corrects_SOURCES = %D%/ShowCorrectsOVL.C %D%/AS_OVL_delcher.C
bin_cat_corrects_SOURCES = %D%/CatCorrectsOVL.C  %D%/AS_OVL_delcher.C
bin_cat_erates_SOURCES = %D%/CatEratesOVL.C    %D%/AS_OVL_delcher.C
bin_correct_frags_SOURCES = %D%/FragCorrectOVL.C  %D%/AS_OVL_delcher.C
bin_correct_olaps_SOURCES = %D%/CorrectOlapsOVL.C %D%/AS_OVL_delcher.C
bin_olap_from_seeds_SOURCES = %D%/OlapFromSeedsOVL.C %D%/AS_OVL_delcher.C

bin_overlap_SOURCES = %D%/AS_OVL_overlap.C %D%/AS_OVL_driver.C %D%/AS_OVL_delcher.C
bin_overlap_CPPFLAGS = $(AM_CPPFLAGS) -std=c++0x $(JELLYFISH2_0_CFLAGS)
bin_overlap_LDFLAGS = $(AM_LDFLAGS) $(JELLYFISH2_0_LIBS)

bin_overlap_ca_SOURCES = %D%/AS_OVL_overlap_ca.C %D%/AS_OVL_driver_ca.C %D%/AS_OVL_delcher.C
bin_overlap_ca_CPPFLAGS = $(AM_CPPFLAGS) -std=c++0x $(JELLYFISH2_0_CFLAGS)
bin_overlap_ca_LDFLAGS = $(AM_LDFLAGS) $(JELLYFISH2_0_LIBS)


noinst_HEADERS += %D%/AS_OVL_delcher.h %D%/AS_OVL_driver_common.h	\
%D%/AS_OVL_olapstats.h %D%/AS_OVL_overlap_common.h			\
%D%/AS_OVL_overlap.h %D%/FragCorrectOVL.h %D%/OlapFromSeedsOVL.h	\
%D%/SharedOVL.h
