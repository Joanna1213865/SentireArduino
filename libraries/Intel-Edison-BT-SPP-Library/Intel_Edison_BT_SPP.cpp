#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <Intel_Edison_BT_SPP.h>

using namespace std;

int Intel_Edison_BT_SPP::open() {
	if (_fd1 != -1)
		close(_fd1);

	_fd1 = ::open(_pipeName1, O_RDONLY | O_NONBLOCK);

	if (_fd1 == -1) {
		perror("Cannot open file\n");
		printf("errno %s\n", strerror(errno));
		Serial.print("arduino fifo open error "); Serial.println(strerror(errno));
		return 0;
	}

	if (_fd2 != -1)
		close(_fd2);

	_fd2 = ::open(_pipeName2, O_WRONLY);

	if (_fd2 == -1) {
		perror("Cannot open file\n");
		printf("errno %s\n", strerror(errno));
		Serial.print("android fifo open error "); Serial.println(strerror(errno));
		return 0;
	}

	return -1;
}

ssize_t Intel_Edison_BT_SPP::read() {
	if (_fd1 == -1)
		open();

	int flags = fcntl(_fd1, F_GETFL, 0);
	fcntl(_fd1, F_SETFL, flags | O_NONBLOCK);

	ssize_t size = ::read(_fd1, _buf, MAX_BUF - 1);

	if (size == -1) {
		if (errno != 11) {
			perror("Read error\n");
			printf("errno %s\n", strerror(errno));
			Serial.print("read error "); Serial.println(strerror(errno));
		}
	}
	else {
		_buf[size] = 0;
	}

	return size;
}

// Write to the output FIFO which can be read by
// the paired device
void Intel_Edison_BT_SPP::write(const char * output) {
	if (_fd2 == -1)
		open();

	int flags = fcntl(_fd2, F_GETFL, 0);
	fcntl(_fd2, F_SETFL, flags | O_NONBLOCK);
	Serial.print("To android: ");
	Serial.println(output);
	::write(_fd2, output, 1); // just 1 byte
}

const char * Intel_Edison_BT_SPP::getBuf() {
	return _buf;
}

Intel_Edison_BT_SPP::~Intel_Edison_BT_SPP() {
	if (_fd1 != -1) {
		close(_fd1);
		_fd1 = -1;
	}

	if (_fd2 != -1) {
		close(_fd2);
		_fd2 = -1;
	}
}