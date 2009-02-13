/* Authors: Martin Nagy <mnagy@redhat.com>
 *
 * Copyright (C) 2009  Red Hat
 * see file 'COPYING' for use and warranty information
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 only
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

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/result.h>
#include <isc/util.h>

#include <dns/name.h>

#include <errno.h>
#define LDAP_DEPRECATED 1
#include <ldap.h>

#include "str.h"
#include "ldap_convert.h"
#include "log.h"
#include "util.h"


static isc_result_t dn_to_text(const char *dn, const char *root_dn,
			       ld_string_t *target);
static isc_result_t explode_dn(const char *dn, char ***explodedp, int notypes);
static unsigned int count_rdns(char **exploded);


isc_result_t
dn_to_dnsname(isc_mem_t *mctx, const char *dn, const char *root_dn,
	      dns_name_t *target)
{
	isc_result_t result;
	ld_string_t *str;
	isc_buffer_t source_buffer;
	isc_buffer_t target_buffer;
	dns_name_t tmp_name;
	unsigned char target_base[DNS_NAME_MAXWIRE];

	REQUIRE(mctx != NULL);
	REQUIRE(dn != NULL);

	str = NULL;
	result = ISC_R_SUCCESS;

	/* Convert the DN into a DNS name. */
	CHECK(str_new(mctx, &str));
	CHECK(dn_to_text(dn, root_dn, str));

	/* TODO: fix this */
	isc_buffer_init(&source_buffer, str_buf(str), str_len(str) - 1);
	isc_buffer_add(&source_buffer, str_len(str) - 1);
	isc_buffer_init(&target_buffer, target_base, sizeof(target_base));

	/* Now create a dns_name_t struct. */
	dns_name_init(&tmp_name, NULL);
	dns_name_setbuffer(&tmp_name, &target_buffer);

	dns_name_fromtext(&tmp_name, &source_buffer, dns_rootname, 0, NULL);

cleanup:
	if (result != ISC_R_FAILURE)
		result = dns_name_dupwithoffsets(&tmp_name, mctx, target);

	str_destroy(&str);

	return result;
}

/*
 * Convert LDAP dn to DNS name. If root_dn is not NULL then count how much RNDs
 * it contains and ignore that much trailing RNDs from dn.
 *
 * Example:
 * dn = "idnsName=foo, idnsName=bar, idnsName=example.org, cn=dns,"
 *      "dc=example, dc=org"
 * root_dn = "cn=dns, dc=example, dc=org"
 *
 * The resulting string will be "foo.bar.example.org."
 */
static isc_result_t
dn_to_text(const char *dn, const char *root_dn, ld_string_t *target)
{
	isc_result_t result;
	unsigned int count;
	char **exploded_dn = NULL;
	char **exploded_root = NULL;

	REQUIRE(dn != NULL);
	REQUIRE(target != NULL);

	result = ISC_R_SUCCESS;

	CHECK(explode_dn(dn, &exploded_dn, 1));
	count = count_rdns(exploded_dn);

	if (root_dn != NULL) {
		unsigned int count_root;

		CHECK(explode_dn(root_dn, &exploded_root, 1));
		count_root = count_rdns(exploded_root);
		if (exploded_root > exploded_dn) {
			result = ISC_R_FAILURE;
			goto cleanup;
		}
		count -= count_root;
	}

	str_init_char(target, "");
	for (unsigned int i = 0; exploded_dn[i] != NULL && i < count; i++) {
		str_cat_char(target, exploded_dn[i]);
		str_cat_char(target, ".");
	}

	if (str_len(target) == 0)
		str_init_char(target, ".");

cleanup:
	if (exploded_dn != NULL)
		ldap_value_free(exploded_dn);
	if (exploded_root != NULL)
		ldap_value_free(exploded_root);

	return result;
}

static isc_result_t
explode_dn(const char *dn, char ***explodedp, int notypes)
{
	isc_result_t result;
	char **exploded;

	REQUIRE(dn != NULL);
	REQUIRE(explodedp != NULL && *explodedp == NULL);

	result = ISC_R_SUCCESS;

	exploded = ldap_explode_dn(dn, notypes);
	if (exploded == NULL) {
		if (errno == ENOMEM) {
			return ISC_R_NOMEMORY;
		} else {
			log_error("ldap_explode_dn(\"%s\") failed, error code %d",
				  dn, errno);
			return ISC_R_FAILURE;
		}
	}

	*explodedp = exploded;

	return ISC_R_SUCCESS;
}

static unsigned int
count_rdns(char **exploded)
{
	unsigned int ret;

	REQUIRE(exploded != NULL);

	ret = 0;
	while (exploded[ret] != NULL)
		ret++;

	return ret;
}