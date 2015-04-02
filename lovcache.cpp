#include "mainwin.h"

#define SCHEMADIALECT <PostgreSQL/PostgreSQLSchema.h>
#ifdef _DEBUG
#include <Sql/sch_schema.h>
#endif
#include <Sql/sch_source.h>

SimpleLovCache species_cache(LOV_SPECIES);
SimpleLovCache sex_cache(LOV_SEX);
SimpleLovCache title_cache(LOV_TITLE);
SimpleLovCache unit_cache(LOV_UNIT);
SimpleLovCache product_cache(LOV_PRODUCT);
SimpleLovCache phrase_cache(LOV_PHRASE);
LinkedLovCache breed_cache(LOV_BREED, LOV_SPECIES_ID);

VectorMap<int, VectorMap<int, String> > city_cache;
VectorMap<int, VectorMap<int, VectorMap<int, String> > > street_cache;

#ifdef _DEBUG
void initDebugDB(PostgreSQLSession &session)
{
	session.SetTrace();
	//schema
	Progress p;
	p.SetText(t_("Creating _DEBUG database"));
	SqlSchema sch(PGSQL);
	StdStatementExecutor se(session);
	All_Tables(sch);
	if(sch.ScriptChanged(SqlSchema::UPGRADE))
		PostgreSQLPerformScript(sch.Upgrade(),se, p);
	if(sch.ScriptChanged(SqlSchema::ATTRIBUTES)) {
		PostgreSQLPerformScript(sch.Attributes(),se, p);
	}
	if(sch.ScriptChanged(SqlSchema::CONFIG)) {
		PostgreSQLPerformScript(sch.ConfigDrop(),se, p);
		PostgreSQLPerformScript(sch.Config(),se, p);
	}
	sch.SaveNormal();
}
#endif

// Utility function for DropGrid
void Add (DropGrid& list, const VectorMap<Value, Value>& vec)
{
	list.Clear();
	for (int i=0; i<vec.GetCount(); i++)
	{
		list.Add(vec.GetKey(i), vec[i]);
	}
}

void SimpleLovCache::refresh()
{
	if (!_cache.IsEmpty())
		_cache.Clear();
	
	SQL & Select(ID, NAME).From(_table_id).OrderBy(NAME);
	while(SQL.Fetch())
		_cache.Add(SQL[ID], SQL[NAME]);
}

void LinkedLovCache::refresh(SimpleLovCache& master)
{
	if (!_cache.IsEmpty())
		_cache.Clear();
		
	for (int i=0; i<master.get().GetCount(); i++)
	{
		VectorMap<Value, Value> vm;
		SQL & Select(ID, NAME).From(_table_id).Where(_join_column.Of(_table_id) == master.get().GetKey(i)).OrderBy(NAME);
		while(SQL.Fetch())
			vm.Add(SQL[ID], SQL[NAME]);
		
		if (!vm.IsEmpty())
			_cache.Add(master.get().GetKey(i), vm);
	}
	
	if (!_all.IsEmpty())
		_all.Clear();
}

void refresh_cache_all()
{
	Progress pi; // progress indicator window
	pi.Icon(Images::boxer_icon).LargeIcon(Images::boxer_icon);
	pi.AlignText(ALIGN_LEFT);
	pi.Title(t_("Loading..."));
	pi.SetText(t_("LOVs"));
		
	// LOV cache;
	//---------------------
	// species cache
	species_cache.refresh();
	
	// breed cache
	breed_cache.refresh(species_cache);
	
	// sex cache
	sex_cache.refresh();
	
	// title cache
	title_cache.refresh();
	
	// unit cache
	unit_cache.refresh();
	
	// product cache
	product_cache.refresh();
	
	// phrase cache
	phrase_cache.refresh();
	
	// city cache
	pi.SetText(t_("Street index"));
	SQL & Select(ID, CITY, PSC, POST_OFFICE, DISTRICT, PROVINCE).From(LOV_CITY).OrderBy(CITY);
	while(SQL.Fetch())
	{
		VectorMap<int, String> vm_rec;
		vm_rec.Add(ctCITY, SQL[CITY]);
		vm_rec.Add(ctPSC, SQL[PSC]);
		vm_rec.Add(ctPOST_OFFICE, SQL[POST_OFFICE]);
		vm_rec.Add(ctDISTRICT, SQL[DISTRICT]);
		vm_rec.Add(ctPROVINCE, SQL[PROVINCE]);
		city_cache.Add(SQL[ID], vm_rec);
		
		if(pi.StepCanceled()) break;
	}
	
	// street cache
	for (int i=0; i<city_cache.GetCount(); i++)
	{
		VectorMap<int, VectorMap<int, String> > vm_str;
		SQL & Select(ID, STREET, PSC, POST_OFFICE)
			.From(LOV_STREET)
			.Where(CITY_ID == city_cache.GetKey(i)).OrderBy(STREET);
		while(SQL.Fetch())
		{
			VectorMap<int, String> vm_rec;
			vm_rec.Add(stSTREET, SQL[STREET]);
			vm_rec.Add(stPSC, SQL[PSC]);
			vm_rec.Add(stPOST_OFFICE, SQL[POST_OFFICE]);
			vm_str.Add(SQL[ID], vm_rec);
			
			if(pi.StepCanceled()) break;
		}
		
		if (!vm_str.IsEmpty())
			street_cache.Add(city_cache.GetKey(i), vm_str);
	}
}
