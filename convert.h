#ifndef _tara_cz_convert_h_
#define _tara_cz_convert_h_

#include <Core/Core.h>
using namespace Upp;

class NumberToTextConverter
{
public: virtual String convert(unsigned long cislo) = 0;
};

class SlovakNumberToTextConverter : public NumberToTextConverter
{
public: String convert(unsigned long cislo);
	SlovakNumberToTextConverter();

private:
	// Number to text conversion...
	VectorMap<unsigned long, String> map_jednotky;
	VectorMap<unsigned long, String> map_desiatky;
	VectorMap<unsigned long, String> map_rady;
	
	String convertJednotky(unsigned long num, unsigned long multipl);
	String convertDeasiatky(unsigned long jednotky, unsigned long desiatky);
	String convertStovky(unsigned long cislo, unsigned long rad);
	String convertRady(unsigned long cislo, unsigned long rad);
};

class ConvertMoney : public ConvertDouble
{
public: ConvertMoney() : ConvertDouble() {Pattern("%2!nl");}
//public: ConvertMoney() : ConvertDouble() {Pattern("%2,!nl");} // ked nejde sk_SK locale pomoze toto
};

class ConvertAmount : public ConvertDouble
{
public: ConvertAmount() : ConvertDouble() {Pattern("%4!nl");}
//public: ConvertMoney() : ConvertDouble() {Pattern("%2,!nl");} // ked nejde sk_SK locale pomoze toto
};

class ConvertLocalized : public ConvertDouble
{
public: ConvertLocalized() : ConvertDouble() {Pattern("%10vl");}
//public: ConvertLocalized() : ConvertDouble() {Pattern("%10,vl");} // ked nejde sk_SK locale pomoze toto
};

#endif
