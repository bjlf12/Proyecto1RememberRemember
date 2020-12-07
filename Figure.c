#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "Figure.h"
#include "mymutex.h"

/**
 * Función para crear una nueva figura y añadirla en la lista de figuras con los datos que son obtenidos desde el archivo
 * de configuración del animador.
 * @param figures Puntero a la posición en la que se almacenara el resultado.
 * @param data String con los datos obtenidos del archivo, en cual se separará por espacios.
 */
void *newFigure(ListFigures **figures, char *data) {
    int scheduler = 0, countFigures = 0;
    ListFigures *new = (ListFigures*)malloc(sizeof(ListFigures)); // Se asigna la memoria necesaria para crear la figura.
    ListFigures *last = *figures; // Puntero que apunta al final de la lista.
    char *split, *temp;
    split = strtok(data, " "); // Se dividen los datos por espacios.
    int index = 0;
    while (split != NULL) {
        if (index == 1) { // El primer valor corresponde al tipo de figura.
            temp = malloc(strlen(split));
            strcpy(temp, split);
            new->type = temp;
        }
        else if (index == 2) { // El segundo valor corresponde al tipo de planificador
            scheduler = atoi(split); // 0 Round Robin o 1 Lottery
            new->scheduler = scheduler;
        }
        else if (index > 2) { // Ahora con los demas valores:
            if(!scheduler) { //Si es Lottery (Sorteo)
                if (index == 3) new->tickets = atoi(split); // Se agregan los tickets
                else if (index == 4) new->rotation = atoi(split); // Se asigna la rotación
                else if (index == 5) new->startTime = atoi(split) * 1000; // Se asigna el tiempo inicial.
                else if (index == 6) new->finishTime = atoi(split) * 1000; // Se asigna el tiempo final.
                else if (index == 7) new->startX = atoi(split); // El valor X del punto inicial
                else if (index == 8) new->startY = atoi(split); // El valor Y del punto inicial
                else if (index == 9) new->endX = atoi(split); //  El valor X del punto final
                else if (index == 10) {
                    new->endY = atoi(split);  // El valor Y del punto final
                    new->id = countFigures; // Se el asigna el id
                    new->isLive = TRUE; // Se marca como activa.
                    countFigures++;
                }
            } else { // Si el planificador es Round Robin
                if (index == 3) new->rotation = atoi(split);// Se asigna la rotación
                else if (index == 4) new->startTime = atoi(split) * 1000; // Se asigna el tiempo inicial.
                else if (index == 5) new->finishTime = atoi(split) * 1000; // Se asigna el tiempo final.
                else if (index == 6) new->startX = atoi(split); // El valor X del punto inicial
                else if (index == 7) new->startY = atoi(split); // El valor Y del punto inicial
                else if (index == 8) new->endX = atoi(split); //  El valor X del punto final
                else if (index == 9) {
                    new->endY = atoi(split); // El valor Y del punto final
                    new->id = countFigures;  // Se el asigna el id
                    new->tickets = 0;
                    new->isLive = TRUE; // Se marca como activa.
                    countFigures++;
                }
            }
        }
        split = strtok(NULL, " "); // Se continua dividiendo los datos en espacios
        index++;
    }
    new->ascii = parseAscii(new->type); // Se obtiene el contenido del archivo correspodiente a cada figura.
    new->next = NULL;
    if(*figures == NULL) *figures = new; // Si la lista esta vacia se asigna el
    else { // Sino se avanza hasta llegar al final de la lista.
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = new;
    }
}

/**
 * Función utilizada para obtener el contenido de los archivos de las distintas figuras.
 * @param figureFile Nombre del archivo que contiene la figura.
 * @return String con el valor obtenido del archivo
 */
char *parseAscii(char *figureFile) {
    char *buffer = malloc(35*sizeof(char)); //Se crea un buffer para almacenar los datos obtenidos del archivo.
    FILE *file = fopen(figureFile, "r"); // Se abre el archvio.
    if (file == NULL) fprintf(stderr, "Doesnt exist figure ASCII."); // Sino se logro abrir muestra un error.
    fgets(buffer,31, file); // Sino solo lee los datos.
    fclose(file); // Se cierra el archivo.
    return buffer; // Se retorna el resultado.
}

