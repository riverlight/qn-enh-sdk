#ifndef QNDEBLOCK_H
#define QNDEBLOCK_H

#include <string>
#include <NvInfer.h>
#include "NvOnnxParser.h"
//#include "parserOnnxConfig.h"

using namespace std;
using namespace cv;
using namespace nvinfer1;


class TLogger : public ILogger
{
public:
	TLogger() {
		mReportableSeverity = Severity::kWARNING;
	}
	~TLogger() {}

	void log(Severity severity, const char* msg) {}
	Severity getReportableSeverity() const
	{
		return mReportableSeverity;
	}
	
private:
	Severity mReportableSeverity;
};


class CQNDeblock
{
public:
	CQNDeblock();
	~CQNDeblock();

	int Init(int nWidth, int nHeight, char* szModelUrl);
	int Process_I420(unsigned char* pI420);

private:
	
	void MAT_INIT(Mat& m, Mat& ref, int ddepth) { if (m.empty()) m = Mat(ref.size(), ddepth); }
	void enableDLA(IBuilder* builder, IBuilderConfig* config, int useDLACore, bool allowGPUFallback = true);

	void Mat_2_Buffer2(cv::Mat& m, float* buffer);
	void Buffer_2_Mat2(float* buffer, cv::Mat& m);

private:
	TLogger _logger;
	IBuilder* _builder;
	nvinfer1::INetworkDefinition* _network;
	nvinfer1::IBuilderConfig* _config;
	IExecutionContext* _context;
	ICudaEngine* _engine;
	//nvonnxparser::IParser* _parser;
	void* _buffers[2];
	float* _pInput, * _pOutput;
	int _inputIndex, _outputIndex;

	int _batchSize;
	bool _runInFp16;
	int _useDLACore;
	string _strInputName, _strOutputName;

	Mat _tm;
	string _strModelUrl;
	int _nWidth, _nHeight;
};


#endif // QNDEBLOCK_H
