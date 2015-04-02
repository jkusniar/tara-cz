#include "services.h"
#include "convert.h"

#define SCHEMADIALECT <PostgreSQL/PostgreSQLSchema.h>
#define MODEL <tara-cz/taradb.sch>
#include <Sql/sch_header.h>

// SQL CONSTANTS
// ----------------------------------------------------------------
// record (invoice) data 
static const char * COL_INV_ID = "INV_ID";
static const char * COL_DATA = "DATA";
static const char * COL_PAYED = "PAYED";
static const char * COL_INV_DELIVERY_DATE = "INV_DELIVERY_DATE";
static const char * COL_INV_CREATE_DATE = "INV_CREATE_DATE";
static const char * COL_INV_PAYMENT_DATE = "INV_PAYMENT_DATE";

// Client
static const char * COL_TITLE = "TITLE";
static const char * COL_FIRST_NAME = "FIRST_NAME";
static const char * COL_LAST_NAME = "LAST_NAME";
static const char * COL_CITY = "CITY";
static const char * COL_STREET= "STREET";
static const char * COL_HOUSE_NO= "HOUSENO";
static const char * COL_CLIENT_IC= "CL_IC";
static const char * COL_CLIENT_DIC= "CL_DIC";
static const char * COL_CLIENT_IC_DPH= "CL_IC_DPH";
static const char * COL_PSC_CITY= "PSC_CITY";
static const char * COL_PSC_STR= "PSC_STR";

// patient
static const char * COL_PAT_NAME = "PAT_NAME";
static const char * COL_PAT_SPEC = "PAT_SPEC";
static const char * COL_PAT_BREED = "PAT_BREED";

// item constants
static const char * COL_ITEM_NAME = "ITEM_NAME";
static const char * COL_UNIT_NAME = "UNIT_NAME";
static const char * COL_VATRATE = "VATRATE";
static const char * COL_VAT = "VAT";
static const char * COL_PRICE_INCL_VAT = "PRICE_INCL_VAT";

InvoiceData inv;
Vector<InvoiceData> inv_list;

void prepareInvoice(Value recordId) {
	inv.clear();
	
	SQL & Select(
		// record
		INVOICE_ID.Of(RECORD).As(COL_INV_ID), 
		DATA.Of(RECORD).As(COL_DATA), 
		PAYED.Of(RECORD).As(COL_PAYED), 
		INV_DELIVERY_DATE.Of(RECORD).As(COL_INV_DELIVERY_DATE), 
		INV_CREATE_DATE.Of(RECORD).As(COL_INV_CREATE_DATE), 
		INV_PAYMENT_DATE.Of(RECORD).As(COL_INV_PAYMENT_DATE),
		// patient
		NAME.Of(PATIENT).As(COL_PAT_NAME),
		NAME.Of(LOV_SPECIES).As(COL_PAT_SPEC), 
		NAME.Of(LOV_BREED).As(COL_PAT_BREED),
		// client
		NAME.Of(LOV_TITLE).As(COL_TITLE),
		FIRST_NAME.Of(CLIENT).As(COL_FIRST_NAME),
		LAST_NAME.Of(CLIENT).As(COL_LAST_NAME),
		CITY.Of(LOV_CITY).As(COL_CITY),
		STREET.Of(LOV_STREET).As(COL_STREET),
		HOUSE_NO.Of(CLIENT).As(COL_HOUSE_NO),
		IC.Of(CLIENT).As(COL_CLIENT_IC),
		DIC.Of(CLIENT).As(COL_CLIENT_DIC),
		ICDPH.Of(CLIENT).As(COL_CLIENT_IC_DPH),
		PSC.Of(LOV_CITY).As(COL_PSC_CITY),
		PSC.Of(LOV_STREET).As(COL_PSC_STR)
	)
	.From(RECORD)
	.InnerJoin(PATIENT).On(PATIENT_ID.Of(RECORD) == ID.Of(PATIENT))
		.LeftJoin(LOV_SPECIES).On(SPECIES_ID.Of(PATIENT) == ID.Of(LOV_SPECIES))
		.LeftJoin(LOV_BREED).On(BREED_ID.Of(PATIENT) == ID.Of(LOV_BREED))
	.InnerJoin(CLIENT).On(CLIENT_ID.Of(PATIENT) == ID.Of(CLIENT))
		.LeftJoin(LOV_TITLE).On(TITLE_ID.Of(CLIENT) == ID.Of(LOV_TITLE))
		.LeftJoin(LOV_CITY).On(CITY_ID.Of(CLIENT) == ID.Of(LOV_CITY))
		.LeftJoin(LOV_STREET).On(STREET_ID.Of(CLIENT) == ID.Of(LOV_STREET))
	.Where(ID.Of(RECORD) == recordId);
	
	if (SQL.Fetch()) {
		inv.inv_id = SQL[COL_INV_ID];
		inv.create_date = SQL[COL_INV_CREATE_DATE];
		inv.delivery_date = SQL[COL_INV_DELIVERY_DATE];
		inv.payment_date = SQL[COL_INV_PAYMENT_DATE];
		inv.payed_date = SQL[COL_PAYED];
		inv.payment_type  = iptCurrency; // Hard coded, not stored in DB
		inv.rec_text = SQL[COL_DATA];
				
		inv.patient_name = SQL[COL_PAT_NAME];
		inv.patient_species = SQL[COL_PAT_SPEC];
		inv.patient_breed = SQL[COL_PAT_BREED];
		
		inv.client_title = SQL[COL_TITLE];
		inv.client_name = SQL[COL_FIRST_NAME];
		inv.client_surname = SQL[COL_LAST_NAME];
		inv.client_ic = SQL[COL_CLIENT_IC];
		inv.client_dic = SQL[COL_CLIENT_DIC];
		inv.client_icdph = SQL[COL_CLIENT_IC_DPH];
		inv.client_city = SQL[COL_CITY];
		inv.client_street = SQL[COL_STREET];
		if (!SQL[COL_PSC_CITY].IsNull())
			inv.client_zip = SQL[COL_PSC_CITY];
		else
			inv.client_zip = SQL[COL_PSC_STR];
		inv.client_house_no = SQL[COL_HOUSE_NO];
	}
	else { 
		RLOG("Nenasla sa faktura pre zaznam s id: " + AsString(recordId));
		NEVER();
	}
}

