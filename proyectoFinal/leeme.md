Generador aleatorio de niveles
===

Este generador aleatorio de niveles llena un arreglo bidimensional con salas y pasillos que conectan salas como para un juego del género roguelike. Las salas tiene un tamaño que se escala de acuerdo al tamaño del arreglo.

El programa utiliza una detección de colisiones mediante AABBs para evitar solapar salas. Además, las salas tiene un borde de una celda. Esto último hace que el generador no genere salas pegadas o contiguas.

El generador es un programa para la línea de comandos y se compila de la siguiente forma:

`g++ main.cpp -o generador`

y se ejecta así:

`./generador #filas #columnas #salas`. 

Por ejemplo, si quiero generar un mapa de 20 por 30 con 5 salas se haría de esta manera:

`./generador 20 30 5`

El generador imprimirá el nivel resultante en la terminal/consola.