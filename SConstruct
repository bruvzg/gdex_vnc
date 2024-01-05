#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For the reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")

thirdparty_vnc_dir = "libvncserver/"

thirdparty_png_dir = "libpng/"
thirdparty_png_sources = [
    "png.c",
    "pngerror.c",
    "pngget.c",
    "pngmem.c",
    "pngpread.c",
    "pngread.c",
    "pngrio.c",
    "pngrtran.c",
    "pngrutil.c",
    "pngset.c",
    "pngtrans.c",
    "pngwio.c",
    "pngwrite.c",
    "pngwtran.c",
    "pngwutil.c",
]
sources += [thirdparty_png_dir + file for file in thirdparty_png_sources]

thirdparty_zlib_dir = "zlib/"
thirdparty_zlib_sources = [
    "adler32.c",
    "compress.c",
    "crc32.c",
    "deflate.c",
    "inffast.c",
    "inflate.c",
    "inftrees.c",
    "trees.c",
    "uncompr.c",
    "zutil.c",
]
sources += [thirdparty_zlib_dir + file for file in thirdparty_zlib_sources]

env.Append(CPPPATH=[thirdparty_vnc_dir + "include/", thirdparty_vnc_dir + "build/include/", thirdparty_png_dir, thirdparty_zlib_dir])
env.Append(LIBPATH=[thirdparty_vnc_dir + "build/"])
env.Append(LIBS=["vncclient"])

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "project/bin/libgdex_vnc.{}.{}.framework/libgdex_vnc.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "project/bin/libgdex_vnc.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "project/bin/libgdex_vnc.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )
else:
    library = env.SharedLibrary(
        "project/bin/libgdex_vnc{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
