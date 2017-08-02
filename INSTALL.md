### Prerequisites ###

The following tools and libraries are required to compile cdownloader:

1. ISO C++11 compliant compiler (tested with GCC 6 and 7)
2. [CMake](https://cmake.org/) build system.
3. [Common Data Format (CDF) I/O library for multi-dimensional data sets](http://cdf.gsfc.nasa.gov/) library for C.
4. [CURL](https://curl.haxx.se/) (or libcurl only if your package repository provides it in a separate package).
5. [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
6. [libarchive](http://www.libarchive.org)
7. [Boost](http://www.boost.org/). `date_time`, `filesystem`, `log`, and  `program_options` libraries are needed.


### Compilation ###

Out of source build is strongly suggested (in-source builds are not tested). The following commands in the source
directory should provide you with compiled binaries:

```bash
mkdir -p build/release
cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ../..
make
```
The following options (control amount of debug output) are available
- DEBUG_METADATA_ACTIONS Log metadata-related actions
- DEBUG_DOWNLOADING_ACTIONS Log downloader actions
- DEBUG_ARCHIVE_EXTRACTING Log archive extraction
- DEBUG_LOG_EVERY_CELL Log every cell times
- DEBUG_ALL OFF Turn on all the DEBUG_ options

The next options may be useful for builds with GCC 5, where std::regex is broken:
- USE_BOOST_REGEX Use regular expression library from boost (std::regex otherwise)


After succesful compilation you will get two executables: `cdownloader` and `tools/async-donwload-url`.
See readme document for a quick user guide.
