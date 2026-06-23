#ifndef MAPA_H
#define MAPA_H

#include <iostream>
#include <iomanip>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <map>

using namespace std;

struct Habitacion {
    int fila, col;
    int tamaño;
    int izquierda, derecha, arriba, abajo;
    int id;
};

struct PuntoBorde {
    int fila;
    int col;
    int idHabitacion;
    char direccion; // 'N', 'S', 'E', 'O'
};

struct PuntoPasillo {
    int fila;
    int col;
    int idConexion; // Par de habitaciones que conecta (ej: 0-1, 1-2)
    bool esInterseccion; // Si es parte de más de un pasillo
};

struct PuntoHabitacion {
    int fila;
    int col;
    int idHabitacion;
};

struct PosicionHabitacion {
    int id;
    int centro_fila;
    int centro_col;
    int tamaño;
    int izquierda;
    int derecha;
    int arriba;
    int abajo;
    vector<PuntoHabitacion> celdas;
};

// Función para verificar si dos habitaciones están demasiado cerca (modificada)
bool habitacionesDemasiadoCercanas(const Habitacion& h1, const Habitacion& h2) {
    // Verificar si están demasiado cerca en el eje X (horizontal)
    if(!(h1.derecha + 2 < h2.izquierda || h1.izquierda - 2 > h2.derecha)) {
        // Están alineadas verticalmente, verificar separación vertical
        int distanciaVertical;
        if(h1.abajo < h2.arriba) {
            distanciaVertical = h2.arriba - h1.abajo;
        } else {
            distanciaVertical = h1.arriba - h2.abajo;
        }
        // Necesitan al menos 3 espacios de separación (2 para las paredes + 1 para el pasillo)
        if(distanciaVertical < 3) {
            return true;
        }
    }
    
    // Verificar si están demasiado cerca en el eje Y (vertical)
    if(!(h1.abajo + 2 < h2.arriba || h1.arriba - 2 > h2.abajo)) {
        // Están alineadas horizontalmente, verificar separación horizontal
        int distanciaHorizontal;
        if(h1.derecha < h2.izquierda) {
            distanciaHorizontal = h2.izquierda - h1.derecha;
        } else {
            distanciaHorizontal = h1.izquierda - h2.derecha;
        }
        // Necesitan al menos 3 espacios de separación (2 para las paredes + 1 para el pasillo)
        if(distanciaHorizontal < 3) {
            return true;
        }
    }
    
    return false;
}

// Función para verificar si una posición está libre
bool posicionValida(const Habitacion& nueva, const vector<Habitacion>& existentes, int filas, int columnas) {
    // Verificar que la habitación esté dentro de los límites del mapa con espacio para paredes
    if(nueva.izquierda < 1 || nueva.derecha >= columnas - 1 || 
       nueva.arriba < 1 || nueva.abajo >= filas - 1) {
        return false;
    }
    
    // Verificar que no se superpone con habitaciones existentes
    for(const auto& existente : existentes) {
        if(!(nueva.derecha < existente.izquierda || 
             nueva.izquierda > existente.derecha ||
             nueva.abajo < existente.arriba || 
             nueva.arriba > existente.abajo)) {
            return false;
        }
        
        if(habitacionesDemasiadoCercanas(nueva, existente)) {
            return false;
        }
    }
    
    return true;
}

