include_directories(
"${CMAKE_SOURCE_DIR}/scribus"
)

#Install our page preset files, selected from *.xml in this directory
file( GLOB SCRIBUS_PAGEPRESET_FILES *.xml )
install(FILES ${SCRIBUS_PAGEPRESET_FILES} DESTINATION ${PAGEPRESETSDIR})
