/*
Autores:
Brandon Ledezma Fernández
Walter Morales Vásquez

Archivo con las estructuras de datos utilizadas en el programa.
*/

#ifndef MYMUTEX_DATA_STRUCTURES_H
#define MYMUTEX_DATA_STRUCTURES_H

#include <ucontext.h>
#include <time.h>

#define _DEBUG
#ifdef _DEBUG
#define PRINTERR(s) fprintf(stderr, s)
#else
#define PRINTERR(s) {}
#endif

#define _INFO
#ifdef _INFO
#define PRINTINFO(s) printf(s)
#define PRINTEXECINFO(s, t) printf(s, t)
#define PRINTSTAT(thread, time) printf("ThreadId: %d, Empezado: %d, Bloqueado: %d, EsRR: %d, Tickets: %d, Desenlazado: %d\n", \
                 thread->mythread_id, thread->started, thread->blocked, thread->is_round_robin, thread->tickets, thread->detached); \
                 printf("Tiempo para explotar: %d, Actual: %d, Comparación: %d\n", \
                 thread->time_to_explode, time, thread->time_to_explode-time)
#else
#define PRINTINFO(s) {}
#define PRINTEXECINFO(s, t) {}
#define PRINTSTAT(thread, finishTime) {}
#endif

typedef int mythread_t;                 // Tipo de dato que representa al identificador de los threads.
typedef struct mythread mythread;       // Estructura que simboliza a los hilos en el programa.
typedef struct threads_queue threads_queue; // Cola de nodos utilizada para almacenar los hilos.
typedef threads_queue tcb;              // Cola donde se encontrarán los hilos elegibles para ser ejecutados.
typedef threads_queue waiting_threads;  // Cola de nodos esperando a ser desbloqueados.
typedef threads_queue dead_threads;     // Cola de nodos que han finalizado su ejecución.

typedef struct ListFigures{
    int id; // Un id que se le asigna a cada figura.
    char *ascii; // ASCII Arts que se encuentra en un archivo con el nombre de la figura de divenciones 5x5.
    char *type; // Tipo de figura, ejemplo: cuadrado, triangulo...
    int scheduler; // 0 para sorteo y un 1 para Round Robin.
    int tickets; // Cantidad de tiquetes 0 si es Round Robin, sino varia segun lo pasado por el usuario.
    int rotation; // Rotación de 0, 90, 180, 270 que deba realizar el ASCII art.
    int startX; // Valor X de la posición inicial.
    int startY; // Valor Y de la posición inicial.
    int endX; // Valor X de la posición final.
    int endY; // Valor Y de la posición final.
    int startTime; // Cantidad de segundos de la hora de inicio.
    int finishTime; // Tiempo de finalización de la figura.
    int isLive; // 1 Si la figura esta activa, 0 lo contrario.
    int moves; // Cantidad de movimientos que tiene una figura en su posición actual.
    int **nextMoves; // Matriz del tamaño de la cantidad de movimientos posibles, almacena las coordenadas del movimiento.
    struct ListFigures *next; // Puntero al siguiente elemento.
}ListFigures;

struct mythread {                   // Estructura que simboliza a los hilos en el programa.
    mythread_t mythread_id;         // Identificador único del thread.
    ucontext_t mythread_context;    // Contexto almacenado para conocer el estado en ejecución del hilo.

    waiting_threads *w_threads_head;    // Cola de thread que esperan al hilo presente.
    waiting_threads *w_threads_tail;

    void *return_value;             // Valor de retorno del thread.

    int is_round_robin;             // Indica cuál algoritmo de planificación será utilizado con el hilo
                                    // (0 sorteo y 1 round robin).
    int tickets;                    // Cantidad de tiquetes que posee el hilo para el algoritmo de sorteo.

    int start_time;                 // Tiempo en que debe iniciar el hilo en milisegundos.
    int finish_time;                // Tiempo que debe durar en ejecución el hilo en milisegundos.
    timer_t timer_id;               // Identificador utilizado para conocer si fue el hilo el que realizó una
                                    // interrupción de tiempo.
    int started;                    // Variable que indica si el thread ha iniciado su ejecución.
    int blocked;                    // Indica si el hilo se encuentra bloqueado.
    int completed;                  // Indica si el thread ha finalizado.
    int exploded;                   // Variable para saber si un hilo ha explotado.
    int detached;                   // Variable que indica si el hilo se encuentra desenlazado del programa,
                                    // si es 1 la memoria del hilo será liberada al final de su ejecución.
    int time_to_explode;            // Momento en segundos donde el thread debe explotar.

    ListFigures *figure;            // Figura asociada al hilo.
};

struct threads_queue {      // Cola de nodos utilizada para almacenar los hilos.
    mythread *thread;       // Thread contenido por el nodo de la cola.
    threads_queue *next;    // Puntero al siguiente elemento de la cola.
};

typedef struct mythread_mutex_t mythread_mutex_t;   // Estructura que representa a los mutex del sistema.

struct  mythread_mutex_t {
    int locked;                // Entero que indica si el mutex se encuentra bloqueado o no. Si se encuentra con el valor 0
    // estará libre y si es de valor 1 estará bloqueado por un hilo.
    mythread *thread;            // Puntero al hilo que se encuentra en el momento asociado al mutex.
    waiting_threads *waiting_head;    // Cola donde se encontrarán los hilos que se encuentran esperando a que se desbloque el mutex.
    waiting_threads *waiting_tail;
};

typedef struct Arguments{
    void *figures; // Lista de figuras.
    void *clients; // Lista de monitores.
    void *scene; // Escenario en que se realiza la animación.
} Arguments;

typedef struct Scene{
    int length; // Largo del escenario
    int width; // Ancho del escenario
    int len; // Largo de la lista de campos
    struct Field *field; // Lista de campos
} Scene;

typedef struct Field{
    int x; // Múmero de fila
    int y; // Múmero de columna
    int isLocked; // Si la casilla esta bloqueada 1 sino 0
    struct Field *next; // Puntero al siguiente elemento
}Field;

typedef struct ListMonitors{
    int id; // Id unico del monitor
    int x1; // Valor X del punto inicial
    int y1; // Valor Y del punto inicial
    int x2; // Valor X del punto final
    int y2; // Valor Y del punto final
    int length; // Largo del monitor
    int width; // Ancho del monitor
    int client; // Valor del socket de la conexión del cliente
    struct ListMonitors *next;
}ListMonitors;

#endif //MYMUTEX_DATA_STRUCTURES_H
