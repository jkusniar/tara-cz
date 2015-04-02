#include "mainwin.h"

InvoiceDataWin::InvoiceDataWin()
{
	CtrlLayoutOKCancel(*this, t_("Invoice data"));
	
	//default data
	name.SetData("Doctor Name");
	address.SetData("Doctor address");
	phone1.SetData("Phone no");
	phone2.SetData("Phone no2");
	ic.SetData("ID");
	dic.SetData("ID TAX");
	icdph.SetData("ID VAT");
	acc_num.SetData("Bank acc.");
	vat.SetData(false);
	vat_from.Enable(false);

	vat.WhenAction = THISBACK(when_vat_changed);
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
	s % vat;
	s % vat_from;
}

void InvoiceDataWin::when_vat_changed()
{
	vat_from.Enable(vat.GetData());
}
