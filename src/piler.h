/*
 * piler.h, SJ
 */

#ifndef _PILER_H
 #define _PILER_H

#include <misc.h>
#include <list.h>
#include <parser.h>
#include <errmsg.h>
#include <smtpcodes.h>
#include <decoder.h>
#include <list.h>
#include <rules.h>
#include <defs.h>
#include <tai.h>
#include <sig.h>
#include <av.h>
#include <rules.h>
#include <config.h>
#include <unistd.h>

#ifdef HAVE_MEMCACHED
   #include "memc.h"
#endif

int read_key(struct __config *cfg);
void insert_offset(struct session_data *sdata, int server_id);

int do_av_check(struct session_data *sdata, char *rcpttoemail, char *virusinfo, struct __data *data, struct __config *cfg);

int make_digests(struct session_data *sdata, struct __config *cfg);
void digest_file(char *filename, char *digest);
void digest_string(char *s, char *digest);

int handle_smtp_session(int new_sd, struct __data *data, struct __config *cfg);
int handle_pilerget_request(int new_sd, struct __data *data, struct __config *cfg);

void remove_stripped_attachments(struct _state *state);
int process_message(struct session_data *sdata, struct _state *state, struct __data *data, struct __config *cfg);
int store_file(struct session_data *sdata, char *filename, int startpos, int len, struct __config *cfg);
int remove_stored_message_files(struct session_data *sdata, struct _state *state, struct __config *cfg);
int store_attachments(struct session_data *sdata, struct _state *state, struct __data *data, struct __config *cfg);
int query_attachments(struct session_data *sdata, struct __data *data, struct ptr_array *ptr_arr, struct __config *cfg);

struct __config read_config(char *configfile);

void check_and_create_directories(struct __config *cfg, uid_t uid, gid_t gid);

void update_counters(struct session_data *sdata, struct __data *data, struct __counters *counters, struct __config *cfg);

int retrieve_email_from_archive(struct session_data *sdata, struct __data *data, FILE *dest, struct __config *cfg);
int file_from_archive_to_network(char *filename, int sd, int tls_enable, struct __data *data, struct __config *cfg);

int prepare_a_mysql_statement(struct session_data *sdata, MYSQL_STMT **stmt, char *s);

int import_message(char *filename, struct session_data *sdata, struct __data *data, struct __config *cfg);
unsigned long get_folder_id(struct session_data *sdata, struct __data *data, char *foldername, int parent_id);
unsigned long add_new_folder(struct session_data *sdata, struct __data *data, char *foldername, int parent_id);

int store_index_data(struct session_data *sdata, struct _state *state, struct __data *data, uint64 id, struct __config *cfg);

void extract_attachment_content(struct session_data *sdata, struct _state *state, char *filename, char *type, int *rec);

int retrieve_file_from_archive(char *filename, int mode, char **buffer, FILE *dest, struct __config *cfg);

void load_mydomains(struct session_data *sdata, struct __data *data, struct __config *cfg);

int create_prepared_statements(struct session_data *sdata, struct __data *data);
void close_prepared_statements(struct __data *data);

#endif /* _PILER_H */

