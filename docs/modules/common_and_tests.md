# Modul Testovací infrastruktura a společné utility

## Základní informace a odpovědnost

- Zodpovídá: celý tým pod dohledem vedoucího
- Přiřazené soubory: src/error.h, src/str.c, src/str.h a testovací skripty ve složce tests
- Účel: sjednocení chybových stavů napříč celým projektem a automatizace ověřování funkčnosti

## Společné komponenty architektury

- Hlavičkový soubor error.h: centrální definice chybových maker a návratových kódů 0 až 7 a 99
- Pomocný modul pro dynamické řetězce str: zapouzdření práce s pamětí pro proměnlivě dlouhé identifikátory a řetězcové literály bez rizika přetečení bufferu
- Správa paměti: jednotný přístup k uvolňování alokovaných zdrojů při předčasném ukončení kvůli syntaktické či sémantické chybě

## Testovací infrastruktura

- Automatické end to end skripty: testovací runner pro kompilaci vzorových programů a jejich spuštění v interpretu ic26int
- Porovnání výstupů: kontrola produkovaného mezikódu přes nástroj diff proti referenčním výstupům
- Memory leak zkoušky: povinné spouštění testovacích sad pod nástrojem Valgrind
- Kompatibilita se Scalou: spouštění programů přes balíček ifj26.scala pro ověření sémantiky jazyka

## Povinné formální náležitosti odevzdání

- Vytvoření souboru rozdeleni: přesný formát řádků s osobním číslem, loginem a procentem bodů bez znaku procent s ukončením LF
- Registrace varianty: oficiální registrace varianty TRP izp v IS VUT do termínu 30. září 23:59
- Příprava na pokusné odevzdání: kontrola projektu skriptem is it ok před termínem 19. listopadu 23:59
- Zdrojové podklady pro finální dokumentaci: průběžné kreslení stavového diagramu, sestavování LL tabulky a precedenční tabulky pro závěrečné PDF
