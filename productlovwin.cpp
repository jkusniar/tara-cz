#include "mainwin.h"
#include "convert.h"

static const char * COL_VATRATE = "VATRATE";
static const char * COL_PRICE_INCL_VAT = "PRICE_INCL_VAT";
static const char * COL_PROD_ID = "COL_PROD_ID";

ProductLovWin::ProductLovWin(bool is_vat_payer)
{
	vat_payer = is_vat_payer;
	products.AddIndex(ID);
	products.AddColumn(NAME, t_("Name")).Edit(name.MaxChars(100));
	products.AddColumn(UNIT_ID, t_("Unit")).Edit(unit).SetConvert(unit);
	products.AddColumn(PRICE, (vat_payer ? t_("Base price") : t_("Price"))).Edit(price).SetConvert(Single<ConvertMoney>());
	if (vat_payer) {
		products.AddColumn(COL_VATRATE, t_("VAT rate")).Edit(vatrate).SetConvert(vatrate);
		products.AddColumn(COL_PRICE_INCL_VAT, t_("Price incl. VAT")).Edit(price_incl_vat.SetEditable(false)).SetConvert(Single<ConvertMoney>());
		products.AddColumn(VALID_FROM, t_("Valid from")).Edit(valid_from.SetEditable(false));
		products.AddColumn(VALID_TO, t_("Valid to")).Edit(valid_to);
	}
	
	products.SetToolBar(true, BarCtrl::BAR_TOP);
	products.WhenInsertRow = THISBACK(when_insert);
	products.WhenUpdateRow = THISBACK(when_update);
	products.WhenStartEdit = THISBACK(when_start_edit);
	//products. = THISBACK(when_cancel_new_vatrate); 			// TODO co sem?

	products_default_menu = products.WhenMenuBar;
	products.WhenMenuBar = THISBACK(when_products_menu);
	
	unit.AddPlus(THISBACK(when_new_unit));
	unit.ColorRows(true);
	::Add(unit, ID, NAME, LOV_UNIT);

	if (vat_payer) {
		vatrate.AddPlus(THISBACK(when_new_vatrate));
		vatrate.ColorRows(true);
		::Add(vatrate, ID, LABEL, LOV_VAT);
	
		SQL & Select(ID.Of(LOV_PRODUCT_VAT), NAME, UNIT_ID, PRICE, 
			VAT_ID.Of(LOV_PRODUCT_VAT).As(COL_VATRATE),
			SqlFunc("CALCULATE_PRICE", PRICE, VALUE.Of(LOV_VAT)).As(COL_PRICE_INCL_VAT),
			VALID_FROM, VALID_TO)
			.From(LOV_PRODUCT)
			.InnerJoin ( LOV_PRODUCT_VAT ).On ( ID.Of ( LOV_PRODUCT ) == PROD_ID.Of ( LOV_PRODUCT_VAT ) )
			.InnerJoin ( LOV_VAT ).On ( VAT_ID.Of ( LOV_PRODUCT_VAT ) == ID.Of ( LOV_VAT ) )
			.OrderBy(NAME.Of(LOV_PRODUCT), VALID_FROM.Of(LOV_PRODUCT_VAT));
	} else {
		SQL & Select(ID.Of(LOV_PRODUCT_VAT), NAME, UNIT_ID, PRICE)
		.From(LOV_PRODUCT)
		.InnerJoin ( LOV_PRODUCT_VAT ).On ( ID.Of ( LOV_PRODUCT ) == PROD_ID.Of ( LOV_PRODUCT_VAT ) )
		.OrderBy(NAME);
	}
	while(SQL.Fetch())
	      products.Add(SQL);

	if (!products.IsEmpty())
		products.SetCursor(0);
	
	// TODO na zmenu vatrate, alebo zakladnej ceny prepocitaj vyslednu cenu produktu
}

void ProductLovWin::when_products_menu(Bar &menu) {
	products_default_menu(menu);
	if (!products.IsEmpty() && vat_payer)
		menu.Add(t_("New VAT rate for product.."), THISBACK(when_new_vatrate_for_product));
}

void ProductLovWin::when_start_edit() {
	if (vat_payer && !IsNull(products.Get(VALID_TO))) {
		name.SetEditable(false);
		unit.SetEditable(false);
		price.SetEditable(false);
		vatrate.SetEditable(false);
		valid_to.SetEditable(false);
	} else {
		name.SetEditable(true);
		unit.SetEditable(true);
		price.SetEditable(true);
		vatrate.SetEditable(true);
		valid_to.SetEditable(true);
	}
}

