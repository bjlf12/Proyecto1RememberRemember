/*
Autores:
Brandon Ledezma Fernández
Walter Morales Vásquez

Código para la creación y el manejo de mutexes para trabajar con los hilos del programa. Permite el uso de las funciones
básicas de cualquier mutex, como: lock, unlock y trylock, para facilitar la utilización de estructuras compartidas por los
hilos sin obtener una condición de carrera.
*/

#include "mymutex.h"
#include <stdlib.h>
#include <stdio.h>

// Función para inicializar un mutex en el programa. Este deberá tener los valores por defecto del mutex.
int mymutex_init(mythread_mutex_t **mymutex) {
    *mymutex = (mythread_mutex_t *)malloc(sizeof(mythread_mutex_t));    // Se solicita espacio para el nuevo mutex.

    if (mymutex == NULL) {                          // Se comprueba si el malloc fue exitoso.
        PRINTERR("Error al usar malloc\n");
        exit(1);
    }

    (*mymutex)->locked = 0;             // Se indica que el mutex no está siendo utilizado por un hilo.
    (*mymutex)->thread = NULL;          // El mutex no posee un hilo asociado.
    (*mymutex)->waiting_head = NULL;    // La cola de threads esperando del mutex se encuentra vacía.
    (*mymutex)->waiting_tail = NULL;
    return 0;
}

// Función para eliminar los datos de un mutex completamente. Con esta función se libera la memoria utilizada por el mutex.
int mymutex_destroy(mythread_mutex_t **mymutex) {

    waiting_threads *temp = (*mymutex)->waiting_head, *to_free;
    while(temp != NULL) {                               // Se recorre la cola de threads esperando del mutex.
        total_tickets += temp->thread->tickets;
        temp->thread->blocked = 0;                      // Se marcan como desbloqueados los threads de la cola.
        to_free = temp;
        temp = temp->next;
        free(to_free);
    }

    free((*mymutex));           // Se libera el espacio previamente solicitado.

    return 0;
}

// Método para marcar como bloqueado un mutex. El primer hilo en solicitarlo es el que mantendrá bloqueado el mutex
// hasta que termine de realizar sus operaciones y llame a mymutex_unlock. En caso de que el hilo intente bloquear un
// mutex ya bloqueado este hilo será bloqueado hasta que el mutex vuelva a estar libre.
int mymutex_lock(mythread_mutex_t **mymutex) {
    try_again:                                  // Etiqueta para regresar a comprobar si el mutex se encuentre bloqueado.
    if((*mymutex)->thread != NULL && !(*mymutex)->thread->exploded) {                // Se pregunta si el mutex se encuentra bloqueado.
        enqueue(&((*mymutex)->waiting_head), &((*mymutex)->waiting_tail), current); // Si está bloqueado se inserta al
                                                                                    // thread que intentó hacer el lock
                                                                                    // en la lista de bloqueados del mutex.
        enqueue(&((*mymutex)->thread->w_threads_head), &((*mymutex)->thread->w_threads_tail), current); // Se inserta
                                                                                    // en la lista de bloqueados del
                                                                                    // thread que posee el mutex.
        total_tickets -= current->tickets;      // Se le resta la cantidad de tiquetes que posee a la cantidad total.
        current->blocked = 1;                   // Se marca al thread como bloqueado.

        unset_scheduler_timer();                // Se remueve la alarma del scheduler.
        swapcontext(&current->mythread_context, &scheduler_context);    // Se cambia al contexto del scheduler.
        goto try_again;                         // Se intenta de nuevo hacer el lock.
    }
    (*mymutex)->locked = 1;                     // Si el mutex no se encontraba bloqueado se bloquea.
    (*mymutex)->thread = current;               // Se apunta al thread que posee el mutex.
    return 0;
}

// Función para desbloquear el mutex que se poseía anteriormente.
int mymutex_unlock(mythread_mutex_t **mymutex) {
    if((*mymutex)->thread == NULL) return 1;        // Se pregunta si existe un thread bloqueando al mutex.
    if((*mymutex)->thread != current) return 1;     // Se pregunta si el thread de la solicitud de desbloqueo es el
                                                    // mismo que lo bloqueó.   TODO revisar
    waiting_threads *temp = (*mymutex)->waiting_head, *to_free;
    while(temp != NULL) {                           // Se desbloquean los hilos que estaban esperando para usar el mutex.
        total_tickets += temp->thread->tickets;     // Se aumenta la cantidad de tickets en la cantidad que poseía el thread
                                                    // antes bloqueado.
        temp->thread->blocked = 0;                  // Se marca como debloqueado el thread.
        to_free = temp;
        temp = temp->next;                          // Se avanza al siguiente.
        free(to_free);
    }
    (*mymutex)->waiting_head = NULL;        // Se indica que la cola del mutex se encuentra vacía.
    (*mymutex)->waiting_tail = NULL;
    (*mymutex)->thread = NULL;              // Se asigna NULL al puntero del thread que posee el mutex.
    (*mymutex)->locked = 0;                 // Se marca al mutex como desbloqueado.

    return 0;
}

// Función para intentar bloquear a un mutex. Si el mutex no se encuentra bloqueado este es bloqueado y el hilo que
// realizó la llamada puede avanzar. Si el mutex no estaba libre se retorna 1 y el thread puede continuar para realizar
// otra acción.
int mymutex_trylock(mythread_mutex_t **mymutex) {
    if((*mymutex)->thread != NULL && !(*mymutex)->thread->exploded) return 1;    // Si el mutex se encuentra bloqueado se
                                                                                // retorna 1.
    (*mymutex)->locked = 1;                 // Si no se marca al mutex como bloqueado.
    (*mymutex)->thread = current;           // Se apunta como dueño del mutex al thread que logró bloquearlo.
    return 0;
}