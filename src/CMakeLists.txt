add_library(xpltimer SHARED
  timer.c
)


set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ALL_CFLAGS} ${PTHREAD_CFLAGS} ${OPENSSL_CFLAGS}")

target_link_libraries(xpltimer
	xplmem${BIT}
)

install(TARGETS xpltimer DESTINATION /usr/lib)
