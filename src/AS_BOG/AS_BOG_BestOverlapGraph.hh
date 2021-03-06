
/**************************************************************************
 * This file is part of Celera Assembler, a software program that
 * assembles whole-genome shotgun reads into contigs and scaffolds.
 * Copyright (C) 1999-2004, The Venter Institute. All rights reserved.
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

#ifndef INCLUDE_AS_BOG_BESTOVERLAPGRAPH
#define INCLUDE_AS_BOG_BESTOVERLAPGRAPH

// static const char *rcsid_INCLUDE_AS_BOG_BESTOVERLAPGRAPH = "$Id: AS_BOG_BestOverlapGraph.hh,v 1.59 2010/04/20 20:11:50 brianwalenz Exp $";

#include "AS_BOG_Datatypes.hh"

//  Checkpointing will save the best overlap graph after it is loaded from the overlap store.  It
//  seems to be working, but is missing a few features:
//
//  1) It has no version number of any other verification of the data (like, is this a checkpoint
//     file).
//
//  2) It doesn't remember the erate/elimit used when reading overlaps.  Changing these command line
//     parameters with a checkpoint file will result in the original overlaps being used.
//
//  3) It doesn't checkpoint _enough_ (maybe).  It is storing only best overlaps, not the state of
//     bog at the time.  It might be useful to save the gatekeeper information too.
//
#undef ENABLE_CHECKPOINTING

struct BestOverlapGraph {
  BestOverlapGraph(FragmentInfo *fi, OverlapStore *ovlStoreUniq, OverlapStore *ovlStoreRept, double erate, double elimit);
  ~BestOverlapGraph();

  //  Given a fragment UINT32 and which end, returns pointer to
  //  BestOverlap node.
  BestEdgeOverlap *getBestEdgeOverlap(uint32 frag_id, uint32 which_end) {
    if(which_end == FIVE_PRIME)    return(&_best_overlaps[frag_id].five_prime);
    if(which_end == THREE_PRIME)   return(&_best_overlaps[frag_id].three_prime);
    return(NULL);
  };

  // given a FragmentEnd sets it to the next FragmentEnd after following the
  // best edge
  FragmentEnd   followOverlap(FragmentEnd end) {
    if (end.fragId() == 0)
      return(FragmentEnd());

    BestEdgeOverlap *edge = getBestEdgeOverlap(end.fragId(), end.fragEnd());

    return(FragmentEnd(edge->frag_b_id, (edge->bend == FIVE_PRIME) ? THREE_PRIME : FIVE_PRIME));
  };

  bool isContained(const uint32 fragid) {
    return(_best_contains[fragid].isContained);
  };

  // Given a containee, returns pointer to BestContainment record
  BestContainment *getBestContainer(const uint32 fragid) {
    return((isContained(fragid)) ? &_best_contains[fragid] : NULL);
  };

  bool containHaveEdgeTo(uint32 contain, uint32 otherRead) {
    BestContainment  *c = &_best_contains[contain];
    bool              r = false;

    if ((c->olapsLen == 0) || (c->olaps == NULL))
      return(r);

    if (c->isContained == false)
      return(r);

    if (c->olapsLen < 16) {
      for (uint32 i=0; i<c->olapsLen; i++)
        if (c->olaps[i] == otherRead) {
          r = true;
          break;
        }
    } else {
      if (c->olapsSorted == false) {
        std::sort(c->olaps, c->olaps + c->olapsLen);
        c->olapsSorted = true;
      }
      r = std::binary_search(c->olaps, c->olaps + c->olapsLen, otherRead);
    }

    return(r);
  };

  // Graph building methods
  uint32  AEnd(const OVSoverlap& olap);
  uint32  BEnd(const OVSoverlap& olap);
  void    processOverlap(const OVSoverlap& olap);

  bool    isOverlapBadQuality(const OVSoverlap& olap) {

    //  The overlap is ALWAYS bad if the original error rate is above what we initially required
    //  overlaps to be at.  We shouldn't have even seen this overlap.  This is a bug in the
    //  overlapper.
    //
    if (olap.dat.ovl.orig_erate > consensusCutoff)
      return(true);

    //  The overlap is GOOD (false == not bad) if the corrected error rate is below the requested
    //  erate.
    //
    if (olap.dat.ovl.corr_erate <= mismatchCutoff) {
      if (logFile)
        fprintf(logFile, "OVERLAP GOOD:     %d %d %c  hangs " F_S64" " F_S64" err %.3f %.3f\n",
                olap.a_iid, olap.b_iid,
                olap.dat.ovl.flipped ? 'A' : 'N',
                olap.dat.ovl.a_hang,
                olap.dat.ovl.b_hang,
                AS_OVS_decodeQuality(olap.dat.ovl.orig_erate),
                AS_OVS_decodeQuality(olap.dat.ovl.corr_erate));
      return(false);
    }

    //  If we didn't allow fixed-number-of-errors, the overlap is now bad.  Just a slight
    //  optimization.
    //
    if (mismatchLimit <= 0)
      return(true);

    //  There are a few cases where the orig_erate is _better_ than the corr_erate.  That is, the
    //  orig erate is 0% but we 'correct' it to something more than 0%.  Regardless, we probably
    //  want to be using the corrected erate here.

    double olen = olapLength(olap.a_iid, olap.b_iid, olap.dat.ovl.a_hang, olap.dat.ovl.b_hang);
    double nerr = olen * AS_OVS_decodeQuality(olap.dat.ovl.corr_erate);

    assert(nerr >= 0);

    if (nerr <= mismatchLimit) {
      if (logFile)
        fprintf(logFile, "OVERLAP SAVED:    %d %d %c  hangs " F_S64" " F_S64" err %.3f %.3f olen %f nerr %f\n",
                olap.a_iid, olap.b_iid,
                olap.dat.ovl.flipped ? 'A' : 'N',
                olap.dat.ovl.a_hang,
                olap.dat.ovl.b_hang,
                AS_OVS_decodeQuality(olap.dat.ovl.orig_erate),
                AS_OVS_decodeQuality(olap.dat.ovl.corr_erate),
                olen, nerr);
      return(false);
    }

    if (logFile)
      fprintf(logFile, "OVERLAP REJECTED: %d %d %c  hangs " F_S64" " F_S64" err %.3f %.3f olen %f nerr %f\n",
              olap.a_iid, olap.b_iid,
              olap.dat.ovl.flipped ? 'A' : 'N',
              olap.dat.ovl.a_hang,
              olap.dat.ovl.b_hang,
              AS_OVS_decodeQuality(olap.dat.ovl.orig_erate),
              AS_OVS_decodeQuality(olap.dat.ovl.corr_erate),
              olen, nerr);
    return(true);
  };

  uint64  scoreOverlap(const OVSoverlap& olap) {

    //  BPW's newer new score.  For the most part, we use the length of the overlap, but we also
    //  want to break ties with the higher quality overlap.
    //
    //  The high 20 bits are the length of the overlap.
    //  The next 12 are the corrected error rate.
    //  The last 12 are the original error rate.
    //
    //  (Well, 12 == AS_OVS_ERRBITS)

    if (isOverlapBadQuality(olap))
      return(0);

    uint64  leng = 0;
    uint64  corr = (AS_OVS_MAX_ERATE - olap.dat.ovl.corr_erate);
    uint64  orig = (AS_OVS_MAX_ERATE - olap.dat.ovl.orig_erate);

    //  Shift AFTER assigning to a 64-bit value to avoid overflows.
    corr <<= AS_OVS_ERRBITS;

    //  Containments - the length of the overlaps are all the same.  We return the quality.
    //
    if (((olap.dat.ovl.a_hang >= 0) && (olap.dat.ovl.b_hang <= 0)) ||
        ((olap.dat.ovl.a_hang <= 0) && (olap.dat.ovl.b_hang >= 0)))
      return(corr | orig);

    //  Dovetails - the length of the overlap is the score, but we bias towards lower error.
    //  (again, shift AFTER assigning to avoid overflows)
    //
    leng   = olapLength(olap.a_iid, olap.b_iid, olap.dat.ovl.a_hang, olap.dat.ovl.b_hang);
    leng <<= (2 * AS_OVS_ERRBITS);

    return(leng | corr | orig);
  };


  uint32  olapLength(uint32 a_iid, uint32 b_iid, int32 a_hang, int32 b_hang) {
    int32  ooff = 0;
    int32  alen = _fi->fragmentLength(a_iid);
    int32  blen = _fi->fragmentLength(b_iid);
    int32  aovl = 0;
    int32  bovl = 0;

    if (a_hang < 0) {
      //  b_hang < 0      ?     ----------  :     ----
      //                  ?  ----------     :  ----------
      //
      aovl = (b_hang < 0) ? (alen + b_hang) : (alen);
      bovl = (b_hang < 0) ? (blen + a_hang) : (blen + a_hang - b_hang);
    } else {
      //  b_hang < 0      ?  ----------              :  ----------
      //                  ?     ----                 :     ----------
      //
      aovl = (b_hang < 0) ? (alen - a_hang + b_hang) : (alen - a_hang);
      bovl = (b_hang < 0) ? (blen)                   : (blen - b_hang);
    }

    assert(aovl > 0);
    assert(bovl > 0);
    assert(aovl <= alen);
    assert(bovl <= blen);

    //  AVE does not work.      return((uint32)((aovl, bovl)/2));
    //  MAX does not work.      return((uint32)MAX(aovl, bovl));

    return(aovl);
  };


  uint32 fragmentLength(uint32 id) {
    return(_fi->fragmentLength(id));
  };

  bool checkForNextFrag(const OVSoverlap& olap);
  void scoreContainment(const OVSoverlap& olap);
  void scoreEdge(const OVSoverlap& olap);

#ifdef ENABLE_CHECKPOINTING
  void save(void) {
    assert(_best_overlaps_5p_score == NULL);
    assert(_best_overlaps_3p_score == NULL);
    assert(_best_contains_score    == NULL);

    errno = 0;
    FILE *bogFile = fopen("bog.ckp", "w");
    if (errno) {
      fprintf(stderr, "Failed to open 'bog.ckp' for writing: %s\n", strerror(errno));
      return;
    }

    AS_UTL_safeWrite(bogFile, _best_overlaps, "best overlaps", sizeof(BestFragmentOverlap), _fi->numFragments() + 1);
    AS_UTL_safeWrite(bogFile, _best_contains, "best contains", sizeof(BestContainment),     _fi->numFragments() + 1);

    for (uint32 i=0; i<_fi->numFragments() + 1; i++)
      if (_best_contains[i].olaps != NULL)
        AS_UTL_safeWrite(bogFile, _best_contains[i].olaps, "best contains olaps", sizeof(uint32), _best_contains[i].olapsLen);

    fclose(bogFile);
  };

  bool load(void) {
    errno = 0;
    FILE *bogFile = fopen("bog.ckp", "r");
    if (errno)
      return(false);

    assert(_best_overlaps != NULL);
    assert(_best_contains != NULL);

    AS_UTL_safeRead(bogFile, _best_overlaps, "best overlaps", sizeof(BestFragmentOverlap), _fi->numFragments() + 1);
    AS_UTL_safeRead(bogFile, _best_contains, "best contains", sizeof(BestContainment),     _fi->numFragments() + 1);

    for (uint32 i=0; i<_fi->numFragments() + 1; i++) {
      if (_best_contains[i].olapsLen > 0) {
        _best_contains[i].olaps = new uint32 [_best_contains[i].olapsLen];
        AS_UTL_safeRead(bogFile, _best_contains[i].olaps, "best contains olaps", sizeof(uint32), _best_contains[i].olapsLen);
      } else {
        assert(_best_contains[i].olaps == NULL);
      }
    }

    fclose(bogFile);

    return(true);
  };
#endif

private:
  BestFragmentOverlap *_best_overlaps;
  BestContainment     *_best_contains;
  FragmentInfo        *_fi;

  uint64              *_best_overlaps_5p_score;
  uint64              *_best_overlaps_3p_score;
  uint64              *_best_contains_score;

public:
  uint64 mismatchCutoff;
  uint64 consensusCutoff;
  double mismatchLimit;
}; //BestOverlapGraph



#endif //INCLUDE_AS_BOG_BESTOVERLAPGRAPH
