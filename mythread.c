/*
Autores:
Brandon Ledezma Fernández
Walter Morales Vásquez

Código con la capacidad de crear y manejar hilos implementados en el espacio de usuario de la misma manera en que
funcionan en la biblioteca de POSIX pthread. Además, permite especificar los tiempos de vida un hilo en particular
al momento de la creación de este. El orden de ejecución de estos hilos será decidido por un scheduler de tiempo real.
*/

#include "mythread.h"
#include "scheduler.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

ListMonitors *pmonitors;
Scene *pscene;

// Cola de hilos que han finalizado con su ejecución.
dead_threads *dthreads_head;
dead_threads *dthreads_tail;
// Cola de hilos esperando a ser ejecutados.
tcb *tcb_head;
tcb *tcb_tail;

mythread *main_thread;              // Hilo principal enlazado a todos los demás hilos.
mythread *current;                  // Puntero al hilo que se encontrará en ejecución en todo momento del programa.

//unlock
//imprimir_explosion
void (*explosion_function)(ListMonitors *monitors, ListFigures *figure, Scene *scene);
//void (*explosion_function) (int);   // Puntero a la función con la capacidad de mostrar la animación de una explosión.
Scene *animator_scene;              // Puntero al canvas

// Estructura para la configuración de la alarma de todos los hilos creados.
struct sigevent thread_sigevent = { .sigev_notify = SIGEV_SIGNAL, .sigev_signo = SIGALRM };

struct itimerspec scheduler_itimer; // Variable de tiempo donde se guardará el quantum a utilizar en el programa.
timer_t scheduler_timer_id;         // Identificador de las interrupciones de tiempo generadas por el scheduler.
ucontext_t scheduler_context;       // Contexto del scheduler al que será cambiado cada vez que se tenga que decidir
// por un nuevo thread a ejecutar.

int threads_counter;                // Contador de threads que permitirá darle un identificador único a cada thread.
int total_tickets;                  // Variable que representa al total de tickets presentes en los hilos.

// Función para colocar al final de una cola de threads un nuevo thread. Se pasa la dirección de los elementos
// importantes de la cola para que los cambios realizados en esta función se vean reflejados correctamente.
void enqueue(tcb **tcb_head, tcb **tcb_tail, mythread *new_mythread) {
    tcb *new_tcb = (tcb *)malloc(sizeof(tcb));  // Se solicita espacio para el nuevo nodo.
    new_tcb->thread = new_mythread;             // Se le indica al nuevo nodo que almacene al thread pasado por
    // parámetro.
    new_tcb->next = NULL;                       // Se le indica que el siguiente elemento será vacío.
    if(*tcb_tail == NULL) *tcb_head = new_tcb;  // Si el elemento final de la cola es vacío significa que toda la
        // cola está vacía, por lo que se coloca al nuevo thread en la cabeza.
    else (*tcb_tail)->next = new_tcb;           // De otra manera se le indica al elemento final de la cola que apunte
    // al nuevo nodo.
    *tcb_tail = new_tcb;                        // El nuevo nodo será el elemento final de la cola.
}

// Función para retirar el último elemento de una cola de threads.
mythread *dequeue(tcb **tcb_head, tcb **tcb_tail) {
    if(*tcb_head == NULL) return NULL;          // Si el elemento cabeza de la cola es nulo, se retorna nulo.

    mythread *result = (*tcb_head)->thread;     // El resultado de esta función será el thread del nodo cabeza.
    tcb *temp = *tcb_head;                      // Se crea un puntero que apunte a la cabeza actual de la cola.
    *tcb_head = (*tcb_head)->next;              // Se le indica a la cola que apunte a su siguiente elemento.
    if(*tcb_head == NULL) *tcb_tail = NULL;     // Si ahora la cabeza es nula significa que solo había un thread
    // en la cola, por lo que ahora el final de la cola también es nulo.
    free(temp);                                 // Se libera el espacio del nodo que almacenaba el thread.
    return result;                              // Se retorna el thread.
}

// Función para buscar un thread por su identificador en una cola. Se recibe la cabeza por donde se iniciará la
// búsqueda y el identificador a buscar.
mythread *search_thread(threads_queue *threads_q, mythread_t thread_id) {
    while(threads_q != NULL) {              // Se itera buscando hasta que el puntero de la cola sea nulo.
        if(threads_q->thread->mythread_id == thread_id) return threads_q->thread;   // Si se encuentra el thread
        // buscado, se retorna.
        threads_q = threads_q->next;        // Si no, se continua buscando.
    }
    return NULL;                            // Si no se encuentra ningún nodo que coincida con el identificador
    // brindado, se retorna nulo.
}

