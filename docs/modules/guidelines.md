# Pravidla pro styl kódu konvence a použité nástroje

## Základní informace a odpovědnost

- Zodpovídá: celý tým pod dohledem vedoucího
- Soubory: .clang_format, Makefile a konfigurační skripty
- Účel: sjednocení kultury psaní kódu, zamezení zbytečným merge konfliktům a splnění formálních podmínek zadání

## Povinné hlavičky souborů a komentáře

```c
/**
 * File: nazev_souboru.c
 * Project: IFJ26
 * Authors: Jméno Příjmení (xlogin00), ...
 * Translator: gcc 16.2.1
 */
```

- Každý zdrojový soubor s příponou `.c` a `.h` musí v úvodu obsahovat zakomentovaný název projektu.
- Povinné uvedení přihlašovacích jmen a celých jmen všech studentů podílejících se autorsky na daném souboru.
- Každá funkce musí mít v hlavičkovém nebo zdrojovém souboru stručný komentář s popisem rozhraní, parametrů a návratové hodnoty.
- **Povinnost pro umělou inteligenci:** Každý segment kódu vytvořený nástrojem umělé inteligence musí být označen komentářem s přesným zněním použitého promptu.

## Jmenné konvence v jazyce C

- **Funkce:** malá písmena oddělená podtržítkem ve stylu `snake_case` s prefixem modulu (například `scanner_get_next_token` nebo `symtable_search`).
- **Proměnné:** malá písmena ve stylu `snake_case` (například `token_count` nebo `current_char`).
- **Datové typy a struktury:** styl `PascalCase` nebo zakončení příponou `_t` (například `Token`, `TokenType` nebo `ast_node_t`).
- **Konstanty a položky maker výčtů:** velká písmena oddělená podtržítkem ve stylu `UPPER_SNAKE_CASE` (například `TOKEN_KEYWORD_DEF` nebo `ERR_LEXICAL`).
- **Identifikátory v mezikódu IFJcode25:** unikátní návěští generovat s prefixem dolar (`$`) nebo podtržítko (`_`) a jednoznačným čítačem.

## Konvence pro systém Git a větvení

- Přísný zákaz přímých zápisů a pushů do hlavní větve `main`.
- **Formát názvů pracovních větví:** `feat/scanner`, `feat/symtable`, `feat/parser`, `feat/codegen` nebo `fix/popis_chyby`.
- **Zprávy commitů:** podle zásad stručnosti s prefixem zasažené části (například `feat: add token buffer` nebo `fix: handle line indentation`).
- Každá změna prochází přes Pull Request se schválením alespoň jedním dalším členem týmu.
- Každý commit musí zanechat projekt v přeložitelném stavu.

## Vývojové nástroje a statická kontrola kvality

- **Formátovač `clang-format`:** povinný pro sjednocení odsazení a mezer napříč editory Neovim, Zed a VS Code.
- Použití příkazu `make format` před každým odesláním kódu do repozitáře.
- **Překladač:** `gcc` nebo `clang` s povinnými přísnými přepínači `-std=c1x -pedantic -Wall -Wextra -O2 -g`.
- **Kontrola paměti nástrojem Valgrind:** nulový výskyt chyb v přístupu k paměti a nulové paměťové úniky.
- **Referenční prostředí:** fakultní server Merlin pro finální ověření funkčnosti překladu i interpretace.
- **Validační skript:** povinné spouštění oficiálního skriptu `is_it_ok` před odevzdáním archivu.

```

```
