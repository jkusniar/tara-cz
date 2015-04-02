#include "constants.h"
#include "mainwin.h"
#include "convert.h"
#include "services.h"

RecordsWin::RecordsWin(int lang)
{
	CtrlLayoutOKCancel(*this, t_("Patient's records"));
	
	this->lang = lang;
	
	recordbox.SetLabel(t_("Record"));
	//text.SetFont(Font().Face(Font::ROMAN).Height(12));
	
	// hide SKK related controls if not required
	if (lang != LANG_SK)
	{
		skk.Show(false);
		skk_lbl.Show(false);
	}
	
	// no bill for CZ implementation
	if (lang == LANG_CZ)
		print_bill.Enable(false);
	
	records.AddIndex(ID);
	records.AddColumn(REC_DATE, t_("Date"));
	records.SetToolBar(true, BarCtrl::BAR_TOP);
	records.WhenChangeRow = THISBACK(when_records_change_row);
	records.WhenInsertRow = THISBACK(when_records_new_inserted);
	
	items.AddIndex(ID);
	items.AddColumn(PROD_ID, t_("Product")).Edit(product).SetConvert(product).Width(239);
	items.AddColumn(AMOUNT, t_("Amount")).Edit(amount).SetConvert(Single<ConvertAmount>()).Width(65);
	items.AddColumn(PROD_PRICE, t_("Unit price")).Edit(price).SetConvert(Single<ConvertMoney>()).Width(95);
	items.AddColumn(UNIT_ID, t_("Unit")).Edit(unit.SetEditable(false)).SetConvert(unit).Width(60);
	items.AddColumn(ITEM_PRICE, t_("Item price")).Edit(final_price.SetEditable(false)).SetConvert(Single<ConvertMoney>()).Width(80);
	
	items.SetToolBar(true, BarCtrl::BAR_TOP);
	items.WhenInsertRow = THISBACK(when_item_inserted);
	items.WhenUpdateRow = THISBACK(when_item_updated);
	items.WhenRemoveRow = THISBACK(when_item_deleted);
	product.WhenAction = THISBACK(when_product_changes);
	amount.WhenAction = THISBACK(when_recompute_price);
	price.WhenAction = THISBACK(when_recompute_price);
	product.ColorRows(true);
	
	// load LOVs
	::Add(product, product_cache.get()); // products
	::Add(unit, unit_cache.get()); // units
	
	is_new = false; // no new record inserted
	
	text.WhenBar = THISBACK(when_text_menu);
	
	print_inv.WhenAction = THISBACK(when_print_inv);
	
	print_bill.WhenAction = THISBACK(when_print_bill);
	
	print_rec.WhenAction = THISBACK(when_print_record);
	
	sum_dbl = 0.0;
}

void RecordsWin::when_records_change_row()
{
	checksave_record_data(records.GetPrevCursorId());
	text.Clear();
	
	// select other record data
	SQL & Select(DATA.Of(RECORD), INVOICE_ID.Of(RECORD), PAYED.Of(RECORD))
		.From(RECORD)
		.Where(ID.Of(RECORD) == records(ID));
	
	while(SQL.Fetch())
	{
		text.SetData(SQL[DATA]);
		
		if (SQL[INVOICE_ID].IsNull())
		{
			inv_id.SetText(t_("New"));
			print_bill.Enable(false);
			print_inv.Enable(false);
		}
		else
		{
			inv_id.SetText(AsString(SQL[INVOICE_ID]));
			if (lang != LANG_CZ)
				print_bill.Enable(true);
			print_inv.Enable(true);
		}
		
		if (SQL[PAYED].IsNull())
			payment_date.SetText(t_("Not payed"));
		else
			payment_date.SetText(AsString(SQL[PAYED]));
	}
	
	//select record items
	items.Clear();
	SQL & Select(ID.Of(RECORD_ITEM), PROD_ID.Of(RECORD_ITEM), AMOUNT.Of(RECORD_ITEM), PROD_PRICE.Of(RECORD_ITEM),
		UNIT_ID.Of(LOV_PRODUCT), ITEM_PRICE.Of(RECORD_ITEM))
		.From(RECORD_ITEM)
		.LeftJoin(LOV_PRODUCT).On(PROD_ID.Of(RECORD_ITEM) == ID.Of(LOV_PRODUCT))
		.Where(RECORD_ID.Of(RECORD_ITEM) == records(ID))
		.OrderBy(ID.Of(RECORD_ITEM));
	
	sum_dbl = 0.0;
	while(SQL.Fetch())
	{
		items.Add(SQL);
		sum_dbl += (double)SQL[ITEM_PRICE];
	}
	
	//compute sum
	sum.SetText(AsString(ConvertMoney().Format(sum_dbl)));
	skk.SetText(convertEurToSkk(sum_dbl));
}