// Función para buscar un thread en una cola y retirarlo de la misma. La búsqueda es realizada por el identificador
// del thread.
mythread *pop_thread(tcb **tcb_head, tcb **tcb_tail, mythread_t thread_id) {
    if(*tcb_head == NULL) return NULL;      // Si la cabeza de la cola es nula, significa que la cola está vacía.

    tcb *temp = *tcb_head, *temp2;          // Se crean nodos temporales para iterar sobre la cola.
    mythread *result;                       // Puntero de thread que almacenará el resultado.
    if((*tcb_head)->thread->mythread_id == thread_id) { // Se comprueba si el elemento buscado es la cabeza.
        if(*tcb_tail == *tcb_head) *tcb_tail = NULL;    // Se realizan operaciones para cambiar la cabeza de la cola.
        *tcb_head = (*tcb_head)->next;
        result = temp->thread;
        free(temp);                         // Se libera el nodo que no se necesitará.
        return result;                      // Se retorna el elemento buscado.
    }
    while(temp->next != NULL) {             // Se itera sobre la cola hasta que se encuentre con un elemento nulo.
        if(temp->next->thread->mythread_id == thread_id) {      // Si se encuentre al nodo buscado.
            if(temp->next == *tcb_tail) {   // Si el elemento buscado resulta ser el final de la cola.
                temp2 = *tcb_tail;          // Se realizan operaciones para cambiar el último elemento.
                temp->next = NULL;
                *tcb_tail = temp;
                result = temp2->thread;
                free(temp2);                // Se libera el nodo.
                return result;              // Se retorna en thread buscado.
            }
            temp2 = temp->next;             // Si no, se realizan operaciones para retirar el nodo intermedio.
            temp->next = temp2->next;
            result = temp2->thread;
            free(temp2);                    // Se libera el nodo.
            return result;                  // Se retorna en thread buscado.
        }
        temp = temp->next;      // Si el thread buscado no se encuentra en el nodo actual, se apunta al
        // siguiente thread.
    }
    return NULL;                // Si no se encuentre el nodo se retorna nulo.
}

// Función para buscar un thread en una cola y retirarlo de la misma. Esta búsqueda es realizada por el identificador
// de la alarma del thread.
mythread *pop_thread_by_timerid(tcb **tcb_head, tcb **tcb_tail, timer_t timerid) {
    if(*tcb_head == NULL) return NULL;          // Si la cabeza de la cola es nula, significa que la cola está vacía.

    tcb *temp = *tcb_head, *temp2;          // Se crean nodos temporales para iterar sobre la cola.
    mythread *result;                       // Puntero de thread que almacenará el resultado.
    if(&((*tcb_head)->thread->timer_id) == timerid) {   // Se verifica si el elemento buscado es la cabeza.
        if(*tcb_tail == *tcb_head) *tcb_tail = NULL;    // Se realizan operaciones para cambiar la cabeza de la cola.
        *tcb_head = (*tcb_head)->next;
        result = temp->thread;
        free(temp);                         // Se libera el nodo que no se necesitará.
        return result;                      // Se retorna el elemento buscado.
    }
    while(temp->next != NULL) {             // Se itera sobre la cola hasta que se encuentre con un elemento nulo.
        if(&(temp->next->thread->timer_id) == timerid) {        // Si se encuentre al nodo buscado.
            if(temp->next == *tcb_tail) {   // Si el elemento buscado resulta ser el final de la cola.
                temp2 = *tcb_tail;          // Se realizan operaciones para cambiar el último elemento.
                temp->next = NULL;
                *tcb_tail = temp;
                result = temp2->thread;
                free(temp2);                // Se libera el nodo.
                return result;              // Se retorna en thread buscado.
            }
            temp2 = temp->next;             // Si no, se realizan operaciones para retirar el nodo intermedio.
            temp->next = temp2->next;
            result = temp2->thread;
            free(temp2);                    // Se libera el nodo.
            return result;                  // Se retorna en thread buscado.
        }
        temp = temp->next;      // Si el thread buscado no se encuentra en el nodo actual, se apunta al
        // siguiente thread.
    }
    return NULL;                // Si no se encuentre el nodo se retorna nulo.
}

