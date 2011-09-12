/*
   Copyright (c) 2009-2011, Jack Poulson
   All rights reserved.

   This file is part of Elemental.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    - Neither the name of the owner nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

namespace {

#ifndef RELEASE
template<typename T>
inline void
CheckInput
( Orientation orientationOfB,
  const DistMatrix<T,MC,STAR>& A,
  const DistMatrix<T,MR,STAR>& B,
  const DistMatrix<T,MC,MR  >& C )
{
    if( orientationOfB == NORMAL )
        throw std::logic_error("B[MR,* ] must be (Conjugate)Transpose'd");
    if( A.Grid() != B.Grid() || B.Grid() != C.Grid() )
        throw std::logic_error
        ("A, B, and C must be distributed over the same grid");
    if( A.Height() != C.Height() || B.Height() != C.Width() ||
        A.Width() != B.Width() || A.Height() != B.Height() )
    {
        std::ostringstream msg;
        msg << "Nonconformal LocalTriangularRankK: \n"
            << "  A[MC,* ] ~ " << A.Height() << " x "
                               << A.Width()  << "\n"
            << "  B[MR,* ] ~ " << B.Height() << " x "
                               << B.Width()  << "\n"
            << "  C[MC,MR] ~ " << C.Height() << " x " << C.Width() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
    if( A.ColAlignment() != C.ColAlignment() ||
        B.ColAlignment() != C.RowAlignment() )
    {
        std::ostringstream msg;
        msg << "Misaligned LocalTriangularRankK: \n"
            << "  A[MC,* ] ~ " << A.ColAlignment() << "\n"
            << "  B[MR,* ] ~ " << B.ColAlignment() << "\n"
            << "  C[MC,MR] ~ " << C.ColAlignment() << " , " <<
                                  C.RowAlignment() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
}

template<typename T>
inline void
CheckInput
( Orientation orientationOfA,
  Orientation orientationOfB,
  const DistMatrix<T,STAR,MC  >& A,
  const DistMatrix<T,MR,  STAR>& B,
  const DistMatrix<T,MC,  MR  >& C )
{
    if( orientationOfA == NORMAL )
        throw std::logic_error("A[* ,MC] must be (Conjugate)Transpose'd");
    if( orientationOfB == NORMAL )
        throw std::logic_error("B[MR,* ] must be (Conjugate)Transpose'd");
    if( A.Grid() != B.Grid() || B.Grid() != C.Grid() )
        throw std::logic_error
        ("A, B, and C must be distributed over the same grid");
    if( A.Width() != C.Height() || B.Height() != C.Width() ||
        A.Height() != B.Width() || A.Width() != B.Height() )
    {
        std::ostringstream msg;
        msg << "Nonconformal LocalTriangularRankK: \n"
            << "  A[* ,MC] ~ " << A.Height() << " x "
                               << A.Width()  << "\n"
            << "  B[MR,* ] ~ " << B.Height() << " x "
                               << B.Width()  << "\n"
            << "  C[MC,MR] ~ " << C.Height() << " x " << C.Width() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
    if( A.RowAlignment() != C.ColAlignment() ||
        B.ColAlignment() != C.RowAlignment() )
    {
        std::ostringstream msg;
        msg << "Misaligned LocalTriangularRankK: \n"
            << "  A[* ,MC] ~ " << A.RowAlignment() << "\n"
            << "  B[MR,* ] ~ " << B.ColAlignment() << "\n"
            << "  C[MC,MR] ~ " << C.ColAlignment() << " , " <<
                                  C.RowAlignment() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
}

template<typename T>
inline void 
CheckInput
( const DistMatrix<T,MC,  STAR>& A, 
  const DistMatrix<T,STAR,MR  >& B,
  const DistMatrix<T,MC,  MR  >& C )
{
    if( A.Grid() != B.Grid() || B.Grid() != C.Grid() )
        throw std::logic_error
        ("A, B, and C must be distributed over the same grid");
    if( A.Height() != C.Height() || B.Width() != C.Width() ||
        A.Width() != B.Height()  || A.Height() != B.Width() )
    {
        std::ostringstream msg;
        msg << "Nonconformal LocalTriangularRankK: \n"
            << "  A[MC,* ] ~ " << A.Height() << " x "
                               << A.Width()  << "\n"
            << "  B[* ,MR] ~ " << B.Height() << " x "
                               << B.Width()  << "\n"
            << "  C[MC,MR] ~ " << C.Height() << " x " << C.Width() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
    if( A.ColAlignment() != C.ColAlignment() ||
        B.RowAlignment() != C.RowAlignment() )
    {
        std::ostringstream msg;
        msg << "Misaligned LocalTriangularRankK: \n"
            << "  A[MC,* ] ~ " << A.ColAlignment() << "\n"
            << "  B[* ,MR] ~ " << B.RowAlignment() << "\n"
            << "  C[MC,MR] ~ " << C.ColAlignment() << " , " <<
                                  C.RowAlignment() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
}

template<typename T>
inline void
CheckInput
( Orientation orientationOfA,
  const DistMatrix<T,STAR,MC>& A,
  const DistMatrix<T,STAR,MR>& B,
  const DistMatrix<T,MC,  MR>& C )
{
    if( orientationOfA == NORMAL )
        throw std::logic_error("A[* ,MC] must be (Conjugate)Transpose'd");
    if( A.Grid() != B.Grid() || B.Grid() != C.Grid() )
        throw std::logic_error
        ("A, B, and C must be distributed over the same grid");
    if( A.Width() != C.Height() || B.Width() != C.Width() ||
        A.Height() != B.Height() || A.Width() != B.Width() )
    {
        std::ostringstream msg;
        msg << "Nonconformal LocalTriangularRankK: \n"
            << "  A[* ,MC] ~ " << A.Height() << " x "
                               << A.Width()  << "\n"
            << "  B[* ,MR] ~ " << B.Height() << " x "
                               << B.Width()  << "\n"
            << "  C[MC,MR] ~ " << C.Height() << " x " << C.Width() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
    if( A.RowAlignment() != C.ColAlignment() ||
        B.RowAlignment() != C.RowAlignment() )
    {
        std::ostringstream msg;
        msg << "Misaligned LocalTriangularRankK: \n"
            << "  A[* ,MC] ~ " << A.RowAlignment() << "\n"
            << "  B[* ,MR] ~ " << B.RowAlignment() << "\n"
            << "  C[MC,MR] ~ " << C.ColAlignment() << " , " <<
                                  C.RowAlignment() << "\n";
        throw std::logic_error( msg.str().c_str() );
    }
}
#endif // WITHOUT_COMPLEX

template<typename T>
inline void
LocalTriangularRankKKernel
( Shape shape,
  Orientation orientationOfB,
  T alpha, const DistMatrix<T,MC,STAR>& A,
           const DistMatrix<T,MR,STAR>& B,
  T beta,        DistMatrix<T,MC,MR  >& C )
{
#ifndef RELEASE
    PushCallStack("LocalTriangularRankKKernel");
    CheckInput( orientationOfB, A, B, C );
#endif
    const Grid& g = C.Grid();

    DistMatrix<T,MC,STAR> AT(g),
                          AB(g);

    DistMatrix<T,MR,STAR> BT(g), 
                          BB(g);

    DistMatrix<T,MC,MR> CTL(g), CTR(g),
                        CBL(g), CBR(g);

    DistMatrix<T,MC,MR> DTL(g), DBR(g);

    const unsigned half = C.Height()/2;

    basic::Scal( beta, C );

    LockedPartitionDown
    ( A, AT,
         AB, half );

    LockedPartitionDown
    ( B, BT, 
         BB, half );

    PartitionDownDiagonal
    ( C, CTL, CTR,
         CBL, CBR, half );

    DTL.AlignWith( CTL );
    DBR.AlignWith( CBR );
    DTL.ResizeTo( CTL.Height(), CTL.Width() );
    DBR.ResizeTo( CBR.Height(), CBR.Width() );
    //------------------------------------------------------------------------//
    if( shape == LOWER )
    {
        basic::internal::LocalGemm
        ( NORMAL, orientationOfB, alpha, AB, BT, (T)1, CBL );
    }
    else
    {
        basic::internal::LocalGemm
        ( NORMAL, orientationOfB, alpha, AT, BB, (T)1, CTR );
    }

    basic::internal::LocalGemm
    ( NORMAL, orientationOfB, alpha, AT, BT, (T)0, DTL );

    // TODO: AxpyTrapezoidal?
    DTL.MakeTrapezoidal( LEFT, shape );
    basic::Axpy( (T)1, DTL, CTL );

    basic::internal::LocalGemm
    ( NORMAL, orientationOfB, alpha, AB, BB, (T)0, DBR );

    // TODO: AxpyTrapezoidal?
    DBR.MakeTrapezoidal( LEFT, shape );
    basic::Axpy( (T)1, DBR, CBR );
    //------------------------------------------------------------------------//
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
inline void
LocalTriangularRankKKernel
( Shape shape,
  Orientation orientationOfA,
  Orientation orientationOfB,
  T alpha, const DistMatrix<T,STAR,MC  >& A,
           const DistMatrix<T,MR,  STAR>& B,
  T beta,        DistMatrix<T,MC,  MR  >& C )
{
#ifndef RELEASE
    PushCallStack("LocalTriangularRankKKernel");
    CheckInput( orientationOfA, orientationOfB, A, B, C );
#endif
    const Grid& g = C.Grid();

    DistMatrix<T,STAR,MC> AL(g), AR(g);

    DistMatrix<T,MR,STAR> BT(g), 
                          BB(g);

    DistMatrix<T,MC,MR> CTL(g), CTR(g),
                        CBL(g), CBR(g);

    DistMatrix<T,MC,MR> DTL(g), DBR(g);

    const unsigned half = C.Height()/2;

    basic::Scal( beta, C );

    LockedPartitionRight( A, AL, AR, half );

    LockedPartitionDown
    ( B, BT, 
         BB, half );

    PartitionDownDiagonal
    ( C, CTL, CTR,
         CBL, CBR, half );

    DTL.AlignWith( CTL );
    DBR.AlignWith( CBR );
    DTL.ResizeTo( CTL.Height(), CTL.Width() );
    DBR.ResizeTo( CBR.Height(), CBR.Width() );
    //------------------------------------------------------------------------//
    if( shape == LOWER )
    {
        basic::internal::LocalGemm
        ( orientationOfA, orientationOfB, alpha, AR, BT, (T)1, CBL );
    }
    else
    {
        basic::internal::LocalGemm
        ( orientationOfA, orientationOfB, alpha, AL, BB, (T)1, CTR );
    }

    basic::internal::LocalGemm
    ( orientationOfA, orientationOfB, alpha, AL, BT, (T)0, DTL );

    DTL.MakeTrapezoidal( LEFT, shape );
    basic::Axpy( (T)1, DTL, CTL );

    basic::internal::LocalGemm
    ( orientationOfA, orientationOfB, alpha, AR, BB, (T)0, DBR );

    DBR.MakeTrapezoidal( LEFT, shape );
    basic::Axpy( (T)1, DBR, CBR );
    //------------------------------------------------------------------------//
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
inline void
LocalTriangularRankKKernel
( Shape shape, 
  T alpha, const DistMatrix<T,MC,  STAR>& A,
           const DistMatrix<T,STAR,MR  >& B,
  T beta,        DistMatrix<T,MC,  MR  >& C )
{
#ifndef RELEASE
    PushCallStack("LocalTriangularRankKKernel");
    CheckInput( A, B, C );
#endif
    const Grid& g = C.Grid();

    DistMatrix<T,MC,STAR> AT(g), 
                          AB(g);

    DistMatrix<T,STAR,MR> BL(g), BR(g);

    DistMatrix<T,MC,MR> CTL(g), CTR(g),
                        CBL(g), CBR(g);

    DistMatrix<T,MC,MR> DTL(g), DBR(g);

    const unsigned half = C.Height()/2;

    basic::Scal( beta, C );

    LockedPartitionDown
    ( A, AT,
         AB, half );

    LockedPartitionRight( B, BL, BR, half );

    PartitionDownDiagonal
    ( C, CTL, CTR,
         CBL, CBR, half );

    DTL.AlignWith( CTL );
    DBR.AlignWith( CBR );
    DTL.ResizeTo( CTL.Height(), CTL.Width() );
    DBR.ResizeTo( CBR.Height(), CBR.Width() );
    //------------------------------------------------------------------------//
    if( shape == LOWER )
    {
        basic::internal::LocalGemm
        ( NORMAL, NORMAL, alpha, AB, BL, (T)1, CBL );
    }
    else
    {
        basic::internal::LocalGemm
        ( NORMAL, NORMAL, alpha, AT, BR, (T)1, CTR );
    }

    basic::internal::LocalGemm
    ( NORMAL, NORMAL, alpha, AT, BL, (T)0, DTL );

    DTL.MakeTrapezoidal( LEFT, shape );
    basic::Axpy( (T)1, DTL, CTL );

    basic::internal::LocalGemm
    ( NORMAL, NORMAL, alpha, AB, BR, (T)0, DBR );

    DBR.MakeTrapezoidal( LEFT, shape );
    basic::Axpy( (T)1, DBR, CBR );
    //------------------------------------------------------------------------//
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
inline void
LocalTriangularRankKKernel
( Shape shape,
  Orientation orientationOfA,
  T alpha, const DistMatrix<T,STAR,MC>& A,
           const DistMatrix<T,STAR,MR>& B,
  T beta,        DistMatrix<T,MC,  MR>& C )
{
#ifndef RELEASE
    PushCallStack("LocalTriangularRankKKernel");
    CheckInput( orientationOfA, A, B, C );
#endif
    const Grid& g = C.Grid();

    DistMatrix<T,STAR,MC> AL(g), AR(g);

    DistMatrix<T,STAR,MR> BL(g), BR(g);

    DistMatrix<T,MC,MR> CTL(g), CTR(g),
                        CBL(g), CBR(g);

    DistMatrix<T,MC,MR> DTL(g), DBR(g);

    const unsigned half = C.Height()/2;

    basic::Scal( beta, C );

    LockedPartitionRight( A, AL, AR, half );
    LockedPartitionRight( B, BL, BR, half );

    PartitionDownDiagonal
    ( C, CTL, CTR,
         CBL, CBR, half );

    DTL.AlignWith( CTL );
    DBR.AlignWith( CBR );
    DTL.ResizeTo( CTL.Height(), CTL.Width() );
    DBR.ResizeTo( CBR.Height(), CBR.Width() );
    //------------------------------------------------------------------------//
    if( shape == LOWER )
    {
        basic::internal::LocalGemm
        ( orientationOfA, NORMAL, alpha, AR, BL, (T)1, CBL );
    }
    else
    {
        basic::internal::LocalGemm
        ( orientationOfA, NORMAL, alpha, AL, BR, (T)1, CTR );
    }

    basic::internal::LocalGemm
    ( orientationOfA, NORMAL, alpha, AL, BL, (T)0, DTL );

    DTL.MakeTrapezoidal( LEFT, shape );
    basic::Axpy( (T)1, DTL, CTL );

    basic::internal::LocalGemm
    ( orientationOfA, NORMAL, alpha, AR, BR, (T)0, DBR );

    DBR.MakeTrapezoidal( LEFT, shape );
    basic::Axpy( (T)1, DBR, CBR );
    //------------------------------------------------------------------------//
#ifndef RELEASE
    PopCallStack();
#endif
}

} // anonymous namespace

template<typename T>
inline void
elemental::basic::internal::LocalTriangularRankK
( Shape shape,
  Orientation orientationOfB,
  T alpha, const DistMatrix<T,MC,STAR>& A,
           const DistMatrix<T,MR,STAR>& B,
  T beta,        DistMatrix<T,MC,MR  >& C )
{
#ifndef RELEASE
    PushCallStack("basic::internal::LocalTriangularRankK");
    CheckInput( orientationOfB, A, B, C );
#endif
    const Grid& g = C.Grid();

    if( C.Height() < g.Width()*LocalTriangularRankKBlocksize<T>() )
    {
        LocalTriangularRankKKernel
        ( shape, orientationOfB, alpha, A, B, beta, C );
    }
    else
    {
        // Split C in four roughly equal pieces, perform a large gemm on corner
        // and recurse on CTL and CBR.

        DistMatrix<T,MC,STAR> AT(g),
                              AB(g);

        DistMatrix<T,MR,STAR> BT(g), 
                              BB(g);

        DistMatrix<T,MC,MR> CTL(g), CTR(g),
                            CBL(g), CBR(g);

        const unsigned half = C.Height() / 2;

        LockedPartitionDown
        ( A, AT,
             AB, half );

        LockedPartitionDown
        ( B, BT, 
             BB, half );

        PartitionDownDiagonal
        ( C, CTL, CTR,
             CBL, CBR, half );

        if( shape == LOWER )
        { 
            basic::internal::LocalGemm
            ( NORMAL, orientationOfB, alpha, AB, BT, beta, CBL );
        }
        else
        {
            basic::internal::LocalGemm
            ( NORMAL, orientationOfB, alpha, AT, BB, beta, CTR );
        }

        // Recurse
        basic::internal::LocalTriangularRankK
        ( shape, orientationOfB, alpha, AT, BT, beta, CTL );

        basic::internal::LocalTriangularRankK
        ( shape, orientationOfB, alpha, AB, BB, beta, CBR );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
inline void
elemental::basic::internal::LocalTriangularRankK
( Shape shape,
  Orientation orientationOfA,
  Orientation orientationOfB,
  T alpha, const DistMatrix<T,STAR,MC  >& A,
           const DistMatrix<T,MR,  STAR>& B,
  T beta,        DistMatrix<T,MC,  MR  >& C )
{
#ifndef RELEASE
    PushCallStack("basic::internal::LocalTriangularRankK");
    CheckInput( orientationOfA, orientationOfB, A, B, C );
#endif
    const Grid& g = C.Grid();

    if( C.Height() < g.Width()*LocalTriangularRankKBlocksize<T>() )
    {
        LocalTriangularRankKKernel
        ( shape, orientationOfA, orientationOfB, alpha, A, B, beta, C );
    }
    else
    {
        // Split C in four roughly equal pieces, perform a large gemm on corner
        // and recurse on CTL and CBR.

        DistMatrix<T,STAR,MC> AL(g), AR(g);

        DistMatrix<T,MR,STAR> BT(g), 
                              BB(g);

        DistMatrix<T,MC,MR> CTL(g), CTR(g),
                            CBL(g), CBR(g);

        const unsigned half = C.Height() / 2;

        LockedPartitionRight( A, AL, AR, half );

        LockedPartitionDown
        ( B, BT, 
             BB, half );

        PartitionDownDiagonal
        ( C, CTL, CTR,
             CBL, CBR, half );

        if( shape == LOWER )
        { 
            basic::internal::LocalGemm
            ( orientationOfA, orientationOfB, alpha, AR, BT, beta, CBL );
        }
        else
        {
            basic::internal::LocalGemm
            ( orientationOfA, orientationOfB, alpha, AL, BB, beta, CTR );
        }

        // Recurse
        basic::internal::LocalTriangularRankK
        ( shape, orientationOfA, orientationOfB, alpha, AL, BT, beta, CTL );

        basic::internal::LocalTriangularRankK
        ( shape, orientationOfA, orientationOfB, alpha, AR, BB, beta, CBR );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
inline void
elemental::basic::internal::LocalTriangularRankK
( Shape shape,
  T alpha, const DistMatrix<T,MC,  STAR>& A,
           const DistMatrix<T,STAR,MR  >& B,
  T beta,        DistMatrix<T,MC,  MR  >& C )
{
#ifndef RELEASE
    PushCallStack("basic::internal::LocalTriangularRankK");
    CheckInput( A, B, C );
#endif
    const Grid& g = C.Grid();

    if( C.Height() < g.Width()*LocalTriangularRankKBlocksize<T>() )
    {
        LocalTriangularRankKKernel
        ( shape, alpha, A, B, beta, C );
    }
    else
    {
        // Split C in four roughly equal pieces, perform a large gemm on corner
        // and recurse on CTL and CBR.

        DistMatrix<T,MC,STAR> AT(g),
                              AB(g);

        DistMatrix<T,STAR,MR> BL(g), BR(g);

        DistMatrix<T,MC,MR> CTL(g), CTR(g),
                            CBL(g), CBR(g);

        const unsigned half = C.Height() / 2;

        LockedPartitionDown
        ( A, AT,
             AB, half );

        LockedPartitionRight( B, BL, BR, half );

        PartitionDownDiagonal
        ( C, CTL, CTR,
             CBL, CBR, half );

        if( shape == LOWER )
        { 
            basic::internal::LocalGemm
            ( NORMAL, NORMAL, alpha, AB, BL, beta, CBL );
        }
        else
        {
            basic::internal::LocalGemm
            ( NORMAL, NORMAL, alpha, AT, BR, beta, CTR );
        }

        // Recurse
        basic::internal::LocalTriangularRankK
        ( shape, alpha, AT, BL, beta, CTL );

        basic::internal::LocalTriangularRankK
        ( shape, alpha, AB, BR, beta, CBR );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T>
inline void
elemental::basic::internal::LocalTriangularRankK
( Shape shape,
  Orientation orientationOfA,
  T alpha, const DistMatrix<T,STAR,MC>& A,
           const DistMatrix<T,STAR,MR>& B,
  T beta,        DistMatrix<T,MC,  MR>& C )
{
#ifndef RELEASE
    PushCallStack("basic::internal::LocalTriangularRankK");
    CheckInput( orientationOfA, A, B, C );
#endif
    const Grid& g = C.Grid();

    if( C.Height() < g.Width()*LocalTriangularRankKBlocksize<T>() )
    {
        LocalTriangularRankKKernel
        ( shape, orientationOfA, alpha, A, B, beta, C );
    }
    else
    {
        // Split C in four roughly equal pieces, perform a large gemm on corner
        // and recurse on CTL and CBR.

        DistMatrix<T,STAR,MC> AL(g), AR(g);

        DistMatrix<T,STAR,MR> BL(g), BR(g);

        DistMatrix<T,MC,MR> CTL(g), CTR(g),
                            CBL(g), CBR(g);

        const unsigned half = C.Height() / 2;

        LockedPartitionRight( A, AL, AR, half );

        LockedPartitionRight( B, BL, BR, half );

        PartitionDownDiagonal
        ( C, CTL, CTR,
             CBL, CBR, half );

        if( shape == LOWER )
        { 
            basic::internal::LocalGemm
            ( orientationOfA, NORMAL, alpha, AR, BL, beta, CBL );
        }
        else
        {
            basic::internal::LocalGemm
            ( orientationOfA, NORMAL, alpha, AL, BR, beta, CTR );
        }

        // Recurse
        basic::internal::LocalTriangularRankK
        ( shape, orientationOfA, alpha, AL, BL, beta, CTL );

        basic::internal::LocalTriangularRankK
        ( shape, orientationOfA, alpha, AR, BR, beta, CBR );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}
