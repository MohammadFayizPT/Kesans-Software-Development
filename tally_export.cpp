#include "consts.h"
#include "tally_export.h"
#include "perm_check.h"
#include "currencies.h"
#include "units.h"
#include "items.h"
#include "tax.h"
#include "contacts.h"
#include "location.h"
#include "banking.h"
#include "settings.h"
#include "invoices.h"
#include "pos.h"
#include "bills.h"
#include "utils.h"
#include "vendor_credits.h"
#include "vendor_credits.h"
#include "credit_notes.h"
#include "message_template.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/xmlstring.h>
#include <vector>
#include <zip.h>
#include <sys/stat.h>
#include <sys/types.h>
extern char* g_host_uri;
extern char* base_dir;
extern char* instance;

struct tallyExport{
  int from_date;
  int to_date;
  tallyExport():from_date(0), to_date(0){
  };
};

static inline double rnd(double var)
{
  bool is_neg = false;
  if(var < 0){
    is_neg = true;
    var *= -1;
  }
  double value = (double)(var * 100 + .5);
  long long lvalue = (long long)(value);
  if(is_neg){
    lvalue *= -1;
  }
  return (double)lvalue / 100;
}

const char* get_decimal_places(int sub_unit_multiples){
  const char* ret = "2";
  if(sub_unit_multiples == 10){
    ret = "1";
  } else if(sub_unit_multiples == 100){
    ret = "2";
  } else if(sub_unit_multiples == 1000){
    ret = "3";
  }
  return ret;
}

char* f_n_replace(const char* buff, const char* searchWord, const char* replaceWord) {
  size_t buffSize = strlen(buff);
  size_t searchWordLength = strlen(searchWord);
  size_t replaceWordLength = strlen(replaceWord);

  // Calculate the size difference between search word and replace word
  int sizeDiff = replaceWordLength - searchWordLength;
  size_t resultSize = buffSize + (sizeDiff > 0 ? sizeDiff * buffSize : 0);

  char* result = (char*)malloc((resultSize + 1) * sizeof(char));  // Buffer to store the resulting data
  size_t resultIndex = 0;  // Index to track the current position in the result buffer

  for (size_t i = 0; i < buffSize; i++) {
    if (strncmp(&buff[i], searchWord, searchWordLength) == 0) {
      // Replace the search word with the replace word
      for (size_t j = 0; replaceWord[j] != '\0'; j++) {
        result[resultIndex++] = replaceWord[j];
      }
      i += searchWordLength - 1;  // Skip the search word in the input buffer
    } else {
      result[resultIndex++] = buff[i];
    }
  }

  result[resultIndex] = '\0';  // Null-terminate the result buffer
  return result;
}


