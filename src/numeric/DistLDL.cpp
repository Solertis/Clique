/*
   Clique: a scalable implementation of the multifrontal algorithm

   Copyright (C) 2011 Jack Poulson, Lexing Ying, and 
   The University of Texas at Austin
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "clique.hpp"
using namespace elemental;

template<typename F> // F represents a real or complex field
void clique::numeric::DistLDL
( Orientation orientation,
        symbolic::DistSymmFact& S, // can't be const due to map...
  const numeric::LocalSymmFact<F>& localL,
        numeric::DistSymmFact<F>& distL )
{
#ifndef RELEASE
    PushCallStack("numeric::DistLDL");
    if( orientation == NORMAL )
        throw std::logic_error("LDL must be (conjugate-)transposed");
#endif
    const int numSupernodes = S.supernodes.size();
    if( numSupernodes == 0 )
        return;

    // The bottom front is already computed, so just view it
    distL.fronts[0].LocalMatrix().LockedView( localL.fronts.back() );

    // Perform the distributed portion of the factorization
    std::vector<int>::const_iterator it;
    for( unsigned k=1; k<numSupernodes; ++k )
    {
        const symbolic::DistSymmFactSupernode& childSymbSN = S.supernodes[k-1];
        const symbolic::DistSymmFactSupernode& symbSN = S.supernodes[k];
        const DistMatrix<F,MC,MR>& childFront = distL.fronts[k-1];
        DistMatrix<F,MC,MR>& front = distL.fronts[k];
        const bool computeRecvIndices = ( symbSN.childRecvIndices.size() == 0 );

        // Grab this front's grid information
        const Grid& grid = front.Grid();
        mpi::Comm comm = grid.VCComm();
        const unsigned commRank = mpi::CommRank( comm );
        const unsigned commSize = mpi::CommSize( comm );
        const unsigned gridHeight = grid.Height();
        const unsigned gridWidth = grid.Width();

        // Grab the child's grid information
        const Grid& childGrid = childFront.Grid();
        mpi::Comm childComm = childGrid.VCComm();
        const unsigned childCommRank = mpi::CommRank( childComm );
        const unsigned childCommSize = mpi::CommSize( childComm );
        const unsigned childGridHeight = childGrid.Height();
        const unsigned childGridWidth = childGrid.Width();
        const unsigned childGridRow = childGrid.MCRank();
        const unsigned childGridCol = childGrid.MRRank();

#ifndef RELEASE
        if( front.Height() != symbSN.size+symbSN.lowerStruct.size() ||
            front.Width()  != symbSN.size+symbSN.lowerStruct.size() )
            throw std::logic_error("Front was not the proper size");
#endif

        // Pack our child's updates
        const bool isLeftChild = ( commRank < commSize/2 );
        it = std::max_element
             ( symbSN.numChildSendIndices.begin(), 
               symbSN.numChildSendIndices.end() );
        const int sendPortionSize = std::max(*it,mpi::MIN_COLL_MSG);
        std::vector<int> sendBuffer( sendPortionSize*commSize );
        std::vector<int> sendOffsets( commSize, 0 );
        const std::vector<int>& myChildRelIndices = 
            ( isLeftChild ? symbSN.leftChildRelIndices
                          : symbSN.rightChildRelIndices );
        // HERE: Must rethink the fact that the child's update matrix does not
        //       have trivial alignments. Must compute shifts.
        for( int jChild=childGridCol; 
                 jChild<childSymbSN.lowerStruct.size(); jChild+=childGridWidth )
        {
            const int destGridCol = myChildRelIndices[jChild] % gridWidth;
            const int align = jChild % childGridHeight;
            const int shift = 
                (childGridRow+childGridHeight-align) % childGridHeight;
            for( int iChild=jChild+shift; 
                     iChild<childSymbSN.lowerStruct.size(); 
                     iChild+=childGridHeight )
            {
                const int destGridRow = myChildRelIndices[iChild] % gridHeight;
                const int destRank = destGridRow + destGridCol*gridHeight;
                //sendBuffer[sendOffsets[destRank]++] = 
                //    childFront.GetLocalEntry(iChildLocal,jChildLocal);
            }
        }

        // AllToAll to send and receive the child updates
        if( computeRecvIndices )
            symbolic::ComputeRecvIndices( symbSN );
        int recvPortionSize = mpi::MIN_COLL_MSG;
        for( int i=0; i<commSize; ++i )
        {
            const int thisPortion = symbSN.childRecvIndices[i].size();
            recvPortionSize = std::max(thisPortion,recvPortionSize);
        }
        std::vector<int> recvBuffer( recvPortionSize*commSize );
        // TODO

        // Unpack the child udpates (with an Axpy)
        // TODO
        symbSN.childRecvIndices.clear();

        DistSupernodeLDL( orientation, front, symbSN.size );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template void clique::numeric::DistLDL
( Orientation orientation,
        symbolic::DistSymmFact& S,
  const numeric::LocalSymmFact<float>& localL,
        numeric::DistSymmFact<float>& distL );

template void clique::numeric::DistLDL
( Orientation orientation,
        symbolic::DistSymmFact& S,
  const numeric::LocalSymmFact<double>& localL,
        numeric::DistSymmFact<double>& distL );

template void clique::numeric::DistLDL
( Orientation orientation,
        symbolic::DistSymmFact& S,
  const numeric::LocalSymmFact<std::complex<float> >& localL,
        numeric::DistSymmFact<std::complex<float> >& distL );

template void clique::numeric::DistLDL
( Orientation orientation,
        symbolic::DistSymmFact& S,
  const numeric::LocalSymmFact<std::complex<double> >& localL,
        numeric::DistSymmFact<std::complex<double> >& distL );

