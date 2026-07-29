add_library(
  lfif_container
  STATIC
  src/codec.cc
  src/format.cc
)
target_include_directories(lfif_container PUBLIC include)
target_link_libraries(lfif_container PUBLIC lfif)
