/*
 * pilerimport.c, SJ
 */

#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <getopt.h>
#include <syslog.h>
#include <piler.h>


#define SKIPLIST "junk,trash,spam,draft"
#define MBOX_ARGS 1024

extern char *optarg;
extern int optind;

int quiet=0;
int remove_after_successful_import = 0;


int connect_to_imap_server(int sd, int *seq, char *imapserver, char *username, char *password, int port, struct __data *data, int use_ssl);
void close_connection(int sd, struct __data *data, int use_ssl);
int list_folders(int sd, int *seq, char *folders, int foldersize, int use_ssl, struct __data *data);
int process_imap_folder(int sd, int *seq, char *folder, struct session_data *sdata, struct __data *data, int use_ssl, struct __config *cfg);


int import_from_mailbox(char *mailbox, struct session_data *sdata, struct __data *data, struct __config *cfg){
   FILE *F, *f=NULL;
   int rc=ERR, tot_msgs=0, ret=OK;
   char buf[MAXBUFSIZE], fname[SMALLBUFSIZE];
   time_t t;


   F = fopen(mailbox, "r");
   if(!F){
      printf("cannot open mailbox: %s\n", mailbox);
      return rc;
   }

   t = time(NULL);

   while(fgets(buf, sizeof(buf)-1, F)){

      if(buf[0] == 'F' && buf[1] == 'r' && buf[2] == 'o' && buf[3] == 'm' && buf[4] == ' '){
         tot_msgs++;
         if(f){
            fclose(f);
            rc = import_message(fname, sdata, data, cfg);
            if(rc == ERR) ret = ERR;
            unlink(fname);

            if(quiet == 0) printf("processed: %7d\r", tot_msgs); fflush(stdout);
         }

         snprintf(fname, sizeof(fname)-1, "%ld-%d", t, tot_msgs);
         f = fopen(fname, "w+");
         continue;
      }

      if(f) fprintf(f, "%s", buf);
   }

   if(f){
      fclose(f);
      rc = import_message(fname, sdata, data, cfg);
      if(rc == ERR) ret = ERR;
      unlink(fname);

      if(quiet == 0) printf("processed: %7d\r", ++tot_msgs); fflush(stdout);
   }

   fclose(F);

   return ret;
}


int import_mbox_from_dir(char *directory, struct session_data *sdata, struct __data *data, int *tot_msgs, struct __config *cfg){
   DIR *dir;
   struct dirent *de;
   int rc=ERR, ret=OK, i=0;
   unsigned long folder;
   char fname[SMALLBUFSIZE];
   struct stat st;

   dir = opendir(directory);
   if(!dir){
      printf("cannot open directory: %s\n", directory);
      return ERR;
   }


   while((de = readdir(dir))){
      if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;

      snprintf(fname, sizeof(fname)-1, "%s/%s", directory, de->d_name);

      if(stat(fname, &st) == 0){
         if(S_ISDIR(st.st_mode)){
            folder = data->folder;
            rc = import_mbox_from_dir(fname, sdata, data, tot_msgs, cfg);
            data->folder = folder;
            if(rc == ERR) ret = ERR;
         }
         else {

            if(S_ISREG(st.st_mode)){
               if(i == 0 && data->recursive_folder_names == 1){
                  folder = get_folder_id(sdata, fname, data->folder);
                  if(folder == 0){
                     folder = add_new_folder(sdata, fname, data->folder);

                     if(folder == 0){
                        printf("error: cannot get/add folder '%s' to parent id: %d\n", fname, data->folder);
                        return ERR;
                     }
                     else {
                        data->folder = folder;
                     }
                  }

               }

               rc = import_from_mailbox(fname, sdata, data, cfg);
               if(rc == OK) (*tot_msgs)++;
               else ret = ERR;

               i++;
            }
            else {
               printf("%s is not a file\n", fname);
            }

         }
      }
      else {
         printf("cannot stat() %s\n", fname);
      }

   }
   closedir(dir);

   return ret;
}


