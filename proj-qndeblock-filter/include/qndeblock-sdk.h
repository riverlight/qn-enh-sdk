#ifndef QNDEBLOCK_SDK_H
#define QNDEBLOCK_SDK_H

typedef void *QNDeblockHandle;

#ifdef __cplusplus
extern "C" {
#endif



int QNDeblockFilter_Create(QNDeblockHandle* pHandle, int nWidth, int nHeight, char *szModelUrl);
int QNDeblockFilter_Process_I420(QNDeblockHandle handle, unsigned char* pYuv420);
int QNDeblockFilter_Destroy(QNDeblockHandle handle);


#ifdef __cplusplus
}
#endif

#endif // QNDEBLOCK_SDK_H
