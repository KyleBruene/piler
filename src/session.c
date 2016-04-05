/*
 * session.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <piler.h>
#include "smtp.h"

int is_blocked_by_tcp_wrappers(int sd);


int handle_smtp_session(int new_sd, struct __data *data, struct __config *cfg){
   int i, ret, pos, readpos=0, result, n, inj=ERR, protocol_state, prevlen=0;
   char *p, *rcpt, buf[MAXBUFSIZE], puf[MAXBUFSIZE], resp[MAXBUFSIZE], prevbuf[MAXBUFSIZE], last2buf[2*MAXBUFSIZE+1];
   char virusinfo[SMALLBUFSIZE], delay[SMALLBUFSIZE], tmpbuf[SMALLBUFSIZE];
   char *arule = NULL;
   char *status = NULL;
   struct session_data sdata;
   struct parser_state parser_state;
   int db_conn=0;
   int rc;
   struct __counters counters;

   struct timezone tz;
   struct timeval tv1, tv2;

#ifdef HAVE_STARTTLS
   int starttls = 0;
#endif


#ifdef HAVE_LIBWRAP
   if(is_blocked_by_tcp_wrappers(new_sd) == 1) return 0;
#endif

   srand(getpid());

   protocol_state = SMTP_STATE_INIT;

   init_session_data(&sdata, cfg);
   sdata.tls = 0;

   bzero(&counters, sizeof(counters));


   /* open database connection */

   db_conn = 0;

#ifdef NEED_MYSQL
   if(open_database(&sdata, cfg) == OK){
      db_conn = 1;
   }
   else
      syslog(LOG_PRIORITY, "%s", ERR_MYSQL_CONNECT);
#endif

   if(db_conn == 0){
      snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_421_ERR_TMP, cfg->hostid);
      send(new_sd, buf, strlen(buf), 0);
      return 0;
   }


   gettimeofday(&tv1, &tz);

#ifdef HAVE_LMTP
   snprintf(buf, MAXBUFSIZE-1, LMTP_RESP_220_BANNER, cfg->hostid);
#else
   snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_220_BANNER, cfg->hostid);