// Función para realizar la explosión del thread actual.
void explote_current() {
    waiting_threads *to_free, *temp = current->w_threads_head;  // Punteros para realizar las operaciones de liberar
    // a los threads esperando.
    while (temp != NULL) {                          // Iteramos sobre los threads esperando.
        if(temp->thread->blocked) {
            total_tickets += temp->thread->tickets;     // Se aumentan el total de tickets.
            temp->thread->blocked = 0;                  // Se desbloquea el thread.
        }
        to_free = temp;
        temp = temp->next;                          // Pasamos al siguiente.
        free(to_free);                              // Se libera la estructura actual.
    }
    current->w_threads_head = NULL;             // Se asigna al puntero NULL.
    current->w_threads_tail = NULL;

    if (!current->detached) {                       // Si el thread no está desenlazado se inserta en la cola de muertos.
        current->return_value = (void *)1;          // Se indica que finalizó con código 1.
        enqueue(&dthreads_head, &dthreads_tail, current);   // Se inserta en la cola.
    }
    total_tickets -= current->tickets;      // Se le resta al total de tickets los que poseía.
    current->exploded = 1;                  // Se marca el thread como explotado. TODO
    current->completed = 1;                 // Marcamos el thread como finalizado. TODO

    unset_scheduler_timer();                                        // Se remueve la alarma del scheduler.
    swapcontext(&current->mythread_context, &scheduler_context);    // Se cambia al contexto del scheduler.
}

// Función para explotar un thread indicado.
void explote_thread(mythread **exploded_thread) {

    waiting_threads *to_free, *temp = (*exploded_thread)->w_threads_head;   // Punteros para realizar las operaciones de liberar
    // a los threads esperando.
    while (temp != NULL) {                          // Iteramos sobre los threads esperando.
        total_tickets += temp->thread->tickets;     // Se aumentan el total de tickets.
        temp->thread->blocked = 0;                  // Se desbloquea el thread.
        to_free = temp;
        temp = temp->next;                          // Pasamos al siguiente.
        free(to_free);                              // Se libera la estructura actual.
    }
    (*exploded_thread)->w_threads_head = NULL;
    (*exploded_thread)->w_threads_tail = NULL;

    total_tickets -= (*exploded_thread)->tickets;   // Restamos los tickets que poseía el thread del total.
    if (!(*exploded_thread)->detached) {            // Si el thread no está desenlazado se inserta en la cola de muertos.
        (*exploded_thread)->return_value = (void *)1;   // Se indica que finalizó con código 1.
        enqueue(&dthreads_head, &dthreads_tail, (*exploded_thread));    // Se inserta en la cola.
    }
    else {
        free(*exploded_thread);
        (*exploded_thread) = NULL;
    }
    (*exploded_thread)->exploded = 1;       // TODO revisar esto, no se cae?
    (*exploded_thread)->completed = 1;
}

// Función que configura la alarma del scheduler para que sea llamado cuando pase un quantum de tiempo (especificado
// por el usuario).
void set_scheduler_timer() {

    thread_sigevent.sigev_value.sival_ptr = &scheduler_timer_id;        // Se apunta al timer_id del scheduler.
    if(timer_create(CLOCK_REALTIME,&thread_sigevent,&scheduler_timer_id) == -1) {   // Se crea la alarma.
        PRINTERR("Error al crear la alarma del scheduler\n");
        exit(1);
    }
    if(timer_settime(scheduler_timer_id, 0, &scheduler_itimer , NULL) == -1) {  // Se establece.
        PRINTERR("Error al establecer la alarma del scheduler\n");
        exit(1);
    }
}

// Función para quitar la alarma del scheduler para que no sea interrumpido en medio de su trabajo.
void unset_scheduler_timer() {
    if(timer_delete(scheduler_timer_id) == -1) {                    // Se elimina la alarma con el identificador del
        PRINTERR("Error al remover la alarma del scheduler\n");  // scheduler.
        exit(1);
    }
}

