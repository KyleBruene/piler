/*
 * parser_utils.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <piler.h>
#include "trans.h"
#include "html.h"


void init_state(struct _state *state){
   int i;

   state->message_state = MSG_UNDEF;

   state->line_num = 0;

   state->is_header = 1;
   state->is_1st_header = 1;

   state->textplain = 1; /* by default we are a text/plain message */
   state->texthtml = 0;
   state->message_rfc822 = 0;

   state->base64 = 0;
   state->utf8 = 0;

   state->qp = 0;

   state->htmltag = 0;
   state->style = 0;

   state->skip_html = 0;

   state->content_type_is_set = 0;

   memset(state->message_id, 0, SMALLBUFSIZE);
   memset(state->miscbuf, 0, MAX_TOKEN_LEN);
   memset(state->qpbuf, 0, MAX_TOKEN_LEN);

   memset(state->filename, 0, TINYBUFSIZE);
   memset(state->type, 0, TINYBUFSIZE);

   state->has_to_dump = 0;
   state->fd = -1;
   state->mfd = -1;
   state->realbinary = 0;
   state->octetstream = 0;
   state->pushed_pointer = 0;
   state->saved_size = 0;

   state->boundaries = NULL;
   state->rcpt = NULL;

   state->n_attachments = 0;

   for(i=0; i<MAX_ATTACHMENTS; i++){
      state->attachments[i].size = 0;
      memset(state->attachments[i].type, 0, TINYBUFSIZE);
      memset(state->attachments[i].filename, 0, TINYBUFSIZE);
      memset(state->attachments[i].internalname, 0, TINYBUFSIZE);
      memset(state->attachments[i].digest, 0, 2*DIGEST_LENGTH+1);
   }

   memset(state->b_from, 0, SMALLBUFSIZE);
   memset(state->b_to, 0, SMALLBUFSIZE);
   memset(state->b_subject, 0, MAXBUFSIZE);
   memset(state->b_body, 0, BIGBUFSIZE);
}


unsigned long parse_date_header(char *s){
   char *p;
   unsigned long ts=0;
   struct tm tm;

   s += 5;
   p = s;

   if(*p == ' '){ p++; s++; }

   p = strchr(s, ',');
   if(!p) goto ENDE;

   *p = '\0';
   if(strcmp(s, "Mon") == 0) tm.tm_wday = 1;
   else if(strcmp(s, "Tue") == 0) tm.tm_wday = 2;
   else if(strcmp(s, "Wed") == 0) tm.tm_wday = 3;
   else if(strcmp(s, "Thu") == 0) tm.tm_wday = 4;
   else if(strcmp(s, "Fri") == 0) tm.tm_wday = 5;
   else if(strcmp(s, "Sat") == 0) tm.tm_wday = 6;
   else if(strcmp(s, "Sun") == 0) tm.tm_wday = 0;
   s += 5;

   p = strchr(s, ' '); if(!p) goto ENDE;
   *p = '\0'; tm.tm_mday = atoi(s); s += 3;

   p = strchr(s, ' '); if(!p) goto ENDE;
   *p = '\0';
   if(strcmp(s, "Jan") == 0) tm.tm_mon = 0;
   else if(strcmp(s, "Feb") == 0) tm.tm_mon = 1;
   else if(strcmp(s, "Mar") == 0) tm.tm_mon = 2;
   else if(strcmp(s, "Apr") == 0) tm.tm_mon = 3;
   else if(strcmp(s, "May") == 0) tm.tm_mon = 4;
   else if(strcmp(s, "Jun") == 0) tm.tm_mon = 5;
   else if(strcmp(s, "Jul") == 0) tm.tm_mon = 6;
   else if(strcmp(s, "Aug") == 0) tm.tm_mon = 7;
   else if(strcmp(s, "Sep") == 0) tm.tm_mon = 8;
   else if(strcmp(s, "Oct") == 0) tm.tm_mon = 9;
   else if(strcmp(s, "Nov") == 0) tm.tm_mon = 10;
   else if(strcmp(s, "Dec") == 0) tm.tm_mon = 11;
   s = p+1;

   p = strchr(s, ' '); if(!p) goto ENDE;
   tm.tm_year = atoi(s) - 1900; s = p+1;

   p = strchr(s, ':'); if(!p) goto ENDE;
   *p = '\0'; tm.tm_hour = atoi(s); s = p+1;

   p = strchr(s, ':'); if(!p) goto ENDE;
   *p = '\0'; tm.tm_min = atoi(s); s = p+1;

   p = strchr(s, ' '); if(!p) goto ENDE;
   *p = '\0'; tm.tm_sec = atoi(s); s = p+1;

   tm.tm_isdst = -1;

   ts = mktime(&tm);

ENDE:
   return ts;
}