char *html_entity_decode(const char *str) {
    char *result = NULL;
    char *tmp = NULL;
    const char *p = str;

    while (*p) {
        if (*p == '&') {
            const char *q = p + 1;
            int codepoint = 0;

            if (*q == '#') {
                q++;

                if (*q == 'x') {
                    q++;
                    codepoint = strtol(q, &tmp, 16);
                } else {
                    codepoint = strtol(q, &tmp, 10);
                }

                if (*tmp != ';') {
                    // Not a valid entity
                    free(result);
                    return NULL;
                }

                p = tmp + 1;
            } else {
                // Not a numeric entity
                free(result);
                return NULL;
            }

            char utf8[5] = { 0 };
            if (codepoint <= 0x7F) {
                utf8[0] = (char)codepoint;
            } else if (codepoint <= 0x7FF) {
                utf8[0] = (char)(0xC0 | (codepoint >> 6));
                utf8[1] = (char)(0x80 | (codepoint & 0x3F));
            } else if (codepoint <= 0xFFFF) {
                utf8[0] = (char)(0xE0 | (codepoint >> 12));
                utf8[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
                utf8[2] = (char)(0x80 | (codepoint & 0x3F));
            } else if (codepoint <= 0x10FFFF) {
                utf8[0] = (char)(0xF0 | (codepoint >> 18));
                utf8[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
                utf8[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
                utf8[3] = (char)(0x80 | (codepoint & 0x3F));
            } else {
                // Not a valid Unicode codepoint
                free(result);
                return NULL;
            }

            if(result){
              result = (char *)realloc(result, strlen(result) + 5);
              strcat(result, utf8);
            } else {
              result = strdup(utf8);
            }
        } else {
            char c[2] = { *p, '\0' };
            if(result){
              result = (char *)realloc(result, strlen(result) + 2);
              strcat(result, c);
            } else {
              result = strdup(c);
            }
            p++;
        }
    }

    return result;
}

static void make_date(char* date, int time_stamp){                         
  if(!date){                                        
    return;                                                       
  }                                                             
  time_t loc_time = (time_t) time_stamp;                                         
  struct tm* tmp_date = localtime(&loc_time);
  //tmp_date->tm_mday = 1; //TODO : Remove this.. workaround for testing in trial version. 
  strftime(date,12,"%Y%m%d", tmp_date);
} 

bool fill_currency_objects(xmlNodePtr parent, MYSQL *mysql_handle, int thread_id){
  bool ret = false;
  Currency* currency_list = get_select_currencies();
  while(currency_list){
    if(currency_list->symbol && currency_list->code){
      xmlNodePtr currency_node = xmlNewChild(parent, NULL, BAD_CAST "CURRENCY", NULL);
      char *currency_name = html_entity_decode(currency_list->symbol);
      xmlNewProp(currency_node, BAD_CAST "NAME", BAD_CAST currency_name);
      free(currency_name);
      xmlNodePtr original_name = xmlNewChild(currency_node, NULL, BAD_CAST "ORIGINALNAME", NULL);
      xmlNodeSetContent(original_name, BAD_CAST currency_list->symbol);
      if(!strcmp(currency_list->symbol, currency_list->code) && currency_list->desc){
        xmlNodePtr mailing_name = xmlNewChild(currency_node, NULL, BAD_CAST "MAILINGNAME", NULL);
        xmlNodeSetContent(mailing_name, BAD_CAST currency_list->desc);
        xmlNodePtr expanded_symbol = xmlNewChild(currency_node, NULL, BAD_CAST "EXPANDEDSYMBOL", NULL);
        xmlNodeSetContent(expanded_symbol, BAD_CAST currency_list->desc);
      } else {
        xmlNodePtr mailing_name = xmlNewChild(currency_node, NULL, BAD_CAST "MAILINGNAME", NULL);
        xmlNodeSetContent(mailing_name, BAD_CAST currency_list->code);
        xmlNodePtr expanded_symbol = xmlNewChild(currency_node, NULL, BAD_CAST "EXPANDEDSYMBOL", NULL);
        xmlNodeSetContent(expanded_symbol, BAD_CAST currency_list->code);
      }
      if(currency_list->sub_units){
        xmlNodePtr decimal_symbol = xmlNewChild(currency_node, NULL, BAD_CAST "DECIMALSYMBOL", NULL);
        xmlNodeSetContent(decimal_symbol, BAD_CAST currency_list->sub_units);
      }
      xmlNodePtr in_millions = xmlNewChild(currency_node, NULL, BAD_CAST "INMILLIONS", NULL);
      xmlNodeSetContent(in_millions, BAD_CAST (currency_list->format != 10 ? "Yes": "No"));
      xmlNodePtr decimal_places = xmlNewChild(currency_node, NULL, BAD_CAST "DECIMALPLACES", NULL);
      xmlNodeSetContent(decimal_places, BAD_CAST (currency_list->sub_unit_multiples == 0 ? "2" :
                                             get_decimal_places(currency_list->sub_unit_multiples)));
      ret = true;
    }
    Currency* tmp_currency = currency_list;
    currency_list = currency_list->next;
    delete tmp_currency;
  }
  return ret;
}

bool fill_unit_objects(xmlNodePtr parent, MYSQL *mysql_handle, int thread_id){
  bool ret = false;
  Unit *unit_list = get_units();
  while(unit_list){
    xmlNodePtr unit_node = xmlNewChild(parent, NULL, BAD_CAST "UNIT", NULL);
    xmlNewProp(unit_node, BAD_CAST "NAME", BAD_CAST unit_list->display_name);
    xmlNodePtr name = xmlNewChild(unit_node, NULL, BAD_CAST "NAME", NULL);
    xmlNodeSetContent(name, BAD_CAST unit_list->display_name);
    xmlNodePtr is_simple = xmlNewChild(unit_node, NULL, BAD_CAST "ISSIMPLEUNIT", NULL);
    xmlNodeSetContent(is_simple, BAD_CAST "Yes");
    xmlNodePtr gst_name = xmlNewChild(unit_node, NULL, BAD_CAST "GSTREPUOM", NULL);
    char* tmp_ptr = get_uqc_code(unit_list->id);
    if(tmp_ptr){
      xmlNodeSetContent(gst_name, BAD_CAST tmp_ptr);
      free(tmp_ptr);
    }
    Unit* tmp_unit = unit_list;
    unit_list = unit_list->next;
    delete tmp_unit;
    ret = true;
  }
  std::vector<UnitMap*> *unit_maps =  get_unit_maps();
  for(auto unit_map : *unit_maps){
    const char *primary_unit_name = get_unit_name(unit_map->primary_id);
    const char *secondary_unit_name = get_unit_name(unit_map->secondary_id);
    if(primary_unit_name && secondary_unit_name){
      char tmp_name[100];
      snprintf(tmp_name, 99, "%s of %0.2f %s", primary_unit_name, unit_map->factor, secondary_unit_name);
      xmlNodePtr unit_node = xmlNewChild(parent, NULL, BAD_CAST "UNIT", NULL);
      xmlNewProp(unit_node, BAD_CAST "NAME", BAD_CAST tmp_name);
      xmlNodePtr name = xmlNewChild(unit_node, NULL, BAD_CAST "NAME", NULL);
      xmlNodeSetContent(name, BAD_CAST tmp_name);
      xmlNodePtr as_original = xmlNewChild(unit_node, NULL, BAD_CAST "ASORIGINAL", NULL);
      xmlNodeSetContent(as_original, BAD_CAST "Yes");
      xmlNodePtr base_unit = xmlNewChild(unit_node, NULL, BAD_CAST "BASEUNITS", NULL);
      xmlNodeSetContent(base_unit , BAD_CAST primary_unit_name);
      xmlNodePtr additional_unit = xmlNewChild(unit_node, NULL, BAD_CAST "ADDITIONALUNITS", NULL);
      xmlNodeSetContent(additional_unit, BAD_CAST secondary_unit_name);
      xmlNodePtr is_simple = xmlNewChild(unit_node, NULL, BAD_CAST "ISSIMPLEUNIT", NULL);
      xmlNodeSetContent(is_simple, BAD_CAST "No");
      xmlNodePtr conversion = xmlNewChild(unit_node, NULL, BAD_CAST "CONVERSION", NULL);
      sprintf(tmp_name, "%0.2f", unit_map->factor);
      xmlNodeSetContent(conversion, BAD_CAST tmp_name);
    }
    delete unit_map;
    ret = true;
  }
  delete unit_maps;
  return ret;
}

bool fill_item_categories(xmlNodePtr parent, MYSQL *mysql_handle, int thread_id){
  bool ret = false;
  ItemCategory* category_list = get_item_categories();
  while(category_list){
    xmlNodePtr category_node = xmlNewChild(parent, NULL, BAD_CAST "STOCKCATEGORY", NULL);
    char *esc_str = escape_html(category_list->name);
    xmlNewProp(category_node, BAD_CAST "NAME", BAD_CAST esc_str);
    xmlNewChild(category_node, NULL, BAD_CAST "NAME", BAD_CAST esc_str );
    free(esc_str);
    ItemCategory* tmp_category = category_list;
    category_list = category_list->next;
    delete tmp_category;
    ret = true;
  }
  return ret;
}

bool fill_item_objects(xmlNodePtr parent, MYSQL *mysql_handle, int thread_id, bool &error){
  bool ret = false;
  std::vector<Item *> *item_list = get_all_items(mysql_handle, error);
  if(error){
    if(item_list){
      for(auto item : *item_list){
        delete item;
      }
      delete item_list;
    }
    return ret;
  }
  if(!item_list){
    return ret;
  }
  for(auto item : *item_list){
    char tmp_str[50];
    if(is_empty(item->name)){
      delete item;
      continue;
    }
    xmlNodePtr item_node = xmlNewChild(parent, NULL, BAD_CAST "STOCKITEM", NULL);
    xmlNewProp(item_node, BAD_CAST "NAME", BAD_CAST item->name);
    char *esc_str = escape_html(item->name);
    xmlNewChild(item_node, NULL, BAD_CAST "NAME", BAD_CAST esc_str);
    free(esc_str);
    xmlNodePtr gst_applicable = xmlNewChild(item_node, NULL, BAD_CAST "GSTAPPLICABLE", NULL);
    if(item->taxable){
      xmlNodeSetContent(gst_applicable, BAD_CAST "Taxable");
    } else {
      xmlNodeSetContent(gst_applicable, BAD_CAST "Exempt");
    }
    xmlNodePtr gst_type = xmlNewChild(item_node, NULL, BAD_CAST "GSTTYPEOFSUPPLY", NULL);
    if(item->type == PRODUCT){
      xmlNodeSetContent(gst_type, BAD_CAST "Goods");
    } else {
      xmlNodeSetContent(gst_type, BAD_CAST "Services");
    }
    if(item->sku){
      xmlNodePtr base_units = xmlNewChild(item_node, NULL, BAD_CAST "BASEUNITS", NULL);
      xmlNodeSetContent(base_units , BAD_CAST item->sku);
      if(item->inventory){
        sprintf(tmp_str, "%0.2f %s", item->inventory->cur_stock, item->sku);
        xmlNodePtr opening_bal = xmlNewChild(item_node, NULL, BAD_CAST "OPENINGBALANCE", NULL);
        xmlNodeSetContent(opening_bal, BAD_CAST tmp_str);
        sprintf(tmp_str,"%0.2f", -item->inventory->cur_stock * item->inventory->stock_rate);
        xmlNodePtr opening_val = xmlNewChild(item_node, NULL, BAD_CAST "OPENINGVALUE", NULL);
        xmlNodeSetContent(opening_val, BAD_CAST tmp_str);
        sprintf(tmp_str,"%0.2f/%s", item->inventory->stock_rate, item->sku);
        xmlNodePtr opening_rate = xmlNewChild(item_node, NULL, BAD_CAST "OPENINGRATE", NULL);
        xmlNodeSetContent(opening_rate, BAD_CAST tmp_str);
      }
      if(item->sales_rate){
        xmlNodePtr cost_list = xmlNewChild(item_node, NULL, BAD_CAST "STANDARDCOSTLIST.LIST", NULL);
        xmlNewChild(cost_list, NULL, BAD_CAST "DATE", BAD_CAST "20010101");
        sprintf(tmp_str, "%0.2f/%s", item->sales_rate, item->sku);
        xmlNewChild(cost_list, NULL, BAD_CAST "RATE", BAD_CAST tmp_str);
      }
      if(item->purchase_rate){
        xmlNodePtr cost_list = xmlNewChild(item_node, NULL, BAD_CAST "STANDARDPRICELIST.LIST", NULL);
        xmlNewChild(cost_list, NULL, BAD_CAST "DATE", BAD_CAST "20010101");
        sprintf(tmp_str, "%0.2f/%s", item->purchase_rate, item->sku);
        xmlNewChild(cost_list, NULL, BAD_CAST "RATE", BAD_CAST tmp_str);
      }
    }
    if(item->desc){
      esc_str = escape_html(item->desc);
      xmlNewChild(item_node, NULL, BAD_CAST "DESCRIPTION", BAD_CAST esc_str);
      free(esc_str);
    }
    if(item->sales_desc){
      esc_str = escape_html(item->sales_desc);
      xmlNewChild(item_node, NULL, BAD_CAST "CATEGORY", BAD_CAST esc_str);
      free(esc_str);
    }
    xmlNodePtr gst_details = xmlNewChild(item_node, NULL, BAD_CAST "GSTDETAILS.LIST", NULL);
    xmlNewChild(gst_details, NULL, BAD_CAST "APPLICABLEFROM", BAD_CAST "20010101");
    //xmlNodePtr applicable_from = xmlNewChild(gst_details, NULL, BAD_CAST "APPLICABLEFROM", NULL);
    //memset(tmp_str, 0, sizeof(tmp_str));
    //make_date(tmp_str, time(0)); 
    //xmlNodeSetContent(applicable_from, BAD_CAST tmp_str);
    xmlNodePtr calc_type = xmlNewChild(gst_details, NULL, BAD_CAST "CALCULATIONTYPE", NULL);
    xmlNodeSetContent(calc_type, BAD_CAST "On Value");
    //xmlNewChild(gst_details, NULL, BAD_CAST "ISREVERSECHARGEAPPLICABLE", BAD_CAST "No");
    //xmlNewChild(gst_details, NULL, BAD_CAST "ISNONGSTGOODS", BAD_CAST "No");
    //xmlNewChild(gst_details, NULL, BAD_CAST "GSTINELIGIBLEITC", BAD_CAST "No");
    //xmlNewChild(gst_details, NULL, BAD_CAST "INCLUDEEXPFORSLABCALC", BAD_CAST "No");

    if(item->taxable && item->inter_state_tax_id && item->intra_state_tax_id){
      xmlNodePtr taxable = xmlNewChild(gst_details, NULL, BAD_CAST "TAXABILITY", NULL);
      xmlNodeSetContent(taxable, BAD_CAST "Taxable");
      if(item->hsn_sac){
        xmlNodePtr hsn = xmlNewChild(gst_details, NULL, BAD_CAST "HSNCODE", NULL);
        xmlNodeSetContent(hsn, BAD_CAST item->hsn_sac);
      }
      xmlNodePtr state_details= xmlNewChild(gst_details, NULL, BAD_CAST "STATEWISEDETAILS.LIST", NULL);
      xmlNodePtr state_name = xmlNewChild(state_details, NULL, BAD_CAST "STATENAME", NULL);
      const char* content = "&#4; Any";
      xmlChar* escaped_content = xmlEncodeSpecialChars(NULL, BAD_CAST content);
      xmlNodeSetContent(state_name, escaped_content);
      xmlFree(escaped_content);

      float igst_tax = 0;
      float cgst_tax = 0;
      float sgst_tax = 0;
      float cess_tax = 0;
      int *tax_ids = get_related_tax_ids(item->inter_state_tax_id);
      if(tax_ids){
        for(int i = 0; *(tax_ids + i); i++){
          int tax_id = *(tax_ids + i);
          taxDetails *tax_detail = get_tax_details(tax_id);
          if(tax_detail){
            if(tax_id > 0 && tax_id < 6){
              igst_tax += tax_detail->rate;
            } else {
              cess_tax += tax_detail->rate;
            }
            delete tax_detail;
          }
        }
        free(tax_ids);
      }
      tax_ids = get_related_tax_ids(item->intra_state_tax_id);
      if(tax_ids){
        for(int i = 0; *(tax_ids + i); i++){
          int tax_id = *(tax_ids + i);
          taxDetails *tax_detail = get_tax_details(tax_id);
          if(tax_detail){
            if(tax_id > 5 && tax_id < 11){
              cgst_tax += tax_detail->rate;
            }
            else if(tax_id > 10 && tax_id < 16){
              sgst_tax += tax_detail->rate;
            } else {
              cess_tax += tax_detail->rate;
            } 
            delete tax_detail;
          }
        }
        free(tax_ids);
      } 
      xmlNodePtr igst_details = xmlNewChild(state_details, NULL, BAD_CAST "RATEDETAILS.LIST", NULL);
      xmlNodePtr igst_head = 
                      xmlNewChild(igst_details, NULL, BAD_CAST "GSTRATEDUTYHEAD", NULL);
      xmlNodeSetContent(igst_head, BAD_CAST "Integrated Tax");
      xmlNodePtr igst_valuation = 
                      xmlNewChild(igst_details, NULL, BAD_CAST "GSTRATEVALUATIONTYPE", NULL);
      xmlNodeSetContent(igst_valuation, BAD_CAST "Based on Value");
      sprintf(tmp_str, "%0.2f", igst_tax);
      xmlNodePtr igst_rate= 
                      xmlNewChild(igst_details, NULL, BAD_CAST "GSTRATE", NULL);
      xmlNodeSetContent(igst_rate, BAD_CAST tmp_str);

      xmlNodePtr cgst_details = xmlNewChild(state_details, NULL, BAD_CAST "RATEDETAILS.LIST", NULL);
      xmlNodePtr cgst_head = 
                      xmlNewChild(cgst_details, NULL, BAD_CAST "GSTRATEDUTYHEAD", NULL);
      xmlNodeSetContent(cgst_head, BAD_CAST "Central Tax");
      xmlNodePtr cgst_valuation = 
                      xmlNewChild(cgst_details, NULL, BAD_CAST "GSTRATEVALUATIONTYPE", NULL);
      xmlNodeSetContent(cgst_valuation, BAD_CAST "Based on Value");
      sprintf(tmp_str, "%0.2f", cgst_tax);
      xmlNodePtr cgst_rate= 
                      xmlNewChild(cgst_details, NULL, BAD_CAST "GSTRATE", NULL);
      xmlNodeSetContent(cgst_rate, BAD_CAST tmp_str);

      xmlNodePtr sgst_details = xmlNewChild(state_details, NULL, BAD_CAST "RATEDETAILS.LIST", NULL);
      xmlNodePtr sgst_head = 
                      xmlNewChild(sgst_details, NULL, BAD_CAST "GSTRATEDUTYHEAD", NULL);
      xmlNodeSetContent(sgst_head, BAD_CAST "State Tax");
      xmlNodePtr sgst_valuation = 
                      xmlNewChild(sgst_details, NULL, BAD_CAST "GSTRATEVALUATIONTYPE", NULL);
      xmlNodeSetContent(sgst_valuation, BAD_CAST "Based on Value");
      sprintf(tmp_str, "%0.2f", sgst_tax);
      xmlNodePtr sgst_rate= 
                      xmlNewChild(sgst_details, NULL, BAD_CAST "GSTRATE", NULL);
      xmlNodeSetContent(sgst_rate, BAD_CAST tmp_str);

      xmlNodePtr cess_details = xmlNewChild(state_details, NULL, BAD_CAST "RATEDETAILS.LIST", NULL);
      xmlNodePtr cess_head = 
                      xmlNewChild(cess_details, NULL, BAD_CAST "GSTRATEDUTYHEAD", NULL);
      xmlNodeSetContent(cess_head, BAD_CAST "Cess");
      xmlNodePtr cess_valuation = 
                      xmlNewChild(cess_details, NULL, BAD_CAST "GSTRATEVALUATIONTYPE", NULL);
      xmlNodeSetContent(cess_valuation, BAD_CAST "Based on Value");
      sprintf(tmp_str, "%0.2f", cess_tax);
      xmlNodePtr cess_rate= 
                      xmlNewChild(cess_details, NULL, BAD_CAST "GSTRATE", NULL);
      xmlNodeSetContent(cess_rate, BAD_CAST tmp_str);

    } else {
      xmlNodePtr taxable = xmlNewChild(gst_details, NULL, BAD_CAST "TAXABILITY", NULL);
      xmlNodeSetContent(taxable, BAD_CAST "Exempt");
    }
    delete item;
    ret = true;
  }
  delete item_list;
  return ret;
}

bool fill_contact_objects(xmlNodePtr parent, MYSQL *mysql_handle, 
                                            int from_date, int to_date, bool &error){
  bool ret = false;
  do {
    char select_stmt[1500];
    sprintf(select_stmt, "SELECT * FROM(SELECT a.id AS main_id, a.display_name, a.company_name, "
      "b.addr_line_1, b.addr_line_2, b.pincode, b.city, b.state, b.country,a.currency,a.tax_exempted, "
      "a.gst_treatment, a.pan, e.numdays, c.gstin, a.type FROM contacts AS a LEFT JOIN (SELECT contact_id, "
      "MIN(id) AS min_id, addr_line_1, addr_line_2, city, pincode, state, country FROM addresses GROUP "
      "BY contact_id ) AS b ON b.contact_id = a.id LEFT JOIN(SELECT contact_id, MIN(id) AS min_id, "
      "gstin, place_of_supply FROM tax_info GROUP BY contact_id) AS c ON c.contact_id = a.id LEFT JOIN "
      "payment_terms AS e ON (e.id = a.payment_terms) INNER JOIN "
      "(SELECT customer_id, date, status FROM invoices WHERE status NOT IN(0,1,9) AND "
      "date > %d AND date < %d UNION SELECT customer_id, date, status FROM bills WHERE status NOT IN "
      "(0,1,9) AND date > %d AND date < %d UNION SELECT customer_id,date,status FROM debit_notes WHERE "
      "status NOT IN(0,1,9) AND date > %d AND date < %d UNION SELECT customer_id, date, status FROM "
      "credit_notes WHERE status NOT IN(0,1,9) AND date > %d AND date < %d UNION SELECT customer_id, "
      "date, status FROM vendor_credit_notes WHERE status NOT IN(0,1,9) AND date > %d AND date < %d "
      "UNION SELECT customer_id, date, status FROM vendor_credits WHERE status NOT IN(0,1,9) AND date > "
      "%d AND date < %d UNION SELECT contact_id,date,1 FROM payments_received WHERE date > %d AND date "
      "< %d UNION SELECT contact_id, date, 1 FROM payments_made WHERE date > %d AND date < %d) AS d ON "
      "d.customer_id = a.id) AS a GROUP BY main_id", from_date, to_date, from_date, to_date, 
        from_date, to_date, from_date, to_date, from_date, to_date, from_date, to_date, from_date, 
        to_date, from_date, to_date);
    int query_length = strlen(select_stmt);
    if(mysql_real_query(mysql_handle, select_stmt, query_length)){
      ERROR_LOG("Failed in executing query - (%s)\n",select_stmt);
      ERROR_LOG(mysql_error(mysql_handle));
      error = true;
      break;
    }
    MYSQL_RES *result = mysql_store_result(mysql_handle);
    if(!result){ 
      ERROR_LOG("Error storing result of query - (%s)\n",select_stmt);
      error = true;
      break;
    }
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result))) {
      char *esc_str = escape_html(row[1]);
      xmlNodePtr contact_node = xmlNewChild(parent, NULL, BAD_CAST "LEDGER", NULL);
      xmlNewProp(contact_node, BAD_CAST "NAME", BAD_CAST esc_str);
      free(esc_str);
      xmlNodePtr address_list = xmlNewChild(contact_node, NULL, BAD_CAST "ADDRESS.LIST", NULL);
      xmlSetProp(address_list, BAD_CAST "TYPE", BAD_CAST "String");
      if(row[2] && *row[2]){
        esc_str = escape_html(row[2]);
        xmlNewChild(address_list, NULL, BAD_CAST "ADDRESS", BAD_CAST esc_str);
        free(esc_str);
      }
      if(row[3] && *row[3]){
        esc_str = escape_html(row[3]);
        xmlNewChild(address_list, NULL, BAD_CAST "ADDRESS", BAD_CAST esc_str);
        free(esc_str);
      }
      if(row[4] && *row[4]){
        esc_str = escape_html(row[4]);
        xmlNewChild(address_list, NULL, BAD_CAST "ADDRESS", BAD_CAST esc_str);
        free(esc_str);
      }
      char tmp_str[200];
      bool started = false;
      memset(tmp_str, 0, sizeof(tmp_str));
      if(row[5]){
        snprintf(tmp_str, 199, "Pincode: %s", row[5]);
        xmlNewChild(contact_node, NULL,BAD_CAST "PINCODE" , BAD_CAST row[5]);
        started = true;
      }
      if(row[6]){
        if(started){
          strncat(tmp_str, ", ", 199);
        }
        strncat(tmp_str, row[6], 199);
        started = true;
      } 
      if(row[7]){
        if(started){
          strncat(tmp_str, ", ", 199);
        }
        const char *state_name = get_state_name(atoi(row[7]));
        if(state_name){
          xmlNewChild(contact_node, NULL,BAD_CAST "LEDSTATENAME" , BAD_CAST state_name);
          strncat(tmp_str, state_name ,199);
          started = true;
        }
      } 
      if(row[8]){
        if(started){
          strncat(tmp_str, ", ", 199);
        }
        const char *country_name = get_country_name(atoi(row[8]));
        if(country_name){ 
          xmlNewChild(contact_node, NULL,BAD_CAST "COUNTRYNAME" , BAD_CAST country_name);
          strncat(tmp_str, country_name,199);
        }
        started = true;
      } 
      if(started){
        esc_str = escape_html(tmp_str);
        xmlNewChild(address_list, NULL, BAD_CAST "ADDRESS", BAD_CAST esc_str);
        free(esc_str);
      }
      if(row[9]){
        int currency_id = atoi(row[9]);
        const char* symbol = get_currency_symbol(currency_id);
        if(symbol){
          char *currency_name = html_entity_decode(symbol);
          xmlNewChild(contact_node, NULL, BAD_CAST "CURRENCYNAME", BAD_CAST currency_name );
          free(currency_name);
        }
      }
      if(row[10] && (atoi(row[10]) == 1)){
        xmlNewChild(contact_node, NULL, BAD_CAST "ISEXEMPTED", BAD_CAST "Yes");
      } else {
        if(row[11]){
          int gst_treatment = atoi(row[11]);
          const char *gst_str = NULL;
          switch(gst_treatment){
            case REGISTERED:
              gst_str = "Regular";
              break;
            case REGISTERED_COMPOSITION:
              gst_str = "Composition";
              break;
            case UNREGISTERED:
              gst_str = "Unregistered";
              break;
            case CONSUMER:
              gst_str = "Consumer";
              break;
          }
          if(gst_str){
            xmlNewChild(contact_node, NULL, BAD_CAST "GSTREGISTRATIONTYPE", BAD_CAST gst_str);
          }
          if(gst_treatment == SEZ){
            xmlNewChild(contact_node, NULL, BAD_CAST "ISSEZPARTY", BAD_CAST "Yes");
          }
        }
      }
      if(row[12] && *row[12]){
        xmlNewChild(contact_node, NULL, BAD_CAST "INCOMETAXNUMBER", BAD_CAST row[12]);
      }
      if(row[13] && (atoi(row[13]) > 0)){
        sprintf(tmp_str, "%d Days", atoi(row[13]));
        xmlNewChild(contact_node, NULL, BAD_CAST "BILLCREDITPERIOD", BAD_CAST tmp_str);
      }

      if(row[14] && *row[14]){
        xmlNewChild(contact_node, NULL, BAD_CAST "PARTYGSTIN", BAD_CAST row[14]);
      }
      int type = atoi(row[15]);
      esc_str = escape_html(row[1]);
      if(type == VENDOR){
        xmlNewChild(contact_node, NULL, BAD_CAST "PARENT", BAD_CAST "Sundry Creditors");
        xmlNewChild(contact_node, NULL,BAD_CAST "NAME" , BAD_CAST esc_str);
      } else if(type == CUSTOMER){
        xmlNewChild(contact_node, NULL, BAD_CAST "PARENT", BAD_CAST "Sundry Debtors");
        xmlNewChild(contact_node, NULL,BAD_CAST "NAME" , BAD_CAST esc_str);
      } else {
        xmlNodePtr contact_copy = xmlCopyNode(contact_node, 1);
        xmlAddChild(parent, contact_copy);
        sprintf(tmp_str, "%s (Supplier)", esc_str);
        xmlSetProp(contact_node, BAD_CAST "NAME", BAD_CAST tmp_str);
        xmlNodePtr sup_name = xmlNewChild(contact_node, NULL,BAD_CAST "NAME" , NULL);
        xmlNodeSetContent(sup_name, BAD_CAST tmp_str);
        xmlNewChild(contact_node, NULL, BAD_CAST "PARENT", BAD_CAST "Sundry Creditors");
        sprintf(tmp_str, "%s (Customer)", esc_str);
        xmlSetProp(contact_copy, BAD_CAST "NAME", BAD_CAST tmp_str);
        xmlNodePtr cust_name = xmlNewChild(contact_copy, NULL,BAD_CAST "NAME" , NULL);
        xmlNodeSetContent(cust_name, BAD_CAST tmp_str);
        xmlNewChild(contact_copy, NULL, BAD_CAST "PARENT", BAD_CAST "Sundry Debtors");
      }
      free(esc_str);
      ret = true;
    }
    mysql_free_result(result);
  } while(0);
  return ret;
}

