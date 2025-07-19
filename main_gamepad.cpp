#include "tools.h"

#include <fcntl.h> // open
#include <unistd.h> // read
#include <linux/input.h> // input_event
#include <linux/ioctl.h> // ioctl

// Docs:
// https://www.kernel.org/doc/Documentation/input/input.txt

const char *selectFirstDevicePath(char path[MAX_PATH_LENGTH])
{
	char name[256];
	const char *basedir = "/dev/input";

	const char *devicePath = nullptr;

	Dir dir = {};

	if ( OpenDir(dir, basedir) )
	{
		DirEntry entry = {};

		while ( !devicePath && ReadDir(dir, entry) )
		{
			SPrintf(path, "%s/%s", basedir, entry.name);

			if (StrEqN(entry.name, "event", 5))
			{
				int fd = open(path, O_RDONLY);
				if (fd < 0) {
					continue;
				}

				if ( ioctl(fd, EVIOCGNAME(sizeof(name)), name) != -1 )
				{
					LOG(Info, "Device info:\n");
					LOG(Info, "- path: %s\n", path);
					LOG(Info, "- name: %s\n", name);

					devicePath = path;
				}

				close(fd);
			}
		}

		CloseDir(dir);
	}

	return devicePath;
}

int main(int argc, char **argv)
{
	char path[MAX_PATH_LENGTH] = {};
	const char *devicePath = selectFirstDevicePath(path);

	if (devicePath == nullptr)
	{
		LOG(Error, "Could not find any gamepad device\n");
		return -1;
	}

	int fd = open(devicePath, O_RDONLY);
	if (fd >= 0)
	{
		LOG(Info, "Reading events from: %s\n", devicePath);
		while (1)
		{
			input_event event;
			ssize_t size = read(fd, &event, sizeof(event));
			if ( size > 0 )
			{
				u32 type = event.type;
				u32 code = event.code;
				i32 value = event.value;
				if (type == EV_SYN) {
					//LOG(Info,"--- EV_SYN ---\n");
				} else if (type == EV_KEY) {
					const char *codeStr = "<unknown>";
					switch (code) {
						case BTN_A: codeStr = "A"; break;
						case BTN_B: codeStr = "B"; break;
						case BTN_X: codeStr = "X"; break;
						case BTN_Y: codeStr = "Y"; break;
						case BTN_TL: codeStr = "LeftShoulder"; break;
						case BTN_TR: codeStr = "RightShoulder"; break;
						case BTN_SELECT: codeStr = "Select"; break;
						case BTN_START: codeStr = "Start"; break;
						case BTN_MODE: codeStr = "Mode"; break;
						case BTN_THUMBL: codeStr = "LeftThumb"; break;
						case BTN_THUMBR: codeStr = "RightThumb"; break;
						default:;
					}
					LOG(Info, "- Button(%u): %s - value:%d\n", code, codeStr, value);
				} else if (type == EV_ABS) {
					const char *codeStr = "<unknown>";
					switch (code) {
						case ABS_X: codeStr = "LeftAxisX"; break;
						case ABS_Y: codeStr = "LeftAxisY"; break;
						case ABS_RX: codeStr = "RightAxisX"; break;
						case ABS_RY: codeStr = "RightAxisY"; break;
						case ABS_Z: codeStr = "LeftTrigger"; break;
						case ABS_RZ: codeStr = "RigtTrigger"; break;
						case ABS_HAT0X: codeStr = "DPadX"; break;
						case ABS_HAT0Y: codeStr = "DPadY"; break;
					}
					LOG(Info, "- Abs(%u): %s - value:%d\n", code, codeStr, value);
				} else if (type == EV_MSC) {
					LOG(Info, "- Msc: code:%u, value:%d\n", code, value);
				} else {
					LOG(Warning, "- Unknown event type\n");
				}
			}
			else if ( size == -1 )
			{
				LOG(Warning, "Could not read from input device: %s\n", devicePath);
			}
		}
	}
	else
	{
		LOG(Error, "Could not open input device: %s\n", devicePath);
		perror("open() failed");
	}
	return 0;
}
