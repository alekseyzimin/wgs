
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

static char const *rcsid = "$Id: AS_ALN_forcns.c,v 1.25 2010/04/26 03:52:57 brianwalenz Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "AS_global.h"
#include "AS_ALN_aligners.h"
#include "AS_ALN_bruteforcedp.h"

#include "AS_UTL_reverseComplement.h"

#undef DEBUG_GENERAL
#undef DEBUG_SHOW_TRACE

#define AFFINE_QUALITY   /* overlap diff and length reported in affine terms */


// The following functions make copies of the a and b sequences, returning a
// pointer to the location of the first character of the sequence; Gene's
// local alignment code (like DP_Compare?) seems to assume that the character
// before the beginning of the sequence will be '\0' -- so we copy the
// sequence into a character array starting at the second position, and
// return the location of the second position.
//
// Need two versions of the function because of the use of static memory.

static char *safe_copy_Astring_with_preceding_null(char *in){
  static char* out=NULL;
  static int outsize=0;
  int length=strlen(in);
  if(outsize<length+2){
    if(outsize==0){
      outsize=length * 2 + 2;
      out=(char*)safe_malloc(outsize*sizeof(char));
    } else {
      outsize=length * 2 + 2;
      out=(char*)safe_realloc(out,outsize*sizeof(char));
    }
  }
  out[0]='\0';
  strcpy(out+1,in);
  return(out+1);
}

static char *safe_copy_Bstring_with_preceding_null(char *in){
  static char* out=NULL;
  static int outsize=0;
  int length=strlen(in);
  if(outsize<length+2){
    if(outsize==0){
      outsize=length * 2 + 2;
      out=(char*)safe_malloc(outsize*sizeof(char));
    } else {
      outsize=length * 2 + 2;
      out=(char*)safe_realloc(out,outsize*sizeof(char));
    }
  }
  out[0]='\0';
  strcpy(out+1,in);
  return(out+1);
}