void calculateItems(Value recordId) {
	// invoice items		
	SQL & Select ( AMOUNT, PROD_PRICE, ITEM_PRICE,
				   NAME.Of ( LOV_PRODUCT ).As ( COL_ITEM_NAME ),
				   NAME.Of ( LOV_UNIT ).As ( COL_UNIT_NAME ) )
	.From ( RECORD_ITEM )
	.LeftJoin ( LOV_PRODUCT_VAT ).On ( PROD_ID.Of ( RECORD_ITEM ) == ID.Of ( LOV_PRODUCT_VAT ) )
	.LeftJoin ( LOV_PRODUCT ).On ( PROD_ID.Of ( LOV_PRODUCT_VAT ) == ID.Of ( LOV_PRODUCT ) )
	.LeftJoin ( LOV_UNIT ).On ( UNIT_ID.Of ( LOV_PRODUCT ) == ID.Of ( LOV_UNIT ) )
	.Where ( RECORD_ID.Of ( RECORD_ITEM ) == recordId )
	.OrderBy ( ID.Of ( RECORD_ITEM ) );
	
	while ( SQL.Fetch() )
	{
		VectorMap<int, String> vmap;
	
		vmap.Add ( ritProduct, AsString ( SQL[COL_ITEM_NAME] ) );
		vmap.Add ( ritUnit, AsString ( SQL[COL_UNIT_NAME] ) );
		vmap.Add ( ritAmount, ConvertLocalized().Format ( SQL[AMOUNT] ) );
		vmap.Add ( ritUnitPrice, ConvertMoney().Format ( SQL[PROD_PRICE] ) );
		vmap.Add ( ritPrice, ConvertMoney().Format ( SQL[ITEM_PRICE] ) );
	
		inv.record_items.Add ( vmap );
	}
	
	// summary price
	SQL & Select ( SqlSum ( ITEM_PRICE ) ).From ( RECORD_ITEM ).Where ( RECORD_ID == recordId );
	
	if ( SQL.Fetch() )
	{
		inv.summary_price = SQL[0];
	}
	
	else
	{
		RLOG ( "Nepodaril sa sucet poloziek faktury pre zaznam s id: " + AsString ( recordId ) );
		NEVER();
	}
}

