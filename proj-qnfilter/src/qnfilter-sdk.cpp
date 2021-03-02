#include <opencv2/highgui/highgui.hpp>    
#include <opencv2/imgproc/imgproc.hpp>

#include "../include/qnfilter-sdk.h"
#include "qnfilter.h"

using namespace cv;


int QNFilter_Create(Handle* pHandle, QNFILTER_TYPE eType)
{
	CQNFilter* pFilter;
	pFilter = new CQNFilter(eType);
	if (!pFilter) {
		*pHandle = NULL;
		return -1;
	}
	*pHandle = pFilter;

	return 0;
}

int QNFilter_Process_I420(Handle handle, unsigned char* pYuv420, int width, int height)
{
	if (!handle) {
		return -1;
	}
	CQNFilter* pFilter = (CQNFilter*)handle;
	
	return pFilter->Process_I420(pYuv420, width, height);
}

int QNFilter_Destroy(Handle handle)
{
	CQNFilter* pFilter = (CQNFilter*)handle;
	if (!handle) {
		return -1;
	}

	delete pFilter;
	return 0;
}

int QNFilter_Setting(Handle handle, QNFilterSetting* pSetting)
{
	CQNFilter* pFilter = (CQNFilter*)handle;
	if (!handle) {
		return -1;
	}

	return pFilter->Setting(pSetting);
}