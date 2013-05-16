/*
 * defs.h, SJ
 */

#ifndef _DEFS_H
   #define _DEFS_H

#ifdef NEED_MYSQL
  #include <mysql.h>
#endif
#ifdef NEED_SQLITE3
  #include <sqlite3.h>

   /* for older versions of sqlite3 do not have the sqlite3_prepare_v2() function, 2009.12.30, SJ */

  #if SQLITE_VERSION_NUMBER < 3006000
    #define sqlite3_prepare_v2 sqlite3_prepare
  #endif

#endif
#ifdef HAVE_TRE
   #include <tre/tre.h>
   #include <tre/regex.h>
#endif
#ifdef HAVE_LIBWRAP
   #include <tcpd.h>
#endif

#include <openssl/sha.h>
#include <openssl/ssl.h>
#include "tai.h"
#include "config.h"

#define MSG_UNDEF -1
#define MSG_BODY 0
#define MSG_RECEIVED 1
#define MSG_FROM 2
#define MSG_TO 3
#define MSG_CC 4
#define MSG_SUBJECT 5
#define MSG_CONTENT_TYPE 6
#define MSG_CONTENT_TRANSFER_ENCODING 7
#define MSG_CONTENT_DISPOSITION 8
#define MSG_MESSAGE_ID 9
#define MSG_REFERENCES 10
#define MSG_RECIPIENT 11

#define MAXHASH 8171

#define BASE64_RATIO 1.33333333

#define DIGEST_LENGTH SHA256_DIGEST_LENGTH

#define UNDEF 0
#define READY 1
#define BUSY 2

#define MAX_SQL_VARS 20

#define TYPE_UNDEF 0
#define TYPE_SHORT 1
#define TYPE_LONG 2
#define TYPE_LONGLONG 3
#define TYPE_STRING 4

#define MAXCHILDREN 64


typedef void signal_func (int);


struct child {
   pid_t pid;
   int messages;
   int status;
};


struct attachment {
   int size;
   char type[TINYBUFSIZE];
   char shorttype[TINYBUFSIZE];
   char aname[TINYBUFSIZE];
   char filename[TINYBUFSIZE];
   char internalname[TINYBUFSIZE];
   char digest[2*DIGEST_LENGTH+1];
   char dumped;
};


struct ptr_array {
   uint64 ptr;
   char piler_id[RND_STR_LEN+2];
   int attachment_id;
};


struct list {
   char s[SMALLBUFSIZE];
   struct list *r;
};


struct rule {
#ifdef HAVE_TRE
   regex_t from;
   regex_t to;
   regex_t subject;
   regex_t attachment_type;
#endif
   int spam;
   int size;
   char _size[4];
   int attachment_size;
   char _attachment_size[4];

   int days;

   char *rulestr;
   char compiled;

   struct rule *r;
};


struct _state {
   int line_num;
   int message_state;
   int is_header;
   int is_1st_header;
   int textplain;
   int texthtml;
   int message_rfc822;
   int base64;
   int utf8;
   int qp;
   int htmltag;
   int style;
   int skip_html;
   int has_to_dump;
   int fd;
   int b64fd;
   int mfd;
   int octetstream;
   int realbinary;
   int content_type_is_set;
   int pushed_pointer;
   int saved_size;
   int writebufpos;
   int abufpos;
   char attachedfile[RND_STR_LEN+SMALLBUFSIZE];
   char message_id[SMALLBUFSIZE];
   char miscbuf[MAX_TOKEN_LEN];
   char qpbuf[MAX_TOKEN_LEN];
   unsigned long n_token;
   unsigned long n_subject_token;
   unsigned long n_body_token;
   unsigned long n_chain_token;

   char filename[TINYBUFSIZE];
   char type[TINYBUFSIZE];

   struct list *boundaries;
   struct list *rcpt;
   struct list *rcpt_domain;
   struct list *journal_recipient;

   int n_attachments;
   struct attachment attachments[MAX_ATTACHMENTS];

   char reference[SMALLBUFSIZE];