void calculateItemsForVatPayer(Value recordId) {
	// invoice items		
	SQL & Select ( AMOUNT, PROD_PRICE, ITEM_PRICE,
				   NAME.Of ( LOV_PRODUCT ).As ( COL_ITEM_NAME ),
				   NAME.Of ( LOV_UNIT ).As ( COL_UNIT_NAME ),
				   SqlFunc ( "CALCULATE_VAT", ITEM_PRICE.Of ( RECORD_ITEM ), VALUE.Of ( LOV_VAT ) ).As ( COL_VAT ),
				   SqlFunc ( "CALCULATE_PRICE", ITEM_PRICE.Of ( RECORD_ITEM ), VALUE.Of ( LOV_VAT ) ).As ( COL_PRICE_INCL_VAT ),
				   LABEL.Of ( LOV_VAT ).As ( COL_VATRATE ) )
	.From ( RECORD_ITEM )
	.LeftJoin ( LOV_PRODUCT_VAT ).On ( PROD_ID.Of ( RECORD_ITEM ) == ID.Of ( LOV_PRODUCT_VAT ) )
	.LeftJoin ( LOV_PRODUCT ).On ( PROD_ID.Of ( LOV_PRODUCT_VAT ) == ID.Of ( LOV_PRODUCT ) )
	.LeftJoin ( LOV_UNIT ).On ( UNIT_ID.Of ( LOV_PRODUCT ) == ID.Of ( LOV_UNIT ) )
	.LeftJoin ( LOV_VAT ).On ( VAT_ID.Of ( LOV_PRODUCT_VAT ) == ID.Of ( LOV_VAT ) )
	.Where ( RECORD_ID.Of ( RECORD_ITEM ) == recordId )
	.OrderBy ( ID.Of ( RECORD_ITEM ) );
	
	while ( SQL.Fetch() )
	{
		VectorMap<int, String> vmap;
	
		vmap.Add ( ritProduct, AsString ( SQL[COL_ITEM_NAME] ) );
		vmap.Add ( ritUnit, AsString ( SQL[COL_UNIT_NAME] ) );
		vmap.Add ( ritAmount, ConvertLocalized().Format ( SQL[AMOUNT] ) );
		vmap.Add ( ritUnitPrice, ConvertMoney().Format ( SQL[PROD_PRICE] ) );
		vmap.Add ( ritPrice, ConvertMoney().Format ( SQL[ITEM_PRICE] ) );
		vmap.Add ( ritVat, ConvertMoney().Format ( SQL[COL_VAT] ) );
		vmap.Add ( ritPriceInclVat, ConvertMoney().Format ( SQL[COL_PRICE_INCL_VAT] ) );
		vmap.Add ( ritVatrate, AsString ( SQL[COL_VATRATE] ) );
	
		inv.record_items.Add ( vmap );
	}
	
	// summary price
	SQL & Select ( SqlSum ( SqlFunc ( "CALCULATE_PRICE",
				   ITEM_PRICE.Of ( RECORD_ITEM ),
				   VALUE.Of ( LOV_VAT ) ) ) )
	.From ( RECORD_ITEM )
	.InnerJoin ( LOV_PRODUCT_VAT ).On ( PROD_ID.Of ( RECORD_ITEM ) == ID.Of ( LOV_PRODUCT_VAT ) )
	.InnerJoin ( LOV_VAT ).On ( VAT_ID.Of ( LOV_PRODUCT_VAT ) == ID.Of ( LOV_VAT ) )
	.Where ( RECORD_ID == recordId );
	
	if ( SQL.Fetch() )
	{
		inv.summary_price = SQL[0];
	}
	
	else
	{
		RLOG ( "Nepodaril sa sucet poloziek faktury pre zaznam s id: " + AsString ( recordId ) );
		NEVER();
	}
	
	// VAT summary
	SQL & Select ( SqlSum ( SqlFunc ( "CALCULATE_PRICE",
				   ITEM_PRICE.Of ( RECORD_ITEM ),
				   VALUE.Of ( LOV_VAT ) ) ).As ( COL_PRICE_INCL_VAT ),
				   SqlSum ( SqlFunc ( "CALCULATE_VAT",
									  ITEM_PRICE.Of ( RECORD_ITEM ),
									  VALUE.Of ( LOV_VAT ) ) ).As ( COL_VAT ),
				   SqlSum ( ITEM_PRICE ).As ( ITEM_PRICE ),
				   LABEL.Of ( LOV_VAT ).As ( COL_VATRATE ) )
	.From ( RECORD_ITEM )
	.InnerJoin ( LOV_PRODUCT_VAT ).On ( PROD_ID.Of ( RECORD_ITEM ) == ID.Of ( LOV_PRODUCT_VAT ) )
	.InnerJoin ( LOV_VAT ).On ( VAT_ID.Of ( LOV_PRODUCT_VAT ) == ID.Of ( LOV_VAT ) )
	.Where ( RECORD_ID == recordId )
	.GroupBy ( LABEL.Of ( LOV_VAT ) )
	.OrderBy ( LABEL.Of ( LOV_VAT ) );
	
	while ( SQL.Fetch() )
	{
		VectorMap<int, String> vmap;
	
		vmap.Add ( vsitPriceInclVat, ConvertMoney().Format ( SQL[COL_PRICE_INCL_VAT] ) );
		vmap.Add ( vsitVat, ConvertMoney().Format ( SQL[COL_VAT] ) );
		vmap.Add ( vsitPrice, ConvertMoney().Format ( SQL[ITEM_PRICE] ) );
		vmap.Add ( vsitVatrate, AsString ( SQL[COL_VATRATE] ) );
	
		inv.vat_summary.Add ( vmap );
	}
}

/**
 * Find invoice by record id
 */
InvoiceData& findInvoice(Value recordId, Date vat_payer_from) {
	// invoice header data query
	prepareInvoice(recordId);
	
	if (inv.create_date >= vat_payer_from) {
		LOG("Vypocet poloziek pre platcu DPH");
		calculateItemsForVatPayer(recordId);
	}
	else {
		LOG("Vypocet poloziek neplatca");
		calculateItems(recordId);
	}
	
	return inv;
}

