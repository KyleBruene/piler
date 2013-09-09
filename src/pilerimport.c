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
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <getopt.h>
#include <syslog.h>
#include <piler.h>


#define SKIPLIST "junk,trash,spam,draft,"
#define MBOX_ARGS 1024

extern char *optarg;
extern int optind;

int quiet=0;
int dryrun=0;
int remove_after_successful_import = 0;
int import_from_gui=0;


int connect_to_imap_server(int sd, int *seq, char *username, char *password, int port, struct __data *data, int use_ssl);
int list_folders(int sd, int *seq, int use_ssl, struct __data *data);
int process_imap_folder(int sd, int *seq, char *folder, struct session_data *sdata, struct __data *data, int use_ssl, int dryrun, struct __config *cfg);
int connect_to_pop3_server(int sd, char *username, char *password, int port, struct __data *data, int use_ssl);
int process_pop3_emails(int sd, struct session_data *sdata, struct __data *data, int use_ssl, int dryrun, struct __config *cfg);
void close_connection(int sd, struct __data *data, int use_ssl);

void update_import_job_stat(struct session_data *sdata, struct __data *data);


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
            f = NULL;
            rc = import_message(fname, sdata, data, cfg);
            if(rc == ERR){
               printf("error importing: '%s'\n", fname);
               ret = ERR;
            }
            else unlink(fname);

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
      if(rc == ERR){
         printf("error importing: '%s'\n", fname);
         ret = ERR;
      }
      else unlink(fname);

      if(quiet == 0) printf("processed: %7d\r", tot_msgs); fflush(stdout);
   }

   fclose(F);

   return ret;
}


