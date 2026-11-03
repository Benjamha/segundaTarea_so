# Torneo de Komikan Concurrente

Tarea 2 de Sistemas Operativos.
Simulación en C++17 de un torneo de Komikan en tiempo real, con hilos POSIX,
mutex y semáforos.

## Compilar

    make

## Ejecutar

    ./torneo N M [config.txt]

- `N`: cantidad de jugadores (se emparejan de a 2: un Puma y un Perros).
- `M`: máximo de mesas jugando al mismo tiempo.
- `config.txt`: archivo opcional.

Ejemplo:

    ./torneo 8 2 config.txt

Con `Ctrl+C` el programa deja de iniciar partidas, termina las que están en
curso y libera todos los recursos.

## Archivo de configuración (opcional)

    retardo_min 10
    retardo_max 50
    seed 42
    abandono 0

- `retardo_min` y `retardo_max`: espera entre movimientos de cada ficha (ms).
- `seed`: semilla de los números aleatorios.
- `abandono`: probabilidad por mil de que un jugador abandone en cada ciclo.
- `tablero` (opcional): 7 filas de 5 caracteres a continuación de la palabra
  `tablero`. `#` casilla que no existe, `.` vacía, `P` puma y `D` perro
  (un puma y máximo 12 perros).

Si no se entrega el archivo, se usan retardos de 50 a 300 ms, semilla 1, sin
abandonos y el tablero por defecto.

## Cómo funciona

- Hilo por cliente, hilo juez por mesa y 13 hilos de fichas por mesa
  (1 puma y 12 perros).
- Un mutex por tablero protege los movimientos; otro mutex ordena la salida
  por pantalla.
- Los semáforos `sem_pumas`, `sem_perros` y `sem_mesas` forman las parejas y
  limitan las mesas a M.
- Gana el Puma con 6 capturas, ganan los Perros si lo acorralan, y hay empate
  a los 2000 movimientos.

## Archivos

- `main.cpp`: código del programa.
- `Makefile`: compilación.
- `config.txt`: ejemplo de configuración.
- `informe_tarea2SO.pdf`: informe de diseño.