// Función para comprobar el estado de los threads y llamar al scheduler de tiempo real para obtener el siguiente
// thread a ejecutar.
void schedule() {
    printf("TotalTick: %d", total_tickets);
    PRINTEXECINFO("Saliendo del thread: %d\n", current->mythread_id);
    schedule_again:

    if(tcb_head == NULL) {      // Se comprueba si la cola de threads está vacía.
        PRINTEXECINFO("Entrando al thread %d\n", current->mythread_id);
        PRINTINFO("--------------------------\n");

        set_scheduler_timer();  // Se reinicia el quantum para los threads.
        if(setcontext(&current->mythread_context) == -1) {      // Se cambia al contexto del thread actual.
            PRINTERR("Error al cambiar de contexto\n");
            exit(1);
        }
        return;
    }

    mythread *temp_current;
    if(current->completed) {    // Si el thread actual ya ha finalizado.
        if(current->detached) temp_current = current;   // Se apunta con un thread temporal al actual.
        temp_current = real_time(&tcb_head, &tcb_tail);      // Se busca el siguiente thread a ejecutar.
        //free(temp_current);     // Se libera el actual si se encontraba desenlazado. TODO
        if(temp_current == NULL) {   // Si no se puede ejecutar ningún thread se le indica al usuario y se termina la
            //set_scheduler_timer();
            if(has_not_started(tcb_head)) {
                pause();
                goto schedule_again;
            }
            PRINTINFO("La ejecución del programa ha finalizado\n");           // ejecución del programa.
            exit(0);
        }
        if(temp_current == current) {
            //set_scheduler_timer();
            if(has_not_started(tcb_head)) {
                pause();
                goto schedule_again;
            }
            PRINTINFO("La ejecución del programa ha finalizado\n");           // ejecución del programa.
            exit(0);
        }
    }
    else {
        temp_current = real_time(&tcb_head, &tcb_tail); // Se busca el siguiente thread a ejecutar.
        if(temp_current == NULL) {                      // Si no se puede continuar con nigún thread de la cola.
            printf("%d", total_tickets);
            printf("%d, %d, %d\n", current->mythread_id, current->blocked, current->exploded);
            if(current->blocked) {                      // Si el thread actual se encuentra bloqueado.
                if(has_not_started(tcb_head)) {
                    pause();
                    goto schedule_again;
                }
                PRINTINFO("No se puede continuar con ningún thread\n"); // Se le indica al usuario que no se puede
                exit(0);                                            // continuar con ningún thread.
            }//TODO comprobar
            /*PRINTEXECINFO("Entrando al thread %d\n", current->mythread_id);  // Si se puede continuar con el actual.
            set_scheduler_timer();                      // Se reinicia el quantum para los threads.
            setcontext(&current->mythread_context);     // Se cambia al contexto del actual.
            return;*/
        }
        else if (temp_current == current) {             // Si el thread a ejecutar resultó ser el mismo thread actual.
            printf("%d, %d, %d\n", current->mythread_id, current->blocked, current->exploded);
            if(current->blocked) {                      // Se pregunta si se encuentra bloqueado.
                if(has_not_started(tcb_head)) {
                    pause();
                    goto schedule_again;
                }
                PRINTINFO("No se puede continuar con ningún thread\n");  // Se le indica al usuario que no se puede
                exit(0);                                            // continuar con ningún thread.
            }
            /*PRINTEXECINFO("Entrando al thread %d\n", current->mythread_id);
            set_scheduler_timer();                      // Se reinicia el quantum para los threads.
            setcontext(&current->mythread_context);     // Se cambia al contexto del actual.
            return;*/
        }
        else {                                          // Si se llega aquí el thread calculado se puede ejecutar.
            enqueue(&tcb_head, &tcb_tail, current);         // Colocamos al antiguo thread actual en la cola de threads.
            current = temp_current;                         // Ahora el actual apunta al nuevo thread.
        }
    }
    PRINTEXECINFO("Entrando al thread %d\n", current->mythread_id);
    PRINTINFO("--------------------------\n");

    set_scheduler_timer();                      // Se reinicia el quantum para los threads.
    if(setcontext(&current->mythread_context) == -1) {      // Se cambia al contexto del thread actual.
        PRINTERR("Error al cambiar de contexto\n");
        exit(1);
    }
}

