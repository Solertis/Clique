/*
   Copyright (c) 2009-2012, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/

namespace elem {

// To make our life easier. Undef'd at the bottom of the header
#define M  Matrix<T,Int>
#define DM DistMatrix<T,U,V,Int>

//
// SlidePartitionUp
//

template<typename T,typename Int>
inline void
SlidePartitionUp
( M& AT, M& A0,
         M& A1,
  M& AB, M& A2 )
{
#ifndef RELEASE
    PushCallStack("SlidePartitionUp [Matrix]");
#endif
    AT.View( A0 );
    AB.View2x1( A1,
                A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,Distribution U,Distribution V,typename Int>
inline void
SlidePartitionUp
( DM& AT, DM& A0,
          DM& A1,
  DM& AB, DM& A2 )
{
#ifndef RELEASE
    PushCallStack("SlidePartitionUp [DistMatrix]");
#endif
    AT.View( A0 );
    AB.View2x1( A1,
                A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,typename Int>
inline void
SlideLockedPartitionUp
( M& AT, const M& A0,
         const M& A1,
  M& AB, const M& A2 )
{
#ifndef RELEASE
    PushCallStack("SlideLockedPartitionUp [Matrix]");
#endif
    AT.LockedView( A0 );
    AB.LockedView2x1( A1,
                      A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,Distribution U,Distribution V,typename Int>
inline void
SlideLockedPartitionUp
( DM& AT, const DM& A0,
          const DM& A1,
  DM& AB, const DM& A2 )
{
#ifndef RELEASE
    PushCallStack("SlideLockedPartitionUp [DistMatrix]");
#endif
    AT.LockedView( A0 );
    AB.LockedView2x1( A1,
                      A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

//
// SlidePartitionDown
//

template<typename T,typename Int>
inline void
SlidePartitionDown
( M& AT, M& A0,
         M& A1,
  M& AB, M& A2 )
{
#ifndef RELEASE
    PushCallStack("SlidePartitionDown [Matrix]");
#endif
    AT.View2x1( A0,
                A1 );
    AB.View( A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,Distribution U,Distribution V,typename Int>
inline void
SlidePartitionDown
( DM& AT, DM& A0,
          DM& A1,
  DM& AB, DM& A2 )
{
#ifndef RELEASE
    PushCallStack("SlidePartitionDown [DistMatrix]");
#endif
    AT.View2x1( A0,
                A1 );
    AB.View( A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,typename Int>
inline void
SlideLockedPartitionDown
( M& AT, const M& A0,
         const M& A1,
  M& AB, const M& A2 )
{
#ifndef RELEASE
    PushCallStack("SlideLockedPartitionDown [Matrix]");
#endif
    AT.LockedView2x1( A0,
                      A1 );
    AB.LockedView( A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,Distribution U,Distribution V,typename Int>
inline void
SlideLockedPartitionDown
( DM& AT, const DM& A0,
          const DM& A1,
  DM& AB, const DM& A2 )
{
#ifndef RELEASE
    PushCallStack("SlideLockedPartitionDown [DistMatrix]");
#endif
    AT.LockedView2x1( A0,
                      A1 );
    AB.LockedView( A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

//
// SlidePartitionLeft
//

template<typename T,typename Int>
inline void
SlidePartitionLeft
( M& AL, M& AR,
  M& A0, M& A1, M& A2 )
{
#ifndef RELEASE
    PushCallStack("SlidePartitionLeft [Matrix]");
#endif
    AL.View( A0 );
    AR.View1x2( A1, A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,Distribution U,Distribution V,typename Int>
inline void
SlidePartitionLeft
( DM& AL, DM& AR,
  DM& A0, DM& A1, DM& A2 )
{
#ifndef RELEASE
    PushCallStack("SlidePartitionLeft [DistMatrix]");
#endif
    AL.View( A0 );
    AR.View1x2( A1, A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,typename Int>
inline void
SlideLockedPartitionLeft
( M& AL, M& AR,
  const M& A0, const M& A1, const M& A2 )
{
#ifndef RELEASE
    PushCallStack("SlideLockedPartitionLeft [Matrix]");
#endif
    AL.LockedView( A0 );
    AR.LockedView1x2( A1, A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,Distribution U,Distribution V,typename Int>
inline void
SlideLockedPartitionLeft
( DM& AL, DM& AR,
  const DM& A0, const DM& A1, const DM& A2 )
{
#ifndef RELEASE
    PushCallStack("SlideLockedPartitionLeft [DistMatrix]");
#endif
    AL.LockedView( A0 );
    AR.LockedView1x2( A1, A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

//
// SlidePartitionRight
//

template<typename T,typename Int>
inline void
SlidePartitionRight
( M& AL, M& AR,
  M& A0, M& A1, M& A2 )
{
#ifndef RELEASE
    PushCallStack("SlidePartitionRight [Matrix]");
#endif
    AL.View1x2( A0, A1 );
    AR.View( A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,Distribution U,Distribution V,typename Int>
inline void
SlidePartitionRight
( DM& AL, DM& AR,
  DM& A0, DM& A1, DM& A2 )
{
#ifndef RELEASE
    PushCallStack("SlidePartitionRight [DistMatrix]");
#endif
    AL.View1x2( A0, A1 );
    AR.View( A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,typename Int>
inline void
SlideLockedPartitionRight
( M& AL, M& AR,
  const M& A0, const M& A1, const M& A2 )
{
#ifndef RELEASE
    PushCallStack("SlideLockedPartitionRight [Matrix]");
#endif
    AL.LockedView1x2( A0, A1 );
    AR.LockedView( A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,Distribution U,Distribution V,typename Int>
inline void
SlideLockedPartitionRight
( DM& AL, DM& AR,
  const DM& A0, const DM& A1, const DM& A2 )
{
#ifndef RELEASE
    PushCallStack("SlideLockedPartitionRight [DistMatrix]");
#endif
    AL.LockedView1x2( A0, A1 );
    AR.LockedView( A2 );
#ifndef RELEASE
    PopCallStack();
#endif
}

//
// SlidePartitionUpDiagonal
//

template<typename T,typename Int>
inline void
SlidePartitionUpDiagonal
( M& ATL, M& ATR, M& A00, M& A01, M& A02,
                  M& A10, M& A11, M& A12,
  M& ABL, M& ABR, M& A20, M& A21, M& A22 )
{
#ifndef RELEASE
    PushCallStack("SlidePartitionUpDiagonal [Matrix]");
#endif
    ATL.View( A00 );
    ATR.View1x2( A01, A02 );
    ABL.View2x1( A10,
                 A20 );
    ABR.View2x2( A11, A12,
                 A21, A22 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,Distribution U,Distribution V,typename Int>
inline void
SlidePartitionUpDiagonal
( DM& ATL, DM& ATR, DM& A00, DM& A01, DM& A02,
                    DM& A10, DM& A11, DM& A12,
  DM& ABL, DM& ABR, DM& A20, DM& A21, DM& A22 )
{
#ifndef RELEASE
    PushCallStack("SlidePartitionUpDiagonal [DistMatrix]");
#endif
    ATL.View( A00 );
    ATR.View1x2( A01, A02 );
    ABL.View2x1( A10,
                 A20 );
    ABR.View2x2( A11, A12,
                 A21, A22 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,typename Int>
inline void
SlideLockedPartitionUpDiagonal
( M& ATL, M& ATR, const M& A00, const M& A01, const M& A02,
                  const M& A10, const M& A11, const M& A12,
  M& ABL, M& ABR, const M& A20, const M& A21, const M& A22 )
{
#ifndef RELEASE
    PushCallStack("SlideLockedPartitionUpDiagonal [Matrix]");
#endif
    ATL.LockedView( A00 );
    ATR.LockedView1x2( A01, A02 );
    ABL.LockedView2x1( A10,
                       A20 );
    ABR.LockedView2x2( A11, A12,
                       A21, A22 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,Distribution U,Distribution V,typename Int>
inline void
SlideLockedPartitionUpDiagonal
( DM& ATL, DM& ATR, const DM& A00, const DM& A01, const DM& A02,
                    const DM& A10, const DM& A11, const DM& A12,
  DM& ABL, DM& ABR, const DM& A20, const DM& A21, const DM& A22 )
{
#ifndef RELEASE
    PushCallStack("SlideLockedPartitionUpDiagonal [DistMatrix]");
#endif
    ATL.LockedView( A00 );
    ATR.LockedView1x2( A01, A02 );
    ABL.LockedView2x1( A10,
                       A20 );
    ABR.LockedView2x2( A11, A12,
                       A21, A22 );
#ifndef RELEASE
    PopCallStack();
#endif
}

//
// SlidePartitionDownDiagonal
//

template<typename T,typename Int>
inline void
SlidePartitionDownDiagonal
( M& ATL, M& ATR, M& A00, M& A01, M& A02,
                  M& A10, M& A11, M& A12,
  M& ABL, M& ABR, M& A20, M& A21, M& A22 )
{
#ifndef RELEASE
    PushCallStack("SlidePartitionDownDiagonal [Matrix]");
#endif
    ATL.View2x2( A00, A01,
                 A10, A11 );
    ATR.View2x1( A02,
                 A12 );
    ABL.View1x2( A20, A21 );
    ABR.View( A22 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,Distribution U,Distribution V,typename Int>
inline void
SlidePartitionDownDiagonal
( DM& ATL, DM& ATR, DM& A00, DM& A01, DM& A02,
                    DM& A10, DM& A11, DM& A12,
  DM& ABL, DM& ABR, DM& A20, DM& A21, DM& A22 )
{
#ifndef RELEASE
    PushCallStack("SlidePartitionDownDiagonal [DistMatrix]");
#endif
    ATL.View2x2( A00, A01,
                 A10, A11 );
    ATR.View2x1( A02,
                 A12 );
    ABL.View1x2( A20, A21 );
    ABR.View( A22 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,typename Int>
inline void
SlideLockedPartitionDownDiagonal
( M& ATL, M& ATR, const M& A00, const M& A01, const M& A02,
                  const M& A10, const M& A11, const M& A12,
  M& ABL, M& ABR, const M& A20, const M& A21, const M& A22 )
{
#ifndef RELEASE
    PushCallStack("SlideLockedPartitionDownDiagonal [Matrix]");
#endif
    ATL.LockedView2x2( A00, A01,
                       A10, A11 );
    ATR.LockedView2x1( A02,
                       A12 );
    ABL.LockedView1x2( A20, A21 );
    ABR.LockedView( A22 );
#ifndef RELEASE
    PopCallStack();
#endif
}

template<typename T,Distribution U,Distribution V,typename Int>
inline void
SlideLockedPartitionDownDiagonal
( DM& ATL, DM& ATR, const DM& A00, const DM& A01, const DM& A02,
                    const DM& A10, const DM& A11, const DM& A12,
  DM& ABL, DM& ABR, const DM& A20, const DM& A21, const DM& A22 )
{
#ifndef RELEASE
    PushCallStack("SlideLockedPartitionDownDiagonal [DistMatrix]");
#endif
    ATL.LockedView2x2( A00, A01,
                       A10, A11 );
    ATR.LockedView2x1( A02,
                       A12 );
    ABL.LockedView1x2( A20, A21 );
    ABR.LockedView( A22 );
#ifndef RELEASE
    PopCallStack();
#endif
}

#undef DM
#undef M

} // namespace elem
