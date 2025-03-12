zephyr_library_named(burst-link-protocol)
zephyr_library_sources(
"../../src/decoder.c"
"../../src/encoder.c"
"../../src/crc.c"
)

target_include_directories(burst-link-protocol PUBLIC ../../src)

