#set(MONGO_DRIVER_LIBS
#    -L${MONGOCLIENT_PATH}/lib
#    mongoclient
#    )
    
#Lets us address our internal includes as <Persistence/foo.h>
include_directories(
        ${CMAKE_SOURCE_DIR}/src
    )

#Lets us know we're building the library so export definition can be set up
#correctly for win32
#IF(WIN32)
ADD_DEFINITIONS(-DPERSISTENCE_LIBRARY)
#ENDIF (WIN32)

FILE( GLOB srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cxx" )
FILE( GLOB hdrs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.h" )

list(REMOVE_ITEM srcs "MongoStore.cxx")
list(REMOVE_ITEM hdrs "MongoStore.h")


SET(TARGET_SRC ${srcs})
SET(PUBLIC_HEADERS ${hdrs})

SET(TARGET_NAME Persistence)
ADD_LIBRARY(${TARGET_NAME} SHARED ${TARGET_SRC} ${PUBLIC_HEADERS})
TARGET_LINK_LIBRARIES(${TARGET_NAME} ${MONGO_DRIVER_LIBS} ${POCO_LIBRARIES} ${Boost_LIBRARIES})


SET(INSTALL_INCDIR include)
SET(INSTALL_BINDIR bin)
SET(INSTALL_ARCHIVEDIR ${CMAKE_INSTALL_LIBDIR})
IF(WIN32)
    SET(INSTALL_LIBDIR bin)
ELSE()
    SET(INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
ENDIF()

set_target_properties( ${TARGET_NAME} PROPERTIES VERSION ${crunchstore_VERSION} )
set_target_properties( ${TARGET_NAME} PROPERTIES SOVERSION ${crunchstore_VERSION} )
set_target_properties( ${TARGET_NAME} PROPERTIES PROJECT_LABEL "Lib ${TARGET_NAME}" )

INSTALL(
        TARGETS ${TARGET_NAME}
        EXPORT crunchstore-targets
        RUNTIME DESTINATION ${INSTALL_BINDIR}
        LIBRARY DESTINATION ${INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR}
    )

INSTALL(
        FILES       ${PUBLIC_HEADERS}
        DESTINATION ${INSTALL_INCDIR}/${TARGET_NAME}
    )