int isHexNumber(char *p){
   for(; *p; p++){
      if(!(*p == '-' || (*p >= 0x30 && *p <= 0x39) || (*p >= 0x41 && *p <= 0x46) || (*p >= 0x61 && *p <= 0x66)) )
         return 0;
   }

   return 1;
}


int extract_boundary(char *p, struct _state *state){
   char *q;

   p += strlen("boundary");

   q = strchr(p, '"');
   if(q) *q = ' ';

   /*
    * I've seen an idiot spammer using the following boundary definition in the header:
    *
    * Content-Type: multipart/alternative;
    *     boundary=3D"b1_52b92b01a943615aff28b7f4d2f2d69d"
    */

   if(strncmp(p, "=3D", 3) == 0){
      *(p+3) = '=';
      p += 3;
   }

   p = strchr(p, '=');
   if(p){
      p++;
      for(; *p; p++){
         if(isspace(*p) == 0)
            break;
      }
      q = strrchr(p, '"');
      if(q) *q = '\0';

      q = strrchr(p, '\r');
      if(q) *q = '\0';

      q = strrchr(p, '\n');
      if(q) *q = '\0';

      append_list(&(state->boundaries), p);

      return 1;
   }

   return 0;
}


void fixupEncodedHeaderLine(char *buf){
   char *sb, *sq, *p, *q, *r, *s, v[SMALLBUFSIZE], puf[MAXBUFSIZE];
   char *start, *end;

   memset(puf, 0, sizeof(puf));

   q = buf;

   do {
      q = split_str(q, " ", v, sizeof(v)-1);

      p = v;

      do {
         start = strstr(p, "=?");
         if(start){
            *start = '\0';
            if(strlen(p) > 0){
               strncat(puf, p, sizeof(puf)-1);
            }

            start++;

            s = NULL;
            sb = strcasestr(start, "?B?"); if(sb) s = sb;
            sq = strcasestr(start, "?Q?"); if(sq) s = sq;

            if(s){
               end = strstr(s+3, "?=");
               if(end){
                  *end = '\0';

                  if(sb){ decodeBase64(s+3); }
                  if(sq){ decodeQP(s+3); r = s + 3; for(; *r; r++){ if(*r == '_') *r = ' '; } }

                  /* encode everything if it's not utf-8 encoded */
                  if(strncasecmp(start+1, "utf-8", 5)) utf8_encode((unsigned char*)s+3);

                  strncat(puf, s+3, sizeof(puf)-1);

                  p = end + 2;
               }
            }
            else {
               strncat(puf, start, sizeof(puf)-1);

               break;
            }
         }
         else {
            strncat(puf, p, sizeof(puf)-1);
            break;
         }

      } while(p);

      if(q) strncat(puf, " ", sizeof(puf)-1);

   } while(q);

   snprintf(buf, MAXBUFSIZE-1, "%s", puf);
}


void fixupSoftBreakInQuotedPritableLine(char *buf, struct _state *state){
   int i=0;
   char *p, puf[MAXBUFSIZE];

   if(strlen(state->qpbuf) > 0){
      memset(puf, 0, MAXBUFSIZE);
      strncpy(puf, state->qpbuf, MAXBUFSIZE-1);
      strncat(puf, buf, MAXBUFSIZE-1);

      memset(buf, 0, MAXBUFSIZE);
      memcpy(buf, puf, MAXBUFSIZE);

      memset(state->qpbuf, 0, MAX_TOKEN_LEN);
   }

   if(buf[strlen(buf)-1] == '='){
      buf[strlen(buf)-1] = '\0';
      i = 1;
   }

   if(i == 1){
      p = strrchr(buf, ' ');
      if(p){
         memset(state->qpbuf, 0, MAX_TOKEN_LEN);
         if(strlen(p) < MAX_TOKEN_LEN-1){
            //snprintf(state->qpbuf, MAX_TOKEN_LEN-1, "%s", p);
            memcpy(&(state->qpbuf[0]), p, MAX_TOKEN_LEN-1);

            *p = '\0';
         }

      }
   }
}


void fixupBase64EncodedLine(char *buf, struct _state *state){
   char *p, puf[MAXBUFSIZE];

   if(strlen(state->miscbuf) > 0){
      memset(puf, 0, sizeof(puf));
      strncpy(puf, state->miscbuf, sizeof(puf)-1);
      strncat(puf, buf, sizeof(puf)-1);

      memset(buf, 0, MAXBUFSIZE);
      memcpy(buf, puf, MAXBUFSIZE);

      memset(state->miscbuf, 0, MAX_TOKEN_LEN);
   }

   if(buf[strlen(buf)-1] != '\n'){
      p = strrchr(buf, ' ');
      if(p){
         //strncpy(state->miscbuf, p+1, MAX_TOKEN_LEN-1);
         memcpy(&(state->miscbuf[0]), p+1, MAX_TOKEN_LEN-1);

         *p = '\0';
      }
   }
}


void markHTML(char *buf, struct _state *state){
   char *s, puf[MAXBUFSIZE], html[SMALLBUFSIZE];
   int k=0, j=0, pos=0;

   memset(puf, 0, MAXBUFSIZE);
   memset(html, 0, SMALLBUFSIZE);

   s = buf;

   for(; *s; s++){
      if(*s == '<'){
         state->htmltag = 1;
         puf[k] = ' ';
         k++;

         memset(html, 0, SMALLBUFSIZE); j=0;

         pos = 0;

         //printf("start html:%c\n", *s);
      }

      if(state->htmltag == 1){
    
         if(j == 0 && *s == '!'){
            state->skip_html = 1;
            //printf("skiphtml=1\n");
         }

         if(state->skip_html == 0){ 
            if(*s != '>' && *s != '<' && *s != '"'){
               //printf("j=%d/%c", j, *s);
               html[j] = tolower(*s);
               if(j < SMALLBUFSIZE-10) j++;
            }

            if(isspace(*s)){
               if(j > 0){
                  k += appendHTMLTag(puf, html, pos, state);
                  memset(html, 0, SMALLBUFSIZE); j=0;
               }
               pos++;
            }
         }
      }
      else {
         if(state->style == 0){
            puf[k] = *s;
            k++;
         }
      }

      if(*s == '>'){
         state->htmltag = 0;
         state->skip_html = 0;

         //printf("skiphtml=0\n");
         //printf("end html:%c\n", *s);

         //strncat(html, " ", SMALLBUFSIZE-1);

         if(j > 0){
            strncat(html, " ", SMALLBUFSIZE-1);
            k += appendHTMLTag(puf, html, pos, state);
            memset(html, 0, SMALLBUFSIZE); j=0;
         }
      }

   }

   //printf("append last in line:*%s*, html=+%s+, j=%d\n", puf, html, j);
   if(j > 0){ k += appendHTMLTag(puf, html, pos, state); }

   strcpy(buf, puf);
}


int appendHTMLTag(char *buf, char *htmlbuf, int pos, struct _state *state){
   char *p, html[SMALLBUFSIZE];
   int len;

   if(pos == 0 && strncmp(htmlbuf, "style ", 6) == 0) state->style = 1;
   if(pos == 0 && strncmp(htmlbuf, "/style ", 7) == 0) state->style = 0;

   return 0;

   //printf("appendHTML: pos:%d, +%s+\n", pos, htmlbuf);

   if(state->style == 1) return 0;

   if(strlen(htmlbuf) == 0) return 0;

   snprintf(html, SMALLBUFSIZE-1, "HTML*%s", htmlbuf);
   len = strlen(html);

   if(len > 8 && strchr(html, '=')){
      p = strstr(html, "cid:");
      if(p){
         *(p+3) = '\0';
         strncat(html, " ", SMALLBUFSIZE-1);
      }

      strncat(buf, html, MAXBUFSIZE-1);
      return len;
   }

   if(strstr(html, "http") ){
      strncat(buf, html+5, MAXBUFSIZE-1);
      return len-5;
   }

   return 0;
}


