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


struct State{
  char name[40];
  int id;
  char code[5];
};

State states[] = {
    { "Andaman and Nicobar Islands",  35,  "AN"},
    { "Andhra Pradesh",  37,  "AD"},
    { "Arunachal Pradesh",  12,  "AR"},
    { "Assam",  18,  "AS"},
    { "Bihar",  10,  "BR"},
    { "Chandigarh",  4,  "CH"},
    { "Chattisgarh",  22,  "CG"},
    { "Dadra and Nagar Haveli",  26,  "DN"},
    { "Daman and Diu",  25,  "DD"},
    { "Delhi",  7,  "DL"},
    { "Goa",  30,  "GA"},
    { "Gujarat",  24,  "GJ"},
    { "Haryana",  6,  "HR"},
    { "Himachal Pradesh",  2,  "HP"},
    { "Jammu and Kashmir",  1,  "JK"},
    { "Jharkhand",  20,  "JH"},
    { "Karnataka",  29,  "KA"},
    { "Kerala",  32,  "KL"},
    { "Lakshadweep Islands",  31,  "LD"},
    { "Ladakh",  38,  "LA"},
    { "Madhya Pradesh",  23,  "MP"},
    { "Maharashtra",  27,  "MH"},
    { "Manipur",  14,  "MN"},
    { "Meghalaya",  17,  "ML"},
    { "Mizoram",  15,  "MZ"},
    { "Nagaland",  13,  "NL"},
    { "Odisha",  21,  "OD"},
    { "Pondicherry",  34,  "PY"},
    { "Punjab",  3,  "PB"},
    { "Rajasthan",  8,  "RJ"},
    { "Sikkim",  11,  "SK"},
    { "Tamil Nadu",  33,  "TN"},
    { "Telangana",  36,  "TS"},
    { "Tripura",  16,  "TR"},
    { "Uttar Pradesh",  9,  "UP"},
    { "Uttarakhand",  5,  "UK"},
    { "West Bengal",  19,  "WB"},
    { "Other Territory",  97,  "OT"}
};

