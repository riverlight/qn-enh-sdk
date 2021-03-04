#ifndef QNFILTER_H
#define QNFILTER_H

#include <opencv2/highgui/highgui.hpp>    
#include <opencv2/imgproc/imgproc.hpp>

#include "../include/qnfilter-sdk.h"

using namespace cv;

class CQNFilter
{
public:
	CQNFilter(QNFILTER_TYPE eType);
	virtual ~CQNFilter() {}

	int Setting(QNFilterSetting* pSetting);
	int Process_I420(unsigned char* pI420, int nWidth, int nHeight);

private:
	int Process_I420_lowlight(unsigned char* pI420, int nWidth, int nHeight);
	int Process_I420_dehaze(unsigned char* pI420, int nWidth, int nHeight);

private:
	int Filter_lowlight(Mat &m);
	int Filter_dehaze(Mat& m);

	void dehaze_getV1(int &A, Mat &V1_64f, Mat &m, int r, float eps, float w, float maxV1);
	void image_convert(Mat& m);
	void guidedFilter_int(Mat& out, cv::Mat I, cv::Mat p, int r, double eps);

private:
	int _nDehaze_erode_r;
	int _nDehaze_R;
	double _dDehaze_eps;
	double _dDehaze_w;
	double _dDehaze_maxV1;

private:
	QNFILTER_TYPE _eType;
};


#endif // QNFILTER_H
