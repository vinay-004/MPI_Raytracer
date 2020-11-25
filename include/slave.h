#ifndef __SLAVE_PROCESS_H__
#define __SLAVE_PROCESS_H__

#include "RayTrace.h"

void slaveMain( ConfigData *data );
// void slaveMPIHorizontal(ConfigData *data, float *pixels);
void slaveMPIVertical(ConfigData *data, float *pixels);
// void slaveMPIBlock(ConfigData *data, float *pixels);

#endif
