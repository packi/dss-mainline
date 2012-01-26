SET(SCRIPTING_SRCS
  jshandler.cpp
  scriptobject.cpp
  jsevent.cpp
  jsproperty.cpp
  jslogger.cpp
  jsmetering.cpp
  jssocket.cpp
  jsmodel.cpp
)

if (CURL_FOUND)
  SET(SCRIPTING_SRCS ${SCRIPTING_SRCS}
  jscurl.cpp
)
endif (CURL_FOUND)

add_library(scripting ${SCRIPTING_SRCS})
