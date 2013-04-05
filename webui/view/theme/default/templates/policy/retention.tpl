
<h4><?php print $text_add_new_rule; ?></h4>

<form method="post" id="add1" name="add1" action="index.php?route=policy/retention" class="form-horizontal">
      <div class="control-group">
		<label class="control-label" for="from"><?php print $text_from; ?>:</label>
        <div class="controls">
            <input type="text" class="text" name="from" />
         </div>
      </div>
      <div class="control-group">
		<label class="control-label" for="to"><?php print $text_to; ?>:</label>
        <div class="controls">
            <input type="text" class="text" name="to" />
         </div>
      </div>      
      <div class="control-group">
		<label class="control-label" for="subject"><?php print $text_subject; ?>:</label>
        <div class="controls">
            <input type="text" class="text" name="subject" />
         </div>
      </div>
      <div class="control-group">
		<label class="control-label" for="size"><?php print $text_size; ?>:</label>
        <div class="controls">
            <select class="ruleselect" name="_size">
               <option value=">">&gt;</option>
               <option value="=">=</option>
               <option value="<">&lt;</option>
            </select>
            <input type="text" class="ruletext" name="size" />
         </div>
      </div>      
      <div class="control-group">
		<label class="control-label" for="attachment_type"><?php print $text_attachment_type; ?>:</label>
        <div class="controls">
            <input type="text" class="text" name="attachment_type" />
         </div>
      </div>
      <div class="control-group">
		<label class="control-label" for="attachment_size"><?php print $text_attachment_size; ?>:</label>
        <div class="controls">
            <select class="ruleselect" name="_attachment_size">
               <option value=">">&gt;</option>
               <option value="=">=</option>
               <option value="<">&lt;</option>
            </select>
            <input type="text" class="ruletext" name="attachment_size" />
         </div>
      </div>        
      <div class="control-group">
		<label class="control-label" for="spam"><?php print $text_spam; ?>:</label>
        <div class="controls">
            <select class="ruleselect" name="spam">
               <option value="-1">-</option>
               <option value="0"><?php print $text_not_spam; ?></option>
               <option value="1"><?php print $text_spam2; ?></option>
            </select>
         </div>
      </div>
      <div class="control-group">
		<label class="control-label" for="days"><?php print $text_days_to_retain; ?>:</label>
        <div class="controls">
            <input type="text" class="text" name="days" />
        </div>
      </div>      
      <div class="form-actions">
          <input type="submit" class="btn btn-primary" value="<?php print $text_add; ?>" /><input type="reset" class="btn" value="<?php print $text_cancel; ?>" />
      </div>
</form>


<h4><?php print $text_existing_rules; ?></h4>

<?php if(isset($rules)){ ?>

   <table id="ss1" class="table table-striped table-condensed">
      <tr class="domainrow">
         <th><?php print $text_from; ?></th>
         <th><?php print $text_to; ?></th>
         <th><?php print $text_subject; ?></th>
         <th><?php print $text_spam; ?></th>
         <th><?php print $text_size; ?></th>
         <th><?php print $text_attachment_type; ?></th>
         <th><?php print $text_attachment_size; ?></th>
         <th><?php print $text_days; ?></th>
         <th>&nbsp;</th>
      </tr>

<?php foreach($rules as $rule) { ?>
      <tr class="domainrow">
         <td><?php print $rule['from']; ?></td>
         <td><?php print $rule['to']; ?></td>
         <td><?php print $rule['subject']; ?></td>
         <td><?php if($rule['spam'] == -1) { print "-"; } else if($rule['spam'] == 0) { print $text_not_spam; } else { print $text_spam; } ?></td>
         <td><?php if($rule['size'] > 0) { print $rule['_size']; ?> <?php print $rule['size']; } ?></td>
         <td><?php print $rule['attachment_type']; ?></td>
         <td><?php if($rule['attachment_size'] > 0) { print $rule['_attachment_size']; ?> <?php print $rule['attachment_size']; } ?></td>
         <td><?php print $rule['days']; ?></td>
         <td><a href="index.php?route=policy/removeretention&amp;id=<?php print $rule['id']; ?>"><?php print $text_remove; ?></a></td>
      </tr>
<?php } ?>

   </table>

<?php } else { ?>
<div class="alert alert-error lead">
<?php print $text_not_found; ?>
</div>
<?php } ?>


