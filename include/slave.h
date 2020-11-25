#ifndef __SLAVE_PROCESS_H__
#define __SLAVE_PROCESS_H__

#include "RayTrace.h"

void slaveMain( ConfigData *data );
void slaveMPIHorizontal(ConfigData *data);
void slaveMPIVertical(ConfigData *data);
void slaveMPIBlock(ConfigData *data);
void slaveMPICylicVertical(ConfigData *data);
#endif
