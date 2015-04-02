#ifndef _tara_cz_services_h_
#define _tara_cz_services_h_

#include <PostgreSQL/PostgreSQL.h>

using namespace Upp;

#include "invoice.h"

InvoiceData& findInvoice(Value recordId, Date vat_payer_from);
Vector<InvoiceData>& findInvoices(Date date_from, Date date_to, Date vat_payer_from);

InvoiceData& findBill(Value recordId);
InvoiceData& findRecord(Value recordId);

#endif
