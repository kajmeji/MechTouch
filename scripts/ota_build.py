"""Keep LTO enabled at archive and link stages, not only during compilation."""
Import("env")
env.Replace(AR="xtensa-esp-elf-gcc-ar", RANLIB="xtensa-esp-elf-gcc-ranlib")
env.Append(CCFLAGS=["-Oz"])
env.Append(LINKFLAGS=["-flto", "-fuse-linker-plugin", "-Oz", "-fno-strict-aliasing"])
