/*
 * Authors: Martin Nagy <mnagy@redhat.com>
 *
 * Copyright (C) 2009  Red Hat
 * see file 'COPYING' for use and warranty information
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 or later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LD_UTIL_H_
#define _LD_UTIL_H_

#include <string.h>

#include <isc/mem.h>
#include <isc/buffer.h>
#include <dns/types.h>
#include <dns/name.h>

#include "log.h"

extern isc_boolean_t verbose_checks; /* from settings.c */

#define CLEANUP_WITH(result_code)				\
	do {							\
		result = (result_code);				\
		goto cleanup;					\
	} while(0)

#define CHECK(op)						\
	do {							\
		result = (op);					\
		if (result != ISC_R_SUCCESS) {			\
			if (verbose_checks == ISC_TRUE)		\
				log_error_position("check failed: %s",		\
						   dns_result_totext(result));	\
			goto cleanup;				\
		}						\
	} while (0)

#define CHECKED_MEM_ALLOCATE(m, target_ptr, s)			\
	do {							\
		(target_ptr) = isc_mem_allocate((m), (s));	\
		if ((target_ptr) == NULL) {			\
			result = ISC_R_NOMEMORY;		\
			log_error_position("Memory allocation failed");	\
			goto cleanup;				\
		}						\
	} while (0)

#define CHECKED_MEM_GET(m, target_ptr, s)			\
	do {							\
		(target_ptr) = isc_mem_get((m), (s));		\
		if ((target_ptr) == NULL) {			\
			result = ISC_R_NOMEMORY;		\
			log_error_position("Memory allocation failed");	\
			goto cleanup;				\
		}						\
	} while (0)

#define CHECKED_MEM_GET_PTR(m, target_ptr)			\
	CHECKED_MEM_GET(m, target_ptr, sizeof(*(target_ptr)))

#define CHECKED_MEM_STRDUP(m, source, target)			\
	do {							\
		(target) = isc_mem_strdup((m), (source));	\
		if ((target) == NULL) {				\
			result = ISC_R_NOMEMORY;		\
			log_error_position("Memory allocation failed");	\
			goto cleanup;				\
		}						\
	} while (0)

#define ZERO_PTR(ptr) memset((ptr), 0, sizeof(*(ptr)))

#define SAFE_MEM_PUT(m, target_ptr, target_size)		\
	do {							\
		if ((target_ptr) != NULL)			\
			isc_mem_put((m), (target_ptr),		\
				    (target_size));		\
	} while (0)

#define SAFE_MEM_PUT_PTR(m, target_ptr)				\
	SAFE_MEM_PUT((m), (target_ptr), sizeof(*(target_ptr)))

#define MEM_PUT_AND_DETACH(target_ptr)				\
	isc_mem_putanddetach(&(target_ptr)->mctx, target_ptr,	\
			     sizeof(*(target_ptr)))

#define DECLARE_BUFFER(name, len)				\
	isc_buffer_t name;					\
	unsigned char name##__base[len]

#define INIT_BUFFER(name)					\
	isc_buffer_init(&name, name##__base, sizeof(name##__base))

#define DECLARE_BUFFERED_NAME(name)				\
	dns_name_t name;					\
	DECLARE_BUFFER(name##__buffer, DNS_NAME_MAXWIRE)

#define INIT_BUFFERED_NAME(name)					\
	do {								\
		INIT_BUFFER(name##__buffer);				\
		dns_name_init(&name, NULL);				\
		dns_name_setbuffer(&name, &name##__buffer);		\
	} while (0)

/* If no argument index list is given to the nonnull attribute,
 * all pointer arguments are marked as non-null. */
#define ATTR_NONNULLS     ATTR_NONNULL()
#ifdef __GNUC__
#define ATTR_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#define ATTR_CHECKRESULT __attribute__((warn_unused_result))
#else
#define ATTR_NONNULL(...)
#define ATTR_CHECKRESULT
#endif

/*
 * Static (compile-time) assert for C:
 * C99 doesn't require support for "sizeof" in preprocessor conditionals so
 * we can't do something like #if (sizeof(my_struct) != 512).
 *
 * This macro has no runtime side affects as it just defines an enum whose name
 * depends on the current line, and whose value will give a divide by zero error
 * at compile time if the assertion is false.
 *
 * Taken from
 * http://www.pixelbeat.org/programming/gcc/static_assert.html
 * version 10 Feb 2015. Padraig Brady told me that it is licensed under
 * "GNU All-Permissive License":
 *
 * Copying and distribution of this file, with or without modification,
 * are permitted in any medium without royalty provided the copyright notice
 * and this notice are preserved. This code is offered as-is,
 * without any warranty.
 */
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
/* These can't be used after statements in c89. */
#ifdef __COUNTER__
  #define STATIC_ASSERT(e, m) \
    ;enum { ASSERT_CONCAT(static_assert_, __COUNTER__) = 1/(!!(e)) }
#else
  /* This can't be used twice on the same line so ensure if using in headers
   * that the headers are not included twice (by wrapping in #ifndef...#endif)
   * Note it doesn't cause an issue when used on same line of separate modules
   * compiled with gcc -combine -fwhole-program.  */
  #define STATIC_ASSERT(e, m) \
    ;enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
#endif

#endif /* !_LD_UTIL_H_ */