#endif

   send(new_sd, buf, strlen(buf), 0);
   if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

   while((n = recvtimeoutssl(new_sd, &puf[readpos], sizeof(puf)-readpos, TIMEOUT, sdata.tls, data->ssl)) > 0){
         pos = 0;

         /* accept mail data */

         if(protocol_state == SMTP_STATE_DATA){

            /* join the last 2 buffer */

            memset(last2buf, 0, 2*MAXBUFSIZE+1);
            memcpy(last2buf, prevbuf, MAXBUFSIZE);
            memcpy(last2buf+prevlen, puf, MAXBUFSIZE);


            pos = searchStringInBuffer(last2buf, 2*MAXBUFSIZE+1, SMTP_CMD_PERIOD, 5);
            if(pos > 0){

	       /* fix position */
               pos = pos - prevlen;

               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: period found", sdata.ttmpfile);


               /* write data only to (and including) the trailing period (.) */
               ret = write(sdata.fd, puf, pos);
               sdata.tot_len += ret;

               /* fix posistion! */
               pos += strlen(SMTP_CMD_PERIOD);

               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: (.)", sdata.ttmpfile);


               protocol_state = SMTP_STATE_PERIOD;

               /* make sure we had a successful read */

               rc = fsync(sdata.fd);
               close(sdata.fd);

               gettimeofday(&tv2, &tz);
               sdata.__acquire = tvdiff(tv2, tv1);

               if(rc){
                  syslog(LOG_PRIORITY, "failed writing data: %s", sdata.ttmpfile);

               #ifdef HAVE_LMTP
                  for(i=0; i<sdata.num_of_rcpt_to; i++){
               #endif

                     write1(new_sd, SMTP_RESP_421_ERR_WRITE_FAILED, strlen(SMTP_RESP_421_ERR_WRITE_FAILED), sdata.tls, data->ssl);

               #ifdef HAVE_LMTP
                  }
               #endif

                  memset(puf, 0, MAXBUFSIZE);
                  goto AFTER_PERIOD;
               }


               gettimeofday(&tv1, &tz);

               data->folder = 0;

               parser_state = parse_message(&sdata, 1, data, cfg);
               post_parse(&sdata, &parser_state, cfg);

               gettimeofday(&tv2, &tz);
               sdata.__parsed = tvdiff(tv2, tv1);

               if(cfg->syslog_recipients == 1){
                  rcpt = parser_state.b_to;
                  do {
                     rcpt = split_str(rcpt, " ", tmpbuf, sizeof(tmpbuf)-1);

                     if(does_it_seem_like_an_email_address(tmpbuf) == 1){
                        syslog(LOG_PRIORITY, "%s: rcpt=%s", sdata.ttmpfile, tmpbuf);
                     }
                  } while(rcpt);
               }

               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: parsed message", sdata.ttmpfile);

               if(cfg->archive_only_mydomains == 1 && sdata.internal_sender == 0 && sdata.internal_recipient == 0){
                  remove_stripped_attachments(&parser_state);
                  inj = ERR_MYDOMAINS;

                  snprintf(sdata.acceptbuf, SMALLBUFSIZE-1, "250 Ok %s\r\n", sdata.ttmpfile);
                  write1(new_sd, sdata.acceptbuf, strlen(sdata.acceptbuf), sdata.tls, data->ssl);

                  syslog(LOG_PRIORITY, "%s: discarding: not on mydomains, from=%s, message-id=%s", sdata.ttmpfile, sdata.fromemail, parser_state.message_id);

                  goto END_OF_PROCESSING;
               }

               make_digests(&sdata, cfg);

            #ifdef HAVE_ANTIVIRUS
               if(cfg->use_antivirus == 1){
                  sdata.rav = do_av_check(&sdata, &virusinfo[0], data, cfg);
               }
            #endif



            #ifdef HAVE_LMTP
               for(i=0; i<sdata.num_of_rcpt_to; i++){
            #else
               i = 0;
            #endif
                  if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: round %d in injection", sdata.ttmpfile, i);

                  inj = ERR;
                  status = S_STATUS_UNDEF;


                  if(db_conn == 1){

                     if(sdata.restored_copy == 1){
                        syslog(LOG_PRIORITY, "%s: discarding: restored copy", sdata.ttmpfile);
                        inj = OK;
                     }
                     else if(sdata.tot_len < cfg->min_message_size){
                        syslog(LOG_PRIORITY, "%s: discarding: too short message (%d bytes)", sdata.ttmpfile, sdata.tot_len);
                        inj = OK;
                     }
                     else if(AVIR_VIRUS == sdata.rav){
                        syslog(LOG_PRIORITY, "%s: found virus: %s", sdata.ttmpfile, virusinfo);
                        counters.c_virus++;
                        inj = OK;
                     } else if(strlen(sdata.bodydigest) < 10) {
                        syslog(LOG_PRIORITY, "%s: invalid digest", sdata.ttmpfile);
                        inj = ERR;
                     } else {
                        if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: processing message", sdata.ttmpfile);

                        /* check message against archiving rules */

                        arule = check_againt_ruleset(data->archiving_rules, &parser_state, sdata.tot_len, sdata.spam_message);

                        if(arule){
                           syslog(LOG_PRIORITY, "%s: discarding: archiving policy: *%s*", sdata.ttmpfile, arule);
                           inj = OK;
                           counters.c_ignore++;

                           remove_stripped_attachments(&parser_state);

                           status = S_STATUS_DISCARDED;
                        }
                        else {
                           inj = process_message(&sdata, &parser_state, data, cfg);
                           unlink(parser_state.message_id_hash);
                           counters.c_size += sdata.tot_len;
                           counters.c_stored_size = sdata.stored_len;

                           status = S_STATUS_STORED;
                        }

                     }

                  }



                  /* set the accept buffer */

                  snprintf(sdata.acceptbuf, SMALLBUFSIZE-1, "250 Ok %s <%s>\r\n", sdata.ttmpfile, sdata.rcptto[i]);

                  if(inj == ERR){
                     snprintf(sdata.acceptbuf, SMALLBUFSIZE-1, "451 %s <%s>\r\n", sdata.ttmpfile, sdata.rcptto[i]);
                     status = S_STATUS_ERROR;
                  }

                  write1(new_sd, sdata.acceptbuf, strlen(sdata.acceptbuf), sdata.tls, data->ssl);

                  if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, sdata.acceptbuf);

                  counters.c_rcvd++;

                  if(inj == ERR_EXISTS){
                     syslog(LOG_PRIORITY, "%s: discarding: duplicate message, id: %llu, message-id: %s", sdata.ttmpfile, sdata.duplicate_id, parser_state.message_id);
                     counters.c_duplicate++;
                     status = S_STATUS_DUPLICATE;
                  }

                  snprintf(delay, SMALLBUFSIZE-1, "delay=%.2f, delays=%.2f/%.2f/%.2f/%.2f/%.2f/%.2f", 
                               (sdata.__acquire+sdata.__parsed+sdata.__av+sdata.__compress+sdata.__encrypt+sdata.__store)/1000000.0,
                                   sdata.__acquire/1000000.0, sdata.__parsed/1000000.0, sdata.__av/1000000.0, sdata.__compress/1000000.0, sdata.__encrypt/1000000.0, sdata.__store/1000000.0);

                  syslog(LOG_PRIORITY, "%s: from=%s, size=%d/%d, attachments=%d, reference=%s, message-id=%s, retention=%d, folder=%d, %s, status=%s", sdata.ttmpfile, sdata.fromemail, sdata.tot_len, sdata.stored_len, parser_state.n_attachments, parser_state.reference, parser_state.message_id, parser_state.retention, data->folder, delay, status);



            #ifdef HAVE_LMTP
               } /* for */
            #endif

            END_OF_PROCESSING:

               unlink(sdata.ttmpfile);
               unlink(sdata.tmpframe);


               /* if we have nothing after the trailing (.), we can read
                  the next command from the network */

               if(puf[n-3] == '.' && puf[n-2] == '\r' && puf[n-1] == '\n') continue;


               /* if we left something in the puffer, we are ready to proceed
                  to handle the additional commands, such as QUIT */

               /* if we miss the trailing \r\n, ie. we need another read */

               if(puf[n-2] != '\r' && puf[n-1] != '\n'){
                  memmove(puf, puf+pos, n-pos);
                  memset(puf+n-pos, 0, MAXBUFSIZE-n+pos);
                  recvtimeout(new_sd, buf, MAXBUFSIZE, TIMEOUT);
                  strncat(puf, buf, MAXBUFSIZE-1-n+pos);
                  if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: partial read: %s", sdata.ttmpfile, puf);
                  pos = 0;
               }

            } /* pos > 0, PERIOD found */
            else {
               ret = write(sdata.fd, puf, n);
               sdata.tot_len += ret;

               memcpy(prevbuf, puf, n);
               prevlen = n;

               continue;
            }

         } /* if protocol_state == SMTP_STATE_DATA */

