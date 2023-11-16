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

using namespace std;

enum gstTreatment{
  REGISTERED, 
  REGISTERED_COMPOSITION,
  UNREGISTERED,
  CONSUMER,
  SEZ,
  DEEMED_EXPORT,
  OUT_OF_SCOPE,
};

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


bool format_payments_made(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/PaymentsMade.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"Payment Number\",\"Date\",\"Customer Name\",\"Company Name\"," //1-4
                            "\"Place of Supply\",\"GST Treatment\",\"GST Identification Number\","//5-7
                            "\"Payment Type\",\"Description Of Supply\",\"Deposit To\"," //8-10
                            "\"Paid Through\",\"TDS Tax Account\",\"TDS Section\",\"TDS Amount\","//11-14
                            "\"Payment Mode\",\"Amount\", \"Reference\",\"Notes\",\"Bill Number\","
                            "\"Bill Amount\",\"Exchange Rate\",\"Currency Code\"";//15-20
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_vendor_credits(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/SupplierCredits.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"VendorCredit Number\",\"VendorCredit Date\",\"VendorCredit Status\",\"Reference\",\"Is Linked To Bill\", "//1-5
                            "\"Bill ID\", \"Customer Name\",\"Company Name\",\"GST Treatment\"," //6-9
                            "\"GST Identification Number\",\"Place of Supply\",\"Destination of Supply\"," //10-12
                            "\"Is Inclusive Tax\",\"Reverse Charge\",\"Discount Type\",\"Item Type\",\"Item Name\"," //13-17
                            "\"Description\",\"Quantity\",\"Item Price\",\"Item Tax\",\"Item Tax Exemption Reason\",\"ITC Eligibility\"," //18-23
                            "\"Is Discount Before Tax\",\"Discount is percentage\",\"Discount \",\"Account ID\"," //24-27
                            "\"Adjustment\",\"Exchange Rate\",\"Currency Code\",\"Notes\""; //27-28
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_sales_order(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/SalesOrders.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"SalesOrder Number\",\"SalesOrder Date\",\"SalesOrder Status\",\"Reference\",\"Shipment Date\","//1-5
                            "\"Customer Name\",\"Company Name\",\"GST Treatment\",\"GST Identification Number\"," //6-9
                            "\"Delivery Method\", \"Payment Terms\", \"Place of Supply\"," //10-12
                            "\"Is Inclusive Tax\",\"Discount Type\",\"Entity Type\",\"Item Type\",\"Item Name\",\"HSN/SAC\"," //13-18
                            "\"Description\",\"Quantity\",\"Item Price\",\"Item Tax\",\"Item Tax Exemption Reason\"," //19-23
                            "\"Is Discount Before Tax\",\"Discount is percentage\",\"Discount \",\"Cancelled Count\"," //24-27
                            "\"Invoiced Count\", \"Shipping Charge\", \"Shipping Charge Tax\", \"Adjustment\",\"Round off\",\"Template Name\"," //28-33
                            "\"Currency Code\",\"Exchange Rate\",\"Notes\",\"Terms & Conditions\",\"Billing Address\",\"Billing City\",\"Billing State\","
                            "\"Billing Country\",\"Billing Code\",\"Billing Fax\",\"Shipping Address\",\"Shipping City\","
                            "\"Shipping State\",\"Shipping Country\",\"Shipping Code\",\"Shipping Fax\"";

  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_credit_note_invoice(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/CreditNote_Applied_Invoices.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  }
  const char *header_line = "\"Number\",\"Date\",\"Is Invoice\",\"Invoice Number\",\"Amount\""; 
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_refund(xmlNode *tallyNode, const char *output_folder, bool is_cn = true){
  FILE *fp_out;
  char out_file[500];
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  }
  const char *header_line = "\"Refund Date\",\"Refund Type\",\"Related Type Number\",\"Payment Mode\","//1-4

                          "\"Account Name\",\"Amount\",\"Reference\",\"Description\""; //5-8


  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_purchase_order(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/PurchaseOrders.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 

  const char *header_line = "\"PurchaseOrder Number\",\"PurchaseOrder Date\",\"PurchaseOrder Status\",\"Reference\",\"Delivery Date\","//1-5
                            "\"Supplier Name\",\"Company Name\",\"GST Treatment\",\"GST Identification Number\"," //6-9
                            "\"Delivery Method\",\"Payment Terms\", \"Place of Supply\",\"Source of Supply\"," //10-13
                            "\"Is Inclusive Tax\",\"Reverse Charge Enabled\",\"Discount Type\",\"Item Type\",\"Item Name\"," //14-18
                            "\"HSN/SAC\",\"Description\",\"Quantity\",\"Item Price\",\"Item Tax\",\"Tax Exemption Reason\",\"Account Name\"," //19-25
                            "\"Is Discount Before Tax\",\"Discount is percentage\",\"Discount \",\"Discount Account ID\", \"Cancelled Count\"," //26-30
                            "\"Billed Count\",  \"Adjustment\",\"TCS Tax Name\",\"TCS Tax Amount\",\"Notes\",\"Terms & Conditions\","//31-36
                            "\"Billing Address\",\"Billing City\",\"Billing State\"," //37-39
                            "\"Billing Country\",\"Billing Code\",\"Billing Fax\",\"Shipping Address\",\"Shipping City\","//40-44
                            "\"Shipping State\",\"Shipping Country\",\"Shipping Code\",\"Shipping Fax\"";//45-48
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_price_list(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/SalesPriceLists.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"Name\",\"Description\",\"Currency Code\",\"Type\","
                      "\"Percentage\",\"Rounding Type\",\"Item Name\",\"SKU\","
                      "\"Rate\",\"Start Quantity\",\"End Quantity\"";
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_journal(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/Journals.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"Journal Date\",\"Reference Number\","
              "\"Journal Number\",\"Description\",\"Journal Type\","
              "\"Currency\",\"Status\",\"Account\",\"Notes\","
              "\"Contact Name\",\"Debit\",\"Credit\"";
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_item(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/Items.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"Name\",\"SKU\",\"HSN/SAC\",\"Active\","
                      "\"Type\",\"Unit\",\"Sales account\",\"Sales rate\","
                      "\"Sales description\",\"Purchase account\",\"Purchase rate\","
                      "\"Purchase description\",\"Taxable\",\"Tax exemption reason\","
                      "\"Intrastate tax type\",\"Inter state tax type\",\"Inventory enabled\","
                      "\"Inventory account\",\"Preffered Vendor\",\"Initial Stock\","
                      "\"Initial Stock Rate\",\"Reorder point\",\"Current Stock\", \"Item Category\"";

  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_debit_note(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/DebitNotes.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"Debit Note Number\",\"Debit Note Date\",\"Debit Note Status\",\"Invoice Number\",\"Due Date\","//1-5
                          "\"Customer Name\",\"Company Name\",\"GST Treatment\",\"GST Identification Number\"," //6-9
                          "\"TCS Tax Name\",\"TCS Tax Amount\",\"TDS Name\",\"TDS Amount\",\"Payment Terms\", \"Place of Supply\"," //10-15
                          "\"Reason\",\"Is Inclusive Tax\",\"Discount Type\",\"Item Description\"," //16-19
                          "\"HSN/SAC\",\"Item Price\",\"Item Tax\",\"Item Tax Exemption Reason\"," //20-23
                          "\"Is Discount Before Tax\",\"Discount is percentage\",\"Discount \"," //24-26
                          "\"Shipping Charge\", \"Shipping Charge Tax\", \"Adjustment\",\"Round off\",\"Write Off\"" //27-30
                          "\"Exchange Rate\",\"Currency Code\",\"Notes\",\"Terms & Conditions\","
                          "\"Billing Address\",\"Billing City\",\"Billing State\"," //31-35
                          "\"Billing Country\",\"Billing Code\",\"Billing Fax\",\"Shipping Address\",\"Shipping City\"," //36-40
                          "\"Shipping State\",\"Shipping Country\",\"Shipping Code\",\"Shipping Fax\""; //41-44
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_invoice(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/Invoices.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"Invoice Number\",\"Invoice Date\",\"Invoice Status\",\"Invoice Type\",\"Reference\",\"Due Date\","//1-6
                            "\"Customer Name\",\"Company Name\",\"GST Treatment\",\"GST Identification Number\"," //7-10
                            "\"TCS Tax Name\",\"TCS Tax Amount\",\"TDS Name\", \"TDS Amount\", \"Payment Terms\", \"Place of Supply\"," //11-16
                            "\"Is Inclusive Tax\",\"Discount Type\",\"Entity Type\",\"Item Type\",\"Item Name\",\"HSN/SAC\"," //17-21
                            "\"Description\",\"Quantity\",\"Item Price\",\"Item Tax\",\"Item Tax Exemption Reason\"," //22-26
                            "\"Is Discount Before Tax\",\"Discount is percentage\",\"Discount \",\"Total\"," //27-30
                            "\"Balance\", \"Shipping Charge\", \"Shipping Charge Tax\", \"Adjustment\",\"Round off\",\"Write Off\", \"Template Name\"," //31-37
                            "\"Exchange Rate\",\"Currency Code\",\"Notes\",\"Terms & Conditions\",\"Billing Address\",\"Billing City\",\"Billing State\"," //38-43
                            "\"Billing Country\",\"Billing Code\",\"Billing Fax\",\"Shipping Address\",\"Shipping City\"," //44-48
                            "\"Shipping State\",\"Shipping Country\",\"Shipping Code\",\"Shipping Fax\""; //49-52

  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_expense(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/Expenses.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"Expense Name\",\"Expense Date\",\"Expense Type\",\"Expense Status\","
                            "\"Reference\",\"Paid Through Account\",\"Paid To Account\","
                            "\"Vendor Name\",\"Customer Name\",\"GST Treatment\","
                            "\"GST Identification Number\",\"Amount\", \"Place of Supply\","
                            "\"Source of Supply\", \"Currency Code\",\"Exchange Rate\",\"Is Inclusive Tax\","
                            "\"Tax Type\",\"Tax Excemption Reason\",\"ITC Type\",\"Billable\","
                            "\"Reverse charge enabled\",\"Markup Percentage\",\"HSN/SAC\",\"Notes\"";
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_estimate(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/Estimates.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"Estimate Number\",\"Estimate Date\",\"Estimate Status\",\"Reference\",\"Expiry Date\"," //1-5
                          "\"Customer Name\",\"Company Name\",\"GST Treatment\",\"GST Identification Number\"," //6-9
                          "\"Place of Supply\",\"Currency Code\",\"Exchange Rate\",\"Is Inclusive Tax\"," //10-13
                          "\"Discount Type\",\"Entity Type\",\"Item Type\",\"Item Name\",\"HSN/SAC\", \"Description\"," //14-19
                          "\"Quantity\",\"Item Price\",\"Item Tax\",\"Item Tax Exemption Reason\"," //20-23
                          "\"Is Discount Before Tax\",\"Discount is percentage\",\"Discount \",\"Shipping Charge\"," //24-27
                          "\"Shipping Charge Tax\",\"Adjustment\",\"Round off\",\"Template Name\"," //28-31
                          "\"TCS Tax Name\",\"TCS Tax Amount\",\"Notes\",\"Terms & Conditions\"," //32-35
                          "\"Billing Address\",\"Billing City\",\"Billing State\",\"Billing Country\"," //36-39
                          "\"Billing Code\",\"Billing Fax\",\"Shipping Address\",\"Shipping City\"," //40-43
                          "\"Shipping State\",\"Shipping Country\",\"Shipping Code\",\"Shipping Fax\""; //44-47
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_payments_received(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/PaymentsReceived.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"Payment Number\",\"Date\",\"Customer Name\",\"Company Name\","
                            "\"Place of Supply\",\"GST Treatment\",\"GST Identification Number\","
                            "\"Payment Type\",\"Description Of Supply\",\"Deposit To\",\"TDS Tax Account\","
                            "\"Currency Code\",\"Exchange Rate\",\"Payment Mode\", \"Bank Charges\","
                            "\"Amount\", \"Reference\",\"Notes\",\"Invoice Number\",\"Invoice Amount\","
                            "\"Withholding Tax Amount\",\"Tax Account\"";
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_credit_note(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/CreditNotes.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 

  const char *header_line = "\"CreditNote Number\",\"CreditNote Date\",\"CreditNote Status\",\"Reference\",\"Is Linked To Invoice\", "//1-5
                            "\"Invoice ID\", \"Customer Name\",\"Company Name\",\"GST Treatment\"," //6-9
                            "\"GST Identification Number\",\"Place of Supply\",\"Reason\"," //10-12
                            "\"Is Inclusive Tax\",\"Discount Type\",\"Entity Type\",\"Item Type\",\"Item Name\",\"HSN/SAC\"," //13-18
                            "\"Description\",\"Quantity\",\"Item Price\",\"Item Tax\",\"Item Tax Exemption Reason\"," //19-23
                            "\"Is Discount Before Tax\",\"Discount is percentage\",\"Discount \",\"Account ID\"," //24-27
                            " \"Shipping Charge\", \"Shipping Charge Tax\", \"Adjustment\",\"Round off\"," //28-31
                            "\"TCS Tax Name\",\"TCS Tax Amount\",\"Currency Code\",\"Exchange Rate\",\"Notes\","
                            "\"Terms & Conditions\",\"Billing Address\",\"Billing City\",\"Billing State\"," //32-40
                            "\"Billing Country\",\"Billing Code\",\"Billing Fax\",\"Shipping Address\",\"Shipping City\"," //31-45
                            "\"Shipping State\",\"Shipping Country\",\"Shipping Code\",\"Shipping Fax\""; //46-49


  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_bill(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/Bills.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"Bill Number\",\"Bill Date\",\"Bill Status\",\"Reference\",\"Due Date\","//1-5
                            "\"Supplier Name\",\"Company Name\",\"GST Treatment\",\"GST Identification Number\"," //6-9
                            "\"TCS Tax Name\",\"TCS Tax Percentage\",\"TDS Tax Name\",\"TDS Tax Percentage\"," //10-13
                            "\"TCS/TCS Tax Amount\",\"Payment Terms\", \"Place of Supply\", \"Source of Supply\"," //14-17
                            "\"Is Inclusive Tax\",\"Reverse Charge\",\"Discount Type\",\"Item Type\",\"Item Name\",\"HSN/SAC\"," //18-23
                            "\"Description\",\"Quantity\",\"Item Price\",\"Item Tax\",\"Tax Excemption Reason\",\"ITC Eligibility\"," //24-29
                            "\"Account Id\",\"Is Discount Before Tax\",\"Discount is percentage\",\"Discount \",\"Discount Account id\","//30-34
                            "\"Total\",\"Paid Amount\", \"Adjustment\",\"Exchange Rate\",\"Currency Code\",\"Notes\""; //35-38*/
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

bool format_chart_of_accounts(xmlNode *tallyNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/Chart_of_accounts.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"Name\",\"Identifier\",\"Description\",\"Category\","
                    "\"Parent Account\"";
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  fclose(fp_out);
  return true; 
}

enum contactType {
  CONTACT_TYPE_CUSTOMER,
  CONTACT_TYPE_VENDOR,
};

bool format_contacts(xmlNode *requestNode, const char* parent_string, 
            contactType contact_type,  FILE *fp_out) {
  xmlNode *ledgerNode = NULL;
  for (xmlNode *tallyNode = requestNode->children; tallyNode; tallyNode = tallyNode->next) {
    if (tallyNode->type == XML_ELEMENT_NODE &&  
      xmlStrcmp(tallyNode->name, (const xmlChar *)"TALLYMESSAGE") == 0) {
      for (ledgerNode = tallyNode->children; ledgerNode; ledgerNode = ledgerNode->next) {
        if (ledgerNode->type == XML_ELEMENT_NODE && xmlStrcmp(ledgerNode->name, (const xmlChar *)"LEDGER") == 0) {
          for (xmlNode *childNode = ledgerNode->children; childNode; childNode = childNode->next) {
            if (childNode->type == XML_ELEMENT_NODE && xmlStrcmp(childNode->name, (const xmlChar *)"PARENT") == 0) {
              xmlChar *parentContent = xmlNodeGetContent(childNode);
              if (parentContent && strcmp((const char *)parentContent, parent_string) == 0) {
                char *name = get_attribute(ledgerNode, "NAME");
                if(name){
                  fwrite("\"",1,1,fp_out);
                  fwrite(name, 1, strlen(name), fp_out); 
                  fwrite("\",",1,2,fp_out);
                  free(name);
                } else {
                  fwrite(",",1,1,fp_out);
                }
                fwrite(",", 1,1, fp_out);
                const char* contact_str = contact_type == CONTACT_TYPE_CUSTOMER ? "customer" : "vendor";
                fwrite(contact_str, 1, strlen(contact_str), fp_out);
                fwrite(",,,,",1,strlen(",,,,"),fp_out);
                char *state = get_node_val_by_tag_name(ledgerNode, "LEDSTATENAME");
                const char *state_code = NULL;
                if(state && (state_code = get_state_code(state))){
                  if(contact_type == CONTACT_TYPE_CUSTOMER){
                    fwrite(",\"",1,2,fp_out);
                    fwrite(state_code, 1, strlen(state_code), fp_out); 
                    fwrite("\"",1,1,fp_out);
                  } else {
                    fwrite("\"",1,1,fp_out);
                    fwrite(state_code, 1, strlen(state_code), fp_out); 
                    fwrite("\",",1,2,fp_out);
                  }
                } else {
                  fwrite(",,",1,1,fp_out);
                }
                fwrite(",,,,,,,",1,strlen(",,,,,,,"),fp_out); //Upto Website

                gstTreatment gst_treatment = UNREGISTERED;
                char *is_sez = get_node_val_by_tag_name(ledgerNode, "ISSEZPARTY");
                if(is_sez && strcasecmp(is_sez, "Yes") == 0){
                  gst_treatment = SEZ;
                } else {
                  char *is_exempted = get_node_val_by_tag_name(ledgerNode, "ISEXEMPTED"); 
                  if(is_exempted && strcasecmp(is_exempted, "Yes") == 0){
                    gst_treatment = OUT_OF_SCOPE;
                  } else {
                    char* gst_type = get_node_val_by_tag_name(ledgerNode, "GSTREGISTRATIONTYPE");
                    if(gst_type){
                      if(strcasecmp(gst_type, "Regular") == 0){
                        gst_treatment = REGISTERED;
                      } else if(strcasecmp(gst_type, "Composition") == 0){
                        gst_treatment = REGISTERED_COMPOSITION;
                      } else if(strcasecmp(gst_type, "Unregistered") == 0){
                        gst_treatment = UNREGISTERED;
                      } else if(strcasecmp(gst_type, "Customer") == 0){
                        gst_treatment = CONSUMER;
                      }
                    }
                    free(gst_type);
                  }
                  free(is_exempted);
                }
                free(is_sez);
                const char *gst_str = "business_none";
                switch(gst_treatment){
                  case REGISTERED:
                    gst_str = "business_gst";
                  break; 
                  case REGISTERED_COMPOSITION:
                    gst_str = "business_registered_composition";
                  break;
                  case CONSUMER:
                    gst_str = "business_registered_composition";
                  break;
                  case SEZ:
                    gst_str = "business_sez";
                  break;
                  default:; 
                }
                fwrite("\"",1,1,fp_out);
                fwrite(gst_str,1,strlen(gst_str),fp_out); 
                fwrite("\",",1,2,fp_out);
                char* gst_no = get_node_val_by_tag_name(ledgerNode, "PARTYGSTIN");
                if(gst_no){
                  fwrite("\"",1,1,fp_out);
                  fwrite(gst_no,1,strlen(gst_no),fp_out);
                  fwrite("\",",1,2,fp_out);
                } else {
                  fwrite(",",1,1,fp_out);
                }
                free(gst_no);
                char* pan_no = get_node_val_by_tag_name(ledgerNode, "INCOMETAXNUMBER");
                if(pan_no){
                  fwrite("\"",1,1,fp_out);
                  fwrite(pan_no,1,strlen(pan_no),fp_out);
                  fwrite("\",",1,2,fp_out);
                } else {
                  fwrite(",",1,1,fp_out);
                }
                free(pan_no);
                fwrite(",,",1,2,fp_out);
                xmlNode *address_list = get_single_node(ledgerNode, "ADDRESS.LIST");
                char* addr_line_1 = NULL;
                char* addr_line_2 = NULL;
                if(address_list){
                  for (xmlNode *addrNode = address_list->children; addrNode; addrNode = addrNode->next) {
                    if (addrNode->type == XML_ELEMENT_NODE && !(addr_line_1 && addr_line_2) &&
                        xmlStrcmp(addrNode->name, (const xmlChar *)"ADDRESS") == 0) {
                      if(addr_line_1 == NULL){
                        xmlChar* content = xmlNodeGetContent(addrNode);
                        if(content){
                          addr_line_1 = strdup((char *)content);
                          xmlFree(content);
                        }
                      } else {
                        xmlChar* content = xmlNodeGetContent(addrNode);
                        if(content){
                          addr_line_2 = strdup((char *)content);
                          xmlFree(content);
                        }
                      }
                    }
                  }
                }
                if(addr_line_1){
                  fwrite("\"",1,1,fp_out);
                  fwrite(addr_line_1,1,strlen(addr_line_1),fp_out);
                  fwrite("\",",1,2,fp_out);
                } else {
                  fwrite(",",1,1,fp_out);
                }
                if(addr_line_2){
                  fwrite("\"",1,1,fp_out);
                  fwrite(addr_line_2,1,strlen(addr_line_2),fp_out);
                  fwrite("\",",1,2,fp_out);
                } else {
                  fwrite(",",1,1,fp_out);
                }
                fwrite(",",1,1,fp_out);
                if(state){
                  fwrite("\"",1,1,fp_out);
                  fwrite(state,1,strlen(state),fp_out);
                  fwrite("\",",1,2,fp_out);
                } else {
                  fwrite(",",1,1,fp_out);
                }
                char* country = get_node_val_by_tag_name(ledgerNode, "COUNTRYNAME");
                if(country){
                  fwrite("\"",1,1,fp_out);
                  fwrite(country,1,strlen(country),fp_out);
                  fwrite("\",",1,2,fp_out);
                } else {
                  fwrite(",",1,1,fp_out);
                }
                char* pincode = get_node_val_by_tag_name(ledgerNode, "PINCODE");
                if(pincode){
                  fwrite("\"",1,1,fp_out);
                  fwrite(pincode,1,strlen(pincode),fp_out);
                  fwrite("\",",1,2,fp_out);
                } else {
                  fwrite(",",1,1,fp_out);
                }
                fwrite(",,",1,2,fp_out);
                if(addr_line_1){
                  fwrite("\"",1,1,fp_out);
                  fwrite(addr_line_1,1,strlen(addr_line_1),fp_out);
                  fwrite("\",",1,2,fp_out);
                } else {
                  fwrite(",",1,1,fp_out);
                }
                if(addr_line_2){
                  fwrite("\"",1,1,fp_out);
                  fwrite(addr_line_2,1,strlen(addr_line_2),fp_out);
                  fwrite("\",",1,2,fp_out);
                } else {
                  fwrite(",",1,1,fp_out);
                }
                fwrite(",",1,1,fp_out);
                if(state){
                  fwrite("\"",1,1,fp_out);
                  fwrite(state,1,strlen(state),fp_out);
                  fwrite("\",",1,2,fp_out);
                } else {
                  fwrite(",",1,1,fp_out);
                }
                if(country){
                  fwrite("\"",1,1,fp_out);
                  fwrite(country,1,strlen(country),fp_out);
                  fwrite("\",",1,2,fp_out);
                } else {
                  fwrite(",",1,1,fp_out);
                }
                if(pincode){
                  fwrite("\"",1,1,fp_out);
                  fwrite(pincode,1,strlen(pincode),fp_out);
                  fwrite("\",",1,2,fp_out);
                } else {
                  fwrite(",",1,1,fp_out);
                }
                fwrite("\n",1,1,fp_out);
                free(addr_line_1);
                free(addr_line_2);
                free(country);
                free(pincode);
                free(state);
              }
              xmlFree(parentContent);
            }
          }
        }
      }
    }
  }

  fclose(fp_out);
  return true; 
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
  return format_contacts(requestNode, "Sundry Debtors", CONTACT_TYPE_CUSTOMER, fp_out);
}

bool format_vendors(xmlNode *requestNode, const char *output_folder){
  FILE *fp_out;
  char out_file[500];
  sprintf(out_file,"%s/Vendors.csv", output_folder);
  fp_out = fopen(out_file, "wb");
  if(!fp_out){
    printf("Unable to create file - %s(%s)\n", out_file, strerror(errno));
    return false;
  } 
  const char *header_line = "\"Display Name\",\"Company Name\",\"Type\",\"Salutation\","
                      "\"First Name\",\"Last Name\",\"Source Of Supply\",\"Place Of Supply\","
                      "\"Currency Code\",\"EmailID\",\"Phone\",\"MobilePhone\",\"Notes\","
                      "\"Website\",\"GST Treatment\",\"GST Identification Number (GSTIN)\","
                      "\"PAN Number\",\"Billing Attention\",\"Billing Address\",\"Billing Street2\","
                      "\"Billing City\",\"Billing State\",\"Billing Country\",\"Billing Code\","
                      "\"Billing Fax\",\"Shipping Attention\",\"Shipping Address\","
                      "\"Shipping Street2\",\"Shipping City\",\"Shipping State\","
                      "\"Shipping Country\",\"Shipping Code\",\"Shipping Fax\"";
  fwrite(header_line,1,strlen(header_line),fp_out);
  fwrite("\n",1,1,fp_out);
  return format_contacts(requestNode, "Sundry Creditors", CONTACT_TYPE_VENDOR, fp_out);
}
enum {
  MASTER,
  VOUCHER
};

//#define USE_CTXT

int main(int argc, char* argv[]){
  if(argc < 4){
    printf("Usage is %s -t type -f <tall_xml_file>\n", argv[0]);
    return -1;
  }
  int type = MASTER;
  int c;
  char *file_name = NULL; 
  while ((c = getopt(argc, argv, "t:f:")) != -1){
    switch(c){
      case 't':
        type = strcasecmp(optarg, "voucher") == 0 ? VOUCHER : MASTER;
        break;
      case 'f':
        file_name = strdup(optarg);
        break;
    }
  }
  char cwd[400];
  if(getcwd(cwd, sizeof(cwd)) == NULL) {
    printf("unable to get current working dir");
    free(file_name);
    return -2;
  }
  strncat(cwd, "//output", 399);
  mkdir(cwd,0666);
#ifdef USE_CTXT 
  LIBXML_TEST_VERSION;
  xmlParserCtxtPtr ctxt = xmlNewParserCtxt();;
  if (ctxt == NULL) {
    printf("Failed to allocate parser context\n");
    free(file_name);
    return -1; 
  }
  xmlDocPtr doc = xmlCtxtReadFile(ctxt, file_name, NULL, XML_PARSE_DTDVALID);
#else 
  xmlDocPtr doc = xmlReadFile(file_name, NULL, XML_PARSE_RECOVER); 
#endif
  if (doc == NULL) {
    printf("Failed to parse %s\n", file_name);
    free(file_name);

#ifdef USE_CTXT 
    xmlFreeParserCtxt(ctxt);
#endif
    return -1; 
  }
  xmlNode *root = xmlDocGetRootElement(doc);
  if(type == MASTER){
    if(root->type != XML_ELEMENT_NODE || 
          xmlStrcmp(root->name, (const xmlChar *)"ENVELOPE")) {
      printf("Not able to find ENVELOPE tag\n");  
    } else {
      xmlNode *requestdataNode = get_single_node(get_single_node(get_single_node(root, "BODY"), "IMPORTDATA"), "REQUESTDATA");
      if(requestdataNode){
        if(!format_customers(requestdataNode, cwd)){
          printf("Some issue in parsing customers from tally xml file - %s\n", file_name);
        }
        if(!format_vendors(requestdataNode, cwd)){
          printf("Some issue in parsing customers from tally xml file - %s\n", file_name);
        }
      }
    }
    //handle master cases here like items, customers, vendors and may be chart of accounts
  } else {
    //handle various kinds of vouchers here like invoices, sales orders, bills etc. here.
  }
  free(file_name);
  xmlFreeDoc(doc); 
#ifdef USE_CTXT 
  xmlFreeParserCtxt(ctxt);
#endif
  return 0;
}