// Función que se encarga de manejar las interrupciones de tiempo.
void timer_interrupt(int j, siginfo_t *si, void *old_context) {

    if(si->si_value.sival_ptr == &scheduler_timer_id) {         // Si la interrupción es del scheduler.
        if(swapcontext(&current->mythread_context, &scheduler_context) == -1) { // Se guarda el estado del contexto
            PRINTERR("Error al cambiar de contexto\n");                      // actual y se cambia la contexto del
            exit(1);                                                     //  scheduler.
        }
    }
    else if(si->si_value.sival_ptr == &(current->timer_id)) {   // Si la interrupción es del hilo actual.
        PRINTINFO("Interrupción de tiempo con el actual\n");
        if(!current->started) {     // Si el hilo no había iniciado.
            PRINTEXECINFO("Ha iniciado el hilo: %d\n", current->mythread_id);
            // Se configura la alarma para cuando tenga que finalizar.
            setup_timer(current->finish_time/1000, ((long)current->finish_time%1000)*1000000, &(current->timer_id));
            current->started = 1;   // Se marca como iniciado y no bloqueado.
            total_tickets += current->tickets;
            current->blocked = 0;
        }
        else {      // Si ya se había iniciado significa que era una interrupción para explotar.
            PRINTEXECINFO("Ha explotado el hilo: %d\n", current->mythread_id);
            explosion_function(pmonitors, current->figure, pscene);
            //printf("Allahuakbar\n");
            explote_current();
        }
    }
    else {      // Si la interrupción es de un hilo en la cola.
        PRINTINFO("Interrupción de tiempo con otro\n");
        mythread *temp = pop_thread_by_timerid(&tcb_head, &tcb_tail, si->si_value.sival_ptr);   // Se busca al hilo.
        if(temp == NULL) return;
        if(!temp->started) {    // Si no había iniciado
            PRINTEXECINFO("Ha iniciado el hilo: %d\n", temp->mythread_id);
            // Se configura la alarma para cuando tenga que finalizar.
            setup_timer(temp->finish_time/1000, ((long)temp->finish_time%1000)*1000000, &(temp->timer_id));
            temp->started = 1;   // Se marca como iniciado y no bloqueado.
            total_tickets += temp->tickets;
            temp->blocked = 0;
            enqueue(&tcb_head, &tcb_tail, temp);    // Se retorna a la cola el hilo.

            /*if(!current->mythread_id) {

                swapcontext(&current->mythread_context, &scheduler_context);    // Se cambia al contexto del scheduler.
            }

            if(setcontext(&scheduler_context) == -1) {      // Se cambia al contexto del thread actual.
                PRINTERR("Error al cambiar de contexto\n");
                exit(1);
            }*/
        }
        else {          // Si ya había iniciado se explota.
            PRINTEXECINFO("Ha explotado el hilo: %d\n", temp->mythread_id);
            explosion_function(pmonitors, temp->figure, pscene);
            explote_thread(&temp); //TODO revisar
        }
    }
}

// Función para configurar los signals utilizados por las alarmas.
void setup_signals() {
    // Se llama a sigaction para que sean configurados los signals.
    if(sigaction(SIGALRM, &(struct sigaction){ .sa_sigaction=timer_interrupt, .sa_flags=SA_SIGINFO }, 0) == -1) {
        PRINTERR("Error al configurar los signals\n");
        exit(1);
    }
}

// Método para crear una alarma según tiempos dados y un timer_t.
void setup_timer(int seconds, long nanoseconds, timer_t *timer_id) {
    thread_sigevent.sigev_value.sival_ptr = timer_id;   // Se apunta al timer_id del thread.
    if(timer_create(CLOCK_REALTIME,&thread_sigevent,timer_id) == -1) {   // Se crea la nueva alarma.
        PRINTERR("Error al crear la alarma del thread\n");
        exit(1);
    }
    if(timer_settime(*timer_id, 0, &(struct  itimerspec const){ .it_value={seconds,nanoseconds} }, NULL) == -1) {   // Se crea la nueva alarma.
        PRINTERR("Error al establecer la alarma del thread\n");
        exit(1);
    }
}

