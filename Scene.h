#ifndef REMEMBER_SCENE_H
#define REMEMBER_SCENE_H
#define TRUE 1
#define FALSE 0

#include "Server.h"
#include "Client.h"
#include "Figure.h"
#include "data_structures.h"

Scene *newScene(int *dimensions);
int *calculateSceneDimensions(ListMonitors *monitors);
void lockPositions(Scene *scene, int x, int y);
void unlockPositions(Scene *scene, int x, int y);
void unlockNPrint(ListMonitors *monitors, ListFigures *figure, Scene *scene);
int isFieldLock(Scene *scene, int x, int y);

#endif //REMEMBER_SCENE_H
