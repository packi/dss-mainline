<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" >
  <xsl:template match="/api">
  <html>
  <head>
	<link rel="stylesheet" type="text/css" href="doc.css"/>
  </head>
    <body>
      <div class="class_box">
        <h1>Classes</h1>      
        <xsl:for-each select="classes/class">
          <a>
            <xsl:attribute name="href">
              <xsl:value-of select="concat('#', name)" />
            </xsl:attribute>
            <xsl:value-of select="name"/>
          </a>
          <br/>
        </xsl:for-each>
      </div>
      <div class="documentation_box">
      <xsl:for-each select="classes/class">
        <div class="single_class_box">
          <xsl:variable name="className" select="name"/>
          <h2>
            <a>
              <xsl:attribute name="id">
                <xsl:value-of select="$className" />
              </xsl:attribute>
              <xsl:value-of select="name"/>
            </a>
          </h2>
          <xsl:variable name="methodCount" select="count(methods/method)" />
          <xsl:if test="$methodCount > 0">
            <div>
              <h3>Methods</h3>
              <dl>
              <xsl:for-each select="methods/method">
                <dt>
                  <a>
                    <xsl:attribute name="href">
                      <xsl:value-of select="concat('#', $className, '_', name)" />
                    </xsl:attribute>
                    <xsl:value-of select="name"/>
                  </a>
                </dt>
                <dd>
                  <xsl:value-of select="documentation/short" />
                </dd>
              </xsl:for-each>
              </dl>
              <xsl:for-each select="methods/method">
                <hr/>
                <div>
                  <h4>
                    <a>
                      <xsl:attribute name="id">
                        <xsl:value-of select="concat($className, '_', name)" />
                      </xsl:attribute>
                      <xsl:value-of select="name"/>
                    </a>
                  </h4>
                  <xsl:value-of select="concat(documentation/short, ' ', documentation/long)" />
                  <xsl:variable name="paramCount" select="count(parameter/parameter)" />
                  <xsl:choose>
                    <xsl:when test="$paramCount > 0">
                      <h5>Parameter</h5>
                      <table class="parameter">
                        <th class="parameter">Name</th><th class="parameter">Type</th><th class="parameter">Required</th>
                        <xsl:for-each select="parameter/parameter">
                          <tr><td><xsl:value-of select="name" /></td><td><xsl:value-of select="type" /></td><td><xsl:value-of select="required" /></td></tr>
                        </xsl:for-each>
                      </table>
                    </xsl:when>
                    <xsl:otherwise>
                      <p>No parameter</p>
                    </xsl:otherwise>
                  </xsl:choose>
                </div>
                <p/>
              </xsl:for-each>
            </div>
          </xsl:if>
          </div>
        </xsl:for-each>
      </div>
    </body>
  </html>
  </xsl:template>
</xsl:stylesheet>