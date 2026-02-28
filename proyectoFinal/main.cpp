#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

// Tipos de celda para la lógica y eventualmente para Sokol
#define PARED -1
#define PASILLO 1
// Las salas tendrán IDs: 2, 3, 4... esto servirá para elegir texturas

struct Sala {
    int id;
    int x, z; // Esquina superior izquierda (pivote)
    int ancho, alto;
    int centroX, centroZ;
};

// Función de colisión AABB para evitar que las salas se solapen
// Se agrega un margen de +1 para que siempre haya al menos una pared de separación
bool checkColision(const Sala& a, const Sala& b)
{
    return (a.x <= b.x + b.ancho + 1 && a.x + a.ancho + 1 >= b.x && a.z <= b.z + b.alto + 1 && a.z + a.alto + 1 >= b.z);
}

// Comparador para ordenar salas por posición X (Naturalidad en pasillos)
bool compararSalas(const Sala& a, const Sala& b)
{
    return a.x < b.x;
}

int main(int argc, char* argv[])
{
    if (argc < 4) {
        std::cout << "Uso: " << argv[0] << " <ancho> <alto> <num_salas>" << std::endl;
        return 1;
    }

    int MAPA_ANCHO = atoi(argv[1]);
    int MAPA_ALTO = atoi(argv[2]);
    int CANTIDAD_SALAS_OBJETIVO = atoi(argv[3]);

    srand(time(NULL));

    std::vector<std::vector<int>> mapa(MAPA_ALTO, std::vector<int>(MAPA_ANCHO, PARED));

    std::vector<Sala> listaSalas;
    int intentos = 0;
    int maxIntentosGlobales = 2000; // Seguridad para evitar bucles infinitos

    // 2. Escalamiento proporcional de salas
    int menorDimension = std::min(MAPA_ANCHO, MAPA_ALTO);
    int minS = menorDimension * 0.15; // Mínimo 15% del lado más corto
    int maxS = menorDimension * 0.35; // Máximo 35% del lado más corto
    if (minS < 3)
        minS = 3;
    if (maxS < 4)
        maxS = 4;

    // 3. Generación de Salas con Reintentos
    while (listaSalas.size() < (size_t)CANTIDAD_SALAS_OBJETIVO && intentos < maxIntentosGlobales) {
        Sala nueva;
        nueva.ancho = minS + rand() % (maxS - minS + 1);
        nueva.alto = minS + rand() % (maxS - minS + 1);

        // Posicionamiento aleatorio (dejando un margen de 1 para los bordes)
        nueva.x = 1 + rand() % (MAPA_ANCHO - nueva.ancho - 2);
        nueva.z = 1 + rand() % (MAPA_ALTO - nueva.alto - 2);
        nueva.centroX = nueva.x + (nueva.ancho / 2);
        nueva.centroZ = nueva.z + (nueva.alto / 2);

        bool colisiona = false;
        for (const auto& s : listaSalas) {
            if (checkColision(nueva, s)) {
                colisiona = true;
                break;
            }
        }

        if (!colisiona) {
            nueva.id = listaSalas.size() + 2; // IDs empiezan en 2
            // Rellenar submatriz de la sala
            for (int i = nueva.z; i < nueva.z + nueva.alto; i++) {
                for (int j = nueva.x; j < nueva.x + nueva.ancho; j++) {
                    mapa[i][j] = nueva.id;
                }
            }
            listaSalas.push_back(nueva);
        }
        intentos++;
    }

    // 4. Ordenar salas por X para que los pasillos sean "naturales"
    std::sort(listaSalas.begin(), listaSalas.end(), compararSalas);

    // 5. Conexión por pasillos (Lógica sucesiva tras ordenamiento)
    for (size_t i = 0; i < listaSalas.size() - 1; i++) {
        int x1 = listaSalas[i].centroX;
        int z1 = listaSalas[i].centroZ;
        int x2 = listaSalas[i + 1].centroX;
        int z2 = listaSalas[i + 1].centroZ;

        // Alternamos aleatoriamente si el pasillo gira primero en X o en Z
        if (rand() % 2 == 0) {
            // Horizontal primero
            for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++)
                if (mapa[z1][x] == PARED)
                    mapa[z1][x] = PASILLO;
            // Luego Vertical
            for (int z = std::min(z1, z2); z <= std::max(z1, z2); z++)
                if (mapa[z][x2] == PARED)
                    mapa[z][x2] = PASILLO;
        } else {
            // Vertical primero
            for (int z = std::min(z1, z2); z <= std::max(z1, z2); z++)
                if (mapa[z][x1] == PARED)
                    mapa[z][x1] = PASILLO;
            // Luego Horizontal
            for (int x = std::min(x1, x2); x <= std::max(x1, x2); x++)
                if (mapa[z2][x] == PARED)
                    mapa[z2][x] = PASILLO;
        }
    }

    // 6. Impresión por consola
    std::cout << "\nNivel Generado (" << listaSalas.size() << " salas):" << std::endl;
    for (int i = 0; i < MAPA_ALTO; i++) {
        for (int j = 0; j < MAPA_ANCHO; j++) {
            if (mapa[i][j] == PARED)
                std::cout << " # ";
            else if (mapa[i][j] == PASILLO)
                std::cout << " . ";
            else {
                // Imprimimos el ID de la sala (0, 1, 2...)
                int salaID = mapa[i][j] - 2;
                std::cout << " " << salaID << " ";
            }
        }
        std::cout << std::endl;
    }

    return 0;
}