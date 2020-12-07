/*
Autores:
Brandon Ledezma Fernández
Walter Morales Vásquez

Código con la capacidad de crear y manejar hilos implementados en el espacio de usuario de la misma manera en que
funcionan en la biblioteca de POSIX pthread. Además, permite especificar los tiempos de vida un hilo en particular
al momento de la creación de este. El orden de ejecución de estos hilos será decidido por un scheduler de tiempo real.
*/

#ifndef A_MYTHREAD_H
#define A_MYTHREAD_H

#include <signal.h>
#include <time.h>
#include <ucontext.h>
#include "data_structures.h"

#define STACKSIZE 0x10000

extern int total_tickets;               // Variable que representa al total de tickets presentes en los hilos.
extern mythread *current;               // Puntero al hilo que se encontrará en ejecución en todo momento del programa.
extern ucontext_t scheduler_context;    // Contexto del scheduler al que será cambiado cada vez que se tenga que decidir
                                        // por un nuevo thread a ejecutar.

// Función para colocar al final de una cola de threads un nuevo thread. Se pasa la dirección de los elementos
// importantes de la cola para que los cambios realizados en esta función se vean reflejados correctamente.
void enqueue(tcb **tcb_head, tcb **tcb_tail, mythread *new_mythread);
// Función para retirar el último elemento de una cola de threads.
mythread *dequeue(tcb **tcb_head, tcb **tcb_tail);
// Función para buscar un thread por su identificador en una cola. Se recibe la cabeza por donde se iniciará la
// búsqueda y el identificador a buscar.
mythread *search_thread(threads_queue *threads_q, mythread_t thread_id);
// Función para buscar un thread en una cola y retirarlo de la misma. La búsqueda es realizada por el identificador
// del thread.
mythread *pop_thread(tcb **tcb_head, tcb **tcb_tail, mythread_t thread_id);
// Función para buscar un thread en una cola y retirarlo de la misma. Esta búsqueda es realizada por el identificador
// de la alarma del thread.
mythread *pop_thread_by_timerid(tcb **tcb_head, tcb **tcb_tail, timer_t timerid);
// Función para realizar la explosión del thread actual.
void explote_current();
// Función para explotar un thread indicado.
void explote_thread(mythread **exploted_thread);
// Función que configura la alarma del scheduler para que sea llamado cuando pase un quantum de tiempo (especificado
// por el usuario).
void set_scheduler_timer();
// Función para quitar la alarma del scheduler para que no sea interrumpido en medio de su trabajo.
void unset_scheduler_timer();
// Función para comprobar el estado de los threads y llamar al scheduler de tiempo real para obtener el siguiente
// thread a ejecutar.
void schedule();
// Función que indica si existe algún thread que no ha iniciado en el programa.
int has_not_started(tcb *tcb_head);
// Función que se encarga de manejar las interrupciones de tiempo.
void timer_interrupt(int j, siginfo_t *si, void *old_context);
// Función para configurar los signals utilizados por las alarmas.
void setup_signals();
// Método para crear una alarma según tiempos dados y un timer_t.
void setup_timer(int seconds, long nanoseconds, timer_t *timer_id);
// Función para establecer las configuraciones iniciales de los threads. Recibe el quantum a utilizar al asignarle tiempo
// a los hilos.
void mythread_init(int new_quantum, void (*unlockNPrint)(Arguments *arguments), ListMonitors *monitors, Scene *scene);
// Método para crear un nuevo hilo utilizando los parámetros especificados por el usuario. Se deberá rellenar el argumento
// newthread_id con el identificador generado en esta función. Se deberá especificar la función que ejecutará el nuevo
// hilo y datos importantes para la configuración de su funcionamiento.
int mythread_create(mythread_t *newthread_id, void (*thread_function)(), void *arg, int is_rr, int tickets,
                    int start_time_mseconds, int finish_time_mseconds, ListFigures *figure);
// Función para terminar la ejecución de un hilo, deberá ser llamada por el hilo que debe terminar. Recibe un puntero
// que será su valor de retorno.
void mythread_end(void *retval);
// Función por la que un hilo puede ceder el procesador.
int mythread_yield();
// Función por la que un hilo puede esperar y obtener el resultado final de otro.
int mythread_join(mythread_t thread_id, void **retval);
// Función para marcar a un hilo como desenlazado del programa.
int mythread_detach(mythread_t thread_id);
// Función para cambiar el scheduler de un hilo al de sorteo. Es utilizado cuando es necesario darle más prioridad
// a un hilo para que sea capaz de finalizar correctamente.
void mythread_chsched(mythread *thread, int priority, int current_total_tickets);
// Función que maneja las interrupciones para terminar el proceso con "Cntrl + C". Va recorriendo las colas de threads
// liberando memoria.
void sigint_handler(int signo);

#endif //A_MYTHREAD_H