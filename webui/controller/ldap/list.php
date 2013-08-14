<?php


class ControllerLdapList extends Controller {
   private $error = array();

   public function index(){

      $this->id = "content";
      $this->template = "ldap/list.tpl";
      $this->layout = "common/layout";


      $request = Registry::get('request');
      $db = Registry::get('db');

      $this->load->model('saas/ldap');

      $this->document->title = $this->data['text_ldap'];


      $this->data['username'] = Registry::get('username');


      $this->data['page'] = 0;
      $this->data['page_len'] = get_page_length();

      $this->data['total'] = 0;

      $this->data['entries'] = array();

      $this->data['id'] = -1;

      $this->data['ldap_types'] = Registry::get('ldap_types');

      if(isset($this->request->get['id'])) { $this->data['id'] = $this->request->get['id']; }

      /* check if we are admin */

      if(Registry::get('admin_user') == 1) {

         if($this->request->server['REQUEST_METHOD'] == 'POST') {

            if($this->validate() == true) {

               if(isset($this->request->post['id'])) {
                  if($this->model_saas_ldap->update($this->request->post) == 1) {
                     $this->data['x'] = $this->data['text_successfully_modified'];
                  } else {
                     $this->data['errorstring'] = $this->data['text_failed_to_modify'];
                     // set ldap ID to be the submitted id
                     if (isset($this->request->post['id'])) { $this->data['id'] = $this->request->post['id']; }
                  }
               }
               else {
                  if($this->model_saas_ldap->add($this->request->post) == 1) {
                     $this->data['x'] = $this->data['text_successfully_added'];
                  } else {
                     $this->data['errorstring'] = $this->data['text_failed_to_add'];
                  }
               }
            }
            else {
                $this->data['errorstring'] = $this->data['text_error_message'];
                $this->data['errors'] = $this->error;
               // set ldap ID to be the submitted id
               if (isset($this->request->post['id'])) { $this->data['id'] = $this->request->post['id']; }
            } 
         }

         if(isset($this->request->get['id'])) {
            $this->data['a'] = $this->model_saas_ldap->get($this->data['id']);
         }
         else {
            $this->data['entries'] = $this->model_saas_ldap->get();
         }
         
         if ( isset($this->data['errorstring']) ) {
            // use posted values if they differ from database values (ie - form was submitted but failed validation)
            if (isset($this->request->post['ldap_type'])) { $this->data['a']['ldap_type'] = $this->request->post['ldap_type'];}
            if (isset($this->request->post['description'])) { $this->data['a']['description'] = $this->request->post['description'];}
         }
         
      }
      else {
         $this->template = "common/error.tpl";
         $this->data['errorstring'] = $this->data['text_you_are_not_admin'];
      }


      $this->render();
   }


   private function validate() {
      // description is required and must be 1 or more characters in length to meet this
      if(!isset($this->request->post['description']) || strlen($this->request->post['description']) < 1) {
         $this->error['description'] = $this->data['text_field_required'];
      }
      // ldap_host is required and must be 1 or more characters in length to meet this
      if(!isset($this->request->post['ldap_host']) || strlen($this->request->post['ldap_host']) < 1) {
         $this->error['ldap_host'] = $this->data['text_field_required'];
      }
      // ldap_base_dn is required and must be 1 or more characters in length to meet this
      if(!isset($this->request->post['ldap_base_dn']) || strlen($this->request->post['ldap_base_dn']) < 1) {
         $this->error['ldap_base_dn'] = $this->data['text_field_required'];
      }
      // ldap_bind_dn is required and must be 1 or more characters in length to meet this
      if(!isset($this->request->post['ldap_bind_dn']) || strlen($this->request->post['ldap_bind_dn']) < 1) {
         $this->error['ldap_bind_dn'] = $this->data['text_field_required'];
      }
      // ldap_bind_pw is required and must be 1 or more characters in length to meet this
      if(!isset($this->request->post['ldap_bind_pw']) || strlen($this->request->post['ldap_bind_pw']) < 1) {
         $this->error['ldap_bind_pw'] = $this->data['text_field_required'];
      }

      if (!$this->error) {
         return true;
      } else {
         return false;
      }

   }



}

?>
