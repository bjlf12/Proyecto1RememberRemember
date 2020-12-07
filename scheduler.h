/*
Autores:
Brandon Ledezma Fernández
Walter Morales Vásquez

Programa con la capacidad de seleccionar el siguiente thread a ejecutar mediante los algoritmos de round robin y
de sorteo. Los algoritmos empleados para decidir cual será el siguiente thread son gobernados por un scheduler de
tiempo real que aspirará a completar la mayor cantidad de trabajos sin que exploten los threads.
*/

#ifndef A_SCHEDULER_H
#define A_SCHEDULER_H

#include "mythread.h"

// Función que elige un thread de un cola mediante el algoritmo round robin. Recibe un puntero hacía el inicio de
// la cola y otro al final para realizar sus operaciones. El thread resultado será retirado de la cola.
mythread *round_robin(tcb **tcb_head, tcb **tcb_tail);
// Método para obtener un thread de la cola mediante el algoritmo sorteo. Se reciben los punteros a los nodos
// de inicio y final de la cola y el total de tickets presentes actualmente en el programa.
mythread *lottery(tcb **tcb_head, tcb **tcb_tail, int total_tickets);
// Función para definir el orden de ejecución de los hilos del programa. Este planificador realiza operaciones para
// darle más prioridad a los hilos que se encuentren cerca de su tiempo de muerte con el fin de evitar la mayor cantidad
// de explosiones de hilos posibles.
mythread *real_time(tcb **tcb_head, tcb **tcb_tail);

#endif //A_SCHEDULER_H
