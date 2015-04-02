#include "mainwin.h"

LoginWin::LoginWin()
{
	CtrlLayoutOKCancel(*this, t_("Login"));
	Icon(Images::boxer_icon);
	dbname.SetData("tara");
}

void LoginWin::Serialize(Stream &s)
{
	SerializePlacement(s);
	s % login;
	s % password;
	s % store_login;
	s % dbname;
}