void translateLine(unsigned char *p, struct _state *state){
   int url=0;

   for(; *p; p++){

      if( (state->message_state == MSG_RECEIVED || state->message_state == MSG_FROM || state->message_state == MSG_TO || state->message_state == MSG_CC) && *p == '@'){ continue; }

      if(state->message_state == MSG_SUBJECT && (*p == '%' || *p == '_' || *p == '&') ){ continue; }

      if(state->message_state == MSG_CONTENT_TYPE && *p == '_' ){ continue; }

      if(*p == '.' || *p == '-'){ continue; }

      if(strncasecmp((char *)p, "http://", 7) == 0){ p += 7; url = 1; continue; }
      if(strncasecmp((char *)p, "https://", 8) == 0){ p += 8; url = 1; continue; }

      if(url == 1 && (*p == '.' || *p == '-' || *p == '_' || *p == '/' || isalnum(*p)) ) continue;
      if(url == 1) url = 0;

      if(state->texthtml == 1 && state->message_state == MSG_BODY && strncmp((char *)p, "HTML*", 5) == 0){
         p += 5;
         while(isspace(*p) == 0){
            p++;
         }
      }

      if(delimiter_characters[(unsigned int)*p] != ' ')
         *p = ' ';
      else {
         *p = tolower(*p);
      }

   }

}


void fix_email_address_for_sphinx(char *s){
   for(; *s; s++){
      if(*s == '@' || *s == '.' || *s == '+') *s = 'X';
   }
}


/*
 * reassemble 'V i a g r a' to 'Viagra'
 */

void reassembleToken(char *p){
   int i, k=0;

   for(i=0; i<strlen(p); i++){

      if(isspace(*(p+i-1)) && !isspace(*(p+i)) && isspace(*(p+i+1)) && !isspace(*(p+i+2)) && isspace(*(p+i+3)) && !isspace(*(p+i+4)) && isspace(*(p+i+5)) ){
         p[k] = *(p+i); k++;
         p[k] = *(p+i+2); k++;
         p[k] = *(p+i+4); k++;

         i += 5;
      }
      else {
         p[k] = *(p+i);
         k++;
      }
   }

   p[k] = '\0';
}


void degenerateToken(unsigned char *p){
   int i=1, d=0, dp=0;
   unsigned char *s;

   /* quit if this the string does not end with a punctuation character */

   if(!ispunct(*(p+strlen((char *)p)-1)))
      return;

   s = p;

   for(; *p; p++){
      if(ispunct(*p)){
         d = i;

         if(!ispunct(*(p-1)))
            dp = d;
      }
      else
         d = dp = i;

      i++;
   }

   *(s+dp) = '\0';

   if(*(s+dp-1) == '.' || *(s+dp-1) == '!' || *(s+dp-1) == '?') *(s+dp-1) = '\0';
}


void fixURL(char *url){
   char *p, *q, fixed_url[SMALLBUFSIZE];

   memset(fixed_url, 0, sizeof(fixed_url));

   p = url;

   if(strncasecmp(url, "http://", 7) == 0) p += 7;
   if(strncasecmp(url, "https://", 8) == 0) p += 8;

   q = strchr(p, '/');
   if(q) *q = '\0';

   snprintf(fixed_url, sizeof(fixed_url)-1, "__URL__%s ", p);
   fix_email_address_for_sphinx(fixed_url);

   strcpy(url, fixed_url);   
}


int extractNameFromHeaderLine(char *s, char *name, char *resultbuf){
   int rc=0;
   char buf[TINYBUFSIZE], *p, *q;

   snprintf(buf, sizeof(buf)-1, "%s", s);

   p = strstr(buf, name);
   if(p){
      p += strlen(name);
      p = strchr(p, '=');
      if(p){
         p++;
         q = strrchr(p, ';');
         if(q) *q = '\0';
         q = strrchr(p, '"');
         if(q){
            *q = '\0';
            p = strchr(p, '"');
            if(p){
               p++;
            }
         }
         snprintf(resultbuf, TINYBUFSIZE-1, "%s", p);
         rc = 1;
      }
   }

   return rc;
}



