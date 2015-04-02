#include "constants.h"
#include "invoice.h"
#include "convert.h"

void InvoiceData::clear() {
    /*
    inv_id = NULL;
    create_date= NULL;
    delivery_date= NULL;
    payment_date= NULL;
    payed_date= NULL;
    name= NULL;
    address= NULL;
    phone1= NULL;
    phone2= NULL;
    vet_ic= NULL;
    vet_dic= NULL;
    vet_icdph= NULL;
    bank_acc= NULL;
    client_title= NULL;
    client_name= NULL;
    client_surname= NULL;
    client_street= NULL;
    ...
    */
    record_items.Clear();
    vat_summary.Clear();
	summary_price = 0.0;
}


Report& InvoiceFormatter::formatFullInvoice(InvoiceData& invoice) {
	output.Clear();

	output.Header(String("[A0> ") + String(t_("Page")) + String(" $$P]"));
	
	StringBuffer buf;
	
	buf.Cat("{{1f4 ");
	formatHeader(buf, invoice);
	buf.Cat(":: ");
	formatCompanyData(buf);
	buf.Cat(":: ");
	formatClientData(buf, invoice);
	buf.Cat(":: ");
	formatPatientData(buf, invoice);
	buf.Cat(":: ");
	if (invoice.create_date >= vat_payer_from) {
		formatInvoiceItemsVATPayer(buf, invoice);
	} else {
		formatInvoiceItems(buf, invoice);
	}
	buf.Cat("}}");

	if (!AsString(invoice.payed_date).IsEmpty() && lang != LANG_CZ)
	{
		buf.Cat("&&&&");
		buf.Cat("{{1f4 ");
		formatBillHeader(buf, invoice, false);
		buf.Cat(":: ");
		formatBillPrice(buf, invoice);
		buf.Cat("}}");
	}

	output << ~buf;

	return output;
}

Report& InvoiceFormatter::formatFullInvoices(Vector<InvoiceData>& invoices) {
	output.Clear();
	
	output.Header(String("[A0> ") + String(t_("Page")) + String(" $$P]"));
	
	for (int i=0; i<invoices.GetCount(); i++) {
		StringBuffer buf;

		buf.Cat("[ ");
		
		buf.Cat("{{1f4 ");
		formatHeader(buf, invoices[i]);
		buf.Cat(":: ");
		formatCompanyData(buf);
		buf.Cat(":: ");
		formatClientData(buf, invoices[i]);
		buf.Cat(":: ");
		formatPatientData(buf, invoices[i]);
		buf.Cat(":: ");
		if (invoices[i].create_date >= vat_payer_from) {
			formatInvoiceItemsVATPayer(buf, invoices[i]);
		} else {
			formatInvoiceItems(buf, invoices[i]);
		}
		buf.Cat("}}");
	
		if (!AsString(invoices[i].payed_date).IsEmpty() && lang != LANG_CZ)
		{
			buf.Cat("&&&&");
			buf.Cat("{{1f4 ");
			formatBillHeader(buf, invoices[i], false);
			buf.Cat(":: ");
			formatBillPrice(buf, invoices[i]);
			buf.Cat("}}");
		}
		
		buf.Cat(" ]");
		
		output << ~buf;
		
		if (i != invoices.GetCount()-1)
			output.NewPage();
	}
	
	return output;
}

Report& InvoiceFormatter::formatBill(InvoiceData& invoice) {
	output.Clear();
	
	output.Header(String("[A0> ") + String(t_("Page")) + String(" $$P]"));
	
	StringBuffer buf;
	buf.Cat("{{1f4 ");
	formatBillHeader(buf, invoice);
	buf.Cat(":: ");
	formatCompanyData(buf);
	buf.Cat(":: ");
	formatClientData(buf, invoice);
	buf.Cat(":: ");
	formatBillPrice(buf, invoice);
	buf.Cat("}}");
	
	output << ~buf;
	
	return output;
}

Report& InvoiceFormatter::formatRecord(InvoiceData& invoice) {
	output.Clear();
	
	output.Header(String("[A0> ") + String(t_("Page")) + String(" $$P]"));
	
	StringBuffer buf;
	buf.Cat("{{1f4 ");
	formatRecordHeader(buf, invoice);
	buf.Cat(":: ");
	formatCompanyData(buf);
	buf.Cat(":: ");
	formatClientData(buf, invoice);
	buf.Cat(":: ");
	formatPatientData(buf, invoice);
	buf.Cat("}}");
	
	output << ~buf;
	
	return output;
}

