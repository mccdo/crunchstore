cmake_minimum_required(VERSION 2.8.8)

project( crunchstore )

SET( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules" )

set(crunchstore_MAJOR_VERSION 0)
set(crunchstore_MINOR_VERSION 2)
set(crunchstore_SUB_VERSION 0)
set(crunchstore_VERSION ${crunchstore_MAJOR_VERSION}.${crunchstore_MINOR_VERSION}.${crunchstore_SUB_VERSION})
set(VES_PACKAGE_VERSION ${crunchstore_VERSION})

#Set the include directory so that crunchstore can be used as a sub project
if( NOT(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME}) )
    set(crunchstore_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/src" PARENT_SCOPE)
    set(crunchstore_LIBRARIES "crunchstore" PARENT_SCOPE)
endif()

set(POCO_ROOT "Please specify" CACHE PATH "Root directory where POCO is installed" )
find_package(POCO 1.4.1 REQUIRED COMPONENTS PocoFoundation PocoUtil PocoData PocoDataSQLite)

# should we build in mongoDB support?
#option(USE_MONGO "Build mongoDB adapter?" ON)
option(BUILD_EXAMPLES "Build examples?" ON)

set(BOOST_INCLUDEDIR "$ENV{BOOST_INSTALL_DIR}/include" CACHE PATH "docstring")

set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

set( BOOST_ROOT "Please specify" CACHE PATH "Root directory where Boost is installed" )
find_package(Boost 1.44.0 REQUIRED COMPONENTS system)

find_package( MongoDB )

#set(MONGOCLIENT_PATH "$ENV{MONGOCLIENT_INSTALL_DIR}" CACHE PATH "docstring")

INCLUDE_DIRECTORIES( 
    ${Boost_INCLUDE_DIRS}
    #${MONGOCLIENT_PATH}/include
    ${POCO_INCLUDE_DIR}
)

include(VESSetupCompiler)

include(VESBinAndLibDirs)

add_subdirectory(src)

#
# Doxygen
#find_package( Doxygen )
#if( DOXYGEN_FOUND )
#    set( HAVE_DOT "NO" )
#    if( DOXYGEN_DOT_PATH )
#        set( HAVE_DOT "YES" )
#    endif()
#    
#    configure_file( ${PROJECT_SOURCE_DIR}/doc/doxyfile.cmake
#        ${PROJECT_BINARY_DIR}/doc/doxyfile
#    )
#    add_custom_target( Documentation ${DOXYGEN_EXECUTABLE}
#        ${PROJECT_BINARY_DIR}/doc/doxyfile
#    )
#    
#    add_subdirectory( doc/images )
#endif()

include(VESInstallPDBFiles)

set(VES_EXPORT_LIBRARY_TARGETS crunchstore)
set(VES_CONFIGURE_VARS CMAKE_INSTALL_LIBDIR)

#
# Setup the fpc auto detection files
if(WIN32)
    set(crunchstore_FPC_LIBRARIES "/libpath:\"\${libdir}\" crunchstore.lib")
    set(crunchstore_FPC_INCLUDES "/I\"\${includedir}\"")
else(WIN32)
    set(crunchstore_FPC_INCLUDES "-I\${includedir}")
    set(crunchstore_FPC_LIBRARIES "-L\${libdir} -lcrunchstore")
endif(WIN32)

include(VESInstallAutoDiscoveryTools)

