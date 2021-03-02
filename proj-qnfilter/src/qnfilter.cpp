#include <opencv2/highgui/highgui.hpp>    
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#include "qnfilter.h"

using namespace cv;
using namespace std;

#define LMin(x, y) ((x)<(y) ? (x) : (y))
#define LMin_xyz(x, y, z) LMin((x), LMin((y), (z)))

CQNFilter::CQNFilter(QNFILTER_TYPE eType)
{
	_eType = eType;

	// lowlight
	_nLowLight_r = 3;
	_dLowLight_eps = 0.001;
	_dLowLight_w = 0.5;
	_dLowLight_maxV1 = 0.7;

	// dehaze
	_nDehaze_r = 3;
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

/*****************
* https://blog.csdn.net/wangyaninglm/article/details/44838545
http://research.microsoft.com/en-us/um/people/kahe/eccv10/
推酷上的一篇文章：
http://www.tuicool.com/articles/Mv2iiu
************************/
void CQNFilter::guidedFilter2(Mat &out, cv::Mat I, cv::Mat p, int r, double eps)
{
	/*
	% GUIDEDFILTER   O(1) time implementation of guided filter.
	%
	%   - guidance image: I (should be a gray-scale/single channel image)
	%   - filtering input image: p (should be a gray-scale/single channel image)
	%   - local window radius: r
	%   - regularization parameter: eps
	*/

#if 0
	cv::Mat _I;
	I.convertTo(_I, CV_64FC1);
	I = _I;

	cv::Mat _p;
	p.convertTo(_p, CV_64FC1);
	p = _p;
#endif 

	//[hei, wid] = size(I);
	int hei = I.rows;
	int wid = I.cols;

	//N = boxfilter(ones(hei, wid), r); % the size of each local patch; N=(2r+1)^2 except for boundary pixels.
	cv::Mat N;
	cv::boxFilter(cv::Mat::ones(hei, wid, I.type()), N, CV_64FC1, cv::Size(r, r));

	//mean_I = boxfilter(I, r) ./ N;
	cv::Mat mean_I;
	cv::boxFilter(I, mean_I, CV_64FC1, cv::Size(r, r));

	//mean_p = boxfilter(p, r) ./ N;
	cv::Mat mean_p;
	cv::boxFilter(p, mean_p, CV_64FC1, cv::Size(r, r));

	//mean_Ip = boxfilter(I.*p, r) ./ N;
	cv::Mat mean_Ip;
	cv::boxFilter(I.mul(p), mean_Ip, CV_64FC1, cv::Size(r, r));

	//cov_Ip = mean_Ip - mean_I .* mean_p; % this is the covariance of (I, p) in each local patch.
	cv::Mat cov_Ip = mean_Ip - mean_I.mul(mean_p);

	//mean_II = boxfilter(I.*I, r) ./ N;
	cv::Mat mean_II;
	cv::boxFilter(I.mul(I), mean_II, CV_64FC1, cv::Size(r, r));

	//var_I = mean_II - mean_I .* mean_I;
	cv::Mat var_I = mean_II - mean_I.mul(mean_I);

	//a = cov_Ip ./ (var_I + eps); % Eqn. (5) in the paper;	
	cv::Mat a = cov_Ip / (var_I + eps);

	//b = mean_p - a .* mean_I; % Eqn. (6) in the paper;
	cv::Mat b = mean_p - a.mul(mean_I);

	//mean_a = boxfilter(a, r) ./ N;
	cv::Mat mean_a;
	cv::boxFilter(a, mean_a, CV_64FC1, cv::Size(r, r));
	mean_a = mean_a / N;

	//mean_b = boxfilter(b, r) ./ N;
	cv::Mat mean_b;
	cv::boxFilter(b, mean_b, CV_64FC1, cv::Size(r, r));
	mean_b = mean_b / N;

	//q = mean_a .* I + mean_b; % Eqn. (8) in the paper;
	//cv::Mat q = mean_a.mul(I) + mean_b;
	out = mean_a.mul(I) + mean_b;
}


void CQNFilter::dehaze_getV1(int& A, Mat& V1_64f, Mat&m, int r, float eps, float w, float maxV1)
{
	// get dark-channel
	Mat imgDark = Mat(V1_64f.size(), CV_8UC1);
	for ( int i=0; i<m.rows; i++ )
		for (int j = 0; j < m.cols; j++)
			imgDark.at<uchar>(i, j) = LMin_xyz(m.at<Vec3b>(i, j)[0], m.at<Vec3b>(i, j)[1], m.at<Vec3b>(i, j)[2]);
	
	Mat I_64f, p_64f;
	imgDark.convertTo(I_64f, CV_64FC1, 1.0 / 255.0);

	Mat element = getStructuringElement(MORPH_RECT, Size(r * 2 + 1, r * 2 + 1), Point(r, r));
	erode(I_64f, p_64f, element);

	guidedFilter2(V1_64f, I_64f, p_64f, r, eps);

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

	for (int i = 0; i < m.rows; i++)
		for (int j = 0; j < m.cols; j++) {
			V1_64f.at<double>(i, j) = LMin(maxV1, V1_64f.at<double>(i, j)*w);
		}
}

int CQNFilter::Filter_dehaze(Mat& m)
{
	int A = 0;
	Mat V1_64f = Mat(m.size(), CV_64FC1);
	dehaze_getV1(A, V1_64f, m, _nLowLight_r, _dLowLight_eps, _dLowLight_w, _dLowLight_maxV1);

	Mat m_64f;
	m.convertTo(m_64f, CV_64FC3, 1.0 / 255.0);

	double A_64f = double(A) / 255.0;
	for (int i = 0; i < m.rows; i++)
		for (int j = 0; j < m.cols; j++) {
			double b64, g64, r64;
			b64 = ((m_64f.at<Vec3d>(i, j)[0] - V1_64f.at<double>(i, j)) / (1.0 - V1_64f.at<double>(i, j) / A_64f));
			g64 = ((m_64f.at<Vec3d>(i, j)[1] - V1_64f.at<double>(i, j)) / (1.0 - V1_64f.at<double>(i, j) / A_64f));
			r64 = ((m_64f.at<Vec3d>(i, j)[2] - V1_64f.at<double>(i, j)) / (1.0 - V1_64f.at<double>(i, j) / A_64f));
			m.at<Vec3b>(i, j)[0] = 255 * b64;
			m.at<Vec3b>(i, j)[1] = 255 * g64;
			m.at<Vec3b>(i, j)[2] = 255 * r64;
		}

//	imshow("m64", m_64f);
//	imshow("m", m);
//	waitKey(0);

	return 0;
}