Overlap *
Local_Overlap_AS_forCNS(char *a, char *b,
                        int beg, int end,
                        int ahangUNUSED, int bhangUNUSED,
                        int opposite,
                        double erate, double thresh, int minlen,
                        CompareOptions what) {

  InternalFragMesg  A, B;
  OverlapMesg  *O;
  int alen,blen,del,sub,ins,affdel,affins,blockdel,blockins;
  double errRate,errRateAffine;
  int AFFINEBLOCKSIZE=4;
  static Overlap o;
  int where=0;

  //  Ugh, hack to get around C++ not liking A = {0} above.
  memset(&A, 0, sizeof(InternalFragMesg));
  memset(&B, 0, sizeof(InternalFragMesg));

#ifdef DEBUG_GENERAL
  fprintf(stderr, "Local_Overlap_AS_forCNS()--  Begins\n");
#endif

  if (erate > AS_MAX_ERROR_RATE) {
    //fprintf(stderr, "Local_Overlap_AS_forCNS()--  erate=%f >= AS_MAX_ERROR_RATE=%f, reset to max\n", erate, (double)AS_MAX_ERROR_RATE);
    erate = AS_MAX_ERROR_RATE;
  }
  assert((0.0 <= erate) && (erate <= 4 * AS_MAX_ERROR_RATE));

  A.sequence   = safe_copy_Astring_with_preceding_null(a);
  A.quality    = NULL;
  A.iaccession = 1;
  A.eaccession = AS_UID_fromInteger(A.iaccession);

  B.sequence = safe_copy_Bstring_with_preceding_null(b);
  B.quality = NULL;
  B.iaccession = 2;
  B.eaccession = AS_UID_fromInteger(B.iaccession);

  O = Local_Overlap_AS(&A,&B,beg,end,
		       opposite,
		       erate,
		       thresh,
		       minlen,
		       AS_FIND_LOCAL_ALIGN,
		       &where);
  if(O==NULL){
    return(NULL);
  }

  Analyze_Affine_Overlap_AS(&A,&B,O,AS_ANALYZE_ALL,&alen,&blen,&del,&sub,&ins,
		    &affdel,&affins,&blockdel,&blockins,AFFINEBLOCKSIZE, NULL);

  errRate = (sub+ins+del)/(double)(alen+ins);

  errRateAffine = (sub+affins+affdel)/ (double)(alen-del+affins+affdel);

  o.begpos=O->ahg;
  o.endpos=O->bhg;
#ifdef AFFINE_QUALITY
#ifdef REVISED_QUALITY
  o.length=O->length;
  o.diffs=O->diffs;
#else
  o.length=(alen+blen+affins+affdel)/2;
  o.diffs=sub+affins+affdel;
#endif
#else
  o.length=(alen+blen+ins+del)/2;
  o.diffs=sub+ins+del;
#endif
  o.comp=opposite;

  o.trace = O->alignment_trace;

#ifdef DEBUG_GENERAL
  fprintf(stderr, "aifrag=%d bifrag=%d ahg=%d bhg=%d\n", O->aifrag, O->bifrag, O->ahg, O->bhg);
#endif

  if(O->aifrag==2){/*The OverlapMesg gives b first for nonnegative ahang*/
    int i=0;
    while(o.trace[i]!=0){
      o.trace[i++]*=-1;
    }
    o.begpos*=-1;
    o.endpos*=-1;
  }


  //  At this point, we may have gaps at the ends of the sequences
  //  (either before the first base or after the last).
  //
  //  These may occur, e.g., due to disagreements in optimal
  //  non-affine alignment (as determined by Boundary) and optimal
  //  affine alignment (as determined by OKNAffine).
  //
  //  These gaps give consensus hiccups, so we want to eliminate them,
  //  and change the hangs appropriately.

  { int i=0;
    int j=0;
    int changeahang=0;
    int changebhang=0;
    int fullLenA = strlen(a);
    int fullLenB = strlen(b);
    char c;
#ifdef DEBUG_SHOW_TRACE
    fprintf(stderr, "Local_Overlap_AS_forCNS Trace (lens %d %d):",fullLenA,fullLenB);
#endif
    while(o.trace[i]!=0){
      c='*';
      if(o.trace[i]<-fullLenA){
	changebhang++;
      } else if (o.trace[i]>fullLenB){
	changebhang--;
      } else if (o.trace[i]==-1){
	changeahang--;
      } else if (o.trace[i]==1){
	changeahang++;
      } else {
	c=' ';
	o.trace[j++]=o.trace[i];
      }
#ifdef DEBUG_SHOW_TRACE
      fprintf(stderr, " %c%d",c,o.trace[i]);
#endif
      i++;
    }
#ifdef DEBUG_SHOW_TRACE
    fprintf(stderr, "\n");
#endif
    o.trace[j]=0;
    o.begpos+=changeahang;
    o.endpos+=changebhang;
  }

#ifdef DEBUG_GENERAL
  fprintf(stderr, "Local_Overlap_AS_forCNS()--  Ends\n");
#endif

  return(&o);
}



