cmake_minimum_required(VERSION 3.20)

project(felimage-noise
        VERSION 0.2.0
        DESCRIPTION "Felimage Noise plug-in for GIMP 3"
        LANGUAGES C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GIMP REQUIRED IMPORTED_TARGET gimp-3.0 gimpui-3.0)

pkg_get_variable(GIMP_LIBDIR gimp-3.0 gimplibdir)
set(GIMP_PLUGIN_DIR "${GIMP_LIBDIR}/plug-ins" CACHE PATH
    "GIMP plug-ins directory (set to ~/.config/GIMP/3.2/plug-ins for a user install)")

configure_file(config.h.in config.h)

add_executable(felimage-noise
    src/basis.c
    src/cell_3d.c
    src/cell_4d.c
    src/cell_5d.c
    src/interface.c
    src/lnoise_3d.c
    src/lnoise_4d.c
    src/lnoise_5d.c
    src/loadconf.c
    src/main.c
    src/poisson.c
    src/random.c
    src/render.c
    src/saveconf.c
    src/snoise_3d.c
    src/snoise_4d.c
    src/snoise_5d.c)

set_target_properties(felimage-noise PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS ON)

target_include_directories(felimage-noise PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(felimage-noise PRIVATE -Wall)
target_link_libraries(felimage-noise PRIVATE PkgConfig::GIMP m)

# GIMP 3 loads plug-ins from a per-plug-in subdirectory named after the executable.
install(TARGETS felimage-noise
        DESTINATION "${GIMP_PLUGIN_DIR}/felimage-noise")
