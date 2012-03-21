<?php
require_once("tools.php");
gettext_init();
echo '<?xml version="1.0" encoding="utf-8" standalone="no"?>'."\n";
?>
<xsl:stylesheet version="1.0"
	xmlns="http://www.w3.org/2000/svg"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:flo="http://florence.sourceforge.net"
	xmlns:svg="http://www.w3.org/2000/svg"
	xmlns:xlink="http://www.w3.org/1999/xlink">

	<xsl:output version='1.0' method="xml" encoding='UTF-8'/>

	<xsl:output version='1.0' method="xml" encoding='UTF-8'/>
	<xsl:template match="svg:svg">
		<xsl:text disable-output-escaping="yes">&lt;!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd"&gt;
			&lt;?xml-stylesheet type="text/css" href="./florence.css"?&gt;</xsl:text>
		<svg xmlns="http://www.w3.org/2000/svg">
			<xsl:apply-templates/>
		</svg>
	</xsl:template>

	<xsl:template match="svg:svg/svg:svg">
		<svg>
			<xsl:attribute name="id">
				<xsl:value-of select="@id"/>
			</xsl:attribute>

			<xsl:variable name="y" select="@y"/>
			<xsl:variable name="x" select="@x"/>
			<xsl:variable name="position" select="count(//svg:svg/svg:svg[@y=$y and @x&lt;$x])"/>

			<xsl:attribute name="x">
				<xsl:choose>
					<xsl:when test="@x = 0">
						<xsl:value-of select="@x"/>
					</xsl:when>
					<xsl:when test="@y = 0">
						<xsl:value-of select="@x + 40"/>
					</xsl:when>
					<xsl:otherwise>
						<xsl:value-of select="@x + 30 + ( $position * 10 )"/>
					</xsl:otherwise>
				</xsl:choose>
			</xsl:attribute>
			<xsl:attribute name="y">
				<xsl:choose>
					<xsl:when test="@y = 0">
						<xsl:value-of select="@y"/>
					</xsl:when>
					<xsl:otherwise>
						<xsl:value-of select="@y + 40"/>
					</xsl:otherwise>
				</xsl:choose>
			</xsl:attribute>

			<rect style="fill:rgb(220,220,220);stroke-width:2;stroke:rgb(0,0,0)" rx="5" ry="5">
				<xsl:attribute name="width">
					<xsl:choose>
						<xsl:when test="@x = 0">
							<xsl:value-of select="@width + 40"/>
						</xsl:when>
						<xsl:when test="@y = 0">
							<xsl:value-of select="@width + 30"/>
						</xsl:when>
						<xsl:otherwise>
							<xsl:value-of select="@width + 10"/>
						</xsl:otherwise>
					</xsl:choose>
				</xsl:attribute>
				<xsl:attribute name="height">
					<xsl:value-of select="@height + 40"/>
				</xsl:attribute>
			</rect>

			<text font-family="Arial" font-size="22">
				<xsl:attribute name="x">
					<xsl:choose>
						<xsl:when test="@x = 0">-145</xsl:when>
						<xsl:otherwise>5</xsl:otherwise>
					</xsl:choose>
				</xsl:attribute>
				<xsl:attribute name="y">
					<xsl:choose>
						<xsl:when test="@x = 0">-15</xsl:when>
						<xsl:when test="@y = 0">20</xsl:when>
						<xsl:otherwise>
							<xsl:value-of select="35+@height"/>
						</xsl:otherwise>
					</xsl:choose>
				</xsl:attribute>
				<xsl:choose>
					<xsl:when test="@x = 0">
						<xsl:attribute name="transform">rotate(270 50,10)</xsl:attribute>
					</xsl:when>
				</xsl:choose>
	 			<xsl:choose>
					<xsl:when test="@id = 'Florencekeys'"><?php echo _("Florence keys"); ?></xsl:when>
					<xsl:when test="@id = 'Navigationkeys'"><?php echo _("Navigation"); ?></xsl:when>
					<xsl:when test="@id = 'Functionkeys'"><?php echo _("Function keys"); ?></xsl:when>
					<xsl:when test="@id = 'Numerickeys'"><?php echo _("Numeric keys"); ?></xsl:when>
					<xsl:when test="@id = 'Main'"><?php echo _("Main keyboard"); ?></xsl:when>
				</xsl:choose>
			</text>

			<g>
				<xsl:attribute name="transform">
					<xsl:choose>
						<xsl:when test="@id = 'Florencekeys'">translate(35 5) </xsl:when>
						<xsl:when test="@y = 0">translate(5 35)</xsl:when>
						<xsl:otherwise>translate(5 5)</xsl:otherwise>
					</xsl:choose>
				</xsl:attribute>
				<xsl:copy-of select="*"/>
			</g>
		</svg>
	</xsl:template>

	<xsl:template match="svg:svg/svg:title">
		<title>
			<xsl:copy-of select="text()"/>
		</title>
	</xsl:template>

	<xsl:template match="svg:svg/svg:desc">
		<desc>
			<xsl:copy-of select="text()"/>
		</desc>
	</xsl:template>

</xsl:stylesheet>