int import_from_maildir(char *directory, struct session_data *sdata, struct __data *data, int *tot_msgs, struct __config *cfg){
   DIR *dir;
   struct dirent *de;
   int rc=ERR, ret=OK, i=0;
   unsigned long folder;
   char *p, fname[SMALLBUFSIZE];
   struct stat st;

   dir = opendir(directory);
   if(!dir){
      printf("cannot open directory: %s\n", directory);
      return ERR;
   }


   while((de = readdir(dir))){
      if(strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;

      snprintf(fname, sizeof(fname)-1, "%s/%s", directory, de->d_name);

      if(stat(fname, &st) == 0){
         if(S_ISDIR(st.st_mode)){
            folder = data->folder;
            rc = import_from_maildir(fname, sdata, data, tot_msgs, cfg);
            data->folder = folder;
            if(rc == ERR) ret = ERR;
         }
         else {

            if(S_ISREG(st.st_mode)){
               if(i == 0 && data->recursive_folder_names == 1){
                  p = strrchr(directory, '/');
                  if(p) p++;
                  else {
                     printf("invalid directory name: '%s'\n", directory);
                     return ERR;
                  }

                  folder = get_folder_id(sdata, p, data->folder);
                  if(folder == 0){
                     folder = add_new_folder(sdata, p, data->folder);

                     if(folder == 0){
                        printf("error: cannot get/add folder '%s' to parent id: %d\n", p, data->folder);
                        return ERR;
                     }
                     else {
                        data->folder = folder;
                     }
                  }

               }

               rc = import_message(fname, sdata, data, cfg);
               if(rc == OK) (*tot_msgs)++;
               else ret = ERR;

               if(remove_after_successful_import == 1 && ret != ERR) unlink(fname);

               i++;

               if(quiet == 0) printf("processed: %7d\r", *tot_msgs); fflush(stdout);
            }
            else {
               printf("%s is not a file\n", fname);
            }

         }
      }
      else {
         printf("cannot stat() %s\n", fname);
      }

   }
   closedir(dir);

   return ret;
}


int import_from_imap_server(char *imapserver, char *username, char *password, int port, struct session_data *sdata, struct __data *data, char *skiplist, struct __config *cfg){
   int rc=ERR, ret=OK, sd, seq=1, skipmatch, use_ssl=0;
   char *p, puf[SMALLBUFSIZE];
   char *q, muf[SMALLBUFSIZE];
   char folders[MAXBUFSIZE];


   if(port == 993) use_ssl = 1;


   if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
      printf("cannot create socket\n");
      return ERR;
   }

   if(connect_to_imap_server(sd, &seq, imapserver, username, password, port, data, use_ssl) == ERR){
      close(sd);
      return ERR;
   }


   list_folders(sd, &seq, &folders[0], sizeof(folders), use_ssl, data);


   p = &folders[0];
   do {
      memset(puf, 0, sizeof(puf));
      p = split(p, '\n', puf, sizeof(puf)-1);

      if(strlen(puf) < 1) continue;

      skipmatch = 0;

      if(skiplist && strlen(skiplist) > 0){
         q = skiplist;
         do {
            memset(muf, 0, sizeof(muf));
            q = split(q, ',', muf, sizeof(muf)-1);
            if(strncasecmp(puf, muf, strlen(muf)) == 0){
               skipmatch = 1;
               break;
            }
         } while(q);
      }

      if(skipmatch == 1){
         if(quiet == 0) printf("SKIPPING FOLDER: %s\n", puf);
         continue;
      }

      if(quiet == 0) printf("processing folder: %s... ", puf);

      rc = process_imap_folder(sd, &seq, puf, sdata, data, use_ssl, cfg);
      if(rc == ERR) ret = ERR;

   } while(p);


   close_connection(sd, data, use_ssl);

   return ret;
}


void usage(){
   printf("usage: pilerimport [-c <config file>] -e <eml file> | -m <mailbox file> | -d <directory> | -i <imap server> -u <imap username> -p <imap password> -P <imap port> [-F <foldername>] [-R] [-r] [-q]\n");
   exit(0);
}


