## Linux

### Crear archivos de compilación (Reemplazar el texto main.cpp por el nombre del archivo cpp a compilar)
`cmake -B build -DAPP_SRC=main.cpp`

### Compilar
`cmake --build build`

Al compilar, el ejecutable estará en la carpeta build. Para más comodidad, hay un script de bash para Linux que se ejecuta así
`./ce nombreDeCpp.cpp`

## Windows

Recomiendo usar el compilador de Portable MVC. Para eso se debe crear una carpeta aparte y colocar allí el siguiente programa de Python: https://gist.githubusercontent.com/mmozeiko/7f3162ec2988e81e56d5c4e22cde9977/raw/portable-msvc.py. Después de ejecutarlo debe existir el programa `setup_x64.bat`.

### VSCode

Si se quiere usar VSCode, después de clonar este repositorio, se debe crear una carpeta llamada `.vscode`. Dentro de esa carpeta se debe crear un archivo llamado `cmake-kits.json`. El contenido de ese archivo es el siguiente (La ruta del campo `environmentSetupScript` debe apuntar al archivo `setup_x64.bat` del paso anterior):

```JSON
[
  {
    "name": "Portable MSVC (x64)",
    "visualStudio": "16.0", 
    "visualStudioArchitecture": "amd64", 
    "preferredGenerator": { "name": "NMake Makefiles" },
    "environmentSetupScript": "C:\\rutaCarpetaCompilador\\setup_x64.bat"
  }
]
```

Por último, se debe reiniciar VSCode. Al hacerlo, la extensión CMakeTools preguntará cuál kit usar. Se debe seleccionar la opción "Portable MSVC (x64)". Al hacer esto, el proyecto se puede compilar al hacer click en el botón Compilar de la barra inferior.

### Terminal

Los pasos son los mismos que en Linux pero antes se debe ejecutar:
`call C:\rutaCarpetaCompilador\setup_x64.bat`

La ruta que aparece allí es la ruta al archivo .bat que se descargó con el script de Python.

Por último, para mayor comodidad, hay un script llamado ce.bat que hace los 3 pasos (Se debe corregir la ruta de `setup_x64.bat` en la línea 13) y se ejecuta así:
`ce nombreDeCpp.cpp`

