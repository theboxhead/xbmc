/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Fanart.h"
#include "utils/XBMCTinyXML.h"
#include "URIUtils.h"
#include "StringUtils.h"

const unsigned int CFanart::max_fanart_colors=3;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CFanart Functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CFanart::CFanart()
{
}

void CFanart::Pack()
{
  // Take our data and pack it into the m_xml string
  m_xml.Empty();
  TiXmlElement fanart("fanart");
  for (std::vector<SFanartData>::const_iterator it = m_fanart.begin(); it != m_fanart.end(); ++it)
  {
    TiXmlElement thumb("thumb");
    thumb.SetAttribute("dim", it->strResolution.c_str());
    thumb.SetAttribute("colors", it->strColors.c_str());
    thumb.SetAttribute("preview", it->strPreview.c_str());
    TiXmlText text(it->strImage);
    thumb.InsertEndChild(text);
    fanart.InsertEndChild(thumb);
  }
  m_xml << fanart;
}

bool CFanart::Unpack()
{
  CXBMCTinyXML doc;
  doc.Parse(m_xml.c_str());

  m_fanart.clear();

  TiXmlElement *fanart = doc.FirstChildElement("fanart");
  while (fanart)
  {
    CStdString url = fanart->Attribute("url");
    TiXmlElement *fanartThumb = fanart->FirstChildElement("thumb");
    while (fanartThumb)
    {
      SFanartData data;
      if (url.empty())
      {
        data.strImage = fanartThumb->GetText();
        if (fanartThumb->Attribute("preview"))
          data.strPreview = fanartThumb->Attribute("preview");
      }
      else
      {
        data.strImage = URIUtils::AddFileToFolder(url, fanartThumb->GetText());
        if (fanartThumb->Attribute("preview"))
          data.strPreview = URIUtils::AddFileToFolder(url, fanartThumb->Attribute("preview"));
      }
      data.strResolution = fanartThumb->Attribute("dim");
      ParseColors(fanartThumb->Attribute("colors"), data.strColors);
      m_fanart.push_back(data);
      fanartThumb = fanartThumb->NextSiblingElement("thumb");
    }
    fanart = fanart->NextSiblingElement("fanart");
  }
  return true;
}

CStdString CFanart::GetImageURL(unsigned int index) const
{
  if (index >= m_fanart.size())
    return "";

  return m_fanart[index].strImage;
}

CStdString CFanart::GetPreviewURL(unsigned int index) const
{
  if (index >= m_fanart.size())
    return "";

  return m_fanart[index].strPreview.empty() ? m_fanart[index].strImage : m_fanart[index].strPreview;
}

const CStdString CFanart::GetColor(unsigned int index) const
{
  if (index >= max_fanart_colors || m_fanart.size() == 0)
    return "FFFFFFFF";

  // format is AARRGGBB,AARRGGBB etc.
  return m_fanart[0].strColors.Mid(index*9, 8);
}

bool CFanart::SetPrimaryFanart(unsigned int index)
{
  if (index >= m_fanart.size())
    return false;

  std::iter_swap(m_fanart.begin()+index, m_fanart.begin());

  // repack our data
  Pack();

  return true;
}

unsigned int CFanart::GetNumFanarts()
{
  return m_fanart.size();
}

bool CFanart::ParseColors(const CStdString &colorsIn, CStdString &colorsOut)
{
  // Formats:
  // 0: XBMC ARGB Hexadecimal string comma seperated "FFFFFFFF,DDDDDDDD,AAAAAAAA"
  // 1: The TVDB RGB Int Triplets, pipe seperate with leading/trailing pipes "|68,69,59|69,70,58|78,78,68|"

  // Essentially we read the colors in using the proper format, and store them in our own fixed temporary format (3 DWORDS), and then
  // write them back in in the specified format.

  if (colorsIn.IsEmpty())
    return false;

  // check for the TVDB RGB triplets "|68,69,59|69,70,58|78,78,68|"
  if (colorsIn[0] == '|')
  { // need conversion
    colorsOut.Empty();
    CStdStringArray strColors;
    StringUtils::SplitString(colorsIn, "|", strColors);
    for (int i = 0; i < std::min((int)strColors.size()-1, (int)max_fanart_colors); i++)
    { // split up each color
      CStdStringArray strTriplets;
      StringUtils::SplitString(strColors[i+1], ",", strTriplets);
      if (strTriplets.size() == 3)
      { // convert
        if (colorsOut.size())
          colorsOut += ",";
        colorsOut.AppendFormat("FF%2x%2x%2x", atol(strTriplets[0].c_str()), atol(strTriplets[1].c_str()), atol(strTriplets[2].c_str()));
      }
    }
  }
  else
  { // assume is our format
    colorsOut = colorsIn;
  }
  return true;
}
