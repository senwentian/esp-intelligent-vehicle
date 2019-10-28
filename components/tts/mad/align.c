#include "align.h"
#include "esp_attr.h"

#define IRAM_ATTR_FUN IRAM_ATTR

char IRAM_ATTR_FUN unalChar(const char *adr) {
	int *p=(int *)((int)adr&0xfffffffc);
	int v=*p;
	int w=((int)adr&3);
	if (w==0) {
		return ((v>>0)&0xff);
	} else if (w==1) {
		return ((v>>8)&0xff);
	} else if (w==2) {
		return ((v>>16)&0xff);
	} else if (w==3) {
		return ((v>>24)&0xff);
	} else {
		return 0xff;
	}
}


short IRAM_ATTR_FUN unalShort(const short *adr) {
	int *p=(int *)((int)adr&0xfffffffc);
	int v=*p;
	int w=((int)adr&3);
	if (w==0) return (v&0xffff); else return (v>>16);
}
