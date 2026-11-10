option( PLUGIN_STANDARD_QGAE "Check to install qGAE plugin" OFF )

if( PLUGIN_STANDARD_QGAE )
	if( NOT OPTION_USE_SHAPE_LIB )
		message( FATAL_ERROR "ShapeLib is required to compile the qGAE plugin (enable OPTION_USE_SHAPE_LIB)" )
	endif()

	project( QGAE_PLUGIN )

	AddPlugin( NAME ${PROJECT_NAME} )
	target_compile_definitions( ${PROJECT_NAME} PRIVATE QT_NO_KEYWORDS )

	target_link_libraries( ${PROJECT_NAME} QPCP_LIB )

	add_subdirectory( include )
	add_subdirectory( src )
	add_subdirectory( ui )
endif()