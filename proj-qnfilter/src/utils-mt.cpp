#include <thread>
#include <iostream>

#include "utils-mt.h"

using namespace std;


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
