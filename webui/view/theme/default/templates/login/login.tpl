
<?php if(!isset($_SESSION['username'])){ ?>
<form name="login" action="login.php" method="post">
   <input type="hidden" name="relocation" value="<?php if(isset($_GET['route']) && !preg_match("/^login/", $_GET['route']) ) { if(isset($_SERVER['REDIRECT_URL'])) { print $_SERVER['REDIRECT_URL']; } else { print $_SERVER['QUERY_STRING']; } } ?>" />


   <div id="search">

      <div id="search1">

<?php if(ENABLE_GOOGLE_LOGIN == 1) { ?>
         <div class="row">
            <div class="logincell"><a href="<?php print $auth_url; ?>"><?php print $text_login_via_google; ?></a></div>
         </div>
<?php } else { ?>
         <div class="row">
            <div class="logincell"><?php print $text_email; ?>:</div>
            <div class="logincell"><input type="text" class="textregular" name="username" /></div>
         </div>

         <div class="row">
            <div class="logincell"><?php print $text_password; ?>:</div>
            <div class="logincell"><input type="password" class="textregular" name="password" /></div>
         </div>

         <div class="row">
            <div class="logincell">&nbsp;</div>
            <div class="logincell"><input type="submit" value="<?php print $text_submit; ?>" /> <input type="reset" value="<?php print $text_cancel; ?>" /></div>
         </div>
<?php } ?>

      </div>

   </div>

</form>

<?php if(isset($x)){ ?><p class="loginfailed"><?php print $x; ?></p><?php } ?>

<?php } ?>

