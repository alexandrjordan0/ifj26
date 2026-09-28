# Modul Syntaktická a sémantická analýza parser

## Základní informace a odpovědnost

- Odpovědná osoba: člen týmu
- Přiřazené soubory: src/parser.c a src/parser.h
- Základní princip: metoda rekurzivního sestupu podle LL gramatiky
- Zákaz generátorů: nástroje Bison a yacc jsou přísně zakázány
- Spolupráce: úzká vazba na modul scanner pro získávání tokenů a modul symtable pro kontrolu symbolů

## Rozhraní modulu parser.h

- Hlavní funkce modulu: parse_program bez parametrů případně s ukazatelem na AST
- Návratové hodnoty funkce: 0 úspěch, 2 syntaktická chyba, 3 až 7 sémantické chyby, 99 interní chyba
- Pomocné funkce pro rekurzivní sestup odpovídající neterminálům gramatiky
- Předávání řízení na precedenční syntaktickou analýzu při výskytu výrazu

## Požadavky na syntaktické a sémantické kontroly jazyka IFJ26

- Kontrola povinného prologu import ifj26.\* na začátku souboru, absence vrací chybu 2
- Kontrola právě jedné vstupní funkce označené anotací @main s návratovým typem Unit a bez parametrů
- Chybějící vstupní funkce vrací chybu 3, více vstupních funkcí chybu 4, parametry chybu 5, jiný typ chybu 6
- Zpracování definic funkcí def s parametry a návratovým typem
- Zákaz přetěžování funkcí, redefinice funkce vrací sémantickou chybu 4
- Zpracování deklarace a inicializace proměnných val pro nemodifikovatelné a var pro modifikovatelné
- Kontrola zápisu do nemodifikovatelné proměnné nebo parametru funkce s návratovým kódem 7
- Odvozování datových typů při inicializaci proměnných, nejednoznačnost vrací chybu 6
- Zpracování odsazených bloků příkazů na základě počtu mezer bez použití složených závorek
- Zpracování podmínek if then else a cyklů while do s kontrolou samostatných rozsahů platnosti
- Kontrola volání funkcí se správným počtem a typem parametrů s chybovým kódem 5
- Použití nedefinované proměnné nebo funkce vrací sémantickou chybu 3
- Správná integrace rozšíření BOOLTHEN a FUNEXP v rámci syntaktických pravidel

## Fáze implementace a postupu práce

1. Návrh LL gramatiky a vytvoření LL tabulky na papír jako podklad pro dokumentaci
2. Vytvoření hlavičkového souboru parser.h s deklarací hlavního vstupu
3. Implementace kontroly prologu a vstupní funkce @main
4. Implementace rekurzivního sestupu pro definice funkcí a seznam parametrů
5. Implementace zpracování lokálních bloků, proměnných val a var a přiřazení
6. Implementace řídicích struktur if else a while se správou vnořených scopes v symtable
7. Napojení na precedenční analýzu výrazů a předávání zkontrolovaných struktur do generátoru kódu

## Samostatné testování

- Vytvoření testovací sady se správnou strukturou funkcí a bloků
- Testy na syntaktické chyby jako chybějící prolog, neplatné odsazení nebo chybějící dvojtečka
- Testy na sémantické chyby jako nedefinovaný symbol, redefinice identifikátoru nebo zápis do val
- Ověření správného vracení přesných chybových návratových kódů podle zadání

## Prostor na poznámky a otázky řešitele

- Zde si řešitel zapisuje zjištění a dotazy k návrhu gramatiky a návaznosti na generátor kódu
