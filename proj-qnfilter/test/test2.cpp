
#include "test2.h"


#ifdef _WIN32
#include <opencv2/highgui/highgui.hpp>    
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#include "../include/qnfilter-sdk.h"

using namespace std;
using namespace cv;


void test2()
{
	Handle handle;
	QNFILTER_TYPE type = QF_LOWLIGHT;
	QNFilter_Create(&handle, type);
	QNFilterSetting setting = {
		.dW = 0.6,
	};
	QNFilter_Setting(handle, &setting);

	VideoCapture cap("d:/workroom/testroom/48.mp4");
	while (1) {
		Mat frame;
		cap >> frame;
		if (frame.empty())
			break;

		Mat img420;
		cvtColor(frame, img420, COLOR_BGR2YUV_I420);
		QNFilter_Process_I420(handle, img420.data, frame.cols, frame.rows);
		
		Mat out = Mat(frame.rows + frame.rows / 2, frame.cols, CV_8UC1, img420.data);
		cvtColor(out, out, COLOR_YUV2BGR_I420);
		imshow("1", out);
		waitKey(1);
		cout << cap.get(CAP_PROP_POS_MSEC) << endl;
	}


	QNFilter_Destroy(handle);
	cout << "test2 done..\n";
	exit(0);
}
#else
void test2()
{

}
#endif