const char* get_state_code(const char* name){
  const char *ret = NULL;
  static int array_size = sizeof(states)/sizeof(State);
  for(int index = 0; index < array_size; index++){
    if(strcasecmp(states[index].name, name) == 0){
      ret = states[index].code;
      break;
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

bool format_invoices(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/Invoices.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"Invoice Number\",\"Invoice Date\",\"Invoice Status\",\"Invoice Type\",\"Reference\",\"Due Date\","
                            "\"Customer Name\",\"Company Name\",\"GST Treatment\",\"GST Identification Number\"," 
                            "\"TCS Tax Name\",\"TCS Tax Amount\",\"TDS Name\", \"TDS Amount\", \"Payment Terms\", \"Place of Supply\","
                            "\"Is Inclusive Tax\",\"Discount Type\",\"Entity Type\",\"Item Type\",\"Item Name\",\"HSN/SAC\","
                            "\"Description\",\"Quantity\",\"Item Price\",\"Item Tax\",\"Item Tax Exemption Reason\"," 
                            "\"Is Discount Before Tax\",\"Discount is percentage\",\"Discount \",\"Total\"," 
                            "\"Balance\", \"Shipping Charge\", \"Shipping Charge Tax\", \"Adjustment\",\"Round off\",\"Write Off\", \"Template Name\"," 
                            "\"Exchange Rate\",\"Currency Code\",\"Notes\",\"Terms & Conditions\",\"Billing Address\",\"Billing City\",\"Billing State\"," 
                            "\"Billing Country\",\"Billing Code\",\"Billing Fax\",\"Shipping Address\",\"Shipping City\"," 
                            "\"Shipping State\",\"Shipping Country\",\"Shipping Code\",\"Shipping Fax\""; 

  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  
  xmlNode *voucherNode = NULL;
  for(xmlNode *tallyNode = requestNode->children;tallyNode;tallyNode = tallyNode->next){
    if(tallyNode->type == XML_ELEMENT_NODE && xmlStrcmp(tallyNode->name,(const xmlChar *)"TALLYMESSAGE") == 0){
     for(voucherNode = tallyNode->children;voucherNode;voucherNode = voucherNode->next){
       if(voucherNode->type == XML_ELEMENT_NODE && xmlStrcmp(voucherNode->name,(const xmlChar *)"VOUCHER") == 0){
        
        //Invoice Number
         char *invoice_no = get_node_val_by_tag_name(voucherNode,"VOUCHERNUMBER");
         if(invoice_no){
           fwrite("\"",1,1,fp_out);
           fwrite(invoice_no,1,strlen(invoice_no),fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         free(invoice_no);
         
        //Invoice Date
         char *date = get_node_val_by_tag_name(voucherNode,"DATE");
         if(date){
           fwrite("\"",1,1,fp_out);
           fwrite(date,1,strlen(date),fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         free(date);
         
         //Invoice Status
         
         //Invoice Type
         if(strcmp(get_node_val_by_tag_name(voucherNode,"ISINVOICE"),"Yes") == 0){
           fwrite("\"",1,1,fp_out);
           fwrite("Invoice",1,7,fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
           fwrite("\"",1,1,fp_out);
           fwrite("Bill Of Supply",1,14,fp_out);
           fwrite("\",",1,2,fp_out);
         }
         
         //Reference
         char *reference = get_node_val_by_tag_name(voucherNode,"REFERENCE");
         if(reference){
           fwrite("\"",1,1,fp_out);
           fwrite(reference,1,strlen(reference),fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         free(reference);
         
         //Due Date
         
         //Customer Name
         char *party_name = get_node_val_by_tag_name(voucherNode,"PARTYNAME");
         if(party_name){
           fwrite("\"",1,1,fp_out);
           fwrite(party_name,1,strlen(party_name),fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         free(party_name);
         
         //Company Name
         char *company_name = NULL;
         xmlNode *address_list = get_single_node(voucherNode,"ADDRESS.LIST");
         if(address_list){
           for(xmlNode *addressNode = address_list->children;addressNode;addressNode = addressNode->next){
             if(addressNode->type == XML_ELEMENT_NODE && xmlStrcmp(addressNode->name,(const xmlChar *)"ADDRESS") == 0){
               company_name = xmlNodeGetContent(addressNode);
             }
           }
         }
         
         if(company_name){
           fwrite("\"",1,1,fp_out);
           fwrite(company_name,1,strlen(company_name),fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         free(company_name);
         
         //GST Treatment
         
         //GST Identification Number
         char *gi_no = get_node_val_by_tag_name(voucherNode,"PARTYGSTIN");
         if(gi_no){
           fwrite("\"",1,1,fp_out);
           fwrite(gi_no,1,strlen(gi_no),fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         free(gi_no);
         
         //TCS Tax Name
         
         //TCS Tax Amount
         
         //TDS Name
         
         //TDS Amount
         
         //Payment Terms
         
         //Place Of Supply
         char *state = get_node_val_by_tag_name(voucherNode, "PLACEOFSUPPLY");
         const char *state_code = NULL;
         if(state && (state_code = get_state_code(state))){
           fwrite("\"",1,1,fp_out);
           fwrite(state_code,1,strlen(state_code),fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         free(state);
         free(state_code);
         
         //Is Inclusive Tax
         
         //Discount Type
         
         //Entity Type
         fwrite("\"",1,1,fp_out);
         fwrite("item",1,4,fp_out);
         fwrite("\",",1,2,fp_out);
         
         //Item Type & Item Name
         char *new_qty = NULL;
         xmlNode *inventory_list = get_single_node(voucherNode,"INVENTORYENTRIES.LIST");
         if(inventory_list){
           for(xmlNode *itemNode = inventory_list->children;itemNode;itemNode = itemNode->next){
             if(itemNode->type == XML_ELEMENT_NODE && xmlStrcmp(itemNode->name,(const xmlChar *)"STOCKITEMNAME") == 0){
               xmlChar* item_name = xmlNodeGetContent(itemNode);
             }
             else if(itemNode->type == XML_ELEMENT_NODE && !new_qty && xmlStrcmp(itemNode->name,(const xmlChar *)"ACTUALQTY") == 0){
               xmlChar* qty = xmlNodeGetContent(itemNode);
               if(new_qty == NULL){
                 xmlChar* content = xmlNodeGetContent(itemNode);
                 if(content){
                   new_qty = strdup((char *)content);
                   xmlFree(content);
                 }
               }
             }
             else if(itemNode->type == XML_ELEMENT_NODE && xmlStrcmp(itemNode->name,(const xmlChar *)"RATE") == 0){
               xmlChar* item_price = xmlNodeGetContent(itemNode);
             }
    
             xmlNode *desc_list = get_single_node(itemNode,"BASICUSERDESCRIPTION.LIST");
              if(desc_list){
                for(xmlNode *descNode = desc_list->children;descNode;descNode = descNode->next){
                  if(descNode->type == XML_ELEMENT_NODE && xmlStrcmp(descNode->name,(const xmlChar *)"BASICUSERDESCRIPTION") == 0){
                    xmlChar * description = xmlNodeGetContent(descNode);
                  }
                }
              }
              
             xmlNode *discount_list = get_single_node(itemNode,"UDF:EIDISCOUNTRATE.LIST");
              if(discount_list){
                for(xmlNode *discNode = discount_list->children;discNode;discNode = discNode->next){
                  if(discNode->type == XML_ELEMENT_NODE && xmlStrcmp(discNode->name,(const xmlChar *)"UDF:EIDISCOUNTRATE") == 0){
                    xmlChar * discount = xmlNodeGetContent(discNode);
                  }
                }
              }
           }
         }
         
         if(item_name){
         //Item Type
           fwrite("\"",1,1,fp_out);
           fwrite("goods",1,5,fp_out);
           fwrite("\",",1,2,fp_out);
         //Item Name
           fwrite("\"",1,1,fp_out);
           fwrite(item_name,1,strlen(item_name),fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
         //Item Type
           fwrite("\"",1,1,fp_out);
           fwrite("service",1,7,fp_out);
           fwrite("\",",1,2,fp_out);
          //Item Name
          fwrite(",",1,1,fp_out);
         }
         xmlFree(item_name);
         
         //HSN/SAC
         
         //Description
         if(desciption){
           fwrite("\"",1,1,fp_out);
           fwrite(description,1,strlen(description),fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         xmlFree(description);
         
         //Quantity
         if(new_qty){
           fwrite("\"",1,1,fp_out);
           fwrite(new_qty,1,strlen(new_qty),fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         free(new_qty);
         
         //Item Price
         if(item_price){
           fwrite("\"",1,1,fp_out);
           fwrite(item_price,1,strlen(item_price),fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
           fwrite(",",1,1,fp_out);
         }
         xmlFree(item_price);
         
         //Item Tax
         
         //Item Tax Exemption Reason
         fwrite(",",1,1,fp_out);
         
         //Is Discount Before Tax
         if(strcmp(get_node_val_by_tag_name(voucherNode,"HASDISCOUNTS"),"Yes") == 0){
           fwrite("\"",1,1,fp_out);
           fwrite("true",1,4,fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
           fwrite("\"",1,1,fp_out);
           fwrite("false",1,5,fp_out);
           fwrite("\",",1,2,fp_out);
         }
         
         //Discount Is Percentage
         fwrite("\"",1,1,fp_out);
         fwrite("false",1,5,fp_out);
         fwrite("\",",1,2,fp_out);
         
         //Discount
         int new_discount = 0;
         int discount_value = atoi((const char *)discount);
         char buffer[5];
         if(discount_value > 0){
           new_discount = discount_value;
           sprintf(buffer, "%d", new_discount);
           
           fwrite("\"",1,1,fp_out);
           fwrite(buffer,1,strlen(buffer),fp_out);
           fwrite("\",",1,2,fp_out);
         }
         else{
           sprintf(buffer, "%d" , new_discount);
           fwrite("\"",1,1,fp_out);
           fwrite(buffer,1,strlen(buffer),fp_out);
           fwrite("\",",1,2,fp_out);
         }
         xmlFree(discount);
         
         //Total
         
         //Balance 
         
         //Shipping Charge
         xmlChar *shipping_charge = (xmlChar*) "0";
         xmlChar *round_off =  (xmlChar*) "0";
         xmlNode *ledger_list = get_single_node(voucherNode,"LEDGERENTRIES.LIST");
         if(ledger_list){
           for(xmlNode *ledgerNode = ledger_list->children;ledgerNode;ledgerNode = ledgerNode->next){
             if(ledgerNode->type == XML_ELEMENT_NODE && xmlStrcmp(ledgerNode->name,(const xmlChar *)"LEDGERNAME") == 0){
               if(xmlStrcmp(xmlNodeGetContent(ledgerNode), (xmlChar*) "Shipping Charge") == 0){
                 if(ledgerNode->type == XML_ELEMENT_NODE && xmlStrcmp(ledgerNode->name,(const xmlChar *)"AMOUNT") == 0){
                   shipping_charge = xmlNodeGetContent(ledgerNode);
                 }
               }
               else if(xmlStrcmp(xmlNodeGetContent(ledgerNode), (xmlChar*) "Other Charges") == 0){
                 if(ledgerNode->type == XML_ELEMENT_NODE && xmlStrcmp(ledgerNode->name,(const xmlChar *)"AMOUNT") == 0){
                   round_off = xmlNodeGetContent(ledgerNode);
                 }
               }
             }
           }
         }
         fwrite("\"",1,1,fp_out);
         fwrite(shipping_charge,1,strlen(shipping_charge),fp_out);
         fwrite("\",",1,2,fp_out);
         xmlFree(shipping_charge);
         
         //Shipping Charge Tax
         
         //Adjustment
         
         //Round Off
         fwrite("\"",1,1,fp_out);
         fwrite(round_off,1,strlen(round_off),fp_out);
         fwrite("\",",1,2,fp_out);
         xmlFree(round_off);
         
         //Write Off
         
         //Template Name
         fwrite("\"",1,1,fp_out);
         fwrite("Spreadsheet Template",1,strlen("Spreadsheet Template"),fp_out);
         fwrite("\",",1,2,fp_out);
         
         //Exchange Rate
         fwrite("\"",1,1,fp_out);
         fwrite("1",1,1,fp_out);
         fwrite("\",",1,2,fp_out);
         
         //Currency code
         fwrite("\"",1,1,fp_out);
         fwrite("INR",1,3,fp_out);
         fwrite("\",",1,2,fp_out);
         
         //Notes
         fwrite("\"",1,1,fp_out);
         fwrite("Thank for your business",1,strlen("Thank for your business"),fp_out);
         fwrite("\",",1,2,fp_out);
         
         //Terms & Conditions
         fwrite("\"",1,1,fp_out);
         fwrite("We will be charging 12% interest after the due date. Returns will be acceptable only within 2 weeks of delivery",1,strlen("We will be charging 12% interest after the due date. Returns will be acceptable only within 2 weeks of delivery"),fp_out);
         fwrite("\",",1,2,fp_out);
         
         fwrite(",,,,,,,,,,,,",1,12,fp_out);
         
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
    if (!format_invoices(root, "output_folder")) {
        std::cerr << "Error formatting items." << std::endl;
    } else {
        std::cout << "Items formatted successfully." << std::endl;
    }

    // Free the XML document and cleanup libxml2
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return 0; // Exit with a success code
}

