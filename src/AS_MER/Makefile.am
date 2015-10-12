bin_PROGRAMS += bin/meryl
bin_meryl_SOURCES = %D%/AS_MER_meryl.cc

# Don't compile what depends on kmer library
# bin_PROGRAMS += bin/meryl bin/mercy bin/overmerry bin/merTrim		\
#                 bin/merTrimApply bin/estimate-mer-threshold
# KMERLIBS = lib/libmerylguts.a lib/libkmer.a lib/libmeryl.a lib/libseq.a lib/libbio.a lib/libutil.a

# bin_meryl_SOURCES = %D%/meryl.C %D%/AS_MER_gkpStore_to_FastABase.C %D%/AS_MER_gkpStoreChain.C
# bin_meryl_LDADD = $(LDADD) $(KMERLIBS)

# bin_mercy_SOURCES = %D%/mercy.C
# bin_mercy_LDADD = $(LDADD) $(KMERLIBS)

# bin_overmerry_SOURCES = %D%/overmerry.C %D%/AS_MER_gkpStore_to_FastABase.C
# bin_overmerry_LDADD = $(LDADD) $(KMERLIBS)

# bin_merTrim_SOURCES = %D%/merTrim.C %D%/merTrimAdapter.C %D%/AS_MER_gkpStore_to_FastABase.C 
# bin_merTrim_LDADD = $(LDADD) $(KMERLIBS)

# bin_merTrimApply_SOURCES = %D%/merTrimApply.C 
# bin_merTrimApply_LDADD = $(LDADD) $(KMERLIBS)

# bin_estimate_mer_threshold_SOURCES = %D%/estimate-mer-threshold.C %D%/AS_MER_gkpStore_to_FastABase.C 
# bin_estimate_mer_threshold_LDADD = $(LDADD) $(KMERLIBS)

noinst_HEADERS += %D%/AS_MER_gkpStoreChain.H	\
%D%/AS_MER_gkpStore_to_FastABase.H