void findInvoicesNonVatPayer(Date date_from, Date date_to) {
	SQL & Select(
		// record
		INVOICE_ID.Of(RECORD).As(COL_INV_ID), 
		DATA.Of(RECORD).As(COL_DATA), 
		PAYED.Of(RECORD).As(COL_PAYED), 
		INV_DELIVERY_DATE.Of(RECORD).As(COL_INV_DELIVERY_DATE), 
		INV_CREATE_DATE.Of(RECORD).As(COL_INV_CREATE_DATE), 
		INV_PAYMENT_DATE.Of(RECORD).As(COL_INV_PAYMENT_DATE),
		// patient
		NAME.Of(PATIENT).As(COL_PAT_NAME),
		NAME.Of(LOV_SPECIES).As(COL_PAT_SPEC), 
		NAME.Of(LOV_BREED).As(COL_PAT_BREED),
		// client
		NAME.Of(LOV_TITLE).As(COL_TITLE),
		FIRST_NAME.Of(CLIENT).As(COL_FIRST_NAME),
		LAST_NAME.Of(CLIENT).As(COL_LAST_NAME),
		CITY.Of(LOV_CITY).As(COL_CITY),
		STREET.Of(LOV_STREET).As(COL_STREET),
		HOUSE_NO.Of(CLIENT).As(COL_HOUSE_NO),
		IC.Of(CLIENT).As(COL_CLIENT_IC),
		DIC.Of(CLIENT).As(COL_CLIENT_DIC),
		ICDPH.Of(CLIENT).As(COL_CLIENT_IC_DPH),
		PSC.Of(LOV_CITY).As(COL_PSC_CITY),
		PSC.Of(LOV_STREET).As(COL_PSC_STR),
		// record items
		AMOUNT.Of(RECORD_ITEM), 
		PROD_PRICE.Of(RECORD_ITEM), 
		ITEM_PRICE.Of(RECORD_ITEM), 
		NAME.Of(LOV_PRODUCT).As(COL_ITEM_NAME), 
		NAME.Of(LOV_UNIT).As(COL_UNIT_NAME)
	)
	.From(RECORD)
	.InnerJoin(RECORD_ITEM).On(RECORD_ID.Of(RECORD_ITEM) == ID.Of(RECORD))
		.InnerJoin(LOV_PRODUCT_VAT).On(PROD_ID.Of(RECORD_ITEM) == ID.Of(LOV_PRODUCT_VAT))
		.InnerJoin(LOV_PRODUCT).On(PROD_ID.Of(LOV_PRODUCT_VAT) == ID.Of(LOV_PRODUCT))
		.InnerJoin(LOV_UNIT).On(UNIT_ID.Of(LOV_PRODUCT) == ID.Of(LOV_UNIT))
	.InnerJoin(PATIENT).On(PATIENT_ID.Of(RECORD) == ID.Of(PATIENT))
		.LeftJoin(LOV_SPECIES).On(SPECIES_ID.Of(PATIENT) == ID.Of(LOV_SPECIES))
		.LeftJoin(LOV_BREED).On(BREED_ID.Of(PATIENT) == ID.Of(LOV_BREED))
	.InnerJoin(CLIENT).On(CLIENT_ID.Of(PATIENT) == ID.Of(CLIENT))
		.LeftJoin(LOV_TITLE).On(TITLE_ID.Of(CLIENT) == ID.Of(LOV_TITLE))
		.LeftJoin(LOV_CITY).On(CITY_ID.Of(CLIENT) == ID.Of(LOV_CITY))
		.LeftJoin(LOV_STREET).On(STREET_ID.Of(CLIENT) == ID.Of(LOV_STREET))
	.Where(NotNull(INVOICE_ID.Of(RECORD)) && (Between(REC_DATE.Of(RECORD), date_from, date_to)))
	.OrderBy(INVOICE_ID.Of(RECORD));
	
	String oldId = "";
	
	while (SQL.Fetch()) {
		// when starting new invoice
		if (!oldId.IsEqual(SQL[COL_INV_ID])) {
			InvoiceData &inv = inv_list.Add();
			
			oldId = SQL[COL_INV_ID];
			LOG("Starting invoice: " + oldId);
			
			inv.inv_id = oldId;
			inv.create_date = SQL[COL_INV_CREATE_DATE];
			inv.delivery_date = SQL[COL_INV_DELIVERY_DATE];
			inv.payment_date = SQL[COL_INV_PAYMENT_DATE];
			inv.payed_date = SQL[COL_PAYED];
			inv.payment_type  = iptCurrency; // Hard coded, not stored in DB
			inv.rec_text = SQL[COL_DATA];
					
			inv.patient_name = SQL[COL_PAT_NAME];
			inv.patient_species = SQL[COL_PAT_SPEC];
			inv.patient_breed = SQL[COL_PAT_BREED];
			
			inv.client_title = SQL[COL_TITLE];
			inv.client_name = SQL[COL_FIRST_NAME];
			inv.client_surname = SQL[COL_LAST_NAME];
			inv.client_ic = SQL[COL_CLIENT_IC];
			inv.client_dic = SQL[COL_CLIENT_DIC];
			inv.client_icdph = SQL[COL_CLIENT_IC_DPH];
			inv.client_city = SQL[COL_CITY];
			inv.client_street = SQL[COL_STREET];
			if (!SQL[COL_PSC_CITY].IsNull())
				inv.client_zip = SQL[COL_PSC_CITY];
			else
				inv.client_zip = SQL[COL_PSC_STR];
			inv.client_house_no = SQL[COL_HOUSE_NO];
		}
		
		// add item
		LOG("adding item");
		VectorMap<int, String> vmap;
		
		vmap.Add(ritProduct, AsString(SQL[COL_ITEM_NAME]));
		vmap.Add(ritUnit, AsString(SQL[COL_UNIT_NAME]));
		vmap.Add(ritAmount, ConvertLocalized().Format(SQL[AMOUNT]));
		vmap.Add(ritUnitPrice, ConvertMoney().Format(SQL[PROD_PRICE]));
		vmap.Add(ritPrice, ConvertMoney().Format(SQL[ITEM_PRICE]));

		inv_list.Top().record_items.Add(vmap);
	}
	
	// compute summary price for all invoices
	SQL & Select(
		SqlSum(ITEM_PRICE)
		//,INVOICE_ID.Of(RECORD) // for debug purpose
	)
	.From(RECORD)
		.InnerJoin(RECORD_ITEM).On(RECORD_ID.Of(RECORD_ITEM) == ID.Of(RECORD))
		.InnerJoin(PATIENT).On(PATIENT_ID.Of(RECORD) == ID.Of(PATIENT))	// safety reason - same inner join as in previous select
		.InnerJoin(CLIENT).On(CLIENT_ID.Of(PATIENT) == ID.Of(CLIENT))	// safety reason - same inner join as in previous select
	.Where(NotNull(INVOICE_ID.Of(RECORD)) && (Between(REC_DATE.Of(RECORD), date_from, date_to)))
	.GroupBy(INVOICE_ID.Of(RECORD))
	.OrderBy(INVOICE_ID.Of(RECORD));	 
	// put to results vector
	int i = 0;
	while (SQL.Fetch()) {
		inv_list[i++].summary_price = SQL[0];
	}
	
	LOG ("invoice count for non-payer: " + AsString(inv_list.GetCount()));
}

