
spec2def(framebuf.dll framebuf.spec)

list(APPEND SOURCE
    enable.c
    palette.c
    pointer.c
    screen.c
    surface.c
    framebuf.h)

add_library(framebuf SHARED
    ${SOURCE}
    framebuf.rc
    ${CMAKE_CURRENT_BINARY_DIR}/framebuf.def)

add_pch(framebuf framebuf.h SOURCE)
set_entrypoint(framebuf DrvEnableDriver 12)
set_subsystem(framebuf native)
set_image_base(framebuf 0x00010000)
target_link_libraries(framebuf libcntpr)
add_importlibs(framebuf win32k)
add_dependencies(framebuf psdk bugcodes)
add_cd_file(TARGET framebuf DESTINATION reactos/system32 FOR all)
