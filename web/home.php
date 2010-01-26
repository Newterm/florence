<?php
require_once("tools.php");
gettext_init();
echo '<?xml version="1.0" encoding="ISO-8859-15" standalone="no"?>'."\n";
?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta content="text/html; charset=ISO-8859-15" http-equiv="content-type"><title><?php echo _("Florence Virtual Keyboard"); ?></title></head>
<link rel="stylesheet" type="text/css" href="style.css">
<body>
<?php main_menu("Home"); ?>
<div class="corps">
<?php echo _("Florence is an extensible scalable virtual keyboard for GNOME. You need it if you can't use a real hardware keyboard, for example because you are disabled, your keyboard is broken or because you use a tablet PC, but you must be able to use a pointing device (as a mouse, a trackball, or a touchscreen); If you can't use a pointing device, there is gok, which can be used with just simple switches."); ?>
   <br><br>
<?php echo _("Florence stays out of your way when you don't need it: it appears on the screen only when you need it. A Timer-based auto-click functionality is available to help disabled people having difficulties to click."); ?>
   <br><br>
<?php echo _("Florence is primarily intended to be used with the GNOME desktop, although it can be used on any desktop environment."); ?>  
</div>
	<center><img src="images/<?php echo _("florence-gb"); ?>.png" alt="<?php echo _("Florence Virtual Keyboard"); ?>"/></center>
<br>
<center><a href="http://sourceforge.net/"><img src="http://sflogo.sourceforge.net/sflogo.php?group_id=217749&amp;type=5" alt="<?php echo _("SourceForge.net Logo"); ?>" border="0" height="62" width="210"></a></center><br>
<hr>
<center>
<?php echo _("Author:"); ?> <a href="mailto:f.agrech@gmail.com">François Agrech</a><br>
<?php echo _("Last update: 25th of January, 2010"); ?>
</center>
</body>
</html>
