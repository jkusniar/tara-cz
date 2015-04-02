#include "constants.h"
#include "mainwin.h"
#include "services.h"
#include "invoice.h"
#include "convert.h"

#define IMAGECLASS Images
#define IMAGEFILE <tara-cz/tara-cz.iml>
#include <Draw/iml_source.h>

#include <Core/Hash.h>

MainWin::MainWin(String dbname)
{
	String title = String("Tara " + VERSION + " - db:") + dbname;
	CtrlLayout(*this, ~title);
	Sizeable().Zoomable().MinimizeBox().Icon(Images::boxer_icon).LargeIcon(Images::boxer_icon);
	
	//default settings
	splitter_position = 5000;	// 50% of window
	lang = LANG_SK; // Slovensky
}

void MainWin::WindowSetup()
{
	// prepare invoice formatter based on invoice info
	prepareInvoiceFormatter();
	
	// main window layout
	AddFrame(menu);							// main menu
	AddFrame(TopSeparatorFrame());			// separator
	AddFrame(toolbar);						// toolbar
	splitter.Vert(clients, patients);		// clients/patients GridCtrls splitted
	Add(splitter.SizePos());
	splitter.SetPos(splitter_position);
	
	menu.Set(THISBACK(when_main_menu));		// menu setup
	toolbar.Set(THISBACK(when_toolbar));	// toolbar setup
	
	// clients GridCtrl
	clients.AddIndex(ID);
	clients.AddColumn(TITLE_ID, t_("Title")).Edit(title).SetConvert(title);//.Width(70);
	clients.AddColumn(FIRST_NAME, t_("FName")).Edit(first_name.MaxChars(50));//.Width(90);
	clients.AddColumn(LAST_NAME, t_("Surname")).Edit(last_name.NotNull().MaxChars(100));//.Width(120);
	clients.AddColumn(CITY_ID, t_("City")).Edit(city.NotNull()).SetConvert(city);//.Width(120);
	clients.AddColumn(STREET_ID, t_("Street")).Edit(street).SetConvert(street);//.Width(130);
	clients.AddColumn(HOUSE_NO, t_("House No.")).Edit(house_no.MaxChars(10));//.Width(70);
	clients.AddColumn(PSC, t_("ZIP")).Editable(false);//.Width(60);
	clients.AddColumn(PHONE_1, t_("Phone 1")).Edit(phone1.MaxChars(20));//.Width(105);
	clients.AddColumn(PHONE_2, t_("Phone 2")).Edit(phone2.MaxChars(20));//.Width(95);
	clients.AddColumn(EMAIL, t_("Email")).Edit(email.MaxChars(100));//.Width(85);
	clients.AddColumn(NOTE, t_("Note")).Edit(note.MaxChars(100));//.Width(85);
	clients.AddColumn(IC, t_("IC")).Edit(ic.MaxChars(20));//.Width(75);
	clients.AddColumn(DIC, t_("DIC")).Edit(dic.MaxChars(20));//.Width(75);
	clients.AddColumn(ICDPH, t_("ICDPH")).Edit(icdph.MaxChars(20)).Hidden(lang == LANG_CZ);//.Width(75);
	//clients.SetToolBar(true, BarCtrl::BAR_TOP);
	clients.WhenInsertRow = THISBACK(when_insert_client);
	clients.WhenUpdateRow = THISBACK(when_update_client);
	clients.WhenStartEdit = THISBACK(fill_street);
	clients.WhenChangeRow = THISBACK(when_client_change_row);
	
	city.WhenAction = THISBACK(when_change_city);
	street.WhenAction = THISBACK(when_change_street);
	title.AddPlus(THISBACK(when_new_title));
	city.ColorRows(true);
	street.ColorRows(true);
	title.ColorRows(true);
	first_name.WhenAction = THISBACK(when_change_first_name);
	last_name.WhenAction =THISBACK(when_change_last_name);
	
	// patients GridCtrl
	patients.AddIndex(ID);
	patients.AddColumn(NAME, t_("FName")).Edit(name.NotNull().MaxChars(100));
	patients.AddColumn(BIRTH_DATE, t_("Birth date")).Edit(birthdate);
	patients.AddColumn(SPECIES_ID, t_("Species")).Edit(species).SetConvert(species);
	patients.AddColumn(BREED_ID, t_("Breed")).Edit(breed).SetConvert(breed);
	patients.AddColumn(SEX_ID, t_("Sex")).Edit(sex).SetConvert(sex);
	patients.AddColumn(NOTE, t_("Note")).Edit(pnote.MaxChars(100));
	patients.AddColumn(IS_DEAD, t_("Is dead")).Edit(is_dead.NotNull()).SetDisplay(Single<DispIsDead>());
	patients.AddColumn(LYSSET, t_("LyssetNum")).Edit(lysset);
	patients.AddColumn(ID, t_("Id")).Editable(false);
	patients.SetToolBar(true, BarCtrl::BAR_TOP);
	patients.WhenInsertRow = THISBACK(when_insert_patient);
	patients.WhenUpdateRow = THISBACK(when_update_patient);
	patients.WhenStartEdit = THISBACK(fill_breed);
	species.WhenAction = THISBACK(fill_breed);
	patients.WhenChangeRow = THISBACK(fill_breed_all);
	patients_default_menu = patients.WhenMenuBar;
	patients.WhenMenuBar = THISBACK(when_patiens_menu);
	name.WhenAction = THISBACK(when_change_patient_name);
	lysset <<= THISBACK(when_edit_lysset);
	
	species.AddPlus(THISBACK(when_new_species));
	species.ColorRows(true);
	breed.AddPlus(THISBACK(when_new_breed));
	breed.ColorRows(true);
	
	// fill controls from DB
	fill_controls();
}

