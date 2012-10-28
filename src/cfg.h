/*
 * cfg.h, SJ
 */

#ifndef _CFG_H
 #define _CFG_H

#include "config.h"

struct __config {
   char username[MAXVAL];

   char hostid[MAXVAL];
   char pidfile[MAXVAL];

   char listen_addr[MAXVAL];
   int listen_port;

   char clamd_addr[MAXVAL];
   int clamd_port;
   char clamd_socket[MAXVAL];

   int tls_enable;
   char pemfile[MAXVAL];
   char cipher_list[MAXVAL];

   int use_antivirus;

   char memcached_servers[MAXVAL];
   int memcached_ttl;

   int number_of_worker_processes;
   int max_requests_per_child;

   int backlog;

   char workdir[MAXVAL];
   char queuedir[MAXVAL];

   int verbosity;
   char locale[MAXVAL];

   int session_timeout;

   char piler_header_field[MAXVAL];

   unsigned char key[KEYLEN];
   unsigned char iv[MAXVAL];

   char mydomains[MAXVAL];

   char spam_header_line[MAXVAL];

   int default_retention_days;

   // mysql stuff

   char mysqlhost[MAXVAL];
   int mysqlport;
   char mysqlsocket[MAXVAL];
   char mysqluser[MAXVAL];
   char mysqlpwd[MAXVAL];
   char mysqldb[MAXVAL];
   int mysql_connect_timeout;

   int update_counters_to_memcached;
   int memcached_to_db_interval;

   int archive_emails_not_having_message_id;

   int min_word_len;

   int debug;
};


#endif /* _CFG_H */
