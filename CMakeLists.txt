cmake_minimum_required(VERSION 3.10)

# --- 1. Configuración Dinámica del Archivo Fuente ---
set(APP_SRC "main.cpp" CACHE STRING "Archivo fuente de entrada")

# Extraer el nombre SIN extensión (ej: de "01_Triangulo.cpp" saca "01_Triangulo")
get_filename_component(APP_NAME ${APP_SRC} NAME_WE)

project(${APP_NAME})

message(STATUS "------------------------------------------------")
message(STATUS "Proyecto: ${APP_NAME}")
message(STATUS "Fuente C++: ${APP_SRC}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 2. DETECCIÓN Y COMPILACIÓN DE SHADERS (sokol-shdc) ---

# Usamos APP_NAME para buscar el shader (ej: 01_Triangulo.glsl)
set(SHADER_FILE "${APP_NAME}.glsl")
set(SHADER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE}")
set(SHADER_HEADER "${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.glsl.h")

# Definir ruta de la herramienta según el SO
if(WIN32)
    set(SHDC_TOOL "${CMAKE_SOURCE_DIR}/tools/sokol-shdc.exe")
elseif(APPLE)
    set(SHDC_TOOL "${CMAKE_SOURCE_DIR}/tools/sokol-shdc")
else()
    set(SHDC_TOOL "${CMAKE_SOURCE_DIR}/tools/sokol-shdc")
endif()

# Variable para guardar los shaders generados (si existen)
set(APP_GENERATED_FILES "")

if(EXISTS "${SHADER_SRC}")
    message(STATUS "Shader encontrado: ${SHADER_FILE}")
    message(STATUS "Herramienta shdc: ${SHDC_TOOL}")

    # Definir lenguajes de salida
    set(SLANG "glsl430:glsl300es:hlsl5:metal_macos")

    add_custom_command(
        OUTPUT ${SHADER_HEADER}
        COMMAND ${SHDC_TOOL} --input ${SHADER_SRC} --output ${SHADER_HEADER} --slang ${SLANG}
        DEPENDS ${SHADER_SRC}
        COMMENT "Compilando Shader ${SHADER_FILE}..."
    )

    # Añadimos el header a la lista de archivos generados
    list(APPEND APP_GENERATED_FILES ${SHADER_HEADER})
else()
    message(WARNING "AVISO: No se encontró ${SHADER_FILE}. Si tu código usa shaders, fallará al compilar.")
endif()

# --- 3. Crear el Ejecutable ---

# Añadimos tanto el .cpp como el .h generado al ejecutable
add_executable(${APP_NAME} ${APP_SRC} ${APP_GENERATED_FILES})

# Incluimos el directorio de build para poder hacer #include "archivo.glsl.h"
target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# --- 4. Selección de Backend ---

if(EMSCRIPTEN)
    target_compile_definitions(${APP_NAME} PRIVATE SOKOL_GLES3)
    target_link_options(${APP_NAME} PRIVATE
        "-sUSE_WEBGL2=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "--shell-file" "${CMAKE_CURRENT_SOURCE_DIR}/shell.html"
    )

elseif(WIN32)
    target_compile_definitions(${APP_NAME} PRIVATE SOKOL_D3D11)
    target_link_libraries(${APP_NAME} d3d11 dxgi)

elseif(APPLE)
    target_compile_definitions(${APP_NAME} PRIVATE SOKOL_METAL)
    target_link_libraries(${APP_NAME} "-framework Cocoa" "-framework Metal" "-framework MetalKit" "-framework QuartzCore")

elseif(UNIX AND NOT APPLE)
    target_compile_definitions(${APP_NAME} PRIVATE SOKOL_GLCORE)
    target_link_libraries(${APP_NAME} X11 Xi Xcursor GL dl pthread)

endif()