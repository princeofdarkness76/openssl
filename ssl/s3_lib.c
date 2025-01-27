/* ssl/s3_lib.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 *
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 *
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 *
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */
/* ====================================================================
 * Copyright (c) 1998-2007 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */
/* ====================================================================
 * Copyright 2002 Sun Microsystems, Inc. ALL RIGHTS RESERVED.
 *
 * Portions of the attached software ("Contribution") are developed by
 * SUN MICROSYSTEMS, INC., and are contributed to the OpenSSL project.
 *
 * The Contribution is licensed pursuant to the OpenSSL open source
 * license provided above.
 *
 * ECC cipher suite support in OpenSSL originally written by
 * Vipul Gupta and Sumit Gupta of Sun Microsystems Laboratories.
 *
 */
/* ====================================================================
 * Copyright 2005 Nokia. All rights reserved.
 *
 * The portions of the attached software ("Contribution") is developed by
 * Nokia Corporation and is licensed pursuant to the OpenSSL open source
 * license.
 *
 * The Contribution, originally written by Mika Kousa and Pasi Eronen of
 * Nokia Corporation, consists of the "PSK" (Pre-Shared Key) ciphersuites
 * support (see RFC 4279) to OpenSSL.
 *
 * No patent licenses or other rights except those expressly stated in
 * the OpenSSL open source license shall be deemed granted or received
 * expressly, by implication, estoppel, or otherwise.
 *
 * No assurances are provided by Nokia that the Contribution does not
 * infringe the patent or other intellectual property rights of any third
 * party or that the license provides you with all the necessary rights
 * to make use of the Contribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND. IN
 * ADDITION TO THE DISCLAIMERS INCLUDED IN THE LICENSE, NOKIA
 * SPECIFICALLY DISCLAIMS ANY LIABILITY FOR CLAIMS BROUGHT BY YOU OR ANY
 * OTHER ENTITY BASED ON INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS OR
 * OTHERWISE.
 */

#include <stdio.h>
#include <openssl/objects.h>
#include "ssl_locl.h"
#include <openssl/md5.h>
#ifndef OPENSSL_NO_DH
# include <openssl/dh.h>
#endif
#include <openssl/rand.h>

#define SSL3_NUM_CIPHERS        OSSL_NELEM(ssl3_ciphers)

