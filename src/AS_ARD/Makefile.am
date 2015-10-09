AS_ARD_CMN_OBJS = %D%/Sybase.cc
AS_ARD_DB_OBJS = %D%/AS_ARD_database.cc %D%/DBTextOutput.cc	\
%D%/BCPOutput.cc %D%/SQLOutput.cc

bin_PROGRAMS += bin/loadAssembly bin/getAssembly
bin_loadAssembly_SOURCES = %D%/loadAssembly.cc $(AS_ARD_CMN_OBJS)	\
                           $(AS_ARD_DB_OBJS)
bin_getAssembly_SOURCES = %D%/getAssembly.cc $(AS_ARD_CMD_OBJS)

noinst_HEADERS += %D%/AS_ARD_database.hh %D%/BCPOutput.hh	\
%D%/DBTextOutput.hh %D%/IAssemblyDB.hh %D%/IDBConnection.hh	\
%D%/IDBOutput.hh %D%/SQLOutput.hh %D%/Sybase.hh
