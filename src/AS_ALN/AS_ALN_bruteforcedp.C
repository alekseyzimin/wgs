
/**************************************************************************
 * This file is part of Celera Assembler, a software program that
 * assembles whole-genome shotgun reads into contigs and scaffolds.
 * Copyright (C) 2007-2008, J. Craig Venter Institute
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

// static const char *rcsid = "$Id: AS_ALN_bruteforcedp.c,v 1.14 2009/10/26 13:20:26 brianwalenz Exp $";

#include "AS_global.h"
#include "AS_ALN_bruteforcedp.h"
#include "AS_UTL_reverseComplement.h"


#define MATCH            0
#define GAPA             1
#define GAPB             2
#define STOP             3

//  3, -2, -2 original
//  5, -4, -3 suggested
//  3, -6, -4 got around one problem, but made many more
//
#define MATCHSCORE       3
#define GAPSCORE        -6
#define MISMATCHSCORE   -4

#define SLOP             10

//  Scores in the dynamic programming matrix are unsigned ints,
//  currently 30 bits wide.
//
//  The smallest score is 0.
//  The zero score is 2^29 (512 million)
//  The largest score is 2^30-1 (1 billion).
//
//  Edges that we don't want to end up at are initialized to DP_NEGT
//  (2^28, 256 million) which will take an awful lot of alignment to
//  make it decent, or to underflow.
//
//  Edges that we want to end up at are set to DP_ZERO (2^29, 512
//  million).

#define DP_NEGT          (1 << 28)
#define DP_ZERO          (1 << 29)

//
//  ahang, bhang represent any sequence to EXCLUDE from the alignmet.  There is a little bit of slop
//  in this exclusion.
//
//  Setting both to the length of the sequence will try to find an alignment over all bases.  The
//  highest scoring alignment is returned, which is likely not an alignment that uses all bases --
//  the only requirement (assuming endToEnd is set) is that the alignment reaches the end of one
//  sequence.
//

void
alignLinker(char           *alignA,
            char           *alignB,
            const char     *stringA,
            char           *stringB,
            dpCell        (*M)[AS_READ_MAX_NORMAL_LEN + 1],
            alignLinker_s  *a,
            int             endToEnd,
            int             allowNs,
            int             ahang,
            int             bhang) {

  int   lenA = strlen(stringA);
  int   lenB = strlen(stringB);

  //  Definition of the box we want to do dynamic programming in.
  int   i, ibgn, iend;
  int   j, jbgn, jend;

  if ((lenA > AS_READ_MAX_NORMAL_LEN) || (lenB > AS_READ_MAX_NORMAL_LEN)) {
    a->alignLen = 0;
    return;
  }

  //  Set the edges.

#if 0
  //  Debug.  Clear everything.
  for (i=0; i<AS_READ_MAX_NORMAL_LEN; i++) {
    for (j=0; j<AS_READ_MAX_NORMAL_LEN; j++) {
      M[i][j].score  = DP_NEGT;
      M[i][j].action = STOP;
    }
  }
#endif

  if (!endToEnd) {
    //  Looking for best local alignment.  Hitting the start of a sequence is NOT deadly.

    ibgn = 1;
    iend = lenA;
    jbgn = 1;
    jend = lenB;

    for (i=ibgn-1, j=jbgn-1; i<=lenA; i++) {
      M[i][j].score  = DP_ZERO;
      M[i][j].action = STOP;
    }
    for (i=ibgn-1, j=jbgn-1; j<=lenB; j++) {
      M[i][j].score  = DP_ZERO;
      M[i][j].action = STOP;
    }

  } else if ((ahang == lenA) && (bhang == lenB)) {
    //  Looking for the best global-ish alignment.  Hitting the start of the I sequence is deadly,
    //  but J is encouraged.  (This will give us a nice dovetail overlap).

    ibgn = 1;
    iend = lenA;
    jbgn = 1;
    jend = lenB;

    for (i=ibgn-1, j=jbgn-1; i<=lenA; i++) {
      M[i][j].score  = DP_ZERO;
      M[i][j].action = STOP;
    }
    for (i=ibgn-1, j=jbgn-1; j<=lenB; j++) {
      M[i][j].score  = DP_NEGT;
      M[i][j].action = STOP;
    }

  } else {
    //  Looking for the best global-ish alignment.  Hitting the ends of sequences is deadly, unless
    //  they are where we expect to end.
    //
    //  Four cases here (+-ahang, +-bhang).  
    //
    //  Plus 1 to the bgn -- zero is for the border.
    //  Plus 0 to the end -- loops iterate till <=.

    ibgn = (ahang < 0) ? (1) : (ahang+1);
    iend = (bhang < 0) ? (lenA + bhang) : (lenA);
    jbgn = (ahang < 0) ? (-ahang+1) : (1);
    jend = (bhang < 0) ? (lenB) : (lenB - bhang);

    //  Catch an error

    if ((ibgn < 0) || (iend < 0) ||
        (jbgn < 0) || (jend < 0) ||
        (iend < ibgn) ||
        (jend < jbgn)) {
      fprintf(stderr, "WARNING:  bgn: %d,%d  end: %d,%d  lens: %d,%d  hangs: %d,%d\n",
              ibgn, jbgn, iend, jend, lenA, lenB, ahang, bhang);
      a->alignLen = 0;
      return;
    }

    //fprintf(stderr, "bgn: %d,%d  end: %d,%d  lens: %d,%d  hangs: %d,%d\n", ibgn, jbgn, iend, jend, lenA, lenB, ahang, bhang);
    //fprintf(stderr, "A: %s\n", stringA);
    //fprintf(stderr, "B: %s\n", stringB);

    //  To easily allow a little slip in the starting position, we
    //  shift the Border of Death a little to the origin.  Ideally, we
    //  would have instead carved out a little square near the origin,
    //  which would save us the cost of computing the extra cells
    //  included in the shift.
    //
    //  Note that one of ibgn or jbgn is 1 already, and so the
    //  following changes only one bgn point.

    ibgn = MAX(1, ibgn - SLOP);
    jbgn = MAX(1, jbgn - SLOP);

    iend = MIN(lenA, lenA + SLOP);
    jend = MIN(lenB, lenB + SLOP);

    //  Border of Death

#if 1
    //  This should be all that is needed.
    for (i=ibgn-1, j=jbgn-1; i<=iend+1; i++) {
      M[i][j].score  = DP_NEGT;
      M[i][j].action = STOP;
    }

    for (i=ibgn-1, j=jbgn-1; j<=jend+1; j++) {
      M[i][j].score  = DP_NEGT;
      M[i][j].action = STOP;
    }
#endif

#if 0
    //  Overkill, box in the D.P.
    for (i=0, j=jbgn-1; i<AS_READ_MAX_NORMAL_LEN; i++) {
      M[i][j].score  = DP_NEGT;
      M[i][j].action = STOP;
      M[i][0].score  = DP_NEGT;
      M[i][0].action = STOP;
    }

    for (i-ibgn-1, j=0; j<AS_READ_MAX_NORMAL_LEN; j++) {
      M[i][j].score  = DP_NEGT;
      M[i][j].action = STOP;
      M[0][j].score  = DP_NEGT;
      M[0][j].action = STOP;
    }
#endif

    //  Successful Alignment.  The alignment hits the start of either
    //  the A or B sequence.  We allow 2*SLOP change in the expected
    //  alignment start.  The slop "wraps" around the origin:
    //
    //   |
    //   |     x = the expected end of alignment
    //  a|     a = allowed end points
    //  a|
    //  a|x
    //  a|
    //   o-------
    //  a aa
    //
    //  Which means that, in this case, we will allow an alignment
    //  that was supposed to have a negative ahang to instead have a
    //  positive ahang.  Simpler case: ahang=0 would insist on an
    //  alignment going to the origin.  Instead, we allow an alignment
    //  with a small positive or negative ahang.
    //
    //  The 'wrap around' case is needed when the ahang value is
    //  small.  The extra 2 units of slop are needed to get around the
    //  origin (see those two gaps between the a's in the figure
    //  above?)

    assert((ibgn == 1) || (jbgn == 1));

    if (jbgn == 1) {
      int slop = 2 * SLOP + 1;

      i = ibgn + SLOP;
      j = jbgn - 1;

      for (; (i >= ibgn-1) && (slop > 0); i--, slop--) {
        M[i][j].score  = DP_ZERO;
        M[i][j].action = STOP;
      }

      if ((i == ibgn-1) && (slop >= 0))
        slop += 2;

      for (; (slop > 0); j++, slop--) {
        M[i][j].score  = DP_ZERO;
        M[i][j].action = STOP;
      }
    }

    if (ibgn == 1) {
      int slop = 2 * SLOP + 1;

      i = ibgn - 1;
      j = jbgn + SLOP;

      for (; (j >= jbgn-1) && (slop > 0); j--, slop--) {
        M[i][j].score  = DP_ZERO;
        M[i][j].action = STOP;
      }

      if ((j == jbgn-1) && (slop >= 0))
        slop += 2;

      for (; (slop > 0); i++, slop--) {
        M[i][j].score  = DP_ZERO;
        M[i][j].action = STOP;
      }
    }

  }

  int   scoreMax  = 0;

  int   endI=0, curI=0;
  int   endJ=0, curJ=0;

  //fprintf(stderr, "%d,%d - %d,%d -- ahang,bhang %d,%d  alen,blen %d,%d\n",
  //        ibgn, jbgn, iend, jend, ahang, bhang, lenA, lenB);

  assert(ibgn >= 1);
  assert(jbgn >= 1);

  for (i=ibgn; i<=iend; i++){
    for (j=jbgn; j<=jend; j++){

      //  Pick the max of these

      int ul = M[i-1][j-1].score + ((stringA[i-1] == stringB[j-1]) ? MATCHSCORE : MISMATCHSCORE);
      int lf = M[i-1][j].score + GAPSCORE;
      int up = M[i][j-1].score + GAPSCORE;

      //  When finding the alignments, we do not really want to allow
      //  matches to Ns.  If we do, and one of the sequences has a
      //  gap, we'll optimally align to the gap instead of the real
      //  sequence.  However, in AS_ALN_forcns.c, we'll adjust our
      //  score to not count any alignment to an N as a mismatch.
      //
      if (allowNs)
        if ((stringA[i-1] == 'N') || (stringB[j-1] == 'N'))
          ul = M[i-1][j-1].score + MATCHSCORE;

      if (endToEnd) {
        //  Set score to zero; we will then ALWAYS pick an action
        //  below.
        //
        M[i][j].score  = 0;
        M[i][j].action = MATCH;
      } else {
        //  (i,j) is the beginning of a subsequence, our default
        //  behavior.  If we're looking for local alignments, make the
        //  alignment stop here.
        //
        M[i][j].score  = DP_ZERO;
        M[i][j].action = STOP;
      }

      if (M[i][j].score < ul) {
        M[i][j].score  = ul;
        M[i][j].action = MATCH;
      }

      if (M[i][j].score < lf) {
        M[i][j].score  = lf;
        M[i][j].action = GAPB;
      }

      if (M[i][j].score < up) {
        M[i][j].score  = up;
        M[i][j].action = GAPA;
      }

      if (scoreMax < M[i][j].score) {
        scoreMax  = M[i][j].score;
        endI = curI = i;
        endJ = curJ = j;
      }
    }
  }

  //  If we're not looking for local alignments, scan the end points
  //  for the best value.  If we are looking for local alignments,
  //  we've already found and remembered the best end point.

  if (endToEnd) {
    scoreMax    = 0;
    endI = curI = 0;
    endJ = curJ = 0;

    for (i=ibgn, j=jend; i<=iend; i++) {
      if (scoreMax < M[i][j].score) {
        scoreMax  = M[i][j].score;
        endI = curI = i;
        endJ = curJ = j;
        //fprintf(stderr, "IscoreMax = %d at %d,%d\n", scoreMax - DP_ZERO, i, j);
      }
    }

    for (j=jbgn, i=iend; j<=jend; j++) {
      if (scoreMax < M[i][j].score) {
        scoreMax  = M[i][j].score;
        endI = curI = i;
        endJ = curJ = j;
        //fprintf(stderr, "JscoreMax = %d at %d,%d\n", scoreMax - DP_ZERO, i, j);
      }
    }

    M[endI][endJ].score = 0;
  }

  int  alignLen  = 0;
  int  matches   = 0;
  int  terminate = 0;

  while (terminate == 0) {
    switch (M[curI][curJ].action) {
      case STOP:
        terminate = 1;
        break;
      case MATCH:
        alignA[alignLen] = stringA[curI-1];
        alignB[alignLen] = stringB[curJ-1];

        if (alignA[alignLen] == alignB[alignLen]) {
          alignA[alignLen] = tolower(alignA[alignLen]);
          alignB[alignLen] = tolower(alignB[alignLen]);
          matches++;
        } else {
          alignA[alignLen] = toupper(alignA[alignLen]);
          alignB[alignLen] = toupper(alignB[alignLen]);
        }

        curI--;
        curJ--;
        alignLen++;
        break;
      case GAPA:
        alignA[alignLen] = '-';
        alignB[alignLen] = stringB[curJ-1];
        curJ--;
        alignLen++;
        break;
      case GAPB:
        alignA[alignLen] = stringA[curI-1];
        alignB[alignLen] = '-';
        curI--;
        alignLen++;
        break;
    }
  }

  alignA[alignLen] = 0;
  alignB[alignLen] = 0;

  reverse(alignA, alignB, alignLen);

  a->matches  = matches;
  a->alignLen = alignLen;
  a->begI     = curI;
  a->begJ     = curJ;
  a->endI     = endI;
  a->endJ     = endJ;
  a->lenA     = lenA;
  a->lenB     = lenB;
}