/**
 * Función utilizada para enviar los caracteres que se imprimiran en el cliente.
 * @param monitors Lista de monitores de los clientes.
 * @param figure String con los datos del ASCII art.
 * @param x Número de fila donde se imprimira
 * @param y Número de columna donde se imprimira.
 */
void printFigure(ListMonitors *monitors, char figure[], int x, int y) {
    mymutex_lock(&str); //todo probar
    char buffer[35]; // Buffer temporal
    strcpy(buffer, figure);
    char *split = strtok(buffer, ","); // Se divide la figura por comas.
    int i = y, j = x;
    for (int k = 0; k < 5; ++k) { // Se itera entre los elementos del char que se van a imprimir.
        sendFigure(monitors, split, i, j); // Se envia finalmente al cliente.
        split = strtok(NULL, ",");
        i++;
    }
    mymutex_unlock(&str);
}

/**
 * Función utilizada para envíar la información a los clientes y estos puedan observarla en los monitores correspondientes
 * @param pMonitors Lista de monitores de los clientes.
 * @param line Primera fila de la figura a envíar al cliente.
 * @param x  Fila en la que se imprimira.
 * @param y Columa en la que se imprimira.
 */
void sendFigure(ListMonitors *pMonitors, char line[], int x, int y) {
    static char buffer[9]; // Buffer donde se ingresan los datos a envíar.
    int i = y, j = x ;
    ListMonitors *monitor;
    for (int k = 0; k < strlen(line); ++k) { // Se itera por el largo de la linea que se envia al cliente.
        monitor = pMonitors;
        char c = line[k];
        while(monitor != NULL) { // Se avanza en la lista de monitores para ver a cual debe enviarse los datos.
            if (monitor->x1 <= i && monitor->x2 >= i && monitor->y1 <= j && monitor->y2 >= j) { // Si el valor de los puntos es inferior al de los puntos del monitor se elige el monitor para enviar los datos.
                sprintf(buffer, "\033[%d;%dH%c", i - (monitor->x2 - monitor->length), j - (monitor->y2 - monitor->width), c); // Se calcula segun en punto y el largo y ancho del monitor la posición en la que se debe imprimir, esto utlizando los ajustes de terminal.
                write(monitor->client, buffer, strlen(buffer)); // Se envían los datos al cliente para que imprima el valor.
                break; // Se termina de iterar.
            }
            monitor = monitor->next;
        }
        i++;
    }
    setCursor(pMonitors);
}

/**
 * Funcion utilizara para asignar a una figura los posibles futuros movimientos que pueda realizar, tanto la cantidad,
 * como a los puntos a los que se puede movilizar.
 * @param figure Figura a la que se le buscara los proximos movientos.
 */
void searchPositions(ListFigures *figure) {
    figure->moves = countMoves(figure->startX, figure->startY, figure->endX, figure->endY); // Calcula la cantidad de movimientos
    figure->nextMoves = nextMoves(figure->startX, figure->startY, figure->endX, figure->endY, figure->moves); // Almacena los movimientos.
}

/**
 * Esta función se encarga de almacenar todas las posibles posiciones a las cuales les es posible moverse a una figura,
 * ya sea adelante, atras, ariba o abajo o en diagonal.
 * @param x1 Punto de origen en X.
 * @param y1 Punto de origen en Y.
 * @param x2 Punto de destino en X.
 * @param y2 Punto de destino en Y.
 * @param rows Cantidad de movimientos que se pueden hacer.
 * @return La matriz con las casillas de los movimientos.
 */
int **nextMoves(int x1, int y1, int x2, int y2, int rows){
    int **result = (int**)malloc(rows*sizeof(int)); //Se crea una matriz de largo N
    for (int i = 0; i < rows; i++) result[i] = (int *) malloc (2*sizeof(int)); // y ancho 2
    int index = 0;
    if(x1 > x2 && y1 < y2) { // Si el solo las Y de origen es menor al de destino se mueve al suroeste.
        result[index][0] = x1 - 1;
        result[index][1] = y1 + 1;
        index++;
    }
    if(x1 < x2 && y1 < y2) { // Si el punto de origen es menor al de destino se mueve al sureste.
        result[index][0] = x1 + 1;
        result[index][1] = y1 + 1;
        index++;
    }
    if(x1 > x2 && y1 > y2) { // Si el punto de origen es mayor al de destino se mueve al noroeste.
        result[index][0] = x1 - 1;
        result[index][1] = y1 - 1;
        index++;
    }
    if(x1 > x2 && y1 < y2) { // Si el punto de origen es mayor al de destino se mueve al noreste.
        result[index][0] = x1 + 1;
        result[index][1] = y1 - 1;
        index++;
    }
    if(y1 < y2) { //Se mueve al sur.
        result[index][0] = x1;
        result[index][1] = y1 + 1;
        index++;
    }
    if(x1 > x2) { // Se mueve al oeste.
        result[index][0] = x1 - 1;
        result[index][1] = y1;
        index++;
    }
    if(x1 < x2) { // Se mueve al este.
        result[index][0] = x1 + 1;
        result[index][1] = y1;
        index++;
    }
    if(y1 > y2) { // Se mueve al norte.
        result[index][0] = x1;
        result[index][1] = y1 - 1;
        index++;
    }
    return result;
}

/**
 * Función para contar la cantidad de movimientos posibles de un figura desde su punto actual al destino.
 * @param x1 Punto de origen en X.
 * @param y1 Punto de origen en Y.
 * @param x2 Punto de destino en X.
 * @param y2 Punto de destino en Y.
 * @return Cantidad de movimientos.
 */
int countMoves(int x1, int y1, int x2, int y2) {
    int result = 0;
    if(x1 > x2 && y1 < y2) result++; // Si el solo las Y de origen es menor al de destino se mueve al suroeste.
    if(x1 < x2 && y1 < y2) result++; // Si el punto de origen es menor al de destino se mueve al sureste.
    if(x1 > x2 && y1 > y2) result++; // Si el punto de origen es mayor al de destino se mueve al noroeste.
    if(x1 > x2 && y1 < y2) result++; // Si el punto de origen es mayor al de destino se mueve al noreste.
    if(y1 < y2) result++; // Se mueve al sur.
    if(x1 > x2) result++; // Se mueve al oeste.
    if(x1 < x2) result++; // Se mueve al este.
    if(y1 > y2) result++; // Se mueve al norte.
    return result;
}

/**
 * Funcion utilizada para comprobar si un objeto puede moverse a una de los puntos que se obtuvieron anteriormente.
 * @param scene Matriz camvas donde se encuentran las casillas, las cuales se pueden bloquear y desbloquear,
 * @param monitors
 * @param figure
 */
void move(Scene *scene, ListMonitors *monitors,  ListFigures *figure) {
    mymutex_lock(&mutex); // Se realiza un mutex para evitar que otros hilos consulten al mismo tiempo es estado de las casillas del escenario y así no se sobrepongan.
    char *turn;
    if (figure->startX != figure->endX || figure->startY != figure->endY){ // Se pregunta si la figura no esta ya en la posición final.
        int flag = FALSE, moves = figure->moves;
        for (int i = 0; i < moves; ++i) { // Se itera por todas las filas de la matriz de proxímos movimientos de la figura,
            int x = figure->nextMoves[i][0], y = figure->nextMoves[i][1]; // Se guandan los valores de la fila.
            unlockPositions(scene, figure->startX, figure->startY); // Se
            int enable = isFieldLock(scene, x, y); // Se reviza si la posición se destino no esta bloqueada
            if (!enable) { // Si esta desbloqueada
                flag = TRUE;
                lockPositions(scene, x, y); //Bloquea esas posiciones para mover la figura.
                figure->startX = x; // Actualiza la posición de la figura.
                figure->startY = y;
                printFigure(monitors, TEMP2, figure->startX, figure->startY); // Limpia el monitor del cliente en la nueva posición.
                turn = turnFigure(figure->ascii, figure->rotation); // Gira la figura.
                strcpy(figure->ascii, turn); // Actualiza el resultado.
                printFigure(monitors, figure->ascii, x, y); // Imprime finalmente la figura en el cliente.
                break;
            } else { // Si la casilla estaba bloqueada.
                lockPositions(scene, figure->startX, figure->startY); // Se vuelven a bloquear la posición actual.
                turn = turnFigure(figure->ascii, figure->rotation); // Se gira la figura.
                strcpy(figure->ascii, turn);
                printFigure(monitors, figure->ascii, figure->startX, figure->startY); // Se imprime en el monitor correspondiente.
                break;
            }
        }
    }
    mymutex_unlock(&mutex);
}

/***
 * Función para girar el contenido de la figura en los grados 0°, 90°, 180° y 270° segun los datos obtenidos del usuario.
 * @param ascii Contenido obtenido del archivo, es el ASCII art.
 * @param rotation Grados a los cuales dar vuelta a la figura.
 * @return Resuldado se haber realizado la rotación indicada.
 */
char *turnFigure(char *ascii, int rotation) {
    char *result = NULL;
    if (rotation == 0) return ascii; // Si la rotación es de cero, simplemente se retorna el parametro.
    if (rotation == 90) result = turn90(ascii); // Si es de 90, se llama a la función indicada y se devuelve la figura volteada 90°.
    if (rotation == 180) result = turn180(ascii); // Si es 180 se llama a la función de 180.
    if (rotation == 270) result = turn270(ascii); // Si es de 270 tambien se llama a la función de 270
    return result;
}

/**
 * Función que calcula la matriz transpuesta de una figura, es decir pasar las filas a columnas y las columnas por las filas.
 * @param ascii Arreglo de caracteres que sera transpuesto.
 * @return Resultado de aplicar la operacion al parametro recibido.
 */
char *turn90(char *ascii) {
    static char buffer[31]; static char result[31];
    sprintf(result, "%s", ascii);
    sprintf(buffer, "%s", ascii);
    for (int i = 0; i < 5; ++i) { // Se itera entre las filas
        int temp = 4;
        for (int j = 0; j < 5; ++j) { // Se itera entre las columnas.
            int id = i*6+j;
            int new = i+temp*6;
            result[new] = buffer[id]; // Realmente es un arreglo, al ser de tamaño fijo es simple calcular las posiciones.
            temp--;
        }
    }
    return result;
}

/**
 * Esta función se apoya de la de gigar 90° grados, ya que lo que realiza es llamar en dos ocaciones a esta para obtener
 * dos veces la transpuesta
 * @param ascii Valor obtenido del archivo.
 * @return Resultado de girar dos veces el string.
 */
char *turn180(char *ascii) {
    char *turn = turn90(ascii); // Gira 90°
    char *result = turn90(turn); // Gira otros 90° para tener los 180°
    return result;
}

/**
 * Esta función usa tres veces la de girar 90° para obtener el resultado deseado.
 * @param ascii Valor obtenido del archivo.
 * @return Resultado de girar tres veces el string.
 */
char *turn270(char *ascii) {
    char *turn = turn90(ascii); // Se gira se obtiene 90°
    char *turn2 = turn90(turn); // Se gira ahora son 180°
    char *result = turn90(turn2); // Finalmente se obtiene los 270°
    return result;
}

/**
 * Función utilizada para mover las distintas figuras en el escenario.
 * @param args Estructura temporal para poder pasar los argumentos correctamente.
 * @return
 */
void *startFigure(void *args) {
    while (1) {
        Arguments *arguments = (Arguments *) args; // Se castea el tipo de datos del parametro.
        ListFigures *figures = arguments->figures; // Se extrae la lista de figuras.
        ListMonitors *monitors = arguments->clients; // Se extrae la lista de monitores de los clientes.
        Scene *scene = arguments->scene; // Se extrae el escenario o camvas donde se muestra los resultados.
        if (figures->startX == figures->endX && figures->startY == figures->endY) { // Si una figura llega a su posición final
            printf("Termina: %d\n", current->mythread_id);
            mythread_end(0); // Se termina de ejecutar el hilo.
        }
        sleep(1);
        //usleep(500000);
        printFigure(monitors, TEMP, figures->startX, figures->startY); // Primero se imprime caracteres de espacio para garantizar que no se sobre imprima.
        lockPositions(scene, figures->startX, figures->startY);  // Luego se bloquean las posiciones
        searchPositions(figures); // Busca a que casillas se puede mover la figura.
        move(scene, monitors, figures); // Intenta realizar los movimientos de la figura
    }
}