<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0"
	xmlns="http://www.w3.org/2000/svg"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:flo="http://florence.sourceforge.net"
	xmlns:svg="http://www.w3.org/2000/svg"
	xmlns:xlink="http://www.w3.org/1999/xlink">

	<xsl:template name="coords">
		<xsl:param name="size" select="1"/>
		<xsl:choose>
			<xsl:when test="string(flo:width)">
				<xsl:attribute name="x"><xsl:value-of select="$size * ( flo:xpos - ( flo:width div 2 ) )"/></xsl:attribute>
				<xsl:attribute name="width"><xsl:value-of select="$size * flo:width"/></xsl:attribute>
			</xsl:when>
			<xsl:otherwise>
				<xsl:attribute name="x"><xsl:value-of select="$size * ( flo:xpos - 1 )"/></xsl:attribute>
			</xsl:otherwise>
		</xsl:choose>
		<xsl:choose>
			<xsl:when test="string(flo:height)">
				<xsl:attribute name="y"><xsl:value-of select="$size * ( flo:ypos - ( flo:height div 2 ) )"/></xsl:attribute>
				<xsl:attribute name="height"><xsl:value-of select="$size * flo:height"/></xsl:attribute>
			</xsl:when>
			<xsl:otherwise>
				<xsl:attribute name="y"><xsl:value-of select="$size * ( flo:ypos - 1 )"/></xsl:attribute>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template name="text">
		<xsl:param name="text" select="'?'"/>
		<xsl:param name="size" select="18"/>
		<xsl:param name="coords" select="1"/>
		<text class="symbol" font-family="Arial">
			<xsl:attribute name="font-size"><xsl:value-of select="$size"/></xsl:attribute>
			<xsl:attribute name="text-anchor">middle</xsl:attribute>
			<xsl:if test="$coords = 1">
				<xsl:attribute name="x"><xsl:value-of select="flo:xpos * 20"/></xsl:attribute>
				<xsl:attribute name="y"><xsl:value-of select="( flo:ypos * 20 ) + 10"/></xsl:attribute>
			</xsl:if>
			<xsl:value-of select="$text"/>
		</text>
	</xsl:template>

</xsl:stylesheet>