struct inAccount{
  int id;
  char *name;
  int category;
  inAccount():id(0), name(NULL), category(0){}
  ~inAccount(){
    free(name);
  }
};

std::vector<inAccount *>* get_used_accounts(MYSQL *mysql_handle, int from_date, int to_date,
                                                std::vector<int>* filter_accounts){
  std::vector<inAccount*> * ret = NULL;
  do {
    char select_stmt[500];
    sprintf(select_stmt, "SELECT a.id, a.name, a.category FROM accounts AS a INNER JOIN "
      "transaction_entries AS c ON (c.account_id = a.id) INNER JOIN transactions AS d ON "
      "(c.transaction_id = d.id) WHERE d.date >= %d AND d.date < %d AND d.type NOT IN "
      "(1,4,6,7,8,9,13,18,19,20,21,22,23,24,25,26,27,29,31,32) AND a.category NOT IN "
      "(5,6,8,18,20,21) AND a.id NOT IN(", from_date, to_date);
    char tmp_str[40];
    bool started = false;
    for(auto account_id : *filter_accounts){
      if(started){
        strcat(select_stmt, ",");
      }
      sprintf(tmp_str, "%d", account_id);
      strcat(select_stmt, tmp_str);
      started = true;
    }
    strcat(select_stmt, ") GROUP BY a.id");
    //printf(">> %s\n", select_stmt);
    if(mysql_query(mysql_handle, select_stmt)){
      ERROR_LOG("Failed in executing query - (%s)\n",select_stmt);
      ERROR_LOG(mysql_error(mysql_handle));
      break;
    }
    MYSQL_RES *result = mysql_store_result(mysql_handle);
    if(!result){ 
      ERROR_LOG("Error storing result of query - (%s)\n",select_stmt);
      break;
    }
    ret = new std::vector<inAccount*>();
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result))) {
      if(row[0] && row[1] && row[2]){
        inAccount* tmp_accnt = new inAccount();
        tmp_accnt->id = atoi(row[0]);
        tmp_accnt->name = strdup(row[1]);
        tmp_accnt->category = atoi(row[2]);
        ret->push_back(tmp_accnt);
      }
    } 
    mysql_free_result(result);
  } while(0);
  return ret;    
}

std::vector<inAccount *>* get_used_accounts_2(MYSQL *mysql_handle, int from_date, int to_date, 
                        std::vector<int>* in_accounts){
  std::vector<inAccount*> * ret = NULL;
  do {
    char select_stmt[500];
    if(!in_accounts || in_accounts->size() <= 0){
      break;
    }
    sprintf(select_stmt, "SELECT a.id, a.name, a.category FROM accounts AS a INNER JOIN "
      "transaction_entries AS c ON (c.account_id = a.id) INNER JOIN transactions AS d ON "
      "(c.transaction_id = d.id) WHERE d.date >= %d AND d.date < %d AND a.category NOT IN "
      "(5,6,8,20,21,18) AND a.id IN (", from_date, to_date);

    char tmp_str[40];
    bool started = false;
    for(auto account_id : *in_accounts){
      if(started){
        strcat(select_stmt, ",");
      }
      sprintf(tmp_str, "%d", account_id);
      strcat(select_stmt, tmp_str);
      started = true;
    }
    strcat(select_stmt, ") GROUP BY a.id");    
    if(mysql_query(mysql_handle, select_stmt)){
      ERROR_LOG("Failed in executing query - (%s)\n",select_stmt);
      ERROR_LOG(mysql_error(mysql_handle));
      break;
    }
    MYSQL_RES *result = mysql_store_result(mysql_handle);
    if(!result){ 
      ERROR_LOG("Error storing result of query - (%s)\n",select_stmt);
      break;
    }
    ret = new std::vector<inAccount*>();
    MYSQL_ROW row;
    while((row = mysql_fetch_row(result))) {
      if(row[0] && row[1] && row[2]){
        inAccount* tmp_accnt = new inAccount();
        tmp_accnt->id = atoi(row[0]);
        tmp_accnt->name = strdup(row[1]);
        tmp_accnt->category = atoi(row[2]);
        ret->push_back(tmp_accnt);
      }
    } 
    mysql_free_result(result);
  } while(0);
  return ret;    
}

bool fill_other_accounts(xmlNodePtr parent, MYSQL *mysql_handle, int from_date, 
                                                            int to_date, bool &error){
  bool ret = true;
  char *currency_name = NULL; 
  do {
    const char *input_taxes[] = {"Input IGST", "Input CGST", "Input SGST", "Input Cess"};
    const char *output_taxes[] = {"Output IGST", "Output CGST", "Output SGST", "Output Cess"};
    const char *gst_tax_types[] = {"Integrated Tax", "Central Tax", "State Tax", "Cess"};
    const char* symbol = get_currency_symbol(get_base_currency());
    if(symbol){
        currency_name = html_entity_decode(symbol);
    }
    for(int i = 0; i < 4; i++){
      xmlNodePtr account_node = xmlNewChild(parent, NULL, BAD_CAST "LEDGER", NULL);
      xmlNewProp(account_node, BAD_CAST "NAME", BAD_CAST input_taxes[i]);
      xmlNodePtr name = xmlNewChild(account_node, NULL,BAD_CAST "NAME" , NULL);
      xmlNodeSetContent(name, BAD_CAST input_taxes[i]);
      if(currency_name){
        xmlNewChild(account_node, NULL, BAD_CAST "CURRENCYNAME", BAD_CAST currency_name );
      }
      xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Duties &amp; Taxes");
      xmlNewChild(account_node, NULL, BAD_CAST "TAXTYPE", BAD_CAST "GST");
      xmlNewChild(account_node, NULL, BAD_CAST "GSTDUTYHEAD", BAD_CAST gst_tax_types[i]);
    }
    for(int i = 0; i < 4; i++){
      xmlNodePtr account_node = xmlNewChild(parent, NULL, BAD_CAST "LEDGER", NULL);
      xmlNewProp(account_node, BAD_CAST "NAME", BAD_CAST output_taxes[i]);
      xmlNodePtr name = xmlNewChild(account_node, NULL,BAD_CAST "NAME" , NULL);
      xmlNodeSetContent(name, BAD_CAST output_taxes[i]);
      if(currency_name){
        xmlNewChild(account_node, NULL, BAD_CAST "CURRENCYNAME", BAD_CAST currency_name );
      }
      xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Duties &amp; Taxes");
      xmlNewChild(account_node, NULL, BAD_CAST "TAXTYPE", BAD_CAST "GST");
      xmlNewChild(account_node, NULL, BAD_CAST "GSTDUTYHEAD", BAD_CAST gst_tax_types[i]);
    }
    std::vector<int> check_accounts;
    check_accounts.push_back(24);
    check_accounts.push_back(67);
    check_accounts.push_back(68);
    check_accounts.push_back(69);
    std::vector<inAccount *> *open_accounts = get_used_accounts_2(mysql_handle, from_date, to_date, 
                        &check_accounts);
    if(open_accounts){
      for(auto account : *open_accounts){
        xmlNodePtr account_node = xmlNewChild(parent, NULL, BAD_CAST "LEDGER", NULL);
        xmlNewProp(account_node, BAD_CAST "NAME", BAD_CAST account->name);
        xmlNodePtr name = xmlNewChild(account_node, NULL,BAD_CAST "NAME" , NULL);
        xmlNodeSetContent(name, BAD_CAST account->name);
        if(currency_name){
          xmlNewChild(account_node, NULL, BAD_CAST "CURRENCYNAME", BAD_CAST currency_name );
        }
        switch(account->id){
          case 24:
            xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Duties &amp; Taxes");
            xmlNewChild(account_node, NULL, BAD_CAST "TAXTYPE", BAD_CAST "TDS");
            break;
          case 67:
            xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Current Assets");
            xmlNewChild(account_node, NULL, BAD_CAST "TAXTYPE", BAD_CAST "TDS");
            break;
          case 68:
            xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Duties &amp; Taxes");
            xmlNewChild(account_node, NULL, BAD_CAST "TAXTYPE", BAD_CAST "TCS");
            break;
          case 69:
            xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Current Assets");
            xmlNewChild(account_node, NULL, BAD_CAST "TAXTYPE", BAD_CAST "TCS");
            break;
        }
        delete account;
      }
    }
    xmlNodePtr account_node = xmlNewChild(parent, NULL, BAD_CAST "LEDGER", NULL);
    xmlNewProp(account_node, BAD_CAST "NAME", BAD_CAST "Sales");
    xmlNodePtr name = xmlNewChild(account_node, NULL,BAD_CAST "NAME" , NULL);
    xmlNodeSetContent(name, BAD_CAST "Sales");
    if(currency_name){
      xmlNewChild(account_node, NULL, BAD_CAST "CURRENCYNAME", BAD_CAST currency_name );
    }
    xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Sales Accounts");
    xmlNewChild(account_node, NULL, BAD_CAST "TAXTYPE", BAD_CAST "Others");

    account_node = xmlNewChild(parent, NULL, BAD_CAST "LEDGER", NULL);
    xmlNewProp(account_node, BAD_CAST "NAME", BAD_CAST "Purchase");
    name = xmlNewChild(account_node, NULL,BAD_CAST "NAME" , NULL);
    xmlNodeSetContent(name, BAD_CAST "Purchase");
    if(currency_name){
      xmlNewChild(account_node, NULL, BAD_CAST "CURRENCYNAME", BAD_CAST currency_name );
    }
    xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Purchase Accounts");
    xmlNewChild(account_node, NULL, BAD_CAST "TAXTYPE", BAD_CAST "Others");


    account_node = xmlNewChild(parent, NULL, BAD_CAST "LEDGER", NULL);
    xmlNewProp(account_node, BAD_CAST "NAME", BAD_CAST "Discount");
    name = xmlNewChild(account_node, NULL,BAD_CAST "NAME" , NULL);
    xmlNodeSetContent(name, BAD_CAST "Discount");
    if(currency_name){
      xmlNewChild(account_node, NULL, BAD_CAST "CURRENCYNAME", BAD_CAST currency_name );
    }
    xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Income (Direct)");
    xmlNewChild(account_node, NULL, BAD_CAST "TAXTYPE", BAD_CAST "Others");
    xmlNewChild(account_node, NULL, BAD_CAST "AFFECTSSTOCK", BAD_CAST "No");
    xmlNewChild(account_node, NULL, BAD_CAST "TDSAPPLICABLE", BAD_CAST "&#4; Not Applicable");
    xmlNewChild(account_node, NULL, BAD_CAST "TCSAPPLICABLE", BAD_CAST "&#4; Not Applicable");
    xmlNewChild(account_node, NULL, BAD_CAST "GSTAPPLICABLE", BAD_CAST "&#4; Not Applicable");
    xmlNodePtr vat_dealer_nature_list = xmlNewChild(account_node, NULL, BAD_CAST "UDF:VATDEALERNATURE.LIST", NULL);
    xmlNewProp(vat_dealer_nature_list, BAD_CAST "DESC", BAD_CAST "VATDealerNature");
    xmlNewProp(vat_dealer_nature_list, BAD_CAST "ISLIST", BAD_CAST "Yes");
    xmlNewProp(vat_dealer_nature_list, BAD_CAST "TYPE", BAD_CAST "String");
    xmlNodePtr vat_child = xmlNewChild(vat_dealer_nature_list, NULL, BAD_CAST "UDF:VATDEALERNATURE", BAD_CAST "Discount");
    xmlNewProp(vat_child, BAD_CAST "DESC", BAD_CAST "VATDealerNature");

    account_node = xmlNewChild(parent, NULL, BAD_CAST "LEDGER", NULL);
    xmlNewProp(account_node, BAD_CAST "NAME", BAD_CAST "General Service");
    name = xmlNewChild(account_node, NULL,BAD_CAST "NAME" , NULL);
    xmlNodeSetContent(name, BAD_CAST "General Service");
    if(currency_name){
      xmlNewChild(account_node, NULL, BAD_CAST "CURRENCYNAME", BAD_CAST currency_name );
    }
    xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Direct Incomes");
    xmlNewChild(account_node, NULL, BAD_CAST "TAXTYPE", BAD_CAST "Others");
    xmlNewChild(account_node, NULL, BAD_CAST "AFFECTSSTOCK", BAD_CAST "No");
    xmlNewChild(account_node, NULL, BAD_CAST "GSTTYPEOFSUPPLY", BAD_CAST "Services");
    xmlNewChild(account_node, NULL, BAD_CAST "TDSAPPLICABLE", BAD_CAST "&#4; Not Applicable");
    xmlNewChild(account_node, NULL, BAD_CAST "TCSAPPLICABLE", BAD_CAST "&#4; Not Applicable");
    xmlNewChild(account_node, NULL, BAD_CAST "GSTAPPLICABLE", BAD_CAST "&#4; Applicable");
    check_accounts.push_back(5);
    check_accounts.push_back(6);
    check_accounts.push_back(7);
    check_accounts.push_back(8);
    check_accounts.push_back(9);
    check_accounts.push_back(18);
    check_accounts.push_back(19);
    check_accounts.push_back(20);
    check_accounts.push_back(21);
    check_accounts.push_back(22);
    check_accounts.push_back(31);
    check_accounts.push_back(34);
    check_accounts.push_back(38);
    check_accounts.push_back(59);
    check_accounts.push_back(62);
    check_accounts.push_back(63);
    check_accounts.push_back(64);
    delete open_accounts;
    open_accounts = get_used_accounts(mysql_handle, from_date, to_date, 
                        &check_accounts);
    if(open_accounts){
      for(auto account : *open_accounts){
        xmlNodePtr account_node = xmlNewChild(parent, NULL, BAD_CAST "LEDGER", NULL);
        xmlNewProp(account_node, BAD_CAST "NAME", BAD_CAST account->name);
        xmlNodePtr name = xmlNewChild(account_node, NULL,BAD_CAST "NAME" , NULL);
        xmlNewChild(account_node, NULL, BAD_CAST "TAXTYPE", BAD_CAST "Others");
        xmlNodeSetContent(name, BAD_CAST account->name);
        if(currency_name){
          xmlNewChild(account_node, NULL, BAD_CAST "CURRENCYNAME", BAD_CAST currency_name );
        }
        switch(account->category){
          case 1:
          case 2:
          case 9:
            xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Current Assets");
            break;
          case 7:
            xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Fixed Assets");
            break;
          case 10:
          case 11:
          case 12:
            xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Current Liabilities");
            break;
          case 13:
            xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Loans (Liability)");
            break;
          case 15:
            xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Income (Direct)");
            break;
          case 16:
            xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Income (Indirect)");
            break;
          case 17:
            xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Expenses (Indirect)");
            break;
          case 19:
            xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Expenses (Direct)");
            break;
          case 14:
            xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Investments");
            break;
        }
        if(account->id == 33){
          xmlNewChild(account_node, NULL, BAD_CAST "AFFECTSSTOCK", BAD_CAST "No");
          xmlNewChild(account_node, NULL, BAD_CAST "GSTTYPEOFSUPPLY", BAD_CAST "Services");
          xmlNewChild(account_node, NULL, BAD_CAST "TDSAPPLICABLE", BAD_CAST "&#4; Not Applicable");
          xmlNewChild(account_node, NULL, BAD_CAST "TCSAPPLICABLE", BAD_CAST "&#4; Not Applicable");
          xmlNewChild(account_node, NULL, BAD_CAST "GSTAPPLICABLE", BAD_CAST "&#4; Applicable");
        }
        delete account;
      }
      delete open_accounts;
    }
    std::vector<BankDetails *> *bank_accounts = get_all_bank_details(mysql_handle);
    if(bank_accounts){
      for(auto bank_account : *bank_accounts){
        xmlNodePtr account_node = xmlNewChild(parent, NULL, BAD_CAST "LEDGER", NULL);
        xmlNewProp(account_node, BAD_CAST "NAME", BAD_CAST bank_account->upi_id);
        xmlNodePtr name = xmlNewChild(account_node, NULL,BAD_CAST "NAME" , NULL);
        xmlNewChild(account_node, NULL, BAD_CAST "TAXTYPE", BAD_CAST "Others");
        xmlNodeSetContent(name, BAD_CAST bank_account->upi_id);
        xmlNewChild(account_node, NULL, BAD_CAST "MAILINGNAME", BAD_CAST bank_account->upi_id);
        xmlNewChild(account_node, NULL, BAD_CAST "PARENT", BAD_CAST "Bank Accounts");
        if(bank_account->ifsc_code){
          xmlNewChild(account_node, NULL, BAD_CAST "IFSCODE", BAD_CAST bank_account->ifsc_code);
        }
        if(bank_account->account_number){
          xmlNewChild(account_node, NULL, BAD_CAST "BANKDETAILS", BAD_CAST bank_account->account_number);
        }
        char *bank_currency = NULL;
        if(bank_account->currency_id){
          const char* symbol = get_currency_symbol(bank_account->currency_id);
          if(symbol){
              bank_currency = html_entity_decode(symbol);
          }
          if(bank_currency){
            xmlNewChild(account_node, NULL, BAD_CAST "CURRENCYNAME", BAD_CAST bank_currency);
          }
        } else if(currency_name){
          xmlNewChild(account_node, NULL, BAD_CAST "CURRENCYNAME", BAD_CAST currency_name );
        }
        free(bank_currency);
        delete bank_account;
      }
      delete bank_accounts;
    }
    char select_stmt[100];
    sprintf(select_stmt, "SELECT COUNT(*) FROM transactions WHERE date >= %d AND date < %d AND "
      "type = %d", from_date, to_date, POS_BILL);
    if(mysql_query(mysql_handle, select_stmt)){
      ERROR_LOG("Failed in executing query - (%s)\n",select_stmt);
      ERROR_LOG(mysql_error(mysql_handle));
      ret = false;
      break;
    }
    MYSQL_RES *result = mysql_store_result(mysql_handle);
    if(!result){ 
      ERROR_LOG("Error storing result of query - (%s)\n",select_stmt);
      ret = false;
      break;
    }
    MYSQL_ROW row= mysql_fetch_row(result);
    if(row && row[0] && atoi(row[0]) > 0){
      xmlNodePtr contact_node = xmlNewChild(parent, NULL, BAD_CAST "LEDGER", NULL);
      xmlNewProp(contact_node, BAD_CAST "NAME", BAD_CAST "Walk-in Customer");
      const char* const_ptr = get_state_name(get_org_state());
      if(const_ptr){
        xmlNewChild(contact_node, NULL,BAD_CAST "LEDSTATENAME" , BAD_CAST const_ptr);
      }
      const_ptr = get_country_name(get_org_country());
      if(const_ptr){
        xmlNewChild(contact_node, NULL,BAD_CAST "COUNTRYNAME" , BAD_CAST const_ptr);
      }
      const_ptr = get_currency_symbol(get_base_currency());
      if(const_ptr){
        char *currency_name = html_entity_decode(const_ptr);
        xmlNewChild(contact_node, NULL, BAD_CAST "CURRENCYNAME", BAD_CAST currency_name );
        free(currency_name);
      }
      xmlNewChild(contact_node, NULL, BAD_CAST "GSTREGISTRATIONTYPE", BAD_CAST "Consumer");
      xmlNewChild(contact_node, NULL, BAD_CAST "PARENT", BAD_CAST "Sundry Debtors");
      xmlNewChild(contact_node, NULL,BAD_CAST "NAME" , BAD_CAST "Walk-in Customer");
    }
    mysql_free_result(result);
  } while(0);
  free(currency_name);
  return ret;
}

