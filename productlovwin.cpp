#include "mainwin.h"
#include "convert.h"

ProductLovWin::ProductLovWin()
{
	products.AddIndex(ID);
	products.AddColumn(NAME, t_("Name")).Edit(name.MaxChars(100));
	products.AddColumn(UNIT_ID, t_("Unit")).Edit(unit).SetConvert(unit);
	products.AddColumn(PRICE, t_("Price")).Edit(price).SetConvert(Single<ConvertMoney>());
	products.SetToolBar(true, BarCtrl::BAR_TOP);
	products.WhenInsertRow = THISBACK(when_insert);
	products.WhenUpdateRow = THISBACK(when_update);
	
	unit.AddPlus(THISBACK(when_new_unit));
	unit.ColorRows(true);
	::Add(unit, unit_cache.get());
	
	SQL & Select(ID, NAME, UNIT_ID, PRICE).From(LOV_PRODUCT).OrderBy(NAME);
	while(SQL.Fetch())
	      products.Add(SQL);

	if (!products.IsEmpty())
		products.SetCursor(0);
}

void ProductLovWin::when_insert()
{
	SQL & Insert(LOV_PRODUCT)
	    (NAME, products(NAME))
	    (UNIT_ID, products(UNIT_ID))
	    (PRICE, products(PRICE))
	;
}

void ProductLovWin::when_update()
{
	SQL & ::Update(LOV_PRODUCT)
		(NAME, products(NAME))
	    (UNIT_ID, products(UNIT_ID))
	    (PRICE, products(PRICE))
	.Where(ID == products(ID));
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
				(NAME, ~dlg.name)
			;
			
			unit_cache.refresh();
			
			::Add(unit, unit_cache.get());
			
			// select new unit in dropgrid
			int rec_id = 0;
			SQL & Select(SqlMax(ID)).From(LOV_UNIT);
			while (SQL.Fetch())
			{
				rec_id = SQL[0];
			}
			products.Set(UNIT_ID, rec_id);
		}
		else
			PromptOK(Format(t_("Unit %s already exists."), ~dlg.name));
			
	}
}
