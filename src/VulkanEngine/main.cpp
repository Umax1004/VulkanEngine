#include "Renderer.h"
#include <conio.h>

int main()
{
	Renderer r;
	r.OpenWindow(800, 600, "Test");
	while (r.Run())
	{

	}
	return 0;
}