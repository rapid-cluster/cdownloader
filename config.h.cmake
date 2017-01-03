#ifndef CDOWNLOADER_CONFIG_H
#define CDOWNLOADER_CONFIG_H

#define DEFAULT_EXPANSION_DICTIONARY_FILE "@DEFAULT_EXPANSION_DICTIONARY_FILE@"

#cmakedefine DEBUG_METADATA_ACTIONS
#cmakedefine DEBUG_DOWNLOADING_ACTIONS
#cmakedefine DEBUG_ARCHIVE_EXTRACTING
#cmakedefine DEBUG_LOG_EVERY_CELL

#define SYSTEM_IS_BIG_ENDIAN @SYSTEN_BIG_ENDIAN@

#cmakedefine USE_BOOST_REGEX

#endif // CDOWNLOADER_CONFIG_H
