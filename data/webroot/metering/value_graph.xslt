<?xml version="1.0" encoding="utf-8"?>
<xsl:transform xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" >
<xsl:output method="xml" indent="yes" media-type="image/svg"/>
  <xsl:template match="/values">
    <svg width="400px" height="250px" xmlns="http://www.w3.org/2000/svg">
      <g id="bar" transform="translate(0,250)">
        <polyline style="fill:white;stroke:red;stroke-width:1">
          <xsl:attribute name="points">
            <xsl:for-each select="value">
              <xsl:if test="not(position() = 1)"><xsl:text> </xsl:text></xsl:if>
              <!--<xsl:variable name="val" select="@value"/>-->
              <xsl:value-of select="concat(position() * 1, ',', -@value div 500)"/>
            </xsl:for-each>
          </xsl:attribute>
        </polyline>
      </g>
    </svg>
  </xsl:template>
</xsl:transform>