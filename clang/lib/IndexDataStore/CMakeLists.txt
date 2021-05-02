set(LLVM_LINK_COMPONENTS
  Support
  )

# For the #include "IndexDataStoreUtils.h"
include_directories(BEFORE
  "${CMAKE_CURRENT_SOURCE_DIR}/../Index"
)

add_clang_library(clangIndexDataStore
  IndexDataStore.cpp

  LINK_LIBS
  clangDirectoryWatcher
  clangIndex
  )
