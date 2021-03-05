#include <thread>
#include <iostream>

#include "utils-mt.h"

using namespace std;

#define LMin(x, y) ((x)<(y) ? (x) : (y))
#define LMin_xyz(x, y, z) LMin((x), LMin((y), (z)))
#define LMax(x, y) ((x)>(y) ? (x) : (y))
#define LClip(a, lo, hi) ((a)<lo ? lo : ((a)>hi ? hi : (a)) )

static void thread_mat_mul(Mat *d, Mat* s0, Mat* s1)
{
	*d = (*s0).mul(*s1);
}

static void thread_boxfilter(Mat* pSrc, Mat* pDst, int ddepth, cv::Size size)
{
	boxFilter(*pSrc, *pDst, ddepth, size);
}

static void thread_cov_Ip(Mat *pDst, Mat *mean_Ip, Mat *mean_I, Mat *mean_p)
{
	*pDst = (*mean_I).mul(*mean_p);
	*pDst = *mean_Ip - *pDst;
}

static void thread_convertTo(Mat* pDst, Mat* pSrc, int ddepth)
{
	(*pSrc).convertTo(*pDst, ddepth);
}

void CQNFilter::guidedFilter_mt(Mat& out, cv::Mat I, cv::Mat p, int r, double eps)
{
	// I and P is CV_8UC1
	//Mat I_32;
	//I.convertTo(I_32, CV_32SC1);
	thread t_cvt(thread_convertTo, &_I_32, &I, CV_32SC1);

	MAT_INIT(_II, I, CV_8UC1);
	thread t_II(thread_mat_mul, &_II, &I, &I);

	//Mat mean_I;
	//boxFilter(I, mean_I, CV_32SC1, Size(r, r));
	thread t0(thread_boxfilter, &I, &_mean_I, CV_32SC1, Size(r, r));
	
	//Mat mean_p;
	//boxFilter(p, mean_p, CV_32SC1, Size(r, r));
	thread t1(thread_boxfilter, &p, &_mean_p, CV_32SC1, Size(r, r));

	//Mat mean_Ip;
	//boxFilter(I.mul(p), mean_Ip, CV_32SC1, Size(r, r));
	//_mul_Ip = I.mul(p);
	MAT_INIT(_mul_Ip, I, CV_8UC1);
	_mul_Ip = I.mul(p);
	thread t2(thread_boxfilter, &_mul_Ip, &_mean_Ip, CV_32SC1, Size(r, r));

	t0.join(); // _mean_I
	t1.join(); // _mean_p
	t2.join(); // _mean_Ip

	MAT_INIT(_cov_Ip, I, CV_32SC1);
	thread t3(thread_cov_Ip, &_cov_Ip, &_mean_Ip, &_mean_I, &_mean_p);

	MAT_INIT(_meanImeanI, I, CV_32SC1);
	thread t_meanImeanI(thread_mat_mul, &_meanImeanI, &_mean_I, &_mean_I);
	
	t_II.join(); // _II
	MAT_INIT(_mean_II, I, CV_32SC1);
	boxFilter(_II, _mean_II, CV_32SC1, Size(r, r));
	t_meanImeanI.join();
	_var_I = _mean_II - _meanImeanI;

	t3.join(); // _cov_Ip
	_gf_a = _cov_Ip / (_var_I + 1);

	//Mat mean_a;
	//boxFilter(a, mean_a, CV_32SC1, Size(r, r));
	thread t5(thread_boxfilter, &_gf_a, &_mean_a, CV_32SC1, Size(r, r));

	MAT_INIT(_AmeanI, I, CV_32SC1);
	_AmeanI = _gf_a.mul(_mean_I);
	_gf_b = _mean_p - _AmeanI;
	//Mat mean_b;
	//boxFilter(b, mean_b, CV_32SC1, Size(r, r));
	thread t6(thread_boxfilter, &_gf_b, &_mean_b, CV_32SC1, Size(r, r));

	t_cvt.join(); // _I_32
	t5.join(); // _mean_a
	t6.join(); // _mean_b

	//Mat out_32;
	MAT_INIT(_ImeanA, I, CV_32SC1);
	_ImeanA = _mean_a.mul(_I_32);
	_gf_out_32 = _ImeanA + _mean_b;
	_gf_out_32.convertTo(out, CV_8UC1);
}

static void image_convert_slice(Mat* img, int start_rows, int end_rows)
{
	for (int i = start_rows; i < end_rows; i++)
		for (int j = 0; j < img->cols; j++)
		{
			unsigned char* p = (uchar*)(&img->at<Vec3b>(i, j));
			p[0] = 255 - p[0];
			p[1] = 255 - p[1];
			p[2] = 255 - p[2];
//			img->at<Vec3b>(i, j)[0] = 255 - img->at<Vec3b>(i, j)[0];
//			img->at<Vec3b>(i, j)[1] = 255 - img->at<Vec3b>(i, j)[1];
//			img->at<Vec3b>(i, j)[2] = 255 - img->at<Vec3b>(i, j)[2];
		}
}

void CQNFilter::image_convert_mt(Mat& m)
{
	const int thd_num = 6;
	thread *t[thd_num];
	for (int i = 0; i < thd_num; i++) {
		t[i] = new thread(image_convert_slice, &m, i * m.rows / thd_num, (i + 1) * m.rows / thd_num);
	}
	for (int i = 0; i < thd_num; i++)
		t[i]->join();
	for (int i = 0; i < thd_num; i++)
		delete t[i];
}

static void get_darkchannel_slice(Mat* img, Mat* imgDark, int start_rows, int end_rows)
{
	for (int i = start_rows; i < end_rows; i++)
		for (int j = 0; j < img->cols; j++)
		{
			unsigned char* p = (uchar*)(&img->at<Vec3b>(i, j));
			imgDark->at<uchar>(i, j) = LMin_xyz(p[0], p[1], p[2]);
		}
}

void CQNFilter::get_darkchannel_mt(Mat& m, Mat& imgDark)
{
	const int thd_num = 6;
	thread* t[thd_num];
	for (int i = 0; i < thd_num; i++) {
		t[i] = new thread(get_darkchannel_slice, &m, &imgDark, i * m.rows / thd_num, (i + 1) * m.rows / thd_num);
	}
	for (int i = 0; i < thd_num; i++)
		t[i]->join();
	for (int i = 0; i < thd_num; i++)
		delete t[i];
}

static void stretch_image_slice(Mat* m, Mat* V1, int A, int start_rows, int end_rows)
{
	double A_64f = double(A) / 1.0;
	for (int i = start_rows; i < end_rows; i++)
	{
		unsigned char* p = (uchar*)(&m->at<Vec3b>(i, 0));
		unsigned char *pv = &V1->at<uchar>(i, 0);
		for (int j = 0; j < m->cols; j++) {
			unsigned char v = *pv++;
			double f = (1.0 - double(v) / A_64f);
#if 0
			double b = double(p[0] - v) / f;
			double g = double(p[1] - v) / f;
			double r = double(p[2] - v) / f;
			p[0] = LClip(b, 0, 255);
			p[1] = LClip(g, 0, 255);
			p[2] = LClip(r, 0, 255);
#endif
			p[0] = LMin(double(LMax(p[0] - v, 0)) / f, 255);
			p[1] = LMin(double(LMax(p[1] - v, 0)) / f, 255);
			p[2] = LMin(double(LMax(p[2] - v, 0)) / f, 255);
			p += 3;
		}
	}
}

void CQNFilter::stretch_image_mt(Mat& m, Mat& V1, int A)
{
	const int thd_num = 8;
	thread* t[thd_num];
	for (int i = 0; i < thd_num; i++) {
		t[i] = new thread(stretch_image_slice, &m, &V1, A, i * m.rows / thd_num, (i + 1) * m.rows / thd_num);
	}
	for (int i = 0; i < thd_num; i++)
		t[i]->join();
	for (int i = 0; i < thd_num; i++)
		delete t[i];
}
