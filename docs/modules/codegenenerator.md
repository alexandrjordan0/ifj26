# Modul Generátor cílového kódu a precedenční syntaktická analýza výrazů

## Základní informace a odpovědnost

- Odpovědná osoba: Saša
- Přiřazené soubory: src/codegenerator.c, src/codegenerator.h a src/precedence.c, src/precedence.h
- Základní principy: operátorová precedenční analýza pro výrazy a překlad sémantických struktur do instrukcí tříadresného a zásobníkového mezikódu IFJcode26
- Spolupráce: přebírání tokenů ze scanneru pro výrazy, čtení typových dat ze symtable, generování instrukcí na standardní výstup pro interpret ic26int

## Rozhraní modulu codegenerator.h a precedence.h

- Hlavní funkce precedenční analýzy: parse_expression s návratem syntaktického stromu výrazu nebo jeho okamžitým vyhodnocením
- Hlavní funkce generátoru: gen_init pro tisk hlavičky a globálních struktur, gen_function_start a gen_function_end pro těla funkcí, gen_statement pro příkazy
- Řízení paměťových rámců: explicitní přepínání mezi globálním rámcem GF, dočasným rámcem TF a lokálním rámcem LF
- Návratové chování: vracení kódu 0 při úspěšném generování, návratové kódy 2 až 7 při chybě ve výrazech nebo sémantice, návratový kód 99 při selhání alokace paměti

## Požadavky na generování kódu pro jazyk IFJcode26

- Povinný úvodní řádek .IFJcode26 na začátku programu
- Inicializace globálního rámce GF a příprava skoku na návěští vstupní funkce označené anotací @main
- Správa unikátních návěští pro podmíněné větvení if else a cykly while pomocí jednoznačných čítačů
- Správná tvorba dočasného rámce TF instrukcí CREATEFRAME před voláním funkce a jeho přesun do LF instrukcí PUSHFRAME
- Realizace volání funkcí instrukcí CALL a návrat z funkce instrukcí RETURN s úklidem lokálního rámce POPFRAME
- Realizace vyhodnocování výrazů s využitím datového zásobníku a instrukcí zakončených písmenem S
- Zpracování vestavěných funkcí readString, readInt, readDouble, write, toStr, chr s přesným ošetřením mezních hodnot
- Formátování konstant: celá čísla int s prefixem int@, desetinná čísla float s prefixem float@ a formátem procento a, pravdivostní hodnoty bool@true a bool@false, řetězce string@ s povinným nahrazením mezer, mřížek a řídicích znaků escape sekvencí lomítko xyz
- Integrace rozšíření BOOLTHEN: plná podpora typu Boolean ve výrazech, ukládání hodnot true a false do proměnných, generování logických operací AND, OR, NOT
- Integrace rozšíření FUNEXP: povolení volání funkcí jako operandů ve výrazech s bezpečným odložením rozpracovaného datového zásobníku

## Fáze implementace a postupu práce

1. Sestavení precedenční tabulky operátorů na papír včetně asociativity a priorit jako součást dokumentace
2. Příprava hlavičkových souborů pro precedenční analýzu a generátor instrukcí
3. Implementace generování úvodního kódu programu, definice proměnných a přiřazení
4. Implementace zásobníku precedenčního parseru a redukčních pravidel pro operátory
5. Implementace generování instrukcí pro skoky, řídicí konstrukce if else a cyklus while
6. Implementace volání uživatelských funkcí, předávání parametrů a návratových hodnot přes rámce
7. Implementace těla vestavěných funkcí a integrace s výstupy rekurzivního sestupu

## Samostatné testování

- Tvorba testovacích skriptů porovnávajících generovaný výstup s interpretem ic26int přes nástroj diff
- Testy matematických a relačních výrazů na správnou prioritu operátorů a uzávorkování
- Testy na vnořené podmínky a složité cykly pro ověření unikátnosti generovaných návěští
- Testy na rekurzivní volání funkcí s kontrolou správné hloubky zásobníku rámců
- Ověření nulových paměťových úniků generátoru přes nástroj Valgrind

## Prostor na poznámky a otázky řešitele

- Zde si řešitel zapisuje zjištění, návrhy na formát AST a pravidla synchronizace s parserem před zavedením do hlavní větve
