

#ifndef _PMON_H_
#define _PMON_H_

#include "xil_types.h"


int Init_Pmon (void);
void Reset_Pmon (void);
void SetPmonAXIS(void);
void GetPmonAXIS(void);
void SetPmonAXI4(void);
void GetPmonAXI4(void);


#endif
