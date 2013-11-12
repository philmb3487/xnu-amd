/*
 * Copyright (c) 2005-2006 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <debug.h>

#include <kern/page_decrypt.h>
#include <kern/task.h>
#include <machine/commpage.h>


/* Sinetek: Use in-kernel decryptor unless FAKESMC defined. */
/* #define FAKESMC */
#ifndef FAKESMC
#include "../../bsd/crypto/blowfish/blowfish.h"
#include "../../bsd/crypto/blowfish/bf_locl.h"

/* BLOWFISH DECRYPT from OpenSSL.
 */
void BF_cbc_encrypt(const unsigned char *in, unsigned char *out, long length,
	const BF_KEY *schedule, unsigned char *ivec, int encrypt)
{
	register BF_LONG tin0,tin1;
	register BF_LONG tout0,tout1,xor0,xor1;
	register long l=length;
	BF_LONG tin[2];

	if (encrypt)
	{
	n2l(ivec,tout0);
	n2l(ivec,tout1);
	ivec-=8;
	for (l-=8; l>=0; l-=8)
	{
	n2l(in,tin0);
	n2l(in,tin1);
	tin0^=tout0;
	tin1^=tout1;
	tin[0]=tin0;
	tin[1]=tin1;
	BF_encrypt(tin,schedule);
	tout0=tin[0];
	tout1=tin[1];
	l2n(tout0,out);
	l2n(tout1,out);
	}
	if (l != -8)
	{
	n2ln(in,tin0,tin1,l+8);
	tin0^=tout0;
	tin1^=tout1;
	tin[0]=tin0;
	tin[1]=tin1;
	BF_encrypt(tin,schedule);
	tout0=tin[0];
	tout1=tin[1];
	l2n(tout0,out);
	l2n(tout1,out);
	}
	l2n(tout0,ivec);
	l2n(tout1,ivec);
	}
	else
	{
	n2l(ivec,xor0);
	n2l(ivec,xor1);
	ivec-=8;
	for (l-=8; l>=0; l-=8)
	{
	n2l(in,tin0);
	n2l(in,tin1);
	tin[0]=tin0;
	tin[1]=tin1;
	BF_decrypt(tin,schedule);
	tout0=tin[0]^xor0;
	tout1=tin[1]^xor1;
	l2n(tout0,out);
	l2n(tout1,out);
	xor0=tin0;
	xor1=tin1;
	}
	if (l != -8)
	{
	n2l(in,tin0);
	n2l(in,tin1);
	tin[0]=tin0;
	tin[1]=tin1;
	BF_decrypt(tin,schedule);
	tout0=tin[0]^xor0;
	tout1=tin[1]^xor1;
	l2nn(tout0,tout1,out,l+8);
	xor0=tin0;
	xor1=tin1;
	}
	l2n(xor0,ivec);
	l2n(xor1,ivec);
	}
	tin0=tin1=tout0=tout1=xor0=xor1=0;
	tin[0]=tin[1]=0;
}

/* Perform in-kernel memory decryption of a page. Since Snow Leopard they use BLOWFISH.
 * "ops" is treated as a value, and it indicates which decryption type should be attempted.
 */
int ink_decrypt(const void* from, void *to, unsigned long long src_offset, void *ops)
{
	BF_KEY key;
	unsigned char null_ivec[32] = {0,};
	static const unsigned char plain_key [65] =
		"ourhardworkbythesewordsguardedpleasedontsteal(c)AppleComputerInc";
	BF_set_key(&key, 64, plain_key);
	BF_cbc_encrypt(from, to, PAGE_SIZE, &key, null_ivec, BF_DECRYPT);
	return KERN_SUCCESS;
}

static dsmos_page_transform_hook_t dsmos_hook = ink_decrypt;
#else
static dsmos_page_transform_hook_t dsmos_hook = NULL;
#endif

void
dsmos_page_transform_hook(dsmos_page_transform_hook_t hook)
{

	printf("DSMOS has arrived\n");
	/* set the hook now - new callers will run with it */
	dsmos_hook = hook;
}

int
dsmos_page_transform(const void* from, void *to, unsigned long long src_offset, void *ops)
{
	static boolean_t first_wait = TRUE;

	if (dsmos_hook == NULL) {
		if (first_wait) {
			first_wait = FALSE;
			printf("Waiting for DSMOS...\n");
		}
		return KERN_ABORTED;
	}
	return (*dsmos_hook) (from, to, src_offset, ops);
}


text_crypter_create_hook_t text_crypter_create=NULL;
void text_crypter_create_hook_set(text_crypter_create_hook_t hook)
{
	text_crypter_create=hook;
}
