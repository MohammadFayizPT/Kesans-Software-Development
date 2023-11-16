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
#include <cerrno>


char* get_attribute(xmlNode *node, const char* name){
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

char* get_node_val_by_tag_name(xmlNodePtr node, const char* tagName) {
  char *ret = NULL;
  for (xmlNodePtr child = node->children; child != NULL; child = child->next) {
    if (child->type == XML_ELEMENT_NODE && strcmp((const char*)child->name, tagName) == 0) {
      xmlChar* content = xmlNodeGetContent(child);
      if (content != NULL) {
        ret = strdup((const char*)content);
        xmlFree(content);
        break;
      }
    }
  }
  return ret;
}


xmlNode* get_single_node(xmlNode *parent, const char*  tag){
  xmlNode* ret = NULL; 
  if(!parent){
    return ret;
  }
  for (xmlNode *node = parent->children; node; node = node->next) {
    if (node->type == XML_ELEMENT_NODE && xmlStrcmp(node->name, (const xmlChar *)tag) == 0) {
      ret = node;    
    }
  }
  return ret;
}

bool format_items(xmlNode *requestNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/Items.csv",output_folder);
  fp_out = fopen(out_file,"wb");
  
  //if file is not created
  if(!fp_out){
    printf("Unable to create %s(%s)\n",out_file,strerror(errno));
    return false;
  }
  //header line of csv
  const char *header_line = "\"Name\",\"SKU\",\"HSN/SAC\",\"Active\",\"Type\",\"Sales account\",\"Sales rate\",\"Opening balance\",\"Opening value\",\"Opening rate\",\"Taxability\"";
  
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  
  //retrieving items from xml file
  
  xmlNode *itemNode = NULL;
  for(xmlNode *tallyNode = requestNode->children;tallyNode;tallyNode = tallyNode->next){
    if(tallyNode->type == XML_ELEMENT_NODE && xmlStrcmp(tallyNode->name,(const xmlChar *)"TALLYMESSAGE") == 0){
     for(itemNode = tallyNode->children;itemNode;itemNode = itemNode->next){
       if(itemNode->type == XML_ELEMENT_NODE && xmlStrcmp(itemNode->name,(const xmlChar *)"STOCKITEM") == 0){    
       
        //Name
          char *name = get_attribute(itemNode,"NAME");
          if(name){
          fwrite("\"",1,1,fp_out);
          fwrite(name,1,strlen(name),fp_out);
          fwrite("\",",1,2,fp_out);
          free(name);
          }
          else{
          fwrite(",",1,1,fp_out);
          }
          
         //SKU
         char *sku = get_node_val_by_tag_name(itemNode,"BASEUNITS");
         if(sku){
          fwrite(sku,1,strlen(sku),fp_out);
          fwrite("\",",1,2,fp_out);
         }
         else{
          fwrite(",",1,1,fp_out);
         }
         
         //HSN/SAC
         char *hsn = get_node_val_by_tag_name(itemNode,"HSNCODE");
         if(sku){
          fwrite(hsn,1,strlen(hsn),fp_out);
          fwrite("\",",1,2,fp_out);
         }
         else{
          fwrite(",",1,1,fp_out);
         }
         //Active
         fwrite("Active",1,strlen("Active"),fp_out);
         fwrite("\",",1,2,fp_out);
         
         //Type
         if(get_node_val_by_tag_name(itemNode,"GSTTYPEOFSUPPLY") == "Goods"){
          fwrite("Goods",1,strlen("Goods"),fp_out);
          fwrite("\",",1,2,fp_out);
         }
         else if(get_node_val_by_tag_name(itemNode,"GSTTYPEOFSUPPLY") == "Service"){
          fwrite("Service",1,strlen("Service"),fp_out);
          fwrite("\",",1,2,fp_out);
         }
         else{
          fwrite(",",1,1,fp_out);
         }
         
         //sales account
         fwrite("Sales",1,strlen("Sales"),fp_out);
         fwrite("\",",1,2,fp_out);
         
         //Sales rate
         fwrite(",",1,1,fp_out);
         
         //Opening balance
         char *opening_balance = get_node_val_by_tag_name(itemNode,"OPENINGBALANCE");
         if(opening_balance){
           fwrite("\"",1,1,fp_out);
           fwrite(opening_balance,1,strlen(opening_balance),fp_out);
           fwrite("\",",1,2,fp_out);
           free(opening_balance);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         
         //Opening value
          char *opening_value = get_node_val_by_tag_name(itemNode,"OPENINGVALUE");
          if(opening_value){
           fwrite("\"",1,1,fp_out);
           fwrite(opening_value,1,strlen(opening_value),fp_out);
           fwrite("\",",1,2,fp_out);
           free(opening_value);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         
         //Opening rate
          char *opening_rate = get_node_val_by_tag_name(itemNode,"OPENINGRATE");
          if(opening_rate){
           fwrite("\"",1,1,fp_out);
           fwrite(opening_rate,1,strlen(opening_rate),fp_out);
           fwrite("\",",1,2,fp_out);
           free(opening_rate);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         
         //Taxability
          char *taxability = get_node_val_by_tag_name(itemNode,"TAXABILITY");
          if(taxability){
           fwrite("\"",1,1,fp_out);
           fwrite(taxability,1,strlen(taxability),fp_out);
           fwrite("\",",1,2,fp_out);
           free(taxability);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         
     
       }
     }
    }
  }
  fclose(fp_out);
  
  return true;
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <xml_file>" << std::endl;
        return 1; // Exit with an error code
    }

    const char* xmlFileName = argv[1];

    // Initialize the libxml2 library
    LIBXML_TEST_VERSION;

    // Parse the XML file
    xmlDocPtr doc = xmlReadFile(xmlFileName, NULL, XML_PARSE_RECOVER);

    if (doc == NULL) {
        std::cerr << "Failed to parse the XML file: " << xmlFileName << std::endl;
        return 1; // Exit with an error code
    }

    // Get the root node of the parsed XML document
    xmlNode* root = xmlDocGetRootElement(doc);

    // Call the format_items function
    if (!format_items(root, "output_folder")) {
        std::cerr << "Error formatting items." << std::endl;
    } else {
        std::cout << "Items formatted successfully." << std::endl;
    }

    // Free the XML document and cleanup libxml2
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return 0; // Exit with a success code
}

