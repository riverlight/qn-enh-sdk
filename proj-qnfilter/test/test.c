#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../include/qnfilter-sdk.h"
#include "test2.h"

int main(int argc, char* argv[])
{
	printf("Hi, this is qnfilter-sdk test program!\n");
	//test2();

	Handle handle;
	QNFILTER_TYPE type = QF_LOWLIGHT;

	QNFilter_Create(&handle, type);

	FILE* pf = fopen("d:/workroom/testroom/48.y4m", "rb");
	unsigned char* pI420 = (unsigned char*)malloc(1280 * 720 * 3 / 2 );
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
		QNFilter_Process_I420(handle, pI420, 720, 1280);
		count++;
		if (count == 100)
			break;
	}
	clock_t t2 = clock();
	printf("time : %lld\n", t2 - t1);
	fclose(pf);
	
	QNFilter_Destroy(handle);

	return 0;
}
