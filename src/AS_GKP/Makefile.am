bin_PROGRAMS += bin/gatekeeper bin/gatekeeperbench bin/sffToCA	\
                bin/fastqToCA bin/fastqSample

bin_gatekeeper_SOURCES = %D%/AS_GKP_main.C				\
%D%/AS_GKP_buildPartition.C %D%/AS_GKP_checkLibrary.C			\
%D%/AS_GKP_checkFrag.C %D%/AS_GKP_checkLink.C %D%/AS_GKP_checkPlace.C	\
%D%/AS_GKP_dump.C %D%/AS_GKP_edit.C %D%/AS_GKP_errors.C			\
%D%/AS_GKP_illumina.C
bin_gatekeeperbench_SOURCES = %D%/AS_GKP_bench.C
bin_sffToCA_SOURCES = %D%/sffToCA.C
bin_fastqToCA_SOURCES = %D%/fastqToCA.C
bin_fastqSample_SOURCES = %D%/fastqSample.C

noinst_HEADERS += %D%/AS_GKP_include.h
