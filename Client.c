
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "Client.h"

/**
 * Función utilizada para crear una lista de monitores apartir de los monitores que son pasados por argumento a la hora
 * de ejecutarse el programa, cada archivo debe tener un formato preestablecido, el cual es el siguiente.
 * setMonitor id inicioX inicioY finX finY, donde los últimos cinco corresponden a numeros enteros.
 * @param pMonitors Puntero al lugar donde se almacenara los resultados.
 * @param data string con los valores extraidos de los archivos.
 */
void newMonitor(ListMonitors **pMonitors, char *data) {
    ListMonitors *new = (ListMonitors*)malloc(sizeof(ListMonitors)); // Se asigna la memoria correspondiente.
    ListMonitors *last = *pMonitors; // Puntero que apunta al ultimo elemento de la lista.
    char *split = strtok(data, " "); //Se divide el valor obtenido entre espacios
    int index = 0; // Variable que funciona como contador
    while (split != NULL){ // Se itera hasta llegar al final de las separaciones por espacio.
        if (index == 1) new->id = atoi(split); // Se lee el id
        if (index == 2) new->x1 = atoi(split); // Se lee el X del punto inicial.
        if (index == 3) new->y1 = atoi(split); // Se lee el Y del punto inicial.
        if (index == 4) new->x2 = atoi(split); // Se lee el X del punto final.
        if (index == 5) new->y2 = atoi(split); // Se lee el Y del punto final.
        split = strtok(NULL, " ");
        index++;
    }
    new->length = new->x2 - new->x1; // Se le asigna el largo del monitor con la resta entre el punto final e inicial
    new->width = new->y2 - new->y1;
    new->next = NULL;
    if (*pMonitors == NULL) *pMonitors = new; // Si la lista esta vacia se asigna el valor al puntero del final.
    else{
        while (last->next != NULL){ // Si no se itera hasta llegar al final de la lista.
            last = last->next;
        }
        last->next = new;
    }
}

/**
 * Esta funcion se utiliza para mover el cursor de escritura en terminal a su posición original, es decir (0,0), esto se
 * realiza en las terminales de los clientes que se conectan al programa para imprimir las figuras.
 * @param monitors Lista de los monitores que fueron pasados por parametros
 */
void setCursor(ListMonitors *monitors) {
    char *restart = "\033[0;0H\n"; //Este formato de texto se utiliza para colocar el cursor en un posicion X,y de la pantalla https://repl.it/talk/ask/I-want-to-use-clear-screen-function-in-my-c-code-on-repl/11001 .
    while (monitors != NULL){ // Se itera entre la lista de monitores para restablecer la posición del cursor.
        int client = monitors->client;
        write(client, restart, strlen(restart)); // El valor de client fue el que se obtubo a la hora en que se conectaron los clientes.
        monitors = monitors->next;
    }
}

/**
 * Función utilizada para asignar a cada monitor que se pasa por parametro al inicio del programa, el valor obtenido de
 * la conexión del Cliente con el socket del server mediante telnet.
 * @param pMonitors Puntero de la lista de monitores parseados.
 * @param clients Arreglo de enteros con el valor obtenido a la hora de conexión de cada cliente.
 */
void setConnections(ListMonitors **pMonitors, int *clients) {
    ListMonitors *temp = *pMonitors;
    int index = 0;
    while (temp != NULL){ // Se itera la lista
        temp->client = (int) clients[index]; // Se le asigna al monitor el valor en el arreglo en la posición index.
        temp = temp->next; // Se avanza en la lista.
        index++;
    }
}

/**
 * Utiliza una secuencia de terminal para limpiar el contenido actual de las terminales, el valor \033 que el valor del
 * caracter ESC que a la hora de unirce con "c" restablece los valores de la terminal a los predeterminados.
 * @param monitors Lista de monitores que van a ser limpiados, o bien restablecer a lo predeterminado.
 */
void clear(ListMonitors *monitors){
    while (monitors != NULL) { // Se itera en la lista de monitores.
        int clientSocket = monitors->client;
        write(clientSocket, "\033c", 2); // Se escribe en el socket de cada cliente, a quien corresponda el
        // monitor para que se restablesca https://stackoverflow.com/questions/47503734/what-does-printf-033c-mean .
        monitors = monitors->next; // Se avanza en la lista.
    }
}

/**
 * Función utilizada para que al final de haber mostrado el contenido en las terminales, se cierre la conexión con los
 * clientes y así evitar dejar sockets abiertos.
 * @param clients Arreglo que contierne todos los valores enteros obtenidos de la conexión de los clientes.
 */
void closeSockets(int *clients) {
    int len = sizeof(clients); // Se calcula el largo del arreglo
    for(int i = 0; i < len; i++) { //Se itera el arreglo desde la posición 0 hasta N - 1.
        close(clients[i]); // Se cierra la conexión con el cliente.
    }
}