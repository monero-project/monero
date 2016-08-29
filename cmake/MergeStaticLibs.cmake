#    Copyright (C) 2012 Modelon AB

#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the BSD style license.

#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    FMILIB_License.txt file for more details.

#    You should have received a copy of the FMILIB_License.txt file
#    along with this program. If not, contact Modelon AB <http://www.modelon.com>.

# Merge_static_libs(outlib lib1 lib2 ... libn) merges a number of static
# libs into a single static library
function(merge_static_libs outlib )
	set(libs ${ARGV})
	list(REMOVE_AT libs 0)
	list(REMOVE_DUPLICATES libs) # just in case

	# First get the file names of the libraries to be merged
	foreach(lib ${libs})
		get_target_property(libtype ${lib} TYPE)
		if(NOT libtype STREQUAL "STATIC_LIBRARY")
			message(FATAL_ERROR "merge_static_libs can only process static libraries")
		endif()
		set(libfiles "${libfiles} $<TARGET_FILE:${lib}>")
	endforeach()

	if(MSVC) # lib.exe does the merging of given a list
		set_target_properties(${outlib} PROPERTIES STATIC_LIBRARY_FLAGS "${libfiles}")

	elseif(APPLE) # Use OSX's libtool to merge archives
		add_custom_command(TARGET ${outlib} POST_BUILD
			COMMAND rm "$<TARGET_FILE:${outlib}>"
			COMMAND /usr/bin/libtool -static -o "$<TARGET_FILE:${outlib}>" ${libfiles})

	else() # general UNIX: use "ar" to extract objects and re-add to a common lib
		foreach(lib ${libs})
			set(objlistfile ${lib}.objlist) # list of objects in the input library
			set(objdir ${lib}.objdir)

			add_custom_command(OUTPUT ${objdir}
				COMMAND ${CMAKE_COMMAND} -E make_directory ${objdir})

			add_custom_command(OUTPUT ${objlistfile}
				COMMAND ${CMAKE_AR} -x "$<TARGET_FILE:${lib}>"
				COMMAND ${CMAKE_AR} -t "$<TARGET_FILE:${lib}>" > ../${objlistfile}
				DEPENDS ${lib} ${objdir}
				WORKING_DIRECTORY ${objdir})

			# Empty dummy source file that goes into merged library
			set(mergebase ${lib}.mergebase.c)
			add_custom_command(OUTPUT ${mergebase}
				COMMAND ${CMAKE_COMMAND} -E touch ${mergebase}
				DEPENDS ${objlistfile})

			list(APPEND mergebases "${mergebase}")
		endforeach()

		# We need a target for the output merged library
		add_library(${outlib} STATIC ${mergebases})
		set(outlibfile "$<TARGET_FILE:${outlib}>")

		foreach(lib ${libs})
			add_custom_command(TARGET ${outlib} POST_BUILD
				COMMAND ${CMAKE_AR} ru ${outlibfile} @"../${objlistfile}"
				WORKING_DIRECTORY ${objdir})
		endforeach()

		add_custom_command(TARGET ${outlib} POST_BUILD
			COMMAND ${CMAKE_RANLIB} ${outlibfile})
	endif()
endfunction()
