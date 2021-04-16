#include <thread>
#include <opencv2/highgui/highgui.hpp>    
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <cuda_runtime_api.h>

#include "qndeblock.h"

using namespace cv;
using namespace std;

#define LClip(x, lmin ,lmax) ((x)<lmin ? lmin : ( (x)>lmax ? lmax : (x) ))


CQNDeblock::CQNDeblock()
{
	_batchSize = 1;
	_runInFp16 = true;
	_useDLACore = -1;
	_strInputName = "input11";
	_strOutputName = "output44";
}

CQNDeblock::~CQNDeblock()
{

}

int CQNDeblock::Init(int nWidth, int nHeight, char* szModelUrl)
{
	if (szModelUrl == NULL)
		return -1;

	_strModelUrl = szModelUrl;
	_nWidth = nWidth;
	_nHeight = nHeight;

	_builder = createInferBuilder(_logger);
	if (!_builder)
		return -2;
	_network = _builder->createNetworkV2(_batchSize);
	if (!_network)
		return -3;
	_config = _builder->createBuilderConfig();
	if (!_config)
		return -4;
	auto parser = nvonnxparser::createParser(*_network, _logger);
	if (!parser)
		return -5;
	if (!parser->parseFromFile(szModelUrl, static_cast<int>(_logger.getReportableSeverity())))
		return -6;

	auto profile = _builder->createOptimizationProfile();
	profile->setDimensions("input11", OptProfileSelector::kMIN, Dims4{ 1, 3, 96, 96 });
	profile->setDimensions("input11", OptProfileSelector::kOPT, Dims4{ 1, 3, 1280, 720 });
	profile->setDimensions("input11", OptProfileSelector::kMAX, Dims4{ 1, 3, 1920, 1080 });
	_config->addOptimizationProfile(profile);
	_builder->setMaxBatchSize(_batchSize);
	_builder->setMaxWorkspaceSize(1024 * (1 << 20));
	_config->setMaxWorkspaceSize(1024 * (1 << 20));
	if (_runInFp16)
		_config->setFlag(BuilderFlag::kFP16);
	enableDLA(_builder, _config, _useDLACore);
	_engine = _builder->buildEngineWithConfig(*_network, *_config);
	if (!_engine)
		return -7;
	parser->destroy();

	_context = _engine->createExecutionContext();
	if (_context == nullptr)
	{
		return -8;
	}

	_context->setOptimizationProfile(0);

	Dims4 dims4 = { 1, 3, _nHeight, _nWidth };
	_context->setBindingDimensions(0, dims4);

	_inputIndex = _engine->getBindingIndex(_strInputName.c_str());
	_outputIndex = _engine->getBindingIndex(_strOutputName.c_str());
	int inputSize = _nWidth * _nHeight * 3;
	int outputSize = _nWidth * _nHeight * 3;
	cudaMalloc(&_buffers[_inputIndex], inputSize * sizeof(float));
	cudaMalloc(&_buffers[_outputIndex], outputSize * sizeof(float));

	_pInput = new float[inputSize];
	_pOutput = new float[outputSize];

	return 0;
}

int CQNDeblock::Process_I420(unsigned char* pI420)
{
	int inputSize = _nWidth * _nHeight * 3;
	int outputSize = _nWidth * _nHeight * 3;

	Mat m = Mat(_nHeight + _nHeight / 2, _nWidth, CV_8UC1, pI420);
	MAT_INIT(_tm, m, CV_8UC3);
	cvtColor(m, _tm, COLOR_YUV2BGR_I420);
	//imshow("2", _tm);
	//waitKey(0);

	Mat_2_Buffer2(_tm, _pInput);
	cudaMemcpy(_buffers[_inputIndex], _pInput, inputSize * sizeof(float), cudaMemcpyHostToDevice);
	_context->executeV2(_buffers);
	cudaMemcpy(_pOutput, _buffers[_outputIndex], outputSize * sizeof(float), cudaMemcpyDeviceToHost);
	Buffer_2_Mat2(_pOutput, _tm);

	//imshow("1", _tm);
	//waitKey(1);

	cvtColor(_tm, m, COLOR_BGR2YUV_I420);

	return 0;
}

void CQNDeblock::enableDLA(IBuilder* builder, IBuilderConfig* config, int useDLACore, bool allowGPUFallback)
{
	if (useDLACore >= 0)
	{
		if (builder->getNbDLACores() == 0)
		{
			std::cerr << "Trying to use DLA core " << useDLACore << " on a platform that doesn't have any DLA cores"
				<< std::endl;
			assert("Error: use DLA core on a platfrom that doesn't have any DLA cores" && false);
		}
		if (allowGPUFallback)
		{
			config->setFlag(BuilderFlag::kGPU_FALLBACK);
		}
		if (!builder->getInt8Mode() && !config->getFlag(BuilderFlag::kINT8))
		{
			// User has not requested INT8 Mode.
			// By default run in FP16 mode. FP32 mode is not permitted.
			builder->setFp16Mode(true);
			config->setFlag(BuilderFlag::kFP16);
		}
		config->setDefaultDeviceType(DeviceType::kDLA);
		config->setDLACore(useDLACore);
		config->setFlag(BuilderFlag::kSTRICT_TYPES);
	}
}


void CQNDeblock::Mat_2_Buffer2(cv::Mat& m, float* buffer)
{
	int width = m.cols;
	int height = m.rows;
	float* r = buffer + width * height * 0;
	float* g = buffer + width * height * 1;
	float* b = buffer + width * height * 2;

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			unsigned char* s = (uchar*)&m.at<cv::Vec3b>(i, j)[0];
			int offset = i * width + j;
			b[offset] = float(s[0]) / 255.0;
			g[offset] = float(s[1]) / 255.0;
			r[offset] = float(s[2]) / 255.0;
		}
	}
}

void CQNDeblock::Buffer_2_Mat2(float* buffer, cv::Mat& m)
{
	int width = m.cols;
	int height = m.rows;
	float* r = buffer + width * height * 0;
	float* g = buffer + width * height * 1;
	float* b = buffer + width * height * 2;
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			unsigned char* d = (uchar*)&m.at<cv::Vec3b>(i, j)[0];
			int offset = i * width + j;
			d[2] = LClip(r[offset] * 255, 0, 255);
			d[1] = LClip(g[offset] * 255, 0, 255);
			d[0] = LClip(b[offset] * 255, 0, 255);
		}
	}
}

