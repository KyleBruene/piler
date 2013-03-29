<?php  


class ControllerCommonMenu extends Controller {

   protected function index() {

      $this->id = "menu";
      $this->template = "common/menu.tpl";

      $db = Registry::get('db');

      $this->data['admin_user'] = Registry::get('admin_user');
      $this->data['auditor_user'] = Registry::get('auditor_user');
      $this->data['readonly_admin'] = Registry::get('readonly_admin');

      $this->render();
   }


}



?>
