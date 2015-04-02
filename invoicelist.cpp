#include "constants.h"
#include "mainwin.h"
#include "invoice.h"
#include "convert.h"

InvoiceList::InvoiceList(int lang, Date date_from, Date date_to)
{
	this->lang = lang;
	
	from = date_from;
	to = date_to;
	
	// init invoice data
	LoadFromFile(inv_data, "tara_invoice.cfg");
}

Report& InvoiceList::report()
{
	return invoiceList;
}

bool InvoiceList::prepare()
{
	String sqlStatement = "select r.invoice_id, r.inv_create_date, \
					(select sum(ri.item_price) from record_item ri where ri.record_id = r.id) \
					from record r \
					where r.invoice_id is not null and r.inv_create_date between '"
					 + AsString(from) + "' and '" + AsString(to) 
					 + "' order by r.inv_create_date, r.invoice_id";
	SQL.Execute(sqlStatement);
	
	// najdi faktury za dane obdobie:
	/*
	SQL & Select(INV_CREATE_DATE, INVOICE_ID)
		.From(RECORD)
		.Where(NotNull(INVOICE_ID) && (Between(INV_CREATE_DATE, from, to)))
		.OrderBy(INV_CREATE_DATE, INVOICE_ID);
	*/
	
	bool found = false;
	while (SQL.Fetch()) {
		found = true;
		
		VectorMap<int, String> vmap;
		vmap.Add(iliInvNum, AsString(SQL[0]));
		vmap.Add(iliDate, AsString(SQL[1]));
		//vmap.Add(iliTotal, AsString(SQL[2]));
		vmap.Add(iliTotal, fixFuckedLinuxFormating(ConvertMoney().Format(SQL[2])));
		inv_list_items.Add(vmap);
	}
	// ak nic nenaslo, koncime
	if (!found) return false;
	
	// suma vsetkych
	SQL & Select(SqlSum(ITEM_PRICE.Of(RECORD_ITEM))).From(RECORD_ITEM)
		.InnerJoin(RECORD).On(RECORD_ID.Of(RECORD_ITEM) == ID.Of(RECORD))
		.Where(NotNull(INVOICE_ID.Of(RECORD)) && (Between(INV_CREATE_DATE.Of(RECORD), from, to)));
	if (SQL.Fetch())
		summary_price = SQL[0];	
	
	// priprav report
	invoiceList.Clear();
	invoiceList.Header(String("[A0> ") + String(t_("Page")) + String(" $$P"));
	
	StringBuffer buf;
	
	buf.Cat("{{1f4 ");
	formatHeader(buf);
	buf.Cat(":: ");
	formatCompanyData(buf);
	buf.Cat(":: ");
	formatItems(buf);
	buf.Cat("}}");
	
	LOG(~buf);
	invoiceList << ~buf;

	return true;
}

void InvoiceList::formatHeader(StringBuffer &buf)
{
	buf.Cat("{{1:1~ ");
	buf.Cat("[*A3 " + AsString(t_("Invoice list")) + "] :: [*A3 ]:: ");
	buf.Cat("[A1 " + AsString(t_("Date from:")) + "-|" + AsString(from));
	buf.Cat("&" + AsString(t_("Date to:")) + "-|" + AsString(to));
	buf.Cat("]:: ");
	buf.Cat("}}");
}

void InvoiceList::formatCompanyData(StringBuffer &buf)
{
	buf.Cat("[*A1 " + AsString(t_("Supplier:")) + "&]");
	buf.Cat("[A1 {{1:1~ ");
	buf.Cat("[A1 " + AsString(~inv_data.name) + "&" + AsString(~inv_data.address));
	buf.Cat("&" + AsString(t_("pho.")) + " 1:-|" + AsString(~inv_data.phone1) + "&" + AsString(t_("pho.")) + " 2:-|" + AsString(~inv_data.phone2) +"]:: ");
	buf.Cat("[A1 " + AsString(t_("IC:")) + "-|-|-|" + AsString(~inv_data.ic) + "&" + AsString(t_("DIC:")) + "-|-|-|" );
	buf.Cat( AsString(~inv_data.dic) + "&");
	if (lang != LANG_CZ)
		buf.Cat(AsString(t_("ICDPH:")) + "-|-|-|" + AsString(~inv_data.icdph) + "&");
	buf.Cat(AsString(t_("Bank account:")) + "-|-|" + AsString(~inv_data.acc_num) +"] ");
	buf.Cat("}}] ");
}

void InvoiceList::formatItems(StringBuffer &buf)
{
	buf.Cat("[A1 {{1:2:2:2f4 ");
	buf.Cat("[*A1 " + AsString(t_("N.")) + "] :: [*A1 " + AsString(t_("Create date")) + "] :: ");
	buf.Cat("[*A1 " + AsString(t_("Invoice num")) + "] :: [*A1 " + AsString(t_("Invoice total")) + "]");
	
	for (int i=0; i<inv_list_items.GetCount(); i++)
	{
		const VectorMap<int, String> &vmap = inv_list_items[i];
		buf.Cat(":: ");
		buf.Cat(AsString(i+1)); buf.Cat(":: ");
		buf.Cat(vmap.Get(iliDate)); buf.Cat(":: ");
		buf.Cat(vmap.Get(iliInvNum)); buf.Cat(":: ");
		buf.Cat(vmap.Get(iliTotal) + AsString(t_("CUR")));
	}
	
	buf.Cat("::-2 ");
	buf.Cat("[*A1> " + AsString(t_("Invoice list total:"))); buf.Cat(" :: :: ]:: ");
	buf.Cat("[*A1 " + fixFuckedLinuxFormating(ConvertMoney().Format(summary_price)));
	buf.Cat(" " + AsString(t_("CUR")) +  "]");
	buf.Cat("}}");
	
	//buf.Cat("}}");
	//buf.Cat("&[*A1 " + AsString(t_("Invoice list total")) + " -|");
	//buf.Cat(InvoiceFormatter::fixFuckedLinuxFormating(ConvertMoney().Format(summary_price)));
	//buf.Cat(" " + AsString(t_("CUR")) +  "]");
}