void ProductLovWin::when_new_vatrate_for_product() {
	int original_row_num = products.GetCurrentRow();
	
	if (original_row_num >= 0) {
		int new_row_num = original_row_num +1;
		products.Insert(new_row_num);
		products.SetCursor(new_row_num);
		
		products.Set(new_row_num, NAME, products.Get(original_row_num, NAME));
		products.Set(new_row_num, UNIT_ID, products.Get(original_row_num, UNIT_ID));
		products.Set(new_row_num, PRICE, products.Get(original_row_num, PRICE));
		products.Set(new_row_num, VALID_FROM, GetSysDate());
		original_product_id = products.Get(original_row_num, ID);
		
		products.StartEdit();
	}
}

void ProductLovWin::when_cancel_new_vatrate() {
	PromptOK("TADAA");
}

void ProductLovWin::when_insert()
{
	if (original_product_id != -1) {
		PromptOK("ADD VATRATE SAVE");
		SQL & Insert (LOV_PRODUCT_VAT)
	    	(VAT_ID, products(COL_VATRATE))
	    	(PROD_ID, original_product_id)
	    	(VALID_FROM, products(VALID_FROM));
		
		original_product_id = -1;
	} else {
		PromptOK("ADD PRODUCT SAVE");
		SQL & Insert(LOV_PRODUCT)
	    	(NAME, products(NAME))
	    	(UNIT_ID, products(UNIT_ID))
	    	(PRICE, products(PRICE));
	    int prod_id = SQL.GetInsertedId();
	    
	    if (vat_payer) {   
	    	SQL & Insert (LOV_PRODUCT_VAT)
	    		(VAT_ID, products(COL_VATRATE))
	    		(PROD_ID, prod_id)
	    		(VALID_FROM, products(VALID_FROM));
		} else {
	    	SQL & Select(ID).From(LOV_VAT).Where(LABEL == "0%");
	    	int vat_id = 1;
	    	if (SQL.Fetch())
	    		vat_id = SQL[0];
	    
		    SQL & Insert (LOV_PRODUCT_VAT)
		    	(VAT_ID, vat_id)
	    		(PROD_ID, prod_id)
	    		(VALID_FROM, GetSysDate());
		}
	}
	
	products.Set(ID, SQL.GetInsertedId());
}

void ProductLovWin::when_update()
{
	SQL & ::Update(LOV_PRODUCT)
		(NAME, products(NAME))
	    (UNIT_ID, products(UNIT_ID))
	    (PRICE, products(PRICE))
	.Where(ID == Select(PROD_ID).From(LOV_PRODUCT_VAT).Where(ID == products(ID)));
	
	if (vat_payer) {
		SQL & ::Update(LOV_PRODUCT_VAT)
			(VAT_ID, products(COL_VATRATE))
	    	(VALID_TO, products(VALID_TO))
		.Where(ID == products(ID));
	}
}

void ProductLovWin::when_new_unit()
{
	WithInputLovLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, t_("New unit"));
	dlg.label.SetText(t_("Unit name:"));
	if(dlg.Run(true) == IDOK)
	{
		// check whether unit allready exists
		SQL & Select(SqlCountRows()).From(LOV_UNIT).Where(NAME == ~dlg.name);
		if(SQL.Fetch() && SQL[0] == 0)
		{
			// if not, insert and refresh unit cache
			SQL & Insert(LOV_UNIT)
				(NAME, ~dlg.name);
			
			::Add(unit, ID, NAME, LOV_UNIT);
			
			Value new_id = SQL.GetInsertedId();
			products.Set(UNIT_ID, new_id);
		}
		else
			PromptOK(Format(t_("Unit %s already exists."), ~dlg.name));
			
	}
}

void ProductLovWin::when_new_vatrate()
{
	WithAddVatrateLayout<TopWindow> dlg;
	dlg.value.SetConvert ( Single<ConvertMoney>() );
	CtrlLayoutOKCancel ( dlg, t_ ( "New VAT rate" ) );

	if ( dlg.Run ( true ) == IDOK )
	{
		SQL & Select ( SqlCountRows() ).From ( LOV_VAT ).Where ( LABEL == ~dlg.label );

		if ( SQL.Fetch() && SQL[0] == 0 )
		{
			SQL & Insert ( LOV_VAT )
			( LABEL, ~dlg.label )
			( VALUE, ~dlg.value )
			( NOTE, ~dlg.note );
			
			::Add ( vatrate, ID, LABEL, LOV_VAT );
			
			Value new_id = SQL.GetInsertedId();
			products.Set ( COL_VATRATE, new_id );
		}

		else
		{
			PromptOK ( Format ( t_ ( "VAT rate %s already exists." ), ~dlg.label ) );
		}
	}
}

