# Try to find NASA Common Data Format (CDF) Library
# Defines:  CDF_FOUND - system contains CDF
#           CDF_INCLUDE_DIR - Include Directories for CDF
#           CDF_LIB - Lirbraries required for CDF
#           CDF_COMPILE_DEFINITIONS - Compiler flags needed for CDF

if(CDF_INCLUDES)
    #already in cache
    set(CDF_FIND_QUIETLY TRUE)
endif(CDF_INCLUDES)

find_path(CDF_INCLUDES cdf.h)

if(CDF_USE_STATIC_LIBS)
    find_library(CDF_LIB NAMES ${CMAKE_STATIC_LIBRARY_PREFIX}cdf${CMAKE_STATIC_LIBRARY_SUFFIX})
else()
    find_library(CDF_LIB NAMES ${CMAKE_SHARED_LIBRARY_PREFIX}cdf${CMAKE_SHARED_LIBRARY_SUFFIX})
endif(CDF_USE_STATIC_LIBS)

set(CDF_LIBRARIES "${CDF_LIB}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args( CDF DEFAULT_MSG CDF_LIBRARIES CDF_INCLUDES )

mark_as_advanced(CDF_LIB CDF_INCLUDES)

if (CDF_FOUND)
	set(CDF_COMPILE_DEFINITIONS
		_FILE_OFFSET_BITS=64
		_LARGEFILE64_SOURCE
		_LARGEFILE_SOURCE
		)

	if(NOT TARGET CDF::CDF)
		add_library(CDF::CDF UNKNOWN IMPORTED)
		set_target_properties(CDF::CDF PROPERTIES
			INTERFACE_COMPILE_DEFINITIONS "${CDF_COMPILE_DEFINITIONS}"
			INTERFACE_INCLUDE_DIRECTORIES "${CDF_INCLUDES}"
			IMPORTED_LINK_INTERFACE_LANGUAGES "C"
			IMPORTED_LOCATION "${CDF_LIB}")
	endif(NOT TARGET CDF::CDF)
endif(CDF_FOUND)
