#include <opencv2/highgui/highgui.hpp>    
#include <opencv2/imgproc/imgproc.hpp>

#include "../include/qndeblock-sdk.h"
#include "qndeblock.h"

using namespace cv;


int QNDeblockFilter_Create(QNDeblockHandle* pHandle, int nWidth, int nHeight, char* szModelUrl)
{
	CQNDeblock* pFilter;
	pFilter = new CQNDeblock();
	if (pFilter == NULL)
	{
		*pHandle = NULL;
		return -1;
	}
	
	int ret = pFilter->Init(nWidth, nHeight, szModelUrl);
	if (ret!=0)
	{
		*pHandle = NULL;
		return ret;
	}

	*pHandle = pFilter;

	return 0;
}

int QNDeblockFilter_Process_I420(QNDeblockHandle handle, unsigned char* pYuv420)
{
	if (!handle)
		return -1;
	
	CQNDeblock* pFilter = (CQNDeblock*)handle;

	return pFilter->Process_I420(pYuv420);
}

int QNDeblockFilter_Destroy(QNDeblockHandle handle)
{
	CQNDeblock* pFilter = (CQNDeblock*)handle;
	if (!pFilter)
		return -1;

	delete pFilter;

	return 0;
}