bool fill_tax_objects(xmlNodePtr parent, MYSQL *mysql_handle,std::vector<TaxObject *>* tax_obj_list, 
        int thread_id, bool &error, template_type_t type = INVOICE){
  bool is_sales = true;
  bool is_outward = true;
  const char* type_name = "Sales";
 
  switch(type){
    case CREDIT_NOTE:
      is_sales = true;
      is_outward = false;
      type_name = "Credit Note";
      break;
    case BILLS:
      is_sales = false;
      is_outward = false;
      type_name = "Purchase";
      break;
    case DEBIT_NOTE:
      is_sales = false;
      is_outward = true;
      type_name = "Debit Note";
      break;
    default:;
  } 

  bool ret = false;
  char tmp_str[200];
  do {
    for(auto invoice : *tax_obj_list){
      double net_amount = 0;
      xmlNodePtr sales_node = xmlNewChild(parent, NULL, BAD_CAST "VOUCHER", NULL);
      xmlNewProp(sales_node, BAD_CAST "VCHTYPE", BAD_CAST type_name);
      char *company_name = get_company_name(invoice->cust_id, mysql_handle);
      if(invoice->shipping_address_id || company_name || invoice->shipping_address_id){
        int err = 0;
        xmlNodePtr address_list = xmlNewChild(sales_node, NULL, BAD_CAST "ADDRESS.LIST", NULL);
        xmlNewProp(address_list, BAD_CAST "TYPE", BAD_CAST "String");
        Address shipping_address;
        int addr_id = invoice->shipping_address_id ? invoice->shipping_address_id :
                                                        invoice->billing_address_id;
        if(addr_id && get_address(mysql_handle, addr_id, &shipping_address, err, thread_id)){
          if(!is_empty(shipping_address.attention)){
            char *esc_str = escape_html(shipping_address.attention);
            xmlNewChild(address_list, NULL, BAD_CAST "BASICBUYERADDRESS", BAD_CAST esc_str);
            free(esc_str);
          }
          if(!is_empty(company_name)){
            char *esc_str = escape_html(company_name);
            xmlNewChild(address_list, NULL, BAD_CAST "BASICBUYERADDRESS", BAD_CAST esc_str);
            free(esc_str);
          }
          if(!is_empty(shipping_address.addr_line_1)){
            char *esc_str = escape_html(shipping_address.addr_line_1);
            xmlNewChild(address_list, NULL, BAD_CAST "BASICBUYERADDRESS", BAD_CAST esc_str);
            free(esc_str);
          }
          if(!is_empty(shipping_address.addr_line_2)){
            char *esc_str = escape_html(shipping_address.addr_line_2);
            xmlNewChild(address_list, NULL, BAD_CAST "BASICBUYERADDRESS", BAD_CAST esc_str);
            free(esc_str);
          }
          if(!is_empty(shipping_address.city)){
            char *esc_str = escape_html(shipping_address.city);
            xmlNewChild(address_list, NULL, BAD_CAST "BASICBUYERADDRESS", BAD_CAST esc_str);
            free(esc_str);
          }
          if(!is_empty(shipping_address.pincode)){
            char tmp_str[40];
            snprintf(tmp_str, 39, "Pincode : %s", shipping_address.pincode);
            xmlNewChild(address_list, NULL, BAD_CAST "BASICBUYERADDRESS", BAD_CAST tmp_str);
          }
        } else if(!is_empty(company_name)){
          char *esc_str = escape_html(company_name);
          xmlNewChild(address_list, NULL, BAD_CAST "ADDRESS", BAD_CAST esc_str);
          free(esc_str);
        }
      }
      if(invoice->billing_address_id || company_name){
        int err = 0;
        xmlNodePtr address_list = xmlNewChild(sales_node, NULL, BAD_CAST "BASICBUYERADDRESS.LIST", NULL);
        xmlNewProp(address_list, BAD_CAST "TYPE", BAD_CAST "String");
        Address billing_address;
        if(invoice->billing_address_id && 
            get_address(mysql_handle, invoice->billing_address_id, &billing_address, err, thread_id)){
          if(!is_empty(billing_address.attention)){
            char *esc_str = escape_html(billing_address.attention);
            xmlNewChild(address_list, NULL, BAD_CAST "BASICBUYERADDRESS", BAD_CAST esc_str);
            free(esc_str);
          }
          if(!is_empty(company_name)){
            char *esc_str = escape_html(company_name);
            xmlNewChild(address_list, NULL, BAD_CAST "BASICBUYERADDRESS", BAD_CAST esc_str);
            free(esc_str);
          }
          if(!is_empty(billing_address.addr_line_1)){
            char *esc_str = escape_html(billing_address.addr_line_1);
            xmlNewChild(address_list, NULL, BAD_CAST "BASICBUYERADDRESS", BAD_CAST esc_str);
            free(esc_str);
          }
          if(!is_empty(billing_address.addr_line_2)){
            char *esc_str = escape_html(billing_address.addr_line_2);
            xmlNewChild(address_list, NULL, BAD_CAST "BASICBUYERADDRESS", BAD_CAST esc_str);
            free(esc_str);
          }
          if(!is_empty(billing_address.city)){
            char *esc_str = escape_html(billing_address.city);
            xmlNewChild(address_list, NULL, BAD_CAST "BASICBUYERADDRESS", BAD_CAST esc_str);
            free(esc_str);
          }
          if(!is_empty(billing_address.pincode)){
            char tmp_str[40];
            snprintf(tmp_str, 39, "Pincode : %s", billing_address.pincode);
            xmlNewChild(address_list, NULL, BAD_CAST "BASICBUYERADDRESS", BAD_CAST tmp_str);
          }
        } else if(!is_empty(company_name)){
          char *esc_str = escape_html(company_name);
          xmlNewChild(address_list, NULL, BAD_CAST "BASICBUYERADDRESS", BAD_CAST esc_str);
          free(esc_str);
        }
      }
      free(company_name);
      make_date(tmp_str, invoice->date);
      xmlNewChild(sales_node, NULL, BAD_CAST "DATE", BAD_CAST tmp_str);
      if(!is_outward && invoice->currency_id){
        make_date(tmp_str, invoice->currency_id);
        xmlNewChild(sales_node, NULL, BAD_CAST "REFERENCEDATE", BAD_CAST tmp_str);
      }
      const char *const_ptr = get_state_name(get_org_state());
      if(const_ptr){
        xmlNewChild(sales_node, NULL, BAD_CAST "STATENAME", BAD_CAST const_ptr);
      }
      const_ptr = get_country_name(get_org_country());
      if(const_ptr){
        xmlNewChild(sales_node, NULL, BAD_CAST "COUNTRYOFRESIDENCE", BAD_CAST const_ptr);
      }
      if(!is_empty(invoice->gstin)){
        xmlNewChild(sales_node, NULL, BAD_CAST "PARTYGSTIN", BAD_CAST invoice->gstin);
      } else {
        char *gstin = get_contact_gstin(invoice->cust_id, mysql_handle);
        if(gstin){
          xmlNewChild(sales_node, NULL, BAD_CAST "PARTYGSTIN", BAD_CAST gstin);
          free(gstin);
        }
      }
      if(type == CREDIT_NOTE && invoice->discount_accnt_id){
        const char* reason = "07-Others";
        switch(invoice->discount_accnt_id){
          case SALES_RETURN:
            reason = "01-Sales Return";
            break;
          case POST_SALE_DISCOUNT:
            reason = "02-Post Sale Discount";
            break;
          case DEFICIENCY_IN_SERVICE:
            reason = "03-Deficiency in services";
            break;
          case CORECCTION:
            reason = "04-Correction in Invoice";
            break;
          case CHANGE_IN_POS:
            reason = "05-Change in POS";
            break;
          case FINALIZATION:
            reason = "06-Finalization of Provisional assesment";
            break;
          default:;
        }
        xmlNewChild(sales_node, NULL, BAD_CAST "GSTNATUREOFRETURN", BAD_CAST reason);
      }
      
      if(invoice->place_of_supply){
        const_ptr = get_state_name(invoice->place_of_supply);
        if(const_ptr){
          xmlNewChild(sales_node, NULL, BAD_CAST "PLACEOFSUPPLY", BAD_CAST const_ptr);
        }
      }
      char *tmp_ptr = get_contact_name(invoice->cust_id, mysql_handle);
      if(!is_empty(tmp_ptr)){
        char *esc_str = escape_html(tmp_ptr);
        xmlNewChild(sales_node, NULL, BAD_CAST "PARTYNAME", BAD_CAST esc_str);
        //xmlNewChild(sales_node, NULL, BAD_CAST "PARTYNAME", BAD_CAST "Diva Boutique");
        free(esc_str);
        free(tmp_ptr);
      }
      xmlNewChild(sales_node, NULL, BAD_CAST "VOUCHERTYPENAME", BAD_CAST type_name);
      if(!is_empty(invoice->reference)){
        char *esc_str = escape_html(invoice->reference);
        xmlNewChild(sales_node, NULL, BAD_CAST "REFERENCE", BAD_CAST esc_str);
        free(esc_str);
      }
      if(!is_empty(invoice->number)){
        char *esc_str = escape_html(invoice->number);
        xmlNewChild(sales_node, NULL, BAD_CAST "VOUCHERNUMBER", BAD_CAST invoice->number);
        free(esc_str);
      }
      xmlNewChild(sales_node, NULL, BAD_CAST "ISGSTOVERRIDDEN", BAD_CAST "Yes");
      xmlNewChild(sales_node, NULL, BAD_CAST "PERSISTEDVIEW", BAD_CAST "Invoice Voucher View");
      xmlNewChild(sales_node, NULL, BAD_CAST "ISINVOICE", BAD_CAST "Yes");
      xmlNewChild(sales_node, NULL, BAD_CAST "HASDISCOUNTS", 
                    BAD_CAST (invoice->net_discount ? "Yes" : "No"));
      bool same_state = get_org_state() == invoice->place_of_supply;
      double igst_tax_amount = 0;
      double cgst_tax_amount = 0;
      double sgst_tax_amount = 0;
      double cess_tax_amount = 0;
      double net_tax_amount = 0;
      double discount_factor = 1;
      double total_amount = 0;
      double discount = 0;
      for(auto item : invoice->items){
        double tmp_discount = 0;
        total_amount += item->net_amount;
        if(invoice->discount_type == DISCOUNT_ITEM ||
            invoice->discount_type == DISCOUNT_ITEM_INCLUDE){
          if(item->discount_is_percentage == 1){
            tmp_discount = item->rate * item->quantity * item->discount/100;
          } else {
            tmp_discount = item->discount;
            if(invoice->discount_type == DISCOUNT_ITEM_INCLUDE && is_gst_enabled() && 
                !is_compostion() && (item->et_type == ET_ITEM) && invoice->tax_inclusive){
              int *tax_ids = get_related_tax_ids(item->tax_id);
              double net_tax = 0;
              if(tax_ids){
                for(int i = 0; *(tax_ids + i); i++){
                  int tax_id = *(tax_ids + i);
                  taxDetails *tax_detail = get_tax_details(tax_id);
                  net_tax += tax_detail->rate; 
                  delete tax_detail;
                }
                free(tax_ids);
              }
              tmp_discount += tmp_discount * net_tax / 100;
            }
          }
        }
        discount += tmp_discount;
      }
      if(invoice->discount_type == DISCOUNT_TRANS_BEFORE && total_amount){
        if(invoice->discount_is_percentage == 1){
          discount = total_amount * invoice->discount / 100;
        } else {
          discount = invoice->discount;
        }
        //net_amount -= rnd(discount);
        discount_factor = (total_amount - discount)/total_amount;
      }
      for(auto item : invoice->items){
        if(item->et_type == ET_BUNDLE && !item->tax_id){
          double bundle_net_amount = 0;
          for(auto bundle_item : *item->bundle_items){
            Item *tmp_item = bundle_item->item;
            bundle_net_amount += tmp_item->sales_rate * tmp_item->purchase_rate;
          }
          if(!bundle_net_amount){
            continue;
          }
          for(auto bundle_item : *item->bundle_items){
            Item *tmp_item = bundle_item->item;
            double bundle_item_amount = tmp_item->sales_rate * tmp_item->purchase_rate;
            bundle_item_amount = bundle_item_amount * item->amount / bundle_net_amount;
            xmlNodePtr inv_list = xmlNewChild(sales_node, NULL, BAD_CAST "INVENTORYENTRIES.LIST", NULL);
            if(!is_empty(tmp_item->desc)){
              xmlNodePtr desc_list = xmlNewChild(inv_list, NULL, 
                                                    BAD_CAST "BASICUSERDESCRIPTION.LIST", NULL);
              xmlNewProp(desc_list, BAD_CAST "TYPE", BAD_CAST "String");
              xmlNewChild(desc_list, NULL, BAD_CAST "BASICUSERDESCRIPTION", BAD_CAST tmp_item->desc);
            }
            tmp_ptr = get_item_name(tmp_item->id);
            if(!is_empty(tmp_ptr)){
              char *esc_str = escape_html(tmp_ptr);
              xmlNewChild(inv_list, NULL, BAD_CAST "STOCKITEMNAME", BAD_CAST esc_str);
              free(esc_str);
              free(tmp_ptr);
            }
            xmlNewChild(inv_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", 
                                                  BAD_CAST (is_outward ? "No" : "Yes"));
            const_ptr = get_unit_name(get_unit_id(tmp_item->id));
            if(const_ptr){  //We have unit now
              sprintf(tmp_str, "%0.2f/%s", tmp_item->sales_rate, const_ptr);
              xmlNewChild(inv_list, NULL, BAD_CAST "RATE", BAD_CAST const_ptr);
              sprintf(tmp_str, "%0.2f %s", tmp_item->purchase_rate, const_ptr);
              xmlNewChild(inv_list, NULL, BAD_CAST "ACTUALQTY", BAD_CAST const_ptr);
              xmlNewChild(inv_list, NULL, BAD_CAST "BILLEDQTY", BAD_CAST const_ptr);
            } else {
              sprintf(tmp_str, "%0.2f", tmp_item->sales_rate);
              xmlNewChild(inv_list, NULL, BAD_CAST "RATE", BAD_CAST tmp_str);
              sprintf(tmp_str, "%0.2f", tmp_item->purchase_rate);
              xmlNewChild(inv_list, NULL, BAD_CAST "ACTUALQTY", BAD_CAST tmp_str);
              xmlNewChild(inv_list, NULL, BAD_CAST "BILLEDQTY", BAD_CAST tmp_str);
            }
            net_amount += rnd(bundle_item_amount);
            if(!is_outward){
              bundle_item_amount *= -1;
            }
            sprintf(tmp_str, "%0.2f", bundle_item_amount);
            xmlNewChild(inv_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
            xmlNodePtr acct_list = xmlNewChild(inv_list, NULL, 
                                          BAD_CAST "ACCOUNTINGALLOCATIONS.LIST", NULL);
            
            xmlNewChild(acct_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Sales");
            if(same_state){
              xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales Taxable");
            } else {
              xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", 
                                                      BAD_CAST "Intestate Sales Taxable");
            }
            xmlNewChild(acct_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", 
                                                        BAD_CAST (is_outward ? "No" : "Yes"));
            xmlNewChild(acct_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
            xmlNewChild(acct_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
            xmlNewChild(acct_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
            if(item->discount){
              xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNASSESSABLEVALUE", BAD_CAST tmp_str);
            }
            int tax_id;
            if(same_state){
              tax_id = tmp_item->intra_state_tax_id ?
                            tmp_item->intra_state_tax_id : get_default_intrastate_tax();
            } else {
              tax_id = tmp_item->inter_state_tax_id ?
                            tmp_item->inter_state_tax_id : get_default_interstate_tax();
            }
            int *tax_ids = get_related_tax_ids(tax_id);
            if(tax_ids){
              for(int i = 0; *(tax_ids + i); i++){
                tax_id = *(tax_ids + i);
                taxDetails *tax_detail = get_tax_details(tax_id);
                xmlNodePtr rate_list = xmlNewChild(acct_list, NULL, 
                                                    BAD_CAST "RATEDETAILS.LIST", NULL);
                if(tax_id > 0 && tax_id < 6){
                  xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Integrated Tax");
                  sprintf(tmp_str, "%0.2f", tax_detail->rate);
                } else if(tax_id > 5 && tax_id < 11){
                  xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Central Tax");
                } else if(tax_id > 10 && tax_id < 16) {
                  xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "State Tax");
                } else {
                  xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Cess");
                }
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEVALUATIONTYPE", 
                                              BAD_CAST "Based on Value");
                sprintf(tmp_str, "%0.2f", tax_detail->rate);
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATE", BAD_CAST tmp_str);
                delete tax_detail;
              }
              free(tax_ids);
            }
            
          } 
        } else if(item->item_id) {
          xmlNodePtr inv_list = xmlNewChild(sales_node, NULL, BAD_CAST "INVENTORYENTRIES.LIST", NULL);
          if(!is_empty(item->desc)){
            xmlNodePtr desc_list = xmlNewChild(inv_list, NULL, 
                                                  BAD_CAST "BASICUSERDESCRIPTION.LIST", NULL);
            xmlNewProp(desc_list, BAD_CAST "TYPE", BAD_CAST "String");
            char *esc_str = escape_html(item->desc);
            xmlNewChild(desc_list, NULL, BAD_CAST "BASICUSERDESCRIPTION", BAD_CAST esc_str);
            free(esc_str);
          }
          tmp_ptr = get_item_name(item->item_id);
          if(!is_empty(tmp_ptr)){
            char *esc_str = escape_html(tmp_ptr);
            xmlNewChild(inv_list, NULL, BAD_CAST "STOCKITEMNAME", BAD_CAST esc_str);
            free(esc_str);
            free(tmp_ptr);
          } else if(!is_empty(item->desc)) {
            char *esc_str = escape_html(item->desc);
            xmlNewChild(inv_list, NULL, BAD_CAST "STOCKITEMNAME", BAD_CAST esc_str);
            free(esc_str);
          }
          xmlNewChild(inv_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST (is_outward ? "No" : "Yes"));
          const_ptr = get_unit_name(get_unit_id(item->id));
          double net_tax = 0;
          int *tax_ids = get_related_tax_ids(item->tax_id);
          double rate = item->rate;
          if(tax_ids){
            for(int i = 0; *(tax_ids + i); i++){
              int tax_id = *(tax_ids + i);
              taxDetails *tax_detail = get_tax_details(tax_id);
              net_tax += tax_detail->rate;
              delete tax_detail;
            }
            for(int i = 0; *(tax_ids + i); i++){
              int tax_id = *(tax_ids + i);
              taxDetails *tax_detail = get_tax_details(tax_id);
              if(tax_detail){
                double tax_amount;
                if(invoice->tax_inclusive && net_tax != -100){
                  tax_amount = item->amount * discount_factor * tax_detail->rate/(100 + net_tax);
                } else {
                  tax_amount = item->amount * discount_factor * tax_detail->rate/100;
                }
                if(invoice->tax_inclusive){
                  rate -= (tax_amount/item->quantity);
                }
                net_tax_amount += tax_amount;
                if(tax_id > 0 && tax_id < 6){
                  igst_tax_amount += tax_amount; 
                }
                else if(tax_id > 5 && tax_id < 11){
                  cgst_tax_amount += tax_amount; 
                }
                else if(tax_id > 10 && tax_id < 16){
                  sgst_tax_amount += tax_amount; 
                } else {
                  cess_tax_amount += tax_amount; 
                }
                delete tax_detail;
              }
            }
            free(tax_ids);
          }
          
          if(const_ptr){  //We have unit now
            sprintf(tmp_str, "%0.2f/%s", rate, const_ptr);
            xmlNewChild(inv_list, NULL, BAD_CAST "RATE", BAD_CAST tmp_str);
            sprintf(tmp_str, "%0.2f %s", item->quantity, const_ptr);
            xmlNewChild(inv_list, NULL, BAD_CAST "ACTUALQTY", BAD_CAST tmp_str);
            xmlNewChild(inv_list, NULL, BAD_CAST "BILLEDQTY", BAD_CAST tmp_str);
          } else {
            sprintf(tmp_str, "%0.2f", rate);
            xmlNewChild(inv_list, NULL, BAD_CAST "RATE", BAD_CAST tmp_str);
            sprintf(tmp_str, "%0.2f", item->quantity);
            xmlNewChild(inv_list, NULL, BAD_CAST "ACTUALQTY", BAD_CAST tmp_str);
            xmlNewChild(inv_list, NULL, BAD_CAST "BILLEDQTY", BAD_CAST tmp_str);
          }
          double amount = item->quantity * rate;
          net_amount += rnd(amount);
          if(!is_outward){
            amount *= -1;
          }
          sprintf(tmp_str, "%0.2f", amount);
          xmlNewChild(inv_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
          xmlNodePtr acct_list = xmlNewChild(inv_list, NULL, 
                                        BAD_CAST "ACCOUNTINGALLOCATIONS.LIST", NULL);
          
          xmlNewChild(acct_list, NULL, BAD_CAST "LEDGERNAME", 
                                                BAD_CAST (is_sales ? "Sales" : "Purchase"));
          if(is_sales){
            if(invoice->gst_treatment == REGISTERED || invoice->gst_treatment == REGISTERED_COMPOSITION
                || invoice->gst_treatment == UNREGISTERED){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  if(same_state){
                    xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales Taxable");
                  } else {
                    xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Sales Taxable");
                  }
                } else {
                  if(same_state){
                    xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales Nil Rated");
                  } else {
                    xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Sales Nil Rated");
                  }
                }
              } else {
                if(same_state){
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales Exempt");
                } else {
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Sales Exempt");
                }
              }
            } else if(invoice->gst_treatment == CONSUMER ){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to Consumer - Taxable");
                } else {
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to consumer Nil Rated");
                }
              } else {
                xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to Consumer - Exempt");
              }
            } else if(invoice->gst_treatment == SEZ){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to SEZ - Taxable");
                } else {
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to SEZ - Nil Rated");
                }
              } else {
                xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to SEZ - Exempt");
              }
            } else if(invoice->gst_treatment == OVERSEAS){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Exports Taxable");
                } else {
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Exports Nil Rated");
                }
              } else {
                xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Exports Exempt");
              }
            } else if(invoice->gst_treatment == DEEMED_EXPORT){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Deemed Exports Taxable");
                } else {
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Deemed Exports Nil Rated");
                }
              } else {
                xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Deemed Exports Exempt");
              }
            } else {
              xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales Exempt");
            }
          } else {
            if(invoice->gst_treatment == REGISTERED || invoice->gst_treatment == REGISTERED_COMPOSITION){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  if(same_state){
                    xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Taxable");
                  } else {
                    xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Purchase Taxable");
                  }
                } else {
                  if(same_state){
                    xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Nil Rated");
                  } else {
                    xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Purchase Nil Rated");
                  }
                }
              } else {
                if(same_state){
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Exempt");
                } else {
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Purchase Exempt");
                }
              }
            } else if(invoice->gst_treatment == CONSUMER || invoice->gst_treatment == UNREGISTERED){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  if(same_state){
                    xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase From Unregistered Dealer - Taxable");
                  } else {
                    if(item->type == PRODUCT){
                      xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", 
                                                    BAD_CAST "Interstate Purchase From Unregistered Dealer - Taxable"); 
                    } else {
                      xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", 
                                                    BAD_CAST "Interstate Purchase From Unregistered Dealer - Services"); 
                    }
        
                  }
                } else {
                  if(same_state){
                    xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase From Unregistered Dealer - Nil Rated");
                  } else {
                    xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Purchase From Unregistered Dealer - Nil Rated");
                  }
                }
              } else {
                if(same_state){
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase From Unregistered Dealer - Exempt");
                } else {
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Purchase From Unregistered Dealer - Exempt");
                }
              }
            } else if(invoice->gst_treatment == SEZ){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase From SEZ - Taxable");
                } else {
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase From SEZ - Nil Rated");
                }
              } else {
                xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase From SEZ - Exempt");
              }
            } else if(invoice->gst_treatment == OVERSEAS){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Imports Taxable");
                } else {
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Imports Nil Rated");
                }
              } else {
                xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Imports Exempt");
              }
            } else if(invoice->gst_treatment == DEEMED_EXPORT){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Deemed Exports - Taxable");
                } else {
                  xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Deemed Exports - Nil Rated");
                }
              } else {
                xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Deemed Exports - Exempt");
              }
            } else {
              xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Exempt");
            }
          }
          xmlNewChild(acct_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST (is_outward ? "No" : "Yes"));
          xmlNewChild(acct_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
          xmlNewChild(acct_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
          xmlNewChild(acct_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
          tax_ids = get_related_tax_ids(item->tax_id);
          double tmp_discount = 0;

          if(tax_ids){
            for(int i = 0; *(tax_ids + i); i++){
              int tax_id = *(tax_ids + i);
              taxDetails *tax_detail = get_tax_details(tax_id);
              xmlNodePtr rate_list = xmlNewChild(acct_list, NULL, 
                                                  BAD_CAST "RATEDETAILS.LIST", NULL);
              if(tax_id > 0 && tax_id < 6){
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Integrated Tax");
                sprintf(tmp_str, "%0.2f", tax_detail->rate);
              } else if(tax_id > 5 && tax_id < 11){
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Central Tax");
              } else if(tax_id > 10 && tax_id < 16) {
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "State Tax");
              } else {
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Cess");
              }
              xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEVALUATIONTYPE", 
                                            BAD_CAST "Based on Value");
              sprintf(tmp_str, "%0.2f", tax_detail->rate);
              xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATE", BAD_CAST tmp_str);
              delete tax_detail;
            }
            free(tax_ids);
          }

          if(invoice->discount_type == DISCOUNT_ITEM || invoice->discount_type == DISCOUNT_ITEM_INCLUDE){
            double amount = item->quantity * item->rate;
            if(item->discount_is_percentage == 1){
              tmp_discount = amount * item->discount/100;
            } else {
              tmp_discount = item->discount;
              tmp_discount += tmp_discount * net_tax / 100;
              if(invoice->discount_type == DISCOUNT_ITEM && is_gst_enabled() && !is_compostion() && 
                  (item->et_type == ET_ITEM) && invoice->tax_inclusive ){
                tax_ids = get_related_tax_ids(item->tax_id);
                double net_tax = 0;
                if(tax_ids){
                  for(int i = 0; *(tax_ids + i); i++){
                    int tax_id = *(tax_ids + i);
                    taxDetails *tax_detail = get_tax_details(tax_id);
                    net_tax += tax_detail->rate; 
                    delete tax_detail;
                  }
                  free(tax_ids);
                }
                tmp_discount += tmp_discount * net_tax / 100;
              }
            }
          }

          if(item->discount){
            double amount = item->quantity * item->rate;
            sprintf(tmp_str, "%0.2f", amount);
            xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNASSESSABLEVALUE", BAD_CAST tmp_str);
            xmlNodePtr exp_list = xmlNewChild(inv_list,NULL,BAD_CAST "EXPENSEALLOCATIONS.LIST",NULL);
            xmlNewChild(exp_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Discount");
            xmlNodePtr ei_desc_list = xmlNewChild(inv_list, NULL, 
                                                  BAD_CAST "UDF:EIDISCOUNTRATE.LIST", NULL );
            xmlNewProp(ei_desc_list, BAD_CAST "ISLIST", BAD_CAST "YES");
            if(item->discount_is_percentage == 1){
              xmlNewProp(ei_desc_list, BAD_CAST "DESC", BAD_CAST "`EI DiscountRate`");
              xmlNewProp(ei_desc_list, BAD_CAST "TYPE", BAD_CAST "Number");
              sprintf(tmp_str,"%0.2f", (tmp_discount));
              xmlNewChild(exp_list, NULL, BAD_CAST "VATASSESSABLEAMOUNT", BAD_CAST tmp_str);
              //net_amount -= rnd(tmp_discount);
              if(!is_outward){
                sprintf(tmp_str,"%0.2f", -item->discount);
              } else {
                sprintf(tmp_str,"%0.2f", item->discount);
              }
              xmlNodePtr ei_desc = xmlNewChild(ei_desc_list, NULL, 
                                                  BAD_CAST "UDF:EIDISCOUNTRATE", BAD_CAST tmp_str);
              xmlNewProp(ei_desc, BAD_CAST "DESC", BAD_CAST "`EI DiscountRate`");
              
            } else {
              xmlNewProp(ei_desc_list, BAD_CAST "DESC", BAD_CAST "`EI DiscountAmt`");
              xmlNewProp(ei_desc_list, BAD_CAST "TYPE", BAD_CAST "Amount");
              sprintf(tmp_str,"%0.2f", tmp_discount);
              xmlNewChild(exp_list, NULL, BAD_CAST "VATASSESSABLEAMOUNT", BAD_CAST tmp_str);
              //net_amount -= rnd(tmp_discount);
              if(!is_outward){
                tmp_discount *= -1;
              }
              sprintf(tmp_str,"%0.2f", tmp_discount);
              xmlNodePtr ei_desc = xmlNewChild(ei_desc_list, NULL, 
                                                  BAD_CAST "UDF:EIDISCOUNTAMT", BAD_CAST tmp_str);
              xmlNewProp(ei_desc, BAD_CAST "DESC", BAD_CAST "`EI DiscountAmt`");
            }
          }
        } else if(!is_empty(item->desc)){
          double net_tax = 0;
          int *tax_ids = get_related_tax_ids(item->tax_id);
          if(tax_ids){
            for(int i = 0; *(tax_ids + i); i++){
              int tax_id = *(tax_ids + i);
              taxDetails *tax_detail = get_tax_details(tax_id);
              net_tax += tax_detail->rate;
              delete tax_detail;
            }
            for(int i = 0; *(tax_ids + i); i++){
              int tax_id = *(tax_ids + i);
              taxDetails *tax_detail = get_tax_details(tax_id);
              if(tax_detail){
                double tax_amount;
                if(invoice->tax_inclusive && net_tax != -100){
                  tax_amount = item->amount * discount_factor * tax_detail->rate/(100 + net_tax);
                } else {
                  tax_amount = item->amount * discount_factor * tax_detail->rate/100;
                }
                if(invoice->tax_inclusive){ //note here wa are not considering inline discounts.
                  item->rate -= tax_amount;
                }
                tax_amount *= item->quantity;
                net_tax_amount += tax_amount;
                if(tax_id > 0 && tax_id < 6){
                  igst_tax_amount += tax_amount; 
                }
                else if(tax_id > 5 && tax_id < 11){
                  cgst_tax_amount += tax_amount; 
                }
                else if(tax_id > 10 && tax_id < 16){
                  sgst_tax_amount += tax_amount; 
                } else {
                  cess_tax_amount += tax_amount; 
                }
                delete tax_detail;
              }
            }
            free(tax_ids);
          }
          net_amount += rnd(item->quantity * item->rate);
        }
      }
      //net_amount += rnd(net_tax_amount) + rnd(invoice->shipping_charges) + rnd(invoice->adjustment) + rnd(invoice->roundoff);
      net_amount += rnd(igst_tax_amount) + rnd(sgst_tax_amount)  + rnd(cgst_tax_amount)+ rnd(cess_tax_amount); 
      net_amount += rnd(invoice->shipping_charges);
      net_amount += rnd(invoice->adjustment);
      net_amount += rnd(invoice->roundoff);
      net_amount -= rnd(discount); 
      tmp_ptr = get_contact_name(invoice->cust_id, mysql_handle);
      if(!is_empty(tmp_ptr)){
        xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                BAD_CAST "LEDGERENTRIES.LIST", NULL);
        contactType type = get_customer_type(invoice->cust_id);
        char *esc_str = escape_html(tmp_ptr);
        if(type == BOTH){
          if(is_sales){
            sprintf(tmp_str, "%s (Customer)", esc_str);
          } else {
            sprintf(tmp_str, "%s (Supplier)", esc_str);
          }
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST tmp_str);
        } else {
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST esc_str);
        }
        free(esc_str);
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", 
                                                BAD_CAST (is_outward ? "Yes" : "No"));
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "Yes");
        if(invoice->tax_inclusive){
          net_tax_amount = 0;
        }
        //sprintf(tmp_str, "%0.2f", -(total_amount + net_tax_amount - discount 
        //                  + invoice->shipping_charges + invoice->adjustment + invoice->roundoff));
        //printf("calcuulated = %0.2f vs current = %s\n", net_amount, tmp_str);
        
        if(!is_outward){
          sprintf(tmp_str, "%0.2f", net_amount);
        } else {
          sprintf(tmp_str, "%0.2f", -net_amount);
        }
        xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
      }
      free(tmp_ptr);
      for(auto item : invoice->items){
        if(!(item->et_type == ET_BUNDLE && !item->tax_id) && !item->item_id && !is_empty(item->desc)){
          xmlNodePtr inv_list = xmlNewChild(sales_node, NULL, BAD_CAST "LEDGERENTRIES.LIST", NULL);
          xmlNewChild(inv_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "General Service");
          xmlNodePtr desc_list = xmlNewChild(inv_list, NULL, 
                                                BAD_CAST "UDF:USERDESCRIPTION.LIST", NULL);
          xmlNewProp(desc_list, BAD_CAST "DESC", BAD_CAST "`User Description`");
          xmlNewProp(desc_list, BAD_CAST "ISLIST", BAD_CAST "YES");
          xmlNewProp(desc_list, BAD_CAST "TYPE", BAD_CAST "String");
          char *esc_str = escape_html(item->desc);
          xmlNodePtr desc = xmlNewChild(desc_list, NULL, 
                                          BAD_CAST "UDF:USERDESCRIPTION", BAD_CAST esc_str);
          free(esc_str);
          xmlNewProp(desc, BAD_CAST "DESC", BAD_CAST "`User Description`");
          xmlNewChild(inv_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST (is_outward ? "No" : "Yes"));
          
          double amount = item->quantity * item->rate;
          if(!is_outward){
            amount *= -1;
          }
          sprintf(tmp_str, "%0.2f", amount);
          xmlNewChild(inv_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);

          if(is_sales){
            if(invoice->gst_treatment == REGISTERED || invoice->gst_treatment == REGISTERED_COMPOSITION
                || invoice->gst_treatment == UNREGISTERED){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  if(same_state){
                    xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales Taxable");
                  } else {
                    xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Sales Taxable");
                  }
                } else {
                  if(same_state){
                    xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales Nil Rated");
                  } else {
                    xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Sales Nil Rated");
                  }
                }
              } else {
                if(same_state){
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales Exempt");
                } else {
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Sales Exempt");
                }
              }
            } else if(invoice->gst_treatment == CONSUMER ){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to Consumer - Taxable");
                } else {
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to consumer Nil Rated");
                }
              } else {
                xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to Consumer - Exempt");
              }
            } else if(invoice->gst_treatment == SEZ){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to SEZ - Taxable");
                } else {
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to SEZ - Nil Rated");
                }
              } else {
                xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to SEZ - Exempt");
              }
            } else if(invoice->gst_treatment == OVERSEAS){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Exports Taxable");
                } else {
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Exports Nil Rated");
                }
              } else {
                xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Exports Exempt");
              }
            } else if(invoice->gst_treatment == DEEMED_EXPORT){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Deemed Exports Taxable");
                } else {
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Deemed Exports Nil Rated");
                }
              } else {
                xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Deemed Exports Exempt");
              }
            } else {
              xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales Exempt");
            }
          } else {
            if(invoice->gst_treatment == REGISTERED || invoice->gst_treatment == REGISTERED_COMPOSITION){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  if(same_state){
                    xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Taxable");
                  } else {
                    xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Purchase Taxable");
                  }
                } else {
                  if(same_state){
                    xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Nil Rated");
                  } else {
                    xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Purchase Nil Rated");
                  }
                }
              } else {
                if(same_state){
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Exempt");
                } else {
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Purchase Exempt");
                }
              }
            } else if(invoice->gst_treatment == CONSUMER || invoice->gst_treatment == UNREGISTERED){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  if(same_state){
                    xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase From Unregistered Dealer - Taxable");
                  } else {
                    if(item->type == PRODUCT){
                      xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", 
                                                    BAD_CAST "Interstate Purchase From Unregistered Dealer - Taxable"); 
                    } else {
                      xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", 
                                                    BAD_CAST "Interstate Purchase From Unregistered Dealer - Services"); 
                    }
        
                  }
                } else {
                  if(same_state){
                    xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase From Unregistered Dealer - Nil Rated");
                  } else {
                    xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Purchase From Unregistered Dealer - Nil Rated");
                  }
                }
              } else {
                if(same_state){
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase From Unregistered Dealer - Exempt");
                } else {
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Interstate Purchase From Unregistered Dealer - Exempt");
                }
              }
            } else if(invoice->gst_treatment == SEZ){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase From SEZ - Taxable");
                } else {
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase From SEZ - Nil Rated");
                }
              } else {
                xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase From SEZ - Exempt");
              }
            } else if(invoice->gst_treatment == OVERSEAS){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Imports Taxable");
                } else {
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Imports Nil Rated");
                }
              } else {
                xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Imports Exempt");
              }
            } else if(invoice->gst_treatment == DEEMED_EXPORT){
              if(item->tax_id > 0){
                if(item->tax_id != 1 && item->tax_id != 16){
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Deemed Exports - Taxable");
                } else {
                  xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Deemed Exports - Nil Rated");
                }
              } else {
                xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Deemed Exports - Exempt");
              }
            } else {
              xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Purchase Exempt");
            }
          }
          xmlNewChild(inv_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST (is_outward ? "No" : "Yes"));
          xmlNewChild(inv_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
          xmlNewChild(inv_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
          xmlNewChild(inv_list, NULL, BAD_CAST "GSTOVRDNTAXABILITY", BAD_CAST "TAXABLE");
          int *tax_ids = get_related_tax_ids(item->tax_id);
          if(tax_ids){
            for(int i = 0; *(tax_ids + i); i++){
              int tax_id = *(tax_ids + i);
              taxDetails *tax_detail = get_tax_details(tax_id);
              xmlNodePtr rate_list = xmlNewChild(inv_list, NULL, 
                                                  BAD_CAST "RATEDETAILS.LIST", NULL);
              if(tax_id > 0 && tax_id < 6){
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Integrated Tax");
                sprintf(tmp_str, "%0.2f", tax_detail->rate);
              } else if(tax_id > 5 && tax_id < 11){
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Central Tax");
              } else if(tax_id > 10 && tax_id < 16) {
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "State Tax");
              } else {
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Cess");
              }
              xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEVALUATIONTYPE", 
                                            BAD_CAST "Based on Value");
              sprintf(tmp_str, "%0.2f", tax_detail->rate);
              xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATE", BAD_CAST tmp_str);
              delete tax_detail;
            }
            free(tax_ids);
          }
        }
      }
      if(invoice->shipping_charges){
        xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, BAD_CAST "LEDGERENTRIES.LIST", NULL);
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Shipping Charge");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST (is_outward ? "No" : "Yes"));
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
        if(!is_outward){
          sprintf(tmp_str, "%0.2f", -invoice->shipping_charges); 
        } else {
          sprintf(tmp_str, "%0.2f", invoice->shipping_charges); 
        }
        xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
        if(invoice->shipping_charge_tax_id){
          if(invoice->shipping_charge_tax_id != 1 && invoice->shipping_charge_tax_id != 16){
            if(same_state){
              xmlNewChild(ledger_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales Taxable");
            } else {
              xmlNewChild(ledger_list, NULL, BAD_CAST "GSTOVRDNNATURE", 
                                                              BAD_CAST "Interstate Sales Taxable");
            }
          } else {
            if(same_state){
              xmlNewChild(ledger_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales Nil Rated");
            } else {
              xmlNewChild(ledger_list, NULL, BAD_CAST "GSTOVRDNNATURE", 
                                                              BAD_CAST "Interstate Sales Nil Rated");
            }
          }
          int *tax_ids = get_related_tax_ids(invoice->shipping_charge_tax_id);
          if(tax_ids){
            for(int i = 0; *(tax_ids + i); i++){
              int tax_id = *(tax_ids + i);
              taxDetails *tax_detail = get_tax_details(tax_id);
              xmlNodePtr rate_list = xmlNewChild(ledger_list, NULL, 
                                                  BAD_CAST "RATEDETAILS.LIST", NULL);
              if(tax_id > 0 && tax_id < 6){
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Integrated Tax");
                sprintf(tmp_str, "%0.2f", tax_detail->rate);
              } else if(tax_id > 5 && tax_id < 11){
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Central Tax");
              } else if(tax_id > 10 && tax_id < 16) {
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "State Tax");
              } else {
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Cess");
              }
              xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEVALUATIONTYPE", 
                                            BAD_CAST "Based on Value");
              sprintf(tmp_str, "%0.2f", tax_detail->rate);
              xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATE", BAD_CAST tmp_str);
              delete tax_detail;
            }
            free(tax_ids);
          }
        }
      }
      if(invoice->adjustment + invoice->roundoff ){
        xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                BAD_CAST "LEDGERENTRIES.LIST", NULL);
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Other Charges");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST (is_outward ? "No" : "Yes"));
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
        if(!is_outward){
          sprintf(tmp_str, "%0.2f", -(invoice->adjustment + invoice->roundoff));
        } else {
          sprintf(tmp_str, "%0.2f", invoice->adjustment + invoice->roundoff);
        }
        xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
      }
      if(igst_tax_amount){
        xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                BAD_CAST "LEDGERENTRIES.LIST", NULL);
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Output IGST");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST (is_outward ? "No" : "Yes"));
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
        if(!is_outward){
          sprintf(tmp_str, "%0.2f", -igst_tax_amount);
        } else {
          sprintf(tmp_str, "%0.2f", igst_tax_amount);
        }
        xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
      }
      if(cgst_tax_amount){
        xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                BAD_CAST "LEDGERENTRIES.LIST", NULL);
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Output CGST");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST (is_outward ? "No" : "Yes"));
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
        if(!is_outward){
          sprintf(tmp_str, "%0.2f", -cgst_tax_amount);
        } else {
          sprintf(tmp_str, "%0.2f", cgst_tax_amount);
        }
        xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
      }
      if(sgst_tax_amount){
        xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                BAD_CAST "LEDGERENTRIES.LIST", NULL);
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Output SGST");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST (is_outward ? "No" : "Yes"));
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
        if(!is_outward){
          sprintf(tmp_str, "%0.2f", -sgst_tax_amount);
        } else {
          sprintf(tmp_str, "%0.2f", sgst_tax_amount);
        }
        xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
      }
      if(cess_tax_amount){
        xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                BAD_CAST "LEDGERENTRIES.LIST", NULL);
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Output Cess");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST (is_outward ? "No" : "Yes"));
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
        if(!is_outward){
          sprintf(tmp_str, "%0.2f", -cess_tax_amount);
        } else {
          sprintf(tmp_str, "%0.2f", cess_tax_amount);
        }
        xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
      }
      if(discount){
        xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                BAD_CAST "LEDGERENTRIES.LIST", NULL);
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Discount");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST (is_outward ? "No" : "Yes"));
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
        if(!is_outward){
          sprintf(tmp_str, "%0.2f", discount);
        } else {
          sprintf(tmp_str, "%0.2f", -discount);
        }
        xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
      }
      
      delete invoice;
      ret = true;
    }
  } while(0);
  return ret;
}

