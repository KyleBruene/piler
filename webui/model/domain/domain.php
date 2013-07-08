<?php

class ModelDomainDomain extends Model {

   public function getDomains() {
      $data = array();

      $query = $this->db->query("SELECT domain, mapped, ldap_id FROM " . TABLE_DOMAIN . " ORDER BY domain ASC");

      if(isset($query->rows)) {
         foreach($query->rows as $q) {

            $ldap = '';

            if($q['ldap_id'] > 0) {
               $query2 = $this->db->query("SELECT description FROM " . TABLE_LDAP . " WHERE id=?", array($q['ldap_id']));
               if(isset($query2->row)) { $ldap = $query2->row['description']; }
            }

            $data[] = array('domain' => $q['domain'], 'mapped' => $q['mapped'], 'ldap' => $ldap);

         }
      }

      return $data;
   }


   public function get_domains_by_string($s = '', $page = 0, $page_len = PAGE_LEN) {
      $from = (int)$page * (int)$page_len;

      if(strlen($s) < 1) { return array(); }

      $query = $this->db->query("SELECT domain FROM `" . TABLE_DOMAIN . "` WHERE domain LIKE ? ORDER BY domain ASC  LIMIT " . (int)$from . ", " . (int)$page_len, array($s . "%") );

      if(isset($query->rows)) { return $query->rows; }

      return array();
   }


   public function deleteDomain($domain = '') {
      if($domain == "") { return 0; }

      $query = $this->db->query("DELETE FROM " . TABLE_DOMAIN . " WHERE domain=?", array($domain));

      $rc = $this->db->countAffected();

      LOGGER("remove domain: $domain (rc=$rc)");

      return $rc;
   }


   public function addDomain($domain = '', $mapped = '', $ldap_id = 0) {
      if($domain == "" || $mapped == "") { return 0; }

      $domains = explode("\n", $domain);

      foreach ($domains as $domain) {
         $domain = rtrim($domain);
         $query = $this->db->query("INSERT INTO " . TABLE_DOMAIN . " (domain, mapped, ldap_id) VALUES (?,?,?)", array($domain, $mapped, $ldap_id));

         $rc = $this->db->countAffected();

         LOGGER("add domain: $domain (rc=$rc)");

         if($rc != 1){ return 0; }
      }

      return 1;
   }


}

?>