// Función para detectar bordes
vector<PuntoBorde> detectarBordes(const vector<vector<char>>& mapa, const vector<Habitacion>& habitaciones) {
    vector<PuntoBorde> bordes;
    int filas = mapa.size();
    int columnas = mapa[0].size();
    
    for(const auto& hab : habitaciones) {
        for(int j = hab.izquierda; j <= hab.derecha; j++) {
            int filaSuperior = hab.arriba - 1;
            if(filaSuperior >= 0 && filaSuperior < filas && j >= 0 && j < columnas) {
                if(mapa[filaSuperior][j] == '*' || mapa[filaSuperior][j] == '|' || mapa[filaSuperior][j] == '#') {
                    bordes.push_back({filaSuperior, j, hab.id, 'N'});
                }
            }
            
            int filaInferior = hab.abajo + 1;
            if(filaInferior >= 0 && filaInferior < filas && j >= 0 && j < columnas) {
                if(mapa[filaInferior][j] == '*' || mapa[filaInferior][j] == '|' || mapa[filaInferior][j] == '#') {
                    bordes.push_back({filaInferior, j, hab.id, 'S'});
                }
            }
        }
        
        for(int i = hab.arriba; i <= hab.abajo; i++) {
            int colIzquierda = hab.izquierda - 1;
            if(i >= 0 && i < filas && colIzquierda >= 0 && colIzquierda < columnas) {
                if(mapa[i][colIzquierda] == '*' || mapa[i][colIzquierda] == '|' || mapa[i][colIzquierda] == '#') {
                    bordes.push_back({i, colIzquierda, hab.id, 'O'});
                }
            }
            
            int colDerecha = hab.derecha + 1;
            if(i >= 0 && i < filas && colDerecha >= 0 && colDerecha < columnas) {
                if(mapa[i][colDerecha] == '*' || mapa[i][colDerecha] == '|' || mapa[i][colDerecha] == '#') {
                    bordes.push_back({i, colDerecha, hab.id, 'E'});
                }
            }
        }
    }
    
    return bordes;
}

// Función para dibujar bordes
void dibujarBordes(vector<vector<char>>& mapa, const vector<PuntoBorde>& bordes) {
    for(const auto& borde : bordes) {
        if(borde.fila >= 0 && borde.fila < mapa.size() &&
           borde.col >= 0 && borde.col < mapa[0].size()) {
            if(mapa[borde.fila][borde.col] != '#' && mapa[borde.fila][borde.col] != '$') {
                mapa[borde.fila][borde.col] = '|';
            }
        }
    }
}

