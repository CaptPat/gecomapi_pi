##---------------------------------------------------------------------------
## Author:      Pavel Kalian
## Copyright:   
## License:     wxWidgets License
##---------------------------------------------------------------------------

# define minimum cmake version
CMAKE_MINIMUM_REQUIRED(VERSION 2.6.2)

PROJECT(gecomapi_pi)

SET(PACKAGE_NAME gecomapi_pi)
SET(VERBOSE_NAME GoogleEarthPlugin)
SET(TITLE_NAME GoogleEarthPlugin)
SET(CPACK_PACKAGE_CONTACT "Pavel Kalian")

SET(VERSION_MAJOR "1")
SET(VERSION_MINOR "2")

#SET(CMAKE_BUILD_TYPE Debug)

OPTION(GECOMAPI_USE_SVG "Use SVG graphics" ON)

INCLUDE("cmake/PluginConfigure.cmake")

IF(GECOMAPI_USE_SVG)
ADD_DEFINITIONS(-DGECOMAPI_USE_SVG)
ENDIF(GECOMAPI_USE_SVG)

SET(SRC_GECOMAPI
            src/gecomapi_pi.h
            src/gecomapi_pi.cpp
            src/icons.h
            src/icons.cpp
            src/gecomapi.h
            src/gecomapi.cpp
            src/gecomapicfgdlg.h
            src/gecomapicfgdlg.cpp
	    src/tinystr.h
            src/tinystr.cpp
            src/tinyxml.h
            src/tinyxml.cpp
            src/tinyxmlparser.cpp
            src/tinyxmlerror.cpp
            src/gpxdocument.h
            src/gpxdocument.cpp
        )
ADD_LIBRARY(${PACKAGE_NAME} SHARED ${SRC_GECOMAPI})

INCLUDE("cmake/PluginInstall.cmake")
INCLUDE("cmake/PluginLocalization.cmake")
INCLUDE("cmake/PluginPackage.cmake")
