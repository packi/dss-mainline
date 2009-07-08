CONFIGURE_FILE(doxygen.conf.in ${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf)
CONFIGURE_FILE(doxygen_webservices.conf.in
	${CMAKE_CURRENT_BINARY_DIR}/doxygen_webservices.conf)


ADD_SUBDIRECTORY(images)

SET(INTERNAL_HTML_TARGET ${CMAKE_CURRENT_BINARY_DIR}/internal/html/index.html)

ADD_CUSTOM_COMMAND(
  OUTPUT ${INTERNAL_HTML_TARGET}
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf 
          images/block.png
	  images/block.eps
  COMMAND doxygen ${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf
  COMMENT "internal documentation")

SET(WEBSERVICES_HTML_TARGET ${CMAKE_CURRENT_BINARY_DIR}/webservices/html/index.html)

ADD_CUSTOM_TARGET(
  OUTPUT ${WEBSERVICES_HTML_TARGET}
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/doxygen_webservices.conf
  COMMAND doxygen ${CMAKE_CURRENT_BINARY_DIR}/doxygen_webservices.conf
  COMMENT "webservices documentation")

ADD_CUSTOM_TARGET(doc
  DEPENDS ${WEBSERVICES_HTML_TARGET}
  DEPENDS ${INTERNAL_HTML_TARGET}
  COMMENT "documentation")
