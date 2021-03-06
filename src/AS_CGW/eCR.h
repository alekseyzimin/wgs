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

#ifndef ECR_H
#define ECR_H

// static const char *rcsid_ECR_H = "$Id: eCR.h,v 1.11 2009/06/10 18:05:13 brianwalenz Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "AS_global.h"
#include "AS_PER_gkpStore.h"

#define CONTIG_BASES          1000

// Except as noted:
//    0 nothing, 1 warnings, 2 lots of stuff
//
//  The names end in LV for "level" to distinguish them from the
//  function that is usually a very similar name.  This would be
//  incredibly useful if someone wants to, say, merge two classes into
//  one.
//
typedef struct {
  int   eCRmainLV;
  FILE *eCRmainFP;

  int   examineGapLV;
  FILE *examineGapFP;

  int   diagnosticLV;
  FILE *diagnosticFP;
} debugflags_t;

extern debugflags_t            debug;

extern int                     totalContigsBaseChange;
extern gkFragment              fsread;
extern int                     iterNumber;

#endif