bool fill_sales_objects(xmlNodePtr parent, MYSQL *mysql_handle,int from_date,int to_date, 
                              int thread_id, bool &error){
  bool ret = false;
  do{
    std::vector<Invoice *>* invoice_list = get_all_invoices(mysql_handle, from_date, to_date, 
                                        thread_id, error);
    if(error){
      break;
    }
    std::vector<TaxObject*> tax_obj_list; 
    for(auto invoice : *invoice_list){
      //if(!strcmp(invoice->number, "INV-000005")){
        tax_obj_list.push_back(invoice);
      //}
    }
    ret = fill_tax_objects(parent, mysql_handle, &tax_obj_list, thread_id,  error);
    delete invoice_list;
    
  } while(0);
  
  return ret;
}

bool fill_cn_objects(xmlNodePtr parent, MYSQL *mysql_handle,int from_date,int to_date, 
                              int thread_id, bool &error){
  bool ret = false;
  do{
    std::vector<CreditNote*>* credit_note_list = get_all_credit_notes(mysql_handle, from_date, to_date, 
                                        thread_id, error);
    if(error){
      break;
    }
    std::vector<TaxObject*> tax_obj_list; 
    for(auto credit_note : *credit_note_list){
      if(credit_note->invoice_id){
        free(credit_note->reference);
        credit_note->currency_id = 0;
        credit_note->reference = get_invoice_number_n_date(credit_note->invoice_id, mysql_handle, 
                                                                            credit_note->currency_id);
        credit_note->discount_accnt_id = credit_note->reason_id;
      }
      tax_obj_list.push_back(credit_note);
    }
    ret = fill_tax_objects(parent, mysql_handle, &tax_obj_list, thread_id,  error, CREDIT_NOTE);
    delete credit_note_list;
    
  } while(0);
  
  return ret;
}

