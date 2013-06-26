/*
   Copyright (c) 2009-2013, Jack Poulson, Lexing Ying,
   The University of Texas at Austin, and Stanford University
   All rights reserved.
 
   This file is part of Clique and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/

namespace cliq {

template<typename F> 
void DistLowerForwardSolve
( UnitOrNonUnit diag, const DistSymmInfo& info, 
  const DistSymmFrontTree<F>& L, DistNodalMultiVec<F>& X );

template<typename F>
void DistLowerBackwardSolve
( Orientation orientation, UnitOrNonUnit diag, const DistSymmInfo& info, 
  const DistSymmFrontTree<F>& L, DistNodalMultiVec<F>& X );

//----------------------------------------------------------------------------//
// Implementation begins here                                                 //
//----------------------------------------------------------------------------//

template<typename F> 
inline void DistLowerForwardSolve
( UnitOrNonUnit diag, const DistSymmInfo& info, 
  const DistSymmFrontTree<F>& L, DistNodalMultiVec<F>& X )
{
#ifndef RELEASE
    CallStackEntry entry("DistLowerForwardSolve");
#endif
    const int numDistNodes = info.distNodes.size();
    const int width = X.Width();
    const SymmFrontType frontType = L.frontType;
    const bool frontsAre1d = FrontsAre1d( frontType );
    if( frontType != LDL_1D && 
        frontType != LDL_SELINV_1D && 
        frontType != LDL_SELINV_2D && 
        frontType != BLOCK_LDL_2D )
        throw std::logic_error("This solve mode is not yet implemented");
#ifndef RELEASE
    const bool blockLDL = ( L.frontType == BLOCK_LDL_2D );
    if( blockLDL && diag == UNIT )
        throw std::logic_error("Unit diagonal is nonsensical for block LDL");
#endif

    // Copy the information from the local portion into the distributed leaf
    const SymmFront<F>& localRootFront = L.localFronts.back();
    const DistSymmFront<F>& distLeafFront = L.distFronts[0];
    const Grid& leafGrid = ( frontsAre1d ? distLeafFront.front1dL.Grid() 
                                         : distLeafFront.front2dL.Grid() );
    distLeafFront.work1d.LockedAttach
    ( localRootFront.work.Height(), localRootFront.work.Width(), 0,
      localRootFront.work.LockedBuffer(), localRootFront.work.LDim(), 
      leafGrid );
    
    // Perform the distributed portion of the forward solve
    for( int s=1; s<numDistNodes; ++s )
    {
        const DistSymmNodeInfo& childNode = info.distNodes[s-1];
        const DistSymmNodeInfo& node = info.distNodes[s];
        const DistSymmFront<F>& childFront = L.distFronts[s-1];
        const DistSymmFront<F>& front = L.distFronts[s];
        const Grid& childGrid = ( frontsAre1d ? childFront.front1dL.Grid()
                                              : childFront.front2dL.Grid() );
        const Grid& grid = ( frontsAre1d ? front.front1dL.Grid()
                                         : front.front2dL.Grid() );
        mpi::Comm comm = grid.VCComm();
        mpi::Comm childComm = childGrid.VCComm();
        const int commSize = mpi::CommSize( comm );
        const int childCommSize = mpi::CommSize( childComm );
        const int frontHeight = ( frontsAre1d ? front.front1dL.Height()
                                              : front.front2dL.Height() );

        // Set up a workspace
        DistMatrix<F,VC,STAR>& W = front.work1d;
        W.SetGrid( grid );
        W.ResizeTo( frontHeight, width );
        DistMatrix<F,VC,STAR> WT(grid), WB(grid);
        elem::PartitionDown
        ( W, WT,
             WB, node.size );

        // Pull in the relevant information from the RHS
        Matrix<F> localXT;
        View
        ( localXT, X.multiVec, node.localOffset1d, 0, node.localSize1d, width );
        WT.Matrix() = localXT;
        elem::MakeZeros( WB );

        // Pack our child's update
        DistMatrix<F,VC,STAR>& childW = childFront.work1d;
        const int updateSize = childW.Height()-childNode.size;
        DistMatrix<F,VC,STAR> childUpdate;
        LockedView( childUpdate, childW, childNode.size, 0, updateSize, width );
        int sendBufferSize = 0;
        std::vector<int> sendCounts(commSize), sendDispls(commSize);
        for( int proc=0; proc<commSize; ++proc )
        {
            const int sendSize = node.numChildSolveSendIndices[proc]*width;
            sendCounts[proc] = sendSize;
            sendDispls[proc] = sendBufferSize;
            sendBufferSize += sendSize;
        }
        std::vector<F> sendBuffer( sendBufferSize );

        const bool onLeft = childNode.onLeft;
        const std::vector<int>& myChildRelIndices = 
            ( onLeft ? node.leftRelIndices : node.rightRelIndices );
        const int updateColShift = childUpdate.ColShift();
        const int updateLocalHeight = childUpdate.LocalHeight();
        std::vector<int> packOffsets = sendDispls;
        for( int iChildLoc=0; iChildLoc<updateLocalHeight; ++iChildLoc )
        {
            const int iChild = updateColShift + iChildLoc*childCommSize;
            const int destRank = myChildRelIndices[iChild] % commSize;
            F* packBuf = &sendBuffer[packOffsets[destRank]];
            for( int jChild=0; jChild<width; ++jChild )
                packBuf[jChild] = childUpdate.GetLocal(iChildLoc,jChild);
            packOffsets[destRank] += width;
        }
        std::vector<int>().swap( packOffsets );
        childW.Empty();
        if( s == 1 )
            L.localFronts.back().work.Empty();

        // Set up the receive buffer
        int recvBufferSize = 0;
        std::vector<int> recvCounts(commSize), recvDispls(commSize);
        for( int proc=0; proc<commSize; ++proc )
        {
            const int recvSize = node.childSolveRecvIndices[proc].size()*width;
            recvCounts[proc] = recvSize;
            recvDispls[proc] = recvBufferSize;
            recvBufferSize += recvSize;
        }
        std::vector<F> recvBuffer( recvBufferSize );
#ifndef RELEASE
        VerifySendsAndRecvs( sendCounts, recvCounts, comm );
#endif

        // AllToAll to send and receive the child updates
        SparseAllToAll
        ( sendBuffer, sendCounts, sendDispls,
          recvBuffer, recvCounts, recvDispls, comm );
        std::vector<F>().swap( sendBuffer );
        std::vector<int>().swap( sendCounts );
        std::vector<int>().swap( sendDispls );

        // Unpack the child updates (with an Axpy)
        for( int proc=0; proc<commSize; ++proc )
        {
            const F* recvValues = &recvBuffer[recvDispls[proc]];
            const std::deque<int>& recvIndices = 
                node.childSolveRecvIndices[proc];
            for( unsigned k=0; k<recvIndices.size(); ++k )
            {
                const int iFrontLoc = recvIndices[k];
                const F* recvRow = &recvValues[k*width];
                F* WRow = W.Buffer( iFrontLoc, 0 );
                const int WLDim = W.LDim();
                for( int j=0; j<width; ++j )
                    WRow[j*WLDim] += recvRow[j];
            }
        }
        std::vector<F>().swap( recvBuffer );
        std::vector<int>().swap( recvCounts );
        std::vector<int>().swap( recvDispls );

        // Now that the RHS is set up, perform this node's solve
        if( frontType == LDL_1D )
            FrontLowerForwardSolve( diag, front.front1dL, W );
        else if( frontType == LDL_SELINV_1D )
            FrontFastLowerForwardSolve( diag, front.front1dL, W );
        else if( frontType == LDL_SELINV_2D )
            FrontFastLowerForwardSolve( diag, front.front2dL, W );
        else // frontType == BLOCK_LDL_2D
            FrontBlockLowerForwardSolve( front.front2dL, W );

        // Store this node's portion of the result
        localXT = WT.Matrix();
    }
    L.localFronts.back().work.Empty();
    L.distFronts.back().work1d.Empty();
}

template<typename F>
inline void DistLowerBackwardSolve
( Orientation orientation, UnitOrNonUnit diag, const DistSymmInfo& info, 
  const DistSymmFrontTree<F>& L, DistNodalMultiVec<F>& X )
{
#ifndef RELEASE
    CallStackEntry entry("DistLowerBackwardSolve");
#endif
    const int numDistNodes = info.distNodes.size();
    const int width = X.Width();
    const SymmFrontType frontType = L.frontType;
    const bool frontsAre1d = FrontsAre1d( frontType );
    const bool blockLDL = ( frontType == BLOCK_LDL_2D );
    if( frontType != LDL_1D && 
        frontType != LDL_SELINV_1D && 
        frontType != LDL_SELINV_2D && 
        frontType != BLOCK_LDL_2D )
        throw std::logic_error("This solve mode is not yet implemented");
#ifndef RELEASE
    if( blockLDL && diag == UNIT )
        throw std::logic_error("Unit diagonal is nonsensical for block LDL");
#endif

    // Directly operate on the root separator's portion of the right-hand sides
    Matrix<F>& localX = X.multiVec;
    const DistSymmNodeInfo& rootNode = info.distNodes.back();
    const SymmFront<F>& localRootFront = L.localFronts.back();
    if( numDistNodes == 1 )
    {
        localRootFront.work.Attach
        ( rootNode.size, width, 
          localX.Buffer(rootNode.localOffset1d,0), localX.LDim() );
        if( !blockLDL )
            FrontLowerBackwardSolve
            ( orientation, diag, localRootFront.frontL, localRootFront.work );
        else
            FrontBlockLowerBackwardSolve
            ( orientation, localRootFront.frontL, localRootFront.work );
    }
    else
    {
        const DistSymmFront<F>& rootFront = L.distFronts.back();
        const Grid& rootGrid = ( frontsAre1d ? rootFront.front1dL.Grid() 
                                             : rootFront.front2dL.Grid() );
        rootFront.work1d.Attach
        ( rootNode.size, width, 0,
          localX.Buffer(rootNode.localOffset1d,0), localX.LDim(), rootGrid );
        if( frontType == LDL_1D )
            FrontLowerBackwardSolve
            ( orientation, diag, rootFront.front1dL, rootFront.work1d );
        else if( frontType == LDL_SELINV_1D )
            FrontFastLowerBackwardSolve
            ( orientation, diag, rootFront.front1dL, rootFront.work1d );
        else if( frontType == LDL_SELINV_2D )
            FrontFastLowerBackwardSolve
            ( orientation, diag, rootFront.front2dL, rootFront.work1d );
        else
            FrontBlockLowerBackwardSolve
            ( orientation, rootFront.front2dL, rootFront.work1d );
    }

    for( int s=numDistNodes-2; s>=0; --s )
    {
        const DistSymmNodeInfo& parentNode = info.distNodes[s+1];
        const DistSymmNodeInfo& node = info.distNodes[s];
        const DistSymmFront<F>& parentFront = L.distFronts[s+1];
        const DistSymmFront<F>& front = L.distFronts[s];
        const Grid& grid = ( frontsAre1d ? front.front1dL.Grid() 
                                         : front.front2dL.Grid() );
        const Grid& parentGrid = ( frontsAre1d ? parentFront.front1dL.Grid()
                                               : parentFront.front2dL.Grid() );
        mpi::Comm comm = grid.VCComm(); 
        mpi::Comm parentComm = parentGrid.VCComm();
        const int commSize = mpi::CommSize( comm );
        const int parentCommSize = mpi::CommSize( parentComm );
        const int frontHeight = ( frontsAre1d ? front.front1dL.Height()
                                              : front.front2dL.Height() );

        // Set up a workspace
        DistMatrix<F,VC,STAR>& W = front.work1d;
        W.SetGrid( grid );
        W.ResizeTo( frontHeight, width );
        DistMatrix<F,VC,STAR> WT(grid), WB(grid);
        elem::PartitionDown
        ( W, WT,
             WB, node.size );

        // Pull in the relevant information from the RHS
        Matrix<F> localXT;
        View( localXT, localX, node.localOffset1d, 0, node.localSize1d, width );
        WT.Matrix() = localXT;

        //
        // Set the bottom from the parent
        //

        // Pack the updates using the recv approach from the forward solve
        int sendBufferSize = 0;
        std::vector<int> sendCounts(parentCommSize), sendDispls(parentCommSize);
        for( int proc=0; proc<parentCommSize; ++proc )
        {
            const int sendSize = 
                parentNode.childSolveRecvIndices[proc].size()*width;
            sendCounts[proc] = sendSize;
            sendDispls[proc] = sendBufferSize;
            sendBufferSize += sendSize;
        }
        std::vector<F> sendBuffer( sendBufferSize );

        DistMatrix<F,VC,STAR>& parentWork = parentFront.work1d;
        for( int proc=0; proc<parentCommSize; ++proc )
        {
            F* sendValues = &sendBuffer[sendDispls[proc]];
            const std::deque<int>& recvIndices = 
                parentNode.childSolveRecvIndices[proc];
            for( unsigned k=0; k<recvIndices.size(); ++k )
            {
                const int iFrontLoc = recvIndices[k];
                F* sendRow = &sendValues[k*width];
                const F* workRow = parentWork.LockedBuffer( iFrontLoc, 0 );
                const int workLDim = parentWork.LDim();
                for( int j=0; j<width; ++j )
                    sendRow[j] = workRow[j*workLDim];
            }
        }
        parentWork.Empty();

        // Set up the receive buffer
        int recvBufferSize = 0;
        std::vector<int> recvCounts(parentCommSize), recvDispls(parentCommSize);
        for( int proc=0; proc<parentCommSize; ++proc )
        {
            const int recvSize = 
                parentNode.numChildSolveSendIndices[proc]*width;
            recvCounts[proc] = recvSize;
            recvDispls[proc] = recvBufferSize;
            recvBufferSize += recvSize;
        }
        std::vector<F> recvBuffer( recvBufferSize );
#ifndef RELEASE
        VerifySendsAndRecvs( sendCounts, recvCounts, parentComm );
#endif

        // AllToAll to send and recv parent updates
        SparseAllToAll
        ( sendBuffer, sendCounts, sendDispls,
          recvBuffer, recvCounts, recvDispls, parentComm );
        std::vector<F>().swap( sendBuffer );
        std::vector<int>().swap( sendCounts );
        std::vector<int>().swap( sendDispls );

        // Unpack the updates using the send approach from the forward solve
        const bool onLeft = node.onLeft;
        const std::vector<int>& myRelIndices = 
            ( onLeft ? parentNode.leftRelIndices : parentNode.rightRelIndices );
        const int updateColShift = WB.ColShift();
        const int updateLocalHeight = WB.LocalHeight();
        for( int iUpdateLoc=0; iUpdateLoc<updateLocalHeight; ++iUpdateLoc )
        {
            const int iUpdate = updateColShift + iUpdateLoc*commSize;
            const int startRank = myRelIndices[iUpdate] % parentCommSize;
            const F* recvBuf = &recvBuffer[recvDispls[startRank]];
            for( int j=0; j<width; ++j )
                WB.SetLocal(iUpdateLoc,j,recvBuf[j]);
            recvDispls[startRank] += width;
        }
        std::vector<F>().swap( recvBuffer );
        std::vector<int>().swap( recvCounts );
        std::vector<int>().swap( recvDispls );

        // Call the custom node backward solve
        if( s > 0 )
        {
            if( frontType == LDL_1D )
                FrontLowerBackwardSolve
                ( orientation, diag, front.front1dL, W );
            else if( frontType == LDL_SELINV_1D )
                FrontFastLowerBackwardSolve
                ( orientation, diag, front.front1dL, W );
            else if( frontType == LDL_SELINV_2D )
                FrontFastLowerBackwardSolve
                ( orientation, diag, front.front2dL, W );
            else // frontType == BLOCK_LDL_2D
                FrontBlockLowerBackwardSolve
                ( orientation, front.front2dL, front.work1d );
        }
        else
        {
            View( localRootFront.work, W.Matrix() );
            if( !blockLDL )
                FrontLowerBackwardSolve
                ( orientation, diag, localRootFront.frontL, 
                  localRootFront.work );
            else
                FrontBlockLowerBackwardSolve
                ( orientation, localRootFront.frontL, localRootFront.work );
        }

        // Store this node's portion of the result
        localXT = WT.Matrix();
    }
}

} // namespace cliq