void MainWin::fill_controls()
{
	//fill DropGrid controls from LOV cache for clients
	::Add(title, title_cache.get()); // titles
	// cities
	city.Clear();
	for (int i=0; i<city_cache.GetCount(); i++)
	{
		city.Add(city_cache.GetKey(i), city_cache[i].Get(ctCITY));
	}
	fill_street_all();						// streets
	
	//fill DropGrid controls from LOV cache for patients
	::Add(sex, sex_cache.get()); 			// sex
	::Add(species, species_cache.get()); 	// species
	fill_breed_all(); 						// breed
	
	// fill list of clients
	when_find_client();
}

void MainWin::StoreSetup()
{
	splitter_position = splitter.GetPos();
}

void MainWin::Serialize(Stream &s)
{
	SerializePlacement(s);
	s % splitter_position;
	s % lang;
}

void MainWin::when_main_menu(Bar& menu)
{
	menu.Add(t_("&File"), THISBACK(when_file_menu));
	menu.Add(t_("Tools"), THISBACK(when_tools_menu));
	menu.Add(t_("Find"), THISBACK(when_search_menu));
	menu.Add(t_("Statistics"), THISBACK(when_stats_menu));
	menu.Add(t_("&Help"), THISBACK(when_help_menu));
}

void MainWin::when_file_menu(Bar& menu)
{
	menu.Add(t_("E&xit"), THISBACK(Close));
}

void MainWin::when_tools_menu(Bar& menu)
{
	menu.Add(t_("LOVs.."), THISBACK(when_lov_dialog));
	menu.Add(t_("Invoice data.."), THISBACK(when_inv_data_dlg));
	menu.Add(t_("Options.."), THISBACK(when_options));
	menu.Separator();
	menu.Add(t_("Reset accounting.."), THISBACK(when_reset_accounting));
}

void MainWin::when_search_menu(Bar& menu)
{
	menu.Add(t_("Find invoice.."), THISBACK(when_find_invoice));
	menu.Add(t_("Find by lysset.."), THISBACK(when_find_by_lysset));
	menu.Add(t_("Find by patient.."), THISBACK(when_find_by_patient));
	menu.Add(t_("Invoice mass print.."), THISBACK(when_invoice_mass_print));
}

void MainWin::when_stats_menu(Bar& menu)
{
	menu.Add(t_("Invoice list.."), THISBACK(when_invoice_list));
	menu.Add(t_("Complex statistics.."), THISBACK(when_complex_statistics));
	//menu.Add(t_("Non-payers.."), THISBACK(when_dummy));
}

void MainWin::when_help_menu(Bar& menu)
{
	menu.Add(t_("About.."), THISBACK(when_about));
}

