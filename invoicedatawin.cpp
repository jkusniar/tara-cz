#include "mainwin.h"

InvoiceDataWin::InvoiceDataWin()
{
	CtrlLayoutOKCancel(*this, t_("Invoice data"));
	
	//default data
	name.SetData("MVDr. Veronika Žabková");
	address.SetData("Ľubietová 127, 976 55");
	phone1.SetData("0903 169037");
	phone2.SetData("0908 123456");
	ic.SetData("12 345 678");
	dic.SetData("0909234785");
	icdph.SetData("1234321211");
	acc_num.SetData("0987349/1111");
}

void InvoiceDataWin::Serialize(Stream &s)
{
	SerializePlacement(s);
	s % name;
	s % address;
	s % phone1;
	s % phone2;
	s % ic;
	s % dic;
	s % icdph;
	s % acc_num;
}
