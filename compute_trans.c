#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>

#include <selinux/selinux.h>

int main(int argc, char *argv[])
{
	if (argc != 5) {
		error(0, 0, "Usage: %s SCON TCON CLASS OBJNAME", argv[0]);
		return 2;
	}
	const char *scon = argv[1];
	const char *tcon = argv[2];
	const char *tclass_name = argv[3];
	security_class_t tclass = string_to_security_class(tclass_name);
	const char *objname = argv[4];

	char *newcon;
	if (security_compute_create_name(scon, tcon, tclass, objname, &newcon)) {
		error(0, errno, "Failed to get new context");
		return EXIT_FAILURE;
	}
	puts(newcon);
	free(newcon);
	return EXIT_SUCCESS;
}
