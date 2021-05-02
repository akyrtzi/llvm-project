include_directories(BEFORE
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

add_clang_library(clangIncludeFixerPlugin
  IncludeFixerPlugin.cpp

  LINK_LIBS
  clangAST
  clangBasic
  clangFrontend
  clangIncludeFixer
  clangParse
  clangSema
  clangTooling
  ${LLVM_PTHREAD_LIB}

  DEPENDS
  omp_gen
  )