void InvoiceFormatter::formatHeader(StringBuffer &buf, InvoiceData &invoice) {
	buf.Cat("{{1:1~ ");
	buf.Cat("[*A3 " + AsString(t_("Invoice")) + "] :: [*A3 " + AsString(t_("invoice num:")) + " " + AsString(invoice.inv_id) + "]:: ");
	buf.Cat("[A1 " + AsString(t_("Create date:")) + "-|-|-|" + AsString(invoice.create_date));
	if (lang != LANG_CZ) {
		buf.Cat("&" + AsString(t_("Delivery date:")) + "-|-|" + AsString(invoice.delivery_date) + "&");
		buf.Cat(AsString(t_("Payment date:")) + "-|-|-|" + AsString(invoice.payment_date) );
	}
	buf.Cat("]:: ");
	
	// no payment type in CZ
	if (lang != LANG_CZ) {
		String type;
		switch (invoice.payment_type)
		{
				case iptCurrency:
					type = t_("currency");
					break;
				case iptBankTransfer:
					type = t_("bank transfer");
					break;
				
				default:
					type = t_("currency");
		}
		
		buf.Cat("[A1 " + AsString(t_("Payment type:")) + "-|-|" + type + "]&");
	}
	buf.Cat("}}");
}

void InvoiceFormatter::formatCompanyData(StringBuffer &buf) {
	buf.Cat("[*A1 " + AsString(t_("Supplier:")) + "&]");
	buf.Cat("[A1 {{1:1~ ");
	buf.Cat("[A1 " + vet_name + "&" + vet_address);
	buf.Cat("&" + AsString(t_("pho.")) + " 1:-|" + vet_phone1 + "&" + AsString(t_("pho.")) + " 2:-|" + vet_phone2 +"]:: ");
	buf.Cat("[A1 " + AsString(t_("IC:")) + "-|-|-|" + vet_ic + "&" + AsString(t_("DIC:")) + "-|-|-|" );
	buf.Cat( vet_dic + "&");
	if (lang != LANG_CZ)
		buf.Cat(AsString(t_("ICDPH:")) + "-|-|-|" + vet_icdph + "&");
	buf.Cat(AsString(t_("Bank account:")) + "-|" + vet_bank_acc +"] ");
	buf.Cat("}}] ");
}

void InvoiceFormatter::formatClientData(StringBuffer &buf, InvoiceData &invoice) {
	buf.Cat("[*A1 " + AsString(t_("Client:")) + "&]");
	buf.Cat("[A1 {{1:1~ ");
	buf.Cat("[A1 ");
	if (!invoice.client_title.IsEmpty())
		buf.Cat(invoice.client_title + " ");
	buf.Cat(invoice.client_name + " " + invoice.client_surname + "&");
	if(invoice.client_street.IsEmpty())
		buf.Cat(invoice.client_city + " " + invoice.client_house_no + "&" + invoice.client_zip);
	else
		buf.Cat(invoice.client_street + " " + invoice.client_house_no 
			+ "&" + invoice.client_zip + " " + invoice.client_city);
	buf.Cat("]:: ");
	buf.Cat("[A1 " + AsString(t_("IC:")) + "-|-|-|" + invoice.client_ic 
		+ "&" + AsString(t_("DIC:")) + "-|-|-|" );
	buf.Cat(invoice.client_dic);
	if (lang != LANG_CZ)
		buf.Cat("&" + AsString(t_("ICDPH:")) + "-|-|-|" + invoice.client_icdph + "] ");
	buf.Cat("}}] ");
}

void InvoiceFormatter::formatPatientData(StringBuffer &buf, InvoiceData &invoice) {
	buf.Cat("[*A1 " + AsString(t_("Patient:")) + "] [A1 " 
		+ invoice.patient_name + ", " + invoice.patient_species);
	if (!invoice.patient_breed.IsEmpty())
		buf.Cat(" (" + invoice.patient_breed + ")");
	buf.Cat("&]");
	buf.Cat("[A1 " + escapeText(invoice.rec_text) + "] ");
}

