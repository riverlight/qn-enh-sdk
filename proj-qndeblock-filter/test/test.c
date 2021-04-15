#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../include/qndeblock-sdk.h"
#include "test2.h"

int main(int argc, char* argv[])
{
	printf("Hi, this is a qndeblock sdk test program!\n");
	//test2();
	//return 0;

	char* szModelUrl = "d:/nir10_best.onnx";
	QNDeblockHandle handle;
	int ret;
	ret = QNDeblockFilter_Create(&handle, 720, 1280, szModelUrl);
	if (ret != 0)
		printf("ret : %d\n", ret);

	FILE* pf = fopen("d:/workroom/testroom/48.y4m", "rb");
	unsigned char* pI420 = (unsigned char*)malloc(1280 * 720 * 3 / 2);
	if (pf == NULL || pI420 == NULL) {
		printf("init fail..\n");
		return -1;
	}
	fread(pI420, 1, 63, pf);
	int count = 0;
	clock_t t1 = clock();
	while (1) {
		fread(pI420, 1, 6, pf);
		fread(pI420, 1, 1280 * 720 * 3 / 2, pf);
		QNDeblockFilter_Process_I420(handle, pI420);
		count++;
		printf("%d\n", count);
		if (count == 100)
			break;
	}
	clock_t t2 = clock();
	printf("time : %ld\n", t2 - t1);
	fclose(pf);

	QNDeblockFilter_Destroy(handle);

	return 0;
}
