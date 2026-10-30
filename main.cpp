#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <string>
#include <pthread.h>

using namespace std;

const int FILAS = 7;
const int COLUMNAS = 5;

const int NUM_PERROS = 12;
const int MAX_MOVIMIENTOS = 2000;

struct Tablero
{
    char casillas[FILAS][COLUMNAS];
    pthread_mutex_t candado;
    int pumaFila, pumaCol;
    int perroFila[NUM_PERROS];
    int perroCol[NUM_PERROS];
    bool perroVivo[NUM_PERROS];
    int capturados;
    int movimientos;
    bool terminado;
    string ganador;
};

void iniciarTablero(Tablero &tablero)
{
    for (int fila = 0; fila < FILAS; fila++)
    {
        for (int col = 0; col < COLUMNAS; col++)
        {
            tablero.casillas[fila][col] = '.';
        }
    }
    tablero.casillas[0][0] = '#';
    tablero.casillas[0][1] = '#';
    tablero.casillas[0][3] = '#';
    tablero.casillas[0][4] = '#';
    tablero.casillas[1][0] = '#';
    tablero.casillas[1][4] = '#';
    tablero.casillas[1][2] = 'P';
    for (int col = 0; col < COLUMNAS; col++)
    {
        tablero.casillas[5][col] = 'D';
        tablero.casillas[6][col] = 'D';
    }
    tablero.casillas[4][0] = 'D';
    tablero.casillas[4][4] = 'D';
    int n = 0;
    for (int fila = 0; fila < FILAS; fila++)
    {
        for (int col = 0; col < COLUMNAS; col++)
        {
            if (tablero.casillas[fila][col] == 'P')
            {
                tablero.pumaFila = fila;
                tablero.pumaCol = col;
            }
            if (tablero.casillas[fila][col] == 'D' && n < NUM_PERROS)
            {
                tablero.perroFila[n] = fila;
                tablero.perroCol[n] = col;
                tablero.perroVivo[n] = true;
                n++;
            }
        }
    }
    tablero.capturados = 0;
    tablero.movimientos = 0;
    tablero.terminado = false;
    tablero.ganador = "";
    pthread_mutex_init(&tablero.candado, NULL);
}

void mostrarTablero(Tablero &tablero)
{
    for (int fila = 0; fila < FILAS; fila++)
    {
        for (int col = 0; col < COLUMNAS; col++)
        {
            if (tablero.casillas[fila][col] == '#')
            {
                cout << "  ";
            }
            else
            {
                cout << tablero.casillas[fila][col] << " ";
            }
        }
        cout << endl;
    }
}

int espera_min = 50;
int espera_max = 300;
int semilla = 1;

void leer_configuracion(string nombre_archivo)
{
    ifstream archivo(nombre_archivo);
    if (!archivo)
    {
        cout << "No se pudo abrir " << nombre_archivo << endl;
        exit(1);
    }

    string palabra;
    while (archivo >> palabra)
    {
        if (palabra == "retardo_min")
        {
            archivo >> espera_min;
        }
        else if (palabra == "retardo_max")
        {
            archivo >> espera_max;
        }
        else if (palabra == "seed")
        {
            archivo >> semilla;
        }
    }
}

const int DF[4] = {-1, 1, 0, 0};
const int DC[4] = {0, 0, -1, 1};

bool casillaLibre(Tablero &tablero, int fila, int col)
{
    if (fila < 0 || fila >= FILAS || col < 0 || col >= COLUMNAS)
    {
        return false;
    }
    return tablero.casillas[fila][col] == '.';
}

bool moverPerro(Tablero &tablero, int i, int d)
{
    int fila = tablero.perroFila[i];
    int col = tablero.perroCol[i];
    int nuevaFila = fila + DF[d];
    int nuevaCol = col + DC[d];

    if (!casillaLibre(tablero, nuevaFila, nuevaCol))
    {
        return false;
    }

    tablero.casillas[fila][col] = '.';
    tablero.casillas[nuevaFila][nuevaCol] = 'D';
    tablero.perroFila[i] = nuevaFila;
    tablero.perroCol[i] = nuevaCol;
    return true;
}

bool hayPerro(Tablero &tablero, int fila, int col)
{
    if (fila < 0 || fila >= FILAS || col < 0 || col >= COLUMNAS)
    {
        return false;
    }
    return tablero.casillas[fila][col] == 'D';
}

bool moverPuma(Tablero &tablero, int d)
{
    int nuevaFila = tablero.pumaFila + DF[d];
    int nuevaCol = tablero.pumaCol + DC[d];

    if (!casillaLibre(tablero, nuevaFila, nuevaCol))
    {
        return false;
    }

    tablero.casillas[tablero.pumaFila][tablero.pumaCol] = '.';
    tablero.casillas[nuevaFila][nuevaCol] = 'P';
    tablero.pumaFila = nuevaFila;
    tablero.pumaCol = nuevaCol;
    return true;
}

bool capturarPerro(Tablero &tablero, int d)
{
    int filaPerro = tablero.pumaFila + DF[d];
    int colPerro = tablero.pumaCol + DC[d];
    int filaDestino = filaPerro + DF[d];
    int colDestino = colPerro + DC[d];

    if (!hayPerro(tablero, filaPerro, colPerro))
    {
        return false;
    }
    if (!casillaLibre(tablero, filaDestino, colDestino))
    {
        return false;
    }

    for (int i = 0; i < NUM_PERROS; i++)
    {
        if (tablero.perroVivo[i] && tablero.perroFila[i] == filaPerro && tablero.perroCol[i] == colPerro)
        {
            tablero.perroVivo[i] = false;
        }
    }

    tablero.casillas[filaPerro][colPerro] = '.';
    tablero.casillas[tablero.pumaFila][tablero.pumaCol] = '.';
    tablero.casillas[filaDestino][colDestino] = 'P';
    tablero.pumaFila = filaDestino;
    tablero.pumaCol = colDestino;
    tablero.capturados++;
    return true;
}

bool pumaAcorralado(Tablero &tablero)
{
    for (int d = 0; d < 4; d++)
    {
        int fila = tablero.pumaFila + DF[d];
        int col = tablero.pumaCol + DC[d];

        if (casillaLibre(tablero, fila, col))
        {
            return false;
        }
        if (hayPerro(tablero, fila, col) && casillaLibre(tablero, fila + DF[d], col + DC[d]))
        {
            return false;
        }
    }
    return true;
}

void revisarVictoria(Tablero &tablero)
{
    if (tablero.capturados >= 6)
    {
        tablero.terminado = true;
        tablero.ganador = "Puma";
    }
    else if (pumaAcorralado(tablero))
    {
        tablero.terminado = true;
        tablero.ganador = "Perros";
    }
    else if (tablero.movimientos >= MAX_MOVIMIENTOS)
    {
        tablero.terminado = true;
        tablero.ganador = "Empate";
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "uso: ./torneo N M [config.txt]" << endl;
        return 1;
    }

    int jugadores = atoi(argv[1]);
    int mesas = atoi(argv[2]);
    if (argc >= 4)
    {
        leer_configuracion(argv[3]);
    }

    cout << "jugadores: " << jugadores << endl;
    cout << "mesas: " << mesas << endl;
    cout << "espera: " << espera_min << " a " << espera_max << " ms" << endl;
    cout << "semilla: " << semilla << endl;

    Tablero tablero;
    iniciarTablero(tablero);
    mostrarTablero(tablero);
    return 0;
}