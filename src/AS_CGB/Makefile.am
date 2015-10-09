noinst_LIBRARIES += lib/libAS_CGB.a
lib_libAS_CGB_a_SOURCES = %D%/AS_CGB_histo.C

libCA_a_SOURCES += $(lib_libAS_CGB_a_SOURCES)

bin_PROGRAMS += bin/unitigger
AS_FGB_OBJS = %D%/AS_FGB_main.C %D%/AS_FGB_io.C
AS_OGB_OBJS = %D%/AS_CGB_histo.C %D%/AS_CGB_fga.C %D%/AS_CGB_fgb.C	\
%D%/AS_FGB_hanging_fragment.C %D%/AS_FGB_contained.C			\
%D%/AS_CGB_traversal.C %D%/AS_CGB_walk.C %D%/AS_CGB_edgemate.C
AS_CGB_OBJS = %D%/AS_CGB_main.C %D%/AS_CGB_cgb.C	\
%D%/AS_CGB_classify.C %D%/AS_CGB_chimeras.C
AS_CGA_OBJS = %D%/AS_CGB_cga.C
AS_BUB_OBJS = %D%/AS_CGB_Bubble.C %D%/AS_CGB_Bubble_Graph.C	\
%D%/AS_CGB_Bubble_VertexSet.C %D%/AS_CGB_Bubble_dfs.C		\
%D%/AS_CGB_Bubble_Popper.C %D%/AS_CGB_Bubble_PopperMethods.C
bin_unitigger_SOURCES = %D%/AS_CGB_unitigger.C $(AS_FGB_OBJS)		\
                        $(AS_OGB_OBJS) $(AS_CGB_OBJS) $(AS_CGA_OBJS)	\
                        $(AS_BUB_OBJS)

noinst_HEADERS += %D%/AS_CGB_all.h %D%/AS_CGB_Bubble_Graph.h	\
%D%/AS_CGB_Bubble.h %D%/AS_CGB_Bubble_Popper.h			\
%D%/AS_CGB_Bubble_PopperMethods.h %D%/AS_CGB_Bubble_VertexSet.h	\
%D%/AS_CGB_histo.h %D%/AS_CGB_methods.h				\
%D%/AS_CGB_unitigger_globals.h
