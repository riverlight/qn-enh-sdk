#ifndef QNFILTER_SDK_H
#define QNFILTER_SDK_H

#include "qnfilter-basedef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum QNFILTER_TYPE_e {
	QF_LOWLIGHT = 0, 
	QF_DEBLOCK, 
	QF_DEHAZE, 
	QN_NUMBER
} QNFILTER_TYPE;

typedef struct QNFilterSetting_s {
	double dLowlight_w;
	double dDehaze_w;
} QNFilterSetting;

int QNFilter_Create(Handle* pHandle, QNFILTER_TYPE type);
int QNFilter_Process_I420(Handle handle, unsigned char* pYuv420, int width, int height);
int QNFilter_Destroy(Handle handle);

int QNFilter_Setting(Handle handle, QNFilterSetting *pSetting);

#ifdef __cplusplus
}
#endif

#endif // QNFILTER_SDK_H