Overlap *
Affine_Overlap_AS_forCNS(char *a, char *b,
                         int beg, int end,
                         int ahangUNUSED, int bhangUNUSED,
                         int opposite,
                         double erate, double thresh, int minlen,
                         CompareOptions what){
  InternalFragMesg  A, B;
  OverlapMesg  *O;
  int alen,blen,del,sub,ins,affdel,affins,blockdel,blockins;
  double errRate,errRateAffine;
  extern int AS_ALN_TEST_NUM_INDELS;
  int orig_TEST_NUM_INDELS;
  int AFFINEBLOCKSIZE=4;
  int where=0;
  static Overlap o;

#ifdef DEBUG_GENERAL
  fprintf(stderr, "Affine_Overlap_AS_forCNS()--  Begins\n");
#endif

  if (erate > AS_MAX_ERROR_RATE) {
    //fprintf(stderr, "Affine_Overlap_AS_forCNS()--  erate=%f >= AS_MAX_ERROR_RATE=%f, reset to max\n", erate, (double)AS_MAX_ERROR_RATE);
    erate = AS_MAX_ERROR_RATE;
  }
  assert((0.0 <= erate) && (erate <= AS_MAX_ERROR_RATE));

  orig_TEST_NUM_INDELS = AS_ALN_TEST_NUM_INDELS;
  AS_ALN_TEST_NUM_INDELS = 0;

  A.sequence = safe_copy_Astring_with_preceding_null(a);
  A.quality = NULL;
  A.iaccession = 1;
  A.eaccession = AS_UID_fromInteger(A.iaccession);

  B.sequence = safe_copy_Bstring_with_preceding_null(b);
  B.quality = NULL;
  B.iaccession = 2;
  B.eaccession = AS_UID_fromInteger(B.iaccession);

  O =Affine_Overlap_AS(&A,&B,beg,end,
		       opposite,
		       erate,
		       thresh,
		       minlen,
		       AS_FIND_AFFINE_ALIGN,
		       &where);
  if(O==NULL){
    AS_ALN_TEST_NUM_INDELS = orig_TEST_NUM_INDELS;
    return(NULL);
  }

  Analyze_Affine_Overlap_AS(&A,&B,O,AS_ANALYZE_ALL,&alen,&blen,&del,&sub,&ins,
		    &affdel,&affins,&blockdel,&blockins,AFFINEBLOCKSIZE, NULL);

  errRate = (sub+ins+del)/(double)(alen+ins);

  errRateAffine = (sub+affins+affdel)/ (double)(alen-del+affins+affdel);

  o.begpos=O->ahg;
  o.endpos=O->bhg;
#ifdef AFFINE_QUALITY
  o.length=(alen+blen+affins+affdel)/2;
  o.diffs=sub+affins+affdel;
#else
  o.length=(alen+blen+ins+del)/2;
  o.diffs=sub+ins+del;
#endif
  o.comp=opposite;

  o.trace = O->alignment_trace;

  if(O->aifrag==2){/*The OverlapMesg gives b first for nonnegative ahang*/
    int i=0;
    while(o.trace[i]!=0){
      o.trace[i++]*=-1;
    }
    o.begpos*=-1;
    o.endpos*=-1;
  }

  //  At this point, we may have gaps at the ends of the sequences
  //  (either before the first base or after the last).
  //
  //  These seem to occur due to disagreements in optimal non-affine
  //  alignment (as determined by Boundary) and optimal affine
  //  alignment (as determined by OKNAffine).
  //
  //  These gaps give consensus hiccups, so we want to eliminate them,
  //  and change the hangs appropriately.

  { int i=0;
    int j=0;
    int changeahang=0;
    int changebhang=0;
    int fullLenA = strlen(a);
    int fullLenB = strlen(b);
    while(o.trace[i]!=0){
      if(o.trace[i]<-fullLenA){
	changebhang++;
      } else if (o.trace[i]>fullLenB){
	changebhang--;
      } else if (o.trace[i]==-1){
	changeahang--;
      } else if (o.trace[i]==1){
	changeahang++;
      } else {
	o.trace[j++]=o.trace[i];
      }
      i++;
    }
    o.trace[j]=0;
    o.begpos+=changeahang;
    o.endpos+=changebhang;
  }

  AS_ALN_TEST_NUM_INDELS = orig_TEST_NUM_INDELS;

#ifdef DEBUG_GENERAL
  fprintf(stderr, "Affine_Overlap_AS_forCNS()--  Ends\n");
#endif

  return(&o);
}



