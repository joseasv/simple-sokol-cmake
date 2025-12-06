@echo off
setlocal

REM --- 1. Validar que se paso un argumento ---
if "%~1"=="" (
    echo [ERROR] Debes especificar el archivo .cpp a compilar.
    echo Uso: ce.bat 01_Triangulo.cpp
    exit /b 1
)

REM --- 2. Activar el entorno (AGREGADO) ---
REM Si esta ruta cambia en el futuro, ajustala aqui.
call "C:\Dev\setup_x64.bat"

REM --- 3. Procesar nombres de archivo ---
set "INPUT_FILE=%~1"

REM Comprobamos si termina en .cpp (comparacion insensible a mayusculas /I)
if /I not "%INPUT_FILE:~-4%"==".cpp" (
    set "SOURCE_FILE=%INPUT_FILE%.cpp"
) else (
    set "SOURCE_FILE=%INPUT_FILE%"
)

REM Extraemos el nombre sin extension para el EXE
for %%F in ("%SOURCE_FILE%") do set "EXE_NAME=%%~nF"

echo [INFO] Configurando CMake para: %SOURCE_FILE%...

REM --- 4. Configurar CMake ---
cmake -B build -DAPP_SRC="%SOURCE_FILE%"
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Error en la configuracion de CMake.
    exit /b 1
)

echo [INFO] Compilando...

REM --- 5. Compilar ---
cmake --build build
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Error de compilacion.
    exit /b 1
)

echo [EXITO] Compilacion exitosa. Ejecutando %EXE_NAME%...
echo ------------------------------------------------

REM --- 6. Ejecutar ---
REM Busca primero en Debug (comun en MSVC), luego en raiz
if exist "build\Debug\%EXE_NAME%.exe" (
    "build\Debug\%EXE_NAME%.exe"
) else if exist "build\%EXE_NAME%.exe" (
    "build\%EXE_NAME%.exe"
) else (
    echo [ERROR] No se encontro el ejecutable '%EXE_NAME%.exe'.
    echo Verifique la carpeta build/ o build/Debug/
)

endlocal