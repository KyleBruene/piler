<div id="restorebox">
<?php if(Registry::get('auditor_user') == 1 && count($rcpt) > 0) { ?>
<?php foreach($rcpt as $r) { ?>
      <input type="checkbox" class="restorebox" id="rcpt_<?php print $r; ?>" name="rcpt_<?php print $r; ?>" value="1" /> <?php print $r; ?><br />
<?php } ?>
<br />
<input type="button" id="restore_button" name="restore_button" value="<?php print $text_restore; ?>" class="restore_to_mailbox_button" onclick="Piler.restore_message_for_recipients(<?php print $id; ?>, '<?php print $text_restored; ?>', '<?php print $text_select_recipients; ?>');" />
<input type="button" value="<?php print $text_cancel; ?>" class="restore_to_mailbox_button" onclick="$('#restorebox').hide();" />
<?php } ?>

</div>

<p>
   <a class="messagelink" href="index.php?route=message/download&amp;id=<?php print $id; ?>"><?php print $text_download_message; ?></a> |
<?php if(SMARTHOST || ENABLE_IMAP_AUTH == 1) { if(Registry::get('auditor_user') == 1) { ?>
   <a class="messagelink" href="#" onclick="$('#restorebox').show();"><?php print $text_restore_to_mailbox; ?></a> |
<?php } else { ?>
   <a class="messagelink" href="#" onclick="Piler.restore_message(<?php print $id; ?>);"><?php print $text_restore_to_mailbox; ?></a> |
<?php } } ?>
   <a class="messagelink" href="#" onclick="Piler.view_headers(<?php print $id; ?>);"><?php print $text_view_headers; ?></a>
<?php if($message['has_journal'] == 1 && Registry::get('auditor_user') == 1 && SHOW_ENVELOPE_JOURNAL == 1) { ?>
   | <a class="messagelink" href="#" onclick="Piler.view_journal(<?php print $id; ?>);"><i class="icon-envelope"></i>&nbsp;<?php print $text_view_journal_envelope; ?></a>
<?php } ?>

    <?php if(ENABLE_ON_THE_FLY_VERIFICATION == 0) {
       if ($message['verification'] == 1) { ?><?php print $text_verified_flag; ?> <i class="icon-ok-sign sleek" title="<?php print $text_verified_flag; ?>"></i><?php } else { ?><?php print $text_unverified_flag; ?> <i class="unverified icon-remove-sign sleek" title="<?php print $text_unverified_flag; ?>"></i><?php }
    } ?>

</p>

<strong><?php if($message['subject'] == "" || $message['subject'] == "Subject:") { print "&lt;" . $text_no_subject . "&gt;"; } else { print $message['subject']; } ?></strong><br />
<strong><?php print $message['from']; ?></strong><br />
<strong><?php print $message['to']; ?></strong><br />
<strong><?php print $message['date']; ?></strong><br />

<form class="form-inline" name="view">

<strong><?php print $text_notes; ?></strong>: <input type="text" size="60" id="note" name="note" class="input-medium" value="<?php print preg_replace("/\"/", "&quot;", $message['note']); ?>" /> <input type="button" value="<?php print $text_save; ?>" class="btn btn-inverse" onclick="Piler.add_note_to_message(<?php print $id; ?>, '<?php print $text_saved; ?>'); " />

</form>

<?php foreach($attachments as $a) { ?>
   <span><img src="<?php print ICON_ATTACHMENT; ?>" /><a href="index.php?route=message/attachment&id=<?php print $a['id']; ?>"><?php print $a['name']; ?></a></span>
<?php } ?>

<hr />
<div class="messagecontents"><?php print $message['message']; ?></div>
