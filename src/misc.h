/*
 * misc.h, SJ
 */

#ifndef _MISC_H
 #define _MISC_H

#include <openssl/ssl.h>
#include <sys/time.h>
#include <pwd.h>
#include <cfg.h>
#include "defs.h"

#define CHK_NULL(x, errmsg) if ((x)==NULL) { printf("error: %s\n", errmsg); return ERR; }
#define CHK_SSL(err, msg) if ((err)==-1) { printf("ssl error: %s\n", msg); return ERR; }

int get_build();
void __fatal(char *s);
long tvdiff(struct timeval a, struct timeval b);
int searchStringInBuffer(char *s, int len1, char *what, int len2);
int countCharacterInBuffer(char *p, char c);
void replaceCharacterInBuffer(char *p, char from, char to);
char *split(char *row, int ch, char *s, int size);
char *split_str(char *row, char *what, char *s, int size);
int trimBuffer(char *s);
int extractEmail(char *rawmail, char *email);
void create_id(char *id, unsigned char server_id);
int get_random_bytes(unsigned char *buf, int len, unsigned char server_id);
int readFromEntropyPool(int fd, void *_s, size_t n);
int recvtimeout(int s, char *buf, int len, int timeout);
int write1(int sd, char *buf, int use_ssl, SSL *ssl);
int recvtimeoutssl(int s, char *buf, int len, int timeout, int use_ssl, SSL *ssl);

void write_pid_file(char *pidfile);
int drop_privileges(struct passwd *pwd);

int is_email_address_on_my_domains(char *email, struct __config *cfg);
void init_session_data(struct session_data *sdata, unsigned char server_id);
int read_from_stdin(struct session_data *sdata);
void strtolower(char *s);

unsigned long resolve_host(char *host);

#ifndef _GNU_SOURCE
   char *strcasestr(const char *s, const char *find);
#endif

#endif /* _MISC_H */
