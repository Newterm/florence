<?php
require_once("tools.php");
gettext_init();
echo '<?xml version="1.0" encoding="utf-8" standalone="no"?>'."\n";
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta name="description" content="<?php echo _("Florence is an extensible scalable virtual keyboard for GNOME."); ?>"/>
<meta name="keywords" content="<?php echo _("virtual keyboard, onscreen keyboard, software keyboard, gnome keyboard, florence keyboard, linux keyboard"); ?>"/>
<meta content="text/html; charset=utf-8" http-equiv="content-type"/><title><?php echo _("Florence Virtual Keyboard"); ?></title>
<link rel="stylesheet" type="text/css" href="style.css"/>
<?php piwik(); ?>
</head>
<body>
<?php main_menu("Home"); adsense(); ?>
<br>
<center>
<?php echo _(' <form action="https://www.paypal.com/cgi-bin/webscr" method="post"> <input type="hidden" name="cmd" value="_s-xclick"> <input type="hidden" name="encrypted" value="-----BEGIN PKCS7-----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-----END PKCS7----- "> <input type="image" src="https://www.paypal.com/en_US/i/btn/btn_donateCC_LG.gif" border="0" name="submit" alt="PayPal - The safer, easier way to pay online!"> <img alt="" border="0" src="https://www.paypal.com/fr_FR/i/scr/pixel.gif" width="1" height="1"> </form> '); ?>
</center>
<h2><?php echo _("About"); ?></h2>
<div class="corps">
<?php echo _("Florence is an extensible scalable virtual keyboard for GNOME. You need it if you can't use a real hardware keyboard, for example because you are disabled, your keyboard is broken or because you use a tablet PC, but you must be able to use a pointing device (as a mouse, a trackball, a touchscreen or <a href=\"http://www.inference.phy.cam.ac.uk/opengazer\">opengazer</a>); If you can't use a pointing device, there is gok, which can be used with just simple switches."); ?>
   <br/><br/>
<?php echo _("Florence stays out of your way when you don't need it: it appears on the screen only when you need it. A timer-based auto-click input method is available to help disabled people having difficulties to click. You may also check the new efficient ramble method."); ?>
   <br/><br/>
<?php echo _("Florence is primarily intended to be used with the GNOME desktop, although it can be used on any desktop environment."); ?>  
</div>
<br/>
	<center><img src="images/<?php echo _("florence-us"); ?>.png" alt="<?php echo _("Florence Virtual Keyboard"); ?>"/></center>
<br/>
<div class="corps">
<?php echo _("You can browse the full documentation for more informations."); ?>
</div>
<div class="corps">
<h2><?php echo _("Alternatives"); ?></h2>
<?php
echo _("If you are looking for an input method for your GNOME desktop, Florence may suit your needs or you may check the <a href=\"english/alternatives.html\">alternatives</a> section of the documentation."); 
?>
</div>
<h2><?php echo _("License"); ?></h2>
<div class="corps">
<?php
echo _("Florence is free software. You are free do use it for any purpose, to modify it or to hire someone to modify it for you. You are also free to distribute it or a modified version, provided you do not add any other restriction. If you wish to distribute this software, please make sure you understand all the implications of the license. The full license is available at <a href=\"http://www.gnu.org/licenses/old-licenses/gpl-2.0.html\">http://www.gnu.org/licenses/old-licenses/gpl-2.0.html</a>.");
?>
</div>
<h2><?php echo _("Credits"); ?></h2>
<?php
echo _("Florence is integrated in the <a href=\"http://code.google.com/p/ardesia/\">Ardesia project</a>");
?>
<br/>
<?php
echo _("Ardesia is the free digital sketchpad software that help you to make colored free-hand annotations with digital ink everywhere, record it and share on the network. It is easy to use and impressively fast and reactive. You can draw upon the desktop or import an image and annotate it and redistribute your work to the world. Let's create quick sketch and artwork.");
?>
<br/>
<?php
echo _("Pietro Pilolli &lt;alpha@paranoici.org&gt; of the Ardesia project has contributed back a significant amount of code to the Florence project, including the original ramble input method implementation, bug fixes and other patches that have yet to be merged.");
?>
   <br/><br/>
<?php
echo _("Arnaud Andoval &lt;arnaudsandoval@gmail.com&gt;, Stéphane Ancelot &lt;sancelot@free.fr&gt; and Laurent Bessard &lt;laurent.bessard@gmail.com&gt; from Numalliance have contributed a lot of code, including the gnome panel applet implementation, bug fixes and more patches that have yet to be merged.");
?>
   <br/><br/>
<?php
echo _("Sourceforge.net is hosting the git repository, this web site, the bug tracker, the forum, the releases and other useful services at no charge.");
?>
<br/>
<center><a href="http://sourceforge.net/"><img src="http://sflogo.sourceforge.net/sflogo.php?group_id=217749&amp;type=5" alt="<?php echo _("SourceForge.net Logo"); ?>" border="0" height="62" width="210"></a>
</center>
<br>
</div>
<hr>
<center>
<?php echo _("Author:"); ?> <a href="mailto:f.agrech@gmail.com">François Agrech</a><br>
<?php echo _("Last update: 5th of November, 2010"); ?>
</center>
</body>
</html>
