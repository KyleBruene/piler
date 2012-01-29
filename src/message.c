/*
 * message.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <piler.h>
#include <zlib.h>


int is_existing_message_id(struct session_data *sdata, struct _state *state, struct __config *cfg){
   int rc=0;
   char s[SMALLBUFSIZE];
   MYSQL_STMT *stmt;
   MYSQL_BIND bind[1];
   my_bool is_null[1];
   unsigned long len=0;


   stmt = mysql_stmt_init(&(sdata->mysql));
   if(!stmt){
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_init() error", sdata->ttmpfile, SQL_METADATA_TABLE);
      goto ENDE;
   }

   snprintf(s, SMALLBUFSIZE-1, "SELECT message_id FROM %s WHERE message_id=?", SQL_METADATA_TABLE);

   if(mysql_stmt_prepare(stmt, s, strlen(s))){
      syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_prepare() error: %s", sdata->ttmpfile, SQL_METADATA_TABLE, mysql_stmt_error(stmt));
      goto ENDE;
   }

   memset(bind, 0, sizeof(bind));

   bind[0].buffer_type = MYSQL_TYPE_STRING;
   bind[0].buffer = state->message_id;
   bind[0].is_null = 0;
   len = strlen(state->message_id); bind[0].length = &len;

   if(mysql_stmt_bind_param(stmt, bind)){
      syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_bind_param() error: %s", sdata->ttmpfile, SQL_METADATA_TABLE, mysql_stmt_error(stmt));
      goto CLOSE;
   }


   if(mysql_stmt_execute(stmt)){
      syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_execute() error: %s", sdata->ttmpfile, SQL_METADATA_TABLE, mysql_stmt_error(stmt));
      goto CLOSE;
   }


   memset(bind, 0, sizeof(bind));

   bind[0].buffer_type = MYSQL_TYPE_STRING;
   bind[0].buffer = &s[0];
   bind[0].buffer_length = sizeof(s)-1;
   bind[0].is_null = &is_null[0];
   bind[0].length = &len;


   if(mysql_stmt_bind_result(stmt, bind)){
      syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_bind_result() error: %s", sdata->ttmpfile, SQL_METADATA_TABLE, mysql_stmt_error(stmt));
      goto CLOSE;
   }


   if(mysql_stmt_store_result(stmt)){
      syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_store_result() error: %s", sdata->ttmpfile, SQL_METADATA_TABLE, mysql_stmt_error(stmt));
      goto CLOSE;
   }

   if(!mysql_stmt_fetch(stmt)){
      syslog(LOG_PRIORITY, "%s: found message_id:*%s*(%ld) null=%d", sdata->ttmpfile, s, len, is_null[0]);
      if(is_null[0] == 0) rc = 1;
   }

CLOSE:
   mysql_stmt_close(stmt);

ENDE:

   return rc;
}


int is_body_digest_already_stored(struct session_data *sdata, struct _state *state, struct __config *cfg){
   int rc=0;
   char s[SMALLBUFSIZE];
   MYSQL_RES *res;
   MYSQL_ROW row;

   snprintf(s, SMALLBUFSIZE-1, "SELECT `bodydigest` FROM `%s` WHERE `bodydigest`='%s'", SQL_METADATA_TABLE, sdata->bodydigest);

   if(mysql_real_query(&(sdata->mysql), s, strlen(s)) == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res != NULL){
         row = mysql_fetch_row(res);
         if(row) rc = 1;
         mysql_free_result(res);
      }
   }

   return rc;
}


int store_index_data(struct session_data *sdata, struct _state *state, uint64 id, struct __config *cfg){
   int rc=ERR;
   char *subj, s[SMALLBUFSIZE];

   MYSQL_STMT *stmt;
   MYSQL_BIND bind[7];
   unsigned long len[7];

   subj = state->b_subject;
   if(*subj == ' ') subj++;


   stmt = mysql_stmt_init(&(sdata->mysql));
   if(!stmt){
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_init() error", sdata->ttmpfile, SQL_SPHINX_TABLE);
      return rc;
   }


   snprintf(s, sizeof(s)-1, "INSERT INTO %s (`id`, `from`, `to`, `fromdomain`, `todomain`, `subject`, `body`, `arrived`, `sent`, `size`, `direction`, `attachments`, `attachment_types`) values(%llu,?,?,?,?,?,?,%ld,%ld,%d,%d,%d,?)", SQL_SPHINX_TABLE, id, sdata->now, sdata->sent, sdata->tot_len, sdata->direction, state->n_attachments);


   if(mysql_stmt_prepare(stmt, s, strlen(s))){
      syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_prepare() error: %s", sdata->ttmpfile, SQL_SPHINX_TABLE, mysql_stmt_error(stmt));
      return rc;
   }


   fix_email_address_for_sphinx(state->b_from);
   fix_email_address_for_sphinx(state->b_to);
   fix_email_address_for_sphinx(state->b_from_domain);
   fix_email_address_for_sphinx(state->b_to_domain);


   memset(bind, 0, sizeof(bind));

   bind[0].buffer_type = MYSQL_TYPE_STRING;
   bind[0].buffer = state->b_from;
   bind[0].is_null = 0;
   len[0] = strlen(state->b_from); bind[0].length = &len[0];

   bind[1].buffer_type = MYSQL_TYPE_STRING;
   bind[1].buffer = state->b_to;
   bind[1].is_null = 0;
   len[1] = strlen(state->b_to); bind[1].length = &len[1];

   bind[2].buffer_type = MYSQL_TYPE_STRING;
   bind[2].buffer = state->b_from_domain;
   bind[2].is_null = 0;
   len[2] = strlen(state->b_from_domain); bind[2].length = &len[2];

   bind[3].buffer_type = MYSQL_TYPE_STRING;
   bind[3].buffer = state->b_to_domain;
   bind[3].is_null = 0;
   len[3] = strlen(state->b_to_domain); bind[3].length = &len[3];


   bind[4].buffer_type = MYSQL_TYPE_STRING;
   bind[4].buffer = subj;
   bind[4].is_null = 0;
   len[4] = strlen(subj); bind[4].length = &len[4];

   bind[5].buffer_type = MYSQL_TYPE_STRING;
   bind[5].buffer = state->b_body;
   bind[5].is_null = 0;
   len[5] = strlen(state->b_body); bind[5].length = &len[5];

   bind[6].buffer_type = MYSQL_TYPE_STRING;
   bind[6].buffer = sdata->attachments;
   bind[6].is_null = 0;
   len[6] = strlen(sdata->attachments); bind[6].length = &len[6];


   if(mysql_stmt_bind_param(stmt, bind)){
      syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_bind_param() error: %s", sdata->ttmpfile, SQL_SPHINX_TABLE, mysql_stmt_error(stmt));
      goto CLOSE;
   }


   if(mysql_stmt_execute(stmt)){
      syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_execute error: *%s*", sdata->ttmpfile, SQL_SPHINX_TABLE, mysql_error(&(sdata->mysql)));
      goto CLOSE;
   }

   rc = OK;

CLOSE:
   mysql_stmt_close(stmt);


   return rc;
}


int store_recipients(struct session_data *sdata, char *to, uint64 id, struct __config *cfg){
   int ret=OK;
   char *p, *q, s[SMALLBUFSIZE], puf[SMALLBUFSIZE];

   MYSQL_STMT *stmt;
   MYSQL_BIND bind[2];
   unsigned long len[2];

   stmt = mysql_stmt_init(&(sdata->mysql));
   if(!stmt){
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_init() error", sdata->ttmpfile, SQL_RECIPIENT_TABLE);
      return ERR;
   }

   snprintf(s, sizeof(s)-1, "INSERT INTO %s (`id`,`to`,`todomain`) VALUES('%llu',?,?)", SQL_RECIPIENT_TABLE, id);

   if(mysql_stmt_prepare(stmt, s, strlen(s))){
      syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_prepare() error: %s", sdata->ttmpfile, SQL_RECIPIENT_TABLE, mysql_stmt_error(stmt));
      return ERR;
   }

   p = to;
   do {
      p = split_str(p, " ", puf, sizeof(puf)-1);

      q = strchr(puf, '@');

      if(q && strlen(q) > 3 && does_it_seem_like_an_email_address(puf) == 1){
         q++;

         memset(bind, 0, sizeof(bind));

         bind[0].buffer_type = MYSQL_TYPE_STRING;
         bind[0].buffer = &puf[0];
         bind[0].is_null = 0;
         len[0] = strlen(puf); bind[0].length = &len[0];

         bind[1].buffer_type = MYSQL_TYPE_STRING;
         bind[1].buffer = q;
         bind[1].is_null = 0;
         len[1] = strlen(q); bind[1].length = &len[1];

         if(mysql_stmt_bind_param(stmt, bind)){
            syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_bind_param() error: %s", sdata->ttmpfile, SQL_RECIPIENT_TABLE, mysql_stmt_error(stmt));
            ret = ERR;
            goto CLOSE;
         }


         if(mysql_stmt_execute(stmt)){
            syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_execute error: *%s*", sdata->ttmpfile, SQL_RECIPIENT_TABLE, mysql_error(&(sdata->mysql)));
            ret = ERR;
         }

      } else {
         syslog(LOG_PRIORITY, "%s: invalid email address: %s", sdata->ttmpfile, puf);
         continue;
      }

   } while(p);


CLOSE:
   mysql_stmt_close(stmt);

   return ret;
}


int store_meta_data(struct session_data *sdata, struct _state *state, struct __config *cfg){
   int rc, ret=ERR;
   char *subj, *p, s[MAXBUFSIZE], vcode[2*DIGEST_LENGTH+1];

   MYSQL_STMT *stmt;
   MYSQL_BIND bind[4];
   unsigned long len[4];

   my_ulonglong id=0;


   stmt = mysql_stmt_init(&(sdata->mysql));
   if(!stmt){
      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_init() error", sdata->ttmpfile, SQL_METADATA_TABLE);
      return ERR;
   }

   subj = state->b_subject;
   if(*subj == ' ') subj++;

   snprintf(s, sizeof(s)-1, "%llu+%s%s%s%ld%ld%d%d%d%d%s%s%s", id, subj, state->b_from, state->message_id, sdata->now, sdata->sent, sdata->tot_len, sdata->hdr_len, sdata->direction, state->n_attachments, sdata->ttmpfile, sdata->digest, sdata->bodydigest);
   digest_string(s, &vcode[0]);


   snprintf(s, MAXBUFSIZE-1, "INSERT INTO %s (`from`,`fromdomain`,`subject`,`spam`,`arrived`,`sent`,`size`,`hlen`,`direction`,`attachments`,`piler_id`,`message_id`,`digest`,`bodydigest`,`vcode`) VALUES(?,?,?,%d,%ld,%ld,%d,%d,%d,%d,'%s',?,'%s','%s','%s')", SQL_METADATA_TABLE, sdata->spam_message, sdata->now, sdata->sent, sdata->tot_len, sdata->hdr_len, sdata->direction, state->n_attachments, sdata->ttmpfile, sdata->digest, sdata->bodydigest, vcode);

   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: meta sql: *%s*", sdata->ttmpfile, s);

   if(mysql_stmt_prepare(stmt, s, strlen(s))){
      syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_prepare() error: %s", sdata->ttmpfile, SQL_METADATA_TABLE, mysql_stmt_error(stmt));
      return ERR;
   }



   if(strlen(state->b_to) < 5){
      snprintf(state->b_to, SMALLBUFSIZE-1, "undisclosed-recipients@no.domain");
   }

   memset(bind, 0, sizeof(bind));

   bind[0].buffer_type = MYSQL_TYPE_STRING;
   bind[0].buffer = state->b_from;
   bind[0].is_null = 0;
   len[0] = strlen(state->b_from); bind[0].length = &len[0];

   p = strchr(state->b_from, '@');
   if(p && strlen(p) > 3){
      p++;
      bind[1].buffer_type = MYSQL_TYPE_STRING;
      bind[1].buffer = p;
      bind[1].is_null = 0;
      len[1] = strlen(p); bind[1].length = &len[1];
   }

   bind[2].buffer_type = MYSQL_TYPE_STRING;
   bind[2].buffer = subj;
   bind[2].is_null = 0;
   len[2] = strlen(subj); bind[2].length = &len[2];

   bind[3].buffer_type = MYSQL_TYPE_STRING;
   bind[3].buffer = state->message_id;
   bind[3].is_null = 0;
   len[3] = strlen(state->message_id); bind[3].length = &len[3];

   if(mysql_stmt_bind_param(stmt, bind)){
      syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_bind_param() error: %s", sdata->ttmpfile, SQL_METADATA_TABLE, mysql_stmt_error(stmt));
      goto CLOSE;
   }


   rc = mysql_stmt_execute(stmt);

   if(rc){
      syslog(LOG_PRIORITY, "%s: %s.mysql_stmt_execute() error: *%s*", sdata->ttmpfile, SQL_METADATA_TABLE, mysql_error(&(sdata->mysql)));
      ret = ERR_EXISTS;
   }
   else {
      id = mysql_stmt_insert_id(stmt);

      rc = store_recipients(sdata, state->b_to, id, cfg);

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: stored recipients, rc=%d", sdata->ttmpfile, rc);

      if(rc == OK){

         if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: updated verification code, rc=%d", sdata->ttmpfile, rc);

         rc = store_index_data(sdata, state, id, cfg);

         if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: stored indexdata, rc=%d", sdata->ttmpfile, rc);

         if(rc == OK)
            ret = OK;
      }
   }

CLOSE:
   mysql_stmt_close(stmt);

   return ret;
}


int process_message(struct session_data *sdata, struct _state *state, struct __config *cfg){
   int i, rc;

   /* discard if existing message_id */

   if(is_existing_message_id(sdata, state, cfg) == 1){
      return ERR_EXISTS;
   }


   /* check for existing body digest */

   rc = is_body_digest_already_stored(sdata, state, cfg);

   /*
    * TODO: check if the bodydigest were stored, then we should
    *       only store the header and append a 'bodypointer'
    */


   /* store base64 encoded file attachments */

   if(state->n_attachments > 0){
      rc = store_attachments(sdata, state, cfg);

      for(i=1; i<=state->n_attachments; i++){
         unlink(state->attachments[i].internalname);
      }

      if(rc) return ERR;
   }


   rc = store_file(sdata, sdata->tmpframe, 0, 0, cfg);
   if(rc == 0){
      syslog(LOG_PRIORITY, "%s: error storing message: %s", sdata->ttmpfile, sdata->tmpframe);
      return ERR;
   }


   rc = store_meta_data(sdata, state, cfg);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: stored metadata, rc=%d",  sdata->ttmpfile, rc);
   if(rc == ERR_EXISTS) return ERR_EXISTS;

   return OK;
}