void findInvoicesVatPayer(Date date_from, Date date_to) {
		SQL & Select(
		// record
		INVOICE_ID.Of(RECORD).As(COL_INV_ID), 
		DATA.Of(RECORD).As(COL_DATA), 
		PAYED.Of(RECORD).As(COL_PAYED), 
		INV_DELIVERY_DATE.Of(RECORD).As(COL_INV_DELIVERY_DATE), 
		INV_CREATE_DATE.Of(RECORD).As(COL_INV_CREATE_DATE), 
		INV_PAYMENT_DATE.Of(RECORD).As(COL_INV_PAYMENT_DATE),
		// patient
		NAME.Of(PATIENT).As(COL_PAT_NAME),
		NAME.Of(LOV_SPECIES).As(COL_PAT_SPEC), 
		NAME.Of(LOV_BREED).As(COL_PAT_BREED),
		// client
		NAME.Of(LOV_TITLE).As(COL_TITLE),
		FIRST_NAME.Of(CLIENT).As(COL_FIRST_NAME),
		LAST_NAME.Of(CLIENT).As(COL_LAST_NAME),
		CITY.Of(LOV_CITY).As(COL_CITY),
		STREET.Of(LOV_STREET).As(COL_STREET),
		HOUSE_NO.Of(CLIENT).As(COL_HOUSE_NO),
		IC.Of(CLIENT).As(COL_CLIENT_IC),
		DIC.Of(CLIENT).As(COL_CLIENT_DIC),
		ICDPH.Of(CLIENT).As(COL_CLIENT_IC_DPH),
		PSC.Of(LOV_CITY).As(COL_PSC_CITY),
		PSC.Of(LOV_STREET).As(COL_PSC_STR),
		// record items
		AMOUNT.Of(RECORD_ITEM), 
		PROD_PRICE.Of(RECORD_ITEM), 
		ITEM_PRICE.Of(RECORD_ITEM), 
		NAME.Of(LOV_PRODUCT).As(COL_ITEM_NAME), 
		NAME.Of(LOV_UNIT).As(COL_UNIT_NAME),
		SqlFunc ( "CALCULATE_VAT", ITEM_PRICE.Of ( RECORD_ITEM ), VALUE.Of ( LOV_VAT ) ).As ( COL_VAT ),
		SqlFunc ( "CALCULATE_PRICE", ITEM_PRICE.Of ( RECORD_ITEM ), VALUE.Of ( LOV_VAT ) ).As ( COL_PRICE_INCL_VAT ),
		LABEL.Of ( LOV_VAT ).As ( COL_VATRATE )
	)
	.From(RECORD)
	.InnerJoin(RECORD_ITEM).On(RECORD_ID.Of(RECORD_ITEM) == ID.Of(RECORD))
		.InnerJoin(LOV_PRODUCT_VAT).On(PROD_ID.Of(RECORD_ITEM) == ID.Of(LOV_PRODUCT_VAT))
		.InnerJoin ( LOV_VAT ).On ( VAT_ID.Of ( LOV_PRODUCT_VAT ) == ID.Of ( LOV_VAT ) )
		.InnerJoin(LOV_PRODUCT).On(PROD_ID.Of(LOV_PRODUCT_VAT) == ID.Of(LOV_PRODUCT))
		.InnerJoin(LOV_UNIT).On(UNIT_ID.Of(LOV_PRODUCT) == ID.Of(LOV_UNIT))
	.InnerJoin(PATIENT).On(PATIENT_ID.Of(RECORD) == ID.Of(PATIENT))
		.LeftJoin(LOV_SPECIES).On(SPECIES_ID.Of(PATIENT) == ID.Of(LOV_SPECIES))
		.LeftJoin(LOV_BREED).On(BREED_ID.Of(PATIENT) == ID.Of(LOV_BREED))
	.InnerJoin(CLIENT).On(CLIENT_ID.Of(PATIENT) == ID.Of(CLIENT))
		.LeftJoin(LOV_TITLE).On(TITLE_ID.Of(CLIENT) == ID.Of(LOV_TITLE))
		.LeftJoin(LOV_CITY).On(CITY_ID.Of(CLIENT) == ID.Of(LOV_CITY))
		.LeftJoin(LOV_STREET).On(STREET_ID.Of(CLIENT) == ID.Of(LOV_STREET))
	.Where(NotNull(INVOICE_ID.Of(RECORD)) && (Between(REC_DATE.Of(RECORD), date_from, date_to)))
	.OrderBy(INVOICE_ID.Of(RECORD));
	
	String currentInvoiceId = "";
	
	while (SQL.Fetch()) {
		// when starting new invoice
		if (!currentInvoiceId.IsEqual(SQL[COL_INV_ID])) {
			InvoiceData &inv = inv_list.Add();
			
			currentInvoiceId = SQL[COL_INV_ID];
			LOG("Starting invoice: " + currentInvoiceId);
			
			inv.inv_id = currentInvoiceId;
			inv.create_date = SQL[COL_INV_CREATE_DATE];
			inv.delivery_date = SQL[COL_INV_DELIVERY_DATE];
			inv.payment_date = SQL[COL_INV_PAYMENT_DATE];
			inv.payed_date = SQL[COL_PAYED];
			inv.payment_type  = iptCurrency; // Hard coded, not stored in DB
			inv.rec_text = SQL[COL_DATA];
					
			inv.patient_name = SQL[COL_PAT_NAME];
			inv.patient_species = SQL[COL_PAT_SPEC];
			inv.patient_breed = SQL[COL_PAT_BREED];
			
			inv.client_title = SQL[COL_TITLE];
			inv.client_name = SQL[COL_FIRST_NAME];
			inv.client_surname = SQL[COL_LAST_NAME];
			inv.client_ic = SQL[COL_CLIENT_IC];
			inv.client_dic = SQL[COL_CLIENT_DIC];
			inv.client_icdph = SQL[COL_CLIENT_IC_DPH];
			inv.client_city = SQL[COL_CITY];
			inv.client_street = SQL[COL_STREET];
			if (!SQL[COL_PSC_CITY].IsNull())
				inv.client_zip = SQL[COL_PSC_CITY];
			else
				inv.client_zip = SQL[COL_PSC_STR];
			inv.client_house_no = SQL[COL_HOUSE_NO];
		}
		
		// add item
		LOG("adding item");
		VectorMap<int, String> vmap;
		
		vmap.Add(ritProduct, AsString(SQL[COL_ITEM_NAME]));
		vmap.Add(ritUnit, AsString(SQL[COL_UNIT_NAME]));
		vmap.Add(ritAmount, ConvertLocalized().Format(SQL[AMOUNT]));
		vmap.Add(ritUnitPrice, ConvertMoney().Format(SQL[PROD_PRICE]));
		vmap.Add(ritPrice, ConvertMoney().Format(SQL[ITEM_PRICE]));
		vmap.Add ( ritVat, ConvertMoney().Format ( SQL[COL_VAT] ) );
		vmap.Add ( ritPriceInclVat, ConvertMoney().Format ( SQL[COL_PRICE_INCL_VAT] ) );
		vmap.Add ( ritVatrate, AsString ( SQL[COL_VATRATE] ) );

		inv_list.Top().record_items.Add(vmap);
	}
	
	
	// VAT summary
	SQL & Select ( SqlSum ( SqlFunc ( "CALCULATE_PRICE",
				   ITEM_PRICE.Of ( RECORD_ITEM ),
				   VALUE.Of ( LOV_VAT ) ) ).As ( COL_PRICE_INCL_VAT ),
				   SqlSum ( SqlFunc ( "CALCULATE_VAT",
									  ITEM_PRICE.Of ( RECORD_ITEM ),
									  VALUE.Of ( LOV_VAT ) ) ).As ( COL_VAT ),
				   SqlSum ( ITEM_PRICE ).As ( ITEM_PRICE ),
				   LABEL.Of ( LOV_VAT ).As ( COL_VATRATE ),
				   INVOICE_ID.Of(RECORD).As(COL_INV_ID))
	.From(RECORD)
		.InnerJoin(RECORD_ITEM).On(RECORD_ID.Of(RECORD_ITEM) == ID.Of(RECORD))
		.InnerJoin(LOV_PRODUCT_VAT).On(PROD_ID.Of(RECORD_ITEM) == ID.Of(LOV_PRODUCT_VAT))
		.InnerJoin ( LOV_VAT ).On ( VAT_ID.Of ( LOV_PRODUCT_VAT ) == ID.Of ( LOV_VAT ) )
		.InnerJoin(PATIENT).On(PATIENT_ID.Of(RECORD) == ID.Of(PATIENT))	// safety reason - same inner join as in previous select
		.InnerJoin(CLIENT).On(CLIENT_ID.Of(PATIENT) == ID.Of(CLIENT))	// safety reason - same inner join as in previous select
	.Where(NotNull(INVOICE_ID.Of(RECORD)) && (Between(REC_DATE.Of(RECORD), date_from, date_to)))
	.GroupBy(INVOICE_ID.Of(RECORD), LABEL.Of ( LOV_VAT ))
	.OrderBy(INVOICE_ID.Of(RECORD), LABEL.Of ( LOV_VAT ));
	
	currentInvoiceId = "";
	int i = -1;
	while ( SQL.Fetch() ) {
		if (!currentInvoiceId.IsEqual(SQL[COL_INV_ID])) {
			currentInvoiceId = SQL[COL_INV_ID];
			i++;
			LOG("Processing VAT summary for next invoice: " + currentInvoiceId);
		}
		
		VectorMap<int, String> vmap;
		vmap.Add ( vsitPriceInclVat, ConvertMoney().Format ( SQL[COL_PRICE_INCL_VAT] ) );
		vmap.Add ( vsitVat, ConvertMoney().Format ( SQL[COL_VAT] ) );
		vmap.Add ( vsitPrice, ConvertMoney().Format ( SQL[ITEM_PRICE] ) );
		vmap.Add ( vsitVatrate, AsString ( SQL[COL_VATRATE] ) );
		inv_list[i].vat_summary.Add ( vmap );
	}
	
	
	// compute summary price for all invoices
	SQL & Select(
		SqlSum ( SqlFunc ( "CALCULATE_PRICE",
		   ITEM_PRICE.Of ( RECORD_ITEM ),
		   VALUE.Of ( LOV_VAT ) ) )
		//,INVOICE_ID.Of(RECORD) // for debug purpose
	)
	.From(RECORD)
		.InnerJoin(RECORD_ITEM).On(RECORD_ID.Of(RECORD_ITEM) == ID.Of(RECORD))
		.InnerJoin(LOV_PRODUCT_VAT).On(PROD_ID.Of(RECORD_ITEM) == ID.Of(LOV_PRODUCT_VAT))
		.InnerJoin ( LOV_VAT ).On ( VAT_ID.Of ( LOV_PRODUCT_VAT ) == ID.Of ( LOV_VAT ) )
		.InnerJoin(PATIENT).On(PATIENT_ID.Of(RECORD) == ID.Of(PATIENT))	// safety reason - same inner join as in previous select
		.InnerJoin(CLIENT).On(CLIENT_ID.Of(PATIENT) == ID.Of(CLIENT))	// safety reason - same inner join as in previous select
	.Where(NotNull(INVOICE_ID.Of(RECORD)) && (Between(REC_DATE.Of(RECORD), date_from, date_to)))
	.GroupBy(INVOICE_ID.Of(RECORD))
	.OrderBy(INVOICE_ID.Of(RECORD));	 
	// put to results vector
	i = 0;
	while (SQL.Fetch()) {
		inv_list[i++].summary_price = SQL[0];
	}
}

