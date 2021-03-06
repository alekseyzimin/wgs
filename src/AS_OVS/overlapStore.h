
/**************************************************************************
 * This file is part of Celera Assembler, a software program that
 * assembles whole-genome shotgun reads into contigs and scaffolds.
 * Copyright (C) 2007, J. Craig Venter Institute. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received (LICENSE.txt) a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *************************************************************************/

#ifndef OVERLAPSTORE_H
#define OVERLAPSTORE_H

// static const char *rcsid_OVERLAPSTORE_H = "$Id: overlapStore.h,v 1.17 2010/04/16 21:08:10 brianwalenz Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "AS_global.h"

typedef  enum
  {NONE, ALL, INTERNAL}  Ovl_Skip_Type_t;

void
buildStore(char *storeName, char *gkpName, uint64 memoryLimit, uint32 nThreads, uint32 doFilterOBT, uint32 fileListLen, char **fileList, Ovl_Skip_Type_t ovlSkipOpt);

void
mergeStore(char *storeName, char *mergeName);

void
updateErates(char *storeName, char *eratesName);

void
dumpStore(char *storeName, uint32 dumpBinary, double dumpERate, uint32 dumpType, uint32 bgnIID, uint32 endIID, uint32 qryIID);

void
dumpPicture(char *ovlName, char *gkpName, uint32 clearRegion, double dumpERate, uint32 dumpType, uint32 qryIID);

int
OVSoverlap_sort(const void *a, const void *b);


#define OP_NONE           0
#define OP_BUILD          1
#define OP_MERGE          2
#define OP_DUMP           3
#define OP_DUMP_PICTURE   4
#define OP_UPDATE_ERATES  5

#define DUMP_5p         1
#define DUMP_3p         2
#define DUMP_CONTAINED  4
#define DUMP_CONTAINS   8

#endif  //  OVERLAPSTORE_H
