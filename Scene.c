
#include "Scene.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define ecsplosion "<*^*>,<***>,<*0*>,<***>,<*|*>"

/**
 * Esta función se utiliza para crear el escenario necesario para poder mostrar las figuras y que cumpla con las dimensiones
 * de los monitores de los clientes, es decir que tenga el archo y largo indicado, ademas tendra los campos o celdas con la
 * información necesaria para saber si se encuentra bloqueados o no una casilla.
 * @param dimensions Arreglo de enteros de largo dos, contiene el largo y archo que debe tener el escenario.
 * @return Una estrucuta de tipo Scene con la información necesaria
 */
Scene *newScene(int *dimensions) {
    Scene *new = (Scene*)malloc(sizeof(Scene)); // Se asigna la memoria correspondiente para el nuevo escenario.
    Field *field = (Field*)malloc(sizeof(Field)); // Se asigna la memoria para el primer campo de las celdas o campos.
    Field *temp = NULL;
    int x = dimensions[0], y = dimensions[1]; // Se leen los valores de arreglo de las dimensiones obtenido por parametros a la función.
    new->length = y; // Se asigna el largo necesario.
    new->width = x; // Se le asgina el ancho necesario.
    new->len = x*y; // Los campos se manejan mediante una lista simple por lo que el largo de esta lista corresponde a el largo por el ancho.
    new->field = field; // Se le asigna el primer campo.
    for (int i = 0; i < y; ++i) { // Ahora se crean los campos necesarios para representar cada campo de la matriz.
        for (int j = 0; j < x; ++j) {
            temp = (Field*)malloc(sizeof(Field)); // Se le asigna la memoria
            temp->isLocked = FALSE; // Se marca como un campo libre
            temp->x = i; // Se le da un valor X
            temp->y = j; // Se le da un valor Y
            temp->next = NULL;
            if(field != NULL) field->next = temp; // Se añade al final de la lista
            field = temp;
        }
    }
    return new; // Se retorna el escenario creado con los datos necesarios.
}

/**
 * Función que revisa las dimensiones de los monitores para calcular el largo y ancho que debe tener el escenario
 * @param monitors Lista de los monitores parseados desde los argumentos del programa.
 * @return Arreglo de largo dos con la información de largo y ancho en los campos correspondientemente
 */
int *calculateSceneDimensions(ListMonitors *monitors){
    int *result = (int*)malloc(2*sizeof(int)); // Se asigna la meemoria para el arreglo.
    int x = 0, y = 0;
    while (monitors != NULL){ // Se itera entre la lista de monitores.
        if (x < monitors->y2) x = monitors->y2; // Si el valor x es menor al del punto final del monitor actual se actualiza
        if (y < monitors->x2) y = monitors->x2; // Si el valor x es menor al del punto final del monitor actual se actualiza
        monitors = monitors->next;
    }
    result[0] = x; // Se ponen los valores en el arreglo.
    result[1] = y;
    return result; // Se retorna el arreglo.
}

/**
 * Función utilizada para bloquear las posiciones del escenario necesarias para imprimir las figuras y evitar que otres
 * ingresen.
 * @param scene Escenario con la información de los campos, los cuales contienen un campo para marcar como bloqueados.
 * @param x Número de Fila del campo que se va a bloquear.
 * @param y Número de Columna del campo que se va a bloquear.
 */
void lockPositions(Scene *scene, int x, int y) {
    struct Field *field;
    for (int i = x - 2; i < x + 3 ; ++i) { // Se itera en las posiciones adyacenyes a la posición de X y Y, para asegurar que de esquina a esquina no se pueda introducir otra figura
        for (int j = y - 2; j < y + 3; ++j) {
            field = scene->field;
            while (field != NULL) { // Se busca en la lista de campos para encontrar el que tenga los valores "i" y "j" que coinsidan
                if(field->x == i && field->y == j) {
                    field->isLocked = TRUE; // Se marca el campo como bloqueado
                    break;
                }
                field = field->next;
            }
        }
    }
}

/**
 * Función utilizada para marcar los campos como desbloqueados y que otras figuras puedan ingresar a estos.
 * @param scene Escenario con la información de los campos, los cuales contienen un campo para marcar como bloqueados.
 * @param x Número de Fila del campo que se va a bloquear.
 * @param y Número de Columna del campo que se va a bloquear.
 */
void unlockPositions(Scene *scene, int x, int y) {
    Field *field;
    for (int i = x - 2; i < x + 3 ; ++i) { // Se itera en las posiciones adyacenyes a la posición de X y Y, para asegurar que de esquina a esquina se desbloquee
        for (int j = y - 2; j < y + 3; ++j) {
            field = scene->field;
            while (field != NULL) { // Se busca en la lista de campos para encontrar el que tenga los valores "i" y "j" que coinsidan
                if(field->x == i && field->y == j) {
                    field->isLocked = FALSE;  // Se marca el campo como desbloqueado
                    break;
                }
                field = field->next;
            }
        }
    }
}

/**
 * Función para impirmir la exploción de una figura
 * @param monitors Lista de monitores creada al inicio del programa.
 * @param figure Lista de figuras creada al inicio.
 * @param scene Escenario en el que se
 */
void unlockNPrint(ListMonitors *monitors, ListFigures *figure, Scene *scene) {
    printFigure(monitors, ecsplosion, figure->startX, figure->startY); // Imrpime la figura de explosión
    sleep(1);
    printFigure(monitors, TEMP2, figure->startX, figure->startY); // Imrpime la figura de explosión
    printFigure(monitors, ecsplosion, figure->startX, figure->startY); // Imrpime la figura de explosión
    printFigure(monitors, TEMP2, figure->startX, figure->startY); // Imrpime la figura de explosión
    unlockPositions(scene, figure->startX, figure->startY); // Desbloque los campos para que otra figura pase
}

/**
 * Función para saber si los campos adyacentes a un punto se encuentran o no bloqueados.
 * @param scene
 * @param x Número de Fila del campo que se va a bloquear.
 * @param y Número de Columna del campo que se va a bloquear.
 * @return 1 verdadero o 0 falso
 */
int isFieldLock(Scene *scene, int x, int y){
    Field *field;
    int isLock = FALSE;
    for (int i = x - 2; i < x + 3 ; ++i) {// Se itera en las posiciones adyacenyes a la posición de X y Y, para asegurar que se encuentren todos desploqueados
        for (int j = y - 2; j < y + 3; ++j) {
            field = (Field*)malloc(sizeof(Field));
            field = scene->field;
            int flag = FALSE;
            while (field != NULL) { // Se busca en la lista de campos para encontrar el que tenga los valores "i" y "j" que coinsidan
                if (field->x == i && field->y == j) {
                    flag = field->isLocked; // Se actualiza la bandera segun la información de la celda
                    break;
                }
                field = field->next;
            }
            if (flag) { // Si una casilla esta bloqueada
                isLock = TRUE;
                return isLock; // Se retorna un 1 o verdadero
            }
        }
    }
    return isLock; // Sino se retorna un 0 significando que no esta bloqueado.
}