Vector<InvoiceData>& findInvoices(Date date_from, Date date_to, Date vat_payer_from) {
	inv_list.Clear();
	
	if (!IsNull(vat_payer_from) && vat_payer_from.IsValid() 
		&& (vat_payer_from >= date_from) && (vat_payer_from <= date_to)) {
			LOG("Platca DPH sa stal pocas intervalu");
			findInvoicesNonVatPayer(date_from, vat_payer_from.operator--());
			findInvoicesVatPayer(vat_payer_from, date_to);
	} else if (!IsNull(vat_payer_from) && vat_payer_from.IsValid() && (vat_payer_from < date_from)) {
		LOG("Platca DPH je pre cely interval");
		findInvoicesVatPayer(date_from, date_to);
	} else {
		LOG("Nie je platca DPH pocas intervalu");
		findInvoicesNonVatPayer(date_from, date_to);
	}
		
	LOG ("total invoice count: " + AsString(inv_list.GetCount()));
	return inv_list;
}

InvoiceData& findBill(Value recordId) {
	inv.clear();
	
	SQL & Select(
        // record
		INVOICE_ID.Of(RECORD).As(COL_INV_ID),
		PAYED.Of(RECORD).As(COL_PAYED), 
		// client
		NAME.Of(LOV_TITLE).As(COL_TITLE),
		FIRST_NAME.Of(CLIENT).As(COL_FIRST_NAME),
		LAST_NAME.Of(CLIENT).As(COL_LAST_NAME),
		CITY.Of(LOV_CITY).As(COL_CITY),
		STREET.Of(LOV_STREET).As(COL_STREET),
		HOUSE_NO.Of(CLIENT).As(COL_HOUSE_NO),
		IC.Of(CLIENT).As(COL_CLIENT_IC),
		DIC.Of(CLIENT).As(COL_CLIENT_DIC),
		ICDPH.Of(CLIENT).As(COL_CLIENT_IC_DPH),
		PSC.Of(LOV_CITY).As(COL_PSC_CITY),
		PSC.Of(LOV_STREET).As(COL_PSC_STR)
	)
	.From(RECORD)
	.InnerJoin(PATIENT).On(PATIENT_ID.Of(RECORD) == ID.Of(PATIENT))
	.InnerJoin(CLIENT).On(CLIENT_ID.Of(PATIENT) == ID.Of(CLIENT))
		.LeftJoin(LOV_TITLE).On(TITLE_ID.Of(CLIENT) == ID.Of(LOV_TITLE))
		.LeftJoin(LOV_CITY).On(CITY_ID.Of(CLIENT) == ID.Of(LOV_CITY))
		.LeftJoin(LOV_STREET).On(STREET_ID.Of(CLIENT) == ID.Of(LOV_STREET))
	.Where(ID.Of(RECORD) == recordId);
	
	if (SQL.Fetch()) {
		inv.inv_id = SQL[COL_INV_ID];
		inv.payed_date = SQL[COL_PAYED];
		
		inv.client_title = SQL[COL_TITLE];
		inv.client_name = SQL[COL_FIRST_NAME];
		inv.client_surname = SQL[COL_LAST_NAME];
		inv.client_ic = SQL[COL_CLIENT_IC];
		inv.client_dic = SQL[COL_CLIENT_DIC];
		inv.client_icdph = SQL[COL_CLIENT_IC_DPH];
		inv.client_city = SQL[COL_CITY];
		inv.client_street = SQL[COL_STREET];
		if (!SQL[COL_PSC_CITY].IsNull())
			inv.client_zip = SQL[COL_PSC_CITY];
		else
			inv.client_zip = SQL[COL_PSC_STR];
		inv.client_house_no = SQL[COL_HOUSE_NO];
	}
	else { 
		RLOG("Nenasiel sa prijimovy doklad pre zaznam s id: " + AsString(recordId));
		NEVER();
	}
	
	// summary price
	SQL & Select(SqlSum(ITEM_PRICE)).From(RECORD_ITEM).Where(RECORD_ID == recordId);
	if (SQL.Fetch()) {
		inv.summary_price = SQL[0];
	}
	else {
		RLOG("Nepodaril sa sucet poloziek prijimoveho dokladu pre zaznam s id: " + AsString(recordId));
		NEVER();
	}
	
	return inv;
}

