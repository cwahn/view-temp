# macro(subdirlist result curdir)
#     file(GLOB children RELATIVE ${curdir} ${curdir}/*)
#     set(dirlist "")

#     foreach(child ${children})
#         if(IS_DIRECTORY ${curdir}/${child})
#             list(APPEND dirlist ${child})
#         endif()
#     endforeach()

#     set(${result} ${dirlist})
# endmacro()

# subdirlist(SUBDIRS ./)

# foreach(subdir ${SUBDIRS})
#     add_subdirectory(${subdir})
# endforeach()

add_subdirectory(examples)
add_subdirectory(mk0)
add_subdirectory(mk1)
add_subdirectory(mk2)
