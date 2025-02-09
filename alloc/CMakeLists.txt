add_compile_options(-fPIC)

add_library(alloc SHARED sources/methods.c sources/storage.c sources/memory_mgmt.c sources/linked_list_mgmt.c sources/utils.c sources/strats.c)
set_target_properties(alloc PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(alloc PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

target_link_libraries(alloc m)
