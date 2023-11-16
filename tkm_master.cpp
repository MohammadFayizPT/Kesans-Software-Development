#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <iostream>


char* getAttribute(xmlNode *node, const char* name){
  char *ret = NULL;
  xmlAttr *attribute = NULL;
  for (attribute = node->properties; attribute; attribute = attribute->next) {
    if (xmlStrcmp(attribute->name, (const xmlChar *) name) == 0) {
      xmlChar *attributeValue = xmlGetProp(node, attribute->name);
      if (attributeValue != NULL) {
        ret = strdup((const char*)attributeValue);
        xmlFree(attributeValue);
      }
    }
  }
  return ret;
}


bool format_customers(xmlNode *requestNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/Customers.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"*Display Name\",\"Company Name\",\"*Type\",\"Salutation\","
                  "\"First Name\",\"Last Name\",\"Source Of Supply\",\"Place Of Supply\","
                  "\"Currency Code\",\"EmailID\",\"Phone\",\"MobilePhone\",\"Notes\",\"Website\","
                  "\"GST Treatment\",\"GST Identification Number (GSTIN)\",\"PAN Number\","
                  "\"*Payment Terms\",\"Billing Attention\",\"Billing Address\",\"Billing Street2\","
                  "\"Billing City\",\"Billing State\",\"Billing Country\",\"Billing Code\","
                  "\"Billing Fax\",\"Shipping Attention\",\"Shipping Address\","
                  "\"Shipping Street2\",\"Shipping City\",\"Shipping State\","
                  "\"Shipping Country\",\"Shipping Code\",\"Shipping Fax\"";
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  xmlNode *ledgerNode = NULL;
  
  for (xmlNode *tallyNode = requestNode->children; tallyNode; tallyNode = tallyNode->next) {
    if (tallyNode->type == XML_ELEMENT_NODE &&  
      xmlStrcmp(tallyNode->name, (const xmlChar *)"TALLYMESSAGE") == 0) {
      for (ledgerNode = tallyNode->children; ledgerNode; ledgerNode = ledgerNode->next) {
        if (ledgerNode->type == XML_ELEMENT_NODE && xmlStrcmp(ledgerNode->name, (const xmlChar *)"LEDGER") == 0) {
          for (xmlNode *childNode = ledgerNode->children; childNode; childNode = childNode->next) {
            if (childNode->type == XML_ELEMENT_NODE && xmlStrcmp(childNode->name, (const xmlChar *)"PARENT") == 0) {
              const char *parentContent = (const char *)xmlNodeGetContent(childNode);
              if (parentContent && strcmp(parentContent, "Sundry Debtors") == 0) {
                char *name = getAttribute(ledgerNode, "NAME");
                if(name){
                  printf("Ledger with name ='%s'\n", name);
                }
                free(name);
              }
            }
          }
        }
      }
    }
  }

  fclose(fp_out);
  return true; 
}



int main() {
    // Replace 'your_xml_document.xml' with the actual XML file path
    bool status;
    xmlDocPtr doc = xmlReadFile("TKM Master.xml", NULL, XML_PARSE_DTDVALID);

    if (doc == NULL) {
        std::cerr << "Failed to parse XML document" << std::endl;
        return -1;
    }

    if (!format_customers(xmlDocGetRootElement(doc), "output.csv")) {
        std::cerr << "Conversion to CSV failed" << std::endl;
        xmlFreeDoc(doc);
        return -1;
    }
    
     status = format_customers(xmlDocGetRootElement(doc), "output.csv");
     
     if(status)
      {
        printf("\nConversion is successfull");
;      }
        
    xmlFreeDoc(doc);
    return 0;
}