int import_mbox_from_dir(char *directory, struct session_data *sdata, struct __data *data, int *tot_msgs, struct __config *cfg){
   DIR *dir;
   struct dirent *de;
   int rc=ERR, ret=OK, i=0;
   int folder;
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
                  folder = get_folder_id(sdata, data, fname, data->folder);
                  if(folder == ERR_FOLDER){
                     folder = add_new_folder(sdata, data, fname, data->folder);

                     if(folder == ERR_FOLDER){
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
   int folder;
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

                  folder = get_folder_id(sdata, data, p, data->folder);
                  if(folder == ERR_FOLDER){
                     folder = add_new_folder(sdata, data, p, data->folder);

                     if(folder == ERR_FOLDER){
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
               else {
                  printf("error importing: '%s'\n", fname);
                  ret = ERR;
               }
 
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


int import_from_imap_server(char *server, char *username, char *password, int port, struct session_data *sdata, struct __data *data, char *skiplist, int dryrun, struct __config *cfg){
   int i, rc=ERR, ret=OK, sd, seq=1, skipmatch, use_ssl=0;
   char port_string[6];
   struct addrinfo hints, *res;
   struct node *q;


   snprintf(port_string, sizeof(port_string)-1, "%d", port);

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   if((rc = getaddrinfo(server, port_string, &hints, &res)) != 0){
      printf("getaddrinfo for '%s': %s\n", server, gai_strerror(rc));
      return ERR;
   }

   if(port == 993) use_ssl = 1;


   if((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
      printf("cannot create socket\n");
      ret = ERR;
      goto ENDE_IMAP;
   }

   if(connect(sd, res->ai_addr, res->ai_addrlen) == -1){
      printf("connect()\n");
      ret = ERR;
      goto ENDE_IMAP;
   }

   if(connect_to_imap_server(sd, &seq, username, password, port, data, use_ssl) == ERR){
      close(sd);
      ret = ERR;
      goto ENDE_IMAP;
   }


   inithash(data->imapfolders);

   rc = list_folders(sd, &seq, use_ssl, data);
   if(rc == ERR) goto ENDE_IMAP;


   for(i=0;i<MAXHASH;i++){
      q = data->imapfolders[i];
      while(q != NULL){

         if(q && q->str && strlen(q->str) > 1){

            skipmatch = 0;

            if(skiplist && strlen(skiplist) > 0){
               if(strstr(skiplist, q->str)) skipmatch = 1;
            }

            if(skipmatch == 1){
               if(quiet == 0) printf("SKIPPING FOLDER: %s\n", (char *)q->str);
            }
            else {
               if(quiet == 0) printf("processing folder: %s...\n", (char *)q->str);

               if(process_imap_folder(sd, &seq, q->str, sdata, data, use_ssl, dryrun, cfg) == ERR) ret = ERR;
            }

         }

         q = q->r;

      }
   }


   close_connection(sd, data, use_ssl);

ENDE_IMAP:
   freeaddrinfo(res);

   clearhash(data->imapfolders);

   return ret;
}


int import_from_pop3_server(char *server, char *username, char *password, int port, struct session_data *sdata, struct __data *data, int dryrun, struct __config *cfg){
   int rc, ret=OK, sd, use_ssl=0;
   char port_string[6];
   struct addrinfo hints, *res;

   snprintf(port_string, sizeof(port_string)-1, "%d", port);

   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   if((rc = getaddrinfo(server, port_string, &hints, &res)) != 0){
      printf("getaddrinfo for '%s': %s\n", server, gai_strerror(rc));
      return ERR;
   }

   if(port == 995) use_ssl = 1;

   if((sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
      printf("cannot create socket\n");
      ret = ERR;
      goto ENDE_POP3;
   }

   if(connect(sd, res->ai_addr, res->ai_addrlen) == -1){
      printf("connect()\n");
      ret = ERR;
      goto ENDE_POP3;
   }


   if(connect_to_pop3_server(sd, username, password, port, data, use_ssl) == ERR){
      close(sd);
      ret = ERR;
      goto ENDE_POP3;
   }

   if(process_pop3_emails(sd, sdata, data, use_ssl, dryrun, cfg) == ERR) ret = ERR;

   close_connection(sd, data, use_ssl);

ENDE_POP3:
   freeaddrinfo(res);

   return ret;
}


int read_gui_import_data(struct session_data *sdata, struct __data *data, char *skiplist, int dryrun, struct __config *cfg){
   int rc=ERR;
   char s_type[SMALLBUFSIZE], s_username[SMALLBUFSIZE], s_password[SMALLBUFSIZE], s_server[SMALLBUFSIZE];

   memset(s_type, 0, sizeof(s_type));
   memset(s_username, 0, sizeof(s_username));
   memset(s_password, 0, sizeof(s_password));
   memset(s_server, 0, sizeof(s_server));

   if(prepare_sql_statement(sdata, &(data->stmt_generic), SQL_PREPARED_STMT_GET_GUI_IMPORT_JOBS) == ERR) return ERR;

   p_bind_init(data);

   if(p_exec_query(sdata, data->stmt_generic, data) == ERR) goto ENDE;



   p_bind_init(data);

   data->sql[data->pos] = (char *)&(data->import->import_job_id); data->type[data->pos] = TYPE_LONG; data->len[data->pos] = sizeof(int); data->pos++;
   data->sql[data->pos] = &s_type[0]; data->type[data->pos] = TYPE_STRING; data->len[data->pos] = sizeof(s_type)-2; data->pos++;
   data->sql[data->pos] = &s_username[0]; data->type[data->pos] = TYPE_STRING; data->len[data->pos] = sizeof(s_username)-2; data->pos++;
   data->sql[data->pos] = &s_password[0]; data->type[data->pos] = TYPE_STRING; data->len[data->pos] = sizeof(s_password)-2; data->pos++;
   data->sql[data->pos] = &s_server[0]; data->type[data->pos] = TYPE_STRING; data->len[data->pos] = sizeof(s_server)-2; data->pos++;

   p_store_results(sdata, data->stmt_generic, data);

   if(p_fetch_results(data->stmt_generic) == OK) rc = OK;

   p_free_results(data->stmt_generic);



ENDE:
   close_prepared_statement(data->stmt_generic);

   data->import->processed_messages = 0;
   data->import->total_messages = 0;

   time(&(data->import->started));
   data->import->status = 1;
   update_import_job_stat(sdata, data);  

   if(strcmp(s_type, "pop3") == 0){
      rc = import_from_pop3_server(s_server, s_username, s_password, 110, sdata, data, dryrun, cfg);
   }

   if(strcmp(s_type, "imap") == 0){
      rc = import_from_imap_server(s_server, s_username, s_password, 143, sdata, data, skiplist, dryrun, cfg);
   }

   // don't set error in case of a problem, because it
   // will scare users looking at the gui progressbar
   /*if(rc == ERR){
      data->import->status = 3;
      update_import_job_stat(sdata, data);
   }*/

   return rc;
}


void usage(){
   printf("usage: pilerimport [-c <config file>] -e <eml file> | -m <mailbox file> | -d <directory> | -i <imap server> | -K <pop3 server> | -u <imap username> -p <imap password> -P <imap port> [-F <foldername>] [-R] [-r] [-q]\n");
   exit(0);
}


int main(int argc, char **argv){
   int i, c, rc=0, n_mbox=0, tot_msgs=0, port=143;
   char *configfile=CONFIG_FILE, *emlfile=NULL, *mboxdir=NULL, *mbox[MBOX_ARGS], *directory=NULL;
   char *imapserver=NULL, *pop3server=NULL, *username=NULL, *password=NULL, *skiplist=SKIPLIST, *folder=NULL;
   struct session_data sdata;
   struct __config cfg;
   struct __data data;
   struct import import;

   for(i=0; i<MBOX_ARGS; i++) mbox[i] = NULL;

   data.folder = 0;
   data.recursive_folder_names = 0;

   import.import_job_id = import.total_messages = import.processed_messages = 0;
   import.started = import.updated = import.finished = 0;

   data.import = &import;

   inithash(data.mydomains);
   initrules(data.archiving_rules);
   initrules(data.retention_rules);

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
            {"pop3server",   required_argument,  0,  'K' },
            {"username",     required_argument,  0,  'u' },
            {"password",     required_argument,  0,  'p' },
            {"port",         required_argument,  0,  'P' },
            {"skiplist",     required_argument,  0,  'x' },
            {"folder",       required_argument,  0,  'F' },
            {"quiet",        required_argument,  0,  'q' },
            {"recursive",    required_argument,  0,  'R' },
            {"remove-after-import",    required_argument,  0,  'r' },
            {"gui-import",   required_argument,  0,  'G' },
            {"dry-run",      required_argument,  0,  'D' },
            {"help",         no_argument,        0,  'h' },
            {0,0,0,0}
         };

      int option_index = 0;

      c = getopt_long(argc, argv, "c:m:M:e:d:i:K:u:p:P:x:F:GDRrqh?", long_options, &option_index);
#else
      c = getopt(argc, argv, "c:m:M:e:d:i:K:u:p:P:x:F:GDRrqh?");
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

         case 'K' :
                    pop3server = optarg;
                    if(port == 143) port = 110;
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

         case 'G' :
                    import_from_gui = 1;
                    break;

         case 'D' :
                    dryrun = 1;
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



   if(!mbox[0] && !mboxdir && !emlfile && !directory && !imapserver && !pop3server && import_from_gui == 0) usage();

   cfg = read_config(configfile);

   if(read_key(&cfg)){
      printf("%s\n", ERR_READING_KEY);
      return ERR;
   }


   if(open_database(&sdata, &cfg) == ERR) return 0;


   setlocale(LC_CTYPE, cfg.locale);

   (void) openlog("pilerimport", LOG_PID, LOG_MAIL);

#ifdef HAVE_MEMCACHED
   memcached_init(&(data.memc), cfg.memcached_servers, 11211);
#endif

   if(folder){
      data.folder = get_folder_id(&sdata, &data, folder, 0);

      if(data.folder == ERR_FOLDER){
         data.folder = add_new_folder(&sdata, &data, folder, 0);
      }

      if(data.folder == ERR_FOLDER){
         printf("error: cannot get/add folder '%s'\n", folder);
         close_database(&sdata);
         return 0;
      }

   }

   load_rules(&sdata, &data, data.archiving_rules, SQL_ARCHIVING_RULE_TABLE);
   load_rules(&sdata, &data, data.retention_rules, SQL_RETENTION_RULE_TABLE);

   load_mydomains(&sdata, &data, &cfg);

   if(emlfile) rc = import_message(emlfile, &sdata, &data, &cfg);

   if(mbox[0]){
      for(i=0; i<n_mbox; i++){
         rc = import_from_mailbox(mbox[i], &sdata, &data, &cfg);
      }
   }
   if(mboxdir) rc = import_mbox_from_dir(mboxdir, &sdata, &data, &tot_msgs, &cfg);
   if(directory) rc = import_from_maildir(directory, &sdata, &data, &tot_msgs, &cfg);
   if(imapserver && username && password) rc = import_from_imap_server(imapserver, username, password, port, &sdata, &data, skiplist, dryrun, &cfg);
   if(pop3server && username && password) rc = import_from_pop3_server(pop3server, username, password, port, &sdata, &data, dryrun, &cfg);
   if(import_from_gui == 1) rc = read_gui_import_data(&sdata, &data, skiplist, dryrun, &cfg);

   clearrules(data.archiving_rules);
   clearrules(data.retention_rules);

   clearhash(data.mydomains);

   close_database(&sdata);

   if(quiet == 0) printf("\n");

   return rc;
}


