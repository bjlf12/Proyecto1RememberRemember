#ifndef REMEMBER_FIGURE_H
#define REMEMBER_FIGURE_H

#include "Scene.h"
#include "data_structures.h"


char *parseAscii(char *figureFile);
void *newFigure(ListFigures **figures, char *data);
void printFigure(ListMonitors *monitors, char figure[], int x, int y);
void sendFigure(ListMonitors *pMonitors, char *line, int x, int y);
int countMoves(int x1, int y1, int x2, int y2);
void searchPositions(ListFigures *figure);
int **nextMoves(int x1, int y1, int x2, int y2, int rows);
void move(Scene *scene, ListMonitors *monitors,  ListFigures *figure);
char *turnFigure(char *ascii, int rotation);
void *startFigure(void *args);
char *turn90(char *ascii);
char *turn180(char *ascii);
char *turn270(char *ascii);

#endif //REMEMBER_FIGURE_H