   char b_from[SMALLBUFSIZE], b_from_domain[SMALLBUFSIZE], b_to[MAXBUFSIZE], b_to_domain[SMALLBUFSIZE], b_subject[MAXBUFSIZE], b_body[BIGBUFSIZE];
   char b_journal_to[MAXBUFSIZE];

   int bodylen;
   int tolen;
   int journaltolen;
};


struct session_data {
   char filename[SMALLBUFSIZE];
   char ttmpfile[SMALLBUFSIZE], tmpframe[SMALLBUFSIZE], tre, restored_copy;
   char mailfrom[SMALLBUFSIZE], rcptto[MAX_RCPT_TO][SMALLBUFSIZE], client_addr[SMALLBUFSIZE];
   char fromemail[SMALLBUFSIZE];
   char acceptbuf[SMALLBUFSIZE];
   char attachments[SMALLBUFSIZE];
   char internal_sender, internal_recipient, external_recipient;
   short int customer_id;
   int direction;
   int tls;
   int spam_message;
   int fd, hdr_len, tot_len, num_of_rcpt_to, rav;
   int need_scan;
   float __acquire, __parsed, __av, __store, __compress, __encrypt;
   char bodydigest[2*DIGEST_LENGTH+1];
   char digest[2*DIGEST_LENGTH+1];
   time_t now, sent, delivered, retained;
   char ms_journal;
   int journal_envelope_length, journal_bottom_length;
#ifdef NEED_MYSQL
   MYSQL mysql;
#endif
#ifdef NEED_SQLITE3
   sqlite3 *db;
#endif
};


#ifdef HAVE_MEMCACHED

#include <stdbool.h>
#include <netinet/in.h>

struct flags {
   bool no_block:1;
   bool no_reply:1;
   bool tcp_nodelay:1;
   bool tcp_keepalive:1;
};


struct memcached_server {

   struct flags flags;

   int fd;
   unsigned int snd_timeout;
   unsigned int rcv_timeout;

   int send_size;
   int recv_size;
   unsigned int tcp_keepidle;

   int last_read_bytes;

   char *result;
   char buf[MAXBUFSIZE];

   struct sockaddr_in addr;

   char server_ip[IPLEN];
   int server_port;

   char initialised;
};
#endif


struct __data {
   int folder;
   char recursive_folder_names;
   char starttls[TINYBUFSIZE];
   char mydomains[MAXBUFSIZE];

#ifdef NEED_MYSQL
   MYSQL_STMT *stmt_generic;
   MYSQL_STMT *stmt_get_meta_id_by_message_id;
   MYSQL_STMT *stmt_insert_into_rcpt_table;
   MYSQL_STMT *stmt_insert_into_sphinx_table;
   MYSQL_STMT *stmt_insert_into_meta_table;
   MYSQL_STMT *stmt_insert_into_attachment_table;
   MYSQL_STMT *stmt_get_attachment_id_by_signature;
   MYSQL_STMT *stmt_get_attachment_pointer;
   MYSQL_STMT *stmt_query_attachment;
   MYSQL_STMT *stmt_get_folder_id;
   MYSQL_STMT *stmt_insert_into_folder_table;
   MYSQL_STMT *stmt_update_metadata_reference;
   MYSQL_STMT *stmt_select_from_meta_table;
   MYSQL_STMT *stmt_select_non_referenced_attachments;
#endif

   char *sql[MAX_SQL_VARS];
   int type[MAX_SQL_VARS];
   int len[MAX_SQL_VARS];
   int pos;

#ifdef HAVE_TRE
   struct rule *archiving_rules;
   struct rule *retention_rules;
#endif

#ifdef HAVE_MEMCACHED
   struct memcached_server memc;
#endif

   SSL_CTX *ctx;
   SSL *ssl;

#ifdef HAVE_MULTITENANCY
   struct list *customers;
#endif

};


struct __counters {
   unsigned long long c_rcvd;
   unsigned long long c_virus;
   unsigned long long c_duplicate;
   unsigned long long c_ignore;
   unsigned long long c_size;
};

#endif /* _DEFS_H */

