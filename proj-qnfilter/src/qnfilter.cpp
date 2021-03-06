#include <thread>
#include <opencv2/highgui/highgui.hpp>    
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#include "utils-mt.h"
#include "qnfilter.h"

using namespace cv;
using namespace std;

#define LMin(x, y) ((x)<(y) ? (x) : (y))
#define LMin_xyz(x, y, z) LMin((x), LMin((y), (z)))
#define LMax(x, y) ((x)>(y) ? (x) : (y))

CQNFilter::CQNFilter(QNFILTER_TYPE eType)
{
	_eType = eType;

	switch (eType)
	{
	case QF_LOWLIGHT:
		_nDehaze_erode_r = 3;
		_nDehaze_R = 81;
		_dDehaze_eps = 0.001;
		_dDehaze_w = 0.5;
		_dDehaze_maxV1 = 0.7;
		break;
	case QF_DEHAZE:
		_nDehaze_erode_r = 3;
		_nDehaze_R = 81;
		_dDehaze_eps = 0.001;
		_dDehaze_w = 0.6;
		_dDehaze_maxV1 = 0.8;
		break;
	default:
		_nDehaze_erode_r = 3;
		_nDehaze_R = 81;
		_dDehaze_eps = 0.001;
		_dDehaze_w = 0.6;
		_dDehaze_maxV1 = 0.8;
	}
}

int CQNFilter::Setting(QNFilterSetting* pSetting)
{
	if (!pSetting)
		return -1;

	_dDehaze_w = pSetting->dW;

	return 0;
}

int CQNFilter::Process_I420(unsigned char* pI420, int nWidth, int nHeight)
{
	if (!pI420)
		return -1;

	switch (_eType)
	{
	case QF_LOWLIGHT:
		Process_I420_lowlight(pI420, nWidth, nHeight);
		break;
	case QF_DEHAZE:
		Process_I420_dehaze(pI420, nWidth, nHeight);
		break;
	default:
		return -1;
	}

	return 0;
}

int CQNFilter::Process_I420_lowlight(unsigned char* pI420, int nWidth, int nHeight)
{
	Mat m = Mat(nHeight + nHeight / 2, nWidth, CV_8UC1, pI420);
	MAT_INIT(_tm, m, CV_8UC3);
	cvtColor(m, _tm, COLOR_YUV2BGR_I420);
	Filter_lowlight(_tm);
	cvtColor(_tm, m, COLOR_BGR2YUV_I420);
	//memcpy(pI420, m.data, nWidth * nHeight * 3 / 2);

	return 0;
}

int CQNFilter::Process_I420_dehaze(unsigned char* pI420, int nWidth, int nHeight)
{
	Mat m = Mat(nHeight + nHeight / 2, nWidth, CV_8UC1, pI420);
	MAT_INIT(_tm, m, CV_8UC3);
	cvtColor(m, _tm, COLOR_YUV2BGR_I420);
	Filter_dehaze(_tm);
	cvtColor(_tm, m, COLOR_BGR2YUV_I420);

	return 0;
}

int CQNFilter::Filter_lowlight(Mat& m)
{
//	Mat mm = m.clone();
	image_convert_mt(m);
	Filter_dehaze(m);
	image_convert_mt(m);
//	imshow("1", m);
//	waitKey(1);
	return 0;
}

void CQNFilter::image_convert(Mat& m)
{
	for (int i = 0; i < m.rows; i++)
		for (int j = 0; j < m.cols; j++)
		{
			m.at<Vec3b>(i, j)[0] = 255 - m.at<Vec3b>(i, j)[0];
			m.at<Vec3b>(i, j)[1] = 255 - m.at<Vec3b>(i, j)[1];
			m.at<Vec3b>(i, j)[2] = 255 - m.at<Vec3b>(i, j)[2];
		}
}

// guideFilter 整型实现
void CQNFilter::guidedFilter_int(Mat& out, cv::Mat I, cv::Mat p, int r, double eps)
{
	// I and P is CV_8UC1
	Mat mean_I;
	boxFilter(I, mean_I, CV_32SC1, Size(r, r));

	Mat mean_p;
	boxFilter(p, mean_p, CV_32SC1, Size(r, r));

	Mat mean_Ip;
	boxFilter(I.mul(p), mean_Ip, CV_32SC1, Size(r, r));

	Mat cov_Ip = mean_Ip - mean_I.mul(mean_p);

	Mat mean_II;
	boxFilter(I.mul(I), mean_II, CV_32SC1, Size(r, r));

	Mat var_I = mean_II - mean_I.mul(mean_I);

	Mat a = cov_Ip / (var_I + 1);
	Mat b = mean_p - a.mul(mean_I);

	Mat mean_a;
	boxFilter(a, mean_a, CV_32SC1, Size(r, r));

	Mat mean_b;
	boxFilter(b, mean_b, CV_32SC1, Size(r, r));

	Mat I_32;
	I.convertTo(I_32, CV_32SC1);
	Mat out_32;
	out_32 = mean_a.mul(I_32) + mean_b;
	out_32.convertTo(out, CV_8UC1);
}

