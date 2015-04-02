#include "constants.h"
#include "mainwin.h"

#define TFILE <tara-cz/tara-cz.t>
#include <Core/t.h>

GUI_APP_MAIN
{	
	int language = LNGC_('S','K','S','K', CHARSET_UTF8);
	//int language = LNGC_('E','N','U','S', CHARSET_UTF8);
	//int language = LNGC_('C','S','C','Z', CHARSET_UTF8);
	SetLanguage(language);
	setlocale(LC_ALL,"C"); // Linux double formating requires this !!!

	
	PostgreSQLSession sql_session;
	
	//login and open session
	LoginWin login_dlg;
	LoadFromFile(login_dlg, "tara_login.cfg");
	if (!login_dlg.store_login && login_dlg.Run(true) != IDOK)
		return;

	while(!sql_session.Open("host=localhost dbname="+ AsString(~login_dlg.dbname) +" user="+ AsString(~login_dlg.login) +" password=" + AsString(~login_dlg.password) ))
	{
		Exclamation(Format(t_("Error: %s"), DeQtf(sql_session.GetLastError())));
		if (login_dlg.Run(true) != IDOK)
			return;
	}
	
	StoreToFile(login_dlg, "tara_login.cfg");

// if DEBUG, synchronize database schema
#ifdef _DEBUG	
	initDebugDB(sql_session);
#endif
	
	SQL = sql_session;

	try
	{
		// load LOV cache
		refresh_cache_all();
		// create window
		MainWin mwin(AsString(~login_dlg.dbname));
		// load settings from file
		LoadFromFile(mwin, "tara_main.cfg");
		// window setup (according to loaded settings)
		
		// set language according to settings
		switch (mwin.lang)
		{
			case LANG_ENG:
				language = LNGC_('E','N','U','S', CHARSET_UTF8);
				break;
			case LANG_SK:
				language = LNGC_('S','K','S','K', CHARSET_UTF8);
				break;
			case LANG_CZ:
				language = LNGC_('C','S','C','Z', CHARSET_UTF8);
				break;
			default:
				language = LNGC_('S','K','S','K', CHARSET_UTF8);
		}
		SetLanguage(language);
		setlocale(LC_ALL,"C");
		
		mwin.WindowSetup();
		//start app
		mwin.Run();
		//store settings to file
		mwin.StoreSetup();
		StoreToFile(mwin,"tara_main.cfg");
	}
	catch(SqlExc &e)
	{
		Exclamation(Format(t_("Error: %s"), DeQtfLf(e)));
	}
	
	sql_session.Close();
}