bool fill_purchase_objects(xmlNodePtr parent, MYSQL *mysql_handle,int from_date,int to_date, 
                              int thread_id, bool &error){
  bool ret = false;
  do{
    std::vector<Bill *>* bill_list = get_all_bills(mysql_handle, from_date, to_date, 
                                        thread_id, error);
    if(error){
      break;
    }
    std::vector<TaxObject*> tax_obj_list; 
    for(auto bill : *bill_list){
      tax_obj_list.push_back(bill);
    }
    ret = fill_tax_objects(parent, mysql_handle, &tax_obj_list, thread_id,  error, BILLS);
    delete bill_list;
    
  } while(0);
  
  return ret;
}

bool fill_purchase_return_objects(xmlNodePtr parent, MYSQL *mysql_handle,int from_date,int to_date, 
                              int thread_id, bool &error){
  bool ret = false;
  do{
    std::vector<VendorCredit *>* vc_list = get_all_supplier_credits(mysql_handle, from_date, to_date, 
                                        thread_id, error);
    if(error){
      break;
    }
    std::vector<TaxObject*> tax_obj_list; 
    for(auto supplier_credit : *vc_list){
      if(supplier_credit->bill_id){
        free(supplier_credit->reference);
        supplier_credit->currency_id = 0;
        supplier_credit->reference = get_bill_number_n_date(supplier_credit->bill_id, mysql_handle, 
                                                   supplier_credit->currency_id);
      }
      tax_obj_list.push_back(supplier_credit);
    }
    ret = fill_tax_objects(parent, mysql_handle, &tax_obj_list, thread_id,  error, DEBIT_NOTE);
    delete vc_list;
    
  } while(0);
  
  return ret;
}

