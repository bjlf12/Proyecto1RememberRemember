/*
Autores:
Brandon Ledezma Fernández
Walter Morales Vásquez

Código para la creación y el manejo de mutexes para trabajar con los hilos del programa. Permite el uso de las funciones
básicas de cualquier mutex, como: lock, unlock y trylock, para facilitar la utilización de estructuras compartidas por los
hilos sin obtener una condición de carrera.
*/

#ifndef A_MYMUTEX_H
#define A_MYMUTEX_H

#include "mythread.h"
#include "data_structures.h"

// Función para inicializar un mutex en el programa. Este deberá tener los valores por defecto del mutex.
int mymutex_init(mythread_mutex_t **mymutex);
// Función para eliminar los datos de un mutex completamente. Con esta función se libera la memoria utilizada por el mutex.
int mymutex_destroy(mythread_mutex_t **mymutex);
// Método para marcar como bloqueado un mutex. El primer hilo en solicitarlo es el que mantendrá bloqueado el mutex
// hasta que termine de realizar sus operaciones y llame a mymutex_unlock. En caso de que el hilo intente bloquear un
// mutex ya bloqueado este hilo será bloqueado hasta que el mutex vuelva a estar libre.
int mymutex_lock(mythread_mutex_t **mymutex);
// Función para desbloquear el mutex que se poseía anteriormente.
int mymutex_unlock(mythread_mutex_t **mymutex);
// Función para intentar bloquear a un mutex. Si el mutex no se encuentra bloqueado este es bloqueado y el hilo que
// realizó la llamada puede avanzar. Si el mutex no estaba libre se retorna 1 y el thread puede continuar para realizar
// otra acción.
int mymutex_trylock(mythread_mutex_t **mymutex);

#endif //A_MYMUTEX_H