// Función para establecer las configuraciones iniciales de los threads. Recibe el quantum a utilizar al asignarle tiempo
// a los hilos.
void mythread_init(int new_quantum, void (*unlockNPrint)(Arguments *arguments), ListMonitors *monitors, Scene *scene) {
    signal(SIGINT, sigint_handler);     // Se le indica al programa que si sucede una interrupción para terminar el proceso
    // con "Cntrl + C", ejecute la función sigint_handler.
    explosion_function = unlockNPrint;
    pmonitors = monitors;
    pscene = scene;

    total_tickets = 0;      // Número de tiquetes iniciales.
    threads_counter = 0;    // Contador de threads en 0.
    dthreads_head = NULL;   // No se encuentra ningún thread en la cola de muertos.
    dthreads_tail = NULL;
    tcb_head = NULL;        // No hay hilos en la cola de threads a procesar.
    tcb_tail = NULL;
    main_thread = (mythread *)malloc(sizeof(mythread));     // Se configura el thread que representa al thread principal.

    // Se configuran los valores iniciales del thread.
    main_thread->mythread_id = threads_counter++;
    main_thread->w_threads_head = NULL;
    main_thread->w_threads_tail = NULL;
    main_thread->timer_id = NULL;
    main_thread->return_value = NULL;

    main_thread->is_round_robin = 1;
    main_thread->tickets = 0;
    main_thread->start_time = 0;
    main_thread->finish_time = 2147483647;      // Se le asigna un valor alto de finalización.
    main_thread->time_to_explode = 2147483647;
    main_thread->started = 1;
    main_thread->completed = 0;
    main_thread->detached = 0;
    main_thread->exploded = 0;
    main_thread->blocked = 0;

    current = main_thread;      // El thread actual será el de main.

    if(getcontext(&main_thread->mythread_context) == -1) {      // Se obtiene el contexto del main.
        PRINTERR("Error al obtener el contexto del main\n");
        exit(1);
    }

    setup_signals();        // Se inicializan los administradores de signals de tiempo.


    if(getcontext(&scheduler_context) == -1) {      // Se obtiene el contexto del scheduler.
        PRINTERR("Error al obtener el contexto del scheduler\n");
        exit(1);
    }
    scheduler_context.uc_stack.ss_sp = malloc(STACKSIZE);
    scheduler_context.uc_stack.ss_size = STACKSIZE;
    scheduler_context.uc_stack.ss_flags = 0;
    if(sigemptyset(&scheduler_context.uc_sigmask) == -1) {      // Se configura el grupo de señales del contexto a cero.
        PRINTERR("Error al configurar los signals del scheduler\n");
        exit(1);
    }
    makecontext(&scheduler_context, (void (*)(void))schedule, 1);

    // Se crea el itimer del scheduler con el quantum especificado por el usuario.
    scheduler_itimer = (struct  itimerspec const){ .it_value={new_quantum/1000, ((long)new_quantum%1000)*1000000} };
    set_scheduler_timer();
}

// Método para crear un nuevo hilo utilizando los parámetros especificados por el usuario. Se deberá rellenar el argumento
// newthread_id con el identificador generado en esta función. Se deberá especificar la función que ejecutará el nuevo
// hilo y datos importantes para la configuración de su funcionamiento.
int mythread_create(mythread_t *newthread_id, void (*thread_function)(), void *arg, int is_rr, int tickets,
                    int start_time_mseconds, int finish_time_mseconds, ListFigures *new_figure) {

    mythread *temp;
    temp = (mythread *)malloc(sizeof(mythread));        // Se solicita espacio para el nuevo thread.
    if(getcontext(&temp->mythread_context) == -1) {     // Se obtiene un contexto para el nuevo hilo.
        PRINTERR("Error al obtener el contexto del hilo\n");
        exit(1);
    }

    // Se configura la pila del contexto.
    temp->mythread_context.uc_stack.ss_sp = malloc(STACKSIZE);
    temp->mythread_context.uc_stack.ss_size = STACKSIZE;
    temp->mythread_context.uc_stack.ss_flags = 0;
    temp->mythread_context.uc_link = &main_thread->mythread_context; // Si el thread termina se continua con el thread main.

    if(sigemptyset(&temp->mythread_context.uc_sigmask) == -1) {      // Se configura el grupo de señales del contexto a cero.
        PRINTERR("Error al configurar los signals del thread\n");
        exit(1);
    }

    // Se asignan los valores por defecto del thread.
    temp->mythread_id = threads_counter++;      // Se le asigna un identificador al thread.
    *newthread_id = temp->mythread_id;          // Se asigna al puntero recibido por parámetro el nuevo identificador.
    temp->w_threads_head = NULL;
    temp->w_threads_tail = NULL;
    temp->return_value = NULL;
    temp->is_round_robin = is_rr;
    temp->tickets = tickets;
    temp->start_time = start_time_mseconds;
    temp->finish_time = finish_time_mseconds;
    temp->time_to_explode = time(0) + start_time_mseconds/1000 + finish_time_mseconds/1000;
    temp->completed = 0;
    temp->exploded = 0;
    temp->detached = 0;
    temp->figure = new_figure;

    //total_tickets += tickets;

    if(start_time_mseconds==0) {    // Si el tiempo de inicio del thread es 0.
        temp->blocked = 0;          // Se marca como desbloqueado.
        total_tickets += temp->tickets;
        temp->started = 1;          // Se marca como iniciado.
        // Se crea la alarma para cuando tenga que explotar.
        setup_timer(finish_time_mseconds / 1000, ((long)finish_time_mseconds % 1000)*1000000, &(temp->timer_id)); //TODO probar esto
    }
    else {                          // Si el tiempo de inicio no es 0.
        temp->blocked = 1;          // Se marca al thread como bloqueado.
        //total_tickets += temp->tickets;
        temp->started = 0;          // Se indica que el hilo no ha iniciado.
        // Se crea la alarma para cuando tenga que iniciar.
        setup_timer(start_time_mseconds / 1000, ((long)start_time_mseconds % 1000)*1000000, &(temp->timer_id)); //TODO probar esto
    }

    // Se crea el contexto del nuevo hilo.
    makecontext(&temp->mythread_context, (void (*)(void))thread_function, 1, arg);
    // Se almacena en la cola de hilos.
    enqueue(&tcb_head, &tcb_tail, temp);

    return 0;
}

