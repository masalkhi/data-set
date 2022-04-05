/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (C) 2022 by Abd-Alrhman Masalkhi
 */

/**
 * @Description:
 * The purpose of data-set project is to create a data set from a csv 
 * file, the code is optimized for big csv files. Unnecessarily overhead, 
 * such as the read and write syscalls overhead, are removed. If the 
 * compiler implements GCC extensions, the first element guaranteed to 
 * be 64 Byte aligned (on most platforms, that L1 cache line alignment)
 *
 * @auther: Abd-Alrhman Masalkhi
 * @data: 2022
 * 
 */

#ifndef __DATA_SET_H__
#define __DATA_SET_H__


#include <sys/mman.h>

#ifndef __cplusplus
# include <stddef.h>
#else
# include <cstddef>
#endif


/* from error 1 to 29, replace ERROR_DATA_SET_ with E and check in the men page
 * of errno for the documentation */
#define ERROR_DATA_SET_ACCES         (1)
#define ERROR_DATA_SET_DQUOT         (2)
#define ERROR_DATA_SET_EXIST         (3)
#define ERROR_DATA_SET_FAULT         (4)
#define ERROR_DATA_SET_FBIG          (5)
#define ERROR_DATA_SET_INTR          (6)
#define ERROR_DATA_SET_INVAL         (7)
#define ERROR_DATA_SET_ISDIR         (8)
#define ERROR_DATA_SET_LOOP          (9)
#define ERROR_DATA_SET_MFILE         (10)
#define ERROR_DATA_SET_NAMETOOLONG   (11)
#define ERROR_DATA_SET_NFILE         (12)
#define ERROR_DATA_SET_NODEV         (13)
#define ERROR_DATA_SET_NOENT         (14)
#define ERROR_DATA_SET_NOMEM         (15)
#define ERROR_DATA_SET_NOSPC         (16)
#define ERROR_DATA_SET_NOTDIR        (17)
#define ERROR_DATA_SET_NXIO          (18)
#define ERROR_DATA_SET_OPNOTSUPP     (19)
#define ERROR_DATA_SET_OVERFLOW      (20)
#define ERROR_DATA_SET_PERM          (21)
#define ERROR_DATA_SET_ROFS          (22)
#define ERROR_DATA_SET_TXTBSY        (23)
#define ERROR_DATA_SET_WOULDBLOCK    (24)
#define ERROR_DATA_SET_BADF          (25)
#define ERROR_DATA_SET_AGAIN         (26) 
#define ERROR_DATA_SET_SIGSEGV       (27)
#define ERROR_DATA_SET_SIGBUS        (28)
#define	ERROR_DATA_SET_ARGS          (29)
#define ERROR_DATA_SET_FEMPTY        (30)  /* the file is empty */
#define ERROR_DATA_SET_FCORRUPT      (31)  /* the file is corrupted */
#define ERROR_DATA_SET_PARSER        (32)  /* parser has returned non-zero */
#define ERROR_DATA_SET               (33)  /* the default error */


#ifdef __GNUC__
#  define __extension__  __extension__
#else
#  define __extension__
#endif


#undef __align
#ifdef __has_attribute
#  if __has_attribute (aligned)
#    define __align(args) __attribute__((aligned(args)))
#  else
#    define __align(args)
#  endif
#else
#  define __align(args)
#endif


#ifndef __nonnull
#  ifdef __has_attribute
#    if __has_attribute (nonnull)
#      define __nonnull(args) __attribute__((nonnull args))
#    else
#      define __nonnull(args)
#    endif
#  else
#    define __nonnull(args)
#  endif
#endif


#ifndef __always_inline
#  ifdef __has_attribute
#    if __has_attribute (always_inline)
#      define __always_inline inline __attribute__((always_inline))
#    else
#      define __always_inline inline
#    endif
#  else
#    define __always_inline inline
#  endif
#endif


#define __get_entry(ptr, type, member)					\
	((type *)(((unsigned char *)ptr) - offsetof(type, member)))


#ifndef __GNUC__
#  define MEM_MAP_DATA_SIZE (1u)
#  define data_entry(ptr) __get_entry(ptr, struct mem_map, data)
#else
#  define MEM_MAP_DATA_SIZE (0u)
#  define data_entry(ptr) __extension__					\
	({								\
	        __typeof__(*ptr) *__ptr = (ptr);			\
		(void)(__ptr == (struct mem_map){0}.data);		\
		__get_entry(__ptr, struct mem_map, data);		\
	})
#endif


struct mem_map {
	size_t maped_size;       // it would be used with munmap
	size_t items;            // the number of the element in the set
	__extension__  __align(64) unsigned char data[MEM_MAP_DATA_SIZE];
};


#ifdef __cplusplus
extern "C" {
#endif
	
struct mem_map * create_mem_map(char *filename, size_t struct_size,
				int parser(const char *, void *, void *),
				void *, int *);
	
#ifdef __cplusplus
}
#endif


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

	destroy_mem_map(data_entry(ptr));
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
 * @errp: a pointer to an int to save the error if one was encountered
 *
 * It returns a pointer to the fisrt element in the set, or NULL if parser
 * returns a non-zero value or on failure.
 *
 * NOTE: if the filename or parser is NULL, a compiler warning will be 
 * generated if -Wnonnull option is enabled and the compiler implements 
 * gcc extensions.
 */
static __always_inline __nonnull((1, 3))
void * create_data_set(char *filename, size_t struct_size,
		       int parser(const char *, void *, void *),
		       void *priv_data, int *errp)
{
	int err;
        struct mem_map *mem;

	if (!errp)
		errp = &err;
	
	mem = create_mem_map(filename, struct_size, parser, priv_data, errp);
	if (!mem)
		return NULL;
	
	return mem->data;
}


/**
 * get_data_set_length - to get the number of the elements in the data set
 * @ptr: a pointer to the data set
 * 
 * NOTE: the ptr must be the pointer returned from create_data_set fucntion,
 * which is the pointer to the first element in the data set
 */
static __always_inline size_t get_data_set_length(void *ptr)
{
	if (!ptr)
		return 0;

	return data_entry(ptr)->items;
}


#undef __attribute__
#undef __extension__
#undef __align
#undef __get_entry
#undef data_entry
#undef MP_DATA_SIZE


#endif /* !__DATA_SET_H__ */