void InvoiceFormatter::formatInvoiceItems(StringBuffer &buf, InvoiceData &invoice) {
	buf.Cat("[A1 {{1:4:2:2:2:2f4 ");
	buf.Cat("[*A1 " + AsString(t_("N.")) + "] :: [*A1 " + AsString(t_("Description")) + "] :: ");
	buf.Cat("[*A1 " + AsString(t_("Unit")) + "] :: [*A1 " + AsString(t_("Amount")) + "]");
	buf.Cat(" :: [*A1 " + AsString(t_("Unit price")) + "] :: [*A1 " + AsString(t_("Sum")) + "] ");
	for (int i=0; i<invoice.record_items.GetCount(); i++)
	{
		const VectorMap<int, String> &vmap = invoice.record_items[i];
		buf.Cat(":: ");
		buf.Cat(AsString(i+1)); buf.Cat(":: ");
		buf.Cat(vmap.Get(ritProduct)); buf.Cat(":: ");
		buf.Cat(vmap.Get(ritUnit)); buf.Cat(":: ");
		buf.Cat(fixFuckedLinuxFormating(vmap.Get(ritAmount))); buf.Cat(":: ");
		buf.Cat(fixFuckedLinuxFormating(vmap.Get(ritUnitPrice))); buf.Cat(" " + AsString(t_("CUR")) + ":: ");
		buf.Cat(fixFuckedLinuxFormating(vmap.Get(ritPrice))); buf.Cat(" " + AsString(t_("CUR")) + " ");
	}
	buf.Cat("}}&&");
	buf.Cat("[*A1 " + AsString(t_("Total invoiced:")) + " -|");
	buf.Cat(fixFuckedLinuxFormating(ConvertMoney().Format(invoice.summary_price)));
	buf.Cat(" " + AsString(t_("CUR")) +  "]]");

	if (lang == LANG_CZ) {
		buf.Cat("&[*A1 " + AsString(t_("Total payed:")) + " -|");
		buf.Cat(fixFuckedLinuxFormating(ConvertMoney().Format(invoice.summary_price)));
		buf.Cat(" " + AsString(t_("CUR")) +  "]");
		buf.Cat("&&");
	}
	
	buf.Cat("&[*A1 " + AsString(t_("Supplier is not subject to VAT")) + "]");
	
	if (lang == LANG_CZ) {
		buf.Cat("&&[A0> Vystavil: " + AsString(vet_name) + "]");
	}
}

void InvoiceFormatter::formatInvoiceItemsVATPayer(StringBuffer &buf, InvoiceData &invoice) {
	buf.Cat("[A1 {{1:4:2:2:2:2:1:2f4 ");
	buf.Cat("[*A1 " + AsString(t_("N.")) + "] :: [*A1 " + AsString(t_("Description")) + "] :: ");
	buf.Cat("[*A1 " + AsString(t_("Unit")) + "] :: [*A1 " + AsString(t_("Amount")) + "]");
	buf.Cat(" :: [*A1 " + AsString(t_("Unit price")) + "] :: [*A1 " + AsString(t_("Base price")) + "] ");
	buf.Cat(" :: [*A1 " + AsString(t_("VAT rate")) + "] :: [*A1 " + AsString(t_("Price incl. VAT")) + "] ");
	for (int i=0; i<invoice.record_items.GetCount(); i++)
	{
		const VectorMap<int, String> &vmap = invoice.record_items[i];
		buf.Cat(":: ");
		buf.Cat(AsString(i+1)); buf.Cat(":: ");
		buf.Cat(vmap.Get(ritProduct)); buf.Cat(":: ");
		buf.Cat(vmap.Get(ritUnit)); buf.Cat(":: ");
		buf.Cat(fixFuckedLinuxFormating(vmap.Get(ritAmount))); buf.Cat(":: ");
		buf.Cat(fixFuckedLinuxFormating(vmap.Get(ritUnitPrice))); buf.Cat(" " + AsString(t_("CUR")) + ":: ");
		buf.Cat(fixFuckedLinuxFormating(vmap.Get(ritPrice))); buf.Cat(" " + AsString(t_("CUR")) + ":: ");
		buf.Cat(vmap.Get(ritVatrate)); buf.Cat(":: ");
		buf.Cat(fixFuckedLinuxFormating(vmap.Get(ritPriceInclVat))); buf.Cat(" " + AsString(t_("CUR")) + " ");
	}
	buf.Cat("}}&&");
	buf.Cat("[*A1 " + AsString(t_("Total invoiced:")) + " -|");
	buf.Cat(fixFuckedLinuxFormating(ConvertMoney().Format(invoice.summary_price)));
	buf.Cat(" " + AsString(t_("CUR")) +  "]]");

	if (lang == LANG_CZ) {
		buf.Cat("&[*A1 " + AsString(t_("Total payed:")) + " -|");
		buf.Cat(fixFuckedLinuxFormating(ConvertMoney().Format(invoice.summary_price)));
		buf.Cat(" " + AsString(t_("CUR")) +  "]");
		buf.Cat("&&");
	}
	
	buf.Cat("[A1 {{1:2:2:2f4 ");
	buf.Cat("[*A1 " + AsString(t_("VAT rate")) + "] :: [*A1 " + AsString(t_("Base price")) + "] ");
	buf.Cat(" :: [*A1 " + AsString(t_("VAT")) + "] :: [*A1 " + AsString(t_("Price incl. VAT")) + "] ");
	for (int i=0; i<invoice.vat_summary.GetCount(); i++) {
		const VectorMap<int, String> &vmap = invoice.vat_summary[i];
		buf.Cat(":: ");
		buf.Cat(vmap.Get(vsitVatrate)); buf.Cat(":: ");
		buf.Cat(fixFuckedLinuxFormating(vmap.Get(vsitPrice))); buf.Cat(" " + AsString(t_("CUR")) + ":: ");
		buf.Cat(fixFuckedLinuxFormating(vmap.Get(vsitVat))); buf.Cat(" " + AsString(t_("CUR")) + ":: ");
		buf.Cat(fixFuckedLinuxFormating(vmap.Get(vsitPriceInclVat))); buf.Cat(" " + AsString(t_("CUR")) + " ");
	}
	buf.Cat("}} ");
	
	if (lang == LANG_CZ) {
		buf.Cat("&&[A0> Vystavil: " + AsString(vet_name) + "]");
	}
}

