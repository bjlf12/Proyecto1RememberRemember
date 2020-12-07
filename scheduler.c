/*
Autores:
Brandon Ledezma Fernández
Walter Morales Vásquez

Programa con la capacidad de seleccionar el siguiente thread a ejecutar mediante los algoritmos de round robin y
de sorteo. Los algoritmos empleados para decidir cual será el siguiente thread son gobernados por un scheduler de
tiempo real que aspirará a completar la mayor cantidad de trabajos sin que exploten los threads.
*/

#include "scheduler.h"
#include <stdlib.h>
#include <stdio.h>

int alternate = 1;      // Variable que indica cual algoritmo se ejecutará. Cambiará constantemente entre 0 y 1
                        // para, de la misma manera, cambiar el algoritmo a utilizar.
int priority_count = 5; // Variable para llevar un control de si se debe realizar el calculo para aumentar los
                        // tiquetes de un thread.

// Función que elige un thread de un cola mediante el algoritmo round robin. Recibe un puntero hacía el inicio de
// la cola y otro al final para realizar sus operaciones. El thread resultado será retirado de la cola.
mythread *round_robin(tcb **tcb_head, tcb **tcb_tail) {
    mythread *result;               // Variable donde se almacenará el hilo resultado.
    tcb *temp = *tcb_head, *temp2;  // Punteros para iterar sobre la cola de threads.
    if((*tcb_head)->thread->is_round_robin && !(*tcb_head)->thread->blocked) {  // Si el primer elemento de la cola
        result = (*tcb_head)->thread;                                           // es válido para elegir.
        *tcb_head = (*tcb_head)->next;                                          // Se elige y se realizan operaciones
        if(*tcb_head == NULL) *tcb_tail = NULL;                                 // para cambiar la cabeza de la cola
        free(temp);                                                             // con el siguiente nodo.
        return result;
    }
    while(temp->next != NULL) {         // Se recorre la cola en búsqueda de un hilo que sea apto para el algoritmo.
        if(temp->next->thread->is_round_robin && !temp->next->thread->blocked) {    // Si se encuentra un hilo
            result = temp->next->thread;                                            // que utilize el scheduler
            temp2 = temp->next;                                                     // round robin y no esté bloqueado
            temp->next = temp2->next;                                               // se saca de la cola y se retorna.
            if(temp2 == *tcb_tail) *tcb_tail = temp;
            free(temp2);
            return result;
        }
        temp = temp->next;              // Si el actual no funciona para continuar se avanza al siguiente.
    }
    return NULL;                        // Si no se encontraron threads capaces de seguir se retorna NULL.
}

// Método para obtener un thread de la cola mediante el algoritmo sorteo. Se reciben los punteros a los nodos
// de inicio y final de la cola y el total de tickets presentes actualmente en el programa.
mythread *lottery(tcb **tcb_head, tcb **tcb_tail, int total_tickets) {
    if(total_tickets == 0) return NULL;     // Si no hay tickets se retorna NULL.
    srand(time(0));              // Se inicializa el generador de números random.
    int random = rand()%total_tickets;      // Se obtiene un número al azar que no sobrepase la cantidad
                                            // de tiquetes.
    int count = 0;          // Contador que llevará la suma de la cantidad de tiquetes con los
                            // que se ha probado.
    mythread *result;       // Thread que almacenará el resultado.

    if(!current->is_round_robin && !current->blocked) { // Se comprueba si el nodo actual usa sorteo y no está bloqueado.
        count += current->tickets;                      // Se le suma al contador la cantidad de tiquetes del actual.
        if(count>random) return current;                // Si la suma es mayor al número random obtenido se elige este
    }                                                   // thread como el siguiente a ejecutar.

    tcb *temp = *tcb_head, *temp2;      // Punteros para iterar sobre la cola de threads.
    if(!(*tcb_head)->thread->is_round_robin && !(*tcb_head)->thread->blocked) { // Se comprueba si el nodo cabeza usa
                                                                                // sorteo y no está bloqueado.
        count += (*tcb_head)->thread->tickets;          // Se le suma al contador la cantidad de tiquetes del hilo.
        if(count>random) {                              // Si la suma es mayor al número random obtenido se elige este
            result = (*tcb_head)->thread;               // thread como el siguiente a ejecutar.
            *tcb_head = (*tcb_head)->next;              // Se realizan operaciones para sacar la cabeza de la cola y
            if(*tcb_head == NULL) *tcb_tail = NULL;     // retornar su valor.
            free(temp);
            return result;
        }
    }
    while(temp->next != NULL) {                     // Se comprueba con el resto de la cola.
        if(!temp->next->thread->is_round_robin && !temp->next->thread->blocked) {   // Si el thread es de sorteo y no
            count += temp->next->thread->tickets;                                   // está bloqueado.
            if(count>=random) {                             // Se le suma al contador la cantidad de tiquetes del hilo.
                result = temp->next->thread;                // Si la suma es mayor al número random obtenido se elige este
                temp2 = temp->next;                         // thread como el siguiente a ejecutar.
                temp->next = temp2->next;                   // Se realizan operaciones para sacar al elemento de la cola.
                if(temp2 == *tcb_tail) *tcb_tail = temp;    // Si era el último se asigna como último el penúltimo elemento.
                free(temp2);
                return result;
            }
        }
        temp = temp->next;      // Se continua iterando la cola.
    }
    return NULL;                // Si no se encuentra algún hilo que sea válido se retorna NULL.
}

