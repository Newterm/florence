<?php
	$lang=$_SERVER['HTTP_ACCEPT_LANGUAGE'];
	if (substr($lang,0,2)=='fr')
	{
		header("location: francais.html");
	} else {
		header("location: english.html");
	}
?>
