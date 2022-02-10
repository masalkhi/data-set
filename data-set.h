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

#ifndef __DATA_SET_H__
#define __DATA_SET_H__

#ifndef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
#endif

#include <sys/mman.h>

#ifndef __cplusplus
# include <stddef.h>
#else
# include <cstddef>
#endif

#undef __align

#ifdef __GNUC__
# define __attribute__(...)  __attribute__(__VA_ARGS__)
# define __extension__       __extension__
# define __align(arg)        __attribute__((aligned(arg)))
#else
# define __attribute__(...)
# define __extension__
# define __align(arg)
#endif

#ifndef __nonnull
# define __nonnull(args) __attribute__((__nonnull__ args))
#endif

#ifndef __always_inline
# define __always_inline inline __attribute__((always_inline))
#endif

#define __get_entry(ptr, type, member)					\
	((type *)((ptr) - offsetof(type, member)))

#ifdef __GNUC__
# define MP_DATA_SIZE   (0u)
# define data_entry(ptr) __extension__					\
	({								\
		__typeof__(*ptr) *__ptr = (ptr);			\
		(void)(__ptr == (struct mem_map){0}.data);		\
		__get_entry(__ptr, struct mem_map, data);		\
	})
#else
# define MP_DATA_SIZE   (1u)
# define data_entry(ptr) __get_entry(ptr, struct mem_map, data)
#endif /* __GNUC__ */


struct mem_map {
	size_t maped_size;       // it would be used with munmap
	size_t items;            // the number of the element in the set
	__extension__  __align(64) unsigned char data[MP_DATA_SIZE];
};


#ifndef __cplusplus
struct mem_map * create_mem_map(char *filename, size_t struct_size,
				int parser(const char *, void *, void *),
				void *);
#else
extern "C" {
	struct mem_map * create_mem_map(char *filename, size_t struct_size,
					int parser(const char *, void *, void *)
					void *);
}
#endif /* !__cplusplus */


static __always_inline void destroy_mem_map(struct mem_map * mem)
{
	if(!mem)
		return;
	
	munmap(mem, mem->maped_size);
}


/**
 * destroy_data_set - to destroy a data set created with create_data_set
 * @ptr: the pointer which returned from create_data_set function
 *
 * NOTE: use always this function to destroy the data-set that is created 
 * with create_data_set and do not try to free the memory by yourself
 */
static __always_inline void destroy_data_set(void *ptr)
{
	if (!ptr)
		return;
	
	destroy_mem_map(data_entry((unsigned char *)ptr));
}


/**
 * create_data_set - to create a data set
 * @filename: the csv file name that contains the data
 * @struct_size: the size of the structure, that would be used to store 
 *        data from one csv line
 * @parser: function pointer to the function that would be called on each
 *        line in the file, it takes 3 params, a line from the csv file 
 *        terminated with \0 (the \n is omitted), a pointer to a buffer of 
 *        the size struct_size, and a priv_data. the function most return 
 *        integer 0 on on success, and a non-zero value on failure.
 * @priv_data: a pointer that will be passed to the parser, it can be NULL
 *        or an address to a private struct.
 *
 * It returns a pointer to the fisrt element in the set, or NULL if parser
 * returns a non-zero value or on failure and a message printed to stderr.
 *
 * NOTE: if the filename or parser is NULL, a compiler warning will be 
 * generated if -Wnonnull option is enabled and the compiler implements 
 * gcc extensions.
 */
static __always_inline __nonnull((1, 3))
void * create_data_set(char *filename, size_t struct_size,
		       int parser(const char *, void *, void *),
		       void *priv_data)
{
        struct mem_map *mem = create_mem_map(filename, struct_size, parser,
					     priv_data);
	if (!mem)
		return NULL;
	
	return mem->data;
}


/**
 * get_data_length - to get the number of the elements in the data set
 * @ptr: a pointer to the data set
 * 
 * NOTE: the ptr must be the pointer returned from create_data_set fucntion,
 * which is the pointer to the first element in the data set
 */
static __always_inline size_t get_total_items(void *ptr)
{
	if (!ptr)
		return 0;
	
	return data_entry((unsigned char *)ptr)->items;
}


#undef __attribute__
#undef __extension__
#undef __align
#undef __get_entry
#undef data_entry
#undef MP_DATA_SIZE


#endif /* !__DATA_SET_H__ */
