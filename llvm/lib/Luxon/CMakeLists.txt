find_package(SQLite3 REQUIRED)

add_subdirectory(BLAKE2)

if (LLVM_ENABLE_ONDISK_CAS)
  add_definitions(-DLLVM_ENABLE_ONDISK_CAS=1)
endif()

add_llvm_component_library(LLVMLuxon
  FileBackedCAS.cpp
  LMDBCAS.cpp
  LuxonCAS.cpp
  LuxonObjectStore.cpp
  SQLiteCAS.cpp

  lmdb/mdb.c
  lmdb/midl.c

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Luxon

  LINK_COMPONENTS
  CAS
  LuxonBLAKE2
  RemoteCachingService
  Support
)

target_link_libraries(LLVMLuxon PRIVATE
  SQLite::SQLite3
)
