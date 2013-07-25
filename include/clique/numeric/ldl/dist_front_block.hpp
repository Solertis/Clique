/*
   Copyright (c) 2009-2013, Jack Poulson, Lexing Ying,
   The University of Texas at Austin, and Stanford University
   All rights reserved.
 
   This file is part of Clique and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef CLIQ_NUMERIC_LDL_DISTFRONTBLOCK_HPP
#define CLIQ_NUMERIC_LDL_DISTFRONTBLOCK_HPP

namespace cliq {

template<typename F> 
void FrontBlockLDL
( Orientation orientation, DistMatrix<F>& AL, DistMatrix<F>& ABR );

//----------------------------------------------------------------------------//
// Implementation begins here                                                 //
//----------------------------------------------------------------------------//

template<typename F> 
inline void FrontBlockLDL
( Orientation orientation, DistMatrix<F>& AL, DistMatrix<F>& ABR )
{
#ifndef RELEASE
    CallStackEntry entry("internal::FrontBlockLDL");
#endif
    const Grid& g = AL.Grid();
    DistMatrix<F> ATL(g),
                  ABL(g);
    PartitionDown
    ( AL, ATL,
          ABL, AL.Width() );

    // Make a copy of the original contents of ABL
    DistMatrix<F> BBL( ABL );

    // Call the standard routine
    FrontLDL( orientation, AL, ABR );

    // Copy the original contents of ABL back
    ABL = BBL;

    // Overwrite ATL with inv(L D L^[T/H]) = L^[-T/H] D^{-1} L^{-1}
    elem::TriangularInverse( LOWER, UNIT, ATL );
    elem::Trdtrmm( orientation, LOWER, ATL );
    elem::MakeTrapezoidal( LOWER, ATL );
    if( orientation == TRANSPOSE )
    {
        DistMatrix<F> ATLTrans( g );
        elem::Transpose( ATL, ATLTrans );
        elem::MakeTrapezoidal( UPPER, ATLTrans, 1 );
        elem::Axpy( F(1), ATLTrans, ATL );
    }
    else
    {
        DistMatrix<F> ATLAdj( g );
        elem::Adjoint( ATL, ATLAdj );
        elem::MakeTrapezoidal( UPPER, ATLAdj, 1 );
        elem::Axpy( F(1), ATLAdj, ATL );
    }
}

} // namespace cliq

#endif // ifndef CLIQ_NUMERIC_LDL_DISTFRONTBLOCK_HPP