InvoiceData& findRecord(Value recordId) {
	inv.clear();
	
	SQL & Select(
		// record
		REC_DATE.Of(RECORD),
		DATA.Of(RECORD),
		// patient
		NAME.Of(PATIENT).As(COL_PAT_NAME),
		NAME.Of(LOV_SPECIES).As(COL_PAT_SPEC), 
		NAME.Of(LOV_BREED).As(COL_PAT_BREED),
		// client
		NAME.Of(LOV_TITLE).As(COL_TITLE),
		FIRST_NAME.Of(CLIENT).As(COL_FIRST_NAME),
		LAST_NAME.Of(CLIENT).As(COL_LAST_NAME),
		CITY.Of(LOV_CITY).As(COL_CITY),
		STREET.Of(LOV_STREET).As(COL_STREET),
		HOUSE_NO.Of(CLIENT).As(COL_HOUSE_NO),
		IC.Of(CLIENT).As(COL_CLIENT_IC),
		DIC.Of(CLIENT).As(COL_CLIENT_DIC),
		ICDPH.Of(CLIENT).As(COL_CLIENT_IC_DPH),
		PSC.Of(LOV_CITY).As(COL_PSC_CITY),
		PSC.Of(LOV_STREET).As(COL_PSC_STR)
	)
	.From(RECORD)
	.InnerJoin(PATIENT).On(PATIENT_ID.Of(RECORD) == ID.Of(PATIENT))
		.LeftJoin(LOV_SPECIES).On(SPECIES_ID.Of(PATIENT) == ID.Of(LOV_SPECIES))
		.LeftJoin(LOV_BREED).On(BREED_ID.Of(PATIENT) == ID.Of(LOV_BREED))
	.InnerJoin(CLIENT).On(CLIENT_ID.Of(PATIENT) == ID.Of(CLIENT))
		.LeftJoin(LOV_TITLE).On(TITLE_ID.Of(CLIENT) == ID.Of(LOV_TITLE))
		.LeftJoin(LOV_CITY).On(CITY_ID.Of(CLIENT) == ID.Of(LOV_CITY))
		.LeftJoin(LOV_STREET).On(STREET_ID.Of(CLIENT) == ID.Of(LOV_STREET))
	.Where(ID.Of(RECORD) == recordId);
	
	if (SQL.Fetch()) {
		inv.create_date = SQL[REC_DATE];
		inv.rec_text = SQL[DATA];
				
		inv.patient_name = SQL[COL_PAT_NAME];
		inv.patient_species = SQL[COL_PAT_SPEC];
		inv.patient_breed = SQL[COL_PAT_BREED];
		
		inv.client_title = SQL[COL_TITLE];
		inv.client_name = SQL[COL_FIRST_NAME];
		inv.client_surname = SQL[COL_LAST_NAME];
		inv.client_ic = SQL[COL_CLIENT_IC];
		inv.client_dic = SQL[COL_CLIENT_DIC];
		inv.client_icdph = SQL[COL_CLIENT_IC_DPH];
		inv.client_city = SQL[COL_CITY];
		inv.client_street = SQL[COL_STREET];
		if (!SQL[COL_PSC_CITY].IsNull())
			inv.client_zip = SQL[COL_PSC_CITY];
		else
			inv.client_zip = SQL[COL_PSC_STR];
		inv.client_house_no = SQL[COL_HOUSE_NO];
	}
	else { 
		RLOG("Nenasiel sa zaznam s id: " + AsString(recordId));
		NEVER();
	}
	
	return inv;
}
