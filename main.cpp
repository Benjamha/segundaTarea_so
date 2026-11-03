#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <csignal>
#include <ctime>
#include <vector>

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
    int id;
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

pthread_mutex_t candado_pantalla = PTHREAD_MUTEX_INITIALIZER;

void escribir(Tablero &tablero, string mensaje)
{
    pthread_mutex_lock(&candado_pantalla);
    cout << "[Mesa " << tablero.id << "] " << mensaje << endl;
    pthread_mutex_unlock(&candado_pantalla);
}

void mostrarJugada(Tablero &tablero, string mensaje)
{
    pthread_mutex_lock(&candado_pantalla);
    cout << "[Mesa " << tablero.id << "] " << mensaje << endl;
    mostrarTablero(tablero);
    pthread_mutex_unlock(&candado_pantalla);
}

int espera_min = 50;
int prob_abandono = 0;
int espera_max = 300;
int semilla = 1;
sem_t sem_mesas;
sem_t sem_pumas;
sem_t sem_perros;
volatile sig_atomic_t cortar = 0;

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
        else if (palabra == "abandono")
        {
            archivo >> prob_abandono;
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

struct Datos
{
    Tablero *tablero;
    int numero;
    unsigned int semilla;
};

void esperar(Datos *datos)
{
    int espera = espera_min + rand_r(&datos->semilla) % (espera_max - espera_min + 1);
    usleep(espera * 1000);
}

void *hiloPuma(void *arg)
{
    Datos *datos = (Datos *)arg;
    Tablero &tablero = *datos->tablero;

    while (true)
    {
        esperar(datos);

        pthread_mutex_lock(&tablero.candado);
        if (tablero.terminado)
        {
            pthread_mutex_unlock(&tablero.candado);
            break;
        }
        if (rand_r(&datos->semilla) % 1000 < prob_abandono)
        {
            tablero.terminado = true;
            tablero.ganador = "Perros (abandono del puma)";
            pthread_mutex_unlock(&tablero.candado);
            escribir(tablero, "El jugador puma abandona la partida");
            break;
        }
        bool movio = false;
        string mensaje = "";
        for (int d = 0; d < 4 && !movio; d++)
        {
            movio = capturarPerro(tablero, d);
            if (movio)
            {
                mensaje = "El puma captura un perro";
            }
        }
        if (!movio)
        {
            int d = rand_r(&datos->semilla) % 4;
            movio = moverPuma(tablero, d);
            if (movio)
            {
                mensaje = "El puma se mueve";
            }
        }

        if (movio)
        {
            tablero.movimientos++;
            mostrarJugada(tablero, mensaje);
            revisarVictoria(tablero);
        }
        pthread_mutex_unlock(&tablero.candado);
        if (cortar)
        {
            tablero.terminado = true;
        }
    }
    return NULL;
}

void *hiloPerro(void *arg)
{
    Datos *datos = (Datos *)arg;
    Tablero &tablero = *datos->tablero;
    int i = datos->numero - 1;

    while (true)
    {
        esperar(datos);

        pthread_mutex_lock(&tablero.candado);
        if (cortar)
        {
            tablero.terminado = true;
        }
        if (tablero.terminado || !tablero.perroVivo[i])
        {
            pthread_mutex_unlock(&tablero.candado);
            break;
        }

        if (rand_r(&datos->semilla) % 1000 < prob_abandono)
        {
            tablero.terminado = true;
            tablero.ganador = "Puma (abandono de los perros)";
            pthread_mutex_unlock(&tablero.candado);
            escribir(tablero, "El jugador de los perros abandona la partida");
            break;
        }

        int d = rand_r(&datos->semilla) % 4;
        if (moverPerro(tablero, i, d))
        {
            tablero.movimientos++;
            mostrarJugada(tablero, "El perro " + to_string(i + 1) + " se mueve");
            revisarVictoria(tablero);
        }
        pthread_mutex_unlock(&tablero.candado);
    }
    return NULL;
}

void *hiloJuez(void *arg)
{
    Tablero *tablero = (Tablero *)arg;
    iniciarTablero(*tablero);
    mostrarJugada(*tablero, "Comienza la partida");

    pthread_t hilos[NUM_PERROS + 1];
    Datos datos[NUM_PERROS + 1];

    for (int k = 0; k < NUM_PERROS + 1; k++)
    {
        datos[k].tablero = tablero;
        datos[k].numero = k;
        datos[k].semilla = semilla + tablero->id * 100 + k;
        if (k == 0)
        {
            pthread_create(&hilos[k], NULL, hiloPuma, &datos[k]);
        }
        else
        {
            pthread_create(&hilos[k], NULL, hiloPerro, &datos[k]);
        }
    }

    for (int k = 0; k < NUM_PERROS + 1; k++)
    {
        pthread_join(hilos[k], NULL);
    }
    if (tablero->ganador == "")
    {
        tablero->ganador = "Ninguno (interrumpida)";
    }

    escribir(*tablero, "Fin de la partida. Ganador: " + tablero->ganador + ". Perros capturados: " + to_string(tablero->capturados));

    pthread_mutex_destroy(&tablero->candado);
    sem_post(&sem_mesas);
    return NULL;
}

struct Cliente
{
    int numero;
    unsigned int semilla;
};

void *hiloCliente(void *arg)
{
    Cliente *cliente = (Cliente *)arg;
    usleep(rand_r(&cliente->semilla) % 200 * 1000);

    if (cliente->numero % 2 == 0)
    {
        sem_post(&sem_pumas);
    }
    else
    {
        sem_post(&sem_perros);
    }
    return NULL;
}

bool esperarSem(sem_t *sem)
{
    while (!cortar)
    {
        timespec limite;
        clock_gettime(CLOCK_REALTIME, &limite);
        limite.tv_nsec += 100000000;
        if (limite.tv_nsec >= 1000000000)
        {
            limite.tv_sec++;
            limite.tv_nsec -= 1000000000;
        }
        if (sem_timedwait(sem, &limite) == 0)
        {
            return true;
        }
    }
    return false;
}

void manejar_sigint(int)
{
    cortar = 1;
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
    if (jugadores <= 0 || mesas <= 0)
    {
        cout << "N y M tienen que ser mayores que 0" << endl;
        return 1;
    }
    if (argc >= 4)
    {
        leer_configuracion(argv[3]);
    }

    cout << "jugadores: " << jugadores << endl;
    cout << "mesas: " << mesas << endl;
    cout << "espera: " << espera_min << " a " << espera_max << " ms" << endl;
    cout << "semilla: " << semilla << endl;

    signal(SIGINT, manejar_sigint);

    sem_init(&sem_mesas, 0, mesas);
    sem_init(&sem_pumas, 0, 0);
    sem_init(&sem_perros, 0, 0);

    vector<pthread_t> hilos_clientes(jugadores);
    vector<Cliente> clientes(jugadores);
    for (int k = 0; k < jugadores; k++)
    {
        clientes[k].numero = k;
        clientes[k].semilla = semilla + k;
        pthread_create(&hilos_clientes[k], NULL, hiloCliente, &clientes[k]);
    }

    int parejas = jugadores / 2;
    vector<Tablero> tableros(parejas);
    vector<pthread_t> jueces(parejas);
    int creadas = 0;

    for (int p = 0; p < parejas; p++)
    {
        if (!esperarSem(&sem_pumas))
            break;
        if (!esperarSem(&sem_perros))
            break;
        if (!esperarSem(&sem_mesas))
            break;

        tableros[p].id = p + 1;
        pthread_create(&jueces[p], NULL, hiloJuez, &tableros[p]);
        creadas++;
    }

    for (int p = 0; p < creadas; p++)
    {
        pthread_join(jueces[p], NULL);
    }
    for (int k = 0; k < jugadores; k++)
    {
        pthread_join(hilos_clientes[k], NULL);
    }

    sem_destroy(&sem_mesas);
    sem_destroy(&sem_pumas);
    sem_destroy(&sem_perros);
    pthread_mutex_destroy(&candado_pantalla);

    if (cortar)
    {
        cout << "Torneo interrumpido con Ctrl+C. Recursos liberados." << endl;
    }
    else
    {
        cout << "Torneo terminado" << endl;
    }
    return 0;
}