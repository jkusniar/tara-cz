#include "convert.h"

SlovakNumberToTextConverter::SlovakNumberToTextConverter()
{
	map_jednotky.Add(3, "tri");
	map_jednotky.Add(4, "štyri");
	map_jednotky.Add(5, "päť");
	map_jednotky.Add(6, "šesť");
	map_jednotky.Add(7, "sedem");
	map_jednotky.Add(8, "osem");
	map_jednotky.Add(9, "deväť");
	
	map_desiatky.Add(0, "desať");
	map_desiatky.Add(1, "jedenásť");
	map_desiatky.Add(2, "dvanásť");
	map_desiatky.Add(3, "trinásť");
	map_desiatky.Add(4, "štrnásť");
	map_desiatky.Add(5, "pätnásť");
	map_desiatky.Add(6, "šestnásť");
	map_desiatky.Add(7, "sedemnásť");
	map_desiatky.Add(8, "osemnásť");
	map_desiatky.Add(9, "devätnásť");
	
	map_rady.Add(1, "tisíc");
	map_rady.Add(2, "mi");
	map_rady.Add(3, "mi");
	map_rady.Add(4, "bi");
	map_rady.Add(5, "bi");
	map_rady.Add(6, "tri");
	map_rady.Add(7, "tri");
	map_rady.Add(8, "kvadri");
	map_rady.Add(9, "kvadri");
}

String SlovakNumberToTextConverter::convertJednotky(unsigned long num, unsigned long multipl)
{	
	if (num == 1)
		if (multipl == 0) 
			return "jeden";
		else
			return "";
	else if (num == 2)
	{ 
		if ( (multipl % 2) == 0 )
			return "dva";
		else
			return "dve";
	}
	else
		if (map_jednotky.Find(num) >= 0)
			return map_jednotky.Get(num);
		else return "";
}

String SlovakNumberToTextConverter::convertDeasiatky(unsigned long jednotky, unsigned long desiatky)
{
	if (desiatky == 1)
	{
		if (map_desiatky.Find(jednotky) >= 0)
			return map_desiatky.Get(jednotky);
		else
			return "";
	}
	else if (desiatky >= 2 && desiatky <=4)
	{
		return 
			convertJednotky(desiatky, 0)
			+ String("dsať")
			+ convertJednotky(jednotky, 0);
	}
	else
	{
		return
			convertJednotky(desiatky, 0)
			+ String("desiat")
			+ convertJednotky(jednotky, 0);
	}
		
}

String SlovakNumberToTextConverter::convertStovky(unsigned long cislo, unsigned long rad)
{
	unsigned long stovky, desiatky, jednotky;
	
	jednotky = cislo % 10;
 	desiatky = (cislo/10) % 10;
 	stovky = cislo/100;
 	
 	String pom = "";
 	
 	if (desiatky == 0)
 	{
 		if (stovky == 0)
 			pom = convertJednotky(jednotky, rad);
 		else
 			pom = convertJednotky(jednotky, 0);
 	}
 	else
 	{
 		pom = convertDeasiatky(jednotky, desiatky);	
 	}
 	
 	if (stovky == 1)
 	{
 		return String("sto") + pom;
 	}
 	else if (stovky >=2 && stovky <= 9)
 	{
 		return convertJednotky(stovky, 1) + String("sto") + pom;
 	}
 	
 	return pom;
}

String SlovakNumberToTextConverter::convertRady(unsigned long cislo, unsigned long rad)
{
	String prefix = "";
	
	if (map_rady.Find(rad) >= 0)
		prefix = map_rady.Get(rad);
	
	if (rad == 1)
		if (cislo != 0)
			return prefix;
		else
			return "";
	else if ((rad % 2) == 0)
	{
		if (cislo == 0)
			return "";
		else if (cislo == 1)
			return prefix + String("lión");
		else if (cislo >= 2 && cislo <=4)
			return prefix + String("lióny");
		else
			return prefix + String("liónov");
	}
	else
	{
		if (cislo == 0)
			return "";
		else if (cislo == 1)
			return prefix + String("liarda");
		else if (cislo >= 2 && cislo <=4)
			return prefix + String("liardy");
		else
			return prefix + String("liárd");
	}
}

String SlovakNumberToTextConverter::convert(unsigned long cislo)
{
	unsigned long rad;
	unsigned long pom;
	
	String vystup = "";
	
	if (cislo == 0)
	{
		return "Nula";
	}
	else
	{
		rad = 0;
		while (cislo != 0)
		{
			pom = cislo % 1000;
			vystup = convertStovky(pom, rad) + vystup;
			cislo /= 1000;
			
			if (cislo != 0)
			{
				rad++;
				vystup = convertRady(cislo % 1000, rad) + vystup;
			}
		}
		
		if (pom >= 100 && pom <= 199)
			vystup = "jedno" + vystup;
		
		if (pom == 1 && rad == 0)
			vystup = "jedno"; //Jedno euro, jedna koruna
		
		if (pom == 1 && rad > 0)
		{
			if (rad == 1 || rad == 2 || rad == 4 || rad == 6 || rad == 8)
				vystup = "jeden" + vystup;
				
			else if (rad == 3 || rad == 5 || rad == 7 || rad == 9)
				vystup = "jedna" + vystup;
		}
	}
	
	// prve pismeno je velke
	if (!vystup.IsEmpty())
		vystup.Set(0, ToUpper(vystup[0]));
	
	return vystup;
}