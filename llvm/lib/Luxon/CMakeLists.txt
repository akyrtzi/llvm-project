if (LLVM_ENABLE_ONDISK_CAS)
  add_definitions(-DLLVM_ENABLE_ONDISK_CAS=1)
endif()

add_llvm_component_library(LLVMLuxon
  Luxon.cpp
  LuxonActionCache.cpp
  LuxonCAS.cpp

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Luxon

  LINK_COMPONENTS
  CAS
  RemoteCachingService
  Support
)
