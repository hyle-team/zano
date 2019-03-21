add_subdirectory(libethash)

set_property(TARGET ethash PROPERTY FOLDER "contrib")

if(MSVC)
   target_compile_options(ethash PRIVATE /wd4477 /wd4267)
else()
   target_compile_options(ethash PRIVATE -Wno-format -Wno-aggregate-return -Wno-empty-body)
endif() 
	
