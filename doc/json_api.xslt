<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" >
  <xsl:template match="/api">
  <html>
  <ul>
  <xsl:for-each select="classes/class">
    <li>Class: <xsl:value-of select="name" /></li>
    <ul>
      <xsl:for-each select="methods/method">
        <li>Method: <xsl:value-of select="name" /></li>
        <ul>
        <xsl:for-each select="parameter/parameter">
          <li>Parameter: <xsl:value-of select="name" />, <xsl:value-of select="type" />, <xsl:value-of select="required" /></li>
        </xsl:for-each>
        </ul>
      </xsl:for-each>
    </ul>
  </xsl:for-each>
  </ul>
  </html>
  </xsl:template>
</xsl:stylesheet>