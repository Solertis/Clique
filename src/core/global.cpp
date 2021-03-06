/*
   Copyright (c) 2009-2014, Jack Poulson, Lexing Ying,
   The University of Texas at Austin, Stanford University, and the
   Georgia Insitute of Technology.
   All rights reserved.
 
   This file is part of Clique and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#include "clique.hpp"

namespace { 
bool cliqueInitializedElemental; 
int numCliqueInits = 0;
cliq::Args* args = 0;
DEBUG_ONLY(std::stack<std::string> callStack)
}

namespace cliq {

void PrintVersion( std::ostream& os )
{
    os << "Clique version information:\n"
       << "  Git revision: " << EL_GIT_SHA1 << "\n"
       << "  Version:      " << Clique_VERSION_MAJOR << "."
                             << Clique_VERSION_MINOR << "\n"
       << "  Build type:   " << EL_CMAKE_BUILD_TYPE << "\n"
       << std::endl;
}

void PrintConfig( std::ostream& os )
{
    os << "Clique configuration:\n";
#ifdef USE_CUSTOM_ALLTOALLV
    os << "  USE_CUSTOM_ALLTOALLV\n";
#endif
#ifdef BARRIER_IN_ALLTOALLV
    os << "  BARRIER_IN_ALLTOALLV\n";
#endif
#ifdef HAVE_PARMETIS
    os << "  HAVE_PARMETIS\n";
#endif
    El::PrintConfig( os );
}

void PrintCCompilerInfo( std::ostream& os )
{
    os << "Clique's C compiler info:\n"
       << "  CMAKE_C_COMPILER:    " << EL_CMAKE_C_COMPILER << "\n"
       << "  MPI_C_COMPILER:      " << EL_MPI_C_COMPILER << "\n"
       << "  MPI_C_INCLUDE_PATH:  " << EL_MPI_C_INCLUDE_PATH << "\n"
       << "  MPI_C_COMPILE_FLAGS: " << EL_MPI_C_COMPILE_FLAGS << "\n"
       << "  MPI_C_LINK_FLAGS:    " << EL_MPI_C_LINK_FLAGS << "\n"
       << "  MPI_C_LIBRARIES:     " << EL_MPI_C_LIBRARIES << "\n"
       << std::endl;
}

void PrintCxxCompilerInfo( std::ostream& os )
{
    os << "Clique's C++ compiler info:\n"
       << "  CMAKE_CXX_COMPILER:    " << EL_CMAKE_CXX_COMPILER << "\n"
       << "  CXX_FLAGS:             " << EL_CXX_FLAGS << "\n"
       << "  MPI_CXX_COMPILER:      " << EL_MPI_CXX_COMPILER << "\n"
       << "  MPI_CXX_INCLUDE_PATH:  " << EL_MPI_CXX_INCLUDE_PATH << "\n"
       << "  MPI_CXX_COMPILE_FLAGS: " << EL_MPI_CXX_COMPILE_FLAGS << "\n"
       << "  MPI_CXX_LINK_FLAGS:    " << EL_MPI_CXX_LINK_FLAGS << "\n"
       << "  MPI_CXX_LIBRARIES:     " << EL_MPI_CXX_LIBRARIES << "\n"
       << std::endl;
}

bool Initialized()
{ return ::numCliqueInits > 0; }

void Initialize( int& argc, char**& argv )
{
    // If Clique has already been initialized, this is a no-op
    if( ::numCliqueInits > 0 )
    {
        ++::numCliqueInits;
        return;
    }

    ::args = new Args( argc, argv );
    ::numCliqueInits = 1;
    if( !El::Initialized() )
    {
        El::Initialize( argc, argv );
        ::cliqueInitializedElemental = true;
    }
    else
    {
        ::cliqueInitializedElemental = false;
    }
}

void Finalize()
{
    if( ::numCliqueInits <= 0 )
        throw std::logic_error("Finalized Clique more than initialized");
    --::numCliqueInits;
    if( ::cliqueInitializedElemental )
        El::Finalize();

    if( ::numCliqueInits == 0 )
    {
        delete ::args;    
        ::args = 0;
    }
}

DEBUG_ONLY(
    void PushCallStack( std::string s )
    { ::callStack.push( s ); }

    void PopCallStack()
    { ::callStack.pop(); }

    void DumpCallStack()
    {
        std::ostringstream msg;
        while( !::callStack.empty() )
        {
            msg << "[" << ::callStack.size() << "]: " << ::callStack.top() 
                << "\n";
            ::callStack.pop();
        }
        std::cerr << msg.str();;
        std::cerr.flush();
    }
)

void ReportException( std::exception& e )
{
    El::ReportException( e );
    DEBUG_ONLY(DumpCallStack())
}

Args& GetArgs()
{
    if( args == 0 )
        throw std::runtime_error("No available instance of Args");
    return *::args;
}

} // namespace cliq
