
#include"IOmode.h"

int main(int argc,char **argv)
{
	IOmode iom(12345, "192.168.1.103");
	iom.epollmode();

	return 0;
}
