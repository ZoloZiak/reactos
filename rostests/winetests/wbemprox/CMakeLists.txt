
add_definitions(-DUSE_WINE_TODOS)

list(APPEND SOURCE
    query.c
    services.c
    testlist.c)

add_executable(wbemprox_winetest ${SOURCE})
target_link_libraries(wbemprox_winetest uuid)
set_module_type(wbemprox_winetest win32cui)
add_importlibs(wbemprox_winetest oleaut32 ole32 user32 msvcrt kernel32)
add_cd_file(TARGET wbemprox_winetest DESTINATION reactos/bin FOR all)
