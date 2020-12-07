

#ifndef REMEMBER_CLIENT_H
#define REMEMBER_CLIENT_H

#include "data_structures.h"

void newMonitor(ListMonitors **pMonitors, char *data);
void setConnections(ListMonitors **pMonitors, int *clients);
void clear(ListMonitors *monitors);
void closeSockets(int *clients);
void setCursor(ListMonitors *monitors);


#endif //REMEMBER_CLIENT_H
