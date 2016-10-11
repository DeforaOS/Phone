<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="text"/>
	<xsl:template match="/">
		<xsl:apply-templates select="serviceproviders"/>
	</xsl:template>

	<!-- country -->
	<xsl:template match="country">
		<xsl:apply-templates select="provider"/>
	</xsl:template>

	<!-- provider -->
	<xsl:template match="provider">
		<!-- FIXME there may be more than one <apn> -->
		<xsl:text>[</xsl:text><xsl:value-of select="name"/><xsl:text>]
</xsl:text>
		<xsl:text>apn=</xsl:text><xsl:value-of select="gsm/apn/@value"/><xsl:text>
</xsl:text>
		<xsl:if test="string-length(gsm/apn/username) &gt; 0"><xsl:text>username=</xsl:text><xsl:value-of select="gsm/apn/username"/><xsl:text>
</xsl:text></xsl:if>
		<xsl:if test="string-length(gsm/apn/password) &gt; 0"><xsl:text>password=</xsl:text><xsl:value-of select="gsm/apn/password"/><xsl:text>
</xsl:text></xsl:if>
		<xsl:text>
</xsl:text>
	</xsl:template>

	<!-- serviceproviders -->
	<xsl:template match="serviceproviders">
		<xsl:for-each select="country">
			<xsl:apply-templates select="provider"/>
		</xsl:for-each>
	</xsl:template>
</xsl:stylesheet>
