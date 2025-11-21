cmake_minimum_required(VERSION 3.10)

# --- 1. Configuración Dinámica del Archivo Fuente ---

# Definir una variable 'APP_SRC' que por defecto sea "main.cpp"
# Esta variable se puede sobreescribir desde la terminal con -DAPP_SRC=...
set(APP_SRC "main.cpp" CACHE STRING "Archivo fuente de entrada")

# Extraer el nombre del archivo sin la extensión (ej: "01_TrianguloAzul")
get_filename_component(APP_NAME ${APP_SRC} NAME_WE)

# Inicializar el proyecto con el nombre del archivo
project(${APP_NAME})

message(STATUS "Compilando archivo fuente: ${APP_SRC}")
message(STATUS "Nombre del ejecutable: ${APP_NAME}")

# --- 2. Configuración Estándar ---
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 3. Crear el Ejecutable ---
# Usamos las variables dinámicas aquí
add_executable(${APP_NAME} ${APP_SRC})

# --- 4. Selección de Backend (Tu lógica anterior) ---

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