/* list of available SSLv3 ciphers (sorted by id) */
OPENSSL_GLOBAL const SSL_CIPHER ssl3_ciphers[] = {

/* The RSA ciphers */
/* Cipher 01 */
    {
     1,
     SSL3_TXT_RSA_NULL_MD5,
     SSL3_CK_RSA_NULL_MD5,
     SSL_kRSA,
     SSL_aRSA,
     SSL_eNULL,
     SSL_MD5,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },

/* Cipher 02 */
    {
     1,
     SSL3_TXT_RSA_NULL_SHA,
     SSL3_CK_RSA_NULL_SHA,
     SSL_kRSA,
     SSL_aRSA,
     SSL_eNULL,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },

/* Cipher 04 */
    {
     1,
     SSL3_TXT_RSA_RC4_128_MD5,
     SSL3_CK_RSA_RC4_128_MD5,
     SSL_kRSA,
     SSL_aRSA,
     SSL_RC4,
     SSL_MD5,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

/* Cipher 05 */
    {
     1,
     SSL3_TXT_RSA_RC4_128_SHA,
     SSL3_CK_RSA_RC4_128_SHA,
     SSL_kRSA,
     SSL_aRSA,
     SSL_RC4,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

/* Cipher 07 */
#ifndef OPENSSL_NO_IDEA
    {
     1,
     SSL3_TXT_RSA_IDEA_128_SHA,
     SSL3_CK_RSA_IDEA_128_SHA,
     SSL_kRSA,
     SSL_aRSA,
     SSL_IDEA,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },
#endif

/* Cipher 0A */
    {
     1,
     SSL3_TXT_RSA_DES_192_CBC3_SHA,
     SSL3_CK_RSA_DES_192_CBC3_SHA,
     SSL_kRSA,
     SSL_aRSA,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

/* Cipher 13 */
    {
     1,
     SSL3_TXT_DHE_DSS_DES_192_CBC3_SHA,
     SSL3_CK_DHE_DSS_DES_192_CBC3_SHA,
     SSL_kDHE,
     SSL_aDSS,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

/* Cipher 16 */
    {
     1,
     SSL3_TXT_DHE_RSA_DES_192_CBC3_SHA,
     SSL3_CK_DHE_RSA_DES_192_CBC3_SHA,
     SSL_kDHE,
     SSL_aRSA,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

/* Cipher 18 */
    {
     1,
     SSL3_TXT_ADH_RC4_128_MD5,
     SSL3_CK_ADH_RC4_128_MD5,
     SSL_kDHE,
     SSL_aNULL,
     SSL_RC4,
     SSL_MD5,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

/* Cipher 1B */
    {
     1,
     SSL3_TXT_ADH_DES_192_CBC_SHA,
     SSL3_CK_ADH_DES_192_CBC_SHA,
     SSL_kDHE,
     SSL_aNULL,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },
#ifndef OPENSSL_NO_PSK
    /* Cipher 2C */
    {
     1,
     TLS1_TXT_PSK_WITH_NULL_SHA,
     TLS1_CK_PSK_WITH_NULL_SHA,
     SSL_kPSK,
     SSL_aPSK,
     SSL_eNULL,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },
    /* Cipher 2D */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_NULL_SHA,
     TLS1_CK_DHE_PSK_WITH_NULL_SHA,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_eNULL,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },
    /* Cipher 2E */
    {
     1,
     TLS1_TXT_RSA_PSK_WITH_NULL_SHA,
     TLS1_CK_RSA_PSK_WITH_NULL_SHA,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_eNULL,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },
#endif

/* New AES ciphersuites */
/* Cipher 2F */
    {
     1,
     TLS1_TXT_RSA_WITH_AES_128_SHA,
     TLS1_CK_RSA_WITH_AES_128_SHA,
     SSL_kRSA,
     SSL_aRSA,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },
/* Cipher 32 */
    {
     1,
     TLS1_TXT_DHE_DSS_WITH_AES_128_SHA,
     TLS1_CK_DHE_DSS_WITH_AES_128_SHA,
     SSL_kDHE,
     SSL_aDSS,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },
/* Cipher 33 */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_AES_128_SHA,
     TLS1_CK_DHE_RSA_WITH_AES_128_SHA,
     SSL_kDHE,
     SSL_aRSA,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },
/* Cipher 34 */
    {
     1,
     TLS1_TXT_ADH_WITH_AES_128_SHA,
     TLS1_CK_ADH_WITH_AES_128_SHA,
     SSL_kDHE,
     SSL_aNULL,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

/* Cipher 35 */
    {
     1,
     TLS1_TXT_RSA_WITH_AES_256_SHA,
     TLS1_CK_RSA_WITH_AES_256_SHA,
     SSL_kRSA,
     SSL_aRSA,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

/* Cipher 38 */
    {
     1,
     TLS1_TXT_DHE_DSS_WITH_AES_256_SHA,
     TLS1_CK_DHE_DSS_WITH_AES_256_SHA,
     SSL_kDHE,
     SSL_aDSS,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

/* Cipher 39 */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_AES_256_SHA,
     TLS1_CK_DHE_RSA_WITH_AES_256_SHA,
     SSL_kDHE,
     SSL_aRSA,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher 3A */
    {
     1,
     TLS1_TXT_ADH_WITH_AES_256_SHA,
     TLS1_CK_ADH_WITH_AES_256_SHA,
     SSL_kDHE,
     SSL_aNULL,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* TLS v1.2 ciphersuites */
    /* Cipher 3B */
    {
     1,
     TLS1_TXT_RSA_WITH_NULL_SHA256,
     TLS1_CK_RSA_WITH_NULL_SHA256,
     SSL_kRSA,
     SSL_aRSA,
     SSL_eNULL,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },

    /* Cipher 3C */
    {
     1,
     TLS1_TXT_RSA_WITH_AES_128_SHA256,
     TLS1_CK_RSA_WITH_AES_128_SHA256,
     SSL_kRSA,
     SSL_aRSA,
     SSL_AES128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 3D */
    {
     1,
     TLS1_TXT_RSA_WITH_AES_256_SHA256,
     TLS1_CK_RSA_WITH_AES_256_SHA256,
     SSL_kRSA,
     SSL_aRSA,
     SSL_AES256,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher 40 */
    {
     1,
     TLS1_TXT_DHE_DSS_WITH_AES_128_SHA256,
     TLS1_CK_DHE_DSS_WITH_AES_128_SHA256,
     SSL_kDHE,
     SSL_aDSS,
     SSL_AES128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

#ifndef OPENSSL_NO_CAMELLIA
    /* Camellia ciphersuites from RFC4132 (128-bit portion) */

    /* Cipher 41 */
    {
     1,
     TLS1_TXT_RSA_WITH_CAMELLIA_128_CBC_SHA,
     TLS1_CK_RSA_WITH_CAMELLIA_128_CBC_SHA,
     SSL_kRSA,
     SSL_aRSA,
     SSL_CAMELLIA128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 44 */
    {
     1,
     TLS1_TXT_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA,
     TLS1_CK_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA,
     SSL_kDHE,
     SSL_aDSS,
     SSL_CAMELLIA128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 45 */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA,
     TLS1_CK_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA,
     SSL_kDHE,
     SSL_aRSA,
     SSL_CAMELLIA128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 46 */
    {
     1,
     TLS1_TXT_ADH_WITH_CAMELLIA_128_CBC_SHA,
     TLS1_CK_ADH_WITH_CAMELLIA_128_CBC_SHA,
     SSL_kDHE,
     SSL_aNULL,
     SSL_CAMELLIA128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },
#endif                          /* OPENSSL_NO_CAMELLIA */

    /* TLS v1.2 ciphersuites */
    /* Cipher 67 */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_AES_128_SHA256,
     TLS1_CK_DHE_RSA_WITH_AES_128_SHA256,
     SSL_kDHE,
     SSL_aRSA,
     SSL_AES128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 6A */
    {
     1,
     TLS1_TXT_DHE_DSS_WITH_AES_256_SHA256,
     TLS1_CK_DHE_DSS_WITH_AES_256_SHA256,
     SSL_kDHE,
     SSL_aDSS,
     SSL_AES256,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher 6B */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_AES_256_SHA256,
     TLS1_CK_DHE_RSA_WITH_AES_256_SHA256,
     SSL_kDHE,
     SSL_aRSA,
     SSL_AES256,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher 6C */
    {
     1,
     TLS1_TXT_ADH_WITH_AES_128_SHA256,
     TLS1_CK_ADH_WITH_AES_128_SHA256,
     SSL_kDHE,
     SSL_aNULL,
     SSL_AES128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_NOT_DEFAULT | SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 6D */
    {
     1,
     TLS1_TXT_ADH_WITH_AES_256_SHA256,
     TLS1_CK_ADH_WITH_AES_256_SHA256,
     SSL_kDHE,
     SSL_aNULL,
     SSL_AES256,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_NOT_DEFAULT | SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* GOST Ciphersuites */
#ifndef OPENSL_NO_GOST
    {
     1,
     "GOST2001-GOST89-GOST89",
     0x3000081,
     SSL_kGOST,
     SSL_aGOST01,
     SSL_eGOST2814789CNT,
     SSL_GOST89MAC,
     SSL_TLSV1,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_GOST94 | TLS1_PRF_GOST94 | TLS1_STREAM_MAC,
     256,
     256
    },
    {
     1,
     "GOST2001-NULL-GOST94",
     0x3000083,
     SSL_kGOST,
     SSL_aGOST01,
     SSL_eNULL,
     SSL_GOST94,
     SSL_TLSV1,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE,
     SSL_HANDSHAKE_MAC_GOST94 | TLS1_PRF_GOST94,
     0,
     0
    },
#endif
#ifndef OPENSSL_NO_CAMELLIA
    /* Camellia ciphersuites from RFC4132 (256-bit portion) */

    /* Cipher 84 */
    {
     1,
     TLS1_TXT_RSA_WITH_CAMELLIA_256_CBC_SHA,
     TLS1_CK_RSA_WITH_CAMELLIA_256_CBC_SHA,
     SSL_kRSA,
     SSL_aRSA,
     SSL_CAMELLIA256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher 87 */
    {
     1,
     TLS1_TXT_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA,
     TLS1_CK_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA,
     SSL_kDHE,
     SSL_aDSS,
     SSL_CAMELLIA256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher 88 */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA,
     TLS1_CK_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA,
     SSL_kDHE,
     SSL_aRSA,
     SSL_CAMELLIA256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher 89 */
    {
     1,
     TLS1_TXT_ADH_WITH_CAMELLIA_256_CBC_SHA,
     TLS1_CK_ADH_WITH_CAMELLIA_256_CBC_SHA,
     SSL_kDHE,
     SSL_aNULL,
     SSL_CAMELLIA256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },
#endif                          /* OPENSSL_NO_CAMELLIA */

#ifndef OPENSSL_NO_PSK
    /* PSK ciphersuites from RFC 4279 */
    /* Cipher 8A */
    {
     1,
     TLS1_TXT_PSK_WITH_RC4_128_SHA,
     TLS1_CK_PSK_WITH_RC4_128_SHA,
     SSL_kPSK,
     SSL_aPSK,
     SSL_RC4,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 8B */
    {
     1,
     TLS1_TXT_PSK_WITH_3DES_EDE_CBC_SHA,
     TLS1_CK_PSK_WITH_3DES_EDE_CBC_SHA,
     SSL_kPSK,
     SSL_aPSK,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

    /* Cipher 8C */
    {
     1,
     TLS1_TXT_PSK_WITH_AES_128_CBC_SHA,
     TLS1_CK_PSK_WITH_AES_128_CBC_SHA,
     SSL_kPSK,
     SSL_aPSK,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 8D */
    {
     1,
     TLS1_TXT_PSK_WITH_AES_256_CBC_SHA,
     TLS1_CK_PSK_WITH_AES_256_CBC_SHA,
     SSL_kPSK,
     SSL_aPSK,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher 8E */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_RC4_128_SHA,
     TLS1_CK_DHE_PSK_WITH_RC4_128_SHA,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_RC4,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 8F */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_3DES_EDE_CBC_SHA,
     TLS1_CK_DHE_PSK_WITH_3DES_EDE_CBC_SHA,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

    /* Cipher 90 */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_AES_128_CBC_SHA,
     TLS1_CK_DHE_PSK_WITH_AES_128_CBC_SHA,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 91 */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_AES_256_CBC_SHA,
     TLS1_CK_DHE_PSK_WITH_AES_256_CBC_SHA,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher 92 */
    {
     1,
     TLS1_TXT_RSA_PSK_WITH_RC4_128_SHA,
     TLS1_CK_RSA_PSK_WITH_RC4_128_SHA,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_RC4,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 93 */
    {
     1,
     TLS1_TXT_RSA_PSK_WITH_3DES_EDE_CBC_SHA,
     TLS1_CK_RSA_PSK_WITH_3DES_EDE_CBC_SHA,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

    /* Cipher 94 */
    {
     1,
     TLS1_TXT_RSA_PSK_WITH_AES_128_CBC_SHA,
     TLS1_CK_RSA_PSK_WITH_AES_128_CBC_SHA,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 95 */
    {
     1,
     TLS1_TXT_RSA_PSK_WITH_AES_256_CBC_SHA,
     TLS1_CK_RSA_PSK_WITH_AES_256_CBC_SHA,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },
#endif                          /* OPENSSL_NO_PSK */

#ifndef OPENSSL_NO_SEED
    /* SEED ciphersuites from RFC4162 */

    /* Cipher 96 */
    {
     1,
     TLS1_TXT_RSA_WITH_SEED_SHA,
     TLS1_CK_RSA_WITH_SEED_SHA,
     SSL_kRSA,
     SSL_aRSA,
     SSL_SEED,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 99 */
    {
     1,
     TLS1_TXT_DHE_DSS_WITH_SEED_SHA,
     TLS1_CK_DHE_DSS_WITH_SEED_SHA,
     SSL_kDHE,
     SSL_aDSS,
     SSL_SEED,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 9A */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_SEED_SHA,
     TLS1_CK_DHE_RSA_WITH_SEED_SHA,
     SSL_kDHE,
     SSL_aRSA,
     SSL_SEED,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher 9B */
    {
     1,
     TLS1_TXT_ADH_WITH_SEED_SHA,
     TLS1_CK_ADH_WITH_SEED_SHA,
     SSL_kDHE,
     SSL_aNULL,
     SSL_SEED,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

#endif                          /* OPENSSL_NO_SEED */

    /* GCM ciphersuites from RFC5288 */

    /* Cipher 9C */
    {
     1,
     TLS1_TXT_RSA_WITH_AES_128_GCM_SHA256,
     TLS1_CK_RSA_WITH_AES_128_GCM_SHA256,
     SSL_kRSA,
     SSL_aRSA,
     SSL_AES128GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher 9D */
    {
     1,
     TLS1_TXT_RSA_WITH_AES_256_GCM_SHA384,
     TLS1_CK_RSA_WITH_AES_256_GCM_SHA384,
     SSL_kRSA,
     SSL_aRSA,
     SSL_AES256GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher 9E */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_AES_128_GCM_SHA256,
     TLS1_CK_DHE_RSA_WITH_AES_128_GCM_SHA256,
     SSL_kDHE,
     SSL_aRSA,
     SSL_AES128GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher 9F */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_AES_256_GCM_SHA384,
     TLS1_CK_DHE_RSA_WITH_AES_256_GCM_SHA384,
     SSL_kDHE,
     SSL_aRSA,
     SSL_AES256GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher A2 */
    {
     1,
     TLS1_TXT_DHE_DSS_WITH_AES_128_GCM_SHA256,
     TLS1_CK_DHE_DSS_WITH_AES_128_GCM_SHA256,
     SSL_kDHE,
     SSL_aDSS,
     SSL_AES128GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher A3 */
    {
     1,
     TLS1_TXT_DHE_DSS_WITH_AES_256_GCM_SHA384,
     TLS1_CK_DHE_DSS_WITH_AES_256_GCM_SHA384,
     SSL_kDHE,
     SSL_aDSS,
     SSL_AES256GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher A6 */
    {
     1,
     TLS1_TXT_ADH_WITH_AES_128_GCM_SHA256,
     TLS1_CK_ADH_WITH_AES_128_GCM_SHA256,
     SSL_kDHE,
     SSL_aNULL,
     SSL_AES128GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_NOT_DEFAULT | SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher A7 */
    {
     1,
     TLS1_TXT_ADH_WITH_AES_256_GCM_SHA384,
     TLS1_CK_ADH_WITH_AES_256_GCM_SHA384,
     SSL_kDHE,
     SSL_aNULL,
     SSL_AES256GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_NOT_DEFAULT | SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },
#ifndef OPENSSL_NO_PSK
    /* PSK ciphersuites from RFC5487 */

    /* Cipher A8 */
    {
     1,
     TLS1_TXT_PSK_WITH_AES_128_GCM_SHA256,
     TLS1_CK_PSK_WITH_AES_128_GCM_SHA256,
     SSL_kPSK,
     SSL_aPSK,
     SSL_AES128GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher A9 */
    {
     1,
     TLS1_TXT_PSK_WITH_AES_256_GCM_SHA384,
     TLS1_CK_PSK_WITH_AES_256_GCM_SHA384,
     SSL_kPSK,
     SSL_aPSK,
     SSL_AES256GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher AA */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_AES_128_GCM_SHA256,
     TLS1_CK_DHE_PSK_WITH_AES_128_GCM_SHA256,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_AES128GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher AB */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_AES_256_GCM_SHA384,
     TLS1_CK_DHE_PSK_WITH_AES_256_GCM_SHA384,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_AES256GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher AC */
    {
     1,
     TLS1_TXT_RSA_PSK_WITH_AES_128_GCM_SHA256,
     TLS1_CK_RSA_PSK_WITH_AES_128_GCM_SHA256,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_AES128GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher AD */
    {
     1,
     TLS1_TXT_RSA_PSK_WITH_AES_256_GCM_SHA384,
     TLS1_CK_RSA_PSK_WITH_AES_256_GCM_SHA384,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_AES256GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher AE */
    {
     1,
     TLS1_TXT_PSK_WITH_AES_128_CBC_SHA256,
     TLS1_CK_PSK_WITH_AES_128_CBC_SHA256,
     SSL_kPSK,
     SSL_aPSK,
     SSL_AES128,
     SSL_SHA256,
     SSL_TLSV1,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher AF */
    {
     1,
     TLS1_TXT_PSK_WITH_AES_256_CBC_SHA384,
     TLS1_CK_PSK_WITH_AES_256_CBC_SHA384,
     SSL_kPSK,
     SSL_aPSK,
     SSL_AES256,
     SSL_SHA384,
     SSL_TLSV1,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher B0 */
    {
     1,
     TLS1_TXT_PSK_WITH_NULL_SHA256,
     TLS1_CK_PSK_WITH_NULL_SHA256,
     SSL_kPSK,
     SSL_aPSK,
     SSL_eNULL,
     SSL_SHA256,
     SSL_TLSV1,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },

    /* Cipher B1 */
    {
     1,
     TLS1_TXT_PSK_WITH_NULL_SHA384,
     TLS1_CK_PSK_WITH_NULL_SHA384,
     SSL_kPSK,
     SSL_aPSK,
     SSL_eNULL,
     SSL_SHA384,
     SSL_TLSV1,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     0,
     0,
     },

    /* Cipher B2 */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_AES_128_CBC_SHA256,
     TLS1_CK_DHE_PSK_WITH_AES_128_CBC_SHA256,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_AES128,
     SSL_SHA256,
     SSL_TLSV1,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher B3 */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_AES_256_CBC_SHA384,
     TLS1_CK_DHE_PSK_WITH_AES_256_CBC_SHA384,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_AES256,
     SSL_SHA384,
     SSL_TLSV1,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher B4 */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_NULL_SHA256,
     TLS1_CK_DHE_PSK_WITH_NULL_SHA256,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_eNULL,
     SSL_SHA256,
     SSL_TLSV1,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },

    /* Cipher B5 */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_NULL_SHA384,
     TLS1_CK_DHE_PSK_WITH_NULL_SHA384,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_eNULL,
     SSL_SHA384,
     SSL_TLSV1,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     0,
     0,
     },

    /* Cipher B6 */
    {
     1,
     TLS1_TXT_RSA_PSK_WITH_AES_128_CBC_SHA256,
     TLS1_CK_RSA_PSK_WITH_AES_128_CBC_SHA256,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_AES128,
     SSL_SHA256,
     SSL_TLSV1,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher B7 */
    {
     1,
     TLS1_TXT_RSA_PSK_WITH_AES_256_CBC_SHA384,
     TLS1_CK_RSA_PSK_WITH_AES_256_CBC_SHA384,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_AES256,
     SSL_SHA384,
     SSL_TLSV1,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher B8 */
    {
     1,
     TLS1_TXT_RSA_PSK_WITH_NULL_SHA256,
     TLS1_CK_RSA_PSK_WITH_NULL_SHA256,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_eNULL,
     SSL_SHA256,
     SSL_TLSV1,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },

    /* Cipher B9 */
    {
     1,
     TLS1_TXT_RSA_PSK_WITH_NULL_SHA384,
     TLS1_CK_RSA_PSK_WITH_NULL_SHA384,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_eNULL,
     SSL_SHA384,
     SSL_TLSV1,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     0,
     0,
     },
#endif                          /* OPENSSL_NO_PSK */

#ifndef OPENSSL_NO_CAMELLIA
    /* TLS 1.2 Camellia SHA-256 ciphersuites from RFC5932 */

    /* Cipher BA */
    {
     1,
     TLS1_TXT_RSA_WITH_CAMELLIA_128_CBC_SHA256,
     TLS1_CK_RSA_WITH_CAMELLIA_128_CBC_SHA256,
     SSL_kRSA,
     SSL_aRSA,
     SSL_CAMELLIA128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher BD */
    {
     1,
     TLS1_TXT_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA256,
     TLS1_CK_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA256,
     SSL_kEDH,
     SSL_aDSS,
     SSL_CAMELLIA128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher BE */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256,
     TLS1_CK_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256,
     SSL_kEDH,
     SSL_aRSA,
     SSL_CAMELLIA128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher BF */
    {
     1,
     TLS1_TXT_ADH_WITH_CAMELLIA_128_CBC_SHA256,
     TLS1_CK_ADH_WITH_CAMELLIA_128_CBC_SHA256,
     SSL_kEDH,
     SSL_aNULL,
     SSL_CAMELLIA128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_NOT_DEFAULT | SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C0 */
    {
     1,
     TLS1_TXT_RSA_WITH_CAMELLIA_256_CBC_SHA256,
     TLS1_CK_RSA_WITH_CAMELLIA_256_CBC_SHA256,
     SSL_kRSA,
     SSL_aRSA,
     SSL_CAMELLIA256,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },

    /* Cipher C3 */
    {
     1,
     TLS1_TXT_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA256,
     TLS1_CK_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA256,
     SSL_kEDH,
     SSL_aDSS,
     SSL_CAMELLIA256,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },

    /* Cipher C4 */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256,
     TLS1_CK_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256,
     SSL_kEDH,
     SSL_aRSA,
     SSL_CAMELLIA256,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },

    /* Cipher C5 */
    {
     1,
     TLS1_TXT_ADH_WITH_CAMELLIA_256_CBC_SHA256,
     TLS1_CK_ADH_WITH_CAMELLIA_256_CBC_SHA256,
     SSL_kEDH,
     SSL_aNULL,
     SSL_CAMELLIA256,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_NOT_DEFAULT | SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },
#endif

#ifdef OPENSSL_SSL_DEBUG_BROKEN_PROTOCOL
    /* Cipher FF */
    {
     1,
     "SCSV",
     SSL3_CK_SCSV,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0,
     0},
#endif

#ifndef OPENSSL_NO_EC
    /* Cipher C001 */
    {
     1,
     TLS1_TXT_ECDH_ECDSA_WITH_NULL_SHA,
     TLS1_CK_ECDH_ECDSA_WITH_NULL_SHA,
     SSL_kECDHe,
     SSL_aECDH,
     SSL_eNULL,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },

    /* Cipher C002 */
    {
     1,
     TLS1_TXT_ECDH_ECDSA_WITH_RC4_128_SHA,
     TLS1_CK_ECDH_ECDSA_WITH_RC4_128_SHA,
     SSL_kECDHe,
     SSL_aECDH,
     SSL_RC4,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C003 */
    {
     1,
     TLS1_TXT_ECDH_ECDSA_WITH_DES_192_CBC3_SHA,
     TLS1_CK_ECDH_ECDSA_WITH_DES_192_CBC3_SHA,
     SSL_kECDHe,
     SSL_aECDH,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

    /* Cipher C004 */
    {
     1,
     TLS1_TXT_ECDH_ECDSA_WITH_AES_128_CBC_SHA,
     TLS1_CK_ECDH_ECDSA_WITH_AES_128_CBC_SHA,
     SSL_kECDHe,
     SSL_aECDH,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C005 */
    {
     1,
     TLS1_TXT_ECDH_ECDSA_WITH_AES_256_CBC_SHA,
     TLS1_CK_ECDH_ECDSA_WITH_AES_256_CBC_SHA,
     SSL_kECDHe,
     SSL_aECDH,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher C006 */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_NULL_SHA,
     TLS1_CK_ECDHE_ECDSA_WITH_NULL_SHA,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_eNULL,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },

    /* Cipher C007 */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_RC4_128_SHA,
     TLS1_CK_ECDHE_ECDSA_WITH_RC4_128_SHA,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_RC4,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C008 */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_DES_192_CBC3_SHA,
     TLS1_CK_ECDHE_ECDSA_WITH_DES_192_CBC3_SHA,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

    /* Cipher C009 */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,
     TLS1_CK_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C00A */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,
     TLS1_CK_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher C00B */
    {
     1,
     TLS1_TXT_ECDH_RSA_WITH_NULL_SHA,
     TLS1_CK_ECDH_RSA_WITH_NULL_SHA,
     SSL_kECDHr,
     SSL_aECDH,
     SSL_eNULL,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },

    /* Cipher C00C */
    {
     1,
     TLS1_TXT_ECDH_RSA_WITH_RC4_128_SHA,
     TLS1_CK_ECDH_RSA_WITH_RC4_128_SHA,
     SSL_kECDHr,
     SSL_aECDH,
     SSL_RC4,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C00D */
    {
     1,
     TLS1_TXT_ECDH_RSA_WITH_DES_192_CBC3_SHA,
     TLS1_CK_ECDH_RSA_WITH_DES_192_CBC3_SHA,
     SSL_kECDHr,
     SSL_aECDH,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

    /* Cipher C00E */
    {
     1,
     TLS1_TXT_ECDH_RSA_WITH_AES_128_CBC_SHA,
     TLS1_CK_ECDH_RSA_WITH_AES_128_CBC_SHA,
     SSL_kECDHr,
     SSL_aECDH,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C00F */
    {
     1,
     TLS1_TXT_ECDH_RSA_WITH_AES_256_CBC_SHA,
     TLS1_CK_ECDH_RSA_WITH_AES_256_CBC_SHA,
     SSL_kECDHr,
     SSL_aECDH,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher C010 */
    {
     1,
     TLS1_TXT_ECDHE_RSA_WITH_NULL_SHA,
     TLS1_CK_ECDHE_RSA_WITH_NULL_SHA,
     SSL_kECDHE,
     SSL_aRSA,
     SSL_eNULL,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },

    /* Cipher C011 */
    {
     1,
     TLS1_TXT_ECDHE_RSA_WITH_RC4_128_SHA,
     TLS1_CK_ECDHE_RSA_WITH_RC4_128_SHA,
     SSL_kECDHE,
     SSL_aRSA,
     SSL_RC4,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C012 */
    {
     1,
     TLS1_TXT_ECDHE_RSA_WITH_DES_192_CBC3_SHA,
     TLS1_CK_ECDHE_RSA_WITH_DES_192_CBC3_SHA,
     SSL_kECDHE,
     SSL_aRSA,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

    /* Cipher C013 */
    {
     1,
     TLS1_TXT_ECDHE_RSA_WITH_AES_128_CBC_SHA,
     TLS1_CK_ECDHE_RSA_WITH_AES_128_CBC_SHA,
     SSL_kECDHE,
     SSL_aRSA,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C014 */
    {
     1,
     TLS1_TXT_ECDHE_RSA_WITH_AES_256_CBC_SHA,
     TLS1_CK_ECDHE_RSA_WITH_AES_256_CBC_SHA,
     SSL_kECDHE,
     SSL_aRSA,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher C015 */
    {
     1,
     TLS1_TXT_ECDH_anon_WITH_NULL_SHA,
     TLS1_CK_ECDH_anon_WITH_NULL_SHA,
     SSL_kECDHE,
     SSL_aNULL,
     SSL_eNULL,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },

    /* Cipher C016 */
    {
     1,
     TLS1_TXT_ECDH_anon_WITH_RC4_128_SHA,
     TLS1_CK_ECDH_anon_WITH_RC4_128_SHA,
     SSL_kECDHE,
     SSL_aNULL,
     SSL_RC4,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C017 */
    {
     1,
     TLS1_TXT_ECDH_anon_WITH_DES_192_CBC3_SHA,
     TLS1_CK_ECDH_anon_WITH_DES_192_CBC3_SHA,
     SSL_kECDHE,
     SSL_aNULL,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

    /* Cipher C018 */
    {
     1,
     TLS1_TXT_ECDH_anon_WITH_AES_128_CBC_SHA,
     TLS1_CK_ECDH_anon_WITH_AES_128_CBC_SHA,
     SSL_kECDHE,
     SSL_aNULL,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C019 */
    {
     1,
     TLS1_TXT_ECDH_anon_WITH_AES_256_CBC_SHA,
     TLS1_CK_ECDH_anon_WITH_AES_256_CBC_SHA,
     SSL_kECDHE,
     SSL_aNULL,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },
#endif                          /* OPENSSL_NO_EC */

#ifndef OPENSSL_NO_SRP
    /* Cipher C01A */
    {
     1,
     TLS1_TXT_SRP_SHA_WITH_3DES_EDE_CBC_SHA,
     TLS1_CK_SRP_SHA_WITH_3DES_EDE_CBC_SHA,
     SSL_kSRP,
     SSL_aSRP,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

    /* Cipher C01B */
    {
     1,
     TLS1_TXT_SRP_SHA_RSA_WITH_3DES_EDE_CBC_SHA,
     TLS1_CK_SRP_SHA_RSA_WITH_3DES_EDE_CBC_SHA,
     SSL_kSRP,
     SSL_aRSA,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

    /* Cipher C01C */
    {
     1,
     TLS1_TXT_SRP_SHA_DSS_WITH_3DES_EDE_CBC_SHA,
     TLS1_CK_SRP_SHA_DSS_WITH_3DES_EDE_CBC_SHA,
     SSL_kSRP,
     SSL_aDSS,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

    /* Cipher C01D */
    {
     1,
     TLS1_TXT_SRP_SHA_WITH_AES_128_CBC_SHA,
     TLS1_CK_SRP_SHA_WITH_AES_128_CBC_SHA,
     SSL_kSRP,
     SSL_aSRP,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C01E */
    {
     1,
     TLS1_TXT_SRP_SHA_RSA_WITH_AES_128_CBC_SHA,
     TLS1_CK_SRP_SHA_RSA_WITH_AES_128_CBC_SHA,
     SSL_kSRP,
     SSL_aRSA,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C01F */
    {
     1,
     TLS1_TXT_SRP_SHA_DSS_WITH_AES_128_CBC_SHA,
     TLS1_CK_SRP_SHA_DSS_WITH_AES_128_CBC_SHA,
     SSL_kSRP,
     SSL_aDSS,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C020 */
    {
     1,
     TLS1_TXT_SRP_SHA_WITH_AES_256_CBC_SHA,
     TLS1_CK_SRP_SHA_WITH_AES_256_CBC_SHA,
     SSL_kSRP,
     SSL_aSRP,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher C021 */
    {
     1,
     TLS1_TXT_SRP_SHA_RSA_WITH_AES_256_CBC_SHA,
     TLS1_CK_SRP_SHA_RSA_WITH_AES_256_CBC_SHA,
     SSL_kSRP,
     SSL_aRSA,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher C022 */
    {
     1,
     TLS1_TXT_SRP_SHA_DSS_WITH_AES_256_CBC_SHA,
     TLS1_CK_SRP_SHA_DSS_WITH_AES_256_CBC_SHA,
     SSL_kSRP,
     SSL_aDSS,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },
#endif                          /* OPENSSL_NO_SRP */
#ifndef OPENSSL_NO_EC

    /* HMAC based TLS v1.2 ciphersuites from RFC5289 */

    /* Cipher C023 */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_AES_128_SHA256,
     TLS1_CK_ECDHE_ECDSA_WITH_AES_128_SHA256,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_AES128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C024 */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_AES_256_SHA384,
     TLS1_CK_ECDHE_ECDSA_WITH_AES_256_SHA384,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_AES256,
     SSL_SHA384,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher C025 */
    {
     1,
     TLS1_TXT_ECDH_ECDSA_WITH_AES_128_SHA256,
     TLS1_CK_ECDH_ECDSA_WITH_AES_128_SHA256,
     SSL_kECDHe,
     SSL_aECDH,
     SSL_AES128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C026 */
    {
     1,
     TLS1_TXT_ECDH_ECDSA_WITH_AES_256_SHA384,
     TLS1_CK_ECDH_ECDSA_WITH_AES_256_SHA384,
     SSL_kECDHe,
     SSL_aECDH,
     SSL_AES256,
     SSL_SHA384,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher C027 */
    {
     1,
     TLS1_TXT_ECDHE_RSA_WITH_AES_128_SHA256,
     TLS1_CK_ECDHE_RSA_WITH_AES_128_SHA256,
     SSL_kECDHE,
     SSL_aRSA,
     SSL_AES128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C028 */
    {
     1,
     TLS1_TXT_ECDHE_RSA_WITH_AES_256_SHA384,
     TLS1_CK_ECDHE_RSA_WITH_AES_256_SHA384,
     SSL_kECDHE,
     SSL_aRSA,
     SSL_AES256,
     SSL_SHA384,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher C029 */
    {
     1,
     TLS1_TXT_ECDH_RSA_WITH_AES_128_SHA256,
     TLS1_CK_ECDH_RSA_WITH_AES_128_SHA256,
     SSL_kECDHr,
     SSL_aECDH,
     SSL_AES128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C02A */
    {
     1,
     TLS1_TXT_ECDH_RSA_WITH_AES_256_SHA384,
     TLS1_CK_ECDH_RSA_WITH_AES_256_SHA384,
     SSL_kECDHr,
     SSL_aECDH,
     SSL_AES256,
     SSL_SHA384,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* GCM based TLS v1.2 ciphersuites from RFC5289 */

    /* Cipher C02B */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
     TLS1_CK_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_AES128GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C02C */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
     TLS1_CK_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_AES256GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher C02D */
    {
     1,
     TLS1_TXT_ECDH_ECDSA_WITH_AES_128_GCM_SHA256,
     TLS1_CK_ECDH_ECDSA_WITH_AES_128_GCM_SHA256,
     SSL_kECDHe,
     SSL_aECDH,
     SSL_AES128GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C02E */
    {
     1,
     TLS1_TXT_ECDH_ECDSA_WITH_AES_256_GCM_SHA384,
     TLS1_CK_ECDH_ECDSA_WITH_AES_256_GCM_SHA384,
     SSL_kECDHe,
     SSL_aECDH,
     SSL_AES256GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher C02F */
    {
     1,
     TLS1_TXT_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
     TLS1_CK_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
     SSL_kECDHE,
     SSL_aRSA,
     SSL_AES128GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C030 */
    {
     1,
     TLS1_TXT_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
     TLS1_CK_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
     SSL_kECDHE,
     SSL_aRSA,
     SSL_AES256GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher C031 */
    {
     1,
     TLS1_TXT_ECDH_RSA_WITH_AES_128_GCM_SHA256,
     TLS1_CK_ECDH_RSA_WITH_AES_128_GCM_SHA256,
     SSL_kECDHr,
     SSL_aECDH,
     SSL_AES128GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C032 */
    {
     1,
     TLS1_TXT_ECDH_RSA_WITH_AES_256_GCM_SHA384,
     TLS1_CK_ECDH_RSA_WITH_AES_256_GCM_SHA384,
     SSL_kECDHr,
     SSL_aECDH,
     SSL_AES256GCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* PSK ciphersuites from RFC 5489 */
    /* Cipher C033 */
    {
     1,
     TLS1_TXT_ECDHE_PSK_WITH_RC4_128_SHA,
     TLS1_CK_ECDHE_PSK_WITH_RC4_128_SHA,
     SSL_kECDHEPSK,
     SSL_aPSK,
     SSL_RC4,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_MEDIUM,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C034 */
    {
     1,
     TLS1_TXT_ECDHE_PSK_WITH_3DES_EDE_CBC_SHA,
     TLS1_CK_ECDHE_PSK_WITH_3DES_EDE_CBC_SHA,
     SSL_kECDHEPSK,
     SSL_aPSK,
     SSL_3DES,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     112,
     168,
     },

    /* Cipher C035 */
    {
     1,
     TLS1_TXT_ECDHE_PSK_WITH_AES_128_CBC_SHA,
     TLS1_CK_ECDHE_PSK_WITH_AES_128_CBC_SHA,
     SSL_kECDHEPSK,
     SSL_aPSK,
     SSL_AES128,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C036 */
    {
     1,
     TLS1_TXT_ECDHE_PSK_WITH_AES_256_CBC_SHA,
     TLS1_CK_ECDHE_PSK_WITH_AES_256_CBC_SHA,
     SSL_kECDHEPSK,
     SSL_aPSK,
     SSL_AES256,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     256,
     256,
     },

    /* Cipher C037 */
    {
     1,
     TLS1_TXT_ECDHE_PSK_WITH_AES_128_CBC_SHA256,
     TLS1_CK_ECDHE_PSK_WITH_AES_128_CBC_SHA256,
     SSL_kECDHEPSK,
     SSL_aPSK,
     SSL_AES128,
     SSL_SHA256,
     SSL_TLSV1,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128,
     },

    /* Cipher C038 */
    {
     1,
     TLS1_TXT_ECDHE_PSK_WITH_AES_256_CBC_SHA384,
     TLS1_CK_ECDHE_PSK_WITH_AES_256_CBC_SHA384,
     SSL_kECDHEPSK,
     SSL_aPSK,
     SSL_AES256,
     SSL_SHA384,
     SSL_TLSV1,
     SSL_HIGH | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256,
     },

    /* Cipher C039 */
    {
     1,
     TLS1_TXT_ECDHE_PSK_WITH_NULL_SHA,
     TLS1_CK_ECDHE_PSK_WITH_NULL_SHA,
     SSL_kECDHEPSK,
     SSL_aPSK,
     SSL_eNULL,
     SSL_SHA1,
     SSL_SSLV3,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },

    /* Cipher C03A */
    {
     1,
     TLS1_TXT_ECDHE_PSK_WITH_NULL_SHA256,
     TLS1_CK_ECDHE_PSK_WITH_NULL_SHA256,
     SSL_kECDHEPSK,
     SSL_aPSK,
     SSL_eNULL,
     SSL_SHA256,
     SSL_TLSV1,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     0,
     0,
     },

    /* Cipher C03B */
    {
     1,
     TLS1_TXT_ECDHE_PSK_WITH_NULL_SHA384,
     TLS1_CK_ECDHE_PSK_WITH_NULL_SHA384,
     SSL_kECDHEPSK,
     SSL_aPSK,
     SSL_eNULL,
     SSL_SHA384,
     SSL_TLSV1,
     SSL_NOT_DEFAULT | SSL_STRONG_NONE | SSL_FIPS,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     0,
     0,
     },

# ifndef OPENSSL_NO_CAMELLIA
    {                           /* Cipher C072 */
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_CAMELLIA_128_CBC_SHA256,
     TLS1_CK_ECDHE_ECDSA_WITH_CAMELLIA_128_CBC_SHA256,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_CAMELLIA128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128},

    {                           /* Cipher C073 */
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_CAMELLIA_256_CBC_SHA384,
     TLS1_CK_ECDHE_ECDSA_WITH_CAMELLIA_256_CBC_SHA384,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_CAMELLIA256,
     SSL_SHA384,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256},

    {                           /* Cipher C074 */
     1,
     TLS1_TXT_ECDH_ECDSA_WITH_CAMELLIA_128_CBC_SHA256,
     TLS1_CK_ECDH_ECDSA_WITH_CAMELLIA_128_CBC_SHA256,
     SSL_kECDHe,
     SSL_aECDH,
     SSL_CAMELLIA128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128},

    {                           /* Cipher C075 */
     1,
     TLS1_TXT_ECDH_ECDSA_WITH_CAMELLIA_256_CBC_SHA384,
     TLS1_CK_ECDH_ECDSA_WITH_CAMELLIA_256_CBC_SHA384,
     SSL_kECDHe,
     SSL_aECDH,
     SSL_CAMELLIA256,
     SSL_SHA384,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256},

    {                           /* Cipher C076 */
     1,
     TLS1_TXT_ECDHE_RSA_WITH_CAMELLIA_128_CBC_SHA256,
     TLS1_CK_ECDHE_RSA_WITH_CAMELLIA_128_CBC_SHA256,
     SSL_kECDHE,
     SSL_aRSA,
     SSL_CAMELLIA128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128},

    {                           /* Cipher C077 */
     1,
     TLS1_TXT_ECDHE_RSA_WITH_CAMELLIA_256_CBC_SHA384,
     TLS1_CK_ECDHE_RSA_WITH_CAMELLIA_256_CBC_SHA384,
     SSL_kECDHE,
     SSL_aRSA,
     SSL_CAMELLIA256,
     SSL_SHA384,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256},

    {                           /* Cipher C078 */
     1,
     TLS1_TXT_ECDH_RSA_WITH_CAMELLIA_128_CBC_SHA256,
     TLS1_CK_ECDH_RSA_WITH_CAMELLIA_128_CBC_SHA256,
     SSL_kECDHr,
     SSL_aECDH,
     SSL_CAMELLIA128,
     SSL_SHA256,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128},

    {                           /* Cipher C079 */
     1,
     TLS1_TXT_ECDH_RSA_WITH_CAMELLIA_256_CBC_SHA384,
     TLS1_CK_ECDH_RSA_WITH_CAMELLIA_256_CBC_SHA384,
     SSL_kECDHr,
     SSL_aECDH,
     SSL_CAMELLIA256,
     SSL_SHA384,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256},
# endif                         /* OPENSSL_NO_CAMELLIA */
#endif                          /* OPENSSL_NO_EC */

#if !defined(OPENSSL_NO_CAMELLIA) && !defined(OPENSSL_NO_PSK)
    {                           /* Cipher C094 */
     1,
     TLS1_TXT_PSK_WITH_CAMELLIA_128_CBC_SHA256,
     TLS1_CK_PSK_WITH_CAMELLIA_128_CBC_SHA256,
     SSL_kPSK,
     SSL_aPSK,
     SSL_CAMELLIA128,
     SSL_SHA256,
     SSL_TLSV1,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128},

    {                           /* Cipher C095 */
     1,
     TLS1_TXT_PSK_WITH_CAMELLIA_256_CBC_SHA384,
     TLS1_CK_PSK_WITH_CAMELLIA_256_CBC_SHA384,
     SSL_kPSK,
     SSL_aPSK,
     SSL_CAMELLIA256,
     SSL_SHA384,
     SSL_TLSV1,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256},

    {                           /* Cipher C096 */
     1,
     TLS1_TXT_DHE_PSK_WITH_CAMELLIA_128_CBC_SHA256,
     TLS1_CK_DHE_PSK_WITH_CAMELLIA_128_CBC_SHA256,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_CAMELLIA128,
     SSL_SHA256,
     SSL_TLSV1,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128},

    {                           /* Cipher C097 */
     1,
     TLS1_TXT_DHE_PSK_WITH_CAMELLIA_256_CBC_SHA384,
     TLS1_CK_DHE_PSK_WITH_CAMELLIA_256_CBC_SHA384,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_CAMELLIA256,
     SSL_SHA384,
     SSL_TLSV1,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256},

    {                           /* Cipher C098 */
     1,
     TLS1_TXT_RSA_PSK_WITH_CAMELLIA_128_CBC_SHA256,
     TLS1_CK_RSA_PSK_WITH_CAMELLIA_128_CBC_SHA256,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_CAMELLIA128,
     SSL_SHA256,
     SSL_TLSV1,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128},

    {                           /* Cipher C099 */
     1,
     TLS1_TXT_RSA_PSK_WITH_CAMELLIA_256_CBC_SHA384,
     TLS1_CK_RSA_PSK_WITH_CAMELLIA_256_CBC_SHA384,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_CAMELLIA256,
     SSL_SHA384,
     SSL_TLSV1,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256},

    {                           /* Cipher C09A */
     1,
     TLS1_TXT_ECDHE_PSK_WITH_CAMELLIA_128_CBC_SHA256,
     TLS1_CK_ECDHE_PSK_WITH_CAMELLIA_128_CBC_SHA256,
     SSL_kECDHEPSK,
     SSL_aPSK,
     SSL_CAMELLIA128,
     SSL_SHA256,
     SSL_TLSV1,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF,
     128,
     128},

    {                           /* Cipher C09B */
     1,
     TLS1_TXT_ECDHE_PSK_WITH_CAMELLIA_256_CBC_SHA384,
     TLS1_CK_ECDHE_PSK_WITH_CAMELLIA_256_CBC_SHA384,
     SSL_kECDHEPSK,
     SSL_aPSK,
     SSL_CAMELLIA256,
     SSL_SHA384,
     SSL_TLSV1,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384,
     256,
     256},
#endif

    /* Cipher C09C */
    {
     1,
     TLS1_TXT_RSA_WITH_AES_128_CCM,
     TLS1_CK_RSA_WITH_AES_128_CCM,
     SSL_kRSA,
     SSL_aRSA,
     SSL_AES128CCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C09D */
    {
     1,
     TLS1_TXT_RSA_WITH_AES_256_CCM,
     TLS1_CK_RSA_WITH_AES_256_CCM,
     SSL_kRSA,
     SSL_aRSA,
     SSL_AES256CCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },

    /* Cipher C09E */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_AES_128_CCM,
     TLS1_CK_DHE_RSA_WITH_AES_128_CCM,
     SSL_kDHE,
     SSL_aRSA,
     SSL_AES128CCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C09F */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_AES_256_CCM,
     TLS1_CK_DHE_RSA_WITH_AES_256_CCM,
     SSL_kDHE,
     SSL_aRSA,
     SSL_AES256CCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },

    /* Cipher C0A0 */
    {
     1,
     TLS1_TXT_RSA_WITH_AES_128_CCM_8,
     TLS1_CK_RSA_WITH_AES_128_CCM_8,
     SSL_kRSA,
     SSL_aRSA,
     SSL_AES128CCM8,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C0A1 */
    {
     1,
     TLS1_TXT_RSA_WITH_AES_256_CCM_8,
     TLS1_CK_RSA_WITH_AES_256_CCM_8,
     SSL_kRSA,
     SSL_aRSA,
     SSL_AES256CCM8,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },

    /* Cipher C0A2 */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_AES_128_CCM_8,
     TLS1_CK_DHE_RSA_WITH_AES_128_CCM_8,
     SSL_kDHE,
     SSL_aRSA,
     SSL_AES128CCM8,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C0A3 */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_AES_256_CCM_8,
     TLS1_CK_DHE_RSA_WITH_AES_256_CCM_8,
     SSL_kDHE,
     SSL_aRSA,
     SSL_AES256CCM8,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },

    /* Cipher C0A4 */
    {
     1,
     TLS1_TXT_PSK_WITH_AES_128_CCM,
     TLS1_CK_PSK_WITH_AES_128_CCM,
     SSL_kPSK,
     SSL_aPSK,
     SSL_AES128CCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C0A4 */
    {
     1,
     TLS1_TXT_PSK_WITH_AES_256_CCM,
     TLS1_CK_PSK_WITH_AES_256_CCM,
     SSL_kPSK,
     SSL_aPSK,
     SSL_AES256CCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },

    /* Cipher C0A6 */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_AES_128_CCM,
     TLS1_CK_DHE_PSK_WITH_AES_128_CCM,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_AES128CCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C0A7 */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_AES_256_CCM,
     TLS1_CK_DHE_PSK_WITH_AES_256_CCM,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_AES256CCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },

    /* Cipher C0A8 */
    {
     1,
     TLS1_TXT_PSK_WITH_AES_128_CCM_8,
     TLS1_CK_PSK_WITH_AES_128_CCM_8,
     SSL_kPSK,
     SSL_aPSK,
     SSL_AES128CCM8,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C0A9 */
    {
     1,
     TLS1_TXT_PSK_WITH_AES_256_CCM_8,
     TLS1_CK_PSK_WITH_AES_256_CCM_8,
     SSL_kPSK,
     SSL_aPSK,
     SSL_AES256CCM8,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },

    /* Cipher C0AA */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_AES_128_CCM_8,
     TLS1_CK_DHE_PSK_WITH_AES_128_CCM_8,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_AES128CCM8,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C0AB */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_AES_256_CCM_8,
     TLS1_CK_DHE_PSK_WITH_AES_256_CCM_8,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_AES256CCM8,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },

    /* Cipher C0AC */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_AES_128_CCM,
     TLS1_CK_ECDHE_ECDSA_WITH_AES_128_CCM,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_AES128CCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C0AD */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_AES_256_CCM,
     TLS1_CK_ECDHE_ECDSA_WITH_AES_256_CCM,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_AES256CCM,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },

    /* Cipher C0AE */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_AES_128_CCM_8,
     TLS1_CK_ECDHE_ECDSA_WITH_AES_128_CCM_8,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_AES128CCM8,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     128,
     128,
     },

    /* Cipher C0AF */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_AES_256_CCM_8,
     TLS1_CK_ECDHE_ECDSA_WITH_AES_256_CCM_8,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_AES256CCM8,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },
#if !defined(OPENSSL_NO_CHACHA) && !defined(OPENSSL_NO_POLY1305)
# ifndef OPENSSL_NO_EC
    /* Cipher CCA8 as per draft-ietf-tls-chacha20-poly1305-03 */
    {
     1,
     TLS1_TXT_ECDHE_RSA_WITH_CHACHA20_POLY1305,
     TLS1_CK_ECDHE_RSA_WITH_CHACHA20_POLY1305,
     SSL_kECDHE,
     SSL_aRSA,
     SSL_CHACHA20POLY1305,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },
    /* Cipher CCA9 */
    {
     1,
     TLS1_TXT_ECDHE_ECDSA_WITH_CHACHA20_POLY1305,
     TLS1_CK_ECDHE_ECDSA_WITH_CHACHA20_POLY1305,
     SSL_kECDHE,
     SSL_aECDSA,
     SSL_CHACHA20POLY1305,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },
# endif
# ifndef OPENSSL_NO_RSA
    /* Cipher CCAA */
    {
     1,
     TLS1_TXT_DHE_RSA_WITH_CHACHA20_POLY1305,
     TLS1_CK_DHE_RSA_WITH_CHACHA20_POLY1305,
     SSL_kDHE,
     SSL_aRSA,
     SSL_CHACHA20POLY1305,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },
# endif
# ifndef OPENSSL_NO_PSK
    /* Cipher CCAB */
    {
     1,
     TLS1_TXT_PSK_WITH_CHACHA20_POLY1305,
     TLS1_CK_PSK_WITH_CHACHA20_POLY1305,
     SSL_kPSK,
     SSL_aPSK,
     SSL_CHACHA20POLY1305,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },
    /* Cipher CCAC */
    {
     1,
     TLS1_TXT_ECDHE_PSK_WITH_CHACHA20_POLY1305,
     TLS1_CK_ECDHE_PSK_WITH_CHACHA20_POLY1305,
     SSL_kECDHEPSK,
     SSL_aPSK,
     SSL_CHACHA20POLY1305,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },
    /* Cipher CCAD */
    {
     1,
     TLS1_TXT_DHE_PSK_WITH_CHACHA20_POLY1305,
     TLS1_CK_DHE_PSK_WITH_CHACHA20_POLY1305,
     SSL_kDHEPSK,
     SSL_aPSK,
     SSL_CHACHA20POLY1305,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },
    /* Cipher CCAE */
    {
     1,
     TLS1_TXT_RSA_PSK_WITH_CHACHA20_POLY1305,
     TLS1_CK_RSA_PSK_WITH_CHACHA20_POLY1305,
     SSL_kRSAPSK,
     SSL_aRSA,
     SSL_CHACHA20POLY1305,
     SSL_AEAD,
     SSL_TLSV1_2,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256,
     256,
     256,
     },
# endif
#endif
#ifndef OPENSSL_NO_GOST
    {
     1,
     "GOST2012-GOST8912-GOST8912",
     0x0300ff85,
     SSL_kGOST,
     SSL_aGOST12 | SSL_aGOST01,
     SSL_eGOST2814789CNT12,
     SSL_GOST89MAC12,
     SSL_TLSV1,
     SSL_HIGH,
     SSL_HANDSHAKE_MAC_GOST12_256 | TLS1_PRF_GOST12_256 | TLS1_STREAM_MAC,
     256,
     256},
    {
     1,
     "GOST2012-NULL-GOST12",
     0x0300ff87,
     SSL_kGOST,
     SSL_aGOST12 | SSL_aGOST01,
     SSL_eNULL,
     SSL_GOST12_256,
     SSL_TLSV1,
     SSL_STRONG_NONE,
     SSL_HANDSHAKE_MAC_GOST12_256 | TLS1_PRF_GOST12_256 | TLS1_STREAM_MAC,
     0,
     0},
#endif

/* end of list */
};

const SSL3_ENC_METHOD SSLv3_enc_data = {
    ssl3_enc,
    n_ssl3_mac,
    ssl3_setup_key_block,
    ssl3_generate_master_secret,
    ssl3_change_cipher_state,
    ssl3_final_finish_mac,
    MD5_DIGEST_LENGTH + SHA_DIGEST_LENGTH,
    SSL3_MD_CLIENT_FINISHED_CONST, 4,
    SSL3_MD_SERVER_FINISHED_CONST, 4,
    ssl3_alert_code,
    (int (*)(SSL *, unsigned char *, size_t, const char *,
             size_t, const unsigned char *, size_t,
             int use_context))ssl_undefined_function,
    0,
    SSL3_HM_HEADER_LENGTH,
    ssl3_set_handshake_header,
    ssl3_handshake_write
};

long ssl3_default_timeout(void)
{
    /*
     * 2 hours, the 24 hours mentioned in the SSLv3 spec is way too long for
     * http, the cache would over fill
     */
    return (60 * 60 * 2);
}

int ssl3_num_ciphers(void)
{
    return (SSL3_NUM_CIPHERS);
}

const SSL_CIPHER *ssl3_get_cipher(unsigned int u)
{
    if (u < SSL3_NUM_CIPHERS)
        return (&(ssl3_ciphers[SSL3_NUM_CIPHERS - 1 - u]));
    else
        return (NULL);
}

int ssl3_set_handshake_header(SSL *s, int htype, unsigned long len)
{
    unsigned char *p = (unsigned char *)s->init_buf->data;
    *(p++) = htype;
    l2n3(len, p);
    s->init_num = (int)len + SSL3_HM_HEADER_LENGTH;
    s->init_off = 0;

     return 1;
}

int ssl3_handshake_write(SSL *s)
{
    return ssl3_do_write(s, SSL3_RT_HANDSHAKE);
}

int ssl3_new(SSL *s)
{
    SSL3_STATE *s3;

    if ((s3 = OPENSSL_zalloc(sizeof(*s3))) == NULL)
        goto err;
    s->s3 = s3;

#ifndef OPENSSL_NO_SRP
    if (!SSL_SRP_CTX_init(s))
          goto err;
#endif
    s->method->ssl_clear(s);
    return (1);
 err:
    return (0);
}

void ssl3_free(SSL *s)
{
    if (s == NULL || s->s3 == NULL)
        return;

    ssl3_cleanup_key_block(s);

#if !defined(OPENSSL_NO_EC) || !defined(OPENSSL_NO_DH)
    EVP_PKEY_free(s->s3->peer_tmp);
    s->s3->peer_tmp = NULL;
    EVP_PKEY_free(s->s3->tmp.pkey);
    s->s3->tmp.pkey = NULL;
#endif

    sk_X509_NAME_pop_free(s->s3->tmp.ca_names, X509_NAME_free);
    OPENSSL_free(s->s3->tmp.ciphers_raw);
    OPENSSL_clear_free(s->s3->tmp.pms, s->s3->tmp.pmslen);
    OPENSSL_free(s->s3->tmp.peer_sigalgs);
    ssl3_free_digest_list(s);
    OPENSSL_free(s->s3->alpn_selected);

#ifndef OPENSSL_NO_SRP
    SSL_SRP_CTX_free(s);
#endif
    OPENSSL_clear_free(s->s3, sizeof(*s->s3));
    s->s3 = NULL;
}

void ssl3_clear(SSL *s)
{
    ssl3_cleanup_key_block(s);
    sk_X509_NAME_pop_free(s->s3->tmp.ca_names, X509_NAME_free);
    OPENSSL_free(s->s3->tmp.ciphers_raw);
    s->s3->tmp.ciphers_raw = NULL;
    OPENSSL_clear_free(s->s3->tmp.pms, s->s3->tmp.pmslen);
    s->s3->tmp.pms = NULL;
    OPENSSL_free(s->s3->tmp.peer_sigalgs);
    s->s3->tmp.peer_sigalgs = NULL;

#ifndef OPENSSL_NO_EC
    s->s3->is_probably_safari = 0;
#endif
#if !defined(OPENSSL_NO_EC) || !defined(OPENSSL_NO_DH)
    EVP_PKEY_free(s->s3->tmp.pkey);
    s->s3->tmp.pkey = NULL;
    EVP_PKEY_free(s->s3->peer_tmp);
    s->s3->peer_tmp = NULL;
#endif                         /* !OPENSSL_NO_EC */

    ssl3_free_digest_list(s);

    if (s->s3->alpn_selected) {
        OPENSSL_free(s->s3->alpn_selected);
        s->s3->alpn_selected = NULL;
    }

    memset(s->s3, 0, sizeof(*s->s3));

    ssl_free_wbio_buffer(s);

    s->s3->renegotiate = 0;
    s->s3->total_renegotiations = 0;
    s->s3->num_renegotiations = 0;
    s->s3->in_read_app_data = 0;
    s->version = SSL3_VERSION;

#if !defined(OPENSSL_NO_NEXTPROTONEG)
    OPENSSL_free(s->next_proto_negotiated);
    s->next_proto_negotiated = NULL;
    s->next_proto_negotiated_len = 0;
#endif
}

#ifndef OPENSSL_NO_SRP
static char *srp_password_from_info_cb(SSL *s, void *arg)
{
    return OPENSSL_strdup(s->srp_ctx.info);
}
#endif

static int ssl3_set_req_cert_type(CERT *c, const unsigned char *p,
                                  size_t len);

long ssl3_ctrl(SSL *s, int cmd, long larg, void *parg)
{
    int ret = 0;

    switch (cmd) {
    case SSL_CTRL_GET_SESSION_REUSED:
        ret = s->hit;
        break;
    case SSL_CTRL_GET_CLIENT_CERT_REQUEST:
        break;
    case SSL_CTRL_GET_NUM_RENEGOTIATIONS:
        ret = s->s3->num_renegotiations;
        break;
    case SSL_CTRL_CLEAR_NUM_RENEGOTIATIONS:
        ret = s->s3->num_renegotiations;
        s->s3->num_renegotiations = 0;
        break;
    case SSL_CTRL_GET_TOTAL_RENEGOTIATIONS:
        ret = s->s3->total_renegotiations;
        break;
    case SSL_CTRL_GET_FLAGS:
        ret = (int)(s->s3->flags);
        break;
#ifndef OPENSSL_NO_DH
    case SSL_CTRL_SET_TMP_DH:
        {
            DH *dh = (DH *)parg;
            EVP_PKEY *pkdh = NULL;
            if (dh == NULL) {
                SSLerr(SSL_F_SSL3_CTRL, ERR_R_PASSED_NULL_PARAMETER);
                return (ret);
            }
            pkdh = ssl_dh_to_pkey(dh);
            if (pkdh == NULL) {
                SSLerr(SSL_F_SSL3_CTRL, ERR_R_MALLOC_FAILURE);
                return 0;
            }
            if (!ssl_security(s, SSL_SECOP_TMP_DH,
                              EVP_PKEY_security_bits(pkdh), 0, pkdh)) {
                SSLerr(SSL_F_SSL3_CTRL, SSL_R_DH_KEY_TOO_SMALL);
                EVP_PKEY_free(pkdh);
                return ret;
            }
            EVP_PKEY_free(s->cert->dh_tmp);
            s->cert->dh_tmp = pkdh;
            ret = 1;
        }
        break;
    case SSL_CTRL_SET_TMP_DH_CB:
        {
            SSLerr(SSL_F_SSL3_CTRL, ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
            return (ret);
        }
    case SSL_CTRL_SET_DH_AUTO:
        s->cert->dh_tmp_auto = larg;
        return 1;
#endif
#ifndef OPENSSL_NO_EC
    case SSL_CTRL_SET_TMP_ECDH:
        {
            const EC_GROUP *group = NULL;
            int nid;

            if (parg == NULL) {
                SSLerr(SSL_F_SSL3_CTRL, ERR_R_PASSED_NULL_PARAMETER);
                return 0;
            }
            group = EC_KEY_get0_group((const EC_KEY *)parg);
            if (group == NULL) {
                SSLerr(SSL_F_SSL3_CTRL, EC_R_MISSING_PARAMETERS);
                return 0;
            }
            nid = EC_GROUP_get_curve_name(group);
            if (nid == NID_undef)
                return 0;
            return tls1_set_curves(&s->tlsext_ellipticcurvelist,
                                   &s->tlsext_ellipticcurvelist_length,
                                   &nid, 1);
        }
        break;
#endif                          /* !OPENSSL_NO_EC */
    case SSL_CTRL_SET_TLSEXT_HOSTNAME:
        if (larg == TLSEXT_NAMETYPE_host_name) {
            size_t len;

            OPENSSL_free(s->tlsext_hostname);
            s->tlsext_hostname = NULL;

            ret = 1;
            if (parg == NULL)
                break;
            len = strlen((char *)parg);
            if (len == 0 || len > TLSEXT_MAXLEN_host_name) {
                SSLerr(SSL_F_SSL3_CTRL, SSL_R_SSL3_EXT_INVALID_SERVERNAME);
                return 0;
            }
            if ((s->tlsext_hostname = OPENSSL_strdup((char *)parg)) == NULL) {
                SSLerr(SSL_F_SSL3_CTRL, ERR_R_INTERNAL_ERROR);
                return 0;
            }
        } else {
            SSLerr(SSL_F_SSL3_CTRL, SSL_R_SSL3_EXT_INVALID_SERVERNAME_TYPE);
            return 0;
        }
        break;
    case SSL_CTRL_SET_TLSEXT_DEBUG_ARG:
        s->tlsext_debug_arg = parg;
        ret = 1;
        break;

    case SSL_CTRL_SET_TLSEXT_STATUS_REQ_TYPE:
        s->tlsext_status_type = larg;
        ret = 1;
        break;

    case SSL_CTRL_GET_TLSEXT_STATUS_REQ_EXTS:
        *(STACK_OF(X509_EXTENSION) **)parg = s->tlsext_ocsp_exts;
        ret = 1;
        break;

    case SSL_CTRL_SET_TLSEXT_STATUS_REQ_EXTS:
        s->tlsext_ocsp_exts = parg;
        ret = 1;
        break;

    case SSL_CTRL_GET_TLSEXT_STATUS_REQ_IDS:
        *(STACK_OF(OCSP_RESPID) **)parg = s->tlsext_ocsp_ids;
        ret = 1;
        break;

    case SSL_CTRL_SET_TLSEXT_STATUS_REQ_IDS:
        s->tlsext_ocsp_ids = parg;
        ret = 1;
        break;

    case SSL_CTRL_GET_TLSEXT_STATUS_REQ_OCSP_RESP:
        *(unsigned char **)parg = s->tlsext_ocsp_resp;
        return s->tlsext_ocsp_resplen;

    case SSL_CTRL_SET_TLSEXT_STATUS_REQ_OCSP_RESP:
        OPENSSL_free(s->tlsext_ocsp_resp);
        s->tlsext_ocsp_resp = parg;
        s->tlsext_ocsp_resplen = larg;
        ret = 1;
        break;

#ifndef OPENSSL_NO_HEARTBEATS
    case SSL_CTRL_TLS_EXT_SEND_HEARTBEAT:
        if (SSL_IS_DTLS(s))
            ret = dtls1_heartbeat(s);
        else
            ret = tls1_heartbeat(s);
        break;

    case SSL_CTRL_GET_TLS_EXT_HEARTBEAT_PENDING:
        ret = s->tlsext_hb_pending;
        break;

    case SSL_CTRL_SET_TLS_EXT_HEARTBEAT_NO_REQUESTS:
        if (larg)
            s->tlsext_heartbeat |= SSL_TLSEXT_HB_DONT_RECV_REQUESTS;
        else
            s->tlsext_heartbeat &= ~SSL_TLSEXT_HB_DONT_RECV_REQUESTS;
        ret = 1;
        break;
#endif

    case SSL_CTRL_CHAIN:
        if (larg)
            return ssl_cert_set1_chain(s, NULL, (STACK_OF(X509) *)parg);
        else
            return ssl_cert_set0_chain(s, NULL, (STACK_OF(X509) *)parg);

    case SSL_CTRL_CHAIN_CERT:
        if (larg)
            return ssl_cert_add1_chain_cert(s, NULL, (X509 *)parg);
        else
            return ssl_cert_add0_chain_cert(s, NULL, (X509 *)parg);

    case SSL_CTRL_GET_CHAIN_CERTS:
        *(STACK_OF(X509) **)parg = s->cert->key->chain;
        break;

    case SSL_CTRL_SELECT_CURRENT_CERT:
        return ssl_cert_select_current(s->cert, (X509 *)parg);

    case SSL_CTRL_SET_CURRENT_CERT:
        if (larg == SSL_CERT_SET_SERVER) {
            CERT_PKEY *cpk;
            const SSL_CIPHER *cipher;
            if (!s->server)
                return 0;
            cipher = s->s3->tmp.new_cipher;
            if (!cipher)
                return 0;
            /*
             * No certificate for unauthenticated ciphersuites or using SRP
             * authentication
             */
            if (cipher->algorithm_auth & (SSL_aNULL | SSL_aSRP))
                return 2;
            cpk = ssl_get_server_send_pkey(s);
            if (!cpk)
                return 0;
            s->cert->key = cpk;
            return 1;
        }
        return ssl_cert_set_current(s->cert, larg);

#ifndef OPENSSL_NO_EC
    case SSL_CTRL_GET_CURVES:
        {
            unsigned char *clist;
            size_t clistlen;
            if (!s->session)
                return 0;
            clist = s->session->tlsext_ellipticcurvelist;
            clistlen = s->session->tlsext_ellipticcurvelist_length / 2;
            if (parg) {
                size_t i;
                int *cptr = parg;
                unsigned int cid, nid;
                for (i = 0; i < clistlen; i++) {
                    n2s(clist, cid);
                    nid = tls1_ec_curve_id2nid(cid);
                    if (nid != 0)
                        cptr[i] = nid;
                    else
                        cptr[i] = TLSEXT_nid_unknown | cid;
                }
            }
            return (int)clistlen;
        }

    case SSL_CTRL_SET_CURVES:
        return tls1_set_curves(&s->tlsext_ellipticcurvelist,
                               &s->tlsext_ellipticcurvelist_length,
                               parg, larg);

    case SSL_CTRL_SET_CURVES_LIST:
        return tls1_set_curves_list(&s->tlsext_ellipticcurvelist,
                                    &s->tlsext_ellipticcurvelist_length,
                                    parg);

    case SSL_CTRL_GET_SHARED_CURVE:
        return tls1_shared_curve(s, larg);

#endif
    case SSL_CTRL_SET_SIGALGS:
        return tls1_set_sigalgs(s->cert, parg, larg, 0);

    case SSL_CTRL_SET_SIGALGS_LIST:
        return tls1_set_sigalgs_list(s->cert, parg, 0);

    case SSL_CTRL_SET_CLIENT_SIGALGS:
        return tls1_set_sigalgs(s->cert, parg, larg, 1);

    case SSL_CTRL_SET_CLIENT_SIGALGS_LIST:
        return tls1_set_sigalgs_list(s->cert, parg, 1);

    case SSL_CTRL_GET_CLIENT_CERT_TYPES:
        {
            const unsigned char **pctype = parg;
            if (s->server || !s->s3->tmp.cert_req)
                return 0;
            if (s->cert->ctypes) {
                if (pctype)
                    *pctype = s->cert->ctypes;
                return (int)s->cert->ctype_num;
            }
            if (pctype)
                *pctype = (unsigned char *)s->s3->tmp.ctype;
            return s->s3->tmp.ctype_num;
        }

    case SSL_CTRL_SET_CLIENT_CERT_TYPES:
        if (!s->server)
            return 0;
        return ssl3_set_req_cert_type(s->cert, parg, larg);

    case SSL_CTRL_BUILD_CERT_CHAIN:
        return ssl_build_cert_chain(s, NULL, larg);

    case SSL_CTRL_SET_VERIFY_CERT_STORE:
        return ssl_cert_set_cert_store(s->cert, parg, 0, larg);

    case SSL_CTRL_SET_CHAIN_CERT_STORE:
        return ssl_cert_set_cert_store(s->cert, parg, 1, larg);

    case SSL_CTRL_GET_PEER_SIGNATURE_NID:
        if (SSL_USE_SIGALGS(s)) {
            if (s->session) {
                const EVP_MD *sig;
                sig = s->s3->tmp.peer_md;
                if (sig) {
                    *(int *)parg = EVP_MD_type(sig);
                    return 1;
                }
            }
            return 0;
        }
        /* Might want to do something here for other versions */
        else
            return 0;

    case SSL_CTRL_GET_SERVER_TMP_KEY:
#if !defined(OPENSSL_NO_DH) || !defined(OPENSSL_NO_EC)
        if (s->server || s->session == NULL || s->s3->peer_tmp == NULL) {
            return 0;
        } else {
            EVP_PKEY_up_ref(s->s3->peer_tmp);
            *(EVP_PKEY **)parg = s->s3->peer_tmp;
            return 1;
        }
#else
        return 0;
#endif
#ifndef OPENSSL_NO_EC
    case SSL_CTRL_GET_EC_POINT_FORMATS:
        {
            SSL_SESSION *sess = s->session;
            const unsigned char **pformat = parg;
            if (!sess || !sess->tlsext_ecpointformatlist)
                return 0;
            *pformat = sess->tlsext_ecpointformatlist;
            return (int)sess->tlsext_ecpointformatlist_length;
        }
#endif

    default:
        break;
    }
    return (ret);
}

long ssl3_callback_ctrl(SSL *s, int cmd, void (*fp) (void))
{
    int ret = 0;

    switch (cmd) {
#ifndef OPENSSL_NO_DH
    case SSL_CTRL_SET_TMP_DH_CB:
        {
            s->cert->dh_tmp_cb = (DH *(*)(SSL *, int, int))fp;
        }
        break;
#endif
    case SSL_CTRL_SET_TLSEXT_DEBUG_CB:
        s->tlsext_debug_cb = (void (*)(SSL *, int, int,
                                       unsigned char *, int, void *))fp;
        break;

    case SSL_CTRL_SET_NOT_RESUMABLE_SESS_CB:
        {
            s->not_resumable_session_cb = (int (*)(SSL *, int))fp;
        }
        break;
    default:
        break;
    }
    return (ret);
}

long ssl3_ctx_ctrl(SSL_CTX *ctx, int cmd, long larg, void *parg)
{
    switch (cmd) {
#ifndef OPENSSL_NO_DH
    case SSL_CTRL_SET_TMP_DH:
        {
            DH *dh = (DH *)parg;
            EVP_PKEY *pkdh = NULL;
            if (dh == NULL) {
                SSLerr(SSL_F_SSL3_CTX_CTRL, ERR_R_PASSED_NULL_PARAMETER);
                return 0;
            }
            pkdh = ssl_dh_to_pkey(dh);
            if (pkdh == NULL) {
                SSLerr(SSL_F_SSL3_CTX_CTRL, ERR_R_MALLOC_FAILURE);
                return 0;
            }
            if (!ssl_ctx_security(ctx, SSL_SECOP_TMP_DH,
                                  EVP_PKEY_security_bits(pkdh), 0, pkdh)) {
                SSLerr(SSL_F_SSL3_CTX_CTRL, SSL_R_DH_KEY_TOO_SMALL);
                EVP_PKEY_free(pkdh);
                return 1;
            }
            EVP_PKEY_free(ctx->cert->dh_tmp);
            ctx->cert->dh_tmp = pkdh;
            return 1;
        }
        /*
         * break;
         */
    case SSL_CTRL_SET_TMP_DH_CB:
        {
            SSLerr(SSL_F_SSL3_CTX_CTRL, ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
            return (0);
        }
    case SSL_CTRL_SET_DH_AUTO:
        ctx->cert->dh_tmp_auto = larg;
        return 1;
#endif
#ifndef OPENSSL_NO_EC
    case SSL_CTRL_SET_TMP_ECDH:
        {
            const EC_GROUP *group = NULL;
            int nid;

            if (parg == NULL) {
                SSLerr(SSL_F_SSL3_CTX_CTRL, ERR_R_PASSED_NULL_PARAMETER);
                return 0;
            }
            group = EC_KEY_get0_group((const EC_KEY *)parg);
            if (group == NULL) {
                SSLerr(SSL_F_SSL3_CTX_CTRL, EC_R_MISSING_PARAMETERS);
                return 0;
            }
            nid = EC_GROUP_get_curve_name(group);
            if (nid == NID_undef)
                return 0;
            return tls1_set_curves(&ctx->tlsext_ellipticcurvelist,
                                   &ctx->tlsext_ellipticcurvelist_length,
                                   &nid, 1);
        }
        /* break; */
#endif                          /* !OPENSSL_NO_EC */
    case SSL_CTRL_SET_TLSEXT_SERVERNAME_ARG:
        ctx->tlsext_servername_arg = parg;
        break;
    case SSL_CTRL_SET_TLSEXT_TICKET_KEYS:
    case SSL_CTRL_GET_TLSEXT_TICKET_KEYS:
        {
            unsigned char *keys = parg;
            if (!keys)
                return 48;
            if (larg != 48) {
                SSLerr(SSL_F_SSL3_CTX_CTRL, SSL_R_INVALID_TICKET_KEYS_LENGTH);
                return 0;
            }
            if (cmd == SSL_CTRL_SET_TLSEXT_TICKET_KEYS) {
                memcpy(ctx->tlsext_tick_key_name, keys, 16);
                memcpy(ctx->tlsext_tick_hmac_key, keys + 16, 16);
                memcpy(ctx->tlsext_tick_aes_key, keys + 32, 16);
            } else {
                memcpy(keys, ctx->tlsext_tick_key_name, 16);
                memcpy(keys + 16, ctx->tlsext_tick_hmac_key, 16);
                memcpy(keys + 32, ctx->tlsext_tick_aes_key, 16);
            }
            return 1;
        }

    case SSL_CTRL_SET_TLSEXT_STATUS_REQ_CB_ARG:
        ctx->tlsext_status_arg = parg;
        return 1;

#ifndef OPENSSL_NO_SRP
    case SSL_CTRL_SET_TLS_EXT_SRP_USERNAME:
        ctx->srp_ctx.srp_Mask |= SSL_kSRP;
        OPENSSL_free(ctx->srp_ctx.login);
        ctx->srp_ctx.login = NULL;
        if (parg == NULL)
            break;
        if (strlen((const char *)parg) > 255
            || strlen((const char *)parg) < 1) {
            SSLerr(SSL_F_SSL3_CTX_CTRL, SSL_R_INVALID_SRP_USERNAME);
            return 0;
        }
        if ((ctx->srp_ctx.login = OPENSSL_strdup((char *)parg)) == NULL) {
            SSLerr(SSL_F_SSL3_CTX_CTRL, ERR_R_INTERNAL_ERROR);
            return 0;
        }
        break;
    case SSL_CTRL_SET_TLS_EXT_SRP_PASSWORD:
        ctx->srp_ctx.SRP_give_srp_client_pwd_callback =
            srp_password_from_info_cb;
        ctx->srp_ctx.info = parg;
        break;
    case SSL_CTRL_SET_SRP_ARG:
        ctx->srp_ctx.srp_Mask |= SSL_kSRP;
        ctx->srp_ctx.SRP_cb_arg = parg;
        break;

    case SSL_CTRL_SET_TLS_EXT_SRP_STRENGTH:
        ctx->srp_ctx.strength = larg;
        break;
#endif

#ifndef OPENSSL_NO_EC
    case SSL_CTRL_SET_CURVES:
        return tls1_set_curves(&ctx->tlsext_ellipticcurvelist,
                               &ctx->tlsext_ellipticcurvelist_length,
                               parg, larg);

    case SSL_CTRL_SET_CURVES_LIST:
        return tls1_set_curves_list(&ctx->tlsext_ellipticcurvelist,
                                    &ctx->tlsext_ellipticcurvelist_length,
                                    parg);
#endif
    case SSL_CTRL_SET_SIGALGS:
        return tls1_set_sigalgs(ctx->cert, parg, larg, 0);

    case SSL_CTRL_SET_SIGALGS_LIST:
        return tls1_set_sigalgs_list(ctx->cert, parg, 0);

    case SSL_CTRL_SET_CLIENT_SIGALGS:
        return tls1_set_sigalgs(ctx->cert, parg, larg, 1);

    case SSL_CTRL_SET_CLIENT_SIGALGS_LIST:
        return tls1_set_sigalgs_list(ctx->cert, parg, 1);

    case SSL_CTRL_SET_CLIENT_CERT_TYPES:
        return ssl3_set_req_cert_type(ctx->cert, parg, larg);

    case SSL_CTRL_BUILD_CERT_CHAIN:
        return ssl_build_cert_chain(NULL, ctx, larg);

    case SSL_CTRL_SET_VERIFY_CERT_STORE:
        return ssl_cert_set_cert_store(ctx->cert, parg, 0, larg);

    case SSL_CTRL_SET_CHAIN_CERT_STORE:
        return ssl_cert_set_cert_store(ctx->cert, parg, 1, larg);

        /* A Thawte special :-) */
    case SSL_CTRL_EXTRA_CHAIN_CERT:
        if (ctx->extra_certs == NULL) {
            if ((ctx->extra_certs = sk_X509_new_null()) == NULL)
                return (0);
        }
        sk_X509_push(ctx->extra_certs, (X509 *)parg);
        break;

    case SSL_CTRL_GET_EXTRA_CHAIN_CERTS:
        if (ctx->extra_certs == NULL && larg == 0)
            *(STACK_OF(X509) **)parg = ctx->cert->key->chain;
        else
            *(STACK_OF(X509) **)parg = ctx->extra_certs;
        break;

    case SSL_CTRL_CLEAR_EXTRA_CHAIN_CERTS:
        sk_X509_pop_free(ctx->extra_certs, X509_free);
        ctx->extra_certs = NULL;
        break;

    case SSL_CTRL_CHAIN:
        if (larg)
            return ssl_cert_set1_chain(NULL, ctx, (STACK_OF(X509) *)parg);
        else
            return ssl_cert_set0_chain(NULL, ctx, (STACK_OF(X509) *)parg);

    case SSL_CTRL_CHAIN_CERT:
        if (larg)
            return ssl_cert_add1_chain_cert(NULL, ctx, (X509 *)parg);
        else
            return ssl_cert_add0_chain_cert(NULL, ctx, (X509 *)parg);

    case SSL_CTRL_GET_CHAIN_CERTS:
        *(STACK_OF(X509) **)parg = ctx->cert->key->chain;
        break;

    case SSL_CTRL_SELECT_CURRENT_CERT:
        return ssl_cert_select_current(ctx->cert, (X509 *)parg);

    case SSL_CTRL_SET_CURRENT_CERT:
        return ssl_cert_set_current(ctx->cert, larg);

    default:
        return (0);
    }
    return (1);
}

long ssl3_ctx_callback_ctrl(SSL_CTX *ctx, int cmd, void (*fp) (void))
{
    switch (cmd) {
#ifndef OPENSSL_NO_DH
    case SSL_CTRL_SET_TMP_DH_CB:
        {
            ctx->cert->dh_tmp_cb = (DH *(*)(SSL *, int, int))fp;
        }
        break;
#endif
    case SSL_CTRL_SET_TLSEXT_SERVERNAME_CB:
        ctx->tlsext_servername_callback = (int (*)(SSL *, int *, void *))fp;
        break;

    case SSL_CTRL_SET_TLSEXT_STATUS_REQ_CB:
        ctx->tlsext_status_cb = (int (*)(SSL *, void *))fp;
        break;

    case SSL_CTRL_SET_TLSEXT_TICKET_KEY_CB:
        ctx->tlsext_ticket_key_cb = (int (*)(SSL *, unsigned char *,
                                             unsigned char *,
                                             EVP_CIPHER_CTX *,
                                             HMAC_CTX *, int))fp;
        break;

#ifndef OPENSSL_NO_SRP
    case SSL_CTRL_SET_SRP_VERIFY_PARAM_CB:
        ctx->srp_ctx.srp_Mask |= SSL_kSRP;
        ctx->srp_ctx.SRP_verify_param_callback = (int (*)(SSL *, void *))fp;
        break;
    case SSL_CTRL_SET_TLS_EXT_SRP_USERNAME_CB:
        ctx->srp_ctx.srp_Mask |= SSL_kSRP;
        ctx->srp_ctx.TLS_ext_srp_username_callback =
            (int (*)(SSL *, int *, void *))fp;
        break;
    case SSL_CTRL_SET_SRP_GIVE_CLIENT_PWD_CB:
        ctx->srp_ctx.srp_Mask |= SSL_kSRP;
        ctx->srp_ctx.SRP_give_srp_client_pwd_callback =
            (char *(*)(SSL *, void *))fp;
        break;
#endif
    case SSL_CTRL_SET_NOT_RESUMABLE_SESS_CB:
        {
            ctx->not_resumable_session_cb = (int (*)(SSL *, int))fp;
        }
        break;
    default:
        return (0);
    }
    return (1);
}

/*
 * This function needs to check if the ciphers required are actually
 * available
 */
const SSL_CIPHER *ssl3_get_cipher_by_char(const unsigned char *p)
{
    SSL_CIPHER c;
    const SSL_CIPHER *cp;
    uint32_t id;

    id = 0x03000000 | ((uint32_t)p[0] << 8L) | (uint32_t)p[1];
    c.id = id;
    cp = OBJ_bsearch_ssl_cipher_id(&c, ssl3_ciphers, SSL3_NUM_CIPHERS);
#ifdef DEBUG_PRINT_UNKNOWN_CIPHERSUITES
    if (cp == NULL)
        fprintf(stderr, "Unknown cipher ID %x\n", (p[0] << 8) | p[1]);
#endif
    return cp;
}

int ssl3_put_cipher_by_char(const SSL_CIPHER *c, unsigned char *p)
{
    long l;

    if (p != NULL) {
        l = c->id;
        if ((l & 0xff000000) != 0x03000000)
            return (0);
        p[0] = ((unsigned char)(l >> 8L)) & 0xFF;
        p[1] = ((unsigned char)(l)) & 0xFF;
    }
    return (2);
}

const SSL_CIPHER *ssl3_choose_cipher(SSL *s, STACK_OF(SSL_CIPHER) *clnt,
                               STACK_OF(SSL_CIPHER) *srvr)
{
    const SSL_CIPHER *c, *ret = NULL;
    STACK_OF(SSL_CIPHER) *prio, *allow;
    int i, ii, ok;
    unsigned long alg_k, alg_a, mask_k, mask_a;

    /* Let's see which ciphers we can support */

#if 0
    /*
     * Do not set the compare functions, because this may lead to a
     * reordering by "id". We want to keep the original ordering. We may pay
     * a price in performance during sk_SSL_CIPHER_find(), but would have to
     * pay with the price of sk_SSL_CIPHER_dup().
     */
    sk_SSL_CIPHER_set_cmp_func(srvr, ssl_cipher_ptr_id_cmp);
    sk_SSL_CIPHER_set_cmp_func(clnt, ssl_cipher_ptr_id_cmp);
#endif

#ifdef CIPHER_DEBUG
    fprintf(stderr, "Server has %d from %p:\n", sk_SSL_CIPHER_num(srvr),
            (void *)srvr);
    for (i = 0; i < sk_SSL_CIPHER_num(srvr); ++i) {
        c = sk_SSL_CIPHER_value(srvr, i);
        fprintf(stderr, "%p:%s\n", (void *)c, c->name);
    }
    fprintf(stderr, "Client sent %d from %p:\n", sk_SSL_CIPHER_num(clnt),
            (void *)clnt);
    for (i = 0; i < sk_SSL_CIPHER_num(clnt); ++i) {
        c = sk_SSL_CIPHER_value(clnt, i);
        fprintf(stderr, "%p:%s\n", (void *)c, c->name);
    }
#endif

    if (s->options & SSL_OP_CIPHER_SERVER_PREFERENCE || tls1_suiteb(s)) {
        prio = srvr;
        allow = clnt;
    } else {
        prio = clnt;
        allow = srvr;
    }

    tls1_set_cert_validity(s);

    for (i = 0; i < sk_SSL_CIPHER_num(prio); i++) {
        c = sk_SSL_CIPHER_value(prio, i);

        /* Skip TLS v1.2 only ciphersuites if not supported */
        if ((c->algorithm_ssl & SSL_TLSV1_2) && !SSL_USE_TLS1_2_CIPHERS(s))
            continue;
        /* Skip TLS v1.0 ciphersuites if SSLv3 */
        if ((c->algorithm_ssl & SSL_TLSV1) && s->version == SSL3_VERSION)
            continue;

        ssl_set_masks(s, c);
        mask_k = s->s3->tmp.mask_k;
        mask_a = s->s3->tmp.mask_a;
#ifndef OPENSSL_NO_SRP
        if (s->srp_ctx.srp_Mask & SSL_kSRP) {
            mask_k |= SSL_kSRP;
            mask_a |= SSL_aSRP;
        }
#endif

        alg_k = c->algorithm_mkey;
        alg_a = c->algorithm_auth;

#ifndef OPENSSL_NO_PSK
        /* with PSK there must be server callback set */
        if ((alg_k & SSL_PSK) && s->psk_server_callback == NULL)
            continue;
#endif                          /* OPENSSL_NO_PSK */

        ok = (alg_k & mask_k) && (alg_a & mask_a);
#ifdef CIPHER_DEBUG
        fprintf(stderr, "%d:[%08lX:%08lX:%08lX:%08lX]%p:%s\n", ok, alg_k,
                alg_a, mask_k, mask_a, (void *)c, c->name);
#endif

# ifndef OPENSSL_NO_EC
        /*
         * if we are considering an ECC cipher suite that uses an ephemeral
         * EC key check it
         */
        if (alg_k & SSL_kECDHE)
            ok = ok && tls1_check_ec_tmp_key(s, c->id);
# endif                         /* OPENSSL_NO_EC */

        if (!ok)
            continue;
        ii = sk_SSL_CIPHER_find(allow, c);
        if (ii >= 0) {
            /* Check security callback permits this cipher */
            if (!ssl_security(s, SSL_SECOP_CIPHER_SHARED,
                              c->strength_bits, 0, (void *)c))
                continue;
#if !defined(OPENSSL_NO_EC)
            if ((alg_k & SSL_kECDHE) && (alg_a & SSL_aECDSA)
                && s->s3->is_probably_safari) {
                if (!ret)
                    ret = sk_SSL_CIPHER_value(allow, ii);
                continue;
            }
#endif
            ret = sk_SSL_CIPHER_value(allow, ii);
            break;
        }
    }
    return (ret);
}

int ssl3_get_req_cert_type(SSL *s, unsigned char *p)
{
    int ret = 0;
    uint32_t alg_k, alg_a = 0;

    /* If we have custom certificate types set, use them */
    if (s->cert->ctypes) {
        memcpy(p, s->cert->ctypes, s->cert->ctype_num);
        return (int)s->cert->ctype_num;
    }
    /* Get mask of algorithms disabled by signature list */
    ssl_set_sig_mask(&alg_a, s, SSL_SECOP_SIGALG_MASK);

    alg_k = s->s3->tmp.new_cipher->algorithm_mkey;

#ifndef OPENSSL_NO_GOST
    if (s->version >= TLS1_VERSION) {
        if (alg_k & SSL_kGOST) {
            p[ret++] = TLS_CT_GOST01_SIGN;
            p[ret++] = TLS_CT_GOST12_SIGN;
            p[ret++] = TLS_CT_GOST12_512_SIGN;
            return (ret);
        }
    }
#endif

    if ((s->version == SSL3_VERSION) && (alg_k & SSL_kDHE)) {
#ifndef OPENSSL_NO_DH
# ifndef OPENSSL_NO_RSA
        p[ret++] = SSL3_CT_RSA_EPHEMERAL_DH;
# endif
# ifndef OPENSSL_NO_DSA
        p[ret++] = SSL3_CT_DSS_EPHEMERAL_DH;
# endif
#endif                          /* !OPENSSL_NO_DH */
    }
#ifndef OPENSSL_NO_RSA
    if (!(alg_a & SSL_aRSA))
        p[ret++] = SSL3_CT_RSA_SIGN;
#endif
#ifndef OPENSSL_NO_DSA
    if (!(alg_a & SSL_aDSS))
        p[ret++] = SSL3_CT_DSS_SIGN;
#endif
#ifndef OPENSSL_NO_EC
    /*
     * ECDSA certs can be used with RSA cipher suites too so we don't
     * need to check for SSL_kECDH or SSL_kECDHE
     */
    if (s->version >= TLS1_VERSION) {
        if (!(alg_a & SSL_aECDSA))
            p[ret++] = TLS_CT_ECDSA_SIGN;
    }
#endif
    return (ret);
}

static int ssl3_set_req_cert_type(CERT *c, const unsigned char *p, size_t len)
{
    OPENSSL_free(c->ctypes);
    c->ctypes = NULL;
    if (!p || !len)
        return 1;
    if (len > 0xff)
        return 0;
    c->ctypes = OPENSSL_malloc(len);
    if (c->ctypes == NULL)
        return 0;
    memcpy(c->ctypes, p, len);
    c->ctype_num = len;
    return 1;
}

int ssl3_shutdown(SSL *s)
{
    int ret;

    /*
     * Don't do anything much if we have not done the handshake or we don't
     * want to send messages :-)
     */
    if (s->quiet_shutdown || SSL_in_before(s)) {
        s->shutdown = (SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
        return (1);
    }

    if (!(s->shutdown & SSL_SENT_SHUTDOWN)) {
        s->shutdown |= SSL_SENT_SHUTDOWN;
        ssl3_send_alert(s, SSL3_AL_WARNING, SSL_AD_CLOSE_NOTIFY);
        /*
         * our shutdown alert has been sent now, and if it still needs to be
         * written, s->s3->alert_dispatch will be true
         */
        if (s->s3->alert_dispatch)
            return (-1);        /* return WANT_WRITE */
    } else if (s->s3->alert_dispatch) {
        /* resend it if not sent */
        ret = s->method->ssl_dispatch_alert(s);
        if (ret == -1) {
            /*
             * we only get to return -1 here the 2nd/Nth invocation, we must
             * have already signalled return 0 upon a previous invoation,
             * return WANT_WRITE
             */
            return (ret);
        }
    } else if (!(s->shutdown & SSL_RECEIVED_SHUTDOWN)) {
        /*
         * If we are waiting for a close from our peer, we are closed
         */
        s->method->ssl_read_bytes(s, 0, NULL, NULL, 0, 0);
        if (!(s->shutdown & SSL_RECEIVED_SHUTDOWN)) {
            return (-1);        /* return WANT_READ */
        }
    }

    if ((s->shutdown == (SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN)) &&
        !s->s3->alert_dispatch)
        return (1);
    else
        return (0);
}

int ssl3_write(SSL *s, const void *buf, int len)
{
    clear_sys_error();
    if (s->s3->renegotiate)
        ssl3_renegotiate_check(s);

    return s->method->ssl_write_bytes(s, SSL3_RT_APPLICATION_DATA,
                                         buf, len);
}

static int ssl3_read_internal(SSL *s, void *buf, int len, int peek)
{
    int ret;

    clear_sys_error();
    if (s->s3->renegotiate)
        ssl3_renegotiate_check(s);
    s->s3->in_read_app_data = 1;
    ret =
        s->method->ssl_read_bytes(s, SSL3_RT_APPLICATION_DATA, NULL, buf, len,
                                  peek);
    if ((ret == -1) && (s->s3->in_read_app_data == 2)) {
        /*
         * ssl3_read_bytes decided to call s->handshake_func, which called
         * ssl3_read_bytes to read handshake data. However, ssl3_read_bytes
         * actually found application data and thinks that application data
         * makes sense here; so disable handshake processing and try to read
         * application data again.
         */
        ossl_statem_set_in_handshake(s, 1);
        ret =
            s->method->ssl_read_bytes(s, SSL3_RT_APPLICATION_DATA, NULL, buf,
                                      len, peek);
        ossl_statem_set_in_handshake(s, 0);
    } else
        s->s3->in_read_app_data = 0;

    return (ret);
}

int ssl3_read(SSL *s, void *buf, int len)
{
    return ssl3_read_internal(s, buf, len, 0);
}

int ssl3_peek(SSL *s, void *buf, int len)
{
    return ssl3_read_internal(s, buf, len, 1);
}

int ssl3_renegotiate(SSL *s)
{
    if (s->handshake_func == NULL)
        return (1);

    if (s->s3->flags & SSL3_FLAGS_NO_RENEGOTIATE_CIPHERS)
        return (0);

    s->s3->renegotiate = 1;
    return (1);
}

int ssl3_renegotiate_check(SSL *s)
{
    int ret = 0;

    if (s->s3->renegotiate) {
        if (!RECORD_LAYER_read_pending(&s->rlayer)
            && !RECORD_LAYER_write_pending(&s->rlayer)
            && !SSL_in_init(s)) {
            /*
             * if we are the server, and we have sent a 'RENEGOTIATE'
             * message, we need to set the state machine into the renegotiate
             * state.
             */
            ossl_statem_set_renegotiate(s);
            s->s3->renegotiate = 0;
            s->s3->num_renegotiations++;
            s->s3->total_renegotiations++;
            ret = 1;
        }
    }
    return (ret);
}

/*
 * If we are using default SHA1+MD5 algorithms switch to new SHA256 PRF and
 * handshake macs if required.
 *
 * If PSK and using SHA384 for TLS < 1.2 switch to default.
 */
long ssl_get_algorithm2(SSL *s)
{
    long alg2 = s->s3->tmp.new_cipher->algorithm2;
    if (s->method->ssl3_enc->enc_flags & SSL_ENC_FLAG_SHA256_PRF) {
        if (alg2 == (SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF))
            return SSL_HANDSHAKE_MAC_SHA256 | TLS1_PRF_SHA256;
    } else if (s->s3->tmp.new_cipher->algorithm_mkey & SSL_PSK) {
        if (alg2 == (SSL_HANDSHAKE_MAC_SHA384 | TLS1_PRF_SHA384))
            return SSL_HANDSHAKE_MAC_DEFAULT | TLS1_PRF;
    }
    return alg2;
}

/*
 * Fill a ClientRandom or ServerRandom field of length len. Returns <= 0 on
 * failure, 1 on success.
 */
int ssl_fill_hello_random(SSL *s, int server, unsigned char *result, int len)
{
    int send_time = 0;

    if (len < 4)
        return 0;
    if (server)
        send_time = (s->mode & SSL_MODE_SEND_SERVERHELLO_TIME) != 0;
    else
        send_time = (s->mode & SSL_MODE_SEND_CLIENTHELLO_TIME) != 0;
    if (send_time) {
        unsigned long Time = (unsigned long)time(NULL);
        unsigned char *p = result;
        l2n(Time, p);
        return RAND_bytes(p, len - 4);
    } else
        return RAND_bytes(result, len);
}

int ssl_generate_master_secret(SSL *s, unsigned char *pms, size_t pmslen,
                               int free_pms)
{
#ifndef OPENSSL_NO_PSK
    unsigned long alg_k = s->s3->tmp.new_cipher->algorithm_mkey;
    if (alg_k & SSL_PSK) {
        unsigned char *pskpms, *t;
        size_t psklen = s->s3->tmp.psklen;
        size_t pskpmslen;

        /* create PSK premaster_secret */

        /* For plain PSK "other_secret" is psklen zeroes */
        if (alg_k & SSL_kPSK)
            pmslen = psklen;

        pskpmslen = 4 + pmslen + psklen;
        pskpms = OPENSSL_malloc(pskpmslen);
        if (pskpms == NULL) {
            s->session->master_key_length = 0;
            goto err;
        }
        t = pskpms;
        s2n(pmslen, t);
        if (alg_k & SSL_kPSK)
            memset(t, 0, pmslen);
        else
            memcpy(t, pms, pmslen);
        t += pmslen;
        s2n(psklen, t);
        memcpy(t, s->s3->tmp.psk, psklen);

        OPENSSL_clear_free(s->s3->tmp.psk, psklen);
        s->s3->tmp.psk = NULL;
        s->session->master_key_length =
            s->method->ssl3_enc->generate_master_secret(s,
                                                        s->session->master_key,
                                                        pskpms, pskpmslen);
        OPENSSL_clear_free(pskpms, pskpmslen);
    } else
#endif
        s->session->master_key_length =
            s->method->ssl3_enc->generate_master_secret(s,
                                                        s->session->master_key,
                                                        pms, pmslen);
#ifndef OPENSSL_NO_PSK
    err:
#endif
    if (pms) {
        if (free_pms)
            OPENSSL_clear_free(pms, pmslen);
        else
            OPENSSL_cleanse(pms, pmslen);
    }
    if (s->server == 0)
        s->s3->tmp.pms = NULL;
    return s->session->master_key_length >= 0;
}

/* Generate a private key from parameters or a curve NID */
EVP_PKEY *ssl_generate_pkey(EVP_PKEY *pm, int nid)
{
    EVP_PKEY_CTX *pctx = NULL;
    EVP_PKEY *pkey = NULL;
    if (pm != NULL) {
        pctx = EVP_PKEY_CTX_new(pm, NULL);
    } else {
        /*
         * Generate a new key for this curve.
         * Should not be called if EC is disabled: if it is it will
         * fail with an unknown algorithm error.
         */
        pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    }
    if (pctx == NULL)
        goto err;
    if (EVP_PKEY_keygen_init(pctx) <= 0)
        goto err;
#ifndef OPENSSL_NO_EC
    if (pm == NULL && EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, nid) <= 0)
        goto err;
#endif

    if (EVP_PKEY_keygen(pctx, &pkey) <= 0) {
        EVP_PKEY_free(pkey);
        pkey = NULL;
    }

    err:
    EVP_PKEY_CTX_free(pctx);
    return pkey;
}
/* Derive premaster or master secret for ECDH/DH */
int ssl_derive(SSL *s, EVP_PKEY *privkey, EVP_PKEY *pubkey)
{
    int rv = 0;
    unsigned char *pms = NULL;
    size_t pmslen = 0;
    EVP_PKEY_CTX *pctx;

    if (privkey == NULL || pubkey == NULL)
        return 0;

    pctx = EVP_PKEY_CTX_new(privkey, NULL);

    if (EVP_PKEY_derive_init(pctx) <= 0
        || EVP_PKEY_derive_set_peer(pctx, pubkey) <= 0
        || EVP_PKEY_derive(pctx, NULL, &pmslen) <= 0) {
        goto err;
    }

    pms = OPENSSL_malloc(pmslen);
    if (pms == NULL)
        goto err;

    if (EVP_PKEY_derive(pctx, pms, &pmslen) <= 0)
        goto err;

    if (s->server) {
        /* For server generate master secret and discard premaster */
        rv = ssl_generate_master_secret(s, pms, pmslen, 1);
        pms = NULL;
    } else {
        /* For client just save premaster secret */
        s->s3->tmp.pms = pms;
        s->s3->tmp.pmslen = pmslen;
        pms = NULL;
        rv = 1;
    }

    err:
    OPENSSL_clear_free(pms, pmslen);
    EVP_PKEY_CTX_free(pctx);
    return rv;
}

#ifndef OPENSSL_NO_DH
EVP_PKEY *ssl_dh_to_pkey(DH *dh)
{
    EVP_PKEY *ret;
    if (dh == NULL)
        return NULL;
    ret = EVP_PKEY_new();
    if (EVP_PKEY_set1_DH(ret, dh) <= 0) {
        EVP_PKEY_free(ret);
        return NULL;
    }
    return ret;
}
#endif