void MainWin::when_about()
{
	PromptOK("[2 TARA - veterinárny software - verzia " + VERSION 
			+ " && [^https://github.com/jkusniar/tara-cz^ https://github.com/jkusniar/tara-cz] && "\
			" [^https://github.com/jkusniar/tara-cz/blob/master/LICENSE^ LICENSE] && "\
			" [1 copyright (c) 2009-2015 Ján Kušniar <jkusniar@gmail.com>]]");
}

void MainWin::when_toolbar(Bar& bar)
{
	Size btnsize; btnsize.cx = 20; btnsize.cy = 20;
	bar.Add(new_client_btn, btnsize); new_client_btn.Image(CtrlImg::Plus()).Tip(t_("New Client"));
	new_client_btn <<= clients.StdInsert;

	bar.ToolSeparator();
	
	Size fsize; fsize.cx = 200; fsize.cy = 20;
	bar.Add(find, fsize);
	find <<= THISBACK(when_find_client);
}

void  MainWin::when_options()
{
	WithOptionsLayout<TopWindow> dlg;
	CtrlLayoutOK(dlg, t_("Options"));
	dlg.lang.Add(Value(LANG_ENG), "English").Add(Value(LANG_SK), "Slovensky").Add(Value(LANG_CZ), "Česky").SetIndex(lang);
	if(dlg.Execute() != IDOK)
		return;
	lang = ~dlg.lang;
}

void MainWin::prepareInvoiceFormatter() {	
	InvoiceDataWin inv_data;
	LoadFromFile(inv_data, "tara_invoice.cfg");
	
	formatter.vet_name = ~inv_data.name;
	formatter.vet_address = ~inv_data.address;
	formatter.vet_phone1 = ~inv_data.phone1;
	formatter.vet_phone2 = ~inv_data.phone2;
	formatter.vet_ic = ~inv_data.ic;
	formatter.vet_dic = ~inv_data.dic;
	formatter.vet_icdph = ~inv_data.icdph;
	formatter.vet_bank_acc = ~inv_data.acc_num;
	formatter.vat_payer_from = ~inv_data.vat_from;
	formatter.lang = lang;
}

void MainWin::find_client(SqlSelect &sel)
{
	clients.Clear();
	SQL & sel;

	while(SQL.Fetch())
	{
		clients.Add(SQL);
		
		// set ZIP code from LOV cache according to city/street
		String zip = city_cache.Get(SQL[CITY_ID]).Get(ctPSC);
		if (!zip.IsEmpty())
			clients(PSC) = zip;
		else
		{
			zip = street_cache.Get(SQL[CITY_ID]).Get(SQL[STREET_ID]).Get(stPSC);
			clients(PSC) = zip;
		}
	}
			
	// select first record from clients in GridCtrl (if any)
	if (!clients.IsEmpty()) {
		patients.Appending(true);
		clients.SetCursor(0);
	}
	else {
		patients.Appending(false);
		patients.Clear();
	}
	
	// fix new version of GridCtrl
	find.SetFocus();
	find.SetSelection(find.GetText().GetCount());
	// endfix
}

void MainWin::when_find_client()
{
	SqlSelect sel = Select(ID, TITLE_ID, FIRST_NAME, LAST_NAME, CITY_ID, STREET_ID, HOUSE_NO, HOUSE_NO,
		 PHONE_1, PHONE_2, EMAIL, NOTE, IC, DIC, ICDPH)
			.From(CLIENT)
			.Where(Like(LAST_NAME, Wild(Format("*%1:s*", ~find)), false /* case insensitive like */ ))
			.OrderBy(LAST_NAME)
			.Limit(MAX_CLIENTS_DISPLAYED);
	
	find_client(sel);
}

void MainWin::when_insert_client()
{
	SQL & Insert(CLIENT)
	    (FIRST_NAME, clients(FIRST_NAME))
	    (LAST_NAME, clients(LAST_NAME))
	    (TITLE_ID, clients(TITLE_ID))
	    (PHONE_1, clients(PHONE_1))
	    (PHONE_2, clients(PHONE_2))
	    (EMAIL, clients(EMAIL))
	    (CITY_ID, clients(CITY_ID))
	    (STREET_ID, clients(STREET_ID))
	    (HOUSE_NO, clients(HOUSE_NO))
	    (NOTE, clients(NOTE))
	    (IC, clients(IC))
	    (DIC, clients(DIC))
	    (ICDPH, clients(ICDPH))
	;
	
	SQL & Select(SqlMax(ID)).From(CLIENT);
	if (SQL.Fetch())
		clients.Set(ID, SQL[0]);
	
	patients.Clear();
}

