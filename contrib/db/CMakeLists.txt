add_subdirectory(liblmdb)
if(MSVC)
   target_compile_options(lmdb PRIVATE /wd4996 /wd4503 /wd4345 /wd4267 /wd4244 /wd4146 /wd4333 /wd4172)
else()
   target_compile_options(lmdb PRIVATE -Wno-discarded-qualifiers -Wno-empty-body -Wno-unused-but-set-variable)
endif()

