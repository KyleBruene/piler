
        <div id="messagelistcontainer" class="boxlistcontent">

   <div id="results">

         <div class="resultrow">
<?php if(count($terms) > 0){ ?>

<?php } else if(count($terms) == 0) { ?>
            <div class="cell3 error"><?php print $text_empty_search_result; ?></div>
<?php } ?>
         </div>

<?php foreach($terms as $term) {
         parse_str($term['term'], $s);
         if(isset($s['search'])) {
?>
         <div class="resultrow">
            <a href="#" onclick="Piler.load_search_results_for_saved_query('<?php print urldecode($term['term']); ?>');"><?php print $s['search']; ?></a></br />
         </div>
<?php } } ?>

       </div>
   </div>


<?php

?>