void RecordsWin::when_records_new_inserted()
{
	checksave_record_data(records.GetPrevCursorId());
	
	if (!is_new)
	{
		text.Clear();
		items.Clear();
		is_new = true;
		records(REC_DATE) = ::GetSysTime();
		// records.GetToolBar().GetFirstChild()->Enable(false); // iny sposob bez is_new
		
		SQL & Insert(RECORD)
			(PATIENT_ID, patient)
	    	(REC_DATE, records(REC_DATE))
	    	//(DATA, text.GetData()) // not filled yet
		;
			
		// !!! postgresql 8.2> ma v inserte klauzulu RETURNING, ktora vrati pozadovany stlpec rovno
		// pri inserte a nemusime ho takto blbo vracat
		SQL & Select(SqlMax(ID)).From(RECORD);
		if (SQL.Fetch())
			records.Set(ID, SQL[0]);
		
		sum.SetText("0,00");
		skk.SetText("0,00");
		sum_dbl = 0.0;
		
		inv_id.SetText(t_("New"));
		payment_date.SetText(t_("Not payed"));
		print_bill.Enable(false);
		print_inv.Enable(false);
		
		// mark new record using bold font
		records.GetRow(records.GetCursor()).SetFont(Font::GetStdFont().Bold(true));
	}
	else
	{
		records.RemoveLast();
		PromptOK(t_("Cannot insert more than one record."));
	}
}

void RecordsWin::when_product_changes()
{
	// select product data from LOV table
	SQL & Select(UNIT_ID, PRICE)
		.From(LOV_PRODUCT)
		.Where(ID.Of(LOV_PRODUCT) == product.GetKey());

	/// fill gridctrl and controls
	while(SQL.Fetch())
	{
		amount.SetData(1.0);
		price.SetData(SQL[PRICE]);
		final_price.SetData(SQL[PRICE]);
		// change unit on product change
		//unit.Set(ID, SQL[UNIT_ID]);
		
		items(AMOUNT) = 1.0;
		items(PROD_PRICE) = SQL[PRICE];
		items(ITEM_PRICE) = SQL[PRICE];
		items(UNIT_ID) = SQL[UNIT_ID];
	}
	
	sum_items();
	sum.SetText(AsString(ConvertMoney().Format(sum_dbl)));
	skk.SetText(convertEurToSkk(sum_dbl));
}

void RecordsWin::when_recompute_price()
{
	final_price = custom_round_double(amount * price, -2);
	items(ITEM_PRICE) = final_price.GetData();
	
	sum_items();
	sum.SetText(AsString(ConvertMoney().Format(sum_dbl)));
	skk.SetText(convertEurToSkk(sum_dbl));
}

void RecordsWin::when_print_inv()
{
	InvoiceChoiceWin inv_ch_dlg(lang);
	
	// prefill invoice choice dialog from DB
	SQL & Select(PAYED, INV_DELIVERY_DATE, INV_CREATE_DATE, INV_PAYMENT_DATE)
		.From(RECORD)
		.Where(ID == records(ID));
		
	if (SQL.Fetch()) {
		if (SQL[PAYED].IsNull())
			inv_ch_dlg.payed_now.SetData(false);
		
		inv_ch_dlg.delivery_date.SetData(SQL[INV_DELIVERY_DATE]);
		inv_ch_dlg.creation_date.SetData(SQL[INV_CREATE_DATE]);
		inv_ch_dlg.payment_date.SetData(SQL[INV_PAYMENT_DATE]);
	}
	
	if (inv_ch_dlg.Run(true)) {
		if (~inv_ch_dlg.payed_now) {
			SQL & ::Update(RECORD)
				(PAYED, ~inv_ch_dlg.payment_date)
				(INV_DELIVERY_DATE, ~inv_ch_dlg.delivery_date)
				(INV_CREATE_DATE, ~inv_ch_dlg.creation_date)
				(INV_PAYMENT_DATE, ~inv_ch_dlg.payment_date)
				(DATA, text.GetData())
			.Where(ID == records(ID));
			
			payment_date.SetText(~AsString(~inv_ch_dlg.payment_date));
		}
		else {
			SQL & ::Update(RECORD)
				(INV_DELIVERY_DATE, ~inv_ch_dlg.delivery_date)
				(INV_CREATE_DATE, ~inv_ch_dlg.creation_date)
				(INV_PAYMENT_DATE, ~inv_ch_dlg.payment_date)
				(DATA, text.GetData())
			.Where(ID == records(ID));
		}
		
		Perform(formatter->formatFullInvoice(findInvoice(records(ID), formatter->vat_payer_from))); // TODO spravne volanie podla platby DPH
		/*
			static FileSel fs;
			static bool b;
			if(!b) {
				fs.Type(t_("PDF file"), "*.pdf");
				fs.AllFilesType();
			}
			if(!fs.ExecuteSaveAs(t_("Output PDF file")))
				return;
			SaveFile(~fs, UPP::Pdf(inv.prepare()));
			// print do PDF prebrany z ReportDlg. V report dlg sa objavi tlacitko len vtedy ked je
			// do Report package pridany package PdfDraw ako zavislost!
			// najlepsie bude toto naimplementovat si sam do svojho programu
		*/
	}
}

