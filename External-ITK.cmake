#---------------------------------------------------------------------------
# Get and build itk

set(ITK_TAG "v4.13.2")
ExternalProject_Add(ITK
  GIT_REPOSITORY "${git_protocol}://github.com/Slicer/ITK.git"
  GIT_TAG "${ITK_TAG}"
  SOURCE_DIR itk/ITK
  BINARY_DIR itk/ITK-build
  CMAKE_GENERATOR ${gen}
  CMAKE_ARGS
    -DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
    -DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_STANDARD:STRING=${CMAKE_CXX_STANDARD}
    -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=${CMAKE_CXX_STANDARD_REQUIRED}
    -DCMAKE_CXX_EXTENSIONS:BOOL=${CMAKE_CXX_EXTENSIONS}
    -DBUILD_TESTING:BOOL=OFF
    -DBUILD_EXAMPLES:BOOL=OFF
    -DITK_LEGACY_REMOVE:BOOL=ON
    -DITKV3_COMPATIBILITY:BOOL=OFF
    -DITK_BUILD_DEFAULT_MODULES:BOOL=ON
    -DModule_ITKIONIFTI:BOOL=ON
    -DModule_ITKIONRRD:BOOL=ON
    -DBUILD_SHARED_LIBS:BOOL=OFF
    -DITK_INSTALL_NO_DEVELOPMENT:BOOL=ON
    -DITK_WRAPPING:BOOL=OFF #${BUILD_SHARED_LIBS} ## HACK:  QUICK CHANGE
    -DITK_USE_FFTWD:BOOL=OFF
    -DITK_USE_FFTWF:BOOL=OFF
  INSTALL_COMMAND ""
)

set(ITK_DIR ${CMAKE_BINARY_DIR}/itk/ITK-build)