bool fill_pos_sales_objects(xmlNodePtr parent, MYSQL *mysql_handle,int from_date,int to_date, 
                              int thread_id, bool &error){
  bool ret = false;
  do{
    std::vector<posData *> *pos_list = get_all_pos_invoices(mysql_handle, from_date, to_date, error);
    if(pos_list){
      for(auto invoice: *pos_list){
        char tmp_str[40];
        xmlNodePtr sales_node = xmlNewChild(parent, NULL, BAD_CAST "VOUCHER", NULL);
        xmlNewProp(sales_node, BAD_CAST "VCHTYPE", BAD_CAST "Sales");
        make_date(tmp_str, invoice->date);
        xmlNewChild(sales_node, NULL, BAD_CAST "DATE", BAD_CAST tmp_str);
        const char *const_ptr = get_state_name(get_org_state());
        if(const_ptr){
          xmlNewChild(sales_node, NULL, BAD_CAST "STATENAME", BAD_CAST const_ptr);
          xmlNewChild(sales_node, NULL, BAD_CAST "PLACEOFSUPPLY", BAD_CAST const_ptr);
        }
        const_ptr = get_country_name(get_org_country());
        if(const_ptr){
          xmlNewChild(sales_node, NULL, BAD_CAST "COUNTRYOFRESIDENCE", BAD_CAST const_ptr);
        }
        xmlNewChild(sales_node, NULL, BAD_CAST "PARTYNAME", BAD_CAST "Walk-in Customer");
        xmlNewChild(sales_node, NULL, BAD_CAST "VOUCHERTYPENAME", BAD_CAST "Sales");
        sprintf(tmp_str, "POS-%02d-%d",invoice->pos_id, invoice->user_id);
        xmlNewChild(sales_node, NULL, BAD_CAST "VOUCHERNUMBER", BAD_CAST tmp_str);
        xmlNewChild(sales_node, NULL, BAD_CAST "ISGSTOVERRIDDEN", BAD_CAST "Yes");
        xmlNewChild(sales_node, NULL, BAD_CAST "PERSISTEDVIEW", BAD_CAST "Invoice Voucher View");
        xmlNewChild(sales_node, NULL, BAD_CAST "ISINVOICE", BAD_CAST "Yes");
        xmlNewChild(sales_node, NULL, BAD_CAST "HASDISCOUNTS", 
                      BAD_CAST (invoice->discount ? "Yes" : "No"));
        double net_amount = 0;
        for(auto item : invoice->items){
          char *tmp_ptr = get_item_name(item->id);
          if(is_empty(tmp_ptr)){
            continue;
          }
          xmlNodePtr inv_list = xmlNewChild(sales_node, NULL, BAD_CAST "INVENTORYENTRIES.LIST", NULL);
          if(!is_empty(tmp_ptr)){
            char *esc_str = escape_html(tmp_ptr);
            xmlNewChild(inv_list, NULL, BAD_CAST "STOCKITEMNAME", BAD_CAST esc_str);
            free(esc_str);
            free(tmp_ptr);
          }
          xmlNewChild(inv_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST "No");
          const_ptr = get_unit_name(get_unit_id(item->id));
          double rate = item->amount/item->quantity;
          if(const_ptr){  //We have unit now
            sprintf(tmp_str, "%0.2f/%s", rate, const_ptr);
            xmlNewChild(inv_list, NULL, BAD_CAST "RATE", BAD_CAST tmp_str);
            sprintf(tmp_str, "%0.2f %s", item->quantity, const_ptr);
            xmlNewChild(inv_list, NULL, BAD_CAST "ACTUALQTY", BAD_CAST tmp_str);
            xmlNewChild(inv_list, NULL, BAD_CAST "BILLEDQTY", BAD_CAST tmp_str);
          } else {
            sprintf(tmp_str, "%0.2f", rate);
            xmlNewChild(inv_list, NULL, BAD_CAST "RATE", BAD_CAST tmp_str);
            sprintf(tmp_str, "%0.2f", item->quantity);
            xmlNewChild(inv_list, NULL, BAD_CAST "ACTUALQTY", BAD_CAST tmp_str);
            xmlNewChild(inv_list, NULL, BAD_CAST "BILLEDQTY", BAD_CAST tmp_str);
          }
          sprintf(tmp_str, "%0.2f", item->amount);
          xmlNewChild(inv_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
          xmlNodePtr acct_list = xmlNewChild(inv_list, NULL, 
                                        BAD_CAST "ACCOUNTINGALLOCATIONS.LIST", NULL);
          
          xmlNewChild(acct_list, NULL, BAD_CAST "LEDGERNAME",BAD_CAST "Sales");
          int tax_id = 0;
          int inter_tax_id = 0;
          get_tax_ids(item->id, inter_tax_id, tax_id, mysql_handle);
 
          if(tax_id > 0){
            if(tax_id != 1 && tax_id != 16){
              xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to Consumer - Taxable");
            } else {
              xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to consumer Nil Rated");
            }
          } else {
            xmlNewChild(acct_list, NULL, BAD_CAST "GSTOVRDNNATURE", BAD_CAST "Sales to Consumer - Exempt");
          }
          xmlNewChild(acct_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST "No");
          xmlNewChild(acct_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
          xmlNewChild(acct_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
          xmlNewChild(acct_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
          int *tax_ids = get_related_tax_ids(tax_id);

          if(tax_ids){
            for(int i = 0; *(tax_ids + i); i++){
              int tax_id = *(tax_ids + i);
              taxDetails *tax_detail = get_tax_details(tax_id);
              xmlNodePtr rate_list = xmlNewChild(acct_list, NULL, 
                                                  BAD_CAST "RATEDETAILS.LIST", NULL);
              if(tax_id > 0 && tax_id < 6){
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Integrated Tax");
                sprintf(tmp_str, "%0.2f", tax_detail->rate);
              } else if(tax_id > 5 && tax_id < 11){
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Central Tax");
              } else if(tax_id > 10 && tax_id < 16) {
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "State Tax");
              } else {
                xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEDUTYHEAD", BAD_CAST "Cess");
              }
              xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATEVALUATIONTYPE", 
                                            BAD_CAST "Based on Value");
              sprintf(tmp_str, "%0.2f", tax_detail->rate);
              xmlNewChild(rate_list, NULL, BAD_CAST "GSTRATE", BAD_CAST tmp_str);
              delete tax_detail;
            }
            free(tax_ids);
          }
          net_amount += rnd(item->amount);
        }

        double igst_tax_amount = 0;
        double cgst_tax_amount = 0;
        double sgst_tax_amount = 0;
        double cess_tax_amount = 0;

        for(auto tax_item : invoice->tax_items){
          if(tax_item->id > 0 && tax_item->id < 6){
            igst_tax_amount += tax_item->amount;
          } else if(tax_item->id > 5 && tax_item->id < 11){
            cgst_tax_amount += tax_item->amount;
          } else if(tax_item->id > 10 && tax_item->id < 16) {
            sgst_tax_amount += tax_item->amount;
          } else {
            cess_tax_amount += tax_item->amount;
          }
        }
        net_amount += rnd(igst_tax_amount) + rnd(sgst_tax_amount) + 
                                              rnd(cgst_tax_amount)+ rnd(cess_tax_amount); 
        net_amount += rnd(invoice->adjustment);
        net_amount += rnd(invoice->roundoff);
        net_amount -= rnd(invoice->discount); 
        xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                BAD_CAST "LEDGERENTRIES.LIST", NULL);
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Walk-in Customer");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST "Yes");
        xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
        xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "Yes");
        if(invoice->adjustment + invoice->roundoff ){
          xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                  BAD_CAST "LEDGERENTRIES.LIST", NULL);
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Other Charges");
          xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST "No");
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
          xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
          sprintf(tmp_str, "%0.2f", invoice->adjustment + invoice->roundoff);
          xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
        }
        if(igst_tax_amount){
          xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                  BAD_CAST "LEDGERENTRIES.LIST", NULL);
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Output IGST");
          xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST "No");
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
          xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
          sprintf(tmp_str, "%0.2f", igst_tax_amount);
          xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
        }
        if(cgst_tax_amount){
          xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                  BAD_CAST "LEDGERENTRIES.LIST", NULL);
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Output CGST");
          xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST "No");
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
          xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
          sprintf(tmp_str, "%0.2f", cgst_tax_amount);
          xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
        }
        if(sgst_tax_amount){
          xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                  BAD_CAST "LEDGERENTRIES.LIST", NULL);
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Output SGST");
          xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST "No");
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
          xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
          sprintf(tmp_str, "%0.2f", sgst_tax_amount);
          xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
        }
        if(cess_tax_amount){
          xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                  BAD_CAST "LEDGERENTRIES.LIST", NULL);
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Output Cess");
          xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST "No");
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
          xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
          sprintf(tmp_str, "%0.2f", cess_tax_amount);
          xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
        }
        if(invoice->discount){
          xmlNodePtr ledger_list = xmlNewChild(sales_node, NULL, 
                                                  BAD_CAST "LEDGERENTRIES.LIST", NULL);
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERNAME", BAD_CAST "Discount");
          xmlNewChild(ledger_list, NULL, BAD_CAST "ISDEEMEDPOSITIVE", BAD_CAST "No");
          xmlNewChild(ledger_list, NULL, BAD_CAST "LEDGERFROMITEM", BAD_CAST "No");
          xmlNewChild(ledger_list, NULL, BAD_CAST "ISPARTYLEDGER", BAD_CAST "No");
          sprintf(tmp_str, "%0.2f", -invoice->discount);
          xmlNewChild(ledger_list, NULL, BAD_CAST "AMOUNT", BAD_CAST tmp_str);
        }
        delete invoice;
      }
      delete pos_list;
    }
  } while(0);
  return ret;
}