// Función para terminar la ejecución de un hilo, deberá ser llamada por el hilo que debe terminar. Recibe un puntero
// que será su valor de retorno.
void mythread_end(void *retval) {

    waiting_threads *to_free, *temp = current->w_threads_head;  // Punteros para realizar las operaciones de liberar
    // a los threads esperando.
    while (temp != NULL) {                          // Iteramos sobre los threads esperando.
        total_tickets += temp->thread->tickets;     // Se aumentan el total de tickets.
        temp->thread->blocked = 0;                  // Se desbloquea el thread.
        to_free = temp;
        temp = temp->next;                          // Pasamos al siguiente.
        free(to_free);                              // Se libera la estructura actual.
    }
    current->w_threads_head = NULL;
    current->w_threads_tail = NULL;

    if (!current->detached) {               // Si el nodo actual no está desenlazado.
        current->return_value = retval;     // Se guarda el valor de retorno en un espacio del thread.
        enqueue(&dthreads_head, &dthreads_tail, current);   // Se almacena el thread en la cola de muertos.
    }//TODO else un if current NULL en schedule

    total_tickets -= current->tickets;      // Se resta la cantidad de tiquetes del hilo a la actual.
    current->completed = 1;                 // Se marca al hilo como completado.
    printf("Final de thread\n");
    unset_scheduler_timer();                                        // Se remueve la alarma del scheduler.
    swapcontext(&current->mythread_context, &scheduler_context);    // Se cambia al contexto del scheduler.
}

// Función por la que un hilo puede ceder el procesador.
int mythread_yield() {
    unset_scheduler_timer();                                        // Se remueve la alarma del scheduler.
    swapcontext(&current->mythread_context, &scheduler_context);    // Se cambia al contexto del scheduler.

    return 0;
}

// Función por la que un hilo puede esperar y obtener el resultado final de otro.
int mythread_join(mythread_t thread_id, void **retval) {

    mythread *temp = search_thread(tcb_head, thread_id);    // Se busca el hilo en la cola de threads listos.
    if(temp != NULL) {                                      // Si se encuentra ahí.
        enqueue(&(temp->w_threads_head), &(temp->w_threads_tail), current); // Se almacena al hilo actual en la cola
        // de threads esperando del hilo encontrado.
        total_tickets -= current->tickets;                  // Se resta la cantidad de hilos del actual al total.
        current->blocked = 1;                               // Se marca el actual como bloqueado.

        unset_scheduler_timer();                                        // Se remueve la alarma del scheduler.
        swapcontext(&current->mythread_context, &scheduler_context);    // Se cambia al contexto del scheduler.
        *retval = temp->return_value;                                   // Una vez se vuelva a ejecutar es posible
        // obtener el resultado del hilo que se deseaba.
        return 0;
    }

    temp = search_thread(dthreads_head, thread_id);         // Se busca al thread en la cola de muertos.
    if(temp != NULL) {                                      // Si se encuentra ahí.
        *retval = temp->return_value;                       // Se obtiene su valor de retorno.
        return 0;
    }
    return 1;       // No se encontró el thread.
}

// Función para marcar a un hilo como desenlazado del programa.
int mythread_detach(mythread_t thread_id) {

    mythread *temp = search_thread(tcb_head, thread_id);    // Se busca al hilo que se desea desenlazar en la cola de
    // threads listos.
    if(temp != NULL) {          // Si se encuentra.
        temp->detached = 1;     // Se marca al hilo como detached.
        return 0;
    }

    temp = search_thread(dthreads_head, thread_id);         // Se busca en los hilos muertos.
    if(temp != NULL) {          // Si se encuentra.

        free(temp->w_threads_head); // Se libera la cola de threads esperando. TODO funca?
        free(temp->w_threads_tail);
        free(temp->return_value);   // Se libera el valor de retorno del hilo.
        free(temp);
        temp = NULL;
        return 0;
    }
    return 1;
}

