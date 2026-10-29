#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <string>

using namespace std;

const int FILAS = 7;
const int COLUMNAS = 5;

struct Tablero
{
    char casillas[FILAS][COLUMNAS];
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