void export_tally_xml_from_db(MYSQL_STMT *stmt, void *arg, void *params, int thread_id) {
  action_record *act_rec = (action_record*) arg;
  free(act_rec->write_buff); //we dont need this any more..
  MYSQL* mysql_handle = (MYSQL*) params;
  struct client *client = act_rec->client;
  tallyExport *export_details = (tallyExport*)act_rec->ptr;
  xmlDocPtr master_doc = xmlNewDoc(BAD_CAST "1.0");
  xmlDocPtr voucher_doc = xmlNewDoc(BAD_CAST "1.0");
  bool error = false;
  char *org_name = NULL; 
  char path[500];
  char rand_str[21];
  zip_t *zipper = NULL;
  do {
    memset(rand_str,0,21);
    generate_random_string(rand_str,20);
    snprintf(path, 499, "%s/%s/files/_tmp_/%s",base_dir,instance,rand_str);

    if (mkdir(path, 0777) != 0){
      ERROR_LOG("Some error creating directory - %s.. error : %s\n", path, strerror(errno));
      write_response(failed_json, client);
      break;
    }

    xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST "ENVELOPE");
    xmlDocSetRootElement(master_doc, root_node);
    xmlNodePtr header = xmlNewChild(root_node, NULL, BAD_CAST "HEADER", NULL);
    xmlNodePtr tally_req = xmlNewChild(header, NULL, BAD_CAST "TALLYREQUEST", NULL);
    xmlNodeSetContent(tally_req, BAD_CAST "Import Data");
    xmlNodePtr body = xmlNewChild(root_node, NULL, BAD_CAST "BODY", NULL);
    xmlNodePtr import_data = xmlNewChild(body, NULL, BAD_CAST "IMPORTDATA", NULL);
    xmlNodePtr request_desc = xmlNewChild(import_data, NULL, BAD_CAST "REQUESTDESC", NULL);
    xmlNodePtr report_name = xmlNewChild(request_desc, NULL, BAD_CAST "REPORTNAME", NULL);
    xmlNodeSetContent(report_name, BAD_CAST "All Masters");
    xmlNodePtr static_variables = xmlNewChild(request_desc, NULL, BAD_CAST "STATICVARIABLES", NULL);
    xmlNodePtr sv_company = xmlNewChild(static_variables, NULL, BAD_CAST "SVCURRENTCOMPANY", NULL);
    org_name = get_org_name();
    xmlNodeSetContent(sv_company, BAD_CAST org_name);

    xmlNodePtr request_data = xmlNewChild(import_data, NULL, BAD_CAST "REQUESTDATA", NULL);

    xmlNodePtr currency_message = xmlNewChild(request_data, NULL, BAD_CAST "TALLYMESSAGE", NULL);
    if(!fill_currency_objects(currency_message, mysql_handle, thread_id)){
      xmlUnlinkNode(currency_message);
      xmlFreeNode(currency_message);
    }
    xmlNodePtr unit_message = xmlNewChild(request_data, NULL, BAD_CAST "TALLYMESSAGE", NULL);
    if(!fill_unit_objects(unit_message, mysql_handle, thread_id)){
      xmlUnlinkNode(unit_message);
      xmlFreeNode(unit_message);
    }
    xmlNodePtr category_message = xmlNewChild(request_data, NULL, BAD_CAST "TALLYMESSAGE", NULL);
    if(!fill_item_categories(category_message, mysql_handle, thread_id)){
      xmlUnlinkNode(category_message);
      xmlFreeNode(category_message);
    }
    xmlNodePtr item_message = xmlNewChild(request_data, NULL, BAD_CAST "TALLYMESSAGE", NULL);
    if(!fill_item_objects(item_message, mysql_handle, thread_id, error)){
      xmlUnlinkNode(item_message);
      xmlFreeNode(item_message);
      if(error){
        write_response(failed_json, client);
        break;
      }
    }
    xmlNodePtr contact_message = xmlNewChild(request_data, NULL, BAD_CAST "TALLYMESSAGE", NULL);
    if(!fill_contact_objects(contact_message, mysql_handle, export_details->from_date, 
                                                              export_details->to_date, error)){
      xmlUnlinkNode(contact_message);
      xmlFreeNode(contact_message);
      if(error){
        write_response(failed_json, client);
        break;
      }
    }
    xmlNodePtr accounts_message = xmlNewChild(request_data, NULL, BAD_CAST "TALLYMESSAGE", NULL);
    if(!fill_other_accounts(accounts_message, mysql_handle, export_details->from_date, 
                                      export_details->to_date, error)){
      xmlUnlinkNode(accounts_message);
      xmlFreeNode(accounts_message);
      if(error){
        write_response(failed_json, client);
        break;
      }
    }
    int errorp;
    snprintf(path, 499, "%s/%s/files/_tmp_/%s.zip",base_dir,instance,rand_str);
    zipper = zip_open(path, ZIP_CREATE | ZIP_EXCL, &errorp);
    if (!zipper) {
      zip_error_t ziperror;
      zip_error_init_with_code(&ziperror, errorp);
      ERROR_LOG("Failed to open output file - %s : %s", path, zip_error_strerror(&ziperror));
      write_response(failed_json, client);
      break;
    }
    xmlBufferPtr buf = xmlBufferCreate();
    xmlSaveCtxtPtr save_ctx = xmlSaveToBuffer(buf, "UTF-8", XML_SAVE_FORMAT|XML_SAVE_NO_DECL);
    xmlSaveDoc(save_ctx, master_doc);
    xmlSaveFlush(save_ctx);
    xmlSaveClose(save_ctx);
    char* modified_content = f_n_replace((const char*)buf->content,"&amp;#4;", "&#4;");
    xmlBufferFree(buf); 
    snprintf(path, 499, "%s/%s/files/_tmp_/%s/masters.xml",base_dir,instance,rand_str);
    FILE *fp = fopen(path, "w");
    if(!fp){
      ERROR_LOG("Unable to create tally master file");
      write_response(failed_json, client);
      break;
    }
    fputs(modified_content, fp);
    fclose(fp);
    free(modified_content);
    zip_source_t *source = zip_source_file(zipper, path, 0, 0);
    if(!source){
      ERROR_LOG("Failed to add file %s to zip: %s", path, zip_strerror(zipper));
      write_response(failed_json, client);
      break;
    }
    if(zip_file_add(zipper,"masters.xml", source, ZIP_FL_ENC_UTF_8) < 0){
      ERROR_LOG("Failed(2) to add file %s to zip: %s", path, zip_strerror(zipper));
      write_response(failed_json, client);
      break;
    }
    //xmlSaveFormatFileEnc("masters.xml", doc, "UTF-8", XML_SAVE_NO_DECL);
   
    root_node = xmlNewNode(NULL, BAD_CAST "ENVELOPE");
    xmlDocSetRootElement(voucher_doc, root_node);
    header = xmlNewChild(root_node, NULL, BAD_CAST "HEADER", NULL);
    tally_req = xmlNewChild(header, NULL, BAD_CAST "TALLYREQUEST", NULL);
    xmlNodeSetContent(tally_req, BAD_CAST "Import Data");
    body = xmlNewChild(root_node, NULL, BAD_CAST "BODY", NULL);
    import_data = xmlNewChild(body, NULL, BAD_CAST "IMPORTDATA", NULL);
    request_desc = xmlNewChild(import_data, NULL, BAD_CAST "REQUESTDESC", NULL);
    report_name = xmlNewChild(request_desc, NULL, BAD_CAST "REPORTNAME", NULL);
    xmlNodeSetContent(report_name, BAD_CAST "Vouchers");
    static_variables = xmlNewChild(request_desc, NULL, BAD_CAST "STATICVARIABLES", NULL);
    sv_company = xmlNewChild(static_variables, NULL, BAD_CAST "SVCURRENTCOMPANY", NULL);
    xmlNodeSetContent(sv_company, BAD_CAST org_name);

    request_data = xmlNewChild(import_data, NULL, BAD_CAST "REQUESTDATA", NULL);

    xmlNodePtr sales_message = xmlNewChild(request_data, NULL, BAD_CAST "TALLYMESSAGE", NULL);
    bool no_sales_data = false;
    if(!fill_sales_objects(sales_message, mysql_handle, export_details->from_date, 
                            export_details->to_date, thread_id, error)){
      if(error){
        write_response(failed_json, client);
        break;
      }
      no_sales_data = true;
    }
    if(fill_pos_sales_objects(sales_message, mysql_handle, export_details->from_date, 
                                                      export_details->to_date, thread_id, error)){
      no_sales_data = false;
    } else {
      if(error){
        write_response(failed_json, client);
        break;
      }
    }
    if(no_sales_data){
      xmlUnlinkNode(sales_message);
      xmlFreeNode(sales_message);
    }
    xmlNodePtr sales_return_message = xmlNewChild(request_data, NULL, BAD_CAST "TALLYMESSAGE", NULL);
    if(!fill_cn_objects(sales_return_message, mysql_handle, export_details->from_date, 
                            export_details->to_date, thread_id, error)){
      xmlUnlinkNode(sales_message);
      xmlFreeNode(sales_message);
      if(error){
        write_response(failed_json, client);
        break;
      }
    }
    xmlNodePtr purchase_message = xmlNewChild(request_data, NULL, BAD_CAST "TALLYMESSAGE", NULL);
    if(!fill_purchase_objects(purchase_message, mysql_handle, export_details->from_date, 
                            export_details->to_date, thread_id, error)){
      xmlUnlinkNode(sales_message);
      xmlFreeNode(sales_message);
      if(error){
        write_response(failed_json, client);
        break;
      }
    }
    xmlNodePtr purchase_return_message = xmlNewChild(request_data, NULL, BAD_CAST "TALLYMESSAGE", NULL);
    if(!fill_purchase_return_objects(purchase_return_message, mysql_handle, export_details->from_date, 
                            export_details->to_date, thread_id, error)){
      xmlUnlinkNode(sales_message);
      xmlFreeNode(sales_message);
      if(error){
        write_response(failed_json, client);
        break;
      }
    }
    //xmlSaveFormatFileEnc("vouchers.xml", voucher_doc, "UTF-8", XML_SAVE_NO_DECL);
    buf = xmlBufferCreate();
    save_ctx = xmlSaveToBuffer(buf, "UTF-8", XML_SAVE_FORMAT|XML_SAVE_NO_DECL);
    xmlSaveDoc(save_ctx, voucher_doc);
    xmlSaveFlush(save_ctx);
    xmlSaveClose(save_ctx);
    modified_content = f_n_replace((const char*)buf->content,"&amp;#4;", "&#4;");
    xmlBufferFree(buf); 
    snprintf(path, 499, "%s/%s/files/_tmp_/%s/vouchers.xml",base_dir,instance,rand_str);
    fp = fopen(path, "w");
    if(!fp){
      ERROR_LOG("Unable to create tally voucher file");
      write_response(failed_json, client);
      break;
    }
    fputs(modified_content, fp);
    fclose(fp);
    free(modified_content);
    source = zip_source_file(zipper, path, 0, 0);
    if(!source){
      ERROR_LOG("Failed to add file %s to zip: %s", path, zip_strerror(zipper));
      write_response(failed_json, client);
      break;
    }
    if(zip_file_add(zipper,"vouchers.xml", source, ZIP_FL_ENC_UTF_8) < 0){
      ERROR_LOG("Failed(2) to add file %s to zip: %s", path, zip_strerror(zipper));
      write_response(failed_json, client);
      break;
    }
    memset(path,0,sizeof(path));
    snprintf(path, 400, "%s/files/_tmp_/%s.zip",g_host_uri,rand_str);
    json_t *root = json_object();
    json_object_set_new(root, "zip_uri", json_string(path));
    json_object_set_new(root, "result", json_boolean(true));
    char *json_str = json_dumps(root, 0);
    write_response(json_str, client);
    free(json_str);
    json_decref(root);

  } while(0);
  zip_close(zipper);
  free(org_name);
  xmlFreeDoc(master_doc);
  xmlFreeDoc(voucher_doc);
  xmlCleanupParser();
  snprintf(path, 499, "%s/%s/files/_tmp_/%s/masters.xml",base_dir,instance,rand_str);
  unlink(path);
  snprintf(path, 499, "%s/%s/files/_tmp_/%s/vouchers.xml",base_dir,instance,rand_str);
  unlink(path);
  snprintf(path, 499, "%s/%s/files/_tmp_/%s",base_dir,instance,rand_str);
  rmdir(path);
  delete export_details;
  delete act_rec;
}

void export_tally_xml(action_record *act_rec){
	connpool_add_work(export_tally_xml_from_db, -1, act_rec);
}

extern "C" {

void handle_export_tally_xml(json_t *root, struct client *client){
  json_t *json_handle = json_object_get(root, "sid");
  if (!json_handle || !json_is_string(json_handle)){
    write_response(invalid_json, client);
    ERROR_LOG("Required parameters missing..\n");
    return;
  }
  const char *session_id = json_string_value(json_handle);
  json_handle = json_object_get(root, "from_date");
  if (!json_handle || !json_is_integer(json_handle)){
    write_response(invalid_json, client);
    ERROR_LOG("Required parameters missing(from_date)..\n");
    return;
  }

  int from_date = json_integer_value(json_handle);
  json_handle = json_object_get(root, "to_date");
  if (!json_handle || !json_is_integer(json_handle)){
    write_response(invalid_json, client);
    ERROR_LOG("Required parameters missing(to_date)..\n");
    return;
  }
  tallyExport *export_details = new tallyExport();
  export_details->from_date = from_date;
  export_details->to_date = json_integer_value(json_handle);

  action_record* act_rec = new action_record();
  act_rec->act_type = EXPORT_TALLY_XML;
  act_rec->client = client;
  act_rec->ptr = export_details;
  exec_if_permitted(PERMISSION_VIEW_REPORTS, 3, false, session_id, act_rec);
}

};
