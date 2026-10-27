#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "uso: ./torneo N M [config.txt]" << endl;
        return 1;
    }

    int jugadores = atoi(argv[1]);
    int mesas = atoi(argv[2]);

    cout << "jugadores: " << jugadores << endl;
    cout << "mesas: " << mesas << endl;
    return 0;
}