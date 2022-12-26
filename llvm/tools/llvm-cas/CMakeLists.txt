set(LLVM_LINK_COMPONENTS
  Support
  CAS
  Luxon
  RemoteCachingService
  )

add_llvm_tool(llvm-cas
  llvm-cas.cpp
  )
