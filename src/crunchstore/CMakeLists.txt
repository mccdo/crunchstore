#set(MONGO_DRIVER_LIBS
#    -L${MONGOCLIENT_PATH}/lib
#    mongoclient
#    )
    
#Lets us address our internal includes as <Persistence/foo.h>
include_directories(${CMAKE_SOURCE_DIR}/src)

ADD_DEFINITIONS(-DCRUNCHSTORE_LIBRARY)

FILE( GLOB srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cxx" )
FILE( GLOB hdrs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.h" )

list(REMOVE_ITEM srcs "MongoStore.cxx")
list(REMOVE_ITEM hdrs "MongoStore.h")

set(VES_PRIVATE_HEADERS ${hdrs})
set(VES_PUBLIC_HEADERS ${hdrs} )

set(VES_TARGET_NAME crunchstore)
set(VES_TARGET_VERSION ${crunchstore_VERSION})
set(VES_TARGET_EXPORT ${CMAKE_PROJECT_NAME}-targets)
set(VES_TARGET_CATEGORY Lib)

ADD_LIBRARY(${VES_TARGET_NAME} SHARED ${srcs} ${VES_PUBLIC_HEADERS})
TARGET_LINK_LIBRARIES(${VES_TARGET_NAME} ${MONGO_DRIVER_LIBS} ${POCO_LIBRARIES} ${Boost_LIBRARIES})

include(VESModuleInstall)