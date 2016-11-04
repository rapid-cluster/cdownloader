add_executable(download-meta download_meta.cxx)
target_link_libraries(download-meta cdownload)

add_executable(download-sample download_sample.cxx)
target_link_libraries(download-sample cdownload)

add_executable(cdf-info cdf_info_text.cxx)
target_link_libraries(cdf-info cdownload)


add_executable(chunk-downloader-test chunk_downloader_test.cxx)
target_link_libraries(chunk-downloader-test cdownload)
