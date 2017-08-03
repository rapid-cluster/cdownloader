add_executable(csa-async-donwload-url async_download_url.cxx)
target_link_libraries(csa-async-donwload-url cdownload)
install(TARGETS csa-async-donwload-url DESTINATION bin)