Overlap *
Optimal_Overlap_AS_forCNS(char *a, char *b,
                          int begUNUSED, int endUNUSED,
                          int ahang, int bhang,
                          int opposite,
                          double erate, double thresh, int minlen,
                          CompareOptions what) {

  typedef struct {
    char     h_alignA[AS_READ_MAX_NORMAL_LEN + AS_READ_MAX_NORMAL_LEN + 2];
    char     h_alignB[AS_READ_MAX_NORMAL_LEN + AS_READ_MAX_NORMAL_LEN + 2];
    dpCell   h_matrix[AS_READ_MAX_NORMAL_LEN + 1][AS_READ_MAX_NORMAL_LEN + 1];
    int      h_trace[AS_READ_MAX_NORMAL_LEN + AS_READ_MAX_NORMAL_LEN + 2];
  } dpMatrix;

  static Overlap   o = {0};
  static dpMatrix *m = NULL;

  alignLinker_s   al;

  int             allowNs = 0;

  if (m == NULL)
    m = (dpMatrix *)safe_malloc(sizeof(dpMatrix));

#ifdef DEBUG_GENERAL
  fprintf(stderr, "Optimal_Overlap_AS_forCNS()--  Begins\n");
#endif

  if (erate > AS_MAX_ERROR_RATE) {
    //fprintf(stderr, "Optimal_Overlap_AS_forCNS()--  erate=%f >= AS_MAX_ERROR_RATE=%f, reset to max\n", erate, (double)AS_MAX_ERROR_RATE);
    erate = AS_MAX_ERROR_RATE;
  }
  assert((0.0 <= erate) && (erate <= AS_MAX_ERROR_RATE));

 alignLinkerAgain:
  if (opposite)
    reverseComplementSequence(b, strlen(b));

  alignLinker(m->h_alignA,
              m->h_alignB,
              a,
              b,
              m->h_matrix,
              &al,
              TRUE,
              allowNs,
              ahang, bhang);
   if (al.alignLen == 0) {
      return NULL;
   }

#if 0
  fprintf(stderr, "ALIGN %s\n", a);
  fprintf(stderr, "ALIGN %s\n", b);
  fprintf(stderr, "ALIGN %d %d-%d %d-%d opposite=%d\n", al.alignLen, al.begI, al.endI, al.begJ, al.endJ, opposite);
  fprintf(stderr, "ALIGN '%s'\n", m->h_alignA);
  fprintf(stderr, "ALIGN '%s'\n", m->h_alignB);
#endif

  if (opposite) {
    reverseComplementSequence(b, strlen(b));

    reverseComplementSequence(m->h_alignA, al.alignLen);
    reverseComplementSequence(m->h_alignB, al.alignLen);

    int x = al.begJ;
    al.begJ = al.lenB - al.endJ;
    al.endJ = al.lenB - x;
  }

  //  We don't expect partial overlaps here.  At least one fragment
  //  must have an alignment to the very start.
  //
  //  ECR depends on this return value; it is allowed to fail
  //  when building a new unitig multialign.  For example:
  //
  //  <-----------------------
  //        ------>
  //
  //  When ECR tries to extend the second fragment, it checks that
  //  the extended fragment overlaps the next contig.  It does not
  //  check that the extended bits agree with the first fragment,
  //  leaving that up to "does the unitig rebuild".
  //
  if ((al.begJ != 0) && (al.begI != 0)) {

    //  Allow Ns in the alignment, try one more time.
    if (++allowNs == 1)
      goto alignLinkerAgain;

    return(NULL);
  }

  o.begpos  = (al.begI           > 0) ? (al.begI)           : -(al.begJ);
  o.endpos  = (al.lenB - al.endJ > 0) ? (al.lenB - al.endJ) : -(al.lenA - al.endI);
  o.length  = al.alignLen;
  o.diffs   = 0;
  o.comp    = opposite;
  o.trace   = m->h_trace;

  {
    int x=0;

    int tp = 0;
    int ap = al.begI;
    int bp = al.begJ;

    for (x=0; x<al.alignLen; x++) {
      if (m->h_alignA[x] == '-') {
        m->h_trace[tp++] = -(ap + 1);
        ap--;
      }
      if (m->h_alignB[x] == '-') {
        m->h_trace[tp++] = bp + 1;
        bp--;
      }

      //  Count the differences.
      //
      if (m->h_alignA[x] != m->h_alignB[x])
        o.diffs++;

      //  But allow N's as matches.  If either letter is N, then the
      //  other letter is NOT N (if both letters were N, both would be
      //  lowercase n, representing a match).  This just subtracts out
      //  the diff we added in above.
      //
      if ((m->h_alignA[x] == 'N') || (m->h_alignB[x] == 'N'))
        o.diffs--;

      bp++;
      ap++;
    }

    m->h_trace[tp] = 0;

#ifdef DEBUG_SHOW_TRACE
    fprintf(stderr, "trace");
    for (x=0; x<tp; x++)
      fprintf(stderr, " %d", m->h_trace[x]);
    fprintf(stderr, "\n");
    fprintf(stderr, "A: %d-%d %d %s\n", al.begI, al.endI, al.lenA, m->h_alignA);
    fprintf(stderr, "B: %d-%d %d %s\n", al.begJ, al.endJ, al.lenB, m->h_alignB);
#endif
  }

#ifdef DEBUG_GENERAL
  fprintf(stderr, "ERATE:   diffs=%d / length=%d = %f\n", o.diffs, o.length, (double)o.diffs / o.length);
  fprintf(stderr, "Optimal_Overlap_AS_forCNS()--  Ends\n");
#endif

  if ((double)o.diffs / o.length <= erate)
    return(&o);
  else if (++allowNs == 1)
    //  Allow Ns in the alignment, try one more time.
    goto alignLinkerAgain;
  else
    return(NULL);
}
