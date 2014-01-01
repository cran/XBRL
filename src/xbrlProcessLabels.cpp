// -*- mode: C++; c-indent-level: 2; c-basic-offset: 2; tab-width: 8 -*-
//
// Copyright (C) 2014 Roberto Bertolusso and Marek Kimmel
//
// This file is part of XBRL.
//
// XBRL is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// XBRL is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with XBRL. If not, see <http://www.gnu.org/licenses/>.

#include "XBRL.h"


RcppExport SEXP xbrlProcessLabels(SEXP epaDoc) {
  xmlDocPtr doc = (xmlDocPtr) R_ExternalPtrAddr(epaDoc);

  xmlXPathContextPtr context = xmlXPathNewContext(doc);
  xmlXPathObjectPtr label_res = xmlXPathEvalExpression((xmlChar*) "//*[local-name()='label']", context);
  xmlNodeSetPtr label_nodeset = label_res->nodesetval;
  xmlXPathObjectPtr labelArc_res = xmlXPathEvalExpression((xmlChar*) "//*[local-name()='labelArc']", context);
  xmlNodeSetPtr labelArc_nodeset = labelArc_res->nodesetval;
  xmlXPathObjectPtr loc_res = xmlXPathEvalExpression((xmlChar*) "//*[local-name()='loc']", context);
  xmlNodeSetPtr loc_nodeset = loc_res->nodesetval;
  xmlXPathFreeContext(context);

  int label_nodeset_ln = label_nodeset->nodeNr;

  CharacterVector elementId(label_nodeset_ln);
  CharacterVector lang(label_nodeset_ln);
  CharacterVector labelRole(label_nodeset_ln);
  CharacterVector labelString(label_nodeset_ln);
  CharacterVector href(label_nodeset_ln);

  int j=0, k=0;
  for (int i=0; i < label_nodeset_ln; i++) {
    xmlNodePtr label_node = label_nodeset->nodeTab[i];
    xmlChar *label_label = xmlGetProp(label_node, (xmlChar*) "label");
    // We assume the info appears in an ordered fashion, so labelArc elements
    // prior to the last one visited cannot be a match for this label.
    // The last one visited can be a match because there can be multiple labels
    // (same label identifier with different roles) associated to one labelArc.
    // If the assumption is wrong, at least it is extremely fast wrong.
    for (; j < labelArc_nodeset->nodeNr; j++) {
      xmlNodePtr labelArc_node = labelArc_nodeset->nodeTab[j];
      xmlChar *labelArc_to = xmlGetProp(labelArc_node, (xmlChar*) "to");

      int nomatch = xmlStrcmp(labelArc_to, label_label);
      xmlFree(labelArc_to);
      if (nomatch)
	continue;
      xmlFree(label_label);  // There is a match. Not needed anymore.

      xmlChar *labelArc_from = xmlGetProp(labelArc_node, (xmlChar*) "from");
      // Same reasoning as above, but applied to loc elements.
      for (; k < loc_nodeset->nodeNr; k++) {
	xmlNodePtr loc_node = loc_nodeset->nodeTab[k];
	xmlChar *loc_label = xmlGetProp(loc_node, (xmlChar*) "label");
				
	int nomatch = xmlStrcmp(loc_label, labelArc_from);
	xmlFree(loc_label);
	if (nomatch)
	  continue;
	xmlFree(labelArc_from);  // There is a match. Not needed anymore.
				
	xmlChar *tmp_str;
	if ((tmp_str = xmlGetProp(loc_node, (xmlChar*) "href"))) {
	  href[i] = (char *) tmp_str;
	  string str = (char *) tmp_str;
	  xmlFree(tmp_str);
	  size_t found = str.find("#");
	  if (found != string::npos) {
	    str.replace(0, found+1, "");
	    elementId[i] = str;
	  }
	} else {
	  href[i] = NA_STRING;
	  elementId[i] = NA_STRING;
	}
	if ((tmp_str = xmlGetProp(label_node, (xmlChar*) "lang"))) {
	  lang[i] = (char *) tmp_str;
	  xmlFree(tmp_str);
	} else {
	  lang[i] = NA_STRING;
	}
	if ((tmp_str = xmlGetProp(label_node, (xmlChar*) "role"))) { 
	  labelRole[i] = (char *) tmp_str;
	  xmlFree(tmp_str);
	} else {
	  labelRole[i] = NA_STRING;
	}
	if ((tmp_str = xmlNodeListGetString(doc, label_node->xmlChildrenNode, 1))) {
	  labelString[i] = (char *) tmp_str;
	  xmlFree(tmp_str);
	} else {
	  labelString[i] = NA_STRING;
	}
	break;
      }
      break;
    }
  }
  xmlXPathFreeObject(label_res);
  xmlXPathFreeObject(loc_res);
  xmlXPathFreeObject(labelArc_res);

  if (label_nodeset_ln == 0)
    return R_NilValue;

  return DataFrame::create(Named("elementId")=elementId,
			   Named("lang")=lang,
			   Named("labelRole")=labelRole,
			   Named("labelString")=labelString,
			   Named("href")=href);
}