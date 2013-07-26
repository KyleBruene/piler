<h4><?php if(isset($a['description'])) { print $text_edit_entry; } else { print $text_add_new_entry; } ?></h4>

<?php if(isset($x)){ ?>
    <div class="alert alert-info"><?php print $x; ?></div>
<?php } ?>

<form method="post" name="add1" action="index.php?route=ldap/list" class="form-horizontal">

 <?php if(isset($a['description'])) { ?>
    <input type="hidden" name="id" id="id" value="<?php print $id; ?>" />
 <?php } ?>

    <div class="control-group">
       <label class="control-label" for="ldap_type"><?php print $text_ldap_type; ?>:</label>
       <div class="controls">
          <select name="ldap_type" id="ldap_type">
       <?php while(list($k, $v) = each($ldap_types)) { ?>
          <option value="<?php print $v; ?>"<?php if(isset($a['ldap_type']) && $a['ldap_type'] == $v) { ?> selected="selected"<?php } ?>><?php print $v; ?></option>
       <?php } ?>
          </select>
       </div>
    </div>
    <div class="control-group">
		<label class="control-label" for="description"><?php print $text_description; ?>:</label>
        <div class="controls">
            <input type="text" class="text" name="description" id="description" placeholder="" value="<?php if(isset($a['description'])) { print $a['description']; } ?>" />
        </div>
    </div>
    <div class="control-group">
       <label class="control-label" for="ldap_host"><?php print $text_ldap_host; ?>:</label>
       <div class="controls">
          <input type="text" class="text" name="ldap_host" id="ldap_host" placeholder="" value="<?php if(isset($a['ldap_host'])) { print $a['ldap_host']; } ?>" />
       </div>
    </div>
    <div class="control-group">
       <label class="control-label" for="ldap_base_dn"><?php print $text_ldap_base_dn; ?>:</label>
       <div class="controls">
          <input type="text" class="text" name="ldap_base_dn" id="ldap_base_dn" placeholder="" value="<?php if(isset($a['ldap_base_dn'])) { print $a['ldap_base_dn']; } ?>" />
       </div>
    </div>
    <div class="control-group">
       <label class="control-label" for="ldap_bind_dn"><?php print $text_ldap_bind_dn; ?>:</label>
       <div class="controls">
          <input type="text" class="text" name="ldap_bind_dn" id="ldap_bind_dn" placeholder="" value="<?php if(isset($a['ldap_bind_dn'])) { print $a['ldap_bind_dn']; } ?>" />
       </div>
    </div>
    <div class="control-group">
       <label class="control-label" for="ldap_bind_pw"><?php print $text_ldap_bind_pw; ?>:</label>
       <div class="controls">
          <input type="password" class="password" name="ldap_bind_pw" id="ldap_bind_pw" placeholder="" value="<?php if(isset($a['ldap_bind_pw'])) { print $a['ldap_bind_pw']; } ?>" /> <input type="button" value="<?php print $text_test_connection; ?>" class="btn btn-danger" onclick="Piler.test_ldap_connection(); return false;" /> <span id="LDAPTEST"></span>
       </div>
    </div>
    <div class="control-group">
       <label class="control-label" for="ldap_auditor_member_dn"><?php print $text_ldap_auditor_member_dn; ?>:</label>
       <div class="controls">
          <input type="text" class="text" name="ldap_auditor_member_dn" id="ldap_auditor_member_dn" placeholder="" value="<?php if(isset($a['ldap_auditor_member_dn'])) { print $a['ldap_auditor_member_dn']; } ?>" />
       </div>
    </div>

    <div class="form-actions">
        <input type="submit" value="<?php if(isset($a['description'])) { print $text_modify; } else { print $text_add; } ?>" class="btn btn-primary" />
        <input type="reset" value="<?php print $text_clear; ?>" class="btn" onclick="Piler.clear_ldap_test();" />
    </div>

</form>

<?php if($id == -1) { ?>

<h4><?php print $text_existing_entries; ?></h4>

<div class="listarea">

<?php if(isset($entries)){ ?>

   <table id="ss1" class="table table-striped table-condensed">
      <tr>
         <th class="domaincell"><?php print $text_description; ?></th>
         <th class="domaincell"><?php print $text_ldap_type; ?></th>
         <th class="domaincell"><?php print $text_ldap_host; ?></th>
         <td class="domaincell"><?php print $text_ldap_base_dn; ?></td>
         <td class="domaincell"><?php print $text_ldap_bind_dn; ?></td>
         <td class="domaincell"><?php print $text_ldap_auditor_member_dn; ?></td>
         <th class="domaincell">&nbsp;</th>
         <th class="domaincell">&nbsp;</th>
      </tr>

<?php foreach($entries as $e) { ?>
      <tr>
         <td class="domaincell"><?php print $e['description']; ?></td>
         <td class="domaincell"><?php print $e['ldap_type']; ?></td>
         <td class="domaincell"><?php print $e['ldap_host']; ?></td>
         <td class="domaincell"><?php print $e['ldap_base_dn']; ?></td>
         <td class="domaincell"><?php print $e['ldap_bind_dn']; ?></td>
         <td class="domaincell"><?php print $e['ldap_auditor_member_dn']; ?></td>
         <td class="domaincell"><a href="index.php?route=ldap/list&amp;id=<?php print $e['id']; ?>"><?php print $text_edit; ?></a></td>
         <td class="domaincell"><a href="index.php?route=ldap/remove&amp;id=<?php print $e['id']; ?>&amp;description=<?php print urlencode($e['description']); ?>&amp;confirmed=1" onclick="if(confirm('<?php print $text_remove; ?>: ' + '\'<?php print $e['description']; ?>\'')) return true; return false;"><?php print $text_remove; ?></a></td>
      </tr>
<?php } ?>

   </table>

<?php } else { ?>
    <div class="alert alert-error lead">
    <?php print $text_not_found; ?>
    </div>
<?php } ?>

<?php } ?>


</div>
