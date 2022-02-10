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


static int get_error(int err)
{
	switch(err) {
	case EACCES:
		err = ERROR_DATA_SET_ACCES;
		break;
		
	case EDQUOT:
		err = ERROR_DATA_SET_DQUOT;
		break;
		
	case EEXIST:
		err = ERROR_DATA_SET_EXIST;
		break;
		
	case EFAULT:
		err = ERROR_DATA_SET_FAULT;
		break;
		
	case EFBIG:
		err = ERROR_DATA_SET_FBIG;
		break;
		
	case EINTR:
		err = ERROR_DATA_SET_INTR;
		break;
		
	case EINVAL:
		err = ERROR_DATA_SET_INVAL;
		break;
		
	case EISDIR:
		err = ERROR_DATA_SET_ISDIR;
		break;
		
	case ELOOP:
		err = ERROR_DATA_SET_LOOP;
		break;
		
	case EMFILE:
		err = ERROR_DATA_SET_MFILE;
		break;
		
	case ENAMETOOLONG:
		err = ERROR_DATA_SET_NAMETOOLONG;
		break;
		
	case ENFILE:
		err = ERROR_DATA_SET_NFILE;
		break;
		
	case ENODEV:
		err = ERROR_DATA_SET_NODEV;
		break;
		
	case ENOENT:
		err = ERROR_DATA_SET_NOENT;
		break;
		
	case ENOMEM:
		err = ERROR_DATA_SET_NOMEM;
		break;
		
	case ENOSPC:
		err = ERROR_DATA_SET_NOSPC;
		break;
		
	case ENOTDIR:
		err = ERROR_DATA_SET_NOTDIR;
		break;
		
	case ENXIO:
		err = ERROR_DATA_SET_NXIO;
		break;
		
	case EOPNOTSUPP:
		err = ERROR_DATA_SET_OPNOTSUPP;
		break;
		
	case EOVERFLOW:
		err = ERROR_DATA_SET_OVERFLOW;
		break;
		
	case EPERM:
		err = ERROR_DATA_SET_PERM;
		break;
		
	case EROFS:
		err = ERROR_DATA_SET_ROFS;
		break;
		
	case ETXTBSY:
		err = ERROR_DATA_SET_TXTBSY;
		break;
		
	case EBADF:
		err = ERROR_DATA_SET_BADF;
		break;
		
	case EAGAIN:
		err = ERROR_DATA_SET_AGAIN;
		break;

	default:
		err = ERROR_DATA_SET;
	}
	
	return err;
}


struct mem_map * create_mem_map(char *filename, size_t struct_size,
				int parser(const char *, void *, void *),
				void *priv_data, int *errp)
{
	int fd, err, i;
	char *saveptr = NULL;     /* it must be null for strtok_r */
	char *row, *line;
	unsigned char *ptr;
	struct stat info;
	struct mem_map *mem;
	size_t maped_size, file_size, line_size, items;

	if (!filename || !struct_size || !parser) {
		*errp = ERROR_DATA_SET_ARGS;
		return NULL;
	}
	
	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		*errp = get_error(errno);
	        return NULL;
	}

	err = fstat(fd, &info);
	if (err) {
		*errp = ERROR_DATA_SET_FSTAT;
		goto close_file;
	}
	
	file_size = info.st_size;
	if (!file_size) {
	        *errp = ERROR_DATA_SET_FEMPTY;
		goto close_file;
	}
	
        row = mmap(NULL, file_size, MAP_PROT, MAP_FLAGS, fd, 0);
	if (row == MAP_FAILED) {
		*errp = get_error(errno);
		goto close_file;
	}

        line = strtok_r(row, "\n", &saveptr);
	if(!line) {
	        *errp = ERROR_DATA_SET_FEMPTY;
		goto unmap_file;
	}

	/* to know how many itmes should be in the data set */
	line_size = strlen(line) + 1; /* +1 for the newline char */
	items = file_size / line_size;
	if (line_size * items != file_size) {
	        *errp = ERROR_DATA_SET_FCORRUPT;
		goto unmap_file;
	}
	
	maped_size = (items * struct_size) + sizeof(struct mem_map);
	
	mem = mmap(NULL, maped_size, MAP_PROT, MAP_FLAGS | MAP_ANON, -1, 0);
	if (mem == MAP_FAILED) {
		*errp = get_error(errno);
		goto unmap_file;
	}

	mem->items = items;
	mem->maped_size = maped_size;

	ptr = mem->data;
	for (i = 0; line && i < items; i++) {
		err = parser(line, ptr, priv_data);
		if (err) {
			*errp = ERROR_DATA_SET_PARSER;
			goto unmap_data;
		}
	        line = strtok_r(NULL, "\n", &saveptr);
		ptr += struct_size;
	}
	
	if (line || i != items) {
		*errp = ERROR_DATA_SET_FCORRUPT;
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
