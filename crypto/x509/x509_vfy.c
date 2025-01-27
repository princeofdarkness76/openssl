/* crypto/x509/x509_vfy.c */
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

#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#include "internal/cryptlib.h"
#include <openssl/crypto.h>
#include <openssl/lhash.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/asn1.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/objects.h>
#include <internal/dane.h>
#include <internal/x509_int.h>
#include "x509_lcl.h"

/* CRL score values */

/* No unhandled critical extensions */

#define CRL_SCORE_NOCRITICAL    0x100

/* certificate is within CRL scope */

#define CRL_SCORE_SCOPE         0x080

/* CRL times valid */

#define CRL_SCORE_TIME          0x040

/* Issuer name matches certificate */

#define CRL_SCORE_ISSUER_NAME   0x020

/* If this score or above CRL is probably valid */

#define CRL_SCORE_VALID (CRL_SCORE_NOCRITICAL|CRL_SCORE_TIME|CRL_SCORE_SCOPE)

/* CRL issuer is certificate issuer */

#define CRL_SCORE_ISSUER_CERT   0x018

/* CRL issuer is on certificate path */

#define CRL_SCORE_SAME_PATH     0x008

/* CRL issuer matches CRL AKID */

#define CRL_SCORE_AKID          0x004

/* Have a delta CRL with valid times */

#define CRL_SCORE_TIME_DELTA    0x002

static int build_chain(X509_STORE_CTX *ctx);
static int verify_chain(X509_STORE_CTX *ctx);
static int dane_verify(X509_STORE_CTX *ctx);
static int null_callback(int ok, X509_STORE_CTX *e);
static int check_issued(X509_STORE_CTX *ctx, X509 *x, X509 *issuer);
static X509 *find_issuer(X509_STORE_CTX *ctx, STACK_OF(X509) *sk, X509 *x);
static int check_chain_extensions(X509_STORE_CTX *ctx);
static int check_name_constraints(X509_STORE_CTX *ctx);
static int check_id(X509_STORE_CTX *ctx);
static int check_trust(X509_STORE_CTX *ctx, int num_untrusted);
static int check_revocation(X509_STORE_CTX *ctx);
static int check_cert(X509_STORE_CTX *ctx);
static int check_policy(X509_STORE_CTX *ctx);
static int get_issuer_sk(X509 **issuer, X509_STORE_CTX *ctx, X509 *x);
static int check_dane_issuer(X509_STORE_CTX *ctx, int depth);

static int get_crl_score(X509_STORE_CTX *ctx, X509 **pissuer,
                         unsigned int *preasons, X509_CRL *crl, X509 *x);
static int get_crl_delta(X509_STORE_CTX *ctx,
                         X509_CRL **pcrl, X509_CRL **pdcrl, X509 *x);
static void get_delta_sk(X509_STORE_CTX *ctx, X509_CRL **dcrl,
                         int *pcrl_score, X509_CRL *base,
                         STACK_OF(X509_CRL) *crls);
static void crl_akid_check(X509_STORE_CTX *ctx, X509_CRL *crl, X509 **pissuer,
                           int *pcrl_score);
static int crl_crldp_check(X509 *x, X509_CRL *crl, int crl_score,
                           unsigned int *preasons);
static int check_crl_path(X509_STORE_CTX *ctx, X509 *x);
static int check_crl_chain(X509_STORE_CTX *ctx,
                           STACK_OF(X509) *cert_path,
                           STACK_OF(X509) *crl_path);

static int internal_verify(X509_STORE_CTX *ctx);

static int null_callback(int ok, X509_STORE_CTX *e)
{
    return ok;
}

/* Return 1 is a certificate is self signed */
static int cert_self_signed(X509 *x)
{
    /*
     * FIXME: x509v3_cache_extensions() needs to detect more failures and not
     * set EXFLAG_SET when that happens.  Especially, if the failures are
     * parse errors, rather than memory pressure!
     */
    X509_check_purpose(x, -1, 0);
    if (x->ex_flags & EXFLAG_SS)
        return 1;
    else
        return 0;
}

/* Given a certificate try and find an exact match in the store */

static X509 *lookup_cert_match(X509_STORE_CTX *ctx, X509 *x)
{
    STACK_OF(X509) *certs;
    X509 *xtmp = NULL;
    int i;
    /* Lookup all certs with matching subject name */
    certs = ctx->lookup_certs(ctx, X509_get_subject_name(x));
    if (certs == NULL)
        return NULL;
    /* Look for exact match */
    for (i = 0; i < sk_X509_num(certs); i++) {
        xtmp = sk_X509_value(certs, i);
        if (!X509_cmp(xtmp, x))
            break;
    }
    if (i < sk_X509_num(certs))
        X509_up_ref(xtmp);
    else
        xtmp = NULL;
    sk_X509_pop_free(certs, X509_free);
    return xtmp;
}

static int verify_chain(X509_STORE_CTX *ctx)
{
    int (*cb) (int xok, X509_STORE_CTX *xctx) = ctx->verify_cb;
    int err;
    int ok;

    /*
     * Before either returning with an error, or continuing with CRL checks,
     * instantiate chain public key parameters.
     */
    if ((ok = build_chain(ctx)) == 0 ||
        (ok = check_chain_extensions(ctx)) == 0 ||
        (ok = check_name_constraints(ctx)) == 0 ||
        (ok = check_id(ctx)) == 0 || 1)
        X509_get_pubkey_parameters(NULL, ctx->chain);
    if (ok == 0 || (ok = ctx->check_revocation(ctx)) == 0)
        return ok;

    err = X509_chain_check_suiteb(&ctx->error_depth, NULL, ctx->chain,
                                  ctx->param->flags);
    if (err != X509_V_OK) {
        ctx->error = err;
        ctx->current_cert = sk_X509_value(ctx->chain, ctx->error_depth);
        if ((ok = cb(0, ctx)) == 0)
            return ok;
    }

    /* Verify chain signatures and expiration times */
    ok = (ctx->verify != NULL) ? ctx->verify(ctx) : internal_verify(ctx);
    if (!ok)
        return ok;

#ifndef OPENSSL_NO_RFC3779
    /* RFC 3779 path validation, now that CRL check has been done */
    if ((ok = v3_asid_validate_path(ctx)) == 0)
        return ok;
    if ((ok = v3_addr_validate_path(ctx)) == 0)
        return ok;
#endif

    /* If we get this far evaluate policies */
    if (ctx->param->flags & X509_V_FLAG_POLICY_CHECK)
        ok = ctx->check_policy(ctx);
    return ok;
}