int main(int argc, char **argv){
   int i, c, rc=0, n_mbox=0, tot_msgs=0, port=143;
   char *configfile=CONFIG_FILE, *emlfile=NULL, *mboxdir=NULL, *mbox[MBOX_ARGS], *directory=NULL;
   char *imapserver=NULL, *username=NULL, *password=NULL, *skiplist=SKIPLIST, *folder=NULL;
   struct session_data sdata;
   struct __config cfg;
   struct __data data;

   for(i=0; i<MBOX_ARGS; i++) mbox[i] = NULL;

   data.folder = 0;
   data.recursive_folder_names = 0;
   data.archiving_rules = NULL;
   data.retention_rules = NULL;

   while(1){

#ifdef _GNU_SOURCE
      static struct option long_options[] =
         {
            {"config",       required_argument,  0,  'c' },
            {"eml",          required_argument,  0,  'e' },
            {"dir",          required_argument,  0,  'd' },
            {"mbox",         required_argument,  0,  'm' },
            {"mboxdir",      required_argument,  0,  'M' },
            {"imapserver",   required_argument,  0,  'i' },
            {"username",     required_argument,  0,  'u' },
            {"password",     required_argument,  0,  'p' },
            {"port",         required_argument,  0,  'P' },
            {"skiplist",     required_argument,  0,  'x' },
            {"folder",       required_argument,  0,  'F' },
            {"quiet",        required_argument,  0,  'q' },
            {"recursive",    required_argument,  0,  'R' },
            {"remove-after-import",    required_argument,  0,  'r' },
            {"help",         no_argument,        0,  'h' },
            {0,0,0,0}
         };

      int option_index = 0;

      c = getopt_long(argc, argv, "c:m:M:e:d:i:u:p:P:x:F:Rrqh?", long_options, &option_index);
#else
      c = getopt(argc, argv, "c:m:M:e:d:i:u:p:P:x:F:Rrqh?");
#endif

      if(c == -1) break;

      switch(c){

         case 'c' :
                    configfile = optarg;
                    break;

         case 'e' :
                    emlfile = optarg;
                    break;

         case 'd' :
                    directory = optarg;
                    break;

         case 'm' :
                    if(n_mbox < MBOX_ARGS){
                       mbox[n_mbox++] = optarg;
                    } else {
                       printf("too many -m <mailbox> arguments: %s\n", optarg);
                    }

                    break;

         case 'M' :
                    mboxdir = optarg;
                    break;

         case 'i' :
                    imapserver = optarg;
                    break;

         case 'u' :
                    username = optarg;
                    break;

         case 'p' :
                    password = optarg;
                    break;

         case 'P' :
                    port = atoi(optarg);
                    break;

         case 'x' :
                    skiplist = optarg;
                    break;

         case 'F' :
                    folder = optarg;
                    break;

         case 'R' :
                    data.recursive_folder_names = 1;
                    break;

         case 'r' :
                    remove_after_successful_import = 1;
                    break;

         case 'q' :
                    quiet = 1;
                    break;

         case 'h' :
         case '?' :
                    usage();
                    break;


         default  : 
                    break;
       }
   }



   if(!mbox[0] && !mboxdir && !emlfile && !directory && !imapserver) usage();

   cfg = read_config(configfile);

   if(read_key(&cfg)){
      printf("%s\n", ERR_READING_KEY);
      return ERR;
   }

   mysql_init(&(sdata.mysql));
   mysql_options(&(sdata.mysql), MYSQL_OPT_CONNECT_TIMEOUT, (const char*)&cfg.mysql_connect_timeout);
   if(mysql_real_connect(&(sdata.mysql), cfg.mysqlhost, cfg.mysqluser, cfg.mysqlpwd, cfg.mysqldb, cfg.mysqlport, cfg.mysqlsocket, 0) == 0){
      printf("error: cant connect to mysql server\n");
      return ERR;
   }

   mysql_real_query(&(sdata.mysql), "SET NAMES utf8", strlen("SET NAMES utf8"));
   mysql_real_query(&(sdata.mysql), "SET CHARACTER SET utf8", strlen("SET CHARACTER SET utf8"));

   setlocale(LC_CTYPE, cfg.locale);

   (void) openlog("pilerimport", LOG_PID, LOG_MAIL);

   if(folder){
      data.folder = get_folder_id(&sdata, folder, 0);

      if(data.folder == 0){
         data.folder = add_new_folder(&sdata, folder, 0);
      }

      if(data.folder == 0){
         printf("error: cannot get/add folder '%s'\n", folder);
         mysql_close(&(sdata.mysql));
         return 0;
      }

   }

   load_rules(&sdata, &(data.archiving_rules), SQL_ARCHIVING_RULE_TABLE);
   load_rules(&sdata, &(data.retention_rules), SQL_RETENTION_RULE_TABLE);

   if(emlfile) rc = import_message(emlfile, &sdata, &data, &cfg);

   if(mbox[0]){
      for(i=0; i<n_mbox; i++){
         rc = import_from_mailbox(mbox[i], &sdata, &data, &cfg);
      }
   }
   if(mboxdir) rc = import_mbox_from_dir(mboxdir, &sdata, &data, &tot_msgs, &cfg);
   if(directory) rc = import_from_maildir(directory, &sdata, &data, &tot_msgs, &cfg);
   if(imapserver && username && password) rc = import_from_imap_server(imapserver, username, password, port, &sdata, &data, skiplist, &cfg);


   free_rule(data.archiving_rules);
   free_rule(data.retention_rules);

   mysql_close(&(sdata.mysql));

   if(quiet == 0) printf("\n");

   return rc;
}


