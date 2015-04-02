#include "mainwin.h"

PhraseLovWin::PhraseLovWin()
{
	names.AddIndex(ID);
	names.AddColumn(NAME, t_("Phrase name")).Edit(name.MaxChars(100));
	names.SetToolBar(true, BarCtrl::BAR_TOP);
	names.WhenInsertRow = THISBACK(when_insert_row);
	names.WhenUpdateRow = THISBACK(when_update_row);
	names.WhenRemoveRow = THISBACK(when_delete_row);
	names.WhenChangeRow = THISBACK(when_names_change_row);
	
	SQL & Select(ID, NAME).From(LOV_PHRASE).OrderBy(NAME);
	while(SQL.Fetch())
	      names.Add(SQL);

	if (!names.IsEmpty())
		names.SetCursor(0);
}

void PhraseLovWin::when_names_change_row()
{
	check_save_record(names.GetPrevCursorId());

	phrase_text.Clear();
	SQL & Select(PHRASE_TEXT).From(LOV_PHRASE).Where(ID == names(ID));
	if (SQL.Fetch())
		phrase_text.SetData(SQL[0]);
}

void PhraseLovWin::when_insert_row()
{
	check_save_record(names.GetPrevCursorId());
	
	SQL & Insert(LOV_PHRASE)
	    (NAME, names(NAME));
	;
	
	SQL & Select(SqlMax(ID)).From(LOV_PHRASE);
	if (SQL.Fetch())
		names.Set(ID, SQL[0]);
	
	phrase_text.Clear();
}

void PhraseLovWin::when_delete_row()
{
	SQL & Delete(LOV_PHRASE).Where(ID == names(ID));
}

void PhraseLovWin::when_update_row()
{
	SQL & ::Update(LOV_PHRASE)
		(NAME, names(NAME))
		(PHRASE_TEXT, phrase_text.GetData())
	.Where(ID == names(ID));
	
	phrase_text.ClearDirty();
}

void PhraseLovWin::check_save_record(int prevId)
{
	if (phrase_text.IsDirty() && prevId >= 0) {
		SQL & ::Update(LOV_PHRASE)
			(PHRASE_TEXT, phrase_text.GetData())
			.Where(ID == names.Get(prevId, ID));
		
		phrase_text.ClearDirty();
	}
}
