#include "tools.h"

f32 myatof(const char *str)
{
	u32 integer = 0;

	// scan integer part
	while (*str >= '0' && *str <= '9') {
		integer = (integer << 3) + (integer << 1); // x10
		integer += *str++ - '0';
	}

	switch (*str++) {
		case '.':
			break;
		case ' ':
		case '\n':
		case '\0':
			return integer;
		default:
			return 0.0f;
	}

	// scan decimal part
	u32 tenPower = 1;
	while (*str >= '0' && *str <= '9') {
		tenPower = (tenPower << 3) + (tenPower << 1);
		integer = (integer << 3) + (integer << 1); // x10
		integer += *str++ - '0';
	}

	switch (*str++) {
		case ' ':
		case 'f':
		case '\n':
		case '\0':
			return integer / (f32)tenPower;
		default:
			return 0.0f;
	}
}

int main()
{
	const char *numbers [] = { "0", "0.0f", "10.54", "10.54f", ".43", ".43f", "5.f" };

	for ( u32 i = 0; i < ARRAY_COUNT(numbers); ++i )
	{
		const char *numberStr = numbers[i];
		LOG(Info, "atof: %f\n", atof(numberStr));
		LOG(Info, "myatof: %f\n", myatof(numberStr));
	}
	return 0;
}

