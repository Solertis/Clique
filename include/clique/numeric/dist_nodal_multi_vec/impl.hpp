/*
   Copyright (c) 2009-2014, Jack Poulson, Lexing Ying,
   The University of Texas at Austin, Stanford University, and the
   Georgia Insitute of Technology.
   All rights reserved.
 
   This file is part of Clique and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef CLIQ_NUMERIC_DISTNODALMULTIVEC_IMPL_HPP
#define CLIQ_NUMERIC_DISTNODALMULTIVEC_IMPL_HPP

namespace cliq {

template<typename F>
inline
DistNodalMultiVec<F>::DistNodalMultiVec()
: height_(0), width_(0)
{ }

template<typename F>
inline
DistNodalMultiVec<F>::DistNodalMultiVec
( const DistMap& inverseMap, const DistSymmInfo& info,
  const DistMultiVec<F>& X )
{
    DEBUG_ONLY(CallStackEntry cse("DistNodalMultiVec::DistNodalMultiVec"))
    Pull( inverseMap, info, X );
}

template<typename F>
inline
DistNodalMultiVec<F>::DistNodalMultiVec( const DistNodalMatrix<F>& X )
{
    DEBUG_ONLY(CallStackEntry cse("DistNodalMultiVec::DistNodalMultiVec"))
    *this = X;
}

template<typename F>
inline const DistNodalMultiVec<F>&
DistNodalMultiVec<F>::operator=( const DistNodalMatrix<F>& X )
{
    DEBUG_ONLY(CallStackEntry cse("DistNodalMultiVec::operator="))
    height_ = X.Height();
    width_ = X.Width();

    // Copy over the nontrivial distributed nodes
    const int numDist = X.distNodes.size();
    distNodes.resize( numDist );
    for( int s=0; s<numDist; ++s )
    {
        distNodes[s].SetGrid( X.distNodes[s].Grid() );
        distNodes[s] = X.distNodes[s];
    }

    // Copy over the local nodes
    const int numLocal = X.localNodes.size();
    localNodes.resize( numLocal );
    for( int s=0; s<numLocal; ++s )
        localNodes[s] = X.localNodes[s];

    return *this;
}

template<typename F>
inline void
DistNodalMultiVec<F>::Pull
( const DistMap& inverseMap, const DistSymmInfo& info,
  const DistMultiVec<F>& X )
{
    DEBUG_ONLY(CallStackEntry cse("DistNodalMultiVec::Pull"))
    height_ = X.Height();
    width_ = X.Width();

    // Traverse our part of the elimination tree to see how many indices we need
    int numRecvInds=0;
    const int numLocal = info.localNodes.size();
    for( int s=0; s<numLocal; ++s )
        numRecvInds += info.localNodes[s].size;
    const int numDist = info.distNodes.size();
    for( int s=1; s<numDist; ++s )
        numRecvInds += info.distNodes[s].multiVecMeta.localSize;
    
    // Fill the set of indices that we need to map to the original ordering
    int off=0;
    std::vector<int> mappedInds( numRecvInds );
    for( int s=0; s<numLocal; ++s )
    {
        const SymmNodeInfo& nodeInfo = info.localNodes[s];
        for( int t=0; t<nodeInfo.size; ++t )
            mappedInds[off++] = nodeInfo.off+t;
    }
    for( int s=1; s<numDist; ++s )
    {
        const DistSymmNodeInfo& nodeInfo = info.distNodes[s];
        const Grid& grid = *nodeInfo.grid;
        const int gridSize = grid.Size();
        const int gridRank = grid.VCRank();
        const int alignment = 0;
        const int shift = Shift( gridRank, alignment, gridSize );
        for( int t=shift; t<nodeInfo.size; t+=gridSize )
            mappedInds[off++] = nodeInfo.off+t;
    }
    DEBUG_ONLY(
        if( off != numRecvInds )
            LogicError("mappedInds was filled incorrectly");
    )

    // Convert the indices to the original ordering
    inverseMap.Translate( mappedInds );

    // Figure out how many entries each process owns that we need
    mpi::Comm comm = X.Comm();
    const int commSize = mpi::Size( comm );
    std::vector<int> recvSizes( commSize, 0 );
    const int blocksize = X.Blocksize();
    for( int s=0; s<numRecvInds; ++s )
    {
        const int i = mappedInds[s];
        const int q = RowToProcess( i, blocksize, commSize );
        ++recvSizes[q];
    }
    std::vector<int> recvOffs( commSize );
    off=0;
    for( int q=0; q<commSize; ++q )
    {
        recvOffs[q] = off;
        off += recvSizes[q];
    }
    std::vector<int> recvInds( numRecvInds );
    std::vector<int> offs = recvOffs;
    for( int s=0; s<numRecvInds; ++s )
    {
        const int i = mappedInds[s];
        const int q = RowToProcess( i, blocksize, commSize );
        recvInds[offs[q]++] = i;
    }

    // Coordinate for the coming AllToAll to exchange the indices of X
    std::vector<int> sendSizes( commSize );
    mpi::AllToAll( &recvSizes[0], 1, &sendSizes[0], 1, comm );
    int numSendInds=0;
    std::vector<int> sendOffs( commSize );
    for( int q=0; q<commSize; ++q )
    {
        sendOffs[q] = numSendInds;
        numSendInds += sendSizes[q];
    }

    // Request the indices
    std::vector<int> sendInds( numSendInds );
    mpi::AllToAll
    ( &recvInds[0], &recvSizes[0], &recvOffs[0],
      &sendInds[0], &sendSizes[0], &sendOffs[0], comm );

    // Fulfill the requests
    std::vector<F> sendVals( numSendInds*width_ );
    const int firstLocalRow = X.FirstLocalRow();
    for( int s=0; s<numSendInds; ++s )
        for( int j=0; j<width_; ++j )
            sendVals[s*width_+j] = X.GetLocal( sendInds[s]-firstLocalRow, j );

    // Reply with the values
    std::vector<F> recvVals( numRecvInds*width_ );
    for( int q=0; q<commSize; ++q )
    {
        sendSizes[q] *= width_;
        sendOffs[q] *= width_;
        recvSizes[q] *= width_;
        recvOffs[q] *= width_;
    }
    mpi::AllToAll
    ( &sendVals[0], &sendSizes[0], &sendOffs[0],
      &recvVals[0], &recvSizes[0], &recvOffs[0], comm );
    SwapClear( sendVals );
    SwapClear( sendSizes );
    SwapClear( sendOffs );

    // Unpack the values
    off = 0;
    offs = recvOffs;
    localNodes.resize( numLocal );
    for( int s=0; s<numLocal; ++s )
    {
        const SymmNodeInfo& nodeInfo = info.localNodes[s];
        localNodes[s].Resize( nodeInfo.size, width_ );
        for( int t=0; t<nodeInfo.size; ++t )
        {
            const int i = mappedInds[off++];
            const int q = RowToProcess( i, blocksize, commSize );
            for( int j=0; j<width_; ++j )
                localNodes[s].Set( t, j, recvVals[offs[q]++] );
        }
    }
    distNodes.resize( numDist-1 );
    for( int s=1; s<numDist; ++s )
    {
        const DistSymmNodeInfo& nodeInfo = info.distNodes[s];
        DistMatrix<F,VC,STAR>& XNode = distNodes[s-1];
        XNode.SetGrid( *nodeInfo.grid );
        XNode.Resize( nodeInfo.size, width_ );
        const int localHeight = XNode.LocalHeight();
        for( int tLoc=0; tLoc<localHeight; ++tLoc )
        {
            const int i = mappedInds[off++];
            const int q = RowToProcess( i, blocksize, commSize );
            for( int j=0; j<width_; ++j )
                XNode.SetLocal( tLoc, j, recvVals[offs[q]++] );
        }
    }
    DEBUG_ONLY(
        if( off != numRecvInds )
            LogicError("Unpacked wrong number of indices");
    )
}

template<typename F>
inline void
DistNodalMultiVec<F>::Push
( const DistMap& inverseMap, const DistSymmInfo& info,
        DistMultiVec<F>& X ) const
{
    DEBUG_ONLY(CallStackEntry cse("DistNodalMultiVec::Push"))
    const DistSymmNodeInfo& rootNode = info.distNodes.back();
    mpi::Comm comm = rootNode.comm;
    const int height = rootNode.size + rootNode.off;
    const int width = Width();
    X.SetComm( comm );
    X.Resize( height, width );

    const int commSize = mpi::Size( comm );
    const int blocksize = X.Blocksize();
    const int firstLocalRow = X.FirstLocalRow();
    const int numDist = info.distNodes.size();
    const int numLocal = info.localNodes.size();

    // Fill the set of indices that we need to map to the original ordering
    const int numSendInds = LocalHeight();
    int off=0;
    std::vector<int> mappedInds( numSendInds );
    for( int s=0; s<numLocal; ++s )
    {
        const SymmNodeInfo& nodeInfo = info.localNodes[s];
        for( int t=0; t<nodeInfo.size; ++t )
            mappedInds[off++] = nodeInfo.off+t;
    }
    for( int s=1; s<numDist; ++s )
    {
        const DistSymmNodeInfo& nodeInfo = info.distNodes[s];
        const DistMatrix<F,VC,STAR>& XNode = distNodes[s-1];
        for( int t=XNode.ColShift(); t<XNode.Height(); t+=XNode.ColStride() )
            mappedInds[off++] = nodeInfo.off+t;
    }

    // Convert the indices to the original ordering
    inverseMap.Translate( mappedInds );

    // Figure out how many indices each process owns that we need to send
    std::vector<int> sendSizes( commSize, 0 );
    for( int s=0; s<numSendInds; ++s )
    {
        const int i = mappedInds[s];
        const int q = RowToProcess( i, blocksize, commSize );
        ++sendSizes[q];
    }
    std::vector<int> sendOffs( commSize );
    off=0;
    for( int q=0; q<commSize; ++q )
    {
        sendOffs[q] = off;
        off += sendSizes[q];
    }

    // Pack the send indices and values
    off=0;
    std::vector<F> sendVals( numSendInds*width );
    std::vector<int> sendInds( numSendInds );
    std::vector<int> offs = sendOffs;
    for( int s=0; s<numLocal; ++s )
    {
        const SymmNodeInfo& nodeInfo = info.localNodes[s];
        for( int t=0; t<nodeInfo.size; ++t )
        {
            const int i = mappedInds[off++];
            const int q = RowToProcess( i, blocksize, commSize );
            for( int j=0; j<width; ++j )
                sendVals[offs[q]*width+j] = localNodes[s].Get(t,j);    
            sendInds[offs[q]++] = i;
        }
    }
    for( int s=1; s<numDist; ++s )
    {
        const DistMatrix<F,VC,STAR>& XNode = distNodes[s-1];
        const int localHeight = XNode.LocalHeight();
        for( int tLoc=0; tLoc<localHeight; ++tLoc )
        {
            const int i = mappedInds[off++];
            const int q = RowToProcess( i, blocksize, commSize );
            for( int j=0; j<width; ++j )
                sendVals[offs[q]*width+j] = XNode.GetLocal(tLoc,j);
            sendInds[offs[q]++] = i;
        }
    }

    // Coordinate for the coming AllToAll to exchange the indices of x
    std::vector<int> recvSizes( commSize );
    mpi::AllToAll( &sendSizes[0], 1, &recvSizes[0], 1, comm );
    int numRecvInds=0;
    std::vector<int> recvOffs( commSize );
    for( int q=0; q<commSize; ++q )
    {
        recvOffs[q] = numRecvInds;
        numRecvInds += recvSizes[q];
    }
    DEBUG_ONLY(
        if( numRecvInds != X.LocalHeight() )
            LogicError("numRecvInds was not equal to local height");
    )

    // Send the indices
    std::vector<int> recvInds( numRecvInds );
    mpi::AllToAll
    ( &sendInds[0], &sendSizes[0], &sendOffs[0],
      &recvInds[0], &recvSizes[0], &recvOffs[0], comm );

    // Send the values
    std::vector<F> recvVals( numRecvInds*width );
    for( int q=0; q<commSize; ++q )
    {
        sendSizes[q] *= width;
        sendOffs[q] *= width;
        recvSizes[q] *= width;
        recvOffs[q] *= width;
    }
    mpi::AllToAll
    ( &sendVals[0], &sendSizes[0], &sendOffs[0],
      &recvVals[0], &recvSizes[0], &recvOffs[0], comm );
    SwapClear( sendVals );
    SwapClear( sendSizes );
    SwapClear( sendOffs );

    // Unpack the values
    for( int s=0; s<numRecvInds; ++s )
    {
        const int i = recvInds[s];
        const int iLocal = i - firstLocalRow;
        DEBUG_ONLY(
            if( iLocal < 0 || iLocal >= X.LocalHeight() )
                LogicError("iLocal was out of bounds");
        )
        for( int j=0; j<width; ++j )
            X.SetLocal( iLocal, j, recvVals[s*width+j] );
    }
}

template<typename F>
inline int
DistNodalMultiVec<F>::Height() const
{ return height_; }

template<typename F>
inline int
DistNodalMultiVec<F>::Width() const
{ return width_; }

template<typename F>
inline int
DistNodalMultiVec<F>::LocalHeight() const
{
    int localHeight = 0;
    const int numLocal = localNodes.size();
    const int numDist = distNodes.size();
    for( int s=0; s<numLocal; ++s )
        localHeight += localNodes[s].Height();
    for( int s=0; s<numDist; ++s )
        localHeight += distNodes[s].LocalHeight();
    return localHeight;
}

template<typename F>
inline void
DistNodalMultiVec<F>::UpdateHeight()
{
    height_ = 0;
    for( const auto& localNode : localNodes )
        height_ += localNode.Height();
    for( const auto& distNode : distNodes )
        height_ += distNode.Height();
}

template<typename F>
inline void
DistNodalMultiVec<F>::UpdateWidth()
{
    // This should be consistent across all of the nodes
    width_ = localNodes[0].Width();
}

} // namespace cliq

#endif // ifndef CLIQ_NUMERIC_DISTNODALMULTIVEC_IMPL_HPP