// Función para cambiar el scheduler de un hilo al de sorteo. Es utilizado cuando es necesario darle más prioridad
// a un hilo para que sea capaz de finalizar correctamente.
void mythread_chsched(mythread *thread, int priority, int current_total_tickets) {
    int new_sum;
    thread->is_round_robin = 0;     // Se cambia el scheduler del hilo al de sorteo.
    if(priority == 1) {             // Si la prioridad es de nivel 1.
        if(current_total_tickets) {     // Si hay al menos un ticket en el total de tickets.
            new_sum = (current_total_tickets / 4) + (current_total_tickets % 4);    // Se calcula la cantidad a sumar.
            thread->tickets += new_sum;                     // Se le aumenta la cantidad de tickets en un cuarto del total.
            total_tickets += new_sum;     // La operación anterior se ve reflejada en el total de tickets.
        }
        else {                          // Si no hay tickets en el total.
            thread->tickets = 10;       // Se le asigna al thread 10 tickets.
            total_tickets = 10;         // La operación anterior se ve reflejada en el total de tickets.
        }
    }
    else if(priority == 2) {        // Si la prioridad es de nivel 2.
        if(current_total_tickets) {     // Si hay al menos un ticket en el total de tickets.
            new_sum = (current_total_tickets / 3) + (current_total_tickets % 3);    // Se calcula la cantidad a sumar.
            thread->tickets += new_sum;         // Se le aumenta la cantidad de tickets en un tercio del total.
            total_tickets += new_sum;           // La operación anterior se ve reflejada en el total de tickets.
        }
        else {                          // Si no hay tickets en el total.
            thread->tickets = 10;       // Se le asigna al thread 10 tickets.
            total_tickets = 10;         // La operación anterior se ve reflejada en el total de tickets.
        }
    }
    else {                  // Si la prioridad es de nivel 3.
        if(current_total_tickets) { // Si hay al menos un ticket en el total de tickets.
            new_sum = (current_total_tickets / 2) + (current_total_tickets % 2);    // Se calcula la cantidad a sumar.
            thread->tickets += new_sum;         // Se le aumenta la cantidad de tickets en la mitad de la cantidad total.
            total_tickets += new_sum;           // La operación anterior se ve reflejada en el total de tickets.
        }
        else {                          // Si no hay tickets en el total.
            thread->tickets = 10;       // Se le asigna al thread 10 tickets.
            total_tickets = 10;         // La operación anterior se ve reflejada en el total de tickets.
        }
    }
    PRINTINFO("Se aumentó la prioridad de un thread\n");
}

// Función que maneja las interrupciones para terminar el proceso con "Cntrl + C". Va recorriendo las colas de threads
// liberando memoria.
void sigint_handler(int signo) {
    threads_queue *temp_queue = tcb_head, *temp_queue2, *queue_to_free;
    mythread *temp_thread;
    while(temp_queue != NULL) {                     // Se recorre por toda la cola de threads para liberar su espacio.
        temp_thread = temp_queue->thread;
        temp_queue2 = temp_thread->w_threads_head;

        free(temp_thread->return_value);            // Se libera el valor que poseía el thread como resultado.
        temp_thread->return_value = NULL;

        while(temp_queue2 != NULL) {                // Se liberan los punteros a los threads que tenía el thread actual
            queue_to_free = temp_queue2;            // esperando.
            temp_queue2 = temp_queue2->next;
            free(queue_to_free);
        }
        free(temp_thread);                          // Se libera el thread.
        temp_thread = NULL;

        queue_to_free = temp_queue;
        temp_queue = temp_queue->next;              // Se continua con el siguiente.
        free(queue_to_free);
    }

    temp_queue = dthreads_head;
    while(temp_queue != NULL) {                     // Se recorre por toda la cola de threads muertos para
        temp_thread = temp_queue->thread;           // liberar su espacio.

        free(temp_thread->return_value);            // Se libera el valor de retorno.
        temp_thread->return_value = NULL;
        free(temp_thread);                          // Se libera el espacio del thread.
        temp_thread = NULL;

        queue_to_free = temp_queue;
        temp_queue = temp_queue->next;              // Se continua con el siguiente.
        free(queue_to_free);
    }

    // Se libera la memoria utilizada por el thread actual.
    free(current->return_value),
            free(current->w_threads_head);
    free(current->w_threads_tail);
    current = NULL;
}

// Función que indica si existe algún thread que no ha iniciado en el programa.
int has_not_started(tcb *tcb_head) {
    while(tcb_head != NULL) {
        if(!tcb_head->thread->started) return 1;
        tcb_head = tcb_head->next;
    }
    return 0;
}