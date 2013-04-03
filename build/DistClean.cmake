#
# "distclean" target for removing the generated files from CMake
#

if(UNIX)
	add_custom_target(distclean COMMENT "Cleaning up for distribution")
	
	# Clean up files that can appear at multiple places:
	foreach(CLEAN_FILE CMakeFiles CMakeCache.txt '*.a' '*.1.gz'
			cmake_install.cmake Makefile CPackConfig.cmake
			CPackSourceConfig.cmake _CPack_Packages Testing
			CTestTestfile.cmake install_manifest.txt)
		add_custom_command(TARGET distclean POST_BUILD
			COMMAND find . -depth -name ${CLEAN_FILE} | xargs rm -rf
			DEPENDS clean)
	endforeach(CLEAN_FILE)

endif(UNIX)
