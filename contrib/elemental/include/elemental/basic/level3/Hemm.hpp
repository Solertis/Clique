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

#include "./Hemm/HemmLL.hpp"
#include "./Hemm/HemmLU.hpp"
#include "./Hemm/HemmRL.hpp"
#include "./Hemm/HemmRU.hpp"

template<typename T>
inline void
elemental::basic::Hemm
(  Side side, Shape shape,
   T alpha, const DistMatrix<T,MC,MR>& A,
            const DistMatrix<T,MC,MR>& B,
   T beta,        DistMatrix<T,MC,MR>& C )
{
#ifndef RELEASE
    PushCallStack("basic::Hemm");
#endif
    if( side == LEFT && shape == LOWER )
    {
        basic::internal::HemmLL( alpha, A, B, beta, C );
    }
    else if( side == LEFT )
    {
        basic::internal::HemmLU( alpha, A, B, beta, C );
    }
    else if( shape == LOWER )
    {
        basic::internal::HemmRL( alpha, A, B, beta, C );
    }
    else
    {
        basic::internal::HemmRU( alpha, A, B, beta, C );
    }
#ifndef RELEASE
    PopCallStack();
#endif
}
