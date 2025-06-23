cmake_minimum_required(VERSION 3.13)

project(OgreNextCEGUIDemo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(sdl2 REQUIRED)
find_package(OGRE 3.0 REQUIRED COMPONENTS OgreMain Bites Hlms HlmsPbs HlmsUnlit RenderSystemGL3Plus)
find_package(assimp REQUIRED)

pkg_check_modules(CEGUI REQUIRED CEGUI-0)
pkg_check_modules(CEGUIOPENGL3 REQUIRED CEGUI-0-OPENGL3)
pkg_check_modules(GLEW REQUIRED glew)

include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${OGRE_INCLUDE_DIRS}
    ${OGRE_INCLUDE_DIRS}/OGRE-Next/Hlms/Common
    ${OGRE_HlmsPbs_INCLUDE_DIRS}
    ${OGRE_HlmsUnlit_INCLUDE_DIRS}
    ${CEGUIOPENGL3_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${assimp_INCLUDE_DIRS}
)
link_directories(
    ${SDL2_LIBRARY_DIRS}
    ${OGRE_LIBRARY_DIRS}
    ${CEGUI_LIBRARY_DIRS}
    ${CEGUIOPENGL3_LIBRARY_DIRS}
    ${GLEW_LIBRARY_DIRS}
)

add_executable(${PROJECT_NAME}
    src/SceneLoader.cpp
    src/GUI.cpp
    src/FPSGame.cpp
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${SDL2_LIBRARIES}
    ${OGRE_LIBRARIES}
    ${OGRE_HlmsPbs_LIBRARIES}
    ${OGRE_HlmsUnlit_LIBRARIES}
    ${CEGUI_LIBRARIES}
    ${CEGUIOPENGL3_LIBRARIES}
    ${GLEW_LIBRARIES}
    assimp::assimp
)
