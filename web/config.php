<?php
require_once("tools.php");
gettext_init();
echo "<?xml version='1.0' encoding='utf-8'?>\n";
?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:param name="use.id.as.filename" select="'1'"/>
	<xsl:param name="admon.graphics" select="'1'"/>
	<xsl:param name="admon.graphics.path"/>
	<xsl:param name="chunk.section.depth" select="'1'"/>
	<xsl:param name="html.stylesheet" select="'../style.css'"/>
	<xsl:template name="user.footer.content">
		<div align="right"><small><xsl:apply-templates select="//copyright[1]" mode="titlepage.mode"/></small></div>
	</xsl:template>
	<xsl:template name="user.header.navigation">
		<?php main_menu("Documentation"); ?>
	</xsl:template>
	<xsl:template name="user.head.content">
		<xsl:comment> Piwik </xsl:comment>
			<?php piwik(); ?>
		<xsl:comment> End Piwik Tag </xsl:comment>
	</xsl:template>
</xsl:stylesheet>
