
#include "test2.h"


#ifdef _WIN32
#include <opencv2/highgui/highgui.hpp>    
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

#include "../include/qndeblock-sdk.h"

using namespace std;
using namespace cv;


void test2()
{
	const char* szModelUrl = "d:/nir10_best.onnx";
	QNDeblockHandle handle;
	QNDeblockFilter_Create(&handle, 720, 1280, (char *)szModelUrl);
	Mat merge;

	VideoCapture cap("d:/workroom/testroom/48.mp4");
	while (1) {
		Mat frame;
		cap >> frame;
		if (frame.empty())
			break;
		if (merge.empty())
			merge = Mat(frame.size().height, frame.size().width * 2, frame.type());

		Mat img420;
		cvtColor(frame, img420, COLOR_BGR2YUV_I420);
		QNDeblockFilter_Process_I420(handle, img420.data);

		Mat out = Mat(frame.rows + frame.rows / 2, frame.cols, CV_8UC1, img420.data);
		cvtColor(out, out, COLOR_YUV2BGR_I420);
		//imshow("1", out);
		//imshow("2", frame);
		//waitKey(1);
		
		//merge(Rect(0, 0, 720, 1280)) = frame.clone();
		//merge(Rect(720, 0, 720, 1280)) = out.clone();
		frame.copyTo(merge(Rect(0, 0, 720, 1280)));
		out.copyTo(merge(Rect(720, 0, 720, 1280)));
		imshow("1", merge);
		waitKey(0);

		cout << cap.get(CAP_PROP_POS_MSEC) << endl;
	}


	QNDeblockFilter_Destroy(handle);
	cout << "test2 done..\n";
	exit(0);
}
#else
void test2()
{

}
#endif

