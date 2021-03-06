
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
/* $Id: processInter.cc,v 1.7 2008/06/27 06:29:17 brianwalenz Exp $ */
#include <cstdio>  // for sscanf
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <unistd.h> /* man 3 getopt */

using namespace std;

#include "Polygon.h"
#include "MPTypes.h"
#include "Interval.h"
#include "PolygonIntersector.h"
#include "TwoDIntervalClique.h"
#include "IntervalSetCoverSolver.h"
#include "MatePairPolygon.h"
#include "CompositeMPPolygon.h"
#include "CloneLibrary.h"
#include "clusterMPs.h"

#include <unistd.h> /* for getopt */

//#define USE_SATISFIEDS

#define OUTPUT_ATAC_FORMAT

char * MatePairLabel[MPI_NUM_INDICES] =
{
  "stretched",
  "compressed",
  "outtie",
  "normal",
  "antinormal",
  "inversion",
  "transposition",
  "satisfied",
  "interSequence",
  "unknown"
};


class Translation
{
public:
  Translation(){_flip = false; _shiftBP = 0;}
  Translation(const FragmentPosition & f1,
              const FragmentPosition & f2,
              const CloneLibrary & lib)
    {
      set(f1, f2, lib);
    }

  void set(const FragmentPosition & f1,
           const FragmentPosition & f2,
           const CloneLibrary & lib)
    {
      _flip = (f1.getOrientation() == f2.getOrientation());
      if(f1.getOrientation() == SINGLE_A_B)
      {
        // f2 should be to the right
        _shiftBP = (UNIT_TYPE) (f1.getFiveP() + lib.getMean() - f2.getFiveP());
      }
      else
      {
        // f2 should be to the left
        _shiftBP = (UNIT_TYPE) (f1.getFiveP() - lib.getMean() - f2.getFiveP());
      }
    }

  void translate(const FragmentPosition & f, FragmentPosition & fback) const
    {
      fback.setUID(f.getUID());
      fback.setFiveP(f.getFiveP() + getShift());
      fback.setSequenceID(f.getSequenceID());

      switch(f.getOrientation())
      {
        case SINGLE_A_B:
          if(_flip)
            fback.setOrientation(SINGLE_B_A);
          else
            fback.setOrientation(SINGLE_A_B);
          break;
        case SINGLE_B_A:
          if(_flip)
            fback.setOrientation(SINGLE_A_B);
          else
            fback.setOrientation(SINGLE_B_A);
          break;
        default:
          assert(0);
          break;
      }
    }

  bool getFlip() const {return _flip;}
  UNIT_TYPE getShift() const {return _shiftBP;}
  bool isCompatible(const Translation & other, UNIT_TYPE slop) const
    {
      return(getFlip() == other.getFlip() &&
             getShift() + slop >= other.getShift() &&
             getShift() - slop <= other.getShift());
    }

private:
  // _flip indicates whether one or the other fragment has to be flipped
  // in moving from its axis to the other
  bool _flip;
  // _shiftBP is the number of basepairs to shift f2's 5p coordinate
  // on f2's axis to get it to where it should be relative to
  // f1's 5p coordinate on f1's axis
  UNIT_TYPE _shiftBP;
};

/*
  PURPOSE:
    Process inter-sequence mate pairs - mate pairs for which each fragment
    was mapped to a different sequence - to identify mis-assemblies and
    perhaps re-sequenced plates or collapsed repeats

  PLAN:
    Read in clone library length estimates

    Read in all intra-sequence mate pairs for a given sequence/assembly
      store each mate pair twice (by left and by right frag coord)
      sort by 'left' coorinate

    Read in all inter-sequence mate pairs for a given sequence/assembly
      Maintain separate list of mates for each other sequence
        optionally omit lower numbered sequences to avoid duplication
      Sort each list by coordinate on other sequence

    c = this sequence
    For each other sequence, o
      for each mate, m, on that sequence
        create a list, l, of agreeing mate pairs
        determine relative coordinate range & orientation of m
          on c based on m's mate's coordinate & orientation
        for each subsequent mate, n, on o until none for 100kb
          if it agrees with m, add it to l
      if l has 1 member, forget about it
        otherwise print interval on c and o and list of mate pairs & coords
 */

void Usage(char * progname, char * message)
{
  if(message != NULL)
    cerr << endl << message << endl;
#ifdef USE_SATISFIEDS
  cerr << "Usage: " << progname << " [-l lib] [-m intra-sequenceMPs] [-e inter-sequenceMPs] [-n #stddevs] -[A|H|L]\n";
  cerr << "\t-m mps        name of mate pair file\n";
#else
  cerr << "Usage: " << progname << " [-l lib] [-e inter-sequenceMPs] [-n #stddevs] -[A|H|L]\n";
  cerr << "\t-l lib        name of clone library file\n";
#endif

  cerr << "\t-e name of inter-sequence matepairs file\n";
  cerr << "\t-n #stddevs   number of stddevs from mean that is excessive\n\n";
  cerr << "\t                default is " << STDDEVS_THRESHOLD << endl;
  cerr << "\t-a assembly   assembly name (e.g., B33A, VAN)\n";
  cerr << "\t-s #          sequence number (index into fasta file)\n";
  cerr << "\t-g            do not print gnuplot output\n";
  cerr << "\t-f #          filter out mate pair sets with fewer members\n";
  cerr << "\t                default is " << CONFIRMATION_THRESHOLD << endl;
  cerr << "\t-[A|H|L]      process mates in All, Higher-numbered only, or\n";
  cerr << "\t                Lower-numbered only inter-sequence pairs.\n";
  cerr << "\t                default is " << INTER_S_DEFAULT_SCOPE << endl;
  exit(1);
}


void ReadInterSequenceMPs(vector<list<MatePair> > & icmps,
                            ifstream & fin,
                            InterSequenceScope_e interScope)
{
  char line[4096];
  MatePair mp;
  while(fin.getline(line, 4095))
  {
    mp.setFromInterSequenceString(line);
    // left sequence is 'this' sequence
    if(interScope == ISS_ALL ||
       (mp.getRightSequenceID() > mp.getLeftSequenceID() &&
        interScope == ISS_HIGHER) ||
       (mp.getRightSequenceID() < mp.getLeftSequenceID() &&
        interScope == ISS_LOWER))
    {
      // make sure that icmps has enough entries to add a
      // matepair from this sequence to a list in icmps
      while(icmps.size() <= mp.getRightSequenceID())
      {
        list<MatePair> lmp;
        icmps.push_back(lmp);
      }
      icmps[mp.getRightSequenceID()].push_back(mp);
    }
  }
  for(unsigned int i = 0; i < icmps.size(); i++)
  {
    if(icmps[i].size() > 1)
      icmps[i].sort();
  }
}


int main(int argc, char ** argv)
{
  char * libFilename = NULL;
#ifdef USE_SATISFIEDS
  char * mpFilename = NULL;
#endif
  char * ewFilename = NULL;
  char * assembly = NULL;
  int sequenceID = -1;
  double numStddevs = STDDEVS_THRESHOLD;
  int filterThresh = CONFIRMATION_THRESHOLD;
  InterSequenceScope_e interScope = INTER_S_DEFAULT_SCOPE;
  bool printGnuplot = true;

  {
    int ch, errflg = 0;
    while(!errflg && ((ch = getopt(argc, argv, "l:m:e:n:a:s:gf:AHL")) != EOF))
    {
      switch(ch)
      {
        case 'l':
          libFilename = optarg;
          break;
#ifdef USE_SATISFIEDS
        case 'm':
          mpFilename = optarg;
          break;
#endif
        case 'e':
          ewFilename = optarg;
          break;
        case 'n':
          numStddevs = atof(optarg);
          break;
        case 'a':
          assembly = optarg;
          break;
        case 's':
          sequenceID = atoi(optarg);
          break;
        case 'g':
          printGnuplot = false;
          break;
        case 'f':
          filterThresh = atoi(optarg);
          break;
        case 'A':
          interScope = ISS_ALL;
          break;
        case 'H':
          interScope = ISS_HIGHER;
          break;
        case 'L':
          interScope = ISS_LOWER;
          break;
        default:
          errflg++;
          break;
      }
    }
    if(libFilename == NULL)
      Usage(argv[0], "Please specify a clone library filename");
#ifdef USE_SATSIFIEDS
    if(mpFilename == NULL)
      Usage(argv[0], "Please specify an intra-sequence mate pair filename");
#endif
    if(ewFilename == NULL)
      Usage(argv[0], "Please specify an inter-sequence mate pair filename");
    if(numStddevs <= 0)
      Usage(argv[0], "Please specify a positive number of std deviations");
    if(assembly == NULL)
      Usage(argv[0], "Please specify an assembly name");
    if(sequenceID < 0)
      Usage(argv[0], "Please specify a sequence number");
  }

  // read the clone library files
  map<ID_TYPE, CloneLibrary> libs;
  ifstream flib(libFilename, ios::in);
  if(!flib.good())
  {
    cerr << "Failed to open " << libFilename << " for reading\n";
    exit(1);
  }
  // cerr << "Reading clone library data from file " << libFilename << endl;
  ReadCloneLibs(libs, flib);
  flib.close();

  vector<list<MatePair > > icmps;
  {
    // read inter-sequence mate pairs
    ifstream fe(ewFilename, ios::in);
    if(!fe.good())
    {
      cerr << "Failed to open " << ewFilename << " for reading\n";
      exit(1);
    }
    // cerr << "Reading inter-sequence mate pairs from " << ewFilename << endl;
    ReadInterSequenceMPs(icmps, fe, interScope);
    fe.close();
    if(icmps.size() == 0)
    {
      cerr << "No inter-sequence mate pairs read from file "
           << ewFilename << endl;
      exit(0);
    }
  }

#ifdef USE_SATISFIEDS
  {
    // read the mate pairs & create mate pair polygons out of them
    vector<MatePair> mps;
    ifstream fmp(mpFilename, ios::in);
    if(!fmp.good())
    {
      cerr << "Failed to open " << mpFilename << " for reading\n";
      exit(1);
    }
    cerr << "Reading satisfied matepairs from " << mpFilename << endl;
    ReadMatePairs(mps, fmp);
    fmp.close();

    // hold onto just satisfied mate pairs
    list<MatePair> smpsl;
    int numKept = 0;
    int numOmitted = 0;
    for(unsigned int i = 0; i < mps.size(); i++)
    {
      MatePairPolygon<UNIT_TYPE> mpp(mps[i], libs[mps[i].getLibUID()], numStddevs);
      CompositeMPPolygon<UNIT_TYPE> cmpp(mpp);
      if(cmpp.isSatisfied())
      {
        // just keep a list of satisfied mate pairs & their reverse
        smpsl.push_back(mps[i]);
        MatePair revmp(mps[i]);
        revmp.setLeftFrag(mps[i].getRightFrag());
        revmp.setRightFrag(mps[i].getLeftFrag());
        smpsl.push_back(revmp);
        numKept++;
      }
      else
        numOmitted++;
    }
    smpsl.sort(); // sort satisfied mate pairs on their left coordinate

    cerr << numKept << " satisfied mate pairs to be processed\n";

    vector<MatePair> smpsv;
    {
      // copy sorted satisfied mate pairs into vector
      list<MatePair>::iterator smpsi;
      for(smpsi = smpsl.begin(); smpsi != smpsl.end(); smpsi++)
        smpsv.push_back(*smpsi);
    }
  }
#endif

#ifdef OUTPUT_ATAC_FORMAT
  char fname[1024];
  sprintf(fname, "%s.%03d.interSequence.ata", assembly, sequenceID);
  ofstream fo(fname, ios::out);
  if(!fo.good())
  {
    cerr << "Failed to open " << fname << " for writing\n";
    exit(1);
  }

  // write ata header lines
  fo << "! format ata 1.0\n";
  fo << "# numStddevs=" << numStddevs << endl;
  int atacCount = sequenceID * 10000;;
#endif

  // for each other sequence
  for(unsigned int i = 0; i < icmps.size(); i++)
  {
    list<MatePair>::iterator lmp1;
    int numDupes = 0;

    // for each half mate pair in the other sequence
    for(lmp1 = icmps[i].begin(); lmp1 != icmps[i].end(); lmp1++)
    {
      list<MatePair> matches;
      matches.push_back(*lmp1);

      UNIT_TYPE maxDelta =
        (UNIT_TYPE) (numStddevs * libs[lmp1->getLibUID()].getStddev());
      /*
        This coordinate is considered to be LEFT of the other sequence
        For intervals in which there are fragments,
          thisLeft = leftmost coordinate on this sequence
          thisRight = rightmost coordinate on this sequence
          otherLeft = leftmost coordinate on other sequence
          otherRight = rightmost coordinate on other sequence
        For intervals where the fragments might/should go
          otherProjectThisLeft = leftmost coordinate on this sequence
            (left end of other projected onto this)
          otherProjectThisRight = rightmost coordinate on this sequence
          thisProjectOtherLeft = leftmost coordinate on other sequence
          thisProjectOtherRight = rightmost coordinate on other sequence

       */
      Translation trans1(lmp1->getLeftFrag(),
                         lmp1->getRightFrag(),
                         libs[lmp1->getLibUID()]);
      Translation revTrans1(lmp1->getRightFrag(),
                            lmp1->getLeftFrag(),
                            libs[lmp1->getLibUID()]);
      UNIT_TYPE thisLeft, thisRight;
      UNIT_TYPE thisProjectOtherLeft, thisProjectOtherRight;
      UNIT_TYPE otherLeft, otherRight;
      UNIT_TYPE otherProjectThisLeft, otherProjectThisRight;
      FragmentPosition fpt;

      thisLeft = thisRight = lmp1->getLeftCoord();
      otherLeft = otherRight = lmp1->getRightCoord();
      bool mixedOrients = false;

      revTrans1.translate(lmp1->getLeftFrag(), fpt);
      thisProjectOtherLeft = thisProjectOtherRight = fpt.getFiveP();

      trans1.translate(lmp1->getRightFrag(), fpt);
      otherProjectThisLeft = otherProjectThisRight = fpt.getFiveP();

      int numInterfering = 0;
      list<MatePair>::iterator lmp2 = lmp1;
      for(lmp2++; lmp2 != icmps[i].end(); )
      {
        if(lmp2->getLeftCoord() > lmp1->getLeftCoord() + 10000 ||
           numInterfering > 1) break;
        if(lmp1->isWithinDelta(*lmp2, COINCIDENT_THRESHOLD))
        {
          lmp2 = icmps[i].erase(lmp2);
          numDupes++;
          continue;
        }

        Translation trans2(lmp2->getLeftFrag(),
                           lmp2->getRightFrag(),
                           libs[lmp2->getLibUID()]);
        if(trans1.isCompatible(trans2, maxDelta))
        {
          Translation revTrans2(lmp2->getRightFrag(),
                                lmp2->getLeftFrag(),
                                libs[lmp2->getLibUID()]);

          if(lmp2->getLeftFrag().getOrientation() !=
             lmp1->getLeftFrag().getOrientation() &&
             lmp2->getRightFrag().getOrientation() !=
             lmp1->getRightFrag().getOrientation())
            mixedOrients = true;

          matches.push_back(*lmp2);

          // update extreme left/right on this sequence
          thisLeft = (thisLeft < lmp2->getLeftCoord()) ?
            thisLeft : lmp2->getLeftCoord();
          thisRight = (thisRight > lmp2->getLeftCoord()) ?
            thisRight : lmp2->getLeftCoord();

          // update extreme left/right on other sequence
          otherLeft = (otherLeft < lmp2->getRightCoord()) ?
            otherLeft : lmp2->getRightCoord();
          otherRight = (otherRight > lmp2->getRightCoord()) ?
            otherRight : lmp2->getRightCoord();

          // update extreme left/right of projection of this
          // sequence onto other
          revTrans2.translate(lmp2->getLeftFrag(), fpt);
          thisProjectOtherLeft = (thisProjectOtherLeft < fpt.getFiveP()) ?
            thisProjectOtherLeft : fpt.getFiveP();
          thisProjectOtherRight = (thisProjectOtherRight > fpt.getFiveP()) ?
            thisProjectOtherRight : fpt.getFiveP();

          // update extreme left/right of projection of other
          // sequence onto this
          trans2.translate(lmp2->getRightFrag(), fpt);
          otherProjectThisLeft = (otherProjectThisLeft < fpt.getFiveP()) ?
            otherProjectThisLeft : fpt.getFiveP();
          otherProjectThisRight = (otherProjectThisRight > fpt.getFiveP()) ?
            otherProjectThisRight : fpt.getFiveP();

          lmp2 = icmps[i].erase(lmp2);
        }
        else
        {
          numInterfering++;
          lmp2++;
        }
      }
      if(matches.size() > 1)
      {
#ifdef OUTPUT_ATAC_FORMAT
        /*
          write ATAC format:
          Parent Match between an interval on each sequence. (white)
            Intervals are from
              left 5p to where right 5p should be on one sequence
              where left 5p should be to right 5p on the other sequence
          Example:
            M ma ma1 . VAN:1 0 10000 ? VAN:2 5 10005 ?
          2 Child matches between interval with fragments on one sequence
            to interval w/o fragments on other sequence (green)
          Example:
            M mb 2 ma1 VAN:1 8000 10000 ? VAN:2 8005 10005 ?
            M mb 3 ma1 VAN:2 5 2005 ? VAN:1 0 2000 ?
          1 Child match between two intervals with fragments (red)
          Example:
            M mc 4 ma1 VAN:1 8000 10000 ? VAN2: 0 2005 ? > /weight=2
          One match for each mate pair (blue)
            M md 5 ma1 VAN:1 8000 8050 ? VAN:2 5 55 ? > /leftUID=x /rightUID=y /libUID=z
            M md 6 ma1 VAN:1 9050 10000 ? VAN:2 1955 2005 ? > /leftUID=a /rightUID=b /libUID=c
        */

        int refCount = atacCount++;
        UNIT_TYPE delta;

        // parent match between large to/from intervals
        fo << "M xa xa" << refCount << " . "
           << assembly << ":" << lmp1->getLeftSequenceID() << " ";
        if(thisLeft > otherProjectThisRight)
        {
          delta = thisLeft - otherProjectThisRight;
          fo << otherProjectThisRight << " " << delta << " -1 ";
        }
        else
        {
          delta = otherProjectThisRight - thisLeft;
          fo << thisLeft << " " << delta << " 1 ";
        }
        fo << assembly << ":" << lmp1->getRightSequenceID() << " "
           << otherLeft << " " << delta << " 1 "
           << " > /weight=" << matches.size();

        if(mixedOrients)
          fo << " /mixedOrientations=1\n";
        else
          fo << " /mixedOrientations=0\n";

        // first child match between from/to intervals
        // interval of frags on THIS sequence and where they would go
        // on the OTHER sequence
        fo << "M xb " << atacCount++ << " xa" << refCount << " "
           << assembly << ":" << lmp1->getLeftSequenceID() << " "
           << thisLeft << " " << thisRight - thisLeft << " 1 "
           << assembly << ":" << lmp1->getRightSequenceID() << " "
           << thisProjectOtherLeft << " " << thisProjectOtherRight - thisProjectOtherLeft << " 1 " << endl;

        // second child match between from/to intervals
        // interval of frags on OTHER seq and where they would go
        // on the THIS seq
        fo << "M xc " << atacCount++ << " xa" << refCount << " "
           << assembly << ":" << lmp1->getLeftSequenceID() << " "
           << otherProjectThisLeft << " " << otherProjectThisRight - otherProjectThisLeft << " 1 "
           << assembly << ":" << lmp1->getRightSequenceID() << " "
           << otherLeft << " " << otherRight - otherLeft << " 1 " << endl;

        // last interval match, between two intervals with frags
        fo << "M xd " << atacCount++ << " xa" << refCount << " "
           << assembly << ":" << lmp1->getLeftSequenceID() << " "
           << thisLeft << " " << thisRight - thisLeft << " 1 "
           << assembly << ":" << lmp1->getRightSequenceID() << " "
           << otherLeft << " " << otherRight - otherLeft << " 1 " << endl;

        atacCount++;
#else
        cout << endl;
        cout << matches.size() << " mps in ( "
             << lmp1->getLeftSequenceID() << " "
             << thisLeft << " " << thisRight << " ) indicate ";
        cout << "( " << lmp1->getRightSequenceID() << " "
             << otherLeft << " " << otherRight << " ) ==> ( "
             << lmp1->getLeftSequenceID() << " ";
        if(trans1.getFlip())
          cout << otherProjectThisRight << " " << otherProjectThisLeft;
        else
          cout << otherProjectThisLeft << " " << otherProjectThisRight;
        cout << " )\n";

        list<MatePair>::iterator lmatches;
        for(lmatches = matches.begin(); lmatches != matches.end(); lmatches++)
        {
          cout << "(" << lmatches->getLeftFragUID() << ","
               << lmatches->getRightFragUID() << ") ";
        }
        cout << endl;
#endif
      }
    }
  }
#ifdef OUTPUT_ATAC_FORMAT
  fo.close();
#endif
  return 0;
}
