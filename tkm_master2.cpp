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

bool format_vendors(xmlNode *tallyNode, const char *output_folder){
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
  fclose(fp_out);
  return true; 
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
  xmlDocPtr doc = xmlReadFile(file_name, NULL, 0);
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
      for (xmlNode *bodyNode = root->children; bodyNode; bodyNode = bodyNode->next) {
        if (bodyNode->type == XML_ELEMENT_NODE && 
          xmlStrcmp(bodyNode->name, (const xmlChar *)"BODY") == 0) {
          for (xmlNode *importdataNode = bodyNode->children; importdataNode; 
                                                      importdataNode = importdataNode->next) {
            if (importdataNode && importdataNode->type == XML_ELEMENT_NODE && 
                xmlStrcmp(importdataNode->name, (const xmlChar *)"IMPORTDATA") == 0) {
              for (xmlNode *requestdataNode = importdataNode->children; requestdataNode; 
                                                          requestdataNode = requestdataNode->next) {
                if (requestdataNode->type == XML_ELEMENT_NODE && 
                  xmlStrcmp(requestdataNode->name, (const xmlChar *)"REQUESTDATA") == 0) {
                  do {
                    if(!format_customers(requestdataNode, cwd)){
                      printf("Some issue in parsing customers from tally xml file - %s\n", file_name);
                    }
                  } while(0);
                }
              }
            }
          }
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

