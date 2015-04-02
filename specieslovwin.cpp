#include "mainwin.h"

SpeciesLovWin::SpeciesLovWin()
{
	species.AddIndex(ID);
	species.AddColumn(NAME, t_("Name")).Edit(species_name.MaxChars(100));
	species.SetToolBar(true, BarCtrl::BAR_TOP);
	species.WhenChangeRow = THISBACK(when_species_change_row);
	species.WhenInsertRow = THISBACK(when_insert_species);
	species.WhenUpdateRow = THISBACK(when_update_species);
	
	breed.AddIndex(ID);
	breed.AddColumn(NAME, t_("Name")).Edit(breed_name.MaxChars(100));
	breed.SetToolBar(true, BarCtrl::BAR_TOP);
	breed.WhenInsertRow = THISBACK(when_insert_breed);
	breed.WhenUpdateRow = THISBACK(when_update_breed);
	
	SQL & Select(ID, NAME).From(LOV_SPECIES).OrderBy(NAME);
	while(SQL.Fetch())
	      species.Add(SQL);

	if (!species.IsEmpty())
		species.SetCursor(0);
	else
		breed.Appending(false);
}

void SpeciesLovWin::when_species_change_row()
{
	breed.Clear();
	SQL & Select(ID, NAME).From(LOV_BREED).Where(LOV_SPECIES_ID == species(ID)).OrderBy(NAME);
	while(SQL.Fetch())
	      breed.Add(SQL);
}

void SpeciesLovWin::when_insert_species()
{
	SQL & Insert(LOV_SPECIES)
	    (NAME, species(NAME))
	;
	
	SQL & Select(SqlMax(ID)).From(LOV_SPECIES);
	if (SQL.Fetch())
		species.Set(ID, SQL[0]);
	
	breed.Appending(true);
}

void SpeciesLovWin::when_update_species()
{
	SQL & ::Update(LOV_SPECIES)
		(NAME, species(NAME))
	.Where(ID == species(ID));
}

void SpeciesLovWin::when_insert_breed()
{
	SQL & Insert(LOV_BREED)
		(NAME, breed(NAME))
	    (LOV_SPECIES_ID, species(ID))
	;
}

void SpeciesLovWin::when_update_breed()
{
	SQL & ::Update(LOV_BREED)
		(NAME, breed(NAME))
	.Where(ID == breed(ID));
}
