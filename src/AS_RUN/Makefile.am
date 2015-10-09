RUNCAPREF = %D%/runCA-OBT/runCA.pl.head %D%/runCA-OBT/runCAutil/util.pl
RUNCASUF = %D%/runCA-OBT/runCA.pl %D%/runCA-OBT/runCAutil/scheduler.pm

RUNCAOBTUTIL = %D%/runCA-OBT/runCAutil/checkOverlap.pl	\
%D%/runCA-OBT/runCAutil/createConsensusJobs.pl		\
%D%/runCA-OBT/runCAutil/merOverlapper.pl		\
%D%/runCA-OBT/runCAutil/merTrim.pl			\
%D%/runCA-OBT/runCAutil/createOverlapJobs.pl		\
%D%/runCA-OBT/runCAutil/createOverlapStore.pl		\
%D%/runCA-OBT/runCAutil/createPostUnitiggerConsensus.pl	\
%D%/runCA-OBT/runCAutil/meryl.pl			\
%D%/runCA-OBT/runCAutil/overlapCorrection.pl		\
%D%/runCA-OBT/runCAutil/overlapTrim.pl			\
%D%/runCA-OBT/runCAutil/preoverlap.pl			\
%D%/runCA-OBT/runCAutil/shred.pl			\
%D%/runCA-OBT/runCAutil/scaffolder.pl			\
%D%/runCA-OBT/runCAutil/terminate.pl			\
%D%/runCA-OBT/runCAutil/cleaner.pl			\
%D%/runCA-OBT/runCAutil/unitigger.pl			\
%D%/runCA-OBT/runCAutil/umdOverlapper.pl		\
%D%/runCA-OBT/runCAutil/vectorTrim.pl			\
%D%/runCA-OBT/runCAutil/toggler.pl

bin/runCA: $(RUNCAPREF) $(RUNCAOBTUTIL) $(RUNCASUF)
	$(AM_V_GEN)cat $^ > $@

%C%_scripts = bin/runCA %D%/testing/assemblyCompare		\
               %D%/testing/qcCompare %D%/testing/tampaCompare	\
               %D%/testing/mummerCompare
dist_bin_SCRIPTS += $(%C%_scripts)
EXTRA_DIST += $(RUNCAPREF) $(RUNCAOBTUTIL) $(RUNCASUF)
#CLEANFILES += $(%C%_scripts)

dist_bin_SCRIPTS += %D%/asmQC/caqc_help.ini %D%/asmQC/caqc.pl		\
%D%/asmQC/caqc-diff.pl %D%/asmQC/ca2ace.pl %D%/asmQC/asmToAGP.pl	\
%D%/dataQC/glen_est_truncadjusted.pl

tigrbindir = $(bindir)/TIGR
dist_tigrbin_SCRIPTS = %D%/asmQC/TIGR/AsmLib.pm	\
%D%/asmQC/TIGR/Foundation.pm

bin_PROGRAMS += bin/fragmentDepth bin/markUniqueUnique
bin_fragmentDepth_SOURCES = %D%/fragmentDepth.C
bin_markUniqueUnique_SOURCES = %D%/markUniqueUnique.C
