/*
   Clique: a scalable implementation of the multifrontal algorithm

   Copyright (C) 2011-2012 Jack Poulson, Lexing Ying, and 
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

namespace cliq {

// Use a simple 1d distribution where each process owns a fixed number of 
// sources:
//     if last process,  numSources - (commSize-1)*floor(numSources/commSize)
//     otherwise,        floor(numSources/commSize)
class DistGraph
{
public:
    // Construction and destruction
    DistGraph();
    DistGraph( mpi::Comm comm );
    DistGraph( int numVertices, mpi::Comm comm );
    DistGraph( int numSources, int numTargets, mpi::Comm comm );
    DistGraph( const Graph& graph );
    DistGraph( const DistGraph& graph );
    ~DistGraph();

    // High-level information
    int NumSources() const;
    int NumTargets() const;

    // Communicator-management
    void SetComm( mpi::Comm comm );
    mpi::Comm Comm() const;

    // Distribution data
    int Blocksize() const;
    int FirstLocalSource() const;
    int NumLocalSources() const;

    // Assembly-related routines
    void StartAssembly(); 
    void StopAssembly();
    void Reserve( int numLocalEdges );
    void Insert( int source, int target );
    int Capacity() const;

    // Local data
    int NumLocalEdges() const;
    int Source( int localEdge ) const;
    int Target( int localEdge ) const;
    int LocalEdgeOffset( int localSource ) const;
    int NumConnections( int localSource ) const;

    // For resizing the graph
    void Empty();
    void ResizeTo( int numVertices );
    void ResizeTo( int numSources, int numTargets );

    // For copying one graph into another
    const DistGraph& operator=( const Graph& graph );
    const DistGraph& operator=( const DistGraph& graph );

private:
    int numSources_, numTargets_;
    mpi::Comm comm_;

    int blocksize_;
    int firstLocalSource_, numLocalSources_;

    std::vector<int> sources_, targets_;

    // Helpers for local indexing
    bool assembling_, sorted_;
    std::vector<int> localEdgeOffsets_;
    void ComputeLocalEdgeOffsets();

    static bool ComparePairs
    ( const std::pair<int,int>& a, const std::pair<int,int>& b );

    void EnsureNotAssembling() const;
    void EnsureConsistentSizes() const;
    void EnsureConsistentCapacities() const;

    friend class Graph;
    template<typename F> friend class DistSparseMatrix;
    template<typename F> friend class DistSymmFrontTree;
};

} // namespace cliq