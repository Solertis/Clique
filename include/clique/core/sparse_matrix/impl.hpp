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
#ifndef CLIQ_CORE_SPARSEMATRIX_IMPL_HPP
#define CLIQ_CORE_SPARSEMATRIX_IMPL_HPP

namespace cliq {

template<typename T>
inline
SparseMatrix<T>::SparseMatrix()
{ }

template<typename T>
inline
SparseMatrix<T>::SparseMatrix( int height )
: graph_(height)
{ }

template<typename T>
inline 
SparseMatrix<T>::SparseMatrix( int height, int width )
: graph_(height,width)
{ }

template<typename T>
inline
SparseMatrix<T>::SparseMatrix( const SparseMatrix<T>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("SparseMatrix::SparseMatrix"))
    if( &A != this )
        *this = A;
    else
        LogicError("Tried to construct sparse matrix with itself");
}

template<typename T>
inline
SparseMatrix<T>::SparseMatrix( const DistSparseMatrix<T>& A )
{ 
    DEBUG_ONLY(CallStackEntry cse("SparseMatrix::SparseMatrix"))
    *this = A;
}

template<typename T>
inline
SparseMatrix<T>::~SparseMatrix()
{ }

template<typename T>
inline int 
SparseMatrix<T>::Height() const
{ return graph_.NumSources(); }

template<typename T>
inline int 
SparseMatrix<T>::Width() const
{ return graph_.NumTargets(); }

template<typename T>
inline cliq::Graph& 
SparseMatrix<T>::Graph()
{ return graph_; }

template<typename T>
inline const cliq::Graph& 
SparseMatrix<T>::LockedGraph() const
{ return graph_; }

template<typename T>
inline int
SparseMatrix<T>::NumEntries() const
{
    DEBUG_ONLY(
        CallStackEntry cse("SparseMatrix::NumEntries");
        EnsureConsistentSizes();
    )
    return graph_.NumEdges();
}

template<typename T>
inline int
SparseMatrix<T>::Capacity() const
{
    DEBUG_ONLY(
        CallStackEntry cse("SparseMatrix::Capacity");
        EnsureConsistentSizes();
        EnsureConsistentCapacities();
    )
    return graph_.Capacity();
}

template<typename T>
inline int
SparseMatrix<T>::Row( int index ) const
{ 
    DEBUG_ONLY(CallStackEntry cse("SparseMatrix::Row"))
    return graph_.Source( index );
}

template<typename T>
inline int
SparseMatrix<T>::Col( int index ) const
{ 
    DEBUG_ONLY(CallStackEntry cse("SparseMatrix::Col"))
    return graph_.Target( index );
}

template<typename T>
inline int
SparseMatrix<T>::EntryOffset( int row ) const
{
    DEBUG_ONLY(CallStackEntry cse("SparseMatrix::EntryOffset"))
    return graph_.EdgeOffset( row );
}

template<typename T>
inline int
SparseMatrix<T>::NumConnections( int row ) const
{
    DEBUG_ONLY(CallStackEntry cse("SparseMatrix::NumConnections"))
    return graph_.NumConnections( row );
}

template<typename T>
inline T
SparseMatrix<T>::Value( int index ) const
{ 
    DEBUG_ONLY(
        CallStackEntry cse("SparseMatrix::Value");
        if( index < 0 || index >= vals_.size() )
            LogicError("Entry number out of bounds");
    )
    return vals_[index];
}

template<typename T>
inline int*
SparseMatrix<T>::SourceBuffer()
{ return graph_.SourceBuffer(); }

template<typename T>
inline int*
SparseMatrix<T>::TargetBuffer()
{ return graph_.TargetBuffer(); }

template<typename T>
inline T*
SparseMatrix<T>::ValueBuffer()
{ return &vals_[0]; }

template<typename T>
inline const int*
SparseMatrix<T>::LockedSourceBuffer() const
{ return graph_.LockedSourceBuffer(); }

template<typename T>
inline const int*
SparseMatrix<T>::LockedTargetBuffer() const
{ return graph_.LockedTargetBuffer(); }

template<typename T>
inline const T*
SparseMatrix<T>::LockedValueBuffer() const
{ return &vals_[0]; }

