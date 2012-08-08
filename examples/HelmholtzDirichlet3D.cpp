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
#include "clique.hpp"
using namespace cliq;

void Usage()
{
    std::cout
      << "HelmholtzDirichlet3D <nx> <ny> <nz> <omega> <damping> "
      << "[analytic=true] [sequential=true] [cutoff=128] \n"
      << "[numDistSeps=1] [numSeqSeps=1]\n"
      << "  nx: first dimension of nx x ny x nz mesh\n"
      << "  ny: second dimension of nx x ny x nz mesh\n"
      << "  nz: third dimension of nx x ny x nz mesh\n"
      << "  omega: frequency of problem in radians per second\n"
      << "  damping: imaginary damping in radians per second\n"
      << "  analytic: if nonzero, use an analytical reordering\n"
      << "  sequential: if nonzero, then run a sequential symbolic reordering\n"
      << "  cutoff: maximum size of leaf node\n"
      << "  numDistSeps: number of distributed separators to try\n"
      << "  numSeqSeps: number of sequential separators to try\n"
      << std::endl;
}

int
main( int argc, char* argv[] )
{
    cliq::Initialize( argc, argv );
    mpi::Comm comm = mpi::COMM_WORLD;
    const int commRank = mpi::CommRank( comm );
    typedef double R;
    typedef Complex<R> C;

    if( argc < 6 )
    {
        if( commRank == 0 )
            Usage();
        cliq::Finalize();
        return 0;
    }
    const int nx = atoi( argv[1] );
    const int ny = atoi( argv[2] );
    const int nz = atoi( argv[3] );
    const double omega = atof( argv[4] );
    const double damping = atof( argv[5] );
    const bool analytic = ( argc >= 7 ? atoi( argv[6] ) : true );
    const bool sequential = ( argc >= 8 ? atoi( argv[7] ) : true );
    const int cutoff = ( argc >= 9 ? atoi( argv[8] ) : 128 );
    const int numDistSeps = ( argc >= 10 ? atoi( argv[9] ) : 1 );
    const int numSeqSeps = ( argc >= 11 ? atoi( argv[10] ) : 1 );

    try
    {
        const int N = nx*ny*nz;
        DistSparseMatrix<C> A( N, comm );
        C dampedOmega( omega, damping );
        const double h1Inv = nx;
        const double h2Inv = ny;
        const double h3Inv = nz;
        const double h1InvSquared = h1Inv*h1Inv;
        const double h2InvSquared = h2Inv*h2Inv;
        const double h3InvSquared = h3Inv*h3Inv;
        const C mainTerm = 
            2*(h1InvSquared+h2InvSquared+h3InvSquared) - 
            dampedOmega*dampedOmega;

        // Fill our portion of the 3D Helmholtz operator over the unit-square 
        // using a nx x ny x nz 7-point stencil in natural ordering: 
        // (x,y,z) at x + y*nx + z*nx*ny
        if( commRank == 0 )
        {
            std::cout << "Filling local portion of matrix...";
            std::cout.flush();
        }
        const double fillStart = mpi::Time();
        const int firstLocalRow = A.FirstLocalRow();
        const int localHeight = A.LocalHeight();
        A.StartAssembly();
        A.Reserve( 7*localHeight );
        for( int iLocal=0; iLocal<localHeight; ++iLocal )
        {
            const int i = firstLocalRow + iLocal;
            const int x = i % nx;
            const int y = (i/nx) % ny;
            const int z = i/(nx*ny);

            A.Update( i, i, mainTerm );
            if( x != 0 )
                A.Update( i, i-1, -h1InvSquared );
            if( x != nx-1 )
                A.Update( i, i+1, -h1InvSquared );
            if( y != 0 )
                A.Update( i, i-nx, -h2InvSquared );
            if( y != ny-1 )
                A.Update( i, i+nx, -h2InvSquared );
            if( z != 0 )
                A.Update( i, i-nx*ny, -h3InvSquared );
            if( z != nz-1 )
                A.Update( i, i+nx*ny, -h3InvSquared );
        } 
        A.StopAssembly();
        mpi::Barrier( comm );
        const double fillStop =  mpi::Time();
        if( commRank == 0 )
            std::cout << "done, " << fillStop-fillStart << " seconds" 
                      << std::endl;

        if( commRank == 0 )
        {
            std::cout << "Generating random vector x and forming y := A x...";
            std::cout.flush();
        }
        const double multiplyStart = mpi::Time();
        DistVector<C> x( N, comm ), y( N, comm );
        MakeUniform( x );
        MakeZeros( y );
        Multiply( (C)1., A, x, (C)0., y );
        const double yOrigNorm = Norm( y );
        mpi::Barrier( comm );
        const double multiplyStop = mpi::Time();
        if( commRank == 0 )
            std::cout << "done, " << multiplyStop-multiplyStart << " seconds"
                      << std::endl;

        if( commRank == 0 )
        {
            std::cout << "Running nested dissection...";
            std::cout.flush();
        }
        const double nestedStart = mpi::Time();
        const DistGraph& graph = A.Graph();
        DistSymmInfo info;
        DistSeparatorTree sepTree;
        DistMap map, inverseMap;
        if( analytic )
            NaturalNestedDissection
            ( nx, ny, nz, graph, map, sepTree, info, cutoff );
        else
            NestedDissection
            ( graph, map, sepTree, info, 
              sequential, numDistSeps, numSeqSeps, cutoff );
        map.FormInverse( inverseMap );
        mpi::Barrier( comm );
        const double nestedStop = mpi::Time();
        if( commRank == 0 )
            std::cout << "done, " << nestedStop-nestedStart << " seconds"
                      << std::endl;

        const int rootSepSize = info.distNodes.back().size;
        if( commRank == 0 )
        {
            const int numDistNodes = info.distNodes.size();
            const int numLocalNodes = info.localNodes.size();
            std::cout << "\n"
                      << "On the root process:\n"
                      << "-----------------------------------------\n"
                      << numLocalNodes << " local nodes\n"
                      << numDistNodes  << " distributed nodes\n"
                      << rootSepSize << " vertices in root separator\n"
                      << std::endl;
        }

        if( commRank == 0 )
        {
            std::cout << "Building DistSymmFrontTree...";
            std::cout.flush();
        }
        mpi::Barrier( comm );
        const double buildStart = mpi::Time();
        DistSymmFrontTree<C> frontTree( TRANSPOSE, A, map, sepTree, info );
        mpi::Barrier( comm );
        const double buildStop = mpi::Time();
        if( commRank == 0 )
            std::cout << "done, " << buildStop-buildStart << " seconds"
                      << std::endl;

        if( commRank == 0 )
        {
            std::cout << "Running block LDL^T...";
            std::cout.flush();
        }
        mpi::Barrier( comm );
        const double ldlStart = mpi::Time();
        LDL( info, frontTree, BLOCK_LDL_2D );
        mpi::Barrier( comm );
        const double ldlStop = mpi::Time();
        if( commRank == 0 )
            std::cout << "done, " << ldlStop-ldlStart << " seconds" 
                      << std::endl;

        if( commRank == 0 )
        {
            std::cout << "Computing SVD of the largest off-diagonal block of "
                         "numerical Green's function on root separator...";
            std::cout.flush();
        }
        const double svdStart = mpi::Time();
        const DistMatrix<C>& rootFront = frontTree.distFronts.back().front2dL;
        const Grid& rootGrid = rootFront.Grid();
        const int lowerHalf = rootSepSize/2;
        const int upperHalf = rootSepSize - lowerHalf;
        if( commRank == 0 )
            std::cout << "lowerHalf=" << lowerHalf 
                      << ", upperHalf=" << upperHalf << std::endl;
        DistMatrix<C> offDiagBlock;
        offDiagBlock.LockedView
        ( rootFront, lowerHalf, 0, upperHalf, lowerHalf );
        DistMatrix<C> offDiagBlockCopy( offDiagBlock );
        DistMatrix<R,VR,STAR> singVals_VR_STAR( rootGrid );
        elem::SingularValues( offDiagBlockCopy, singVals_VR_STAR );
        const R twoNorm = elem::Norm( singVals_VR_STAR, elem::INFINITY_NORM );
        DistMatrix<R,STAR,STAR> singVals( singVals_VR_STAR );
        int numRankE4=lowerHalf, numRankE6=lowerHalf, numRankE8=lowerHalf;
        for( int j=0; j<lowerHalf; ++j )
        {
            if( singVals.GetLocal(j,0) <= twoNorm*1e-4 )
            {
                numRankE4 = j;
                break;
            }
        }
        for( int j=0; j<lowerHalf; ++j )
        {
            if( singVals.GetLocal(j,0) <= twoNorm*1e-6 )
            {
                numRankE6 = j;
                break;
            }
        }
        for( int j=0; j<lowerHalf; ++j )
        {
            if( singVals.GetLocal(j,0) <= twoNorm*1e-8 )
            {
                numRankE8 = j;
                break;
            }
        }
        mpi::Barrier( comm );
        const double svdStop = mpi::Time();
        if( commRank == 0 )
        {
            std::cout << "done, " << svdStop-svdStart << " seconds\n"
                      << "  two norm=" << twoNorm << "\n"
                      << "      rank (1e-4)=" << numRankE4 << "/" << lowerHalf
                      << "      rank (1e-6)=" << numRankE6 << "/" << lowerHalf
                      << "      rank (1e-8)=" << numRankE8 << "/" << lowerHalf
                      << std::endl;
        }

        if( commRank == 0 )
        {
            std::cout << "Solving against y...";
            std::cout.flush();
        }
        const double solveStart = mpi::Time();
        DistNodalVector<C> yNodal;
        yNodal.Pull( inverseMap, info, y );
        Solve( info, frontTree, yNodal.localVec );
        yNodal.Push( inverseMap, info, y );
        mpi::Barrier( comm );
        const double solveStop = mpi::Time();
        if( commRank == 0 )
            std::cout << "done, " << solveStop-solveStart << " seconds"
                      << std::endl;

        if( commRank == 0 )
            std::cout << "Checking error in computed solution..." << std::endl;
        const double xNorm = Norm( x );
        const double yNorm = Norm( y );
        Axpy( (C)-1., x, y );
        const double errorNorm = Norm( y );
        if( commRank == 0 )
        {
            std::cout << "|| x     ||_2 = " << xNorm << "\n"
                      << "|| xComp ||_2 = " << yNorm << "\n"
                      << "|| A x   ||_2 = " << yOrigNorm << "\n"
                      << "|| error ||_2 / || x ||_2 = " 
                      << errorNorm/xNorm << "\n"
                      << "|| error ||_2 / || A x ||_2 = " 
                      << errorNorm/yOrigNorm
                      << std::endl;
        }
    }
    catch( std::exception& e )
    {
#ifndef RELEASE
        elem::DumpCallStack();
        cliq::DumpCallStack();
#endif
        std::ostringstream msg;
        msg << "Process " << commRank << " caught message:\n"
            << e.what() << "\n";
        std::cerr << msg.str() << std::endl;
    }

    cliq::Finalize();
    return 0;
}