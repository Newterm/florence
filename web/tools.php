<?php
function gettext_init() {
setlocale(LC_ALL, getenv('LC_ALL'));
bindtextdomain("messages", "locale");
textdomain("messages");
}

function main_menu($location) {
if ($location == "Home") { $path=""; } else { $path="../"; }
?>
<div class="mainmenu">
<table width="100%">
<tr>
<td valign="center">
<object type="image/svg+xml" data="<?php echo $path; ?>images/florence.svg" width="32" height="32"></object>
<h1><?php echo _("Florence Virtual Keyboard"); ?></h1>
</td>
<td align="right">
<a href="<?php echo $path._("francais"); ?>.html"><?php echo _("En français"); ?></a>
</td>
</tr>
<tr>
<td align="right" valign="bottom" colspan="2">
<ul class="menu">
<?php if ($location == "Home") { ?>
<li class="menu selected"><?php echo _("Home"); ?></li>
<?php } else { ?>
<li class="menu"><a href="../<?php echo _("english");?>.html"><?php echo _("Home"); ?></a></li>
<?php } ?>
<li class="menu"><a href="http://sourceforge.net/project/screenshots.php?group_id=217749"><?php echo _("Screenshots"); ?></a></li>
<?php if ($location == "Documentation") { ?>
<li class="menu selected"><?php echo _("Documentation"); ?></li>
<?php } else { ?>
<li class="menu"><a href="<?php echo _("english"); ?>/index.html"><?php echo _("Documentation"); ?></a></li>
<?php } ?>
<li class="menu"><a href="http://sourceforge.net/project/platformdownload.php?group_id=217749"><?php echo _("Download"); ?></a></li>
<li class="menu"><a href="http://sourceforge.net/news/?group_id=217749"><?php echo _("News"); ?></a></li>
</ul>
</td>
</tr>
</table>
</div>
<hr/>
<?php
}
?>
