/*
   Copyright (C) 2011-2012 Jack Poulson, Lexing Ying, and 
   The University of Texas at Austin
 
   This file is part of Clique and is under the GNU General Public License,
   which can be found in the LICENSE file in the root directory, or at 
   <http://www.gnu.org/licenses/>.
*/

namespace cliq {

template<typename F>
void FrontFastLowerForwardSolve
( UnitOrNonUnit diag, DistMatrix<F,VC,STAR>& L, DistMatrix<F,VC,STAR>& X );

template<typename F>
void FrontFastLowerForwardSolve
( UnitOrNonUnit diag, DistMatrix<F>& L, DistMatrix<F,VC,STAR>& X );

template<typename F>
void FrontFastLowerBackwardSolve
( Orientation orientation, UnitOrNonUnit diag, 
  DistMatrix<F,VC,STAR>& L, DistMatrix<F,VC,STAR>& X );

template<typename F>
void FrontFastLowerBackwardSolve
( Orientation orientation, UnitOrNonUnit diag, 
  DistMatrix<F>& L, DistMatrix<F,VC,STAR>& X );

//----------------------------------------------------------------------------//
// Implementation begins here                                                 //
//----------------------------------------------------------------------------//

template<typename F>
inline void FrontFastLowerForwardSolve
( UnitOrNonUnit diag, DistMatrix<F,VC,STAR>& L, DistMatrix<F,VC,STAR>& X )
{
#ifndef RELEASE
    CallStackEntry entry("FrontFastLowerForwardSolve");
    if( L.Grid() != X.Grid() )
        throw std::logic_error
        ("L and X must be distributed over the same grid");
    if( L.Height() < L.Width() || L.Height() != X.Height() )
    {
        std::ostringstream msg;
        msg << "Nonconformal solve:\n"
            << "  L ~ " << L.Height() << " x " << L.Width() << "\n"
            << "  X ~ " << X.Height() << " x " << X.Width() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
    if( L.ColAlignment() != X.ColAlignment() )
        throw std::logic_error("L and X are assumed to be aligned");
#endif
    const Grid& g = L.Grid();
    const int commRank = g.VCRank();
    const int commSize = g.Size();
    if( commSize == 1 )
    {
        FrontLowerForwardSolve( diag, L.LockedMatrix(), X.Matrix() );
        return;
    }

    // Separate the top and bottom portions of X and L
    const int snSize = L.Width();
    DistMatrix<F,VC,STAR> LT(g),
                          LB(g);
    PartitionDown
    ( L, LT, 
         LB, snSize );
    DistMatrix<F,VC,STAR> XT(g),
                          XB(g);
    PartitionDown
    ( X, XT,
         XB, snSize );

    const int localTopHeight = LT.LocalHeight();
    std::vector<F> localDiag;
    if( diag == UNIT )
    {
        // Extract the diagonal of the top triangle and replace it with ones
        localDiag.resize( localTopHeight );
        F* LTBuffer = LT.Buffer();
        const int LTLDim = LT.LDim();
        for( int iLocal=0; iLocal<localTopHeight; ++iLocal )
        {
            const int i = commRank + iLocal*commSize;
            localDiag[iLocal] = LTBuffer[iLocal+i*LTLDim];
            LTBuffer[iLocal+i*LTLDim] = 1;
        }
    }

    // Get a copy of all of XT
    DistMatrix<F,STAR,STAR> XT_STAR_STAR( XT );

    // XT := LT XT
    elem::LocalGemm( NORMAL, NORMAL, F(1), LT, XT_STAR_STAR, F(0), XT );

    if( diag == UNIT )
    {
        // Put the diagonal back
        F* LTBuffer = LT.Buffer();
        const int LTLDim = LT.LDim();
        for( int iLocal=0; iLocal<localTopHeight; ++iLocal )
        {
            const int i = commRank + iLocal*commSize;
            LTBuffer[iLocal+i*LTLDim] = localDiag[iLocal];
        }
    }

    if( LB.Height() != 0 )
    {
        // Gather all of XT again
        XT_STAR_STAR = XT;

        // XB := XB - LB XT
        elem::LocalGemm( NORMAL, NORMAL, F(-1), LB, XT_STAR_STAR, F(1), XB );
    }
}

template<typename F>
inline void FrontFastLowerForwardSolve
( UnitOrNonUnit diag, DistMatrix<F>& L, DistMatrix<F,VC,STAR>& X )
{
#ifndef RELEASE
    CallStackEntry entry("FrontFastLowerForwardSolve");
    if( L.Grid() != X.Grid() )
        throw std::logic_error
        ("L and X must be distributed over the same grid");
    if( L.Height() < L.Width() || L.Height() != X.Height() )
    {
        std::ostringstream msg;
        msg << "Nonconformal solve:\n"
            << "  L ~ " << L.Height() << " x " << L.Width() << "\n"
            << "  X ~ " << X.Height() << " x " << X.Width() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
#endif
    const Grid& g = L.Grid();
    const int commSize = g.Size();
    if( commSize == 1 )
    {
        FrontLowerForwardSolve( diag, L.LockedMatrix(), X.Matrix() );
        return;
    }

    // Separate the top and bottom portions of X and L
    const int snSize = L.Width();
    DistMatrix<F> LT(g),
                  LB(g);
    PartitionDown
    ( L, LT, 
         LB, snSize );
    DistMatrix<F,VC,STAR> XT(g),
                          XB(g);
    PartitionDown
    ( X, XT,
         XB, snSize );

    DistMatrix<F,MD,STAR> dTOrig(g), dTReplacement(g);
    if( diag == UNIT )
    {
        // Extract the diagonal of the top triangle and replace it with ones
        LT.GetDiagonal( dTOrig );
        const int localHeight = dTOrig.LocalHeight();
        dTReplacement.AlignWith( dTOrig );
        dTReplacement.ResizeTo( dTOrig.Height(), 1 );
        for( int iLocal=0; iLocal<localHeight; ++iLocal )
            dTReplacement.SetLocal( iLocal, 0, F(1) );
        LT.SetDiagonal( dTReplacement );
    }

    // Get ready for the local multiply
    DistMatrix<F,MR,STAR> XT_MR_STAR(g);
    XT_MR_STAR.AlignWith( LT );

    {
        // ZT[MC,* ] := LT[MC,MR] XT[MR,* ], 
        DistMatrix<F,MC,STAR> ZT_MC_STAR(g);
        ZT_MC_STAR.AlignWith( LT );
        Zeros( ZT_MC_STAR, LT.Height(), XT.Width() );
        XT_MR_STAR = XT;
        elem::LocalGemm
        ( NORMAL, NORMAL, F(1), LT, XT_MR_STAR, F(0), ZT_MC_STAR );

        // XT[VC,* ].SumScatterFrom( ZT[MC,* ] )
        XT.SumScatterFrom( ZT_MC_STAR );
    }

    if( diag == UNIT )
        LT.SetDiagonal( dTOrig );

    if( LB.Height() != 0 )
    {
        // Set up for the local multiply
        XT_MR_STAR = XT;

        // ZB[MC,* ] := LB[MC,MR] XT[MR,* ]
        DistMatrix<F,MC,STAR> ZB_MC_STAR(g);
        ZB_MC_STAR.AlignWith( LB );
        ZB_MC_STAR.ResizeTo( LB.Height(), XT.Width() );
        elem::LocalGemm
        ( NORMAL, NORMAL, F(-1), LB, XT_MR_STAR, F(0), ZB_MC_STAR );

        // XB[VC,* ] += ZB[MC,* ]
        XB.SumScatterUpdate( F(1), ZB_MC_STAR );
    }
}

template<typename F>
inline void FrontFastLowerBackwardSolve
( Orientation orientation, UnitOrNonUnit diag, 
  DistMatrix<F,VC,STAR>& L, DistMatrix<F,VC,STAR>& X )
{
#ifndef RELEASE
    CallStackEntry entry("FrontFastLowerBackwardSolve");
    if( L.Grid() != X.Grid() )
        throw std::logic_error
        ("L and X must be distributed over the same grid");
    if( L.Height() < L.Width() || L.Height() != X.Height() )
    {
        std::ostringstream msg;
        msg << "Nonconformal solve:\n"
            << "  L ~ " << L.Height() << " x " << L.Width() << "\n"
            << "  X ~ " << X.Height() << " x " << X.Width() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
    if( L.ColAlignment() != X.ColAlignment() )
        throw std::logic_error("L and X are assumed to be aligned");
    if( orientation == NORMAL )
        throw std::logic_error("This solve must be (conjugate-)transposed");
#endif
    const Grid& g = L.Grid();
    const int commSize = g.Size();
    const int commRank = g.VCRank();
    if( commSize == 1 )
    {
        FrontLowerBackwardSolve
        ( orientation, diag, L.LockedMatrix(), X.Matrix() );
        return;
    }

    const int snSize = L.Width();
    DistMatrix<F,VC,STAR> LT(g),
                          LB(g);
    elem::PartitionDown
    ( L, LT,
         LB, snSize );
    DistMatrix<F,VC,STAR> XT(g),
                          XB(g);
    elem::PartitionDown
    ( X, XT,
         XB, snSize );

    // XT := XT - LB^{T/H} XB
    DistMatrix<F,STAR,STAR> Z( snSize, XT.Width(), g );
    if( XB.Height() != 0 )
    {
        elem::LocalGemm( orientation, NORMAL, F(-1), LB, XB, F(0), Z );
        XT.SumScatterUpdate( F(1), Z );
    }

    const int localTopHeight = LT.LocalHeight();
    std::vector<F> localDiag;
    if( diag == UNIT )
    {
        // Extract the diagonal of the top triangle and replace it with ones
        localDiag.resize( localTopHeight );
        F* LTBuffer = LT.Buffer();
        const int LTLDim = LT.LDim();
        for( int iLocal=0; iLocal<localTopHeight; ++iLocal )
        {
            const int i = commRank + iLocal*commSize;
            localDiag[iLocal] = LTBuffer[iLocal+i*LTLDim];
            LTBuffer[iLocal+i*LTLDim] = 1;
        }
    }

    // XT := LT^{T/H} XT
    elem::LocalGemm( orientation, NORMAL, F(1), LT, XT, F(0), Z );
    XT.SumScatterFrom( Z );

    if( diag == UNIT )
    {
        // Put the diagonal back
        F* LTBuffer = LT.Buffer();
        const int LTLDim = LT.LDim();
        for( int iLocal=0; iLocal<localTopHeight; ++iLocal )
        {
            const int i = commRank + iLocal*commSize;
            LTBuffer[iLocal+i*LTLDim] = localDiag[iLocal];
        }
    }
}

template<typename F>
inline void FrontFastLowerBackwardSolve
( Orientation orientation, UnitOrNonUnit diag, 
  DistMatrix<F>& L, DistMatrix<F,VC,STAR>& X )
{
#ifndef RELEASE
    CallStackEntry entry("FrontFastLowerBackwardSolve");
    if( L.Grid() != X.Grid() )
        throw std::logic_error
        ("L and X must be distributed over the same grid");
    if( L.Height() < L.Width() || L.Height() != X.Height() )
    {
        std::ostringstream msg;
        msg << "Nonconformal solve:\n"
            << "  L ~ " << L.Height() << " x " << L.Width() << "\n"
            << "  X ~ " << X.Height() << " x " << X.Width() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
    if( orientation == NORMAL )
        throw std::logic_error("This solve must be (conjugate-)transposed");
#endif
    const Grid& g = L.Grid();
    const int commSize = g.Size();
    if( commSize == 1 )
    {
        FrontLowerBackwardSolve
        ( orientation, diag, L.LockedMatrix(), X.Matrix() );
        return;
    }

    const int snSize = L.Width();
    DistMatrix<F> LT(g),
                  LB(g);
    elem::PartitionDown
    ( L, LT,
         LB, snSize );
    DistMatrix<F,VC,STAR> XT(g),
                          XB(g);
    elem::PartitionDown
    ( X, XT,
         XB, snSize );

    DistMatrix<F,MR,STAR> ZT_MR_STAR( g );
    DistMatrix<F,VR,STAR> ZT_VR_STAR( g );
    ZT_MR_STAR.AlignWith( LB );
    Zeros( ZT_MR_STAR, snSize, XT.Width() );
    if( XB.Height() != 0 )
    {
        // ZT[MR,* ] := -(LB[MC,MR])^{T/H} XB[MC,* ]
        DistMatrix<F,MC,STAR> XB_MC_STAR( g );
        XB_MC_STAR.AlignWith( LB );
        XB_MC_STAR = XB;
        elem::LocalGemm
        ( orientation, NORMAL, F(-1), LB, XB_MC_STAR, F(0), ZT_MR_STAR );

        // ZT[VR,* ].SumScatterFrom( ZT[MR,* ] )
        ZT_VR_STAR.SumScatterFrom( ZT_MR_STAR );

        // ZT[VC,* ] := ZT[VR,* ]
        DistMatrix<F,VC,STAR> ZT_VC_STAR( g );
        ZT_VC_STAR.AlignWith( XT );
        ZT_VC_STAR = ZT_VR_STAR;

        // XT[VC,* ] += ZT[VC,* ]
        elem::Axpy( F(1), ZT_VC_STAR, XT );
    }

    DistMatrix<F,MD,STAR> dTOrig(g), dTReplacement(g);
    if( diag == UNIT )
    {
        // Extract the diagonal of the top triangle and replace it with ones
        LT.GetDiagonal( dTOrig );
        dTReplacement.AlignWith( dTOrig );
        dTReplacement.ResizeTo( dTOrig.Height(), 1 );
        const int localHeight = dTOrig.LocalHeight();
        for( int iLocal=0; iLocal<localHeight; ++iLocal )
            dTReplacement.SetLocal( iLocal, 0, F(1) );
        LT.SetDiagonal( dTReplacement );
    }

    {
        // ZT[MR,* ] := (LT[MC,MR])^{T/H} XT[MC,* ]
        DistMatrix<F,MC,STAR> XT_MC_STAR( g );
        XT_MC_STAR.AlignWith( LT );
        XT_MC_STAR = XT;
        elem::LocalGemm
        ( orientation, NORMAL, F(1), LT, XT_MC_STAR, F(0), ZT_MR_STAR );

        // ZT[VR,* ].SumScatterFrom( ZT[MR,* ] )
        ZT_VR_STAR.SumScatterFrom( ZT_MR_STAR );

        // XT[VC,* ] := ZT[VR,* ]
        XT = ZT_VR_STAR;
    }

    if( diag == UNIT )
        LT.SetDiagonal( dTOrig );
}

} // namespace cliq
