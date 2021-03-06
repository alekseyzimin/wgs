
/**************************************************************************
 * This file is part of Celera Assembler, a software program that
 * assembles whole-genome shotgun reads into contigs and scaffolds.
 * Copyright (C) 1999-2004, Applera Corporation. All rights reserved.
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

#ifndef AS_CNS_ARRAY_INCLUDE
#define AS_CNS_ARRAY_INCLUDE

// static const char *rcsid_AS_CNS_ARRAY_INCLUDE = "$Id: Array_CNS.h,v 1.13 2009/09/07 07:40:57 brianwalenz Exp $";

#include "AS_global.h"
#include "AS_PER_gkpStore.h"

int
IMP2Array(IntMultiPos *frags,
          int num_frags,
          int length,
          gkStore *frag_store,
          int *depth,
          char ***multia,
          int ***id_array,
          int ***ori_array,
          int show_cel_status,
          uint32 clrrng_flag);

#endif