void MainWin::when_update_client()
{
	SQL & ::Update(CLIENT)
		(FIRST_NAME, clients(FIRST_NAME))
		(LAST_NAME, clients(LAST_NAME))
		(TITLE_ID, clients(TITLE_ID))
		(PHONE_1, clients(PHONE_1))
		(PHONE_2, clients(PHONE_2))
		(EMAIL, clients(EMAIL))
		(CITY_ID, clients(CITY_ID))
		(STREET_ID, clients(STREET_ID))
		(HOUSE_NO, clients(HOUSE_NO))
		(NOTE, clients(NOTE))
		(IC, clients(IC))
	    (DIC, clients(DIC))
	    (ICDPH, clients(ICDPH))
	.Where(ID == clients(ID));
}

void MainWin::when_client_change_row()
{
	fill_street_all();
	
	patients.Clear();
	SQL & Select(ID, NAME, BIRTH_DATE, SPECIES_ID, BREED_ID, SEX_ID, NOTE, IS_DEAD, LYSSET, ID)
		.From(PATIENT)
		.Where(CLIENT_ID.Of(PATIENT) == clients(ID))
		.OrderBy(NAME);
	
	while(SQL.Fetch())
	{
		patients.Add(SQL);
	}
	
	patients.WhenChangeRow();
}

void MainWin::when_insert_patient()
{
	SQL & Insert(PATIENT)
		(CLIENT_ID, clients(ID))
	    (NAME, patients(NAME))
	    (BIRTH_DATE, patients(BIRTH_DATE))
	    (SPECIES_ID, patients(SPECIES_ID))
	    (BREED_ID, patients(BREED_ID))
	    (SEX_ID, patients(SEX_ID))
	    (NOTE, patients(NOTE))
	    (IS_DEAD, patients(IS_DEAD))
	    (LYSSET, patients(LYSSET))
	;
	
	SQL & Select(SqlMax(ID)).From(PATIENT);
	if (SQL.Fetch()) {
		patients.Set(ID, SQL[0]);
		patients.Set(9, SQL[0]);	// set also read-only column with ID displayed
	}
}

void MainWin::when_update_patient()
{
	SQL & ::Update(PATIENT)
	    (NAME, patients(NAME))
	    (BIRTH_DATE, patients(BIRTH_DATE))
	    (SPECIES_ID, patients(SPECIES_ID))
	    (BREED_ID, patients(BREED_ID))
	    (SEX_ID, patients(SEX_ID))
	    (NOTE, patients(NOTE))
	    (IS_DEAD, patients(IS_DEAD))
	    (LYSSET, patients(LYSSET))
	.Where(ID == patients(ID));
}

void MainWin::fill_street()
{
	const VectorMap<int, VectorMap<int, String> > dummy;
	const VectorMap<int, VectorMap<int, String> > &vm = street_cache.Get(city.GetKey(), dummy);
	street.Clear();
	if (!vm.IsEmpty())
	{
		for (int i=0; i<vm.GetCount(); i++)
			street.Add(vm.GetKey(i), vm[i].Get(stSTREET));
	}
	
	// street field required if streets for city available
	if (street.IsEmpty())
		street.NotNull(false);
	else
		street.NotNull();
}

void MainWin::fill_street_all()
{
	street.Clear();
	for (int i=0; i < street_cache.GetCount(); i++)
	{
		const VectorMap<int, VectorMap<int, String> > &vm = street_cache[i];
		if (!vm.IsEmpty())
		{
			for (int i=0; i<vm.GetCount(); i++)
				street.Add(vm.GetKey(i), vm[i].Get(stSTREET));
		}
	}
}

void MainWin::when_change_city()
{
	if (!clients.IsEmpty() && city.IsSelected())
	{
		fill_street();
		
		clients(PSC) = "";
		String zip = city_cache.Get(city.GetKey()).Get(ctPSC);
		if (!zip.IsEmpty())
			clients(PSC) = zip;
	}
}
	
