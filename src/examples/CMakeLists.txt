#set(MONGO_DRIVER_LIBS
#    -L${MONGOCLIENT_PATH}/lib
#    mongoclient
#    )
    
#Lets us address our internal includes as <crunchstore/foo.h>
include_directories(${PROJECT_SOURCE_DIR}/src)

set( srcs main.cxx )

SET(TARGET_SRC ${srcs})

SET(TARGET_NAME crunchstoreTest)
ADD_EXECUTABLE(${TARGET_NAME} ${TARGET_SRC})
TARGET_LINK_LIBRARIES(${TARGET_NAME} crunchstore ${MONGO_DRIVER_LIBS} ${Boost_LIBRARIES})

SET(INSTALL_BINDIR bin)

INSTALL(
        TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION ${INSTALL_BINDIR}
    )

###############################################################################
set( srcs bulk.cxx )

SET(TARGET_SRC ${srcs})

SET(TARGET_NAME bulkTest)
ADD_EXECUTABLE(${TARGET_NAME} ${TARGET_SRC})
TARGET_LINK_LIBRARIES(${TARGET_NAME} crunchstore ${MONGO_DRIVER_LIBS} ${Boost_LIBRARIES})

SET(INSTALL_BINDIR bin)

INSTALL(
        TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION ${INSTALL_BINDIR}
    )


###############################################################################
if( NOT( "${POCO_PocoJSON_LIBRARY}" STREQUAL "" ))

set( srcs asMember.cxx )

SET(TARGET_SRC ${srcs})

SET(TARGET_NAME asMemberTest)
ADD_EXECUTABLE(${TARGET_NAME} ${TARGET_SRC})
TARGET_LINK_LIBRARIES(${TARGET_NAME} crunchstore ${MONGO_DRIVER_LIBS} ${Boost_LIBRARIES})

SET(INSTALL_BINDIR bin)

INSTALL(
        TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION ${INSTALL_BINDIR}
    )

install(FILES data.json
    DESTINATION bin
    COMPONENT development)
endif()

################################################################################
#set( srcs odbcTest.cxx )
#
#SET(TARGET_SRC ${srcs})
#
#SET(TARGET_NAME odbc)
#ADD_EXECUTABLE(${TARGET_NAME} ${TARGET_SRC})
#TARGET_LINK_LIBRARIES(${TARGET_NAME} ${POCO_LIBRARIES})
#
#SET(INSTALL_BINDIR bin)
#
#INSTALL(
#        TARGETS ${TARGET_NAME}
#        RUNTIME DESTINATION ${INSTALL_BINDIR}
#    )