int X509_verify_cert(X509_STORE_CTX *ctx)
{
    struct dane_st *dane = (struct dane_st *)ctx->dane;

    if (ctx->cert == NULL) {
        X509err(X509_F_X509_VERIFY_CERT, X509_R_NO_CERT_SET_FOR_US_TO_VERIFY);
        return -1;
    }

    if (ctx->chain != NULL) {
        /*
         * This X509_STORE_CTX has already been used to verify a cert. We
         * cannot do another one.
         */
        X509err(X509_F_X509_VERIFY_CERT, ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
        return -1;
    }

    /*
     * first we make sure the chain we are going to build is present and that
     * the first entry is in place
     */
    if (((ctx->chain = sk_X509_new_null()) == NULL) ||
        (!sk_X509_push(ctx->chain, ctx->cert))) {
        X509err(X509_F_X509_VERIFY_CERT, ERR_R_MALLOC_FAILURE);
        return -1;
    }
    X509_up_ref(ctx->cert);
    ctx->num_untrusted = 1;

    /*
     * If dane->trecs is an empty stack, we'll fail, since the user enabled
     * DANE.  If none of the TLSA records were usable, and it makes sense to
     * keep going with an unauthenticated handshake, they can handle that in
     * the verify callback, or not set SSL_VERIFY_PEER.
     */
    if (DANETLS_ENABLED(dane))
        return dane_verify(ctx);
    return verify_chain(ctx);
}

/*
 * Given a STACK_OF(X509) find the issuer of cert (if any)
 */

static X509 *find_issuer(X509_STORE_CTX *ctx, STACK_OF(X509) *sk, X509 *x)
{
    int i;
    X509 *issuer, *rv = NULL;;
    for (i = 0; i < sk_X509_num(sk); i++) {
        issuer = sk_X509_value(sk, i);
        if (ctx->check_issued(ctx, x, issuer)) {
            rv = issuer;
            if (x509_check_cert_time(ctx, rv, 1))
                break;
        }
    }
    return rv;
}

/* Given a possible certificate and issuer check them */

static int check_issued(X509_STORE_CTX *ctx, X509 *x, X509 *issuer)
{
    int ret;
    if (x == issuer)
        return cert_self_signed(x);
    ret = X509_check_issued(issuer, x);
    if (ret == X509_V_OK) {
        int i;
        X509 *ch;
        /* Special case: single self signed certificate */
        if (cert_self_signed(x) && sk_X509_num(ctx->chain) == 1)
            return 1;
        for (i = 0; i < sk_X509_num(ctx->chain); i++) {
            ch = sk_X509_value(ctx->chain, i);
            if (ch == issuer || !X509_cmp(ch, issuer)) {
                ret = X509_V_ERR_PATH_LOOP;
                break;
            }
        }
    }

    if (ret == X509_V_OK)
        return 1;
    /* If we haven't asked for issuer errors don't set ctx */
    if (!(ctx->param->flags & X509_V_FLAG_CB_ISSUER_CHECK))
        return 0;

    ctx->error = ret;
    ctx->current_cert = x;
    ctx->current_issuer = issuer;
    return ctx->verify_cb(0, ctx);
}

/* Alternative lookup method: look from a STACK stored in other_ctx */

static int get_issuer_sk(X509 **issuer, X509_STORE_CTX *ctx, X509 *x)
{
    *issuer = find_issuer(ctx, ctx->other_ctx, x);
    if (*issuer) {
        X509_up_ref(*issuer);
        return 1;
    } else
        return 0;
}

static STACK_OF(X509) *lookup_certs_sk(X509_STORE_CTX *ctx, X509_NAME *nm)
{
    STACK_OF(X509) *sk = NULL;
    X509 *x;
    int i;
    for (i = 0; i < sk_X509_num(ctx->other_ctx); i++) {
        x = sk_X509_value(ctx->other_ctx, i);
        if (X509_NAME_cmp(nm, X509_get_subject_name(x)) == 0) {
            if (sk == NULL)
                sk = sk_X509_new_null();
            if (sk == NULL || sk_X509_push(sk, x) == 0) {
                sk_X509_pop_free(sk, X509_free);
                return NULL;
            }
            X509_up_ref(x);
        }
    }
    return sk;
}

/*
 * Check a certificate chains extensions for consistency with the supplied
 * purpose
 */

static int check_chain_extensions(X509_STORE_CTX *ctx)
{
    int i, ok = 0, must_be_ca, plen = 0;
    X509 *x;
    int (*cb) (int xok, X509_STORE_CTX *xctx);
    int proxy_path_length = 0;
    int purpose;
    int allow_proxy_certs;
    cb = ctx->verify_cb;

    /*-
     *  must_be_ca can have 1 of 3 values:
     * -1: we accept both CA and non-CA certificates, to allow direct
     *     use of self-signed certificates (which are marked as CA).
     * 0:  we only accept non-CA certificates.  This is currently not
     *     used, but the possibility is present for future extensions.
     * 1:  we only accept CA certificates.  This is currently used for
     *     all certificates in the chain except the leaf certificate.
     */
    must_be_ca = -1;

    /* CRL path validation */
    if (ctx->parent) {
        allow_proxy_certs = 0;
        purpose = X509_PURPOSE_CRL_SIGN;
    } else {
        allow_proxy_certs =
            ! !(ctx->param->flags & X509_V_FLAG_ALLOW_PROXY_CERTS);
        /*
         * A hack to keep people who don't want to modify their software
         * happy
         */
        if (getenv("OPENSSL_ALLOW_PROXY_CERTS"))
            allow_proxy_certs = 1;
        purpose = ctx->param->purpose;
    }

    /* Check all untrusted certificates */
    for (i = 0; i == 0 || i < ctx->num_untrusted; i++) {
        int ret;
        x = sk_X509_value(ctx->chain, i);
        if (!(ctx->param->flags & X509_V_FLAG_IGNORE_CRITICAL)
            && (x->ex_flags & EXFLAG_CRITICAL)) {
            ctx->error = X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION;
            ctx->error_depth = i;
            ctx->current_cert = x;
            ok = cb(0, ctx);
            if (!ok)
                goto end;
        }
        if (!allow_proxy_certs && (x->ex_flags & EXFLAG_PROXY)) {
            ctx->error = X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED;
            ctx->error_depth = i;
            ctx->current_cert = x;
            ok = cb(0, ctx);
            if (!ok)
                goto end;
        }
        ret = X509_check_ca(x);
        switch (must_be_ca) {
        case -1:
            if ((ctx->param->flags & X509_V_FLAG_X509_STRICT)
                && (ret != 1) && (ret != 0)) {
                ret = 0;
                ctx->error = X509_V_ERR_INVALID_CA;
            } else
                ret = 1;
            break;
        case 0:
            if (ret != 0) {
                ret = 0;
                ctx->error = X509_V_ERR_INVALID_NON_CA;
            } else
                ret = 1;
            break;
        default:
            if ((ret == 0)
                || ((ctx->param->flags & X509_V_FLAG_X509_STRICT)
                    && (ret != 1))) {
                ret = 0;
                ctx->error = X509_V_ERR_INVALID_CA;
            } else
                ret = 1;
            break;
        }
        if (ret == 0) {
            ctx->error_depth = i;
            ctx->current_cert = x;
            ok = cb(0, ctx);
            if (!ok)
                goto end;
        }
        if (ctx->param->purpose > 0) {
            ret = X509_check_purpose(x, purpose, must_be_ca > 0);
            if ((ret == 0)
                || ((ctx->param->flags & X509_V_FLAG_X509_STRICT)
                    && (ret != 1))) {
                ctx->error = X509_V_ERR_INVALID_PURPOSE;
                ctx->error_depth = i;
                ctx->current_cert = x;
                ok = cb(0, ctx);
                if (!ok)
                    goto end;
            }
        }
        /* Check pathlen if not self issued */
        if ((i > 1) && !(x->ex_flags & EXFLAG_SI)
            && (x->ex_pathlen != -1)
            && (plen > (x->ex_pathlen + proxy_path_length + 1))) {
            ctx->error = X509_V_ERR_PATH_LENGTH_EXCEEDED;
            ctx->error_depth = i;
            ctx->current_cert = x;
            ok = cb(0, ctx);
            if (!ok)
                goto end;
        }
        /* Increment path length if not self issued */
        if (!(x->ex_flags & EXFLAG_SI))
            plen++;
        /*
         * If this certificate is a proxy certificate, the next certificate
         * must be another proxy certificate or a EE certificate.  If not,
         * the next certificate must be a CA certificate.
         */
        if (x->ex_flags & EXFLAG_PROXY) {
            if (x->ex_pcpathlen != -1 && i > x->ex_pcpathlen) {
                ctx->error = X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED;
                ctx->error_depth = i;
                ctx->current_cert = x;
                ok = cb(0, ctx);
                if (!ok)
                    goto end;
            }
            proxy_path_length++;
            must_be_ca = 0;
        } else
            must_be_ca = 1;
    }
    ok = 1;
 end:
    return ok;
}

static int check_name_constraints(X509_STORE_CTX *ctx)
{
    X509 *x;
    int i, j, rv;
    /* Check name constraints for all certificates */
    for (i = sk_X509_num(ctx->chain) - 1; i >= 0; i--) {
        x = sk_X509_value(ctx->chain, i);
        /* Ignore self issued certs unless last in chain */
        if (i && (x->ex_flags & EXFLAG_SI))
            continue;
        /*
         * Check against constraints for all certificates higher in chain
         * including trust anchor. Trust anchor not strictly speaking needed
         * but if it includes constraints it is to be assumed it expects them
         * to be obeyed.
         */
        for (j = sk_X509_num(ctx->chain) - 1; j > i; j--) {
            NAME_CONSTRAINTS *nc = sk_X509_value(ctx->chain, j)->nc;
            if (nc) {
                rv = NAME_CONSTRAINTS_check(x, nc);
                if (rv != X509_V_OK) {
                    ctx->error = rv;
                    ctx->error_depth = i;
                    ctx->current_cert = x;
                    if (!ctx->verify_cb(0, ctx))
                        return 0;
                }
            }
        }
    }
    return 1;
}

static int check_id_error(X509_STORE_CTX *ctx, int errcode)
{
    ctx->error = errcode;
    ctx->current_cert = ctx->cert;
    ctx->error_depth = 0;
    return ctx->verify_cb(0, ctx);
}

static int check_hosts(X509 *x, X509_VERIFY_PARAM *vpm)
{
    int i;
    int n = sk_OPENSSL_STRING_num(vpm->hosts);
    char *name;

    if (vpm->peername != NULL) {
        OPENSSL_free(vpm->peername);
        vpm->peername = NULL;
    }
    for (i = 0; i < n; ++i) {
        name = sk_OPENSSL_STRING_value(vpm->hosts, i);
        if (X509_check_host(x, name, 0, vpm->hostflags, &vpm->peername) > 0)
            return 1;
    }
    return n == 0;
}

static int check_id(X509_STORE_CTX *ctx)
{
    X509_VERIFY_PARAM *vpm = ctx->param;
    X509 *x = ctx->cert;
    if (vpm->hosts && check_hosts(x, vpm) <= 0) {
        if (!check_id_error(ctx, X509_V_ERR_HOSTNAME_MISMATCH))
            return 0;
    }
    if (vpm->email && X509_check_email(x, vpm->email, vpm->emaillen, 0) <= 0) {
        if (!check_id_error(ctx, X509_V_ERR_EMAIL_MISMATCH))
            return 0;
    }
    if (vpm->ip && X509_check_ip(x, vpm->ip, vpm->iplen, 0) <= 0) {
        if (!check_id_error(ctx, X509_V_ERR_IP_ADDRESS_MISMATCH))
            return 0;
    }
    return 1;
}

static int check_trust(X509_STORE_CTX *ctx, int num_untrusted)
{
    int i, ok = 0;
    X509 *x = NULL;
    X509 *mx;
    int (*cb) (int xok, X509_STORE_CTX *xctx) = ctx->verify_cb;
    struct dane_st *dane = (struct dane_st *)ctx->dane;
    int num = sk_X509_num(ctx->chain);
    int trust;

    /*
     * Check for a DANE issuer at depth 1 or greater, if it is a DANE-TA(2)
     * match, we're done, otherwise we'll merely record the match depth.
     */
    if (DANETLS_HAS_TA(dane) && num_untrusted > 0 && num_untrusted < num) {
        switch (trust = check_dane_issuer(ctx, num_untrusted)) {
        case X509_TRUST_TRUSTED:
        case X509_TRUST_REJECTED:
            return trust;
        }
    }

    /*
     * Check trusted certificates in chain at depth num_untrusted and up.
     * Note, that depths 0..num_untrusted-1 may also contain trusted
     * certificates, but the caller is expected to have already checked those,
     * and wants to incrementally check just any added since.
     */
    for (i = num_untrusted; i < num; i++) {
        x = sk_X509_value(ctx->chain, i);
        trust = X509_check_trust(x, ctx->param->trust, 0);
        /* If explicitly trusted return trusted */
        if (trust == X509_TRUST_TRUSTED)
            goto trusted;
        if (trust == X509_TRUST_REJECTED)
            goto rejected;
    }

    /*
     * If we are looking at a trusted certificate, and accept partial chains,
     * the chain is PKIX trusted.
     */
    if (num_untrusted < num) {
        if (ctx->param->flags & X509_V_FLAG_PARTIAL_CHAIN)
            goto trusted;
        return X509_TRUST_UNTRUSTED;
    }

    if (num_untrusted == num && ctx->param->flags & X509_V_FLAG_PARTIAL_CHAIN) {
        /*
         * Last-resort call with no new trusted certificates, check the leaf
         * for a direct trust store match.
         */
        i = 0;
        x = sk_X509_value(ctx->chain, i);
        mx = lookup_cert_match(ctx, x);
        if (!mx)
            return X509_TRUST_UNTRUSTED;

        /*
         * Check explicit auxiliary trust/reject settings.  If none are set,
         * we'll accept X509_TRUST_UNTRUSTED when not self-signed.
         */
        trust = X509_check_trust(mx, ctx->param->trust, 0);
        if (trust == X509_TRUST_REJECTED) {
            X509_free(mx);
            goto rejected;
        }

        /* Replace leaf with trusted match */
        (void) sk_X509_set(ctx->chain, 0, mx);
        X509_free(x);
        ctx->num_untrusted = 0;
        goto trusted;
    }

    /*
     * If no trusted certs in chain at all return untrusted and allow
     * standard (no issuer cert) etc errors to be indicated.
     */
    return X509_TRUST_UNTRUSTED;

 rejected:
    ctx->error_depth = i;
    ctx->current_cert = x;
    ctx->error = X509_V_ERR_CERT_REJECTED;
    ok = cb(0, ctx);
    if (!ok)
        return X509_TRUST_REJECTED;
    return X509_TRUST_UNTRUSTED;

 trusted:
    if (!DANETLS_ENABLED(dane))
        return X509_TRUST_TRUSTED;
    if (dane->pdpth < 0)
        dane->pdpth = num_untrusted;
    /* With DANE, PKIX alone is not trusted until we have both */
    if (dane->mdpth >= 0)
        return X509_TRUST_TRUSTED;
    return X509_TRUST_UNTRUSTED;
}

static int check_revocation(X509_STORE_CTX *ctx)
{
    int i = 0, last = 0, ok = 0;
    if (!(ctx->param->flags & X509_V_FLAG_CRL_CHECK))
        return 1;
    if (ctx->param->flags & X509_V_FLAG_CRL_CHECK_ALL)
        last = sk_X509_num(ctx->chain) - 1;
    else {
        /* If checking CRL paths this isn't the EE certificate */
        if (ctx->parent)
            return 1;
        last = 0;
    }
    for (i = 0; i <= last; i++) {
        ctx->error_depth = i;
        ok = check_cert(ctx);
        if (!ok)
            return ok;
    }
    return 1;
}

static int check_cert(X509_STORE_CTX *ctx)
{
    X509_CRL *crl = NULL, *dcrl = NULL;
    X509 *x = NULL;
    int ok = 0, cnum = 0;
    unsigned int last_reasons = 0;
    cnum = ctx->error_depth;
    x = sk_X509_value(ctx->chain, cnum);
    ctx->current_cert = x;
    ctx->current_issuer = NULL;
    ctx->current_crl_score = 0;
    ctx->current_reasons = 0;
    while (ctx->current_reasons != CRLDP_ALL_REASONS) {
        last_reasons = ctx->current_reasons;
        /* Try to retrieve relevant CRL */
        if (ctx->get_crl)
            ok = ctx->get_crl(ctx, &crl, x);
        else
            ok = get_crl_delta(ctx, &crl, &dcrl, x);
        /*
         * If error looking up CRL, nothing we can do except notify callback
         */
        if (!ok) {
            ctx->error = X509_V_ERR_UNABLE_TO_GET_CRL;
            ok = ctx->verify_cb(0, ctx);
            goto err;
        }
        ctx->current_crl = crl;
        ok = ctx->check_crl(ctx, crl);
        if (!ok)
            goto err;

        if (dcrl) {
            ok = ctx->check_crl(ctx, dcrl);
            if (!ok)
                goto err;
            ok = ctx->cert_crl(ctx, dcrl, x);
            if (!ok)
                goto err;
        } else
            ok = 1;

        /* Don't look in full CRL if delta reason is removefromCRL */
        if (ok != 2) {
            ok = ctx->cert_crl(ctx, crl, x);
            if (!ok)
                goto err;
        }

        X509_CRL_free(crl);
        X509_CRL_free(dcrl);
        crl = NULL;
        dcrl = NULL;
        /*
         * If reasons not updated we wont get anywhere by another iteration,
         * so exit loop.
         */
        if (last_reasons == ctx->current_reasons) {
            ctx->error = X509_V_ERR_UNABLE_TO_GET_CRL;
            ok = ctx->verify_cb(0, ctx);
            goto err;
        }
    }
 err:
    X509_CRL_free(crl);
    X509_CRL_free(dcrl);

    ctx->current_crl = NULL;
    return ok;

}

/* Check CRL times against values in X509_STORE_CTX */

static int check_crl_time(X509_STORE_CTX *ctx, X509_CRL *crl, int notify)
{
    time_t *ptime;
    int i;
    if (notify)
        ctx->current_crl = crl;
    if (ctx->param->flags & X509_V_FLAG_USE_CHECK_TIME)
        ptime = &ctx->param->check_time;
    else if (ctx->param->flags & X509_V_FLAG_NO_CHECK_TIME)
        return 1;
    else
        ptime = NULL;

    i = X509_cmp_time(X509_CRL_get_lastUpdate(crl), ptime);
    if (i == 0) {
        if (!notify)
            return 0;
        ctx->error = X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD;
        if (!ctx->verify_cb(0, ctx))
            return 0;
    }

    if (i > 0) {
        if (!notify)
            return 0;
        ctx->error = X509_V_ERR_CRL_NOT_YET_VALID;
        if (!ctx->verify_cb(0, ctx))
            return 0;
    }

    if (X509_CRL_get_nextUpdate(crl)) {
        i = X509_cmp_time(X509_CRL_get_nextUpdate(crl), ptime);

        if (i == 0) {
            if (!notify)
                return 0;
            ctx->error = X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD;
            if (!ctx->verify_cb(0, ctx))
                return 0;
        }
        /* Ignore expiry of base CRL is delta is valid */
        if ((i < 0) && !(ctx->current_crl_score & CRL_SCORE_TIME_DELTA)) {
            if (!notify)
                return 0;
            ctx->error = X509_V_ERR_CRL_HAS_EXPIRED;
            if (!ctx->verify_cb(0, ctx))
                return 0;
        }
    }

    if (notify)
        ctx->current_crl = NULL;

    return 1;
}

static int get_crl_sk(X509_STORE_CTX *ctx, X509_CRL **pcrl, X509_CRL **pdcrl,
                      X509 **pissuer, int *pscore, unsigned int *preasons,
                      STACK_OF(X509_CRL) *crls)
{
    int i, crl_score, best_score = *pscore;
    unsigned int reasons, best_reasons = 0;
    X509 *x = ctx->current_cert;
    X509_CRL *crl, *best_crl = NULL;
    X509 *crl_issuer = NULL, *best_crl_issuer = NULL;

    for (i = 0; i < sk_X509_CRL_num(crls); i++) {
        crl = sk_X509_CRL_value(crls, i);
        reasons = *preasons;
        crl_score = get_crl_score(ctx, &crl_issuer, &reasons, crl, x);

        if (crl_score > best_score) {
            best_crl = crl;
            best_crl_issuer = crl_issuer;
            best_score = crl_score;
            best_reasons = reasons;
        }
    }

    if (best_crl) {
        X509_CRL_free(*pcrl);
        *pcrl = best_crl;
        *pissuer = best_crl_issuer;
        *pscore = best_score;
        *preasons = best_reasons;
        X509_CRL_up_ref(best_crl);
        X509_CRL_free(*pdcrl);
        *pdcrl = NULL;
        get_delta_sk(ctx, pdcrl, pscore, best_crl, crls);
    }

    if (best_score >= CRL_SCORE_VALID)
        return 1;

    return 0;
}

/*
 * Compare two CRL extensions for delta checking purposes. They should be
 * both present or both absent. If both present all fields must be identical.
 */

static int crl_extension_match(X509_CRL *a, X509_CRL *b, int nid)
{
    ASN1_OCTET_STRING *exta, *extb;
    int i;
    i = X509_CRL_get_ext_by_NID(a, nid, -1);
    if (i >= 0) {
        /* Can't have multiple occurrences */
        if (X509_CRL_get_ext_by_NID(a, nid, i) != -1)
            return 0;
        exta = X509_EXTENSION_get_data(X509_CRL_get_ext(a, i));
    } else
        exta = NULL;

    i = X509_CRL_get_ext_by_NID(b, nid, -1);

    if (i >= 0) {

        if (X509_CRL_get_ext_by_NID(b, nid, i) != -1)
            return 0;
        extb = X509_EXTENSION_get_data(X509_CRL_get_ext(b, i));
    } else
        extb = NULL;

    if (!exta && !extb)
        return 1;

    if (!exta || !extb)
        return 0;

    if (ASN1_OCTET_STRING_cmp(exta, extb))
        return 0;

    return 1;
}

/* See if a base and delta are compatible */

static int check_delta_base(X509_CRL *delta, X509_CRL *base)
{
    /* Delta CRL must be a delta */
    if (!delta->base_crl_number)
        return 0;
    /* Base must have a CRL number */
    if (!base->crl_number)
        return 0;
    /* Issuer names must match */
    if (X509_NAME_cmp(X509_CRL_get_issuer(base), X509_CRL_get_issuer(delta)))
        return 0;
    /* AKID and IDP must match */
    if (!crl_extension_match(delta, base, NID_authority_key_identifier))
        return 0;
    if (!crl_extension_match(delta, base, NID_issuing_distribution_point))
        return 0;
    /* Delta CRL base number must not exceed Full CRL number. */
    if (ASN1_INTEGER_cmp(delta->base_crl_number, base->crl_number) > 0)
        return 0;
    /* Delta CRL number must exceed full CRL number */
    if (ASN1_INTEGER_cmp(delta->crl_number, base->crl_number) > 0)
        return 1;
    return 0;
}

/*
 * For a given base CRL find a delta... maybe extend to delta scoring or
 * retrieve a chain of deltas...
 */

static void get_delta_sk(X509_STORE_CTX *ctx, X509_CRL **dcrl, int *pscore,
                         X509_CRL *base, STACK_OF(X509_CRL) *crls)
{
    X509_CRL *delta;
    int i;
    if (!(ctx->param->flags & X509_V_FLAG_USE_DELTAS))
        return;
    if (!((ctx->current_cert->ex_flags | base->flags) & EXFLAG_FRESHEST))
        return;
    for (i = 0; i < sk_X509_CRL_num(crls); i++) {
        delta = sk_X509_CRL_value(crls, i);
        if (check_delta_base(delta, base)) {
            if (check_crl_time(ctx, delta, 0))
                *pscore |= CRL_SCORE_TIME_DELTA;
            X509_CRL_up_ref(delta);
            *dcrl = delta;
            return;
        }
    }
    *dcrl = NULL;
}

/*
 * For a given CRL return how suitable it is for the supplied certificate
 * 'x'. The return value is a mask of several criteria. If the issuer is not
 * the certificate issuer this is returned in *pissuer. The reasons mask is
 * also used to determine if the CRL is suitable: if no new reasons the CRL
 * is rejected, otherwise reasons is updated.
 */

static int get_crl_score(X509_STORE_CTX *ctx, X509 **pissuer,
                         unsigned int *preasons, X509_CRL *crl, X509 *x)
{

    int crl_score = 0;
    unsigned int tmp_reasons = *preasons, crl_reasons;

    /* First see if we can reject CRL straight away */

    /* Invalid IDP cannot be processed */
    if (crl->idp_flags & IDP_INVALID)
        return 0;
    /* Reason codes or indirect CRLs need extended CRL support */
    if (!(ctx->param->flags & X509_V_FLAG_EXTENDED_CRL_SUPPORT)) {
        if (crl->idp_flags & (IDP_INDIRECT | IDP_REASONS))
            return 0;
    } else if (crl->idp_flags & IDP_REASONS) {
        /* If no new reasons reject */
        if (!(crl->idp_reasons & ~tmp_reasons))
            return 0;
    }
    /* Don't process deltas at this stage */
    else if (crl->base_crl_number)
        return 0;
    /* If issuer name doesn't match certificate need indirect CRL */
    if (X509_NAME_cmp(X509_get_issuer_name(x), X509_CRL_get_issuer(crl))) {
        if (!(crl->idp_flags & IDP_INDIRECT))
            return 0;
    } else
        crl_score |= CRL_SCORE_ISSUER_NAME;

    if (!(crl->flags & EXFLAG_CRITICAL))
        crl_score |= CRL_SCORE_NOCRITICAL;

    /* Check expiry */
    if (check_crl_time(ctx, crl, 0))
        crl_score |= CRL_SCORE_TIME;

    /* Check authority key ID and locate certificate issuer */
    crl_akid_check(ctx, crl, pissuer, &crl_score);

    /* If we can't locate certificate issuer at this point forget it */

    if (!(crl_score & CRL_SCORE_AKID))
        return 0;

    /* Check cert for matching CRL distribution points */

    if (crl_crldp_check(x, crl, crl_score, &crl_reasons)) {
        /* If no new reasons reject */
        if (!(crl_reasons & ~tmp_reasons))
            return 0;
        tmp_reasons |= crl_reasons;
        crl_score |= CRL_SCORE_SCOPE;
    }

    *preasons = tmp_reasons;

    return crl_score;

}

static void crl_akid_check(X509_STORE_CTX *ctx, X509_CRL *crl,
                           X509 **pissuer, int *pcrl_score)
{
    X509 *crl_issuer = NULL;
    X509_NAME *cnm = X509_CRL_get_issuer(crl);
    int cidx = ctx->error_depth;
    int i;

    if (cidx != sk_X509_num(ctx->chain) - 1)
        cidx++;

    crl_issuer = sk_X509_value(ctx->chain, cidx);

    if (X509_check_akid(crl_issuer, crl->akid) == X509_V_OK) {
        if (*pcrl_score & CRL_SCORE_ISSUER_NAME) {
            *pcrl_score |= CRL_SCORE_AKID | CRL_SCORE_ISSUER_CERT;
            *pissuer = crl_issuer;
            return;
        }
    }

    for (cidx++; cidx < sk_X509_num(ctx->chain); cidx++) {
        crl_issuer = sk_X509_value(ctx->chain, cidx);
        if (X509_NAME_cmp(X509_get_subject_name(crl_issuer), cnm))
            continue;
        if (X509_check_akid(crl_issuer, crl->akid) == X509_V_OK) {
            *pcrl_score |= CRL_SCORE_AKID | CRL_SCORE_SAME_PATH;
            *pissuer = crl_issuer;
            return;
        }
    }

    /* Anything else needs extended CRL support */

    if (!(ctx->param->flags & X509_V_FLAG_EXTENDED_CRL_SUPPORT))
        return;

    /*
     * Otherwise the CRL issuer is not on the path. Look for it in the set of
     * untrusted certificates.
     */
    for (i = 0; i < sk_X509_num(ctx->untrusted); i++) {
        crl_issuer = sk_X509_value(ctx->untrusted, i);
        if (X509_NAME_cmp(X509_get_subject_name(crl_issuer), cnm))
            continue;
        if (X509_check_akid(crl_issuer, crl->akid) == X509_V_OK) {
            *pissuer = crl_issuer;
            *pcrl_score |= CRL_SCORE_AKID;
            return;
        }
    }
}

/*
 * Check the path of a CRL issuer certificate. This creates a new
 * X509_STORE_CTX and populates it with most of the parameters from the
 * parent. This could be optimised somewhat since a lot of path checking will
 * be duplicated by the parent, but this will rarely be used in practice.
 */

static int check_crl_path(X509_STORE_CTX *ctx, X509 *x)
{
    X509_STORE_CTX crl_ctx;
    int ret;
    /* Don't allow recursive CRL path validation */
    if (ctx->parent)
        return 0;
    if (!X509_STORE_CTX_init(&crl_ctx, ctx->ctx, x, ctx->untrusted))
        return -1;

    crl_ctx.crls = ctx->crls;
    /* Copy verify params across */
    X509_STORE_CTX_set0_param(&crl_ctx, ctx->param);

    crl_ctx.parent = ctx;
    crl_ctx.verify_cb = ctx->verify_cb;

    /* Verify CRL issuer */
    ret = X509_verify_cert(&crl_ctx);

    if (ret <= 0)
        goto err;

    /* Check chain is acceptable */

    ret = check_crl_chain(ctx, ctx->chain, crl_ctx.chain);
 err:
    X509_STORE_CTX_cleanup(&crl_ctx);
    return ret;
}

/*
 * RFC3280 says nothing about the relationship between CRL path and
 * certificate path, which could lead to situations where a certificate could
 * be revoked or validated by a CA not authorised to do so. RFC5280 is more
 * strict and states that the two paths must end in the same trust anchor,
 * though some discussions remain... until this is resolved we use the
 * RFC5280 version
 */

static int check_crl_chain(X509_STORE_CTX *ctx,
                           STACK_OF(X509) *cert_path,
                           STACK_OF(X509) *crl_path)
{
    X509 *cert_ta, *crl_ta;
    cert_ta = sk_X509_value(cert_path, sk_X509_num(cert_path) - 1);
    crl_ta = sk_X509_value(crl_path, sk_X509_num(crl_path) - 1);
    if (!X509_cmp(cert_ta, crl_ta))
        return 1;
    return 0;
}

/*-
 * Check for match between two dist point names: three separate cases.
 * 1. Both are relative names and compare X509_NAME types.
 * 2. One full, one relative. Compare X509_NAME to GENERAL_NAMES.
 * 3. Both are full names and compare two GENERAL_NAMES.
 * 4. One is NULL: automatic match.
 */

static int idp_check_dp(DIST_POINT_NAME *a, DIST_POINT_NAME *b)
{
    X509_NAME *nm = NULL;
    GENERAL_NAMES *gens = NULL;
    GENERAL_NAME *gena, *genb;
    int i, j;
    if (!a || !b)
        return 1;
    if (a->type == 1) {
        if (!a->dpname)
            return 0;
        /* Case 1: two X509_NAME */
        if (b->type == 1) {
            if (!b->dpname)
                return 0;
            if (!X509_NAME_cmp(a->dpname, b->dpname))
                return 1;
            else
                return 0;
        }
        /* Case 2: set name and GENERAL_NAMES appropriately */
        nm = a->dpname;
        gens = b->name.fullname;
    } else if (b->type == 1) {
        if (!b->dpname)
            return 0;
        /* Case 2: set name and GENERAL_NAMES appropriately */
        gens = a->name.fullname;
        nm = b->dpname;
    }

    /* Handle case 2 with one GENERAL_NAMES and one X509_NAME */
    if (nm) {
        for (i = 0; i < sk_GENERAL_NAME_num(gens); i++) {
            gena = sk_GENERAL_NAME_value(gens, i);
            if (gena->type != GEN_DIRNAME)
                continue;
            if (!X509_NAME_cmp(nm, gena->d.directoryName))
                return 1;
        }
        return 0;
    }

    /* Else case 3: two GENERAL_NAMES */

    for (i = 0; i < sk_GENERAL_NAME_num(a->name.fullname); i++) {
        gena = sk_GENERAL_NAME_value(a->name.fullname, i);
        for (j = 0; j < sk_GENERAL_NAME_num(b->name.fullname); j++) {
            genb = sk_GENERAL_NAME_value(b->name.fullname, j);
            if (!GENERAL_NAME_cmp(gena, genb))
                return 1;
        }
    }

    return 0;

}

static int crldp_check_crlissuer(DIST_POINT *dp, X509_CRL *crl, int crl_score)
{
    int i;
    X509_NAME *nm = X509_CRL_get_issuer(crl);
    /* If no CRLissuer return is successful iff don't need a match */
    if (!dp->CRLissuer)
        return ! !(crl_score & CRL_SCORE_ISSUER_NAME);
    for (i = 0; i < sk_GENERAL_NAME_num(dp->CRLissuer); i++) {
        GENERAL_NAME *gen = sk_GENERAL_NAME_value(dp->CRLissuer, i);
        if (gen->type != GEN_DIRNAME)
            continue;
        if (!X509_NAME_cmp(gen->d.directoryName, nm))
            return 1;
    }
    return 0;
}

/* Check CRLDP and IDP */

static int crl_crldp_check(X509 *x, X509_CRL *crl, int crl_score,
                           unsigned int *preasons)
{
    int i;
    if (crl->idp_flags & IDP_ONLYATTR)
        return 0;
    if (x->ex_flags & EXFLAG_CA) {
        if (crl->idp_flags & IDP_ONLYUSER)
            return 0;
    } else {
        if (crl->idp_flags & IDP_ONLYCA)
            return 0;
    }
    *preasons = crl->idp_reasons;
    for (i = 0; i < sk_DIST_POINT_num(x->crldp); i++) {
        DIST_POINT *dp = sk_DIST_POINT_value(x->crldp, i);
        if (crldp_check_crlissuer(dp, crl, crl_score)) {
            if (!crl->idp || idp_check_dp(dp->distpoint, crl->idp->distpoint)) {
                *preasons &= dp->dp_reasons;
                return 1;
            }
        }
    }
    if ((!crl->idp || !crl->idp->distpoint)
        && (crl_score & CRL_SCORE_ISSUER_NAME))
        return 1;
    return 0;
}

/*
 * Retrieve CRL corresponding to current certificate. If deltas enabled try
 * to find a delta CRL too
 */

static int get_crl_delta(X509_STORE_CTX *ctx,
                         X509_CRL **pcrl, X509_CRL **pdcrl, X509 *x)
{
    int ok;
    X509 *issuer = NULL;
    int crl_score = 0;
    unsigned int reasons;
    X509_CRL *crl = NULL, *dcrl = NULL;
    STACK_OF(X509_CRL) *skcrl;
    X509_NAME *nm = X509_get_issuer_name(x);
    reasons = ctx->current_reasons;
    ok = get_crl_sk(ctx, &crl, &dcrl,
                    &issuer, &crl_score, &reasons, ctx->crls);

    if (ok)
        goto done;

    /* Lookup CRLs from store */

    skcrl = ctx->lookup_crls(ctx, nm);

    /* If no CRLs found and a near match from get_crl_sk use that */
    if (!skcrl && crl)
        goto done;

    get_crl_sk(ctx, &crl, &dcrl, &issuer, &crl_score, &reasons, skcrl);

    sk_X509_CRL_pop_free(skcrl, X509_CRL_free);

 done:

    /* If we got any kind of CRL use it and return success */
    if (crl) {
        ctx->current_issuer = issuer;
        ctx->current_crl_score = crl_score;
        ctx->current_reasons = reasons;
        *pcrl = crl;
        *pdcrl = dcrl;
        return 1;
    }

    return 0;
}

/* Check CRL validity */
static int check_crl(X509_STORE_CTX *ctx, X509_CRL *crl)
{
    X509 *issuer = NULL;
    EVP_PKEY *ikey = NULL;
    int ok = 0, chnum, cnum;
    cnum = ctx->error_depth;
    chnum = sk_X509_num(ctx->chain) - 1;
    /* if we have an alternative CRL issuer cert use that */
    if (ctx->current_issuer)
        issuer = ctx->current_issuer;

    /*
     * Else find CRL issuer: if not last certificate then issuer is next
     * certificate in chain.
     */
    else if (cnum < chnum)
        issuer = sk_X509_value(ctx->chain, cnum + 1);
    else {
        issuer = sk_X509_value(ctx->chain, chnum);
        /* If not self signed, can't check signature */
        if (!ctx->check_issued(ctx, issuer, issuer)) {
            ctx->error = X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER;
            ok = ctx->verify_cb(0, ctx);
            if (!ok)
                goto err;
        }
    }

    if (issuer) {
        /*
         * Skip most tests for deltas because they have already been done
         */
        if (!crl->base_crl_number) {
            /* Check for cRLSign bit if keyUsage present */
            if ((issuer->ex_flags & EXFLAG_KUSAGE) &&
                !(issuer->ex_kusage & KU_CRL_SIGN)) {
                ctx->error = X509_V_ERR_KEYUSAGE_NO_CRL_SIGN;
                ok = ctx->verify_cb(0, ctx);
                if (!ok)
                    goto err;
            }

            if (!(ctx->current_crl_score & CRL_SCORE_SCOPE)) {
                ctx->error = X509_V_ERR_DIFFERENT_CRL_SCOPE;
                ok = ctx->verify_cb(0, ctx);
                if (!ok)
                    goto err;
            }

            if (!(ctx->current_crl_score & CRL_SCORE_SAME_PATH)) {
                if (check_crl_path(ctx, ctx->current_issuer) <= 0) {
                    ctx->error = X509_V_ERR_CRL_PATH_VALIDATION_ERROR;
                    ok = ctx->verify_cb(0, ctx);
                    if (!ok)
                        goto err;
                }
            }

            if (crl->idp_flags & IDP_INVALID) {
                ctx->error = X509_V_ERR_INVALID_EXTENSION;
                ok = ctx->verify_cb(0, ctx);
                if (!ok)
                    goto err;
            }

        }

        if (!(ctx->current_crl_score & CRL_SCORE_TIME)) {
            ok = check_crl_time(ctx, crl, 1);
            if (!ok)
                goto err;
        }

        /* Attempt to get issuer certificate public key */
        ikey = X509_get0_pubkey(issuer);

        if (!ikey) {
            ctx->error = X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY;
            ok = ctx->verify_cb(0, ctx);
            if (!ok)
                goto err;
        } else {
            int rv;
            rv = X509_CRL_check_suiteb(crl, ikey, ctx->param->flags);
            if (rv != X509_V_OK) {
                ctx->error = rv;
                ok = ctx->verify_cb(0, ctx);
                if (!ok)
                    goto err;
            }
            /* Verify CRL signature */
            if (X509_CRL_verify(crl, ikey) <= 0) {
                ctx->error = X509_V_ERR_CRL_SIGNATURE_FAILURE;
                ok = ctx->verify_cb(0, ctx);
                if (!ok)
                    goto err;
            }
        }
    }

    ok = 1;

 err:
    return ok;
}

/* Check certificate against CRL */
static int cert_crl(X509_STORE_CTX *ctx, X509_CRL *crl, X509 *x)
{
    int ok;
    X509_REVOKED *rev;
    /*
     * The rules changed for this... previously if a CRL contained unhandled
     * critical extensions it could still be used to indicate a certificate
     * was revoked. This has since been changed since critical extension can
     * change the meaning of CRL entries.
     */
    if (!(ctx->param->flags & X509_V_FLAG_IGNORE_CRITICAL)
        && (crl->flags & EXFLAG_CRITICAL)) {
        ctx->error = X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION;
        ok = ctx->verify_cb(0, ctx);
        if (!ok)
            return 0;
    }
    /*
     * Look for serial number of certificate in CRL If found make sure reason
     * is not removeFromCRL.
     */
    if (X509_CRL_get0_by_cert(crl, &rev, x)) {
        if (rev->reason == CRL_REASON_REMOVE_FROM_CRL)
            return 2;
        ctx->error = X509_V_ERR_CERT_REVOKED;
        ok = ctx->verify_cb(0, ctx);
        if (!ok)
            return 0;
    }

    return 1;
}

static int check_policy(X509_STORE_CTX *ctx)
{
    int ret;
    if (ctx->parent)
        return 1;
    ret = X509_policy_check(&ctx->tree, &ctx->explicit_policy, ctx->chain,
                            ctx->param->policies, ctx->param->flags);
    if (ret == 0) {
        X509err(X509_F_CHECK_POLICY, ERR_R_MALLOC_FAILURE);
        return 0;
    }
    /* Invalid or inconsistent extensions */
    if (ret == -1) {
        /*
         * Locate certificates with bad extensions and notify callback.
         */
        X509 *x;
        int i;
        for (i = 1; i < sk_X509_num(ctx->chain); i++) {
            x = sk_X509_value(ctx->chain, i);
            if (!(x->ex_flags & EXFLAG_INVALID_POLICY))
                continue;
            ctx->current_cert = x;
            ctx->error = X509_V_ERR_INVALID_POLICY_EXTENSION;
            if (!ctx->verify_cb(0, ctx))
                return 0;
        }
        return 1;
    }
    if (ret == -2) {
        ctx->current_cert = NULL;
        ctx->error = X509_V_ERR_NO_EXPLICIT_POLICY;
        return ctx->verify_cb(0, ctx);
    }

    if (ctx->param->flags & X509_V_FLAG_NOTIFY_POLICY) {
        ctx->current_cert = NULL;
        ctx->error = X509_V_OK;
        if (!ctx->verify_cb(2, ctx))
            return 0;
    }

    return 1;
}

int x509_check_cert_time(X509_STORE_CTX *ctx, X509 *x, int quiet)
{
    time_t *ptime;
    int i;

    if (ctx->param->flags & X509_V_FLAG_USE_CHECK_TIME)
        ptime = &ctx->param->check_time;
    else if (ctx->param->flags & X509_V_FLAG_NO_CHECK_TIME)
        return 1;
    else
        ptime = NULL;

    i = X509_cmp_time(X509_get_notBefore(x), ptime);
    if (i == 0) {
        if (quiet)
            return 0;
        ctx->error = X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD;
        ctx->current_cert = x;
        if (!ctx->verify_cb(0, ctx))
            return 0;
    }

    if (i > 0) {
        if (quiet)
            return 0;
        ctx->error = X509_V_ERR_CERT_NOT_YET_VALID;
        ctx->current_cert = x;
        if (!ctx->verify_cb(0, ctx))
            return 0;
    }

    i = X509_cmp_time(X509_get_notAfter(x), ptime);
    if (i == 0) {
        if (quiet)
            return 0;
        ctx->error = X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD;
        ctx->current_cert = x;
        if (!ctx->verify_cb(0, ctx))
            return 0;
    }

    if (i < 0) {
        if (quiet)
            return 0;
        ctx->error = X509_V_ERR_CERT_HAS_EXPIRED;
        ctx->current_cert = x;
        if (!ctx->verify_cb(0, ctx))
            return 0;
    }

    return 1;
}

static int internal_verify(X509_STORE_CTX *ctx)
{
    int ok = 0, n;
    X509 *xs, *xi;
    EVP_PKEY *pkey = NULL;
    int (*cb) (int xok, X509_STORE_CTX *xctx);

    cb = ctx->verify_cb;

    n = sk_X509_num(ctx->chain) - 1;
    ctx->error_depth = n;
    xi = sk_X509_value(ctx->chain, n);

    /*
     * With DANE-verified bare public key TA signatures, it remains only to
     * check the timestamps of the top certificate.  We report the issuer as
     * NULL, since all we have is a bare key.
     */
    if (ctx->bare_ta_signed) {
        xs = xi;
        xi = NULL;
        goto check_cert;
    }

    if (ctx->check_issued(ctx, xi, xi))
        xs = xi;
    else {
        if (ctx->param->flags & X509_V_FLAG_PARTIAL_CHAIN) {
            xs = xi;
            goto check_cert;
        }
        if (n <= 0) {
            ctx->error = X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE;
            ctx->current_cert = xi;
            ok = cb(0, ctx);
            goto end;
        } else {
            n--;
            ctx->error_depth = n;
            xs = sk_X509_value(ctx->chain, n);
        }
    }

    /*
     * Do not clear ctx->error=0, it must be "sticky", only the user's callback
     * is allowed to reset errors (at its own peril).
     */
    while (n >= 0) {
        ctx->error_depth = n;

        /*
         * Skip signature check for self signed certificates unless
         * explicitly asked for. It doesn't add any security and just wastes
         * time.
         */
        if (xs != xi || (ctx->param->flags & X509_V_FLAG_CHECK_SS_SIGNATURE)) {
            if ((pkey = X509_get0_pubkey(xi)) == NULL) {
                ctx->error = X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY;
                ctx->current_cert = xi;
                ok = (*cb) (0, ctx);
                if (!ok)
                    goto end;
            } else if (X509_verify(xs, pkey) <= 0) {
                ctx->error = X509_V_ERR_CERT_SIGNATURE_FAILURE;
                ctx->current_cert = xs;
                ok = (*cb) (0, ctx);
                if (!ok)
                    goto end;
            }
        }

 check_cert:
        ok = x509_check_cert_time(ctx, xs, 0);
        if (!ok)
            goto end;

        /* The last error (if any) is still in the error value */
        ctx->current_issuer = xi;
        ctx->current_cert = xs;
        ok = (*cb) (1, ctx);
        if (!ok)
            goto end;

        n--;
        if (n >= 0) {
            xi = xs;
            xs = sk_X509_value(ctx->chain, n);
        }
    }
    ok = 1;
 end:
    return ok;
}

int X509_cmp_current_time(const ASN1_TIME *ctm)
{
    return X509_cmp_time(ctm, NULL);
}

int X509_cmp_time(const ASN1_TIME *ctm, time_t *cmp_time)
{
    char *str;
    ASN1_TIME atm;
    long offset;
    char buff1[24], buff2[24], *p;
    int i, j, remaining;

    p = buff1;
    remaining = ctm->length;
    str = (char *)ctm->data;
    /*
     * Note that the following (historical) code allows much more slack in the
     * time format than RFC5280. In RFC5280, the representation is fixed:
     * UTCTime: YYMMDDHHMMSSZ
     * GeneralizedTime: YYYYMMDDHHMMSSZ
     */
    if (ctm->type == V_ASN1_UTCTIME) {
        /* YYMMDDHHMM[SS]Z or YYMMDDHHMM[SS](+-)hhmm */
        int min_length = sizeof("YYMMDDHHMMZ") - 1;
        int max_length = sizeof("YYMMDDHHMMSS+hhmm") - 1;
        if (remaining < min_length || remaining > max_length)
            return 0;
        memcpy(p, str, 10);
        p += 10;
        str += 10;
        remaining -= 10;
    } else {
        /* YYYYMMDDHHMM[SS[.fff]]Z or YYYYMMDDHHMM[SS[.f[f[f]]]](+-)hhmm */
        int min_length = sizeof("YYYYMMDDHHMMZ") - 1;
        int max_length = sizeof("YYYYMMDDHHMMSS.fff+hhmm") - 1;
        if (remaining < min_length || remaining > max_length)
            return 0;
        memcpy(p, str, 12);
        p += 12;
        str += 12;
        remaining -= 12;
    }

    if ((*str == 'Z') || (*str == '-') || (*str == '+')) {
        *(p++) = '0';
        *(p++) = '0';
    } else {
        /* SS (seconds) */
        if (remaining < 2)
            return 0;
        *(p++) = *(str++);
        *(p++) = *(str++);
        remaining -= 2;
        /*
         * Skip any (up to three) fractional seconds...
         * TODO(emilia): in RFC5280, fractional seconds are forbidden.
         * Can we just kill them altogether?
         */
        if (remaining && *str == '.') {
            str++;
            remaining--;
            for (i = 0; i < 3 && remaining; i++, str++, remaining--) {
                if (*str < '0' || *str > '9')
                    break;
            }
        }

    }
    *(p++) = 'Z';
    *(p++) = '\0';

    /* We now need either a terminating 'Z' or an offset. */
    if (!remaining)
        return 0;
    if (*str == 'Z') {
        if (remaining != 1)
            return 0;
        offset = 0;
    } else {
        /* (+-)HHMM */
        if ((*str != '+') && (*str != '-'))
            return 0;
        /* Historical behaviour: the (+-)hhmm offset is forbidden in RFC5280. */
        if (remaining != 5)
            return 0;
        if (str[1] < '0' || str[1] > '9' || str[2] < '0' || str[2] > '9' ||
            str[3] < '0' || str[3] > '9' || str[4] < '0' || str[4] > '9')
            return 0;
        offset = ((str[1] - '0') * 10 + (str[2] - '0')) * 60;
        offset += (str[3] - '0') * 10 + (str[4] - '0');
        if (*str == '-')
            offset = -offset;
    }
    atm.type = ctm->type;
    atm.flags = 0;
    atm.length = sizeof(buff2);
    atm.data = (unsigned char *)buff2;

    if (X509_time_adj(&atm, offset * 60, cmp_time) == NULL)
        return 0;

    if (ctm->type == V_ASN1_UTCTIME) {
        i = (buff1[0] - '0') * 10 + (buff1[1] - '0');
        if (i < 50)
            i += 100;           /* cf. RFC 2459 */
        j = (buff2[0] - '0') * 10 + (buff2[1] - '0');
        if (j < 50)
            j += 100;

        if (i < j)
            return -1;
        if (i > j)
            return 1;
    }
    i = strcmp(buff1, buff2);
    if (i == 0)                 /* wait a second then return younger :-) */
        return -1;
    else
        return i;
}

ASN1_TIME *X509_gmtime_adj(ASN1_TIME *s, long adj)
{
    return X509_time_adj(s, adj, NULL);
}

ASN1_TIME *X509_time_adj(ASN1_TIME *s, long offset_sec, time_t *in_tm)
{
    return X509_time_adj_ex(s, 0, offset_sec, in_tm);
}

ASN1_TIME *X509_time_adj_ex(ASN1_TIME *s,
                            int offset_day, long offset_sec, time_t *in_tm)
{
    time_t t;

    if (in_tm)
        t = *in_tm;
    else
        time(&t);

    if (s && !(s->flags & ASN1_STRING_FLAG_MSTRING)) {
        if (s->type == V_ASN1_UTCTIME)
            return ASN1_UTCTIME_adj(s, t, offset_day, offset_sec);
        if (s->type == V_ASN1_GENERALIZEDTIME)
            return ASN1_GENERALIZEDTIME_adj(s, t, offset_day, offset_sec);
    }
    return ASN1_TIME_adj(s, t, offset_day, offset_sec);
}

int X509_get_pubkey_parameters(EVP_PKEY *pkey, STACK_OF(X509) *chain)
{
    EVP_PKEY *ktmp = NULL, *ktmp2;
    int i, j;

    if ((pkey != NULL) && !EVP_PKEY_missing_parameters(pkey))
        return 1;

    for (i = 0; i < sk_X509_num(chain); i++) {
        ktmp = X509_get0_pubkey(sk_X509_value(chain, i));
        if (ktmp == NULL) {
            X509err(X509_F_X509_GET_PUBKEY_PARAMETERS,
                    X509_R_UNABLE_TO_GET_CERTS_PUBLIC_KEY);
            return 0;
        }
        if (!EVP_PKEY_missing_parameters(ktmp))
            break;
    }
    if (ktmp == NULL) {
        X509err(X509_F_X509_GET_PUBKEY_PARAMETERS,
                X509_R_UNABLE_TO_FIND_PARAMETERS_IN_CHAIN);
        return 0;
    }

    /* first, populate the other certs */
    for (j = i - 1; j >= 0; j--) {
        ktmp2 = X509_get0_pubkey(sk_X509_value(chain, j));
        EVP_PKEY_copy_parameters(ktmp2, ktmp);
    }

    if (pkey != NULL)
        EVP_PKEY_copy_parameters(pkey, ktmp);
    return 1;
}

/* Make a delta CRL as the diff between two full CRLs */

X509_CRL *X509_CRL_diff(X509_CRL *base, X509_CRL *newer,
                        EVP_PKEY *skey, const EVP_MD *md, unsigned int flags)
{
    X509_CRL *crl = NULL;
    int i;
    STACK_OF(X509_REVOKED) *revs = NULL;
    /* CRLs can't be delta already */
    if (base->base_crl_number || newer->base_crl_number) {
        X509err(X509_F_X509_CRL_DIFF, X509_R_CRL_ALREADY_DELTA);
        return NULL;
    }
    /* Base and new CRL must have a CRL number */
    if (!base->crl_number || !newer->crl_number) {
        X509err(X509_F_X509_CRL_DIFF, X509_R_NO_CRL_NUMBER);
        return NULL;
    }
    /* Issuer names must match */
    if (X509_NAME_cmp(X509_CRL_get_issuer(base), X509_CRL_get_issuer(newer))) {
        X509err(X509_F_X509_CRL_DIFF, X509_R_ISSUER_MISMATCH);
        return NULL;
    }
    /* AKID and IDP must match */
    if (!crl_extension_match(base, newer, NID_authority_key_identifier)) {
        X509err(X509_F_X509_CRL_DIFF, X509_R_AKID_MISMATCH);
        return NULL;
    }
    if (!crl_extension_match(base, newer, NID_issuing_distribution_point)) {
        X509err(X509_F_X509_CRL_DIFF, X509_R_IDP_MISMATCH);
        return NULL;
    }
    /* Newer CRL number must exceed full CRL number */
    if (ASN1_INTEGER_cmp(newer->crl_number, base->crl_number) <= 0) {
        X509err(X509_F_X509_CRL_DIFF, X509_R_NEWER_CRL_NOT_NEWER);
        return NULL;
    }
    /* CRLs must verify */
    if (skey && (X509_CRL_verify(base, skey) <= 0 ||
                 X509_CRL_verify(newer, skey) <= 0)) {
        X509err(X509_F_X509_CRL_DIFF, X509_R_CRL_VERIFY_FAILURE);
        return NULL;
    }
    /* Create new CRL */
    crl = X509_CRL_new();
    if (crl == NULL || !X509_CRL_set_version(crl, 1))
        goto memerr;
    /* Set issuer name */
    if (!X509_CRL_set_issuer_name(crl, X509_CRL_get_issuer(newer)))
        goto memerr;

    if (!X509_CRL_set_lastUpdate(crl, X509_CRL_get_lastUpdate(newer)))
        goto memerr;
    if (!X509_CRL_set_nextUpdate(crl, X509_CRL_get_nextUpdate(newer)))
        goto memerr;

    /* Set base CRL number: must be critical */

    if (!X509_CRL_add1_ext_i2d(crl, NID_delta_crl, base->crl_number, 1, 0))
        goto memerr;

    /*
     * Copy extensions across from newest CRL to delta: this will set CRL
     * number to correct value too.
     */

    for (i = 0; i < X509_CRL_get_ext_count(newer); i++) {
        X509_EXTENSION *ext;
        ext = X509_CRL_get_ext(newer, i);
        if (!X509_CRL_add_ext(crl, ext, -1))
            goto memerr;
    }

    /* Go through revoked entries, copying as needed */

    revs = X509_CRL_get_REVOKED(newer);

    for (i = 0; i < sk_X509_REVOKED_num(revs); i++) {
        X509_REVOKED *rvn, *rvtmp;
        rvn = sk_X509_REVOKED_value(revs, i);
        /*
         * Add only if not also in base. TODO: need something cleverer here
         * for some more complex CRLs covering multiple CAs.
         */
        if (!X509_CRL_get0_by_serial(base, &rvtmp, &rvn->serialNumber)) {
            rvtmp = X509_REVOKED_dup(rvn);
            if (!rvtmp)
                goto memerr;
            if (!X509_CRL_add0_revoked(crl, rvtmp)) {
                X509_REVOKED_free(rvtmp);
                goto memerr;
            }
        }
    }
    /* TODO: optionally prune deleted entries */

    if (skey && md && !X509_CRL_sign(crl, skey, md))
        goto memerr;

    return crl;

 memerr:
    X509err(X509_F_X509_CRL_DIFF, ERR_R_MALLOC_FAILURE);
    X509_CRL_free(crl);
    return NULL;
}

int X509_STORE_CTX_set_ex_data(X509_STORE_CTX *ctx, int idx, void *data)
{
    return CRYPTO_set_ex_data(&ctx->ex_data, idx, data);
}

void *X509_STORE_CTX_get_ex_data(X509_STORE_CTX *ctx, int idx)
{
    return CRYPTO_get_ex_data(&ctx->ex_data, idx);
}

int X509_STORE_CTX_get_error(X509_STORE_CTX *ctx)
{
    return ctx->error;
}

void X509_STORE_CTX_set_error(X509_STORE_CTX *ctx, int err)
{
    ctx->error = err;
}

int X509_STORE_CTX_get_error_depth(X509_STORE_CTX *ctx)
{
    return ctx->error_depth;
}

X509 *X509_STORE_CTX_get_current_cert(X509_STORE_CTX *ctx)
{
    return ctx->current_cert;
}

STACK_OF(X509) *X509_STORE_CTX_get_chain(X509_STORE_CTX *ctx)
{
    return ctx->chain;
}

STACK_OF(X509) *X509_STORE_CTX_get1_chain(X509_STORE_CTX *ctx)
{
    if (!ctx->chain)
        return NULL;
    return X509_chain_up_ref(ctx->chain);
}

X509 *X509_STORE_CTX_get0_current_issuer(X509_STORE_CTX *ctx)
{
    return ctx->current_issuer;
}

X509_CRL *X509_STORE_CTX_get0_current_crl(X509_STORE_CTX *ctx)
{
    return ctx->current_crl;
}

X509_STORE_CTX *X509_STORE_CTX_get0_parent_ctx(X509_STORE_CTX *ctx)
{
    return ctx->parent;
}

void X509_STORE_CTX_set_cert(X509_STORE_CTX *ctx, X509 *x)
{
    ctx->cert = x;
}

void X509_STORE_CTX_set_chain(X509_STORE_CTX *ctx, STACK_OF(X509) *sk)
{
    ctx->untrusted = sk;
}

void X509_STORE_CTX_set0_crls(X509_STORE_CTX *ctx, STACK_OF(X509_CRL) *sk)
{
    ctx->crls = sk;
}

int X509_STORE_CTX_set_purpose(X509_STORE_CTX *ctx, int purpose)
{
    return X509_STORE_CTX_purpose_inherit(ctx, 0, purpose, 0);
}

int X509_STORE_CTX_set_trust(X509_STORE_CTX *ctx, int trust)
{
    return X509_STORE_CTX_purpose_inherit(ctx, 0, 0, trust);
}

/*
 * This function is used to set the X509_STORE_CTX purpose and trust values.
 * This is intended to be used when another structure has its own trust and
 * purpose values which (if set) will be inherited by the ctx. If they aren't
 * set then we will usually have a default purpose in mind which should then
 * be used to set the trust value. An example of this is SSL use: an SSL
 * structure will have its own purpose and trust settings which the
 * application can set: if they aren't set then we use the default of SSL
 * client/server.
 */

int X509_STORE_CTX_purpose_inherit(X509_STORE_CTX *ctx, int def_purpose,
                                   int purpose, int trust)
{
    int idx;
    /* If purpose not set use default */
    if (!purpose)
        purpose = def_purpose;
    /* If we have a purpose then check it is valid */
    if (purpose) {
        X509_PURPOSE *ptmp;
        idx = X509_PURPOSE_get_by_id(purpose);
        if (idx == -1) {
            X509err(X509_F_X509_STORE_CTX_PURPOSE_INHERIT,
                    X509_R_UNKNOWN_PURPOSE_ID);
            return 0;
        }
        ptmp = X509_PURPOSE_get0(idx);
        if (ptmp->trust == X509_TRUST_DEFAULT) {
            idx = X509_PURPOSE_get_by_id(def_purpose);
            if (idx == -1) {
                X509err(X509_F_X509_STORE_CTX_PURPOSE_INHERIT,
                        X509_R_UNKNOWN_PURPOSE_ID);
                return 0;
            }
            ptmp = X509_PURPOSE_get0(idx);
        }
        /* If trust not set then get from purpose default */
        if (!trust)
            trust = ptmp->trust;
    }
    if (trust) {
        idx = X509_TRUST_get_by_id(trust);
        if (idx == -1) {
            X509err(X509_F_X509_STORE_CTX_PURPOSE_INHERIT,
                    X509_R_UNKNOWN_TRUST_ID);
            return 0;
        }
    }

    if (purpose && !ctx->param->purpose)
        ctx->param->purpose = purpose;
    if (trust && !ctx->param->trust)
        ctx->param->trust = trust;
    return 1;
}

X509_STORE_CTX *X509_STORE_CTX_new(void)
{
    X509_STORE_CTX *ctx = OPENSSL_zalloc(sizeof(*ctx));

    if (ctx == NULL) {
        X509err(X509_F_X509_STORE_CTX_NEW, ERR_R_MALLOC_FAILURE);
        return NULL;
    }
    return ctx;
}

void X509_STORE_CTX_free(X509_STORE_CTX *ctx)
{
    if (!ctx)
        return;
    X509_STORE_CTX_cleanup(ctx);
    OPENSSL_free(ctx);
}

int X509_STORE_CTX_init(X509_STORE_CTX *ctx, X509_STORE *store, X509 *x509,
                        STACK_OF(X509) *chain)
{
    int ret = 1;

    ctx->ctx = store;
    ctx->current_method = 0;
    ctx->cert = x509;
    ctx->untrusted = chain;
    ctx->crls = NULL;
    ctx->num_untrusted = 0;
    ctx->other_ctx = NULL;
    ctx->valid = 0;
    ctx->chain = NULL;
    ctx->error = 0;
    ctx->explicit_policy = 0;
    ctx->error_depth = 0;
    ctx->current_cert = NULL;
    ctx->current_issuer = NULL;
    ctx->current_crl = NULL;
    ctx->current_crl_score = 0;
    ctx->current_reasons = 0;
    ctx->tree = NULL;
    ctx->parent = NULL;
    ctx->dane = NULL;
    ctx->bare_ta_signed = 0;
    /* Zero ex_data to make sure we're cleanup-safe */
    memset(&ctx->ex_data, 0, sizeof(ctx->ex_data));

    if (store) {
        ctx->verify_cb = store->verify_cb;
        /* Seems to always be 0 in OpenSSL, else must be idempotent */
        ctx->cleanup = store->cleanup;
    } else
        ctx->cleanup = 0;

    if (store && store->check_issued)
        ctx->check_issued = store->check_issued;
    else
        ctx->check_issued = check_issued;

    if (store && store->get_issuer)
        ctx->get_issuer = store->get_issuer;
    else
        ctx->get_issuer = X509_STORE_CTX_get1_issuer;

    if (store && store->verify_cb)
        ctx->verify_cb = store->verify_cb;
    else
        ctx->verify_cb = null_callback;

    if (store && store->verify)
        ctx->verify = store->verify;
    else
        ctx->verify = internal_verify;

    if (store && store->check_revocation)
        ctx->check_revocation = store->check_revocation;
    else
        ctx->check_revocation = check_revocation;

    if (store && store->get_crl)
        ctx->get_crl = store->get_crl;
    else
        ctx->get_crl = NULL;

    if (store && store->check_crl)
        ctx->check_crl = store->check_crl;
    else
        ctx->check_crl = check_crl;

    if (store && store->cert_crl)
        ctx->cert_crl = store->cert_crl;
    else
        ctx->cert_crl = cert_crl;

    if (store && store->lookup_certs)
        ctx->lookup_certs = store->lookup_certs;
    else
        ctx->lookup_certs = X509_STORE_get1_certs;

    if (store && store->lookup_crls)
        ctx->lookup_crls = store->lookup_crls;
    else
        ctx->lookup_crls = X509_STORE_get1_crls;

    ctx->check_policy = check_policy;

    ctx->param = X509_VERIFY_PARAM_new();
    if (ctx->param == NULL) {
        X509err(X509_F_X509_STORE_CTX_INIT, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    /*
     * Inherit callbacks and flags from X509_STORE if not set use defaults.
     */
    if (store)
        ret = X509_VERIFY_PARAM_inherit(ctx->param, store->param);
    else
        ctx->param->inh_flags |= X509_VP_FLAG_DEFAULT | X509_VP_FLAG_ONCE;

    if (ret)
        ret = X509_VERIFY_PARAM_inherit(ctx->param,
                                        X509_VERIFY_PARAM_lookup("default"));

    if (ret == 0) {
        X509err(X509_F_X509_STORE_CTX_INIT, ERR_R_MALLOC_FAILURE);
        goto err;
    }

    if (CRYPTO_new_ex_data(CRYPTO_EX_INDEX_X509_STORE_CTX, ctx,
                           &ctx->ex_data))
        return 1;
    X509err(X509_F_X509_STORE_CTX_INIT, ERR_R_MALLOC_FAILURE);

 err:
    /*
     * On error clean up allocated storage, if the store context was not
     * allocated with X509_STORE_CTX_new() this is our last chance to do so.
     */
    X509_STORE_CTX_cleanup(ctx);
    return 0;
}

/*
 * Set alternative lookup method: just a STACK of trusted certificates. This
 * avoids X509_STORE nastiness where it isn't needed.
 */

void X509_STORE_CTX_trusted_stack(X509_STORE_CTX *ctx, STACK_OF(X509) *sk)
{
    ctx->other_ctx = sk;
    ctx->get_issuer = get_issuer_sk;
    ctx->lookup_certs = lookup_certs_sk;
}

void X509_STORE_CTX_cleanup(X509_STORE_CTX *ctx)
{
    /*
     * We need to be idempotent because, unfortunately, free() also calls
     * cleanup(), so the natural call sequence new(), init(), cleanup(), free()
     * calls cleanup() for the same object twice!  Thus we must zero the
     * pointers below after they're freed!
     */
    /* Seems to always be 0 in OpenSSL, do this at most once. */
    if (ctx->cleanup != NULL) {
        ctx->cleanup(ctx);
        ctx->cleanup = NULL;
    }
    if (ctx->param != NULL) {
        if (ctx->parent == NULL)
            X509_VERIFY_PARAM_free(ctx->param);
        ctx->param = NULL;
    }
    X509_policy_tree_free(ctx->tree);
    ctx->tree = NULL;
    sk_X509_pop_free(ctx->chain, X509_free);
    ctx->chain = NULL;
    CRYPTO_free_ex_data(CRYPTO_EX_INDEX_X509_STORE_CTX, ctx, &(ctx->ex_data));
    memset(&ctx->ex_data, 0, sizeof(ctx->ex_data));
}

void X509_STORE_CTX_set_depth(X509_STORE_CTX *ctx, int depth)
{
    X509_VERIFY_PARAM_set_depth(ctx->param, depth);
}

void X509_STORE_CTX_set_flags(X509_STORE_CTX *ctx, unsigned long flags)
{
    X509_VERIFY_PARAM_set_flags(ctx->param, flags);
}

void X509_STORE_CTX_set_time(X509_STORE_CTX *ctx, unsigned long flags,
                             time_t t)
{
    X509_VERIFY_PARAM_set_time(ctx->param, t);
}

void X509_STORE_CTX_set_verify_cb(X509_STORE_CTX *ctx,
                                  int (*verify_cb) (int, X509_STORE_CTX *))
{
    ctx->verify_cb = verify_cb;
}

X509_POLICY_TREE *X509_STORE_CTX_get0_policy_tree(X509_STORE_CTX *ctx)
{
    return ctx->tree;
}

int X509_STORE_CTX_get_explicit_policy(X509_STORE_CTX *ctx)
{
    return ctx->explicit_policy;
}

int X509_STORE_CTX_get_num_untrusted(X509_STORE_CTX *ctx)
{
    return ctx->num_untrusted;
}

int X509_STORE_CTX_set_default(X509_STORE_CTX *ctx, const char *name)
{
    const X509_VERIFY_PARAM *param;
    param = X509_VERIFY_PARAM_lookup(name);
    if (!param)
        return 0;
    return X509_VERIFY_PARAM_inherit(ctx->param, param);
}

X509_VERIFY_PARAM *X509_STORE_CTX_get0_param(X509_STORE_CTX *ctx)
{
    return ctx->param;
}

void X509_STORE_CTX_set0_param(X509_STORE_CTX *ctx, X509_VERIFY_PARAM *param)
{
    X509_VERIFY_PARAM_free(ctx->param);
    ctx->param = param;
}

void X509_STORE_CTX_set0_dane(X509_STORE_CTX *ctx, struct dane_st *dane)
{
    ctx->dane = dane;
}

static unsigned char *dane_i2d(
    X509 *cert,
    uint8_t selector,
    unsigned int *i2dlen)
{
    unsigned char *buf = NULL;
    int len;

    /*
     * Extract ASN.1 DER form of certificate or public key.
     */
    switch (selector) {
    case DANETLS_SELECTOR_CERT:
        len = i2d_X509(cert, &buf);
        break;
    case DANETLS_SELECTOR_SPKI:
        len = i2d_X509_PUBKEY(X509_get_X509_PUBKEY(cert), &buf);
        break;
    default:
        X509err(X509_F_DANE_I2D, X509_R_BAD_SELECTOR);
        return NULL;
    }

    if (len < 0 || buf == NULL) {
        X509err(X509_F_DANE_I2D, ERR_R_MALLOC_FAILURE);
        return NULL;
    }

    *i2dlen = (unsigned int)len;
    return buf;
}

#define DANETLS_NONE 256        /* impossible uint8_t */

static int dane_match(X509_STORE_CTX *ctx, X509 *cert, int depth)
{
    struct dane_st *dane = (struct dane_st *)ctx->dane;
    unsigned usage = DANETLS_NONE;
    unsigned selector = DANETLS_NONE;
    unsigned ordinal = DANETLS_NONE;
    unsigned mtype = DANETLS_NONE;
    unsigned char *i2dbuf = NULL;
    unsigned int i2dlen = 0;
    unsigned char mdbuf[EVP_MAX_MD_SIZE];
    unsigned char *cmpbuf = NULL;
    unsigned int cmplen = 0;
    int i;
    int recnum;
    int matched = 0;
    danetls_record *t = NULL;
    uint32_t mask;

    mask = (depth == 0) ? DANETLS_EE_MASK : DANETLS_TA_MASK;

    /*
     * The trust store is not applicable with DANE-TA(2)
     */
    if (depth >= ctx->num_untrusted)
        mask &= DANETLS_PKIX_MASK;

    /*
     * If we've previously matched a PKIX-?? record, no need to test any
     * furher PKIX-?? records,  it remains to just build the PKIX chain.
     * Had the match been a DANE-?? record, we'd be done already.
     */
    if (dane->mdpth >= 0)
        mask &= ~DANETLS_PKIX_MASK;

    /*-
     * https://tools.ietf.org/html/rfc7671#section-5.1
     * https://tools.ietf.org/html/rfc7671#section-5.2
     * https://tools.ietf.org/html/rfc7671#section-5.3
     * https://tools.ietf.org/html/rfc7671#section-5.4
     *
     * We handle DANE-EE(3) records first as they require no chain building
     * and no expiration or hostname checks.  We also process digests with
     * higher ordinals first and ignore lower priorities except Full(0) which
     * is always processed (last).  If none match, we then process PKIX-EE(1).
     *
     * NOTE: This relies on DANE usages sorting before the corresponding PKIX
     * usages in SSL_dane_tlsa_add(), and also on descending sorting of digest
     * priorities.  See twin comment in ssl/ssl_lib.c.
     *
     * We expect that most TLSA RRsets will have just a single usage, so we
     * don't go out of our way to cache multiple selector-specific i2d buffers
     * across usages, but if the selector happens to remain the same as switch
     * usages, that's OK.  Thus, a set of "3 1 1", "3 0 1", "1 1 1", "1 0 1",
     * records would result in us generating each of the certificate and public
     * key DER forms twice, but more typically we'd just see multiple "3 1 1"
     * or multiple "3 0 1" records.
     *
     * As soon as we find a match at any given depth, we stop, because either
     * we've matched a DANE-?? record and the peer is authenticated, or, after
     * exhausing all DANE-?? records, we've matched a PKIX-?? record, which is
     * sufficient for DANE, and what remains to do is ordinary PKIX validation.
     */
    recnum = (dane->umask & mask) ? sk_danetls_record_num(dane->trecs) : 0;
    for (i = 0; matched == 0 && i < recnum; ++i) {
        t = sk_danetls_record_value(dane->trecs, i);
        if ((DANETLS_USAGE_BIT(t->usage) & mask) == 0)
            continue;
        if (t->usage != usage) {
            usage = t->usage;

            /* Reset digest agility for each usage/selector pair */
            mtype = DANETLS_NONE;
            ordinal = dane->dctx->mdord[t->mtype];
        }
        if (t->selector != selector) {
            selector = t->selector;

            /* Update per-selector state */
            OPENSSL_free(i2dbuf);
            i2dbuf = dane_i2d(cert, selector, &i2dlen);
            if (i2dbuf == NULL)
                return -1;

            /* Reset digest agility for each usage/selector pair */
            mtype = DANETLS_NONE;
            ordinal = dane->dctx->mdord[t->mtype];
        } else if (t->mtype != DANETLS_MATCHING_FULL) {
            /*-
             * Digest agility:
             *
             *     <https://tools.ietf.org/html/rfc7671#section-9>
             *
             * For a fixed selector, after processing all records with the
             * highest mtype ordinal, ignore all mtypes with lower ordinals
             * other than "Full".
             */
            if (dane->dctx->mdord[t->mtype] < ordinal)
                continue;
        }

        /*
         * Each time we hit a (new selector or) mtype, re-compute the relevant
         * digest, more complex caching is not worth the code space.
         */
        if (t->mtype != mtype) {
            const EVP_MD *md = dane->dctx->mdevp[mtype = t->mtype];
            cmpbuf = i2dbuf;
            cmplen = i2dlen;

            if (md != NULL) {
		cmpbuf = mdbuf;
		if (!EVP_Digest(i2dbuf, i2dlen, cmpbuf, &cmplen, md, 0)) {
		    matched = -1;
                    break;
                }
            }
        }

        /*
         * Squirrel away the certificate and depth if we have a match.  Any
         * DANE match is dispositive, but with PKIX we still need to build a
         * full chain.
         */
        if (cmplen == t->dlen &&
            memcmp(cmpbuf, t->data, cmplen) == 0) {
            if (DANETLS_USAGE_BIT(usage) & DANETLS_DANE_MASK)
                matched = 1;
            if (matched || dane->mdpth < 0) {
                dane->mdpth = depth;
                dane->mtlsa = t;
                OPENSSL_free(dane->mcert);
                dane->mcert = cert;
                X509_up_ref(cert);
            }
            break;
        }
    }

    /* Clear the one-element DER cache */
    OPENSSL_free(i2dbuf);
    return matched;
}

static int check_dane_issuer(X509_STORE_CTX *ctx, int depth)
{
    struct dane_st *dane = (struct dane_st *)ctx->dane;
    int matched = 0;
    X509 *cert;

    if (!DANETLS_HAS_TA(dane) || depth == 0)
        return  X509_TRUST_UNTRUSTED;

    /*
     * Record any DANE trust anchor matches, for the first depth to test, if
     * there's one at that depth. (This'll be false for length 1 chains looking
     * for an exact match for the leaf certificate).
     */
    cert = sk_X509_value(ctx->chain, depth);
    if (cert != NULL && (matched = dane_match(ctx, cert, depth)) < 0)
        return  X509_TRUST_REJECTED;
    if (matched > 0) {
        ctx->num_untrusted = depth - 1;
        return  X509_TRUST_TRUSTED;
    }

    return  X509_TRUST_UNTRUSTED;
}

static int check_dane_pkeys(X509_STORE_CTX *ctx)
{
    struct dane_st *dane = (struct dane_st *)ctx->dane;
    danetls_record *t;
    int num = ctx->num_untrusted;
    X509 *cert = sk_X509_value(ctx->chain, num - 1);
    int recnum = sk_danetls_record_num(dane->trecs);
    int i;

    for (i = 0; i < recnum; ++i) {
        t = sk_danetls_record_value(dane->trecs, i);
        if (t->usage != DANETLS_USAGE_DANE_TA ||
            t->selector != DANETLS_SELECTOR_SPKI ||
            t->mtype != DANETLS_MATCHING_FULL ||
            X509_verify(cert, t->spki) <= 0)
            continue;

        /* Clear PKIX-?? matches that failed to panned out to a full chain */
        X509_free(dane->mcert);
        dane->mcert = NULL;

        /* Record match via a bare TA public key */
        ctx->bare_ta_signed = 1;
        dane->mdpth = num - 1;
        dane->mtlsa = t;

        /* Prune any excess chain certificates */
        num = sk_X509_num(ctx->chain);
        for (; num > ctx->num_untrusted; --num)
            X509_free(sk_X509_pop(ctx->chain));

        return X509_TRUST_TRUSTED;
    }

    return X509_TRUST_UNTRUSTED;
}

static void dane_reset(struct dane_st *dane)
{
    /*
     * Reset state to verify another chain, or clear after failure.
     */
    X509_free(dane->mcert);
    dane->mcert = NULL;
    dane->mtlsa = NULL;
    dane->mdpth = -1;
    dane->pdpth = -1;
}

static int dane_verify(X509_STORE_CTX *ctx)
{
    X509 *cert = ctx->cert;
    int (*cb)(int xok, X509_STORE_CTX *xctx) = ctx->verify_cb;
    struct dane_st *dane = (struct dane_st *)ctx->dane;
    int matched;
    int done;

    dane_reset(dane);

    matched = dane_match(ctx, ctx->cert, 0);
    done = matched != 0 || (!DANETLS_HAS_TA(dane) && dane->mdpth < 0);

    if (done)
        X509_get_pubkey_parameters(NULL, ctx->chain);

    if (matched > 0) {
        ctx->error_depth = 0;
        ctx->current_cert = cert;
        return cb(1, ctx);
    }

    if (matched < 0) {
        ctx->error_depth = 0;
        ctx->current_cert = cert;
        ctx->error = X509_V_ERR_OUT_OF_MEM;
        return -1;
    }

    if (done) {
        /* Fail early, TA-based success is not possible */
        ctx->current_cert = cert;
        ctx->error_depth = 0;
        ctx->error = X509_V_ERR_CERT_UNTRUSTED;
        return cb(0, ctx);
    }

    /*
     * Chain verification for usages 0/1/2.  TLSA record matching of depth > 0
     * certificates happens in-line with building the rest of the chain.
     */
    return verify_chain(ctx);
}

static int build_chain(X509_STORE_CTX *ctx)
{
    struct dane_st *dane = (struct dane_st *)ctx->dane;
    int (*cb) (int, X509_STORE_CTX *) = ctx->verify_cb;
    int num = sk_X509_num(ctx->chain);
    X509 *cert = sk_X509_value(ctx->chain, num - 1);
    int ss = cert_self_signed(cert);
    STACK_OF(X509) *sktmp = NULL;
    unsigned int search;
    int may_trusted = 0;
    int may_alternate = 0;
    int trust = X509_TRUST_UNTRUSTED;
    int alt_untrusted = 0;
    int depth;
    int ok = 0;
    int i;

    /* Our chain starts with a single untrusted element. */
    OPENSSL_assert(num == 1 && ctx->num_untrusted == num);

#define S_DOUNTRUSTED      (1 << 0)     /* Search untrusted chain */
#define S_DOTRUSTED        (1 << 1)     /* Search trusted store */
#define S_DOALTERNATE      (1 << 2)     /* Retry with pruned alternate chain */
    /*
     * Set up search policy, untrusted if possible, trusted-first if enabled.
     * If we're doing DANE and not doing PKIX-TA/PKIX-EE, we never look in the
     * trust_store, otherwise we might look there first.  If not trusted-first,
     * and alternate chains are not disabled, try building an alternate chain
     * if no luck with untrusted first.
     */
    search = (ctx->untrusted != NULL) ? S_DOUNTRUSTED : 0;
    if (DANETLS_HAS_PKIX(dane) || !DANETLS_HAS_DANE(dane)) {
        if (search == 0 || ctx->param->flags & X509_V_FLAG_TRUSTED_FIRST)
            search |= S_DOTRUSTED;
        else if (!(ctx->param->flags & X509_V_FLAG_NO_ALT_CHAINS))
            may_alternate = 1;
        may_trusted = 1;
    }

    /*
     * Shallow-copy the stack of untrusted certificates (with TLS, this is
     * typically the content of the peer's certificate message) so can make
     * multiple passes over it, while free to remove elements as we go.
     */
    if (ctx->untrusted && (sktmp = sk_X509_dup(ctx->untrusted)) == NULL) {
        X509err(X509_F_BUILD_CHAIN, ERR_R_MALLOC_FAILURE);
        return 0;
    }

    /* Include any untrusted full certificates from DNS */
    if (DANETLS_ENABLED(dane) && dane->certs != NULL) {
        for (i = 0; i < sk_X509_num(dane->certs); ++i) {
            if (!sk_X509_push(sktmp, sk_X509_value(dane->certs, i))) {
                sk_X509_free(sktmp);
                X509err(X509_F_BUILD_CHAIN, ERR_R_MALLOC_FAILURE);
                return 0;
            }
        }
    }

    /*
     * Still absurdly large, but arithmetically safe, a lower hard upper bound
     * might be reasonable.
     */
    if (ctx->param->depth > INT_MAX/2)
        ctx->param->depth = INT_MAX/2;

    /*
     * Try to Extend the chain until we reach an ultimately trusted issuer.
     * Build chains up to one longer the limit, later fail if we hit the limit,
     * with an X509_V_ERR_CERT_CHAIN_TOO_LONG error code.
     */
    depth = ctx->param->depth + 1;

    while (search != 0) {
        X509 *x;
        X509 *xtmp = NULL;

        /*
         * Look in the trust store if enabled for first lookup, or we've run
         * out of untrusted issuers and search here is not disabled.  When
         * we exceed the depth limit, we simulate absence of a match.
         */
        if ((search & S_DOTRUSTED) != 0) {
            STACK_OF(X509) *hide = ctx->chain;

            i = num = sk_X509_num(ctx->chain);
            if ((search & S_DOALTERNATE) != 0) {
                /*
                 * As high up the chain as we can, look for an alternative
                 * trusted issuer of an untrusted certificate that currently
                 * has an untrusted issuer.  We use the alt_untrusted variable
                 * to track how far up the chain we find the first match.  It
                 * is only if and when we find a match, that we prune the chain
                 * and reset ctx->num_untrusted to the reduced count of
                 * untrusted certificates.  While we're searching for such a
                 * match (which may never be found), it is neither safe nor
                 * wise to preemptively modify either the chain or
                 * ctx->num_untrusted.
                 *
                 * Note, like ctx->num_untrusted, alt_untrusted is a count of
                 * untrusted certificates, not a "depth".
                 */
                i = alt_untrusted;
            }
            x = sk_X509_value(ctx->chain, i-1);

            /* Suppress duplicate suppression */
            ctx->chain = NULL;
            ok = (depth < num) ? 0 : ctx->get_issuer(&xtmp, ctx, x);
            ctx->chain = hide;

            if (ok < 0) {
                trust = X509_TRUST_REJECTED;
                search = 0;
                continue;
            }

            if (ok > 0) {
                /*
                 * Alternative trusted issuer for a mid-chain untrusted cert?
                 * Pop the untrusted cert's successors and retry.  We might now
                 * be able to complete a valid chain via the trust store.  Note
                 * that despite the current trust-store match we might still
                 * fail complete the chain to a suitable trust-anchor, in which
                 * case we may prune some more untrusted certificates and try
                 * again.  Thus the S_DOALTERNATE bit may yet be turned on
                 * again with an even shorter untrusted chain!
                 *
                 * If in the process we threw away our matching PKIX-TA trust
                 * anchor, reset DANE trust.  We might find a suitable trusted
                 * certificate among the ones from the trust store.
                 */
                if ((search & S_DOALTERNATE) != 0) {
                    OPENSSL_assert(num > i && i > 0 && ss == 0);
                    search &= ~S_DOALTERNATE;
                    for (; num > i; --num)
                        X509_free(sk_X509_pop(ctx->chain));
                    ctx->num_untrusted = num;

                    if (DANETLS_ENABLED(dane) &&
                        dane->mdpth >= ctx->num_untrusted) {
                        dane->mdpth = -1;
                        X509_free(dane->mcert);
                        dane->mcert = NULL;
                    }
                    if (DANETLS_ENABLED(dane) &&
                        dane->pdpth >= ctx->num_untrusted)
                        dane->pdpth = -1;
                }

                /*
                 * Self-signed untrusted certificates get replaced by their
                 * trusted matching issuer.  Otherwise, grow the chain.
                 */
                if (ss == 0) {
                    if (!sk_X509_push(ctx->chain, x = xtmp)) {
                        X509_free(xtmp);
                        X509err(X509_F_BUILD_CHAIN, ERR_R_MALLOC_FAILURE);
                        trust = X509_TRUST_REJECTED;
                        search = 0;
                        continue;
                    }
                    ss = cert_self_signed(x);
                } else if (num == ctx->num_untrusted) {
                    /*
                     * We have a self-signed certificate that has the same
                     * subject name (and perhaps keyid and/or serial number) as
                     * a trust-anchor.  We must have an exact match to avoid
                     * possible impersonation via key substitution etc.
                     */
                    if (X509_cmp(x, xtmp) != 0) {
                        /* Self-signed untrusted mimic. */
                        X509_free(xtmp);
                        ok = 0;
                    } else {
                        X509_free(x);
                        ctx->num_untrusted = --num;
                        (void) sk_X509_set(ctx->chain, num, x = xtmp);
                    }
                }

                /*
                 * We've added a new trusted certificate to the chain, recheck
                 * trust.  If not done, and not self-signed look deeper.
                 * Whether or not we're doing "trusted first", we no longer
                 * look for untrusted certificates from the peer's chain.
                 *
                 * At this point ctx->num_trusted and num must reflect the
                 * correct number of untrusted certificates, since the DANE
                 * logic in check_trust() depends on distinguishing CAs from
                 * "the wire" from CAs from the trust store.  In particular, the
                 * certificate at depth "num" should be the new trusted
                 * certificate with ctx->num_untrusted <= num.
                 */
                if (ok) {
                    OPENSSL_assert(ctx->num_untrusted <= num);
                    search &= ~S_DOUNTRUSTED;
                    switch (trust = check_trust(ctx, num)) {
                    case X509_TRUST_TRUSTED:
                    case X509_TRUST_REJECTED:
                        search = 0;
                        continue;
                    }
                    if (ss == 0)
                        continue;
                }
            }

            /*
             * No dispositive decision, and either self-signed or no match, if
             * we were doing untrusted-first, and alt-chains are not disabled,
             * do that, by repeatedly losing one untrusted element at a time,
             * and trying to extend the shorted chain.
             */
            if ((search & S_DOUNTRUSTED) == 0) {
                /* Continue search for a trusted issuer of a shorter chain? */
                if ((search & S_DOALTERNATE) != 0 && --alt_untrusted > 0)
                    continue;
                /* Still no luck and no fallbacks left? */
                if (!may_alternate || (search & S_DOALTERNATE) != 0 ||
                    ctx->num_untrusted < 2)
                    break;
                /* Search for a trusted issuer of a shorter chain */
                search |= S_DOALTERNATE;
                alt_untrusted = ctx->num_untrusted - 1;
                ss = 0;
            }
        }

        /*
         * Extend chain with peer-provided certificates
         */
        if ((search & S_DOUNTRUSTED) != 0) {
            num = sk_X509_num(ctx->chain);
            OPENSSL_assert(num == ctx->num_untrusted);
            x = sk_X509_value(ctx->chain, num-1);
            xtmp = (depth < num) ? NULL : find_issuer(ctx, sktmp, x);

            /*
             * Once we run out of untrusted issuers, we stop looking for more
             * and start looking only in the trust store if enabled.
             */
            if (xtmp == NULL) {
                search &= ~S_DOUNTRUSTED;
                if (may_trusted)
                    search |= S_DOTRUSTED;
                continue;
            }

            if (!sk_X509_push(ctx->chain, x = xtmp)) {
                X509err(X509_F_BUILD_CHAIN, ERR_R_MALLOC_FAILURE);
                trust = X509_TRUST_REJECTED;
                search = 0;
                continue;
            }
            X509_up_ref(x);
            ++ctx->num_untrusted;
            ss = cert_self_signed(xtmp);

            /*
             * Not strictly necessary, but saves cycles looking at the same
             * certificates over and over.
             */
            (void) sk_X509_delete_ptr(sktmp, x);

            /*
             * Check for DANE-TA trust of the topmost untrusted certificate.
             */
            switch (trust = check_dane_issuer(ctx, ctx->num_untrusted - 1)) {
            case X509_TRUST_TRUSTED:
            case X509_TRUST_REJECTED:
                search = 0;
                continue;
            }
        }
    }
    sk_X509_free(sktmp);

    /*
     * Last chance to make a trusted chain, either bare DANE-TA public-key
     * signers, or else direct leaf PKIX trust.
     */
    num = sk_X509_num(ctx->chain);
    if (num <= depth) {
        if (trust == X509_TRUST_UNTRUSTED && DANETLS_HAS_DANE_TA(dane))
            trust = check_dane_pkeys(ctx);
        if (trust == X509_TRUST_UNTRUSTED && num == ctx->num_untrusted)
            trust = check_trust(ctx, num);
    }

    switch (trust) {
    case X509_TRUST_TRUSTED:
        return 1;
    case X509_TRUST_REJECTED:
        return 0;
    case X509_TRUST_UNTRUSTED:
    default:
        num = sk_X509_num(ctx->chain);
        ctx->current_cert = sk_X509_value(ctx->chain, num - 1);
        ctx->error_depth = num-1;
        if (num > depth)
            ctx->error = X509_V_ERR_CERT_CHAIN_TOO_LONG;
        else if (DANETLS_ENABLED(dane) &&
                 (!DANETLS_HAS_PKIX(dane) || dane->pdpth >= 0))
            ctx->error = X509_V_ERR_CERT_UNTRUSTED;
        else if (ss && sk_X509_num(ctx->chain) == 1)
            ctx->error = X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT;
        else if (ss)
            ctx->error = X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN;
        else if (ctx->num_untrusted == num)
            ctx->error = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY;
        else
            ctx->error = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT;
        if (DANETLS_ENABLED(dane))
            dane_reset(dane);
        return cb(0, ctx);
    }
}
