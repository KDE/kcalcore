# Find the kdepimlibs module

#are we trying to compile kdepimlibs?
#then enter bootstrap mode
if(EXISTS ${CMAKE_SOURCE_DIR}/kdepimlibs.lsm)


else(EXISTS ${CMAKE_SOURCE_DIR}/kdepimlibs.lsm)

  set(KDE4_EMAILFUNCTIONS_LIBS emailfunctions)
  set(KDE4_KCAL_LIBS kcal)
  
endif(EXISTS ${CMAKE_SOURCE_DIR}/kdepimlibs.lsm)