// Función para definir el orden de ejecución de los hilos del programa. Este planificador realiza operaciones para
// darle más prioridad a los hilos que se encuentren cerca de su tiempo de muerte con el fin de evitar la mayor cantidad
// de explosiones de hilos posibles.
mythread *real_time(tcb **tcb_head, tcb **tcb_tail) {
    --priority_count;       // Se resta uno a la variable que define si se realizarán las operaciones para aumentar la prioridad.
    if(!priority_count) {   // Si el valor recién restado ahora es 0 se procede a comprobar el estado de los threads para
                            // comprobar si alguno debe de aumentar de prioridad.
        threads_queue *temp = *tcb_head;
        int current_time = time(0);                 // Se obtiene el tiempo en segundos del sistema.
        int current_total_tickets = total_tickets;        // Se crea una variable que almacenará la cantidad de tiquets
                                                          // al momento de empezar los cálculos.
        PRINTSTAT(current, current_time);
        while (temp != NULL) {                              // Se itera sobre la cola.
            PRINTSTAT(temp->thread, current_time);
            if ((temp->thread->time_to_explode - current_time) < 30) {      // Si al hilo le queda menos de 30 segundos de
                mythread_chsched(temp->thread, 3, current_total_tickets);   // vida, se le da prioridad 3.
            } else if ((temp->thread->time_to_explode - current_time) < 60) {   // Si al hilo le queda menos de 1 minuto de
                mythread_chsched(temp->thread, 2, current_total_tickets);   // vida, se le da prioridad 2.
            } else if ((temp->thread->time_to_explode - current_time) < 300) {  // Si al hilo le queda menos de 5 minutos de
                mythread_chsched(temp->thread, 1, current_total_tickets);   // vida se le da prioridad 1.
            }                       // En otro caso no se realiza ningúna operación.
            temp = temp->next;      // Se pasa al siguiente hilo.
        }
        priority_count = 5;         // Se reinicia el contador para entrar en el priorizador de hilos.
    }

    mythread *result = NULL;        // Variable que almacenará el thread resultado de las operaciones de los planificadores.
    if(alternate) {                 // Si la variable alternate indica 1, se realizará un round robin.
        --alternate;                // Se disminuye en 1 la variable alternate.
        result = round_robin(tcb_head, tcb_tail);   // Se llama a la función round robin para obtener un hilo.
        if(result != NULL) return result;
        return lottery(tcb_head, tcb_tail, total_tickets);  // Si de round robin no se obtuvo ningún hilo se intenta con
    }                                                       // el algoritmo de sorteo.
    else {                          // Si la variable alternate indica 0, se realizará un sorteo.
        ++alternate;                // Se aumenta en 1 la variable alternate.
        result = lottery(tcb_head, tcb_tail, total_tickets);    // Se llama a la función lottery para obtener un hilo.
        if(result != NULL) return result;
        return round_robin(tcb_head, tcb_tail);                 // Si no se obtuvo ningún hilo a partir del algoritmo de
    }                                                           // sorteo, se prueba con el algoritmo round robin.
}