// Función para crear un pasillo entre dos habitaciones (MEJORADA)
void crearPasillo(vector<vector<char>>& mapa, const Habitacion& h1, const Habitacion& h2, 
                  const vector<Habitacion>& habitaciones, vector<PuntoPasillo>& pasillos, int idConexion) {
    
    int mejorDistancia = 1000000;
    int mejorInicioFila = h1.fila, mejorInicioCol = h1.derecha;
    int mejorFinFila = h2.fila, mejorFinCol = h2.izquierda;
    int mejorPuntoSalida = 0;
    
    // Probar diferentes puntos de salida de h1 y entrada de h2
    for(int puntoSalida = 0; puntoSalida < 4; puntoSalida++) {
        int inicioFila, inicioCol, finFila, finCol;
        
        switch(puntoSalida) {
            case 0: // Derecha de h1 a Izquierda de h2
                inicioFila = h1.fila;
                inicioCol = h1.derecha + 1;
                finFila = h2.fila;
                finCol = h2.izquierda - 1;
                break;
            case 1: // Izquierda de h1 a Derecha de h2
                inicioFila = h1.fila;
                inicioCol = h1.izquierda - 1;
                finFila = h2.fila;
                finCol = h2.derecha + 1;
                break;
            case 2: // Abajo de h1 a Arriba de h2
                inicioFila = h1.abajo + 1;
                inicioCol = h1.col;
                finFila = h2.arriba - 1;
                finCol = h2.col;
                break;
            case 3: // Arriba de h1 a Abajo de h2
                inicioFila = h1.arriba - 1;
                inicioCol = h1.col;
                finFila = h2.abajo + 1;
                finCol = h2.col;
                break;
        }
        
        // Verificar que los puntos de inicio y fin estén dentro del mapa
        if(inicioFila >= 0 && inicioFila < mapa.size() && 
           inicioCol >= 0 && inicioCol < mapa[0].size() &&
           finFila >= 0 && finFila < mapa.size() && 
           finCol >= 0 && finCol < mapa[0].size()) {
            
            // Verificar que los puntos de inicio y fin no estén dentro de ninguna habitación
            bool inicioValido = true;
            bool finValido = true;
            
            for(const auto& hab : habitaciones) {
                if(inicioFila >= hab.arriba && inicioFila <= hab.abajo &&
                   inicioCol >= hab.izquierda && inicioCol <= hab.derecha) {
                    inicioValido = false;
                }
                if(finFila >= hab.arriba && finFila <= hab.abajo &&
                   finCol >= hab.izquierda && finCol <= hab.derecha) {
                    finValido = false;
                }
            }
            
            if(inicioValido && finValido) {
                int distancia = abs(inicioFila - finFila) + abs(inicioCol - finCol);
                
                if(distancia < mejorDistancia) {
                    mejorDistancia = distancia;
                    mejorInicioFila = inicioFila;
                    mejorInicioCol = inicioCol;
                    mejorFinFila = finFila;
                    mejorFinCol = finCol;
                    mejorPuntoSalida = puntoSalida;
                }
            }
        }
    }
    
    // Si no se encontró un camino válido, usar el mejor intento
    if(mejorDistancia == 1000000) {
        cout << "  ADVERTENCIA: No se pudo conectar Hab " << h1.id+1 << " con Hab " << h2.id+1 << endl;
        return;
    }
    
    int fila_actual = mejorInicioFila;
    int col_actual = mejorInicioCol;
    
    vector<PuntoPasillo> puntosTemporales;
    
    // Guardar el punto de inicio (conecta con h1)
    puntosTemporales.push_back({fila_actual, col_actual, idConexion, false});
    mapa[fila_actual][col_actual] = '#';
    
    // Crear pasillo horizontal primero
    while(col_actual != mejorFinCol) {
        if(col_actual < mejorFinCol) col_actual++;
        else col_actual--;
        
        if(col_actual >= 0 && col_actual < mapa[0].size() && 
           fila_actual >= 0 && fila_actual < mapa.size()) {
            
            bool dentroHabitacion = false;
            for(const auto& hab : habitaciones) {
                if(fila_actual >= hab.arriba && fila_actual <= hab.abajo &&
                   col_actual >= hab.izquierda && col_actual <= hab.derecha) {
                    dentroHabitacion = true;
                    break;
                }
            }
            
            if(!dentroHabitacion && mapa[fila_actual][col_actual] != '$') {
                mapa[fila_actual][col_actual] = '#';
                puntosTemporales.push_back({fila_actual, col_actual, idConexion, false});
            }
        }
    }
    
    // Crear pasillo vertical
    while(fila_actual != mejorFinFila) {
        if(fila_actual < mejorFinFila) fila_actual++;
        else fila_actual--;
        
        if(col_actual >= 0 && col_actual < mapa[0].size() && 
           fila_actual >= 0 && fila_actual < mapa.size()) {
            
            bool dentroHabitacion = false;
            for(const auto& hab : habitaciones) {
                if(fila_actual >= hab.arriba && fila_actual <= hab.abajo &&
                   col_actual >= hab.izquierda && col_actual <= hab.derecha) {
                    dentroHabitacion = true;
                    break;
                }
            }
            
            if(!dentroHabitacion && mapa[fila_actual][col_actual] != '$') {
                mapa[fila_actual][col_actual] = '#';
                puntosTemporales.push_back({fila_actual, col_actual, idConexion, false});
            }
        }
    }
    
    // Guardar todos los puntos del pasillo
    for(const auto& punto : puntosTemporales) {
        pasillos.push_back(punto);
    }
    
    /*cout << "  Pasillo " << idConexion << " (Hab " << h1.id+1 << " - Hab " << h2.id+1 
         << "): " << puntosTemporales.size() << " celdas, desde (" 
         << mejorInicioFila << "," << mejorInicioCol << ") hasta (" 
         << mejorFinFila << "," << mejorFinCol << ")" << endl;*/
}

