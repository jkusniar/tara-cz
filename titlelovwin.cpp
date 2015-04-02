#include "mainwin.h"

TitleLovWin::TitleLovWin()
{
	titles.AddIndex(ID);
	titles.AddColumn(NAME, t_("Name")).Edit(name.MaxChars(30));
	titles.SetToolBar(true, BarCtrl::BAR_TOP);
	titles.WhenInsertRow = THISBACK(when_insert);
	titles.WhenUpdateRow = THISBACK(when_update);
	
	SQL & Select(ID, NAME).From(LOV_TITLE).OrderBy(NAME);
	while(SQL.Fetch())
	      titles.Add(SQL);

	if (!titles.IsEmpty())
		titles.SetCursor(0);
}

void TitleLovWin::when_insert()
{
	SQL & Insert(LOV_TITLE)
	    (NAME, titles(NAME))
	;
}

void TitleLovWin::when_update()
{
	SQL & ::Update(LOV_TITLE)
		(NAME, titles(NAME))
	.Where(ID == titles(ID));
}
