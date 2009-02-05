<?xml version="1.0" encoding="utf-8"?>
<xsl:transform xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" >
  <xsl:template match="/values">
    <svg width="600px" height="250px" xmlns="http://www.w3.org/2000/svg">
      <g id="bar" transform="translate(0,50)">
        <polyline style="fill:white;stroke:red;stroke-width:2">
          <xsl:attribute name="points">
            <xsl:for-each select="value">
              <xsl:if test="not(position() = 1)"><xsl:text> </xsl:text></xsl:if>
              <!--<xsl:variable name="val" select="@value"/>-->
              <xsl:value-of select="concat(position() * 20, ',', @value*2)"/>
            </xsl:for-each>
          </xsl:attribute>
        </polyline>
      </g>
    </svg>
  </xsl:template>
</xsl:transform>