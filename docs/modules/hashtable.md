# Modul Tabulka symbolů symtabe a hashtable

## Základní informace a odpovědnost

- Odpovědná osoba: Girab
- Přiřazené soubory: src/hashtable.c, src/hashtable.h, src/symtable.c a src/symtable.h
- Varianta zadání: TRP izp což představuje tabulku s rozptýlenými položkami s otevřenou adresací
- Přísný zákaz: na tuto vyhledávací datovou strukturu je podle pravidel projektu přísně zakázáno použít nástroje umělé inteligence

## Rozhraní modulu symtable.h a hashtable.h

- Modul poskytuje abstraktní datový typ pro ukládání a rychlé vyhledávání identifikátorů
- Základní funkce rozhraní: inicializace tabulky, vyhledání prvku podle textového klíče, vložení nového symbolu, aktualizace dat existujícího symbolu a kompletní uvolnění tabulky z paměti
- Klíčem položky je řetězec odpovídající identifikátoru proměnné nebo funkce
- Ukládaná data obsahují: druh symbolu proměnná nebo funkce, datový typ hodnoty, příznak definice, seznam typů parametrů u funkcí, návratový typ a informace o rámci pro generátor kódu
- Návratové chování operací: jasná signalizace nalezení prvku a bezpečné vracení interní chyby 99 při selhání alokace paměti

## Požadavky na implementaci pro variantu TRP izp

- Ukládání přímo do pole pevné nebo dynamicky rozšiřované velikosti
- Implementace otevřené adresace s implicitním zřetězením položek
- Vhodný výběr rozptylovací hashovací funkce pro minimalizaci shlukování klíčů
- Řešení kolizí pomocí systematického posunu na další index v poli
- Rozlišení stavu jednotlivých položek v poli na volné, obsazené nebo dříve smazané
- Podpora pro vnořené rozsahy platnosti buď hierarchickým zřetězením tabulek, nebo zásobníkem rámců
- Důraz na samostatnou autorskou práci kvůli kontrole originality a obhajobám

## Fáze implementace a postupu práce

1. Teoretický návrh struktur, hashovací funkce a postupu řešení kolizí na papír
2. Příprava hlavičkového souboru s čistým rozhraním abstraktního datového typu
3. Implementace alokace tabulky a inicializace všech slotů
4. Implementace vkládací a vyhledávací funkce s ošetřením kolizních stavů
5. Implementace aktualizace dat a podpora více rozsahů platnosti pro těla funkcí a bloků
6. Implementace destruktoru tabulky s důsledným uvolněním všech alokovaných řetězců
7. Důkladná kontrola práce s pamětí pomocí nástroje Valgrind

## Samostatné testování

- Vytvoření vlastního testovacího programu s funkcí main pro zátěžové zkoušky tabulky
- Testy na masivní vkládání velkého množství identifikátorů se stejným i různým hashem
- Testy na kolize a následné vyhledání všech vložených symbolů
- Testy na pokus o vyhledání neexistujícího klíče
- Ověření nulových paměťových úniků při opakovaném vytváření a rušení tabulky

## Prostor na poznámky a otázky řešitele

- Zde si řešitel zapisuje zjištění a dotazy k napojení na sémantickou analýzu před vytvořením pull requestu