void MainWin::when_change_street()
{
	if (!clients.IsEmpty() && !city.GetKey().IsNull() && !street.GetKey().IsNull())
	{
		String zip = street_cache.Get(city.GetKey()).Get(street.GetKey()).Get(stPSC);
		clients(PSC) = zip;
	}
}

void MainWin::fill_breed()
{
	::Add(breed, breed_cache.get(species.GetKey()));
}

void MainWin::fill_breed_all()
{
	::Add(breed, breed_cache.getAll());
}

void MainWin::when_patiens_menu(Bar &menu)
{
	patients_default_menu(menu);
	if (!patients.IsEmpty())
		menu.Add(t_("Patient's records.."), THISBACK(when_patients_records));
}

void MainWin::when_patients_records()
{	
	RecordsWin recordswin(lang);
	recordswin.setInvoiceFormatter(&formatter);
		
	// fetch all patient's records
	SQL & Select(ID.Of(RECORD), REC_DATE.Of(RECORD), DATA.Of(RECORD), INVOICE_ID.Of(RECORD), PAYED.Of(RECORD))
		.From(RECORD)
		.Where(PATIENT_ID.Of(RECORD) == patients(ID))
		.OrderBy(REC_DATE.Of(RECORD));
	
	while(SQL.Fetch())
	{
		recordswin.records.Add(SQL);
		recordswin.inv_id.SetText(AsString(SQL[INVOICE_ID]));
		if (SQL[PAYED].IsNull())
		{
			recordswin.payment_date.SetText(t_("Not payed"));
		}
		else
		{
			recordswin.payment_date.SetText(AsString(SQL[PAYED]));
		}
	}
	
	// select last record
	if (!recordswin.records.IsEmpty())
		recordswin.records.SetCursor(recordswin.records.GetCount()-1);
	
	// set client and patient ID
	recordswin.client = clients(ID);
	recordswin.patient = patients(ID);
	
	// show dialog window
	int ret = recordswin.Run(true);
	
	if (recordswin.is_new)
	{
		if (ret == IDOK)
		{
			// select last item (new record) - required in case other record was selected and OK button hit
			// also check if text of previous selection is saved and 
			// load necessary new record data to dialog controls
			recordswin.records.SetCursor(recordswin.records.GetCount() - 1);
			// necessary if we press OK while other than new item is selected
			recordswin.checksave_record_data(recordswin.records.GetCursor());
			
			//print record only
			if (recordswin.items.IsEmpty()) {
				Perform(formatter.formatRecord(findRecord(recordswin.records(ID))));
			}
			// create and print invoice
			else
			{
				if (PromptYesNo(t_("Create invoice?")) == IDOK)
				{
					InvoiceChoiceWin inv_ch_dlg(lang); // invoice choice dialog
					if(inv_ch_dlg.Run(true))
					{
						String inv_id = "X";
						SQL & Select(NextVal("invoice_seq")).Get();
						if (SQL.Fetch())
							inv_id = Format("%1:04d%2:06d", GetSysDate().year, SQL[0]);
						
						// generate invoice Id
						if (inv_ch_dlg.payed_now)
							SQL & ::Update(RECORD)
					    		(INVOICE_ID, inv_id)
					    		(PAYED, GetSysDate())
					    		(INV_DELIVERY_DATE, ~inv_ch_dlg.delivery_date)
					    		(INV_CREATE_DATE, ~inv_ch_dlg.creation_date)
					    		(INV_PAYMENT_DATE, ~inv_ch_dlg.payment_date)
							.Where(ID == recordswin.records(ID));
						else
							SQL & ::Update(RECORD)
					    		(INVOICE_ID, inv_id)
					    		(INV_DELIVERY_DATE, ~inv_ch_dlg.delivery_date)
					    		(INV_CREATE_DATE, ~inv_ch_dlg.creation_date)
					    		(INV_PAYMENT_DATE, ~inv_ch_dlg.payment_date)
							.Where(ID == recordswin.records(ID));
						
						Perform(formatter.formatFullInvoice(findInvoice(recordswin.records(ID), formatter.vat_payer_from)));
					}
				}
			}
		}
		else
		{
			// delete newly created record
			int last_id = -1;
			SQL & Select(SqlMax(ID)).From(RECORD);
			if (SQL.Fetch())
				last_id = SQL[0];
			
			if (last_id != -1)
			{
				SQL & Delete(RECORD_ITEM).Where(RECORD_ID == last_id);
				SQL & Delete(RECORD).Where(ID == last_id);
			}
		}
	}
	else {
		// save record text if changed
		if (ret == IDOK)
			recordswin.checksave_record_data(recordswin.records.GetCursor());
	}
}

