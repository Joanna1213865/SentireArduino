#ifndef _INTEL_EDISON_BT_SPP_H_
#define _INTEL_EDISON_BT_SPP_H_

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#define MAX_BUF 4096

class Intel_Edison_BT_SPP {
	public:
	int open();
	int read();
	void write(const char * output);
	const char * getBuf();
	~Intel_Edison_BT_SPP();

	private:
	int _fd1 = -1;
	int _fd2 = -1;
	const char * _pipeName1 = "/tmp/arduino_pipe_out";
	const char * _pipeName2 = "/tmp/android_pipe_out";
	char _buf[MAX_BUF];
};

#endif
