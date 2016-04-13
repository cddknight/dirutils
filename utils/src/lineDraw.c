#include <dircmd.h>

int main (void)
{
	if (strcmp (directoryVersion(), VERSION) != 0)
	{
		fprintf (stderr, "Library (%s) does not match Utility (%s).\n", directoryVersion(), VERSION);
		exit (1);
	}

	displayGetWindowSize ();
	displayLine ();
	return 0;
}

