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
		<xsl:if test="count(gsm/*/ussd) &gt; 0">
			<xsl:text>[</xsl:text><xsl:value-of select="name"/><xsl:text>]
</xsl:text>
			<xsl:if test="string-length(gsm/balance-check/ussd) &gt; 0"><xsl:text>balance-check=</xsl:text><xsl:value-of select="gsm/balance-check/ussd"/><xsl:text>
</xsl:text></xsl:if>
			<xsl:if test="string-length(gsm/balance-top-up/ussd) &gt; 0"><xsl:text>balance-top-up=</xsl:text><xsl:value-of select="gsm/balance-top-up/ussd"/><xsl:text>
</xsl:text></xsl:if>
			<xsl:if test="string-length(gsm/msisdn-query/ussd) &gt; 0"><xsl:text>msisdn-query=</xsl:text><xsl:value-of select="gsm/msisdn-query/ussd"/><xsl:text>
</xsl:text></xsl:if>
		<xsl:text>
</xsl:text></xsl:if>
	</xsl:template>

	<!-- serviceproviders -->
	<xsl:template match="serviceproviders">
		<xsl:for-each select="country">
			<xsl:apply-templates select="provider"/>
		</xsl:for-each>
	</xsl:template>
</xsl:stylesheet>
