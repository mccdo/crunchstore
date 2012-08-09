cmake_minimum_required(VERSION 2.8.8)

project( crunchstore )

SET( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules" )

set(crunchstore_MAJOR_VERSION 0)
set(crunchstore_MINOR_VERSION 2)
set(crunchstore_SUB_VERSION 0)
set(crunchstore_VERSION ${crunchstore_MAJOR_VERSION}.${crunchstore_MINOR_VERSION}.${crunchstore_SUB_VERSION})


#include( FindPocoHelper )
#PocoMenuSetup()
#PocoFinder()

set( POCO_ROOT "Please specify" CACHE PATH "Root directory where POCO is installed" )
FIND_PACKAGE(POCO 1.4.1 REQUIRED COMPONENTS PocoFoundation PocoUtil PocoData PocoDataSQLite)

# should we build in mongoDB support?
#option(USE_MONGO "Build mongoDB adapter?" ON)
option(BUILD_EXAMPLES "Build examples?" ON)

set(BOOST_INCLUDEDIR "$ENV{BOOST_INSTALL_DIR}/include" CACHE PATH "docstring")

set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

set( BOOST_ROOT "Please specify" CACHE PATH "Root directory where Boost is installed" )
FIND_PACKAGE(Boost 1.44.0 REQUIRED COMPONENTS system)

find_package( MongoDB )

#set(MONGOCLIENT_PATH "$ENV{MONGOCLIENT_INSTALL_DIR}" CACHE PATH "docstring")

INCLUDE_DIRECTORIES( 
    ${Boost_INCLUDE_DIRS}
    #${MONGOCLIENT_PATH}/include
    ${POCO_INCLUDE_DIR}
)

################################################################################
# Platform specific definitions
IF(WIN32)
    IF(MSVC)
        # To enable /MP, parralel build on MSVC
        OPTION(WIN32_USE_MP "Set to OFF to diable /MP." ON)
        MARK_AS_ADVANCED(WIN32_USE_MP)
        IF(WIN32_USE_MP)
            SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
        ENDIF(WIN32_USE_MP)

        # Other MSVC compilation flags
        #ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)
        #ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)

    ENDIF(MSVC)
ENDIF (WIN32)

if(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32 AND NOT APPLE)
    include(GNUInstallDirs)
endif(CMAKE_COMPILER_IS_GNUCXX AND NOT WIN32 AND NOT APPLE)

if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR lib)
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)

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

# On windows provide the user with the pdb files 
# for debugging if they are present
if(MSVC)
    install(
        DIRECTORY ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/\${CMAKE_INSTALL_CONFIG_NAME}/
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        USE_SOURCE_PERMISSIONS
        COMPONENT Development
        FILES_MATCHING PATTERN "*.pdb"
    )
endif(MSVC)

# Install the CMake find script for crunchstore
install( FILES "extra/Findcrunchstore.cmake"
    DESTINATION "share/${CMAKE_PROJECT_NAME}/extra"
    COMPONENT Development
)

#
# Create and install all of the auto find tools
# =============================================

# Add all targets to the build-tree export set
export(TARGETS Persistence
  FILE "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/crunchstoreLibraryDepends.cmake")

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE crunchstore)

# Create a crunchstoreBuildTreeSettings.cmake file for the use from the build tree
configure_file(crunchstoreBuildTreeSettings.cmake.in
  "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/crunchstoreBuildTreeSettings.cmake" @ONLY)

#configure_file(crunchstoreConfig.cmake.in
#  "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/crunchstoreConfig.cmake" @ONLY)
configure_file(crunchstoreConfigVersion.cmake.in
  "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/crunchstoreConfigVersion.cmake" @ONLY)
configure_file(Usecrunchstore.cmake.in
  "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/Usecrunchstore.cmake" @ONLY)

# Install the export set for use with the install-tree
install(EXPORT crunchstore-targets DESTINATION ${CMAKE_INSTALL_LIBDIR})

include( CMakePackageConfigHelpers )
# Create the crunchstoreConfig.cmake and crunchstoreConfigVersion files
file(RELATIVE_PATH CONF_REL_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
   "${CMAKE_INSTALL_PREFIX}/include")

configure_package_config_file( 
    crunchstoreConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/crunchstoreConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PATH_VARS CONF_REL_INCLUDE_DIR )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/crunchstoreConfig.cmake 
              ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/crunchstoreConfigVersion.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/Usecrunchstore.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR} )

#
# Setup the fpc auto detection files
if(WIN32)
    set(crunchstore_FPC_LIBRARIES "/libpath:\"\${libdir}\" Persistence.lib")
    set(crunchstore_FPC_INCLUDES "/I\"\${includedir}\"")
else(WIN32)
    set(crunchstore_FPC_INCLUDES "-I\${includedir}")
    set(crunchstore_FPC_LIBRARIES "-L\${libdir} -lPersistence")
endif(WIN32)

configure_file(extra/crunchstore.fpc.in
  "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/flagpoll/crunchstore.fpc" @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}/flagpoll/crunchstore.fpc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/flagpoll )

