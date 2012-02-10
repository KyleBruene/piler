<?php

class ModelSearchMessage extends Model {


   public function get_store_path($id = '') {

      if($id == '') { return ''; }

      $len = strlen($id);

      return DIR_STORE . "/" . substr($id, $len-6, 2) . "/" . substr($id, $len-4, 2) . "/" . substr($id, $len-2, 2) . "/" . $id;
   }


   public function verify_message($id = '') {
      if($id == '') { return 0; }

      $q = $this->db->query("SELECT `size`, `hlen`, `digest`, `bodydigest`,`attachments` FROM " . TABLE_META . " WHERE piler_id=?", array($id));

      $digest = $q->row['digest'];
      $bodydigest = $q->row['bodydigest'];
      $size = $q->row['size'];
      $hlen = $q->row['hlen'];
      $attachments = $q->row['attachments'];

      $data = $this->get_raw_message($id);

      $_digest = openssl_digest($data, "SHA256");
      $_bodydigest = openssl_digest(substr($data, $hlen), "SHA256");

      $data = '';

      if($_digest == $digest && $_bodydigest == $bodydigest) { return 1; }

      return 0;
   }


   public function get_raw_message($id = '') {
      $data = '';

      if($id == '' || !preg_match("/^([0-9a-f]+)$/", $id)) { return $data; }

      $handle = popen(DECRYPT_BINARY . " $id", "r");

      while(($buf = fread($handle, DECRYPT_BUFFER_LENGTH))){
         $data .= $buf;
      }

      pclose($handle);

      return $data;
   }


   public function get_message_headers($id = '') {
      $data = '';

      //$f = $this->get_store_path($id);
      //$msg = $this->decrypt_and_uncompress_file($f.".m");
      $msg = $this->get_raw_message($id);

      $pos = strpos($msg, "\n\r\n");
      if($pos == false) {
         $pos = strpos($msg, "\n\n");
      }

      if($pos == false) { return $msg; }

      $data = substr($msg, 0, $pos);
      $msg = '';

      $data = preg_replace("/\</", "&lt;", $data);
      $data = preg_replace("/\>/", "&gt;", $data);

      return $data;
   }


