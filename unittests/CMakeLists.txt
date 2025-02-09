# Compile our wrapper
add_executable(test_that_crash test_that_crash.c)
# Similar to add_test(name command), but test is assumed successfull only if it is crashed(signalled)
macro(add_test_crashed name command)
   # Use generic flow of add_test() command for automatically recognize our executable target
   add_test(NAME ${name} COMMAND test_that_crash ${command})
endmacro(add_test_crashed)
# ...

add_executable(malloc alloc/malloc.c)
target_link_libraries(malloc alloc)
add_executable(calloc alloc/calloc.c)
target_link_libraries(calloc alloc)
add_executable(realloc alloc/realloc.c)
target_link_libraries(realloc alloc)
add_executable(free alloc/free.c)
target_link_libraries(free alloc)
add_executable(special_free alloc/special_free.c)
target_link_libraries(special_free alloc)
add_executable(special_realloc alloc/special_realloc.c)
target_link_libraries(special_realloc alloc)
add_executable(alignment alloc/alignment.c)
target_link_libraries(special_realloc alloc)


add_executable(bestfit strats/bestfit.c)
target_link_libraries(bestfit alloc)
add_executable(firstfit strats/firstfit.c)
target_link_libraries(firstfit alloc)
add_executable(nextfit strats/nextfit.c)
target_link_libraries(nextfit alloc)
add_executable(worstfit strats/worstfit.c)
target_link_libraries(worstfit alloc)

add_executable(add_entry components/add_entry.c)
target_link_libraries(add_entry alloc)
add_executable(remove_entry components/remove_entry.c)
target_link_libraries(remove_entry alloc)
add_executable(expand_list components/expand_list.c)
target_link_libraries(expand_list alloc)


add_test(NAME malloc COMMAND malloc)
add_test(NAME calloc COMMAND calloc)
add_test(NAME realloc COMMAND realloc)
add_test(NAME free COMMAND free)
add_test_crashed(special_free special_free)
add_test_crashed(special_realloc special_realloc)
add_test(NAME alignment COMMAND alignment)


add_test(NAME bestfit COMMAND bestfit)
add_test(NAME firstfit COMMAND firstfit)
add_test(NAME nextfit COMMAND nextfit)
add_test(NAME worstfit COMMAND worstfit)

add_test(NAME add_entry COMMAND add_entry)
add_test(NAME remove_entry COMMAND remove_entry)
add_test(NAME expand_list COMMAND expand_list)

set_property(TEST malloc calloc realloc free special_free special_realloc bestfit firstfit nextfit worstfit add_entry remove_entry expand_list alignment
   PROPERTY
   ENVIRONMENT LD_PRELOAD=${CMAKE_SOURCE_DIR}/build/alloc/liballoc.so
)

set_property(TEST special_free special_realloc
   PROPERTY
   WILL_FAIL TRUE
)
