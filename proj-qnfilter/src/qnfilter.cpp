#include <opencv2/highgui/highgui.hpp>    
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#include "qnfilter.h"

using namespace cv;
using namespace std;

#define LMin(x, y) ((x)<(y) ? (x) : (y))
#define LMin_xyz(x, y, z) LMin((x), LMin((y), (z)))
#define LMax(x, y) ((x)>(y) ? (x) : (y))

CQNFilter::CQNFilter(QNFILTER_TYPE eType)
{
	_eType = eType;

	// lowlight
	_nLowLight_r = 27;
	_dLowLight_eps = 0.001;
	_dLowLight_w = 0.5;
	_dLowLight_maxV1 = 0.7;

	// dehaze
	_nDehaze_r = 27;
	_dDehaze_eps = 0.001;
	_dDehaze_w = 0.95;
	_dDehaze_maxV1 = 0.8;
}

int CQNFilter::Setting(QNFilterSetting* pSetting)
{
	if (!pSetting)
		return -1;

	_dLowLight_w = pSetting->dLowlight_w;
	_dDehaze_w = pSetting->dDehaze_w;

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
	cvtColor(m, m, COLOR_YUV2BGR_I420);
	Filter_lowlight(m);
	cvtColor(m, m, COLOR_BGR2YUV_I420);
	memcpy(pI420, m.data, nWidth * nHeight * 3 / 2);

	return 0;
}

int CQNFilter::Process_I420_dehaze(unsigned char* pI420, int nWidth, int nHeight)
{
	Mat m = Mat(nHeight + nHeight / 2, nWidth, CV_8UC1, pI420);
	cvtColor(m, m, COLOR_YUV2BGR_I420);
	Filter_dehaze(m);
	cvtColor(m, m, COLOR_BGR2YUV_I420);
	memcpy(pI420, m.data, nWidth * nHeight * 3 / 2);

	return 0;
}

int CQNFilter::Filter_lowlight(Mat& m)
{
	image_convert(m);
	Filter_dehaze(m);
	image_convert(m);
//	imshow("1", m);
//	waitKey(0);
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

void CQNFilter::dehaze_getV1(int& A, Mat& V1, Mat&m, int r, float eps, float w, float maxV1)
{
	// get dark-channel
	Mat imgDark = Mat(V1.size(), CV_8UC1);
	for ( int i=0; i<m.rows; i++ )
		for (int j = 0; j < m.cols; j++)
			imgDark.at<uchar>(i, j) = LMin_xyz(m.at<Vec3b>(i, j)[0], m.at<Vec3b>(i, j)[1], m.at<Vec3b>(i, j)[2]);

	Mat p;
	int erode_r = 5;
	Mat element = getStructuringElement(MORPH_RECT, Size(erode_r*2+1, erode_r*2+1), Point(erode_r, erode_r));
	erode(imgDark, p, element);

	guidedFilter_int(V1, imgDark, p, r, eps);
	
	int maxV1_i = maxV1 * 255;
	for (int i = 0; i < m.rows; i++)
		for (int j = 0; j < m.cols; j++) {
			V1.at<uchar>(i, j) = LMin(maxV1_i, V1.at<uchar>(i, j) * w);
		}

	// 计算 A
	int histSize = 256;
	float range[] = { 0,256 };
	const float* histRanges = { range };
	Mat dark_hist;
	calcHist(&imgDark, 1, 0, Mat(), dark_hist, 1, &histSize, &histRanges, true, false);
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
	Mat m_gray;
	cvtColor(m, m_gray, COLOR_BGR2GRAY);
	
	A = 0;
	for (int i = 0; i < m.rows; i++)
		for (int j = 0; j < m.cols; j++) {
			if (imgDark.at<uchar>(i, j) >= lmax)
				A = m_gray.at<uchar>(i, j) > A ? m_gray.at<uchar>(i, j) : A;
		}
}

int CQNFilter::Filter_dehaze(Mat& m)
{
	int A = 0;
	Mat V1 = Mat(m.size(), CV_8UC1);
	dehaze_getV1(A, V1, m, _nLowLight_r, _dLowLight_eps, _dLowLight_w, _dLowLight_maxV1);

	double A_64f = double(A) / 1.0;
	for (int i = 0; i < m.rows; i++)
		for (int j = 0; j < m.cols; j++) {
			int b = LMax(m.at<Vec3b>(i, j)[0] - V1.at<uchar>(i, j), 0);
			int g = LMax(m.at<Vec3b>(i, j)[1] - V1.at<uchar>(i, j), 0);
			int r = LMax(m.at<Vec3b>(i, j)[2] - V1.at<uchar>(i, j), 0);
			m.at<Vec3b>(i, j)[0] = (double(b) / (1.0 - double(V1.at<uchar>(i, j)) / A_64f));
			m.at<Vec3b>(i, j)[1] = (double(g) / (1.0 - double(V1.at<uchar>(i, j)) / A_64f));
			m.at<Vec3b>(i, j)[2] = (double(r) / (1.0 - double(V1.at<uchar>(i, j)) / A_64f));
		}

#if 0
	imshow("m64", m_64f);
	imshow("m", m);
	waitKey(0);
	exit(0);
#endif 

	return 0;
}
