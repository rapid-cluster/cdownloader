macro(set_compiler_flags)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	exec_program(
		${CMAKE_CXX_COMPILER}
		ARGS      -dumpversion
		OUTPUT_VARIABLE GCC_VERSION)
	message(STATUS "C++ compiler version: ${GCC_VERSION} [${CMAKE_CXX_COMPILER}]")

	include(CheckCXXCompilerFlag)
	set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
	set(CMAKE_VISIBILITY_INLINES_HIDDEN True)
	check_cxx_compiler_flag(-fvisibility=hidden HAVE_GCC_VISIBILITY)
#     set(HAVE_GCC_VISIBILITY ${HAVE_GCC_VISIBILITY} CACHE BOOL "GCC support for hidden visibility")
# 	if(HAVE_GCC_VISIBILITY)
# 		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
# 		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
# 	endif(HAVE_GCC_VISIBILITY)
endif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
if (CMAKE_COMPILER_IS_GNUCXX)
	#-Wshadow -Wconversion ?
	set (_GCC_COMMON_CXX_FLAGS_LIST  "-fexceptions -frtti -pedantic -pedantic-errors"
		"-Wall -Wextra -Woverloaded-virtual -Wold-style-cast -Wstrict-null-sentinel"
		"-Wnon-virtual-dtor -Wfloat-equal -Wcast-qual -Wcast-align"
		"-Wsign-conversion -Winvalid-pch -Werror=return-type -Werror=overloaded-virtual -Wno-long-long"
# 		"-Weffc++"
		"-fext-numeric-literals" # boost needs this
# 	this is for developing
		"-fsanitize=address"
#	-fstack-protector-all
		"-Werror -Wno-error=deprecated-declarations -Wno-error=cpp")
	string(REPLACE ";" " " _GCC_COMMON_CXX_FLAGS "${_GCC_COMMON_CXX_FLAGS_LIST}")

	set (_GCC_48_CXX_FLAGS "${_GCC_COMMON_CXX_FLAGS} -Wpedantic")
	set (_GCC_47_CXX_FLAGS "${_GCC_COMMON_CXX_FLAGS} -pedantic")
	if (CMAKE_SYSTEM_NAME MATCHES Linux)
		set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--fatal-warnings -Wl,--no-undefined")
		set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--fatal-warnings -Wl,--no-undefined")
		# AB: _GNU_SOURCE turns on some other definitions (see
		# /usr/include/features.h). in particular we need _ISOC99_SOURCE
		# (atm at least for isnan(), possibly for more, too)
		#add_definitions(-D_GNU_SOURCE)
		#set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
		set (FIX_KDE_FLAGS "-fexceptions -UQT_NO_EXCEPTIONS ${CXX11_FLAGS}") #KDE disables exceptions,

		set ( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native -pipe" )
		set ( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -pipe" )

 		if (GCC_VERSION VERSION_LESS "4.8")
			set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_GCC_47_CXX_FLAGS}")
			set ( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g -ggdb -g3 -march=native -pipe" )
			set ( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -ggdb -g3  -march=native -pipe" )
 		else(GCC_VERSION VERSION_LESS "4.8")
			set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_GCC_48_CXX_FLAGS}")
 			message(STATUS "Using -O0 optimization level")
 			set ( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g -ggdb -g3 -march=native -pipe" )
 			set ( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -ggdb -g3  -march=native -pipe" )
 		endif(GCC_VERSION VERSION_LESS "4.8")

		set ( CMAKE_C_FLAGS_DEBUGFULL "${CMAKE_C_FLAGS_DEBUG}" )
		set ( CMAKE_CXX_FLAGS_DEBUGFULL "${CMAKE_CXX_FLAGS_DEBUG}" )

		set ( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -march=native -pipe" )
		set ( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -march=native -pipe" )

		# if Glibc version is 2.20 or higher, set -D_DEFAULT_SOURCE
		include(MacroGlibcDetect)
		message(STATUS "Detecting Glibc version...")
		glibc_detect(GLIBC_VERSION)
		if(${GLIBC_VERSION})
			if(GLIBC_VERSION LESS "220")
				message(STATUS "Glibc version is ${GLIBC_VERSION}")
			else(GLIBC_VERSION LESS "220")
				message(STATUS "Glibc version is ${GLIBC_VERSION}, adding -D_DEFAULT_SOURCE")
				add_definitions(-D_DEFAULT_SOURCE)
			endif(GLIBC_VERSION LESS "220")
		endif(${GLIBC_VERSION})
	endif (CMAKE_SYSTEM_NAME MATCHES Linux)

endif (CMAKE_COMPILER_IS_GNUCXX)


if (WIN32)
	set(CMAKE_LIBRARY_PATH "${CMAKE_LIBRARY_PATH};$ENV{LIB}")

	if (NOT CMAKE_SYSTEM MATCHES "CYGWIN*")
		if (MSVC)
			set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -wd4290 -wd4275 -wd4251 /W4" )
			set ( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Ox /Ot /Ob2" )
			set ( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox /Ot /Ob2" )
		endif (MSVC)

		if (CMAKE_C_COMPILER MATCHES "icl")
			#set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Wall /w2 /Wcheck" )
			#set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall /w2 /Wcheck" )
		endif (CMAKE_C_COMPILER MATCHES "icl")

#		if (NOT BORLAND)
#			set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /fixed:no /NODEFAULTLIB:LIBCMT" )
#		endif (NOT BORLAND)

		add_definitions(-D_USE_MATH_DEFINES)
		add_definitions(-DNOMINMAX)
		add_definitions(-DWIN32_LEAN_AND_MEAN)

#		find_library(PTHREAD_LIB_DIR NAMES pthreadVC2 PATHS ENV INCLUDE)
#		find_path(PTHREAD_INCLUDE_DIR pthread.h PATHS ENV INCLUDE)

#		if (PTHREAD_LIB_DIR AND PTHREAD_INCLUDE_DIR)
#			set(CMAKE_USE_PTHREADS_INIT 1)
#			set(CMAKE_THREAD_LIBS_INIT ${PTHREAD_LIB_DIR})
#			include_directories(${PTHREAD_INCLUDE_DIR})
#		endif (PTHREAD_LIB_DIR AND PTHREAD_INCLUDE_DIR)
	endif (NOT CMAKE_SYSTEM MATCHES "CYGWIN*")
endif (WIN32)
endmacro(set_compiler_flags)

