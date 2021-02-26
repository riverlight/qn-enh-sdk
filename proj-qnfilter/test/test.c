#include <stdio.h>

#include "../include/qnfilter-sdk.h"


int main(int argc, char* argv[])
{
	printf("Hi, this is qnfilter-sdk test program!\n");

	Handle handle;
	QNFILTER_TYPE type = QF_LOWLIGHT;

	QNFilter_Create(&handle, type);

	char* a = "haha";
	if (a == "haha") {
		printf("y\n");
	}
	else
		printf("n\n");

	return 0;
}
