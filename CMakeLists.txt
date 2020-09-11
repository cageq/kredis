project (kredis)
cmake_minimum_required (VERSION 3.1)
 


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include ("../../env.cmake")
 

include_directories("${LIBEVENTROOT}/include")
include_directories("${LIBFMTROOT}/include") 


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/opt/fmt)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/opt/hiredis)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/opt/fmt/include)


link_directories(${WORKROOT}/deps/lib)

 
 

if(APPLE)
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wc++11-extensions -std=c++11 -stdlib=libc++  -g  -D NDEBUG")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wc++17-extensions -std=c++17 -stdlib=libc++  -g  ")
  include_directories("/usr/local/Cellar/openssl/1.0.2t/include")
  link_directories("/usr/local/Cellar/openssl/1.0.2t/lib") 
elseif(UNIX)
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=c++11 -pthread  -D NDEBUG")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 -pthread  -g -O2 ")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pthread -pg -g -DNDEBUG -Wunused-value ")
elseif(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -D NDEBUG")
      add_compile_options("/std:c++17") 
endif()

  

add_executable(kredis_client samples/kredis_client.cpp)

target_link_libraries(kredis_client ${FMTLIB} )
 
