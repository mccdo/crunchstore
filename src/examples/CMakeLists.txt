#set(MONGO_DRIVER_LIBS
#    -L${MONGOCLIENT_PATH}/lib
#    mongoclient
#    )
    
#Lets us address our internal includes as <crunchstore/foo.h>
include_directories(${PROJECT_SOURCE_DIR}/src)

set( srcs main.cxx )

SET(TARGET_SRC ${srcs})
#SET(PUBLIC_HEADERS ${hdrs})

SET(TARGET_NAME crunchstoreTest)
ADD_EXECUTABLE(${TARGET_NAME} ${TARGET_SRC})
TARGET_LINK_LIBRARIES(${TARGET_NAME} crunchstore ${MONGO_DRIVER_LIBS} ${Boost_LIBRARIES})


#SET(INSTALL_INCDIR include)
SET(INSTALL_BINDIR bin)

INSTALL(
        TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION ${INSTALL_BINDIR}
    )

