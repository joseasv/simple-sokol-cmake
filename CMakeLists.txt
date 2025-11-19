cmake_minimum_required(VERSION 3.10)
project(HolaSokol)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES main.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

# --- LÓGICA DE SELECCIÓN DE BACKEND ---

if(EMSCRIPTEN)
    # --- WEB (Emscripten) ---
    # Usa OpenGLES 3.0 (WebGL 2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SOKOL_GLES3)

    # Flags necesarios para que Emscripten configure el entorno web
    target_link_options(${PROJECT_NAME} PRIVATE
        "-sUSE_WEBGL2=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "--shell-file" "${CMAKE_CURRENT_SOURCE_DIR}/shell.html" # Opcional: si tienes un HTML template
    )

elseif(WIN32)
    # --- WINDOWS ---
    # Usa Direct3D 11 por defecto (es lo más estable para Sokol en Win)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SOKOL_D3D11)

    # Enlazar librerías de DirectX y sistema
    target_link_libraries(${PROJECT_NAME} d3d11 dxgi)

elseif(APPLE)
    # --- MACOS / IOS ---
    # Usa Metal (Obligatorio en versiones modernas de macOS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SOKOL_METAL)

    # Enlazar Frameworks de Apple
    target_link_libraries(${PROJECT_NAME}
        "-framework Cocoa"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
    )

elseif(UNIX AND NOT APPLE)
    # --- LINUX ---
    # Usa OpenGL Core Profile
    target_compile_definitions(${PROJECT_NAME} PRIVATE SOKOL_GLCORE)

    # Enlazar X11, GL, y Pthread
    target_link_libraries(${PROJECT_NAME} X11 Xi Xcursor GL dl pthread)

endif()