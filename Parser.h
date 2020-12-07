//
// Created by estudiante on 1/12/20.
//

#include "Figure.h"

#ifndef REMEMBER_PARSER_H
#define REMEMBER_PARSER_H

ListFigures *parseFigures(char *figuresFileName);
ListMonitors *parseMonitors(int n, char **monitors, int len);
int countFigures(char *figuresFileName);


#endif //REMEMBER_PARSER_H
