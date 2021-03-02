#include <stdio.h>
#include <stdlib.h>

#include "../include/qnfilter-sdk.h"


int main(int argc, char* argv[])
{
	printf("Hi, this is qnfilter-sdk test program!\n");

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
	while (1) {
		fread(pI420, 1, 6, pf);
		fread(pI420, 1, 1280 * 720 * 3 / 2, pf);
		QNFilter_Process_I420(handle, pI420, 720, 1280);
		count++;
		if (count == 10)
			break;
	}
	fclose(pf);
	
	QNFilter_Destroy(handle);

	return 0;
}