void InvoiceFormatter::formatBillHeader(StringBuffer &buf, InvoiceData &invoice, bool isStandelone) {
	if (isStandelone)
		buf.Cat("[*A3 " + AsString(t_("Receipt")) + "&]");
	else
		buf.Cat("[*A2 " + AsString(t_("Receipt")) + "&]");
	buf.Cat("[A1 " + AsString(t_("Payment of the invoice:")) +  " ");
	buf.Cat(AsString(invoice.inv_id));
	buf.Cat("&&" + AsString(t_("Payed date:")) + " ");
	buf.Cat(AsString(invoice.payed_date));
	buf.Cat("] ");
}

void InvoiceFormatter::formatBillPrice(StringBuffer &buf, InvoiceData &invoice)
{
	buf.Cat("[*A1 " + AsString(t_("Amount payed:")) + " -|-|");
	buf.Cat(fixFuckedLinuxFormating(ConvertMoney().Format(invoice.summary_price)));
	buf.Cat(" " + AsString(t_("CUR")) + "]&");
	/*
	if (lang == LANG_SK)
	{
		buf.Cat("[A1 Uhradená suma v SKK: -|"); 	//SKK
		buf.Cat(fixFuckedLinuxFormating(convertEurToSkk(summary_price)));
		buf.Cat("]&");								//SKKend
	}
	*/
	buf.Cat("[A1 " + AsString(t_("Amount in words:")) + " ");
	
	//double int_part;
	//modf(summary_price, &int_part);
	//buf.Cat(convert((unsigned long)int_part));
	
	String xxx = AsString(invoice.summary_price);
	int idx = xxx.Find('.');
	int d;
	String s = xxx;
	if (idx > 0)
		s = xxx.Left(idx);
	sscanf(~s, "%d", &d);
	
	//int d = ffloor(summary_price);
	// TODO konstruuj podla nastavenia jazyka
	NumberToTextConverter *converter = new SlovakNumberToTextConverter();
	buf.Cat(converter->convert(d));
	delete ((SlovakNumberToTextConverter *)converter);
	
	buf.Cat(" " + AsString(t_("CUR")) + "] ");
	/**
	if (lang == LANG_SK)
	{
		buf.Cat("&&[A0> Konverzný kurz 30,1260 SKK/EUR]");
	}
	*/
}

void InvoiceFormatter::formatRecordHeader(StringBuffer &buf, InvoiceData &invoice)
{
	buf.Cat("[*A3 " + AsString(t_("Patient's record")) +  "&][A1 " + AsString(t_("Date:")) + " ");
	buf.Cat(AsString(invoice.create_date));
	buf.Cat("] ");
}

String InvoiceFormatter::escapeText(String& s) {
	StringBuffer out;
	
	out.Cat("\1");
	out.Cat(s);
	out.Cat("\1");
	
	return ~out;
}

// TOTO opravuje chyby formatovania pri 1000 (za 1 ide medzera)
String  fixFuckedLinuxFormating(const String &s) {
	/*
	LOG(s);
	for (int i=0; i<s.GetCount(); i++) {
		LOG(AsString(i) + String(" = '") + AsString(s[i]) + String("', "));	
	}
	*/
	StringBuffer out;
	
	#ifdef PLATFORM_LINUX
	for (int i=0; i<s.GetCount(); i++)
		out.Cat((s[i] == -96) ? ' ' : s[i]);
	#endif
	
	#ifdef PLATFORM_WIN32
	for (int i=0; i<s.GetCount(); i++)
	{
		if (s[i] != -62)
			out.Cat((s[i] == -96) ? ' ' : s[i]);
	}
	#endif
	
	return ~out;
}
