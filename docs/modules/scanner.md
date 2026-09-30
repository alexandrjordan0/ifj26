# Modul Lexikální analýza scanner

## Základní informace a odpovědnost

- Odpovědná osoba: Baestro
- Přiřazené soubory: src/scanner.c a src/scanner.h
- Základní princip: deterministický konečný automat
- Zákaz generátorů: nástroje lex a flex jsou přísně zakázány

## Rozhraní modulu scanner.h

- Hlavní funkce modulu: get_next_token s parametrem ukazatele na strukturu Token
- Návratové hodnoty funkce: 0 úspěch, 1 lexikální chyba, 99 interní chyba
- Výčtový typ TokenType pro jednoznačnou identifikaci kategorie lexému
- Datová struktura Token obsahující typ, atribut hodnoty a číslo řádku pro chybová hlášení

## Požadavky na rozpoznání lexémů jazyka IFJ26

- Klíčová slova základního zadání: def, do, Double, else, if, import, Int, return, String, then, Unit, val, var, while
- Klíčová slova a literály rozšíření BOOLTHEN: Boolean, true, false
- Identifikátory: posloupnost začínající písmenem nebo podtržítkem následovaná písmeny, číslicemi nebo podtržítky
- Samotný znak podtržítka není platný identifikátor
- Celá čísla typu Int: desítkový zápis, zákaz počátečních nul, možnost oddělovat číslice podtržítkem
- Desetinná čísla typu Double: zápis s desetinnou tečkou nebo exponentem e a E
- Řetězcové literály typu String: uzavřené v uvozovkách na jednom řádku včetně sekvencí lomítko n, lomítko r, lomítko t, lomítko uvozovka a dvojité zpětné lomítko
- Víceřádkové řetězce: ohraničené trojicí uvozovek bez vyhodnocování escape sekvencí
- Literál typu Unit: reprezentovaný prázdnými závorkami
- Jednořádkové komentáře: začínající dvojitým lomítkem až po konec řádku
- Blokové komentáře: ohraničené lomítkem s hvězdičkou s plnou podporou vzájemného zanoření
- Pravidla pro odsazení: povoleny jsou výhradně mezery, tabulátor na začátku neprázdného řádku je okamžitá lexikální chyba 1
- Operátory: aritmetické, relační a logické symboly plus, mínus, krát, děleno, rovnost, nerovnost, menší, větší, vykřičník, logické spojky

## Fáze implementace a postupu práce

1. Návrh stavového diagramu konečného automatu na papír jako povinná příloha finální dokumentace
2. Vytvoření hlavičkového souboru scanner.h s deklaracemi datových typů a funkcí
3. Načítání znaků ze vstupu a správné přeskakování nepodstatných bílých znaků
4. Implementace stavů pro identifikátory, klíčová slova a číselné hodnoty
5. Implementace stavů pro řetězce, escape sekvence a oba typy komentářů
6. Ošetření kontroly odsazení řádků a okamžité hlášení nepovoleného tabulátoru
7. Správné vracení chybového kódu 1 při neukončených řetězcích nebo neznámých znacích

## Samostatné testování

- Vytvoření vlastního izolovaného souboru s funkcí main na ověření čtení ze standardního vstupu
- Ověření tisku typu každého rozpoznaného tokenu a jeho hodnoty
- Testování platných vstupů pokrývajících všechny povolené typy literálů
- Testování chybových scénářů jako neznámý znak, neukončený komentář nebo tabulátor na začátku řádku

## Prostor na poznámky a otázky řešitele

- Zde si řešitel zapisuje zjištění a návrhy na úpravu rozhraní před pull requestem
