/*
 * reindex.c, SJ
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <syslog.h>
#include <getopt.h>
#include <piler.h>


extern char *optarg;
extern int optind;

int progressbar = 0;


void usage(){
   printf("\nusage: reindex \n\n");
   printf("    [-c|--config <config file>] \n");
   printf("    -f <from id>\n");
   printf("    -t <to id>\n");
   printf("    -a\n");
   printf("    [-p]\n");

   exit(0);
}


void p_clean_exit(char *msg, int rc){
   if(msg) printf("error: %s\n", msg);

   exit(rc);
}


uint64 get_max_meta_id(struct session_data *sdata){
   MYSQL_RES *res;
   MYSQL_ROW row;
   char s[SMALLBUFSIZE];
   int rc;
   uint64 id=0;

   snprintf(s, sizeof(s)-1, "SELECT MAX(`id`) FROM %s", SQL_METADATA_TABLE);

   rc = mysql_real_query(&(sdata->mysql), s, strlen(s));

   if(rc == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res){
         row = mysql_fetch_row(res);
         if(row){
            id = strtoull(row[0], NULL, 10);
         }
         mysql_free_result(res);
      }
   }
   return id;
}


uint64 retrieve_email_by_metadata_id(struct session_data *sdata, struct __data *data, uint64 from_id, uint64 to_id, struct __config *cfg){
   MYSQL_RES *res;
   MYSQL_ROW row;
   FILE *f;
   char filename[SMALLBUFSIZE];
   char s[SMALLBUFSIZE];
   int rc=0;
   uint64 stored_id=0, reindexed=0;
   struct _state state;


   snprintf(s, sizeof(s)-1, "SELECT `id`, `piler_id`, `arrived`, `sent` FROM %s WHERE (id BETWEEN %llu AND %llu) AND `deleted`=0", SQL_METADATA_TABLE, from_id, to_id);

   rc = mysql_real_query(&(sdata->mysql), s, strlen(s));

   if(rc == 0){
      res = mysql_store_result(&(sdata->mysql));
      if(res){
         while((row = mysql_fetch_row(res))){

            stored_id = strtoull(row[0], NULL, 10);
            if(stored_id > 0){
               snprintf(sdata->ttmpfile, SMALLBUFSIZE-1, "%s", (char*)row[1]);

               snprintf(filename, sizeof(filename)-1, "%llu.eml", stored_id);

               f = fopen(filename, "w");
               if(f){
                  rc = retrieve_email_from_archive(sdata, f, cfg);
                  fclose(f);

                  if(rc){
                     printf("cannot retrieve: %s\n", filename);
                     unlink(filename);
                     continue;
                  }

                  snprintf(sdata->filename, SMALLBUFSIZE-1, "%s", filename);

                  state = parse_message(sdata, 0, data, cfg);
                  post_parse(sdata, &state, cfg);

                  sdata->now = strtoul(row[2], NULL, 10);
                  sdata->sent = strtoul(row[3], NULL, 10);

                  rc = store_index_data(sdata, &state, data, stored_id, cfg);

                  if(rc == OK) reindexed++;
                  else printf("failed to add to %s table: %s\n", SQL_SPHINX_TABLE, filename);

                  unlink(filename);

                  if(progressbar && reindexed % 100 == 0) printf(".");
               }
               else printf("cannot open: %s\n", filename);

            }
         }
         mysql_free_result(res);
      }
      else rc = 1;
   }

   if(progressbar) printf("\n");

   return reindexed;
}


int main(int argc, char **argv){
   int c, all=0;
   uint64 from_id=0, to_id=0, n=0;
   char *configfile=CONFIG_FILE, *folder=NULL;
   struct session_data sdata;
   struct __data data;
   struct __config cfg;


   while(1){
      c = getopt(argc, argv, "c:f:t:F:pahv?");

      if(c == -1) break;

      switch(c){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'f' :
                    from_id = strtoull(optarg, NULL, 10);
                    break;

         case 't' :
                    to_id = strtoull(optarg, NULL, 10);
                    break;

         case 'a' :
                    all = 1;
                    break;

         case 'F' :
                    folder = optarg;
                    break;


         case 'p' :
                    progressbar = 1;
                    break;


         default  :
                    usage();
                    break;
      }

   }


   if(all == 0 && (from_id <= 0 || to_id <= 0) ) usage();


   (void) openlog("reindex", LOG_PID, LOG_MAIL);


   cfg = read_config(configfile);

   if(read_key(&cfg)){
      printf("%s\n", ERR_READING_KEY);
      return 1;
   }

   data.folder = 0;
   data.recursive_folder_names = 0;
   data.archiving_rules = NULL;
   data.retention_rules = NULL;

   if(folder){
      data.folder = get_folder_id(&sdata, folder, 0);
      if(data.folder == 0){
         printf("error: could not get folder id for '%s'\n", folder);
         return 0;
      }
   }

   init_session_data(&sdata, cfg.server_id);


   mysql_init(&(sdata.mysql));
   mysql_options(&(sdata.mysql), MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg.mysql_connect_timeout);
   if(mysql_real_connect(&(sdata.mysql), cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0) == 0){
      p_clean_exit("cannot connect to mysql server", 1);
   }

   mysql_real_query(&(sdata.mysql), "SET NAMES utf8", strlen("SET NAMES utf8"));
   mysql_real_query(&(sdata.mysql), "SET CHARACTER SET utf8", strlen("SET CHARACTER SET utf8"));

   load_mydomains(&sdata, &data, &cfg);

   if(all == 1){
      from_id = 1;
      to_id = get_max_meta_id(&sdata);
   }

   n = retrieve_email_by_metadata_id(&sdata, &data, from_id, to_id, &cfg);

   printf("put %llu messages to %s table for reindexing\n", n, SQL_SPHINX_TABLE);

   mysql_close(&(sdata.mysql));

   return 0;
}


