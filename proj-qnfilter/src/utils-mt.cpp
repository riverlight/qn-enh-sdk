#include <thread>

#include "utils-mt.h"

using namespace std;


void thread_boxfilter(Mat* pSrc, Mat* pDst, int ddepth, cv::Size size)
{
	boxFilter(*pSrc, *pDst, ddepth, size);
}

void thread_cov_Ip(Mat *pDst, Mat *mean_Ip, Mat *mean_I, Mat *mean_p)
{
	*pDst = *mean_Ip - (*mean_I).mul(*mean_p);
}

void thread_var_I(Mat *pDst, Mat *I, Mat *mean_I, int ddepth, Size size)
{
	Mat mean_II;
	boxFilter((*I).mul(*I), mean_II, ddepth, size);
	*pDst = mean_II - (*mean_I).mul(*mean_I);
}

void thread_convertTo(Mat* pDst, Mat* pSrc, int ddepth)
{
	(*pSrc).convertTo(*pDst, ddepth);
}

void guidedFilter_mt(Mat& out, cv::Mat I, cv::Mat p, int r, double eps)
{
	// I and P is CV_8UC1
	Mat I_32;
	//I.convertTo(I_32, CV_32SC1);
	thread t_cvt(thread_convertTo, &I_32, &I, CV_32SC1);

	Mat mean_I;
	//boxFilter(I, mean_I, CV_32SC1, Size(r, r));
	thread t0(thread_boxfilter, &I, &mean_I, CV_32SC1, Size(r, r));
	
	Mat mean_p;
	//boxFilter(p, mean_p, CV_32SC1, Size(r, r));
	thread t1(thread_boxfilter, &p, &mean_p, CV_32SC1, Size(r, r));

	Mat mean_Ip;
	//boxFilter(I.mul(p), mean_Ip, CV_32SC1, Size(r, r));
	Mat mul_Ip = I.mul(p);
	thread t2(thread_boxfilter, &mul_Ip, &mean_Ip, CV_32SC1, Size(r, r));

	t0.join();
	t1.join();
	t2.join();

	//Mat cov_Ip = mean_Ip - mean_I.mul(mean_p);
	Mat cov_Ip;
	thread t3(thread_cov_Ip, &cov_Ip, &mean_Ip, &mean_I, &mean_p);

	//Mat mean_II;
	//boxFilter(I.mul(I), mean_II, CV_32SC1, Size(r, r));
	//Mat var_I = mean_II - mean_I.mul(mean_I);
	Mat var_I;
	thread t4(thread_var_I, &var_I, &I, &mean_I, CV_32SC1, Size(r, r));
	t3.join();
	t4.join();

	Mat a = cov_Ip / (var_I + 1);

	Mat mean_a;
	//boxFilter(a, mean_a, CV_32SC1, Size(r, r));
	thread t5(thread_boxfilter, &a, &mean_a, CV_32SC1, Size(r, r));

	Mat b = mean_p - a.mul(mean_I);
	Mat mean_b;
	//boxFilter(b, mean_b, CV_32SC1, Size(r, r));
	thread t6(thread_boxfilter, &b, &mean_b, CV_32SC1, Size(r, r));

	t_cvt.join(); // I_32
	t5.join(); // mean_a
	t6.join(); // mean_b

	Mat out_32;
	out_32 = mean_a.mul(I_32) + mean_b;
	out_32.convertTo(out, CV_8UC1);
}
