/*
   Copyright (c) 2009-2013, Jack Poulson, Lexing Ying,
   The University of Texas at Austin, and Stanford University
   All rights reserved.
 
   This file is part of Clique and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#ifndef CLIQUE_HPP
#define CLIQUE_HPP

#include <algorithm>
#include <map>
#include <set>

#include "elemental-lite.hpp"
#include "elemental/blas-like/level1/Adjoint.hpp"
#include "elemental/blas-like/level1/Axpy.hpp"
#include "elemental/blas-like/level1/Conjugate.hpp"
#include "elemental/blas-like/level1/DiagonalSolve.hpp"
#include "elemental/blas-like/level1/MakeSymmetric.hpp"
#include "elemental/blas-like/level1/MakeTrapezoidal.hpp"
#include "elemental/blas-like/level1/MakeTriangular.hpp"
#include "elemental/blas-like/level1/QuasiDiagonalSolve.hpp"
#include "elemental/blas-like/level1/SetDiagonal.hpp"
#include "elemental/blas-like/level2/ApplyColumnPivots.hpp"
#include "elemental/blas-like/level2/ApplyRowPivots.hpp"
#include "elemental/blas-like/level2/ApplySymmetricPivots.hpp"
#include "elemental/blas-like/level3/Gemm.hpp"
#include "elemental/blas-like/level3/Trdtrmm.hpp"
#include "elemental/blas-like/level3/Trmm.hpp"
#include "elemental/blas-like/level3/Trsm.hpp"
/** Tx: Updates for new elemental **/
#include "elemental/lapack-like/factor/LDL.hpp"
/** #include "elemental/lapack-like/TriangularInverse.hpp" **/
#include "elemental/lapack-like/funcs/Inverse/Triangular.hpp"
#include "elemental/matrices/Zeros.hpp"
#include "elemental/io.hpp"

#include "clique/config.h"

//
// The core of the library
//

#include "clique/core/environment/decl.hpp"
#include "clique/core/environment/impl.hpp"

// Data-structure declarations (are these needed?!?)
namespace cliq { 
    class Graph; 
    class DistGraph; 
    template<typename T> class SparseMatrix;
    template<typename T> class DistSparseMatrix;
    template<typename T> class DistNodalMultiVec;
    template<typename T> class DistNodalMatrix;
} 
#include "clique/core/graph/decl.hpp"
#include "clique/core/dist_graph/decl.hpp"
//TODO: include "clique/core/map/decl.hpp"
#include "clique/core/dist_map/decl.hpp"
#include "clique/core/sparse_matrix/decl.hpp"
#include "clique/core/dist_sparse_matrix/decl.hpp"
#include "clique/core/multi_vec/decl.hpp"
#include "clique/core/dist_multi_vec/decl.hpp"

// Data-structure implementations
#include "clique/core/graph/impl.hpp"
#include "clique/core/dist_graph/impl.hpp"
//TODO: include "clique/core/map/impl.hpp"
#include "clique/core/dist_map/impl.hpp"
#include "clique/core/sparse_matrix/impl.hpp"
#include "clique/core/dist_sparse_matrix/impl.hpp"
#include "clique/core/multi_vec/impl.hpp"
#include "clique/core/dist_multi_vec/impl.hpp"

//
// Symbolic computation
//

#include "clique/symbolic/dist_separator_tree.hpp"
#include "clique/symbolic/dist_symm_elim_tree.hpp"
#include "clique/symbolic/dist_symm_info.hpp"
#include "clique/symbolic/symm_analysis.hpp"
#include "clique/symbolic/nested_dissection.hpp"
#include "clique/symbolic/natural_nested_dissection.hpp"

//
// Numerical computation
//

#include "clique/numeric/multiply.hpp"

#include "clique/numeric/dist_nodal_multi_vec/decl.hpp"
#include "clique/numeric/dist_nodal_matrix/decl.hpp"
#include "clique/numeric/dist_nodal_multi_vec/impl.hpp"
#include "clique/numeric/dist_nodal_matrix/impl.hpp"

#include "clique/numeric/dist_symm_front_tree/decl.hpp"
#include "clique/numeric/dist_symm_front_tree/impl.hpp"
#include "clique/numeric/change_front_type.hpp"

#include "clique/numeric/ldl.hpp"
#include "clique/numeric/lower_solve.hpp"
#include "clique/numeric/diagonal_solve.hpp"
#include "clique/numeric/solve.hpp"
#include "clique/numeric/lower_multiply.hpp"

//
// Input/Output
//

#include "clique/io/Print.hpp"
#include "clique/io/Display.hpp"

#endif // ifndef CLIQUE_HPP