void MainWin::when_find_invoice()
{
	WithInputLovLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, t_("Find invoice"));
	dlg.label.SetText(t_("Invoice number:"));
	if(dlg.Run(true) == IDOK)
	{
		SQL & Select(ID).From(RECORD).Where(INVOICE_ID == ~dlg.name);
		if(SQL.Fetch()) {
			Perform(formatter.formatFullInvoice(findInvoice(SQL[ID], formatter.vat_payer_from)));
		}
		else {
			PromptOK(Format(t_("Invoice %s not found"), ~dlg.name));
		}
	}
}

void MainWin::when_lov_dialog()
{
	InvoiceDataWin inv_data;
	LoadFromFile(inv_data, "tara_invoice.cfg");
	
	ProductLovWin tab1(~inv_data.vat);
	SpeciesLovWin tab2;
	AddressLovWin tab3;
	TitleLovWin tab4;
	PhraseLovWin tab5;
	
	TabDlg dlg;
	dlg	(tab1, t_("Products"))
		(tab2, t_("Species and Breed"))
		(tab3, t_("Addresses"))
		(tab4, t_("Titles"))
		(tab5, t_("Phrases"))
	   .OK()
	   .Title(t_("LOV"));
	
	if (dlg.Run(true) == IDOK)
	{
		tab5.check_save_record(tab5.names.GetCursor());
		refresh_cache_all();
		fill_controls();
	}
}

void MainWin::when_inv_data_dlg()
{
	InvoiceDataWin dlg;
	LoadFromFile(dlg, "tara_invoice.cfg");
	if (dlg.Run(true) == IDOK)
		StoreToFile(dlg, "tara_invoice.cfg");
	
	prepareInvoiceFormatter();
}

void MainWin::when_reset_accounting()
{
	if (PromptOKCancel(t_("Are You sure, you want to reset accounting for this year?")) == IDOK)
		SQL.Execute("SELECT setval('public.invoice_seq', 0, true);");
}

void MainWin::when_new_species()
{
	WithInputLovLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, t_("New species"));
	dlg.label.SetText(t_("Species name:"));
	if(dlg.Run(true) == IDOK)
	{
		//check whether species already exist
		SQL & Select(SqlCountRows()).From(LOV_SPECIES).Where(NAME == ~dlg.name);
		if(SQL.Fetch() && SQL[0] == 0)
		{
			// if not, insert and refresh cache
			SQL & Insert(LOV_SPECIES)
				(NAME, ~dlg.name)
			;
			
			species_cache.refresh();
			
			::Add(species, species_cache.get());
			
			// select newly created species in drop grid
			int rec_id = 0;
			SQL & Select(SqlMax(ID)).From(LOV_SPECIES);
			while (SQL.Fetch())
			{
				rec_id = SQL[0];
			}
			patients.Set(SPECIES_ID, rec_id);
		}
		else
			PromptOK(Format(t_("Species %s already exists."), ~dlg.name));
	}
}

void MainWin::when_new_breed()
{
	WithInputLovLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, t_("New breed"));
	dlg.label.SetText(t_("Breed name:"));
	if(dlg.Run(true) == IDOK)
	{
		//check whether breed already exists
		SQL & Select(SqlCountRows()).From(LOV_BREED).Where(NAME == ~dlg.name && LOV_SPECIES_ID == species.GetKey());
		if(SQL.Fetch() && SQL[0] == 0)
		{
			// if not, insert and refresh cache
			SQL & Insert(LOV_BREED)
				(NAME, ~dlg.name)
				(LOV_SPECIES_ID, species.GetKey())
			;
			
			breed_cache.refresh(species_cache);
			
			::Add(breed, breed_cache.get(species.GetKey()));
			
			// select newly created breed in drop grid
			int rec_id = 0;
			SQL & Select(SqlMax(ID)).From(LOV_BREED);
			while (SQL.Fetch())
			{
				rec_id = SQL[0];
			}
			patients.Set(BREED_ID, rec_id);
		}
		else
			PromptOK(Format(t_("Breed %s already exists."), ~dlg.name));
	}
}