AFTER_PERIOD:

      /* handle smtp commands */

      memset(resp, 0, sizeof(resp));

      p = &puf[pos];
      readpos = 0;

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: command=*%s*", sdata.ttmpfile, p);

      do {
         p = split(p, '\n', buf, sizeof(buf)-1, &result);

         if(result == 0){
            if(strlen(buf) > 0){
               if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: partial read: *%s*", sdata.ttmpfile, buf);

               snprintf(puf, sizeof(puf)-5, "%s", buf);
               readpos = strlen(puf);
            }

            break;
         }

         if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: got: %s", sdata.ttmpfile, buf);

         if(strncasecmp(buf, SMTP_CMD_EHLO, strlen(SMTP_CMD_EHLO)) == 0 || strncasecmp(buf, LMTP_CMD_LHLO, strlen(LMTP_CMD_LHLO)) == 0){
            process_command_ehlo_lhlo(&sdata, data, &protocol_state, &resp[0], sizeof(resp)-1, cfg);
            continue;

            /* FIXME: implement the ENHANCEDSTATUSCODE extensions */
         }


         if(strncasecmp(buf, SMTP_CMD_HELO, strlen(SMTP_CMD_HELO)) == 0){
            if(protocol_state == SMTP_STATE_INIT) protocol_state = SMTP_STATE_HELO;
            strncat(resp, SMTP_RESP_250_OK, sizeof(resp)-strlen(resp)-1);
            continue;
         }


      #ifdef HAVE_STARTTLS
         if(cfg->tls_enable > 0 && strncasecmp(buf, SMTP_CMD_STARTTLS, strlen(SMTP_CMD_STARTTLS)) == 0 && strlen(data->starttls) > 4 && sdata.tls == 0){
            process_command_starttls(&sdata, data, &protocol_state, &starttls, new_sd, &resp[0], sizeof(resp)-1, cfg);
            continue;
         }
      #endif


         if(strncasecmp(buf, SMTP_CMD_MAIL_FROM, strlen(SMTP_CMD_MAIL_FROM)) == 0){
            process_command_mail_from(&sdata, &protocol_state, buf, &resp[0], sizeof(resp)-1, cfg);
            continue;
         }


         if(strncasecmp(buf, SMTP_CMD_RCPT_TO, strlen(SMTP_CMD_RCPT_TO)) == 0){
            process_command_rcpt_to(&sdata, &protocol_state, buf, &resp[0], sizeof(resp)-1);
            continue;
         }


         if(strncasecmp(buf, SMTP_CMD_DATA, strlen(SMTP_CMD_DATA)) == 0){
            memset(last2buf, 0, 2*MAXBUFSIZE+1);
            memset(prevbuf, 0, MAXBUFSIZE);
            inj = ERR;
            prevlen = 0;

            process_command_data(&sdata, &protocol_state, &resp[0], sizeof(resp)-1);
            continue; 
         }


         if(strncasecmp(buf, SMTP_CMD_QUIT, strlen(SMTP_CMD_QUIT)) == 0){
            process_command_quit(&sdata, &protocol_state, &resp[0], sizeof(resp)-1, cfg);
            continue;
         }


         if(strncasecmp(buf, SMTP_CMD_NOOP, strlen(SMTP_CMD_NOOP)) == 0){
            strncat(resp, SMTP_RESP_250_OK, sizeof(resp)-strlen(resp)-1);
            continue;
         }


         if(strncasecmp(buf, SMTP_CMD_RESET, strlen(SMTP_CMD_RESET)) == 0){
            process_command_reset(&sdata, &protocol_state, &resp[0], sizeof(resp)-1, cfg);
            continue;
         }


         /* by default send 502 command not implemented message */

         syslog(LOG_PRIORITY, "%s: invalid command: *%s*", sdata.ttmpfile, buf);
         strncat(resp, SMTP_RESP_502_ERR, sizeof(resp)-strlen(resp)-1);
      } while(p);


      if(strlen(resp) > 0){
         send_buffered_response(&sdata, data, starttls, new_sd, &resp[0], cfg);
         memset(resp, 0, sizeof(resp));
      }


      if(protocol_state == SMTP_STATE_FINISHED){
         goto QUITTING;
      }

   } /* while */

   /*
    * if we are not in SMTP_STATE_QUIT and the message was not injected,
    * ie. we have timed out than send back 421 error message
    */

   if(protocol_state < SMTP_STATE_QUIT && inj == ERR){
      snprintf(buf, MAXBUFSIZE-1, SMTP_RESP_421_ERR, cfg->hostid);
      write1(new_sd, buf, strlen(buf), sdata.tls, data->ssl);

      if(cfg->verbosity >= _LOG_DEBUG) syslog(LOG_PRIORITY, "%s: sent: %s", sdata.ttmpfile, buf);

      if(sdata.fd != -1){

         syslog(LOG_PRIORITY, "%s: removing stale files: %s, %s", sdata.ttmpfile, sdata.ttmpfile, sdata.tmpframe);

         close(sdata.fd);
         unlink(sdata.ttmpfile);
         unlink(sdata.tmpframe);
      }

      goto QUITTING;
   }


QUITTING:

   update_counters(&sdata, data, &counters, cfg);

#ifdef NEED_MYSQL
   close_database(&sdata);
#endif

#ifdef HAVE_STARTTLS
   if(sdata.tls == 1){
      SSL_shutdown(data->ssl);
      SSL_free(data->ssl);
   }
#endif

   if(cfg->verbosity >= _LOG_INFO) syslog(LOG_PRIORITY, "processed %llu messages", counters.c_rcvd);

   return (int)counters.c_rcvd;
}



#ifdef HAVE_LIBWRAP
int is_blocked_by_tcp_wrappers(int sd){
   struct request_info req;

   request_init(&req, RQ_DAEMON, PROGNAME, RQ_FILE, sd, 0);

   fromhost(&req);

   if(!hosts_access(&req)){
      send(sd, SMTP_RESP_550_ERR_YOU_ARE_BANNED_BY_LOCAL_POLICY, strlen(SMTP_RESP_550_ERR_YOU_ARE_BANNED_BY_LOCAL_POLICY), 0);
      syslog(LOG_PRIORITY, "denied connection from %s by tcp_wrappers", eval_client(&req));
      return 1;
   }

   return 0;
}
#endif