void RecordsWin::when_print_bill()
{
	WithPaymentDateWinLayout<TopWindow> payDtDlg;
	CtrlLayoutOK(payDtDlg, t_("Payment date"));
	
	payDtDlg.payment_date.SetData(GetSysDate());
	SQL & Select(PAYED).From(RECORD).Where(ID == records(ID));
	if (SQL.Fetch()) {
		if (!SQL[PAYED].IsNull())
			payDtDlg.payment_date.SetData(SQL[PAYED]);
	}
	
	if (payDtDlg.Run(true)) {
		SQL & ::Update(RECORD)
			(PAYED, ~payDtDlg.payment_date)
		.Where(ID == records(ID));
		
		payment_date.SetText(~AsString(~payDtDlg.payment_date));
		
		Perform(formatter->formatBill(findBill(records(ID))));
	}
}

void RecordsWin::when_print_record()
{
	checksave_record_data(records.GetCursorId());
	Perform(formatter->formatRecord(findRecord(records(ID))));
}

void RecordsWin::when_item_inserted()
{
	SQL & Insert(RECORD_ITEM)
		(RECORD_ID, records(ID))
		(PROD_ID, items(PROD_ID))
		(AMOUNT, items(AMOUNT))
		(ITEM_PRICE, items(ITEM_PRICE))
		(PROD_PRICE, items(PROD_PRICE))
	;
	
	
	// TODO rework RETURNING ID in previous select
	SQL & Select(SqlMax(ID)).From(RECORD_ITEM);
	if (SQL.Fetch())
		items.Set(ID, SQL[0]);
	
	sum_items();
	sum.SetText(AsString(ConvertMoney().Format(sum_dbl)));
	skk.SetText(convertEurToSkk(sum_dbl));
}

void RecordsWin::when_item_updated()
{
	SQL & ::Update(RECORD_ITEM)
		(PROD_ID, items(PROD_ID))
		(AMOUNT, items(AMOUNT))
		(ITEM_PRICE, items(ITEM_PRICE))
		(PROD_PRICE, items(PROD_PRICE))
	.Where(ID == items(ID));
	
	sum_items();
	sum.SetText(AsString(ConvertMoney().Format(sum_dbl)));
	skk.SetText(convertEurToSkk(sum_dbl));
}

void RecordsWin::when_item_deleted()
{
	SQL & Delete(RECORD_ITEM).Where(ID == items(ID));
	
	sum_items(items.GetRowId());
	sum.SetText(AsString(ConvertMoney().Format(sum_dbl)));
	skk.SetText(convertEurToSkk(sum_dbl));
}

void RecordsWin::sum_items(int removed_idx)
{
	sum_dbl = 0.0;
	for (int i=0; i<items.GetCount(); i++)
		sum_dbl += (removed_idx != i) ? (double) items.Get(i, ITEM_PRICE) : 0.0;
}

void RecordsWin::when_text_menu(Bar& menu)
{
	menu.Add(t_("Phrases"), THISBACK(when_phrases_menu));
}

void RecordsWin::when_phrases_menu(Bar& menu)
{
	const VectorMap<Value, Value> &vm = phrase_cache.get();
	for (int i=0; i < vm.GetCount(); i++)
	{
		menu.Add(AsString(vm[i]), THISBACK1(apply_phrase ,vm.GetKey(i)));
	}
	
	menu.Separator();
	menu.Add(t_("Add new phrase.."), THISBACK(when_new_phrase));
}

void RecordsWin::apply_phrase(int id)
{
	SQL & Select(PHRASE_TEXT).From(LOV_PHRASE).Where(ID == id);
	if (SQL.Fetch())
		text.Insert(text.GetCursor(),  AsString(SQL[0]));
}

void RecordsWin::when_new_phrase()
{
	WithNewPhraseLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, t_("New phrase"));
	if (dlg.Run(true) == IDOK)
	{
		// check if exists
		SQL & Select(SqlCountRows())
			.From(LOV_PHRASE)
			.Where(NAME == ~dlg.name);
			
		if (SQL.Fetch() && SQL[0] == 0)
		{
			SQL & Insert(LOV_PHRASE)
				(NAME, ~dlg.name)
				(PHRASE_TEXT, ~dlg.phrase_text)
			;
		
			phrase_cache.refresh();
			
			//rovno vloz text novej frazy
			text.Insert(text.GetCursor(),  AsString(~dlg.phrase_text));
		}
		else
			PromptOK(Format(t_("Phrase %s already exists."), ~dlg.name));

	}
}

void RecordsWin::checksave_record_data(int recordsCursorId)
{
	// save data to record if it has been changed
	if (text.IsDirty()&& recordsCursorId >= 0) {

		SQL & ::Update(RECORD)
		(DATA, text.GetData())
		.Where(ID == records.Get(recordsCursorId, ID));
		
		text.ClearDirty();
	}
}

/*
 * to round 123.125 to 123.13 use custom_round_double(123.125, -2)
 */
double RecordsWin::custom_round_double(double d, int n)
{
	return floor(d / ipow10(n) + 0.5) * ipow10(n);
}

// conversion utilities
String convertEurToSkk(double eur)
{
	return ConvertMoney().Format((eur * 30.1260));
}

void RecordsWin::setInvoiceFormatter(InvoiceFormatter* fmt)
{
	formatter = fmt;
}