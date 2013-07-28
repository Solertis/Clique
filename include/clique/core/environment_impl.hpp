/*
   Copyright (c) 2009-2013, Jack Poulson, Lexing Ying,
   The University of Texas at Austin, and Stanford University
   All rights reserved.
 
   This file is part of Clique and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef CLIQ_CORE_ENVIRONMENT_IMPL_HPP
#define CLIQ_CORE_ENVIRONMENT_IMPL_HPP

#include "clique/core/environment_decl.hpp"

namespace cliq {

// For getting the MPI argument instance (for internal usage)
inline void Args::HandleVersion( std::ostream& os ) const
{
    std::string version = "--version";
    char** arg = std::find( argv_, argv_+argc_, version );
    const bool foundVersion = ( arg != argv_+argc_ );
    if( foundVersion )
    {
        if( mpi::WorldRank() == 0 )
            PrintVersion();
        throw elem::ArgException();
    }
}

inline void Args::HandleBuild( std::ostream& os ) const
{
    std::string build = "--build";
    char** arg = std::find( argv_, argv_+argc_, build );
    const bool foundBuild = ( arg != argv_+argc_ );
    if( foundBuild )
    {
        if( mpi::WorldRank() == 0 )
        {
            PrintVersion();
            PrintConfig();
            PrintCCompilerInfo();
            PrintCxxCompilerInfo();
        }
        throw elem::ArgException();
    }
}

// For processing command-line arguments
template<typename T>
inline T
Input( std::string name, std::string desc )
{ return GetArgs().Input<T>( name, desc ); }

template<typename T>
inline T
Input( std::string name, std::string desc, T defaultVal )
{ return GetArgs().Input( name, desc, defaultVal ); }

inline void
ProcessInput()
{ GetArgs().Process(); }

inline void
PrintInputReport()
{ GetArgs().PrintReport(); }

template<typename T>
inline bool IsSorted( const std::vector<T>& x )
{
    const int vecLength = x.size();
    for( int i=1; i<vecLength; ++i )
    {
        if( x[i] < x[i-1] )
            return false;
    }
    return true;
}

// While is_strictly_sorted exists in Boost, it does not exist in the STL (yet)
template<typename T>
inline bool IsStrictlySorted( const std::vector<T>& x )
{
    const int vecLength = x.size();
    for( int i=1; i<vecLength; ++i )
    {
        if( x[i] <= x[i-1] )
            return false;
    }
    return true;
}

inline void Union
( std::vector<int>& both, 
  const std::vector<int>& first, const std::vector<int>& second )
{
    both.resize( first.size()+second.size() );
    std::vector<int>::iterator it = std::set_union
      ( first.begin(), first.end(), second.begin(), second.end(), 
        both.begin() );
    both.resize( int(it-both.begin()) );
}

inline void RelativeIndices
( std::vector<int>& relInds, 
  const std::vector<int>& sub, const std::vector<int>& full )
{
    const int numSub = sub.size();
    relInds.resize( numSub );
    std::vector<int>::const_iterator it = full.begin();
    for( int i=0; i<numSub; ++i )
    {
        const int index = sub[i];
        it = std::lower_bound( it, full.end(), index );
#ifndef RELEASE
        if( it == full.end() )
            throw std::logic_error("Index was not found");
#endif
        relInds[i] = int(it-full.begin());
    }
}

inline int
RowToProcess( int i, int blocksize, int commSize )
{
    if( blocksize > 0 )
        return std::min( i/blocksize, commSize-1 );
    else
        return commSize-1;
}

inline int
Find
( const std::vector<int>& sortedInds, int index, std::string msg )
{
#ifndef RELEASE
    CallStackEntry entry("Find");
#endif
    std::vector<int>::const_iterator vecIt;
    vecIt = std::lower_bound( sortedInds.begin(), sortedInds.end(), index );
#ifndef RELEASE
    if( vecIt == sortedInds.end() )
        throw std::logic_error( msg.c_str() );
#endif
    return vecIt - sortedInds.begin();
}

inline void
VerifySendsAndRecvs
( const std::vector<int>& sendCounts,
  const std::vector<int>& recvCounts, mpi::Comm comm )
{
    const int commSize = mpi::CommSize( comm );
    std::vector<int> actualRecvCounts(commSize);
    mpi::AllToAll
    ( &sendCounts[0],       1,
      &actualRecvCounts[0], 1, comm );
    for( int proc=0; proc<commSize; ++proc )
    {
        if( actualRecvCounts[proc] != recvCounts[proc] )
        {
            std::ostringstream msg;
            msg << "Expected recv count of " << recvCounts[proc]
                << " but recv'd " << actualRecvCounts[proc]
                << " from process " << proc << "\n";
            throw std::logic_error( msg.str().c_str() );
        }
    }
}

template<typename T>
inline void
SparseAllToAll
( const std::vector<T>& sendBuffer,
  const std::vector<int>& sendCounts, const std::vector<int>& sendDispls,
        std::vector<T>& recvBuffer,
  const std::vector<int>& recvCounts, const std::vector<int>& recvDispls,
        mpi::Comm comm )
{
#ifdef USE_CUSTOM_ALLTOALLV
    const int commSize = mpi::CommSize( comm );
    int numSends=0,numRecvs=0;
    for( int proc=0; proc<commSize; ++proc )
    {
        if( sendCounts[proc] != 0 )
            ++numSends;
        if( recvCounts[proc] != 0 )
            ++numRecvs;
    }
    std::vector<mpi::Status> statuses(numSends+numRecvs);
    std::vector<mpi::Request> requests(numSends+numRecvs);
    int rCount=0;
    for( int proc=0; proc<commSize; ++proc )
    {
        int count = recvCounts[proc];
        int displ = recvDispls[proc];
        if( count != 0 )
            mpi::IRecv
            ( &recvBuffer[displ], count, proc, 0, comm,
              requests[rCount++] );
    }
#ifdef BARRIER_IN_ALLTOALLV
    // This should help ensure that recvs are posted before the sends
    mpi::Barrier( comm );
#endif
    for( int proc=0; proc<commSize; ++proc )
    {
        int count = sendCounts[proc];
        int displ = sendDispls[proc];
        if( count != 0 )
            mpi::ISend
            ( &sendBuffer[displ], count, proc, 0, comm,
              requests[rCount++] );
    }
    mpi::WaitAll( numSends+numRecvs, &requests[0], &statuses[0] );
#else
    mpi::AllToAll
    ( &sendBuffer[0], &sendCounts[0], &sendDispls[0],
      &recvBuffer[0], &recvCounts[0], &recvDispls[0], comm );
#endif
}

} // namespace cliq

#endif // ifndef CLIQ_CORE_ENVIRONMENT_IMPL_HPP