template<typename T>
inline bool
SparseMatrix<T>::CompareEntries( const Entry<T>& a, const Entry<T>& b )
{ return a.i < b.i || (a.i == b.i && a.j < b.j); }

template<typename T>
inline void
SparseMatrix<T>::StartAssembly()
{
    DEBUG_ONLY(CallStackEntry cse("SparseMatrix::StartAssembly"))
    graph_.EnsureNotAssembling();
    graph_.assembling_ = true;
}

template<typename T>
inline void
SparseMatrix<T>::StopAssembly()
{
    DEBUG_ONLY(CallStackEntry cse("SparseMatrix::StopAssembly"))
    if( !graph_.assembling_ )
        LogicError("Cannot stop assembly without starting");
    graph_.assembling_ = false;

    // Ensure that the connection pairs are sorted
    if( !graph_.sorted_ )
    {
        const int numEntries = vals_.size();
        std::vector<Entry<T>> entries( numEntries );
        for( int s=0; s<numEntries; ++s )
        {
            entries[s].i = graph_.sources_[s];
            entries[s].j = graph_.targets_[s];
            entries[s].value = vals_[s];
        }
        std::sort( entries.begin(), entries.end(), CompareEntries );

        // Compress out duplicates
        int lastUnique=0;
        for( int s=1; s<numEntries; ++s )
        {
            if( entries[s].i != entries[lastUnique].i ||
                entries[s].j != entries[lastUnique].j )
            {
                ++lastUnique;
                entries[lastUnique].i = entries[s].i;
                entries[lastUnique].j = entries[s].j;
                entries[lastUnique].value = entries[s].value;
            }
            else
                entries[lastUnique].value += entries[s].value;
        }
        const int numUnique = lastUnique+1;

        graph_.sources_.resize( numUnique );
        graph_.targets_.resize( numUnique );
        vals_.resize( numUnique );
        for( int s=0; s<numUnique; ++s )
        {
            graph_.sources_[s] = entries[s].i;
            graph_.targets_[s] = entries[s].j;
            vals_[s] = entries[s].value;
        }
    }
    graph_.ComputeEdgeOffsets();
}

template<typename T>
inline void
SparseMatrix<T>::Reserve( int numEntries )
{ 
    graph_.Reserve( numEntries );
    vals_.reserve( numEntries );
}

template<typename T>
inline void
SparseMatrix<T>::Update( int row, int col, T value )
{
    DEBUG_ONLY(
        CallStackEntry cse("SparseMatrix::Update");
        EnsureConsistentSizes();
    )
    graph_.Insert( row, col );
    vals_.push_back( value );
}

template<typename T>
inline void
SparseMatrix<T>::Empty()
{
    graph_.Empty();
    SwapClear( vals_ );
}

template<typename T>
inline void
SparseMatrix<T>::Resize( int height, int width )
{
    graph_.Resize( height, width );
    SwapClear( vals_ );
}

template<typename T>
inline const SparseMatrix<T>&
SparseMatrix<T>::operator=( const SparseMatrix<T>& A )
{
    DEBUG_ONLY(CallStackEntry cse("SparseMatrix::operator="))
    graph_ = A.graph_;
    vals_ = A.vals_;
    return *this;
}

template<typename T>
inline const SparseMatrix<T>&
SparseMatrix<T>::operator=( const DistSparseMatrix<T>& A )
{
    DEBUG_ONLY(CallStackEntry cse("SparseMatrix::operator="))
    mpi::Comm comm = A.Comm();
    const int commSize = mpi::Size( comm );
    if( commSize != 1 )
        LogicError("Can not yet construct from distributed sparse matrix");

    graph_ = A.graph_;
    vals_ = A.vals_;
    return *this;
}

template<typename T>
inline void
SparseMatrix<T>::EnsureConsistentSizes() const
{ 
    graph_.EnsureConsistentSizes();
    if( graph_.NumEdges() != vals_.size() )
        LogicError("Inconsistent sparsity sizes");
}

template<typename T>
inline void
SparseMatrix<T>::EnsureConsistentCapacities() const
{ 
    graph_.EnsureConsistentCapacities();
    if( graph_.Capacity() != vals_.capacity() )
        LogicError("Inconsistent sparsity capacities");
}

} // namespace cliq

#endif // ifndef CLIQ_CORE_SPARSEMATRIX_IMPL_HPP
