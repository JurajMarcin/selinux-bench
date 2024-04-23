#include <bits/time.h>
#include <errno.h>
#include <error.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

#include <selinux/selinux.h>

#include "common.h"

struct bench_data {
	char *line;
	const char *scon;
	const char *tcon;
	security_class_t tclass;
	const char *objname;
	const char *result;
	unsigned long long time;
};

int bench_data_parse(char *line, struct bench_data *data)
{
	const char *scon = line;
	char *tmp = strchr(scon, ' ');
	if (tmp == NULL) {
		error(0, 0, "Failed to find tcon in '%s'", scon);
		return 1;
	}
	*tmp = '\0';
	const char *tcon = tmp + 1;
	
	tmp = strchr(tcon, ' ');
	if (tmp == NULL) {
		error(0, 0, "Failed to find class in '%s'", tcon);
		return 1;
	}
	*tmp = '\0';
	const char *cls = tmp + 1;
	
	tmp = strchr(cls, ' ');
	if (tmp == NULL) {
		error(0, 0, "Failed to find objname in '%s'", cls);
		return 1;
	}
	*tmp = '\0';
	const char *objname = tmp + 2;
	
	tmp = strchr(objname, '"');
	if (tmp == NULL) {
		error(0, 0, "Failed to find default type in '%s'", objname);
		return 1;
	}
	*tmp = '\0';
	const char *res = tmp + 2;
	
	tmp = strchr(res, '\n');
	if (tmp != NULL)
		*tmp = '\0';

	security_class_t tclass = string_to_security_class(cls);
	if (tclass == 0) {
		error(0, errno, "Failed to get security class");
		return 1;
	}

	data->line = line;
	data->scon = scon;
	data->tcon = tcon;
	data->tclass = tclass;
	data->objname = objname;
	data->result = res;

	return 0;
}

void bench_data_free(size_t ndata, struct bench_data data[ndata])
{
	for (size_t i = 0; i < ndata; i++)
		free(data[i].line);
	free(data);
}

int bench_data_read(FILE *data_file, struct bench_data **out_data, size_t *ndata)
{
	struct bench_data *data = NULL;
	size_t allocated = 0;
	size_t loaded = 0;

	char *line = NULL;
	size_t n = 0;
	ssize_t bytes_read;
	while ((bytes_read = getline(&line, &n, data_file)) > 0) {
		if (loaded == allocated) {
			allocated += 4096;
			struct bench_data *tmp = realloc(data,
					allocated * sizeof(struct bench_data));
			if (tmp == NULL) {
				error(0, errno, "Failed to realloc data");
				goto err;
			}
			data = tmp;
		}
		if (bench_data_parse(line, &data[loaded]) != 0)
			goto err;

		loaded++;
		line = NULL;
		n = 0;
	}
	free(line);

	*out_data = data;
	*ndata = loaded;
	return 0;

err:
	bench_data_free(loaded, data);
	free(line);
	return 1;
}

int benchmark(size_t ndata, struct bench_data data[ndata], const char *result_format)
{
	struct timespec before;
	struct timespec after;

	struct bench_clock iterative = {0};
	for (size_t i = 0; i < ndata; i++) {
		char *newcon = NULL;
		clock_gettime(CLOCK_MONOTONIC, &before);
		int status = security_compute_create_name(data->scon, data->tcon, data->tclass,
				data->objname, &newcon);
		clock_gettime(CLOCK_MONOTONIC, &after);
		free(newcon);
		if (status) {
			error(0, errno, "Failed to get new context");
			return 1;
		}
		data[i].time = bench_clock_add(&before, &after, &iterative);
	}
	if (result_format != NULL)
		bench_block_printf(&iterative, result_format);
	else
		bench_clock_print(&iterative);
	return 0;
}

int main(int argc, char *argv[])
{
	if (setlocale(LC_NUMERIC, "") == NULL)
		error(0, errno, "Cannot set locale");

	int argi = 1;

	const char *result_format = NULL;
	if (argc > argi && strcmp(argv[argi], "-f") == 0) {
		result_format = argv[argi + 1];
		argi += 2;
	}

	const char *data_path = NULL;
	if (argc > argi) {
		data_path = argv[argi];
		argi++;
	}

	int status = EXIT_FAILURE;

	FILE *data_file = stdin;
	if (data_path != NULL) {
		data_file = fopen(data_path, "r");
		if (data_file == NULL) {
			error(0, errno, "Failed to open data file %s", data_path);
			goto err_fopen;
		}
	}

	struct bench_data *data;
	size_t ndata;
	if (bench_data_read(data_file, &data, &ndata) != 0) {
		goto err_read;
	}

	if (benchmark(ndata, data, result_format) != 0)
		goto err_bench;

	if (argc > argi) {
		FILE *log_file = fopen(argv[argi], "w");
		if (log_file == NULL) {
			error(0, errno, "Failed to open log file %s", argv[argi]);
			goto err_bench;
		}
		fprintf(log_file, "Time\n");
		for (size_t i = 0; i < ndata; i++)
			fprintf(log_file, "%llu\n", data[i].time);
		if (fclose(log_file)) {
			error(0, errno, "Failed to close log file %s", argv[argi]);
			goto err_bench;
		}
	}
	status = EXIT_SUCCESS;

err_bench:
	bench_data_free(ndata, data);
err_read:
	if (data_path != NULL && fclose(data_file))
		error(0, errno, "Failed to close policy file %s", data_path);
err_fopen:
	return status;
}