void MainWin::when_new_title()
{
	WithInputLovLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, t_("New title"));
	dlg.label.SetText(t_("Title name:"));
	if(dlg.Run(true) == IDOK)
	{
		//check whether title already exist
		SQL & Select(SqlCountRows()).From(LOV_TITLE).Where(NAME == ~dlg.name);
		if(SQL.Fetch() && SQL[0] == 0)
		{
			// if not, insert and refresh cache
			SQL & Insert(LOV_TITLE)
				(NAME, ~dlg.name)
			;
			
			title_cache.refresh();
			
			::Add(title, title_cache.get());
			
			// select newly created species in drop grid
			int rec_id = 0;
			SQL & Select(SqlMax(ID)).From(LOV_TITLE);
			while (SQL.Fetch())
			{
				rec_id = SQL[0];
			}
			clients.Set(TITLE_ID, rec_id);
		}
		else
			PromptOK(Format(t_("Title %s already exists."), ~dlg.name));
	}
}

void MainWin::when_complex_statistics()
{
	if (lang != LANG_CZ) {
		WithPasswordLayout<TopWindow> passDlg;
		CtrlLayoutOKCancel(passDlg, t_("Password"));
		passDlg.password.Password(true);
	
		if (passDlg.Run(true) == IDOK) {
			//PromptOK(AsString("Orig: " + AsString(~passDlg.password) 
			//	+ AsString(", HASH: " + SHA1String(AsString(~passDlg.password)))));
			const String pass = "eb4fcfcd4384d41be9c14e6c2c4069d75cf4d01f";
			if ( pass.IsEqual(SHA1String(AsString(~passDlg.password))))
				ComplexStatsWin().Run(true);
			else
				Exclamation(t_("Incorrect password!"));
		}
	}
	else {
		ComplexStatsWin().Run(true);
	}
}

void MainWin::when_invoice_list()
{
	WithInvoiceListChoiceLayout<TopWindow> invListChoiceDlg;
	CtrlLayoutOKCancel(invListChoiceDlg, t_("Invoice list"));
	invListChoiceDlg.date_from.SetData(GetSysDate());
	invListChoiceDlg.date_to.SetData(GetSysDate());
	if (invListChoiceDlg.Run(true) == IDOK)
	{
		InvoiceList invList(lang, invListChoiceDlg.date_from, invListChoiceDlg.date_to);
		if (invList.prepare())
			Perform(invList.report());
		else
			PromptOK(
				Format(t_("No invoices found for given period (from %s to %s)!"), 
					AsString(invListChoiceDlg.date_from.GetData()), 
					AsString(invListChoiceDlg.date_to.GetData())));
	}
}


void MainWin::when_change_first_name()
{
	if (((String)~first_name).GetCount() == 1)
	{
	 	String s = ~first_name;
	 	s.Set(0, ToUpper(s[0]));
	 	first_name.SetData(s);
	 	first_name.SetSelection(1);
	}
}

void MainWin::when_change_last_name()
{
	if (((String)~last_name).GetCount() == 1)
	{
	 	String s = ~last_name;
	 	s.Set(0, ToUpper(s[0]));
	 	last_name.SetData(s);
	 	last_name.SetSelection(1);
	}
}

void MainWin::when_change_patient_name()
{
	if (((String)~name).GetCount() == 1)
	{
	 	String s = ~name;
	 	s.Set(0, ToUpper(s[0]));
	 	name.SetData(s);
	 	name.SetSelection(1);
	}
}

void MainWin::when_edit_lysset()
{
	WithInputLovLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, t_("New Lysset"));
	dlg.label.SetText(t_("LyssetNum"));
	dlg.name <<= patients.Get(LYSSET);
	
	if (dlg.name.GetData().IsNull())
		dlg.name <<= Format("%1:04d-SK-%2:04d", GetSysDate().year, get_next_lysset());
	
	dlg.name.SetConvert(Single<ConvertLysset>());
	
	if(dlg.Run(true) == IDOK) {
		lysset <<= ~dlg.name;
		patients(LYSSET) = ~dlg.name;
		// TODO check if not existing already!
	}
}