// Función para detectar intersecciones entre pasillos
void detectarIntersecciones(vector<PuntoPasillo>& pasillos) {
    for(size_t i = 0; i < pasillos.size(); i++) {
        for(size_t j = i + 1; j < pasillos.size(); j++) {
            if(pasillos[i].fila == pasillos[j].fila && 
               pasillos[i].col == pasillos[j].col) {
                pasillos[i].esInterseccion = true;
                pasillos[j].esInterseccion = true;
            }
        }
    }
}

// Función para verificar si están conectadas
bool estanConectadas(const vector<vector<char>>& mapa, const Habitacion& h1, const Habitacion& h2) {
    int radio = 5;
    
    for(int i = max(0, h1.arriba - radio); i <= min((int)mapa.size()-1, h1.abajo + radio); i++) {
        for(int j = max(0, h1.izquierda - radio); j <= min((int)mapa[0].size()-1, h1.derecha + radio); j++) {
            if(mapa[i][j] == '#') {
                if(abs(i - h2.fila) <= radio && abs(j - h2.col) <= radio) {
                    return true;
                }
            }
        }
    }
    return false;
}

// FUNCIÓN PRINCIPAL - Con número de habitaciones como parámetro
void generarMapaCompleto(int filas, int columnas, 
                         int numHabitaciones,  // NUEVO PARÁMETRO
                         vector<vector<char>>& mapa,
                         vector<PuntoBorde>& bordes,
                         vector<PuntoPasillo>& pasillos,
                         vector<PosicionHabitacion>& habitacionesInfo) {
    
    // Limpiar vectores de salida
    mapa.clear();
    bordes.clear();
    pasillos.clear();
    habitacionesInfo.clear();
    
    srand(time(NULL));
    
    // Crear mapa base
    mapa = vector<vector<char>>(filas, vector<char>(columnas, '*'));
    vector<Habitacion> habitaciones;
    
    int intentosMaximos = 10000;
    int habitacionesGeneradas = 0;
    
    //cout << "\n--- Generando " << numHabitaciones << " habitaciones aleatorias ---\n";
    
    while(habitacionesGeneradas < numHabitaciones && intentosMaximos > 0) {  // USAR numHabitaciones
        Habitacion nueva;
        nueva.id = habitacionesGeneradas;
        nueva.tamaño = (rand() % 2 == 0) ? 3 : 4;
        
        int offset = nueva.tamaño / 2;
        
        // LÍMITES PARA ASEGURAR QUE LA HABITACIÓN ESTÉ DENTRO DE LA MATRIZ
        // con espacio para paredes (margen de 1)
        int minFila = 1 + offset;
        int maxFila = filas - 2 - (nueva.tamaño - 1 - offset);
        int minCol = 1 + offset;
        int maxCol = columnas - 2 - (nueva.tamaño - 1 - offset);
        
        // Verificar que los límites sean válidos
        if(minFila > maxFila || minCol > maxCol) {
            intentosMaximos--;
            continue;
        }
        
        nueva.fila = minFila + (rand() % (maxFila - minFila + 1));
        nueva.col = minCol + (rand() % (maxCol - minCol + 1));
        
        nueva.izquierda = nueva.col - offset;
        nueva.derecha = nueva.col + (nueva.tamaño - 1 - offset);
        nueva.arriba = nueva.fila - offset;
        nueva.abajo = nueva.fila + (nueva.tamaño - 1 - offset);
        
        // VERIFICACIÓN EXPLÍCITA DE LÍMITES
        if(nueva.izquierda >= 1 && nueva.derecha < columnas - 1 && 
           nueva.arriba >= 1 && nueva.abajo < filas - 1) {
            
            if(posicionValida(nueva, habitaciones, filas, columnas)) {
                habitaciones.push_back(nueva);
                habitacionesGeneradas++;
                //cout << "Habitacion " << habitacionesGeneradas << ": " 
                 //    << nueva.tamaño << "x" << nueva.tamaño 
                  //   << " en centro (" << nueva.fila << "," << nueva.col << ")" << endl;
            }
        }
        
        intentosMaximos--;
    }
    
    if(habitacionesGeneradas < numHabitaciones) {  // USAR numHabitaciones
        cout << "Error: Solo se pudieron generar " << habitacionesGeneradas << " de " << numHabitaciones << " habitaciones.\n";
        return;
    }
    
    // Crear habitaciones y guardar información
    for(const auto& hab : habitaciones) {
        PosicionHabitacion posHab;
        posHab.id = hab.id;
        posHab.centro_fila = hab.fila;
        posHab.centro_col = hab.col;
        posHab.tamaño = hab.tamaño;
        posHab.izquierda = hab.izquierda;
        posHab.derecha = hab.derecha;
        posHab.arriba = hab.arriba;
        posHab.abajo = hab.abajo;
        
        for(int i = hab.arriba; i <= hab.abajo; i++) {
            for(int j = hab.izquierda; j <= hab.derecha; j++) {
                if(i >= 0 && i < filas && j >= 0 && j < columnas) {
                    mapa[i][j] = ' ';
                    posHab.celdas.push_back({i, j, hab.id});
                }
            }
        }
        
        habitacionesInfo.push_back(posHab);
    }
    
    // Colocar items en los centros de las habitaciones
    for(int h = 0; h < habitacionesInfo.size(); h++) {
        int centro_fila = habitacionesInfo[h].centro_fila;
        int centro_col = habitacionesInfo[h].centro_col;
        if(centro_fila >= 0 && centro_fila < filas && 
           centro_col >= 0 && centro_col < columnas) {
            mapa[centro_fila][centro_col] = '$';
        }
    }
    
    // Detectar y dibujar bordes
    bordes = detectarBordes(mapa, habitaciones);
    dibujarBordes(mapa, bordes);
    
    // Ordenar habitaciones
    sort(habitaciones.begin(), habitaciones.end(), 
         [](const Habitacion& a, const Habitacion& b) {
             if(a.fila == b.fila) return a.col < b.col;
             return a.fila < b.fila;
         });
    
    // Crear pasillos (conectar todas las habitaciones en secuencia)
    //cout << "\n--- Creando pasillos ---\n";
    for(size_t i = 0; i < habitaciones.size() - 1; i++) {
        crearPasillo(mapa, habitaciones[i], habitaciones[i+1], habitaciones, pasillos, i);
    }
    
    detectarIntersecciones(pasillos);
    
    // Verificar conectividad
    //cout << "\n--- Verificando conectividad ---\n";
    /*for(size_t i = 0; i < habitaciones.size(); i++) {
        for(size_t j = i + 1; j < habitaciones.size(); j++) {
            if(habitacionesDemasiadoCercanas(habitaciones[i], habitaciones[j])) {
                if(!estanConectadas(mapa, habitaciones[i], habitaciones[j])) {
                    cout << "ADVERTENCIA: Habitaciones " << i+1 << " y " << j+1 
                         << " están demasiado cerca sin pasillo.\n";
                }
            } else {
                cout << "Habitaciones " << i+1 << " y " << j+1 
                     << ": Correctamente separadas (distancia suficiente).\n";
            }
        }
    }*/
    
    /*cout << "\n--- Informacion de habitaciones guardadas ---\n";
    for(const auto& hab : habitacionesInfo) {
        cout << "Habitacion " << hab.id+1 << ": Centro (" << hab.centro_fila << "," << hab.centro_col 
             << "), Tamanio " << hab.tamaño << "x" << hab.tamaño
             << ", Celdas: " << hab.celdas.size() << endl;
    }*/
}

// Función para mostrar el mapa
void mostrarMapa(const vector<vector<char>>& mapa) {
    cout << "\nMapa del Roguelike (" << mapa.size() << "x" << mapa[0].size() << "):\n";
    //cout << "' ' = Habitacion, '#' = Pasillo, '$' = Item, '|' = Borde, '*' = Espacio vacio\n\n";
    
    cout << "    ";
    for(int j = 0; j < mapa[0].size(); j++) {
        cout << setw(2) << j % 10;
    }
    cout << "\n    ";
    for(int j = 0; j < mapa[0].size(); j++) {
        cout << setw(2) << "-";
    }
    cout << endl;
    
    for(int i = 0; i < mapa.size(); i++) {
        cout << setw(2) << i << " | ";
        for(int j = 0; j < mapa[i].size(); j++) {
            cout << setw(2) << mapa[i][j];
        }
        cout << endl;
    }
}

// Función para mostrar info de bordes
void mostrarInfoBordes(const vector<PuntoBorde>& bordes) {
    //cout << "\n--- Informacion de bordes ---\n";
    //cout << "Total de bordes: " << bordes.size() << endl;
    
    // Agrupar por habitación (ahora puede haber más de 3)
    int maxHab = 0;
    for(const auto& borde : bordes) {
        if(borde.idHabitacion > maxHab) maxHab = borde.idHabitacion;
    }
    
    /*for(int hab = 0; hab <= maxHab; hab++) {
        cout << "\nHabitacion " << hab+1 << ":\n";
        int countN = 0, countS = 0, countE = 0, countO = 0;
        
        for(const auto& borde : bordes) {
            if(borde.idHabitacion == hab) {
                switch(borde.direccion) {
                    case 'N': countN++; break;
                    case 'S': countS++; break;
                    case 'E': countE++; break;
                    case 'O': countO++; break;
                }
            }
        }
        
        cout << "  Norte: " << countN << " | Sur: " << countS 
             << " | Este: " << countE << " | Oeste: " << countO << endl;
    }*/
}

