
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
/* $Id: MatePairPolygon.h,v 1.6 2008/06/27 06:29:16 brianwalenz Exp $ */
#ifndef MATEPAIRPOLYGON_H
#define MATEPAIRPOLYGON_H

#include <iostream>
#include <iomanip>
#include <vector>
#include <cassert>

#include "Polygon.h"
#include "CloneLibrary.h"
#include "MatePairGroup.h"

template <class UnitType>
class MatePairPolygon :
  public Polygon<UnitType>,
  public MatePairGroup
{
public:
  MatePairPolygon()
    {
    }
  MatePairPolygon(const vector<Point<UnitType> > & pts, const MatePair & mp) :
    Polygon<UnitType>(pts), MatePairGroup(mp)
    {
    }
  MatePairPolygon(const MatePairPolygon<UnitType> & other) :
    Polygon<UnitType>(other), MatePairGroup(other)
    {
    }
  MatePairPolygon(const MatePair & mp, const CloneLibrary & cl, double n) :
    MatePairGroup(mp)
    {
      Point<UnitType> p;
      UnitType large = (UnitType) (cl.getMean() + n * cl.getStddev());
      UnitType small = (UnitType) (cl.getMean() - n * cl.getStddev());
      switch(mp.getOrientation())
      {
        case PAIR_INNIE:
          if(mp.getRightCoord() > mp.getLeftCoord() + large)
          {
            // stretched
          /* Four points:p
             0. left5p, right5p - (mean + n * stddev)
             1. left5p, right5p - (mean - n * stddev)
             2. left5p + (mean - n * stddev), right5p
             3. left5p + (mean + n * stddev), right5p

             |         2     3
             v

             -  1


             -  0
                --->   (     )
                left frag
           */
            p.setX(mp.getLeftCoord());
            p.setY(mp.getRightCoord() - large);
            append(p);
            p.setY(mp.getRightCoord() - small);
            append(p);
            p.setX(mp.getLeftCoord() + small);
            p.setY(mp.getRightCoord());
            append(p);
            p.setX(mp.getLeftCoord() + large);
            append(p);
            setMPIndex(MPI_STRETCHED);
          }
          else if(mp.getRightCoord() < mp.getLeftCoord() + small)
          {
            // compressed
            /*
              This is CR_NATIVE format, in which both mates are
              on the X axis and the range of possible deleted lengths
              is on the Y axis
            */
            p.setX(mp.getLeftCoord());
            p.setY(mp.getLeftCoord() + small - mp.getRightCoord());
            append(p);
            p.setY(mp.getLeftCoord() + large - mp.getRightCoord());
            append(p);
            p.setX(mp.getRightCoord());
            append(p);
            p.setY(mp.getLeftCoord() + small - mp.getRightCoord());
            append(p);
            setMPIndex(MPI_COMPRESSED);
          }
          else
          {
            // satisfied
            p.setX(mp.getLeftCoord());
            p.setY(mp.getLeftCoord());
            append(p);
            p.setY(mp.getRightCoord());
            append(p);
            p.setX(mp.getRightCoord());
            append(p);
            p.setY(mp.getLeftCoord());
            append(p);
            setMPIndex(MPI_SATISFIED);
          }
          break;
        case PAIR_NORMAL:
          /* Four points:p
             0. left5p, right5p + (mean - n * stddev)
             1. left5p, right5p + (mean + n * stddev)
             2. left5p + mean + n * stddev, right5p
             3. left5p + mean - n * stddev, right5p

            -   1


            -   0

            ^
            |          3     2
                -->    (     )
                left frag
          */
          p.setX(mp.getLeftCoord());
          p.setY(mp.getRightCoord() + small);
          append(p);
          p.setY(mp.getRightCoord() + large);
          append(p);
          p.setX(mp.getLeftCoord() + large);
          p.setY(mp.getRightCoord());
          append(p);
          p.setX(mp.getLeftCoord() + small);
          append(p);
          setMPIndex(MPI_NORMAL);
          break;
        case PAIR_ANTINORMAL:
          /* Four points:p
             0. left5p, right5p + (mean - n * stddev)
             1. left5p, right5p + (mean + n * stddev)
             2. left5p + mean + n * stddev, right5p
             3. left5p + mean - n * stddev, right5p

            |  0     1
            v

            -              2


            -              3
               (     )   <--
                         left frag
          */
          p.setX(mp.getLeftCoord() - large);
          p.setY(mp.getRightCoord());
          append(p);
          p.setX(mp.getLeftCoord() - small);
          append(p);
          p.setX(mp.getLeftCoord());
          p.setY(mp.getRightCoord() - small);
          append(p);
          p.setY(mp.getRightCoord() - large);
          append(p);
          setMPIndex(MPI_ANTINORMAL);
          break;
        case PAIR_OUTTIE:
          /* Four points:p
             0. left5p - (mean + n * stddev), right5p
             1. left5p, right5p + (mean + n * stddev)
             2. left5p, right5p + (mean - n * stddev)
             3. left5p - (mean - n * stddev), right5p

            -             1


            -             2

            ^
            |   0    3
                (    )   <--
                         left frag
          */
          p.setX(mp.getLeftCoord() - large);
          p.setY(mp.getRightCoord());
          append(p);
          p.setX(mp.getLeftCoord());
          p.setY(mp.getRightCoord() + large);
          append(p);
          p.setY(mp.getRightCoord() + small);
          append(p);
          p.setX(mp.getLeftCoord() - small);
          p.setY(mp.getRightCoord());
          append(p);
          setMPIndex(MPI_OUTTIE);
          break;
        default:
          cerr << "Unknown mate pair orientation: "
               << mp.getOrientation() << endl;
          assert(0);
          break;
      }
    }

  void printForGnuplot(ostream & os, CompressedRepresentation_e cr) const
    {
      if(this->ppts.size() == 0) return;

      for(int i = 0; i < getNumMPs(); i++)
        os << "# " << pmps[i] << endl;

      if(isCompressed())
      {
        switch(cr)
        {
          case CR_COMPATIBLE:
            os << this->getMinX() << " " << this->getMinX() << endl;
            os << this->getMinX() << " " << this->getMaxX() << endl;
            os << this->getMaxX() << " " << this->getMaxX() << endl;
            os << this->getMaxX() << " " << this->getMinX() << endl;
            os << this->getMinX() << " " << this->getMinX() << endl;
            break;
          case CR_NATIVE:
            os << this->getMinX() << " " << this->getMinY() << endl;
            os << this->getMinX() << " " << this->getMaxY() << endl;
            os << this->getMaxX() << " " << this->getMaxY() << endl;
            os << this->getMaxX() << " " << this->getMinY() << endl;
            os << this->getMinX() << " " << this->getMinY() << endl;
            break;
        }
        os << endl;
      }
      else
      {
        for(unsigned int i = 0; i < this->ppts.size(); i++)
          os << this->ppts[i].getX() << " " << this->ppts[i].getY() << endl;
        os << this->ppts[0].getX() << " " << this->ppts[0].getY() << endl << endl;
      }
    }

  void print(ostream & os) const
    {
      if(this->ppts.size() == 0) return;

      for(int i = 0; i < getNumMPs(); i++)
        os << "# " << pmps[i] << endl;

      for(unsigned int i = 0; i < this->poly.ppts.size(); i++)
        os << " " << this->poly.ppts[i];
      os << "\n" << this->poly.getNumMPs() << " mate pairs:\n";
      for(int i = 0; i < this->poly.getNumMPs(); i++)
        os << this->poly.pmps[i] << endl;
      return os;
    }

private:
};


#endif // MATEPAIRPOLYGON_H
