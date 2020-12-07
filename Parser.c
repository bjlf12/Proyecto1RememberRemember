#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Parser.h"


/**
 * Función utilizada para obtener la información de las figuras presentes en el archivo.
 * @param figuresFileName Nomber del archivo que contiene la información de las figuras.
 * @return String con la información extraida del archivo
 */
ListFigures *parseFigures(char *figuresFileName) {
    FILE* file = fopen(figuresFileName, "r"); // Se abre el archivo en modo lectura.
    if(file == NULL) fprintf(stderr, "Error open animation file"); // En caso de que no se pueda abrir se muestra un error.
    char *buffer, *split, *copy;
    size_t len = 0; ssize_t read;
    ListFigures *parserFigures = NULL;
    while ((read = getline(&buffer, &len, file)) != -1) { // Se lee linea por linea el archivo en busca de figuras
        copy = malloc(strlen(buffer));
        strcpy(copy, buffer);
        split = strtok(buffer, " "); // Si el inicio de cada linea corresponde a "createFigure"
        if((strcmp(split, "createFigure")) == 0) newFigure(&parserFigures, copy); // Se creara una nueva figura que se guarda en la lista de figuras resultado.
    }
    fclose(file); // Se cierra el archivo
    return parserFigures; // Se retorna una lista de figuras con el contenido que se obtuvo del archivo.
}

/**
 * Esta función se utiliza para analizar cuantas figuras se encuentran presentes en el archivo pasado por los parametros
 * @param figuresFileName Nombre y ruta del archivo que contiene las figuras.
 * @return
 */
int countFigures(char *figuresFileName) {
    FILE* file = fopen(figuresFileName, "r"); // Se abre el archivo en modo lectura.
    if(file == NULL) fprintf(stderr, "Error open animation file"); // En caso de que no se pueda abrir se muestra un error.
    char *buffer, *split, *copy;
    size_t len = 0; ssize_t read;
    int result = 0; // Cantidad de figuras.
    while ((read = getline(&buffer, &len, file)) != -1) { // Se lee linea por linea el archivo en busca de figuras
        copy = malloc(strlen(buffer));
        strcpy(copy, buffer);
        split = strtok(buffer, " ");
        if((strcmp(split, "createFigure")) == 0) result++; // Si el inicio de cada linea corresponde a "createFigure" se aumenta el contador
    }
    fclose(file); // Se cierra el archivo
    return result;
}

/**
 * Esta función se utiliza para obtener los datos de los monitos que son pasados por consola al inicio del programa,
 * extrae la información del archivo para añadir un nuevo monitor de cliente a la lista de monitores.
 * @param n Posición inicial de los monitores
 * @param monitors Arreglo de strings con los nombre de los archivos de cada monitor
 * @param length Largo del arreglo.
 * @return Lista de monitores con la información correspondiente a cada uno.
 */
ListMonitors *parseMonitors(int n, char **monitors, int length){
    ListMonitors *parseMonitors = NULL;
    for (int i = n; i < length; ++i) { // Se itera el arreglo desde la posición N h
        FILE* file = fopen(monitors[i], "r"); // Se abre el archivo del monitor en la posición I.
        if (file == NULL) fprintf(stderr, "Error parsing monitor file."); // En caso de error se muestra un mensaje
        char *buffer, *split, *copy;
        size_t len = 0; ssize_t read;
        while ((read = getline(&buffer, &len, file)) != -1) { //Se lee la linea del archivo
            copy = malloc(strlen(buffer));
            strcpy(copy, buffer);
            split = strtok(buffer, " "); // Se verifica si la primera cosa que encontramos en la linea corresponde a "setMonitor"
            if((strcmp(split, "setMonitor")) == 0) newMonitor(&parseMonitors, copy); // Si es se crea un nuevo monitor y se agrega a la lsita.
        }
        fclose(file); // Se cierra el archivo
    }
    return parseMonitors; // La lista con la información de los monitores.
}


