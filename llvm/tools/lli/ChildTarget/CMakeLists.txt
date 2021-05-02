set(LLVM_LINK_COMPONENTS
  OrcShared
  OrcJIT
  RuntimeDyld
  Support
  )

include_directories(BEFORE
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

add_llvm_utility(lli-child-target
  ChildTarget.cpp

  DEPENDS
  intrinsics_gen
)