   public function extract_message($id = '') {
      $header = "";
      $body_chunk = "";
      $is_header = 1;
      $state = "UNDEF";
      $b = array();
      $boundary = array();
      $text_plain = 1;
      $text_html = 0;
      $charset = "";
      $qp = $base64 = 0;
      $has_text_plain = 0;

      $from = $to = $subject = $date = $message = "";

      $msg = $this->get_raw_message($id);

//print "a: $msg\n";

      $a = explode("\n", $msg); $msg = "";

      while(list($k, $l) = each($a)){
            $l .= "\n";

            if(($l[0] == "\r" && $l[1] == "\n" && $is_header == 1) || ($l[0] == "\n" && $is_header == 1) ){
               $is_header = 0;
            }

            if(preg_match("/^Content-Type:/i", $l)) $state = "CONTENT_TYPE";
            if(preg_match("/^Content-Transfer-Encoding:/i", $l)) $state = "CONTENT_TRANSFER_ENCODING";

            if($state == "CONTENT_TYPE"){
               $x = strstr($l, "boundary");
               if($x){
                  $x = preg_replace("/boundary =/", "boundary=", $x);
                  $x = preg_replace("/boundary= /", "boundary=", $x);

                  $x = preg_replace("/\"/", "", $x);
                  $x = preg_replace("/\'/", "", $x);

                  $b = explode("boundary=", $x);

                  array_push($boundary, rtrim($b[count($b)-1]));
               }

               if(preg_match("/charset/i", $l)){
                  $types = explode(";", $l);
                  foreach ($types as $type){
                     if(preg_match("/charset/i", $type)){
                        $type = preg_replace("/[\"\'\ ]/", "", $type);

                        $x = explode("=", $type);
                        $charset = $x[1];
                     }
                  }
               }

               if(strstr($l, "text/plain")){ $text_plain = 1; $has_text_plain = 1; }
               if(strstr($l, "text/html")){ $text_html = 1; $text_plain = 0; }
            }

            if($state == "CONTENT_TRANSFER_ENCODING"){
               if(strstr($l, "quoted-printable")){ $qp = 1; }
               if(strstr($l, "base64")){ $base64 = 1; }
            }


            if($is_header == 1){
               if($l[0] != " " && $l[0] != "\t"){ $state = "UNDEF"; }
               if(preg_match("/^From:/i", $l)){ $state = "FROM"; }
               if(preg_match("/^To:/i", $l) || preg_match("/^Cc:/i", $l)){ $state = "TO"; }
               if(preg_match("/^Date:/i", $l)){ $state = "DATE"; }
               if(preg_match("/^Subject:/i", $l)){ $state = "SUBJECT"; }
               if(preg_match("/^Content-Type:/", $l)){ $state = "CONTENT_TYPE"; }

               $l = preg_replace("/</", "&lt;", $l);
               $l = preg_replace("/>/", "&gt;", $l);

               if($state == "FROM"){ $from .= preg_replace("/\r|\n/", "", $l); }
               if($state == "TO"){ $to .= preg_replace("/\r|\n/", "", $l); }
               if($state == "SUBJECT"){ $subject .= preg_replace("/\r|\n/", "", $l); }
               if($state == "DATE"){ $date .= preg_replace("/\r|\n/", "", $l); }
            }
            else {

               if($this->check_boundary($boundary, $l) == 1){

                  if($text_plain == 1 || $has_text_plain == 0) {
                  $message .= $this->flush_body_chunk($body_chunk, $charset, $qp, $base64, $text_plain, $text_html);
                  }

                  $text_plain = $text_html = $qp = $base64 = 0;

                  $charset = $body_chunk = "";

                  continue;
               }

               else if(($l[0] == "\r" && $l[1] == "\n") || $l[0] == "\n"){
                  $state = "BODY";
                  $body_chunk .= $l;
               }

               else if($state == "BODY"){
                  if($text_plain == 1 || $text_html == 1){ $body_chunk .= $l; }

               }

            }


         }

      if($body_chunk && ($text_plain == 1 || $has_text_plain == 0) ){
         $message .= $this->flush_body_chunk($body_chunk, $charset, $qp, $base64, $text_plain, $text_html);
      }


      return array('from' => $this->decode_my_str($from),
                   'to' => $this->decode_my_str($to),
                   'subject' => $this->decode_my_str($subject),
                   'date' => $this->decode_my_str($date),
                   'message' => $message
            );
   }


   private function check_boundary($boundary, $line) {

      for($i=0; $i<count($boundary); $i++){
         if(strstr($line, $boundary[$i])){
            return 1;
         }
      }

      return 0;
   }


   private function flush_body_chunk($chunk, $charset, $qp, $base64, $text_plain, $text_html) {

      if($qp == 1){
         $chunk = $this->qp_decode($chunk);
      }

      if($base64 == 1){
         $chunk = base64_decode($chunk);
      }

      if(!preg_match("/utf-8/i", $charset)){
         $chunk = utf8_encode($chunk);
      }

      if($text_plain == 1){
         $chunk = preg_replace("/</", "&lt;", $chunk);
         $chunk = preg_replace("/>/", "&gt;", $chunk);

         //$chunk = "<pre>\n" . $this->print_nicely($chunk) . "</pre>\n";
         $chunk = preg_replace("/\n/", "<br />\n", $chunk);
         $chunk = "\n" . $this->print_nicely($chunk);
      }

      if($text_html == 1){
         $chunk = preg_replace("/\<style([^\>]+)\>([\w\W]+)\<\/style\>/i", "", $chunk);

         if(ENABLE_REMOTE_IMAGES == 0) {
            $chunk = preg_replace("/style([\s]{0,}=[\s]{0,})\"([^\"]+)/", "style=\"xxxx", $chunk);
            $chunk = preg_replace("/style([\s]{0,}=[\s]{0,})\'([^\']+)/", "style=\'xxxx", $chunk);
         }

         $chunk = preg_replace("/\<body ([\w\s\;\"\'\#\d\:\-\=]+)\>/i", "<body>", $chunk);

         if(ENABLE_REMOTE_IMAGES == 0) { $chunk = preg_replace("/\<img([^\>]+)\>/i", "<img src=\"" . REMOTE_IMAGE_REPLACEMENT . "\" />", $chunk); }

         /* prevent scripts in the HTML part */

         $chunk = preg_replace("/document\.write/", "document.writeee", $chunk);
         $chunk = preg_replace("/<\s{0,}script([\w\W]+)\/script\s{0,}\>/i", "<!-- disabled javascript here -->", $chunk);
      }

      return $chunk;
   }


