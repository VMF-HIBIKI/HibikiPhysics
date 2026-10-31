#include <cstdlib>

int main(int inArgc, char **)
{
	auto		*values = new int[4]{};
	volatile int index = inArgc + 3;
	values[index] = 42;
	const int result = values[0];
	delete[] values;
	return result;
}
