cmake_minimum_required(VERSION 3.6.0)
project(cdownloader)

set(CMAKE_CXX_STANDARD "11")
set(CMAKE_CXX_STANDARD_REQUIRED True)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)
include(CompilerSettings)
set_compiler_flags()

find_package(CURL REQUIRED)
find_package(jsoncpp)
if (NOT jsoncpp_FOUND)
	message(STATUS "Trying to find jsoncpp via pkg-config...")
	find_package(PkgConfig)
	pkg_check_modules(jsoncpp REQUIRED jsoncpp)
	message(STATUS "LIBRARIES: ${jsoncpp_LIBRARIES} INCLUDE_DIRS: ${jsoncpp_INCLUDE_DIRS} CFLAGS: ${jsoncpp_CFLAGS} LIBRARY_DIRS: ${jsoncpp_LIBRARY_DIRS}")
	add_library(jsoncpp_lib UNKNOWN IMPORTED)
	find_library(jsoncpp_LIB_LOCATION "${jsoncpp_LIBRARIES}" "${jsoncpp_LIBRARY_DIRS}")
	set_target_properties(jsoncpp_lib PROPERTIES
			IMPORTED_LINK_INTERFACE_LANGUAGES "C"
			IMPORTED_LOCATION "${jsoncpp_LIB_LOCATION}"
			INTERFACE_INCLUDE_DIRECTORIES "${jsoncpp_INCLUDE_DIRS}"
	)
endif(NOT jsoncpp_FOUND)
find_package(LibArchive REQUIRED)
find_package(CDF 3.3.1 REQUIRED)

find_package(Boost COMPONENTS date_time filesystem log program_options REQUIRED)
include(CheckIncludeFileCXX)

check_include_file_cxx(unistd.h HAVE_UNISTD_H)
check_include_file_cxx(sys/types.h HAVE_SYS_TYPES_H)
check_include_file_cxx(pwd.h HAVE_PWD_H)

include(TestBigEndian)
test_big_endian(SYSTEN_BIG_ENDIAN)

include_directories(SYSTEM ${CURL_INCLUDE_DIRS} ${LibArchive_INCLUDE_DIRS})
add_definitions(-DBOOST_LOG_DYN_LINK=1)

add_library(cdownload STATIC "")

target_sources(cdownload
	PRIVATE
		average.hxx
		average.cxx
		commonDefinitions.hxx
		csatime.hxx
		csatime.cxx
		chunkdownloader.hxx
		chunkdownloader.cxx
		dataprovider.hxx
		dataprovider.cxx
		datareader.hxx
		datareader.cxx
		datasource.hxx
		datasource.cxx
		downloader.cxx
		downloader.hxx
		expanding.hxx
		expanding.cxx
		field.hxx
		field.cxx
		fieldbuffer.hxx
		fieldbuffer.cxx
		filter.hxx
		filter.cxx
		floatcomparison.hxx
		epochrange.hxx
		metadata.hxx
		metadata.cxx
		util.hxx
		util.cxx
		writer.hxx
		writer.cxx
		parameters.hxx
		parameters.cxx
		reader.hxx
		reader.cxx
		filters/baddata.hxx
		filters/baddata.cxx
		filters/blankdata.hxx
		filters/blankdata.cxx
		filters/density.hxx
		filters/density.cxx
		filters/plasmasheet.hxx
		filters/plasmasheet.cxx
		filters/nightside.hxx
		filters/nightside.cxx
		filters/quality.hxx
		filters/quality.cxx
		filters/timefilter.hxx
		filters/timefilter.cxx
		writers/ASCIIWriter.hxx
		writers/ASCIIWriter.cxx
		writers/BinaryWriter.hxx
		writers/BinaryWriter.cxx
		cdf/reader.hxx
		cdf/reader.cxx
		csa/chunkdownloader.hxx
		csa/chunkdownloader.cxx
		csa/dataprovider.hxx
		csa/dataprovider.cxx
		csa/datasource.cxx
		csa/datasource.hxx
		csa/downloader.cxx
		csa/downloader.hxx
		csa/metadata.cxx
		csa/metadata.hxx
		csa/unpacker.hxx
		csa/unpacker.cxx
		omni/omnidb.hxx
		omni/omnidb.cxx
		omni/hroreader.hxx
		omni/hroreader.cxx
		omni/dataprovider.hxx
		omni/dataprovider.cxx
		omni/datasource.hxx
		omni/datasource.cxx
		omni/downloader.hxx
		omni/downloader.cxx
		omni/metadata.hxx
		omni/metadata.cxx
)

target_link_libraries(cdownload
	PRIVATE
		${CURL_LIBRARIES}
		${LibArchive_LIBRARIES}
		Boost::log
		CDF::CDF
	PUBLIC
		Boost::filesystem
		Boost::system
		jsoncpp_lib
)

if(UNIX)
	set(DEFAULT_EXPANSION_DICTIONARY_FILE "/usr/share/cdownload/expansion.dict")
endif()


# debug options
option(DEBUG_METADATA_ACTIONS "Log metadata-related actions" OFF)
option(DEBUG_DOWNLOADING_ACTIONS "Log downloader actions" OFF)
option(DEBUG_ARCHIVE_EXTRACTING "Log archive extraction" OFF)
option(DEBUG_LOG_EVERY_CELL "Log every cell times" OFF)
option(USE_BOOST_REGEX "Use regex library from boost (std:: otherwise)" OFF)

option(DEBUG_ALL OFF "Turn on all the DEBUG_ options")
if (DEBUG_ALL)
	set(DEBUG_METADATA_ACTIONS ON)
	set(DEBUG_DOWNLOADING_ACTIONS ON)
	set(DEBUG_ARCHIVE_EXTRACTING ON)
	set(DEBUG_LOG_EVERY_CELL ON)
endif(DEBUG_ALL)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(cdownloader
	cdownloader-main.cxx
	driver.hxx
	driver.cxx
	program_options_helpers.cxx
)
target_link_libraries(cdownloader cdownload Boost::program_options)

install(TARGETS cdownloader RUNTIME DESTINATION bin)

option(BUILD_MANUAL_TESTS ON)
if(BUILD_MANUAL_TESTS)
	add_subdirectory(tests)
endif(BUILD_MANUAL_TESTS)

add_subdirectory(tools)
