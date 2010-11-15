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
<center><a href="http://sourceforge.net/"><img src="http://sflogo.sourceforge.net/sflogo.php?group_id=217749&amp;type=5" alt="<?php echo _("SourceForge.net Logo"); ?>" border="0" height="62" width="210"></a></center><br>
</div>
<hr>
<center>
<?php echo _("Author:"); ?> <a href="mailto:f.agrech@gmail.com">François Agrech</a><br>
<?php echo _("Last update: 5th of November, 2010"); ?>
</center>
</body>
</html>
