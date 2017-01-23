cmake_minimum_required(VERSION 3.4.1)

add_library( rtmp
             SHARED
             src/main/cpp/librtmp/amf.c
             src/main/cpp/librtmp/hashswf.c
             src/main/cpp/librtmp/log.c
             src/main/cpp/librtmp/parseurl.c
             src/main/cpp/librtmp/rtmp.c
             src/main/cpp/librtmp_jni.c )

include_directories(src/main/cpp/librtmp/)

find_library( log-lib
              log )

target_link_libraries( rtmp
                       ${log-lib} )
