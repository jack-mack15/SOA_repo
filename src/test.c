#include <stdio.h>
#include <linux/nospec.h>




int main(int argc, char const *argv[])
{

	int mask = 461;

	int safe = array_index_nospec(0, 461);

	printf("con -3: %d\n", array_index_nospec(-3, 461));
	printf("con 1: %d\n", array_index_nospec(1, 461));
	printf("con 0: %d\n", array_index_nospec(0, 461));
	printf("con 300: %d\n", array_index_nospec(300, 461));
	printf("con 461: %d\n", array_index_nospec(461, 461));
	printf("462: %d\n", array_index_nospec(462, 461));

	return 0;
}