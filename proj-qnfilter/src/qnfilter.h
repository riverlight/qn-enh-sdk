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
	void guidedFilter_mt(Mat& out, cv::Mat I, cv::Mat p, int r, double eps);

private:
	int _nDehaze_erode_r;
	int _nDehaze_R;
	double _dDehaze_eps;
	double _dDehaze_w;
	double _dDehaze_maxV1;

private:
	// guidefilter
	Mat _tm, _imgP;
	Mat _V1, _imgDark, _m_gray;
	Mat _I_32, _mean_I, _mean_p, _mean_Ip, _cov_Ip, _var_I, _gf_a, _gf_b, _mean_a, _mean_b, _gf_out_32;
	Mat _mul_Ip, _II, _meanImeanI, _mean_II, _AmeanI, _ImeanA;
	void MAT_INIT(Mat& m, Mat& ref, int ddepth) { if (m.empty()) m = Mat(ref.size(), ddepth); }


private:
	QNFILTER_TYPE _eType;
};


#endif // QNFILTER_H
