<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:flo="http://florence.sourceforge.net"
	xmlns:svg="http://www.w3.org/2000/svg"
	xmlns:xlink="http://www.w3.org/1999/xlink">

	<xsl:output version='1.0' encoding='UTF-8' indent='yes'/>
	<xsl:template match="/">
		<html xmlns="http://www.w3.org/1999/xhtml">
			<head>
				<title><xsl:value-of select="flo:layout/flo:informations/flo:name"/></title>
			</head>
			<body>
				<xsl:choose>
					<xsl:when test="document('keymap.xml')"/>
					<xsl:otherwise>
To get symbols instead of key codes:
<pre>
{
echo '&lt;?xml version="1.0" encoding="UTF-8"?&gt;'
echo "&lt;keymap&gt;"
xmodmap -pke | while read void code void symbol void; do if [ ! "$symbol" = "" ]; then echo "&lt;key code=\"$code\"&gt;$symbol&lt;/key&gt;"; fi; done
echo "&lt;/keymap&gt;"
} >keymap.xml
</pre>
					</xsl:otherwise>
				</xsl:choose>
				<xsl:apply-templates/>
			</body>
		</html>
	</xsl:template>

	<xsl:template match="flo:informations">
		<table xmlns="http://www.w3.org/1999/xhtml">
			<tr><th>Author</th><td><xsl:value-of select="flo:author"/></td></tr>
			<tr><th>Date</th><td><xsl:value-of select="flo:date"/></td></tr>
			<tr><th>Compatible with Florence version</th><td><xsl:value-of select="flo:florence_version"/></td></tr>
		</table>
	</xsl:template>

	<xsl:template match="svg:svg">
		<xsl:copy>
			<xsl:copy-of select="attribute::*"/>
			<xsl:attribute name="id"><xsl:value-of select="../flo:name"/></xsl:attribute>
			<xsl:copy-of select="descendant::*"/>
		</xsl:copy>
	</xsl:template>

	<xsl:template match="flo:style">
		<svg:defs>
			<xsl:for-each select="flo:shape">
				<xsl:apply-templates select="svg:svg"/>
			</xsl:for-each>
		</svg:defs>
	</xsl:template>

	<xsl:template match="flo:keyboard">
		<svg:svg>
			<xsl:attribute name="viewbox">0 0 <xsl:value-of select="flo:width"/><xsl:text> </xsl:text><xsl:value-of select="flo:height"/></xsl:attribute>
			<xsl:attribute name="width"><xsl:value-of select="flo:width * 10"/></xsl:attribute>
			<xsl:attribute name="height"><xsl:value-of select="flo:height * 10"/></xsl:attribute>
			<xsl:for-each select="flo:key">
				<svg:use transform="scale(10)" width="2" height="2" xlink:href="#default">
					<xsl:choose>
						<xsl:when test="string(flo:width)">
							<xsl:attribute name="x"><xsl:value-of select="flo:xpos - ( flo:width div 2 )"/></xsl:attribute>
							<xsl:attribute name="width"><xsl:value-of select="flo:width"/></xsl:attribute>
						</xsl:when>
						<xsl:otherwise>
							<xsl:attribute name="x"><xsl:value-of select="flo:xpos - 1"/></xsl:attribute>
						</xsl:otherwise>
					</xsl:choose>
					<xsl:choose>
						<xsl:when test="string(flo:height)">
							<xsl:attribute name="y"><xsl:value-of select="flo:ypos - ( flo:height div 2 )"/></xsl:attribute>
							<xsl:attribute name="height"><xsl:value-of select="flo:height"/></xsl:attribute>
						</xsl:when>
						<xsl:otherwise>
							<xsl:attribute name="y"><xsl:value-of select="flo:ypos - 1"/></xsl:attribute>
						</xsl:otherwise>
					</xsl:choose>
					<xsl:if test="string(flo:shape)">
						<xsl:attribute name="xlink:href">#<xsl:value-of select="flo:shape"/></xsl:attribute>
					</xsl:if>
				</svg:use>
				<svg:text fill="white" transform="scale(10)" font-size="1" text-anchor="middle" dominant-baseline="central">
					<xsl:attribute name="x"><xsl:value-of select="flo:xpos"/></xsl:attribute>
					<xsl:attribute name="y"><xsl:value-of select="flo:ypos"/></xsl:attribute>
					<xsl:choose>
						<xsl:when test="document('keymap.xml')">
							<xsl:variable name="code"><xsl:value-of select="flo:code"/></xsl:variable>
							<xsl:value-of select="document('keymap.xml')/keymap/key[@code=$code]"/>
						</xsl:when>
						<xsl:otherwise>
							<xsl:value-of select="flo:code"/>
						</xsl:otherwise>
					</xsl:choose>
				</svg:text>
			</xsl:for-each>
		</svg:svg>
	</xsl:template>

	<xsl:template match="flo:extension">
		<table xmlns="http://www.w3.org/1999/xhtml">
			<tr><th>Extension</th><td><xsl:value-of select="flo:name"/></td></tr>
			<tr><th>Position</th><td><xsl:value-of select="flo:placement"/></td></tr>
		</table>
		<xsl:apply-templates select="flo:keyboard"/>
	</xsl:template>

</xsl:stylesheet>

