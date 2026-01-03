cmake_minimum_required(VERSION 3.22.0)
project(hello-gui-sdl
    VERSION     0.0.1
    LANGUAGES   C CXX
    DESCRIPTION "Hello GUI SDL2"
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

set(DEFAULT_BUILD_TYPE "Debug")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
endif()

set(SRC_FILES
    main.cpp
)

if(ANDROID)

    find_library(SDL2 SDL2)

    add_library(main SHARED)

    target_sources(main PRIVATE ${SRC_FILES})

    target_link_libraries(main SDL2)

else()
    # Desktop (Windows / Linux / macOS)

    # set(SDL_PATH "D:/PROJECTS/compiler-shenannigans/SDL2-2.30.5")
    if(NOT DEFINED SDL_PATH)
        set(SDL_PATH "" CACHE PATH "Path to SDL2 root")
    endif()
    if(SDL_PATH)
        set(CMAKE_C_COMPILER_TARGET msvc)
        list(APPEND CMAKE_PREFIX_PATH "${SDL_PATH}/build-${CMAKE_C_COMPILER_TARGET}/install")
        message(STATUS "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
    endif()

    # Find the package
    find_package(SDL2 REQUIRED)
    message(STATUS "SDL2 found")
    message(STATUS "\tSDL2_DIR: ${SDL2_DIR}\n\tSDL2_INCLUDE_DIRS: ${SDL2_INCLUDE_DIRS}\n\tSDL2_LIBRARIES: ${SDL2_LIBRARIES}")


    add_executable(${PROJECT_NAME} ${SRC_FILES})

    target_include_directories(${PROJECT_NAME} PRIVATE)

    # Link the library
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        SDL2::SDL2
        SDL2::SDL2main
    )
    
    
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<BOOL:${WERROR}>:-Werror>
    )
    
    # Install Path
    set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/${CMAKE_C_COMPILER_TARGET})
    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME}
        RUNTIME         DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY         DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE         DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    
    
    # CRT runtime (for MSVC only)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        #  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>")
    endif()

endif()