void CQNFilter::get_darkchannel(Mat& m, Mat& imgDark)
{
	for (int i = 0; i < m.rows; i++)
		for (int j = 0; j < m.cols; j++)
			imgDark.at<uchar>(i, j) = LMin_xyz(m.at<Vec3b>(i, j)[0], m.at<Vec3b>(i, j)[1], m.at<Vec3b>(i, j)[2]);
}

void CQNFilter::calc_A_smp(Mat& m_gray, int& A)
{
	double dmax, dmin;
	cv::minMaxIdx(m_gray, &dmin, &dmax);
	A = dmax;
}

void CQNFilter::dehaze_getV1(int& A, Mat& V1, Mat&m, int r, float eps, float w, float maxV1)
{
	// get dark-channel
	MAT_INIT(_imgDark, V1, CV_8UC1);
	get_darkchannel_mt(m, _imgDark);

	//Mat p;
	MAT_INIT(_imgP, m, CV_8UC1);
	Mat element = getStructuringElement(MORPH_RECT, Size(_nDehaze_erode_r*2+1, _nDehaze_erode_r *2+1), Point(_nDehaze_erode_r, _nDehaze_erode_r));
	erode(_imgDark, _imgP, element);

	guidedFilter_mt(V1, _imgDark, _imgP, r, eps);
	
	int maxV1_i = maxV1 * 255;
	for (int i = 0; i < m.rows; i++)
		for (int j = 0; j < m.cols; j++) {
			V1.at<uchar>(i, j) = LMin(maxV1_i, V1.at<uchar>(i, j) * w);
		}

	// 计算 A
#if 0
	cvtColor(m, _m_gray, COLOR_BGR2GRAY);
	calc_A_smp(_m_gray, A);
	//cout << "dmax : " << A << endl;
#else
	int histSize = 256;
	float range[] = { 0,256 };
	const float* histRanges = { range };
	Mat dark_hist;
	calcHist(&_imgDark, 1, 0, Mat(), dark_hist, 1, &histSize, &histRanges, true, false);
	Mat hist_cumsum = Mat(dark_hist.size(), dark_hist.type());
	hist_cumsum.at<float>(0) = dark_hist.at<float>(0);
	for (int i = 1; i < 256; i++)
		hist_cumsum.at<float>(i) = dark_hist.at<float>(i) + hist_cumsum.at<float>(i-1);
	int lmax = 255;
	for (; lmax > -1; lmax--) {
		if (hist_cumsum.at<float>(lmax) <= 0.999 * m.rows * m.cols) {
			break;
		}
	}
	cvtColor(m, _m_gray, COLOR_BGR2GRAY);
	
	A = 0;
	for (int i = 0; i < m.rows; i++)
		for (int j = 0; j < m.cols; j++) {
			if (_imgDark.at<uchar>(i, j) >= lmax)
				A = _m_gray.at<uchar>(i, j) > A ? _m_gray.at<uchar>(i, j) : A;
		}
	//cout << "dmax : " << A << endl;
#endif
}

void CQNFilter::stretch_image(Mat& m, Mat& V1, int A)
{
	double A_64f = double(A) / 1.0;
	for (int i = 0; i < m.rows; i++)
		for (int j = 0; j < m.cols; j++) {
			double f = (1.0 - double(V1.at<uchar>(i, j)) / A_64f);
			int b = LMax(m.at<Vec3b>(i, j)[0] - V1.at<uchar>(i, j), 0);
			int g = LMax(m.at<Vec3b>(i, j)[1] - V1.at<uchar>(i, j), 0);
			int r = LMax(m.at<Vec3b>(i, j)[2] - V1.at<uchar>(i, j), 0);
			m.at<Vec3b>(i, j)[0] = LMin(double(b) / f, 255);
			m.at<Vec3b>(i, j)[1] = LMin(double(g) / f, 255);
			m.at<Vec3b>(i, j)[2] = LMin(double(r) / f, 255);
		}
}

int CQNFilter::Filter_dehaze(Mat& m)
{
	int A = 0;
	MAT_INIT(_V1, m, CV_8UC1);
	dehaze_getV1(A, _V1, m, _nDehaze_R, _dDehaze_eps, _dDehaze_w, _dDehaze_maxV1);

	stretch_image_mt(m, _V1, A);

#if 0
	imshow("v1", V1);
	imshow("m", m);
	waitKey(1);
	//exit(0);
#endif 

	return 0;
}
