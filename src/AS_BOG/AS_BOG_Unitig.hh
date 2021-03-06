
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

#ifndef INCLUDE_AS_BOG_UNITIG
#define INCLUDE_AS_BOG_UNITIG

// static const char *rcsid_INCLUDE_AS_BOG_UNITIG = "$Id: AS_BOG_Unitig.hh,v 1.14 2010/03/16 13:06:31 brianwalenz Exp $";

#include "AS_BOG_Datatypes.hh"

//  Derived from IntMultiPos, but removes some of the data (48b in IntMultiPos, 32b in struct
//  DoveTailNode).  The minimum size (bit fields, assuming maximum limits, not using the contained
//  field) seems to be 24b, and is more effort than it is worth (just removing 'contained' would be
//  a chore).
//
#ifdef WITHIMP
typedef IntMultiPos                       DoveTailNode;
#else
struct DoveTailNode {
  int32           ident;
  int32           contained;
  int32           parent;     //  IID of the fragment we align to

  int32           ahang;      //  If parent defined, these are relative
  int32           bhang;      //  that fragment

  SeqInterval     position;

  int32           delta_length;
};
#endif

typedef std::vector<DoveTailNode>         DoveTailPath;


typedef DoveTailPath::iterator            DoveTailIter;
typedef DoveTailPath::const_iterator      DoveTailConstIter;

//  The ContainerMap stores a list of IDs contained in the index fragment:
//    cMap[index] = list of IDs contained in index
//
//  It is used during splitting to get the total number of fragments in a unitig, the number of
//  fragments before/after the split point.
//
typedef std::vector<uint32>                 ContaineeList;
typedef std::map<uint32, ContaineeList>     ContainerMap;


struct BestOverlapGraph;


struct Unitig{
  Unitig(bool report=false);
  ~Unitig(void);

  void sort(void);
  void reverseComplement(bool doSort=true);

  // Accessor methods

  float getAvgRho(FragmentInfo *fi);
  static void setGlobalArrivalRate(float global_arrival_rate);
  void setLocalArrivalRate(float local_arrival_rate);
  float getLocalArrivalRate(FragmentInfo *fi);
  float getCovStat(FragmentInfo *fi);

  // getNumRandomFrags() is a placeholder, random frags should not
  // contain guides, or other fragments that are not randomly sampled
  // across the whole genome.

  long getLength(void)          { return(_length);                   };
  long getNumFrags(void)        { return(dovetail_path_ptr->size()); };
  long getNumRandomFrags(void)  { return(getNumFrags());             };

  DoveTailNode getLastBackboneNode(void);
  DoveTailNode getLastBackboneNode(uint32 &);

  uint32       id(void) { return(_id); };

  bool placeFrag(DoveTailNode &place5, int32 &fidx5, BestEdgeOverlap *bestedge5,
                 DoveTailNode &place3, int32 &fidx3, BestEdgeOverlap *bestedge3);

  void addFrag(DoveTailNode node, int offset=0, bool report=false);
  bool addContainedFrag(int32 fid, BestContainment *bestcont, bool report=false);
  bool addAndPlaceFrag(int32 fid, BestEdgeOverlap *bestedge5, BestEdgeOverlap *bestedge3, bool report=false);


  static uint32 fragIn(uint32 fragId) {
    if ((_inUnitig == NULL) || (fragId == 0))
      return 0;
    return _inUnitig[fragId];
  };

  static uint32 pathPosition(uint32 fragId) {
    if ((_pathPosition == NULL) || (fragId == 0))
      return ~0;
    return _pathPosition[fragId];
  };

  static void resetFragUnitigMap(uint32 numFrags) {
    if (_inUnitig == NULL)
      _inUnitig = new uint32[numFrags+1];
    memset(_inUnitig, 0, (numFrags+1) * sizeof(uint32));

    if (_pathPosition == NULL)
      _pathPosition = new uint32[numFrags+1];
    memset(_pathPosition, 0, (numFrags+1) * sizeof(uint32));
  };

  // Public Member Variables
  DoveTailPath *dovetail_path_ptr;

private:

  // Do not access these private variables directly, they may
  // not be computed yet, use accessors!
  //
  float  _avgRho;
  float  _covStat;
  long   _length;
  float  _localArrivalRate;
  uint32 _id;

  static uint32 nextId;
  static float _globalArrivalRate;
  static uint32 *_inUnitig;      //  Maps a fragment iid to a unitig id.
  static uint32 *_pathPosition;  //  Maps a fragment iid to an index in the dovetail path
};


typedef std::vector<Unitig*> UnitigVector;
typedef UnitigVector::iterator UnitigsIter;
typedef UnitigVector::const_iterator UnitigsConstIter;


#endif  //  INCLUDE_AS_BOG_UNITIG
