
#ifndef REMEMBER_SERVER_H
#define REMEMBER_SERVER_H

#include "data_structures.h"

#define TRUE 1
#define FALSE 0
#define MAX 100
#define TEMP "     ,     ,     ,     ,     ,"
#define TEMP2 "     ,     ,     ,     ,     ,"
mythread_mutex_t *mutex;
mythread_mutex_t *str;
int main(int argc, char **argv);
void start(int argc, char **argv);

#endif //REMEMBER_SERVER_H