int MainWin::get_next_lysset()
{
	SQL & Select(LYSSET).From(PATIENT)
		.Where(Like(LYSSET, Wild(Format("%1:04d-SK-*", GetSysDate().year))))
		.OrderBy(Descending(LYSSET)).Limit(1);
		
	if(SQL.Fetch())
	{
		String s = SQL[LYSSET];
		return ScanInt(s.Mid(8)) + 1;
	}
	
	return 1;
}

void MainWin::when_find_by_lysset()
{
	WithInputLovLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, t_("Find by lysset"));
	dlg.name.SetText(Format("%1:04d-SK-0001", GetSysDate().year));
	dlg.label.SetText(t_("LyssetNum"));
	if(dlg.Run(true) == IDOK)
	{
		SqlSelect sel = Select(
			ID.Of(CLIENT), TITLE_ID.Of(CLIENT), FIRST_NAME.Of(CLIENT), 
			LAST_NAME.Of(CLIENT), CITY_ID.Of(CLIENT), STREET_ID.Of(CLIENT), 
			HOUSE_NO.Of(CLIENT), HOUSE_NO.Of(CLIENT), PHONE_1.Of(CLIENT), 
			PHONE_2.Of(CLIENT), EMAIL.Of(CLIENT), NOTE.Of(CLIENT), IC.Of(CLIENT), 
			DIC.Of(CLIENT), ICDPH.Of(CLIENT))
				.From(PATIENT)
				.InnerJoin(CLIENT).On(CLIENT_ID.Of(PATIENT) == ID.Of(CLIENT))
				.Where(Like(LYSSET.Of(PATIENT), Wild(Format("*%1:s*", ~dlg.name))))
				.OrderBy(LAST_NAME.Of(CLIENT))
				.Limit(MAX_CLIENTS_DISPLAYED);
		
		find_client(sel);
	}
}

void MainWin::when_find_by_patient()
{
	WithInputLovLayout<TopWindow> dlg;
	CtrlLayoutOKCancel(dlg, t_("Find by patient"));
	dlg.label.SetText(t_("Name:"));
	if(dlg.Run(true) == IDOK)
	{
		SqlSelect sel = Select(
			ID.Of(CLIENT), TITLE_ID.Of(CLIENT), FIRST_NAME.Of(CLIENT), 
			LAST_NAME.Of(CLIENT), CITY_ID.Of(CLIENT), STREET_ID.Of(CLIENT), 
			HOUSE_NO.Of(CLIENT), HOUSE_NO.Of(CLIENT), PHONE_1.Of(CLIENT), 
			PHONE_2.Of(CLIENT), EMAIL.Of(CLIENT), NOTE.Of(CLIENT), IC.Of(CLIENT), 
			DIC.Of(CLIENT), ICDPH.Of(CLIENT))
				.From(PATIENT)
				.InnerJoin(CLIENT).On(CLIENT_ID.Of(PATIENT) == ID.Of(CLIENT))
				.Where(Like(NAME.Of(PATIENT), Wild(Format("*%1:s*", ~dlg.name)), false /* case insensitive like */ ))
				.OrderBy(LAST_NAME.Of(CLIENT))
				.Limit(MAX_CLIENTS_DISPLAYED);
		
		find_client(sel);
	}
}

void MainWin::when_invoice_mass_print()
{
	WithInvoiceListChoiceLayout<TopWindow> invDateDlg;
	CtrlLayoutOKCancel(invDateDlg, t_("Invoice mass print"));
	invDateDlg.date_from.SetData(GetSysDate());
	invDateDlg.date_to.SetData(GetSysDate());
	
	if (invDateDlg.Run(true) == IDOK) {
		Vector<InvoiceData>& invoices = findInvoices(invDateDlg.date_from, invDateDlg.date_to, formatter.vat_payer_from);

		if (invoices.GetCount() < 1) {
			PromptOK(
				Format(t_("No invoices found for given period (from %s to %s)!"), 
					AsString(invDateDlg.date_from.GetData()), 
					AsString(invDateDlg.date_to.GetData())));
		}
		else {
			Perform(formatter.formatFullInvoices(invoices));
		}
	}
}
