/******************************************************************************
 * Copyright (C) 2022 by Abd-Alrhman Masalkhi
 *
 * This file is part of data-set.
 *
 *  data-set is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  data-set is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with data-set. If not, see <https://www.gnu.org/licenses/>. 
 *
 *****************************************************************************/

#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

#include "data-set.h"


#define MAP_PROT  (PROT_READ | PROT_WRITE)
#define MAP_FLAGS (MAP_PRIVATE)

#define eprintf(...) fprintf(stderr, __VA_ARGS__)


struct mem_map * create_mem_map(char *filename, size_t struct_size,
				int parser(const char *, void *, void *),
				void *priv_data)
{
	int fd, err, i;
	char *saveptr = NULL;     /* it must be null for strtok_r */
	char *row, *line;
	unsigned char *ptr;
	struct stat info;
	struct mem_map *mem;
	size_t maped_size, file_size, line_size, items;

	if (!filename || !struct_size || !parser)
		return NULL;
	
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror("error");
	        return NULL;
	}

	err = fstat(fd, &info);
	if (err) {
		perror("error");
		goto close_file;
	}
	file_size = info.st_size;

        row = mmap(NULL, file_size, MAP_PROT, MAP_FLAGS, fd, 0);
	if (row == MAP_FAILED) {
		perror("error");
		goto close_file;
	}

        line = strtok_r(row, "\n", &saveptr);
	if(!line) {
		eprintf("error: %s file is empty\n", filename);
		goto unmap_file;
	}

	/* to know how many itmes should be in the data set */
	line_size = strlen(line) + 1; /* +1 for the newline char */
	items = file_size / line_size;
	if (line_size * items != file_size) {
		eprintf("error: %s is corrupted\n", filename);
		goto unmap_file;
	}
	
	maped_size = (items * struct_size) + sizeof(struct mem_map);
	
	mem = mmap(NULL, maped_size, MAP_PROT, MAP_FLAGS | MAP_ANON, -1, 0);
	if (mem == MAP_FAILED) {
		perror("error");
		goto unmap_file;
	}

	mem->items = items;
	mem->maped_size = maped_size;

	ptr = mem->data;
	for (i = 0; line && i < items; i++) {
		err = parser(line, ptr, priv_data);
		if (err)
			goto unmap_data;
		
	        line = strtok_r(NULL, "\n", &saveptr);
		ptr += struct_size;
	}
	
	if (line || i != items) {
		eprintf("error: %s is corrupted\n", filename);
		goto unmap_data;
	}
	
	munmap(row, file_size);
	close(fd);
	
	return mem;

unmap_data:
	munmap(mem, maped_size);
unmap_file:
	munmap(row, file_size);
close_file:
	close(fd);
	
	return NULL;
}
