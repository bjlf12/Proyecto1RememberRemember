#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Server.h"
#include "Parser.h"
#include "mythread.h"
#include "mymutex.h"

int port = 8080;

/**
 * Función que se utilizar para inicializar los mutex que se utilizan durante la ejecución del programa, tambien inicia
 * la ejecución de un hilo por cada figura que se parsea desde el archivo de entrada de la animación.
 * @param scene matriz con el estado de todas las posiciones de la pantalla.
 * @param monitors lista con los datos de los monitores de los clientes donde se mostrara las animaciones.
 * @param figures lista de las figuras parseadas desde el archivo pasado por parametros.
 * @param threads arreglo de hilos de tamaño del largo de la lista de figuras.
 * @param countFigures largo de la lista de figuras.
 */
void run(Scene *scene, ListMonitors *monitors, ListFigures *figures, mythread_t *threads, int countFigures) { //todo hacer esto void
    mythread_init(500, unlockNPrint, monitors, scene);
    mymutex_init(&mutex);
    mymutex_init(&str);
    for (int i = 0; i < countFigures; ++i) {
        Arguments *arguments = (Arguments*)malloc(sizeof(Arguments));
        arguments->scene = scene; arguments->figures = figures; arguments->clients = monitors;
        if(figures->startX != figures->endX || figures->startY != figures->endY) {
            printf("Figura:%d, Scheduler:%d, Tickets:%d, TiempoInicio%d, TiempoFinal:%d\n", i+1, figures->scheduler, figures->tickets, figures->startTime, figures->finishTime);
            mythread_create(&threads[i], startFigure, arguments, figures->scheduler, figures->tickets, figures->startTime, figures->finishTime, figures); // Se inicia un hilo por cada figura de la lista de figuras.
        }
        figures = figures->next;
    }
    int *result = (int *)malloc(sizeof(int));
    for (int i = 0; i < countFigures; ++i) {
        mythread_join(threads[i], (void *) result);
    }
    mymutex_destroy(&mutex);
    mymutex_destroy(&str);
    printf("Hola Mundo\n");
    sleep(5);
    printf("Hola Mundo\n");
    clear(monitors);
}

/**
 * Función utilizada para configurar la conexion del servidor, así como para parsear los datos de los archivos parados
 * por parametros, recibe las conexiones de los clientes y se encarga de cuando se hayan conectado correctamente todos
 * los monitorse clientes.
 * @param argc
 * @param argv
 */

void start(int argc, char **argv) {
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sin_len = sizeof(clientAddress);
    int serverSocket, temp = 1, flag = TRUE, i = 0, countClients = 4, clientSocket[argc - 4], *size;
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error starting connection.\n");
        exit(1);
    }
    ListFigures *figures = parseFigures(argv[2]);
    ListMonitors *monitors = parseMonitors(argc - (argc - 4) , argv, argc);
    size = calculateSceneDimensions(monitors);
    Scene *scene = newScene(size);
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);
    if ((bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress))) <
        0) {      // Si no se puede enlazar
        fprintf(stderr, "Binding error.\n");                                           // el socket del servidor
        close(serverSocket);                                                                     // se finaliza la ejecución
        exit(1);                                                                           // con un error.
    }
    listen(serverSocket, MAX);
    while (flag) { // Se inicia un ciclo hasta que se hayan conectado todos los clientes
        printf("\033[0;32mWaiting for %d clients\n", argc - countClients);
        int newClient;
        newClient = accept(serverSocket, (struct sockaddr *) &clientAddress, &sin_len);
        write(newClient, "\nWaiting for clients\n", 21);
        if (newClient < 0) {
            fprintf(stderr, "Binding error.\n");                                           // el socket del servidor
            close(serverSocket);                                                                     // se finaliza la ejecución
            exit(1);
        } else if (i == argc - 5) {
            int countFigure = countFigures(argv[2]);
            clientSocket[i] = newClient;
            setConnections(&monitors, clientSocket);
            clear(monitors);
            mythread_t *pthreads = (mythread_t*)malloc(countFigure*sizeof(mythread_t));
            run(scene, monitors, figures, pthreads, countFigure);
            flag = FALSE;
            closeSockets(clientSocket);
        } else if (i < argc - 4) {
            clientSocket[i] = newClient;
            countClients++;
        } else if (i > argc - 4) {
            close(newClient);
        }
        i++;
    }
}
/**
 * Funcion principal del programa recibe los argumentos y verifica si
 * @param argc Cantidad de argumentos
 * @param argv Arreglo de strings obtenidos desde la terminal
 * @return 0 si se termino correctamente la ejecucion
 */
int main(int argc, char **argv) {
    if (argc <= 1) fprintf(stderr, "Arguments error");
    else if((strcmp(argv[1], "-e")) == 0) start(argc, argv);
}