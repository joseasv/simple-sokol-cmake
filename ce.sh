#!/bin/bash

# --- Colores para la terminal ---
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 1. Validar que se pasó un argumento
if [ -z "$1" ]; then
    echo -e "${RED}Error: Debes especificar el archivo .cpp a compilar.${NC}"
    echo -e "Uso: ./run.sh 02_TrianguloAzul-shdc.cpp"
    exit 1
fi

# 2. Procesar nombres de archivo
# Si el usuario escribe "archivo" sin .cpp, se lo agregamos automáticamente
FILENAME=$(basename "$1")
if [[ "$FILENAME" != *.cpp ]]; then
    SOURCE_FILE="${FILENAME}.cpp"
else
    SOURCE_FILE="$FILENAME"
fi

# El nombre del ejecutable es el nombre del archivo sin la extensión .cpp
EXE_NAME="${SOURCE_FILE%.cpp}"

echo -e "${BLUE}>>> Configurando CMake para: ${SOURCE_FILE}...${NC}"

# 3. Configurar CMake (Generar Makefiles)
# -B build : Usar carpeta build
# -DAPP_SRC : Pasar el archivo fuente
cmake -B build -DAPP_SRC="$SOURCE_FILE" > /dev/null

if [ $? -ne 0 ]; then
    echo -e "${RED}>>> Error en la configuración de CMake.${NC}"
    exit 1
fi

echo -e "${BLUE}>>> Compilando...${NC}"

# 4. Compilar (Build)
# --build build : Compilar lo que esté en la carpeta build
cmake --build build

if [ $? -ne 0 ]; then
    echo -e "${RED}>>> Error de compilación. Corrige los errores arriba.${NC}"
    exit 1
fi

echo -e "${GREEN}>>> Compilación exitosa. Ejecutando ${EXE_NAME}...${NC}"
echo "------------------------------------------------"

# 5. Ejecutar
# Buscamos el ejecutable dentro de build/
./build/"$EXE_NAME"