<?php
function gettext_init() {
setlocale(LC_ALL, getenv('LC_ALL'));
bindtextdomain("messages", "locale");
textdomain("messages");
}

function adsense() {
?>
<div class="ads">
<script type="text/javascript"><!--
google_ad_client = "pub-2152392459551245";
/* 160x600, date de création 08/04/10 */
google_ad_slot = "3882482999";
google_ad_width = 160;
google_ad_height = 600;
//-->
</script>
<script type="text/javascript"
src="http://pagead2.googlesyndication.com/pagead/show_ads.js">
</script>
</div>
<?php
}

function main_menu($location) {
if ($location == "Home") { $path=""; } else { $path="../"; }
?>
<div class="mainmenu">
<div class="maintitle">
<div style="float: left; margin: -10px 10px 10px -10px;">
<object type="image/svg+xml" data="<?php echo $path; ?>images/florence.svg"></object>
</div>
<div style="float: right;">
<h1><?php echo _("Florence Virtual Keyboard"); ?></h1>
</div>
</div>
<div class="language">
<?php if (substr(getenv('LC_ALL'),0,2)=='fr') {
	echo "<a href=\"${path}english.html\">English</a><p> | </p><em>Français</em><p> | </p><a href=\"${path}russian.html\">Pусский</a>";
} else if (substr(getenv('LC_ALL'),0,2)=='ru') {
	echo "<a href=\"${path}english.html\">English</a><p> | </p><a href=\"${path}francais.html\">Français</a><p> | </p><em>Pусский</em>";
} else {
	echo "<em>English</em><p> | </p><a href=\"${path}francais.html\">Français</a><p> | </p><a href=\"${path}russian.html\">Pусский</a>";
}
?>
</div>
<div class="menu">
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
</div>
</div>
<hr/>
<?php
}

function piwik () {
?>
<!-- Piwik -->
<script type="text/javascript">
var pkBaseURL = (("https:" == document.location.protocol) ? "https://sourceforge.net/apps/piwik/florence/" : "http://sourceforge.net/apps/piwik/florence/");
document.write(unescape("%3Cscript src='" + pkBaseURL + "piwik.js' type='text/javascript'%3E%3C/script%3E"));
</script><script type="text/javascript">
piwik_action_name = '';
piwik_idsite = 1;
piwik_url = pkBaseURL + "piwik.php";
piwik_log(piwik_action_name, piwik_idsite, piwik_url);
</script>
<object><noscript><p><img src="http://sourceforge.net/apps/piwik/florence/piwik.php?idsite=1" alt="piwik"/></p></noscript></object>
<!-- End Piwik Tag -->
<?php
}
?>