   private function print_nicely($chunk) {
      $k = 0;
      $nice_chunk = "";

      $x = explode(" ", $chunk);

      for($i=0; $i<count($x); $i++){
         $nice_chunk .= "$x[$i] ";
         $k += strlen($x[$i]);

         if(strstr($x[$i], "\n")){ $k = 0; }

         if($k > 70){ $nice_chunk .= "\n"; $k = 0; }
      }

      return $nice_chunk;
   }


   public function NiceSize($size) {
      if($size < 1000) return "1k";
      if($size < 100000) return round($size/1000) . "k";

      return sprintf("%.1f", $size/1000000) . "M";
   }


   private function qp_decode($l) {
      $res = "";
      $c = "";

      if($l == ""){ return ""; }

      /* remove soft breaks at the end of lines */

      if(preg_match("/\=\r\n/", $l)){ $l = preg_replace("/\=\r\n/", "", $l); }
      if(preg_match("/\=\n/", $l)){ $l = preg_replace("/\=\n/", "", $l); }

      for($i=0; $i<strlen($l); $i++){
         $c = $l[$i];

         if($c == '=' && ctype_xdigit($l[$i+1]) && ctype_xdigit($l[$i+2])){
            $a = $l[$i+1];
            $b = $l[$i+2];

            $c = chr(16*hexdec($a) + hexdec($b));

            $i += 2;
         }

         $res .= $c;

      }

      return $res;
   }


   public function decode_my_str($what = '') {
      $result = "";

      $what = rtrim($what);

      $a = preg_split("/\s/", $what);

      while(list($k, $v) = each($a)){
         $x = preg_match("/\?\=$/", $v);

         if( ($x == 0 && $k > 0) || ($x == 1 && $k == 1) ){
            $result .= " ";
         }

         $result .= $this->fix_encoded_string($v);
      }

      return $result;
   }


   private function fix_encoded_string($what = '') {
      $s = "";

      $what = rtrim($what, "\"\r\n");
      $what = ltrim($what, "\"");

      if(preg_match("/^\=\?/", $what) && preg_match("/\?\=$/", $what)){
         $what = preg_replace("/^\=\?/", "", $what);
         $what = preg_replace("/\?\=$/", "", $what);

         if(preg_match("/\?Q\?/i", $what)){
            $x = preg_replace("/^([\w\-]+)\?Q\?/i", "", $what);

            $s = quoted_printable_decode($x);
            $s = preg_replace("/_/", " ", $s);
         }

         if(preg_match("/\?B\?/i", $what)){
            $x = preg_replace("/^([\w\-]+)\?B\?/i", "", $what);

            $s = base64_decode($x);
            $s = preg_replace('/\0/', "*", $s);
         }


         if(!preg_match("/utf-8/i", $what)){
            $s = utf8_encode($s);
         }

      }
      else {
         $s = utf8_encode($what);
      }

      return $s;
   }


   public function get_message_tag($id = '', $uid = 0) {
      if($id == '' || $uid <= 0) { return ''; }

      $query = $this->db->query("SELECT `tag` FROM " . TABLE_TAG . "," . TABLE_META . " WHERE " . TABLE_TAG . ".id=" . TABLE_META . ".id AND uid=? AND piler_id=?", array($uid, $id));

      if(isset($query->row['tag'])) { return $query->row['tag']; }

      return '';
   }


   public function add_message_tag($id = '', $uid = 0, $tag = '') {
      if($id == '' || $uid <= 0) { return 0; }

      $query = $this->db->query("SELECT `id` FROM " . TABLE_META . " WHERE piler_id=?", array($id));

      if(isset($query->row['id']) && $query->row['id'] > 0) {

         $id = $query->row['id'];

         if($tag == '') {
            $query = $this->db->query("DELETE FROM " . TABLE_TAG . " WHERE uid=? AND id=?", array($uid, $id));
         } else {
            $query = $this->db->query("UPDATE " . TABLE_TAG . " SET tag=? WHERE uid=? AND id=?", array($tag, $uid, $id));
            if($this->db->countAffected() == 0) {
               $query = $this->db->query("INSERT INTO " . TABLE_TAG . " (id, uid, tag) VALUES(?,?,?)", array($id, $uid, $tag));
            }
         }

         return 1;
      }

      return 0;
   }


}

?>