/* c_rehash.c - Create hash symlinks for certificates
 * C implementation based on the original Perl and shell versions
 *
 * Copyright (c) 2013-2014 Timo Teräs <timo.teras@iki.fi>
 * All rights reserved.
 *
 * This software is licensed under the MIT License.
 * Full license available at: http://opensource.org/licenses/MIT
 */

#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <everest/logging.hpp>

#define MAX_COLLISIONS 256
#define countof(x)     (sizeof(x) / sizeof(x[0]))

namespace ocpp {

struct entry_info {
    struct entry_info* next;
    char* filename;
    unsigned short old_id;
    unsigned char need_symlink;
    unsigned char digest[EVP_MAX_MD_SIZE];
};

struct bucket_info {
    struct bucket_info* next;
    struct entry_info *first_entry, *last_entry;
    unsigned int hash;
    unsigned short type;
    unsigned short num_needed;
};

enum Type {
    TYPE_CERT = 0,
    TYPE_CRL
};

static const char* symlink_extensions[] = {"", "r"};
static const char* file_extensions[] = {"pem", "crt", "cer", "crl"};

static int evpmdsize;
static const EVP_MD* evpmd;

static struct bucket_info* hash_table[257];

static void bit_set(unsigned char* set, unsigned bit) {
    set[bit / 8] |= 1 << (bit % 8);
}

static int bit_isset(unsigned char* set, unsigned bit) {
    return set[bit / 8] & (1 << (bit % 8));
}

static void add_entry(int type, unsigned int hash, const char* filename, const unsigned char* digest, int need_symlink,
                      unsigned short old_id) {
    struct bucket_info* bi;
    struct entry_info *ei, *found = NULL;
    unsigned int ndx = (type + hash) % countof(hash_table);

    for (bi = hash_table[ndx]; bi; bi = bi->next)
        if (bi->type == type && bi->hash == hash)
            break;
    if (!bi) {
        bi = (bucket_info*)(calloc(1, sizeof(*bi)));
        if (!bi)
            return;
        bi->next = hash_table[ndx];
        bi->type = type;
        bi->hash = hash;
        hash_table[ndx] = bi;
    }

    for (ei = bi->first_entry; ei; ei = ei->next) {
        if (digest && memcmp(digest, ei->digest, evpmdsize) == 0) {
            EVLOG_warning << "Skipping duplicate certificate in file " << std::string(filename);
            return;
        }
        if (!strcmp(filename, ei->filename)) {
            found = ei;
            if (!digest)
                break;
        }
    }
    ei = found;
    if (!ei) {
        if (bi->num_needed >= MAX_COLLISIONS)
            return;
        ei = (entry_info*)(calloc(1, sizeof(*ei)));
        if (!ei)
            return;

        ei->old_id = ~0;
        ei->filename = strdup(filename);
        if (bi->last_entry)
            bi->last_entry->next = ei;
        if (!bi->first_entry)
            bi->first_entry = ei;
        bi->last_entry = ei;
    }

    if (old_id < ei->old_id)
        ei->old_id = old_id;
    if (need_symlink && !ei->need_symlink) {
        ei->need_symlink = 1;
        bi->num_needed++;
        memcpy(ei->digest, digest, evpmdsize);
    }
}

static int handle_symlink(const char* filename, const char* fullpath) {
    static char xdigit[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  -1, -1, -1, -1, -1, -1, -1, 10, 11,
                            12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15};
    char linktarget[NAME_MAX], *endptr;
    unsigned int hash = 0;
    unsigned char ch;
    int i, type, id;
    ssize_t n;

    for (i = 0; i < 8; i++) {
        ch = filename[i] - '0';
        if (ch >= countof(xdigit) || xdigit[ch] < 0)
            return -1;
        hash <<= 4;
        hash += xdigit[ch];
    }
    if (filename[i++] != '.')
        return -1;
    for (type = countof(symlink_extensions) - 1; type > 0; type--)
        if (strcasecmp(symlink_extensions[type], &filename[i]) == 0)
            break;
    i += strlen(symlink_extensions[type]);

    id = strtoul(&filename[i], &endptr, 10);
    if (*endptr != 0)
        return -1;

    n = readlink(fullpath, linktarget, sizeof(linktarget));
    if (n >= sizeof(linktarget) || n < 0)
        return -1;
    linktarget[n] = 0;

    EVLOG_debug << "Found existing symlink " << std::string(filename) << " for " << hash << " (" << type
                << "), certname " << std::string(linktarget, strlen(linktarget));
    add_entry(type, hash, linktarget, NULL, 0, id);
    return 0;
}

static int handle_certificate(const char* filename, const char* fullpath) {
    STACK_OF(X509_INFO) * inf;
    X509_INFO* x;
    BIO* b;
    const char* ext;
    unsigned char digest[EVP_MAX_MD_SIZE];
    X509_NAME* name = NULL;
    int i, type, ret = -1;

    ext = strrchr(filename, '.');
    if (ext == NULL)
        return 0;
    for (i = 0; i < countof(file_extensions); i++) {
        if (strcasecmp(file_extensions[i], ext + 1) == 0)
            break;
    }
    if (i >= countof(file_extensions))
        return -1;

    b = BIO_new_file(fullpath, "r");
    if (!b)
        return -1;
    inf = PEM_X509_INFO_read_bio(b, NULL, NULL, NULL);
    BIO_free(b);
    if (!inf)
        return -1;

    if (sk_X509_INFO_num(inf) == 1) {
        x = sk_X509_INFO_value(inf, 0);
        if (x->x509) {
            type = TYPE_CERT;
            name = X509_get_subject_name(x->x509);
            X509_digest(x->x509, evpmd, digest, NULL);
        } else if (x->crl) {
            type = TYPE_CRL;
            name = X509_CRL_get_issuer(x->crl);
            X509_CRL_digest(x->crl, evpmd, digest, NULL);
        }
        if (name)
            add_entry(type, X509_NAME_hash(name), filename, digest, 1, ~0);
    } else {
        EVLOG_warning << std::string(filename) << " does not contain exactly one certificate or CRL: skipping";
    }

    sk_X509_INFO_pop_free(inf, X509_INFO_free);

    return ret;
}

static int hash_dir(const char* dirname) {
    struct bucket_info *bi, *nextbi;
    struct entry_info *ei, *nextei;
    struct dirent* de;
    struct stat st;
    unsigned char idmask[MAX_COLLISIONS / 8];
    int i, n, nextid, buflen, ret = -1;
    const char* pathsep;
    char* buf;
    DIR* d;

    evpmd = EVP_sha1();
    evpmdsize = EVP_MD_size(evpmd);

    if (access(dirname, R_OK | W_OK | X_OK) != 0) {
        EVLOG_error << "Access denied '" << std::string(dirname) << "'";
        return -1;
    }

    buflen = strlen(dirname);
    pathsep = (buflen && dirname[buflen - 1] == '/') ? "" : "/";
    buflen += NAME_MAX + 2;
    buf = (char*)(malloc(buflen));
    if (buf == NULL)
        goto err;

    EVLOG_debug << "Doing " << std::string(dirname);
    d = opendir(dirname);
    if (!d)
        goto err;

    while ((de = readdir(d)) != NULL) {
        if (snprintf(buf, buflen, "%s%s%s", dirname, pathsep, de->d_name) >= buflen)
            continue;
        if (lstat(buf, &st) < 0)
            continue;
        if (S_ISLNK(st.st_mode) && handle_symlink(de->d_name, buf) == 0)
            continue;
        if (strcmp(buf, "/etc/ssl/certs/ca-certificates.crt") == 0) {
            /* Ignore the /etc/ssl/certs/ca-certificates.crt file */
            EVLOG_debug << "Skipping /etc/ssl/certs/ca-certificates.crt file";
            continue;
        }
        handle_certificate(de->d_name, buf);
    }
    closedir(d);

    for (i = 0; i < countof(hash_table); i++) {
        for (bi = hash_table[i]; bi; bi = nextbi) {
            nextbi = bi->next;
            EVLOG_debug << "Type " << bi->type << " hash " << bi->hash << " num entries " << bi->num_needed << ":";

            nextid = 0;
            memset(idmask, 0, (bi->num_needed + 7) / 8);
            for (ei = bi->first_entry; ei; ei = ei->next)
                if (ei->old_id < bi->num_needed)
                    bit_set(idmask, ei->old_id);

            for (ei = bi->first_entry; ei; ei = nextei) {
                nextei = ei->next;
                EVLOG_debug << "\t(old_id " << ei->old_id << ", need_symlink " << ei->need_symlink << ") Cert "
                            << std::string(ei->filename, strlen(ei->filename)) << ":";

                if (ei->old_id < bi->num_needed) {
                    /* Link exists, and is used as-is */
                    snprintf(buf, buflen, "%08x.%s%d", bi->hash, symlink_extensions[bi->type], ei->old_id);
                    EVLOG_debug << "link " << std::string(ei->filename, strlen(ei->filename)) << " -> "
                                << std::string(buf, strlen(buf));
                } else if (ei->need_symlink) {
                    /* New link needed (it may replace something) */
                    while (bit_isset(idmask, nextid))
                        nextid++;

                    snprintf(buf, buflen, "%s%s%n%08x.%s%d", dirname, pathsep, &n, bi->hash,
                             symlink_extensions[bi->type], nextid);
                    EVLOG_debug << "link " << std::string(ei->filename, strlen(ei->filename)) << " -> "
                                << std::string(buf + n, strlen(buf + n));
                    unlink(buf);
                    symlink(ei->filename, buf);
                } else {
                    /* Link to be deleted */
                    snprintf(buf, buflen, "%s%s%n%08x.%s%d", dirname, pathsep, &n, bi->hash,
                             symlink_extensions[bi->type], ei->old_id);
                    EVLOG_debug << "unlink " << std::string(buf + n, strlen(buf + n));
                    unlink(buf);
                }
                free(ei->filename);
                free(ei);
            }
            free(bi);
        }
        hash_table[i] = NULL;
    }

    ret = 0;
err:
    free(buf);
    return ret;
}

} // namespace ocpp