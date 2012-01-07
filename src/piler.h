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

int do_av_check(struct session_data *sdata, char *rcpttoemail, char *fromemail, char *virusinfo, struct __data *data, struct __config *cfg);

int make_digests(struct session_data *sdata, struct __config *cfg);
void digest_file(char *filename, char *digest);

int handle_smtp_session(int new_sd, struct __data *data, struct __config *cfg);

int process_message(struct session_data *sdata, struct _state *sstate, struct __config *cfg);
int store_file(struct session_data *sdata, char *filename, int startpos, int len, struct __config *cfg);
int store_attachments(struct session_data *sdata, struct _state *state, struct __config *cfg);
int query_attachments(struct session_data *sdata, struct ptr_array *ptr_arr, struct __config *cfg);

struct __config read_config(char *configfile);

void check_and_create_directories(struct __config *cfg, uid_t uid, gid_t gid);

void update_counters(struct session_data *sdata, struct __data *data, struct __counters *counters, struct __config *cfg);

int retrieve_email_from_archive(struct session_data *sdata, FILE *dest, struct __config *cfg);


#endif /* _PILER_H */

