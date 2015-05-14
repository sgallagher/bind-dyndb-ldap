/*
 * Authors: Martin Nagy <mnagy@redhat.com>
 *          Adam Tkac <atkac@redhat.com>
 *
 * Copyright (C) 2008, 2011  Red Hat
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

#ifndef _LD_LDAP_ENTRY_H_
#define _LD_LDAP_ENTRY_H_

#include <isc/lex.h>
#include <isc/util.h>
#include <dns/types.h>

#include "fwd_register.h"
#include "util.h"
#include "str.h"
#include "types.h"

#define LDAP_DEPRECATED 1
#include <ldap.h>

/* Represents values associated with LDAP attribute */
typedef struct ldap_value ldap_value_t;
typedef LIST(ldap_value_t) ldap_valuelist_t;
struct ldap_value {
        char                    *value;
        LINK(ldap_value_t)      link;
};

/* Represents LDAP attribute and it's values */
typedef struct ldap_attribute	ldap_attribute_t;
typedef LIST(ldap_attribute_t)	ldap_attributelist_t;

/* Represents LDAP entry and it's attributes */
typedef LIST(ldap_entry_t)	ldap_entrylist_t;
struct ldap_entry {
	char			*dn;
	struct berval		*uuid;
	ldap_attribute_t	*lastattr;
	ldap_attributelist_t	attrs;
	LINK(ldap_entry_t)	link;

	/* Parsing. */
	isc_lex_t		*lex;
	isc_buffer_t		rdata_target;
	unsigned char		*rdata_target_mem;
};

/* Represents LDAP attribute and it's values */
struct ldap_attribute {
	char			*name;
	char			**ldap_values;
	ldap_value_t		*lastval;
	ldap_valuelist_t	values;
	LINK(ldap_attribute_t)	link;
};

#define LDAP_ENTRYCLASS_NONE	0x0
#define LDAP_ENTRYCLASS_RR	0x1
#define LDAP_ENTRYCLASS_MASTER	0x2
#define LDAP_ENTRYCLASS_CONFIG	0x4
#define LDAP_ENTRYCLASS_FORWARD	0x8

#define DEFAULT_TTL 86400

typedef unsigned char		ldap_entryclass_t;

/* Max type length definitions, from lib/dns/master.c */
#define TOKENSIZ (8*1024)

isc_result_t ATTR_NONNULLS ATTR_CHECKRESULT
ldap_entry_init(isc_mem_t *mctx, ldap_entry_t **entryp);

isc_result_t
ldap_entrylist_create(isc_mem_t *mctx, LDAP *ld, LDAPMessage *msg,
		      ldap_entrylist_t *entrylist) ATTR_NONNULLS ATTR_CHECKRESULT;

void
ldap_entrylist_destroy(isc_mem_t *mctx, ldap_entrylist_t *entrylist) ATTR_NONNULLS;

isc_result_t
ldap_entrylist_append(isc_mem_t *mctx, LDAP *ld, LDAPMessage *msg,
		      ldap_entrylist_t *entrylist) ATTR_NONNULLS ATTR_CHECKRESULT;

/*
 * ldap_entry_create
 *
 * Creates ldap_entry_t from message "result" received via "ld" connection
 */
isc_result_t
ldap_entry_create(isc_mem_t *mctx, LDAP *ld, LDAPMessage *ldap_entry,
		  struct berval	*uuid, ldap_entry_t **entryp) ATTR_NONNULLS ATTR_CHECKRESULT;

isc_result_t
ldap_entry_reconstruct(isc_mem_t *mctx, zone_register_t *zr, const char *ldap_base,
		       mldapdb_t *mldap, struct berval *uuid,
		       ldap_entry_t **entryp) ATTR_NONNULLS ATTR_CHECKRESULT;

void
ldap_entry_destroy(isc_mem_t *mctx, ldap_entry_t **entryp) ATTR_NONNULLS;

isc_result_t
ldap_entry_getvalues(const ldap_entry_t *entry, const char *attrname,
		     ldap_valuelist_t *values) ATTR_NONNULLS ATTR_CHECKRESULT;

dns_rdataclass_t
ldap_entry_getrdclass(const ldap_entry_t *entry) ATTR_NONNULLS ATTR_CHECKRESULT;

ldap_attribute_t*
ldap_entry_nextattr(ldap_entry_t *entry) ATTR_NONNULLS ATTR_CHECKRESULT;

isc_result_t
ldap_entry_firstrdtype(ldap_entry_t *entry, ldap_attribute_t **attrp,
		       dns_rdatatype_t *rdtype) ATTR_NONNULLS ATTR_CHECKRESULT;

isc_result_t
ldap_entry_nextrdtype(ldap_entry_t *entry, ldap_attribute_t **attrp,
		      dns_rdatatype_t *rdtype) ATTR_NONNULLS ATTR_CHECKRESULT;

isc_result_t
ldap_entry_getfakesoa(ldap_entry_t *entry, const char *fake_mname,
		      ld_string_t *target) ATTR_NONNULLS ATTR_CHECKRESULT;

/*
 * ldap_entry_getclass
 *
 * Get entry class (bitwise OR of the LDAP_ENTRYCLASS_*). Note that
 * you must ldap_search for objectClass attribute!
 */
isc_result_t
ldap_entry_getclass(ldap_entry_t *entry, ldap_entryclass_t *class) ATTR_NONNULLS ATTR_CHECKRESULT;

isc_result_t
ldap_entry_guessclass(dns_name_t *entry_name, isc_boolean_t iszone,
		      fwd_register_t *fwd_register, ldap_entryclass_t *class)
		      ATTR_NONNULLS ATTR_CHECKRESULT;

isc_result_t
ldap_attr_firstvalue(ldap_attribute_t *attr, ld_string_t *str) ATTR_NONNULLS ATTR_CHECKRESULT;

/*
 * ldap_attr_nextvalue
 *
 * Returns pointer to value in case of success, NULL if no other val is
 * available
 */
isc_result_t
ldap_attr_nextvalue(ldap_attribute_t *attr, ld_string_t *value) ATTR_NONNULLS ATTR_CHECKRESULT;

dns_ttl_t
ldap_entry_getttl(const ldap_entry_t *entry) ATTR_NONNULLS ATTR_CHECKRESULT;

#endif /* !_LD_LDAP_ENTRY_H_ */