// Función para mostrar el recorrido completo de cada pasillo
void mostrarInfoPasillos(const vector<PuntoPasillo>& pasillos) {
   // cout << "\n=== RECORRIDO DE PASILLOS ===\n";
    
    if(pasillos.empty()) {
        cout << "No hay pasillos para mostrar\n";
        return;
    }
    
    // Agrupar pasillos por idConexion
    map<int, vector<PuntoPasillo>> pasillosPorConexion;
    for(const auto& p : pasillos) {
        pasillosPorConexion[p.idConexion].push_back(p);
    }
    
    /*for(auto& par : pasillosPorConexion) {
        auto& pasillo = par.second;
        sort(pasillo.begin(), pasillo.end(), 
             [](const PuntoPasillo& a, const PuntoPasillo& b) {
                 if(a.fila == b.fila) return a.col < b.col;
                 return a.fila < b.fila;
             });
        
        cout << "\n PASILLO " << par.first+1 << ":\n";
        cout << "   Inicia en: (" << pasillo.front().fila << "," << pasillo.front().col << ")\n";
        cout << "   Termina en: (" << pasillo.back().fila << "," << pasillo.back().col << ")\n";
        cout << "   Recorrido: ";
        for(size_t i = 0; i < pasillo.size(); i++) {
            cout << "(" << pasillo[i].fila << "," << pasillo[i].col << ")";
            if(pasillo[i].esInterseccion) cout << "*";
            if(i < pasillo.size() - 1) cout << " -> ";
        }
        cout << "\n   Total: " << pasillo.size() << " pasos\n";
    }*/
}

// Función para mostrar las coordenadas de las habitaciones
void mostrarInfoHabitaciones(const vector<PosicionHabitacion>& habitacionesInfo) {
    //cout << "\n=== COORDENADAS DE HABITACIONES ===\n";
    
   /* for(const auto& hab : habitacionesInfo) {
        cout << "Habitacion " << hab.id+1 << ": ";
        cout << "Centro (" << hab.centro_fila << "," << hab.centro_col << ")";
        cout << " | Celdas: ";
        
        for(const auto& celda : hab.celdas) {
            cout << "(" << celda.fila << "," << celda.col << ") ";
        }
        cout << endl;
    }*/
}

vector<pair<int, int>> obtenerCoordenadasHabitaciones(const vector<PosicionHabitacion>& habitacionesInfo) {
    vector<pair<int, int>> coordenadas;
    
    for(const auto& hab : habitacionesInfo) {
        for(const auto& celda : hab.celdas) {
            coordenadas.push_back({celda.fila, celda.col});
        }
    }
    
    return coordenadas;
}

#endif