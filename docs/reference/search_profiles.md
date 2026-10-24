# Search scopes and creation profiles

A scope selects indexed material. A creation profile selects the generic native behaviour for the product.

## Choose a search scope

A scope selects what kind of material to search. These are the common choices:

| Scope | Use it for |
| --- | --- |
| `Headword` | Entry spellings and readings. Kanwa dictionaries use it for parent kanji entries. |
| `Idiom` | Phrases and idioms, or jukugo in kanwa dictionaries |
| `Example` | Example sentences and quotations |
| `Sense` | Keys associated with meanings or definitions |
| `Kanji` | Kanji lookup in dictionaries that provide it |

Names here use C++ spelling. Python uses uppercase names such as `HEADWORD`.
Other scopes are available for dictionaries that supply those indexes:

| Scope | Indexed material |
| --- | --- |
| `English` | English keys |
| `Collocation` | Collocations |
| `Fulltext` | Full-text search keys |
| `Category` | Categories or groups |
| `CompoundNoun` | Compound nouns. Python calls this `COMPOUND`. |
| `Numeral` | Counters and numeral expressions |
| `Modern` | Specialised modern-language lookup |
| `CJ`, `JC` | Chinese–Japanese and Japanese–Chinese lookup |

Selecting a scope does not scan article text. The dictionary must contain its
index. When authoring, the general profile supports all scopes above except
`Modern`, `CJ` and `JC`. The kanwa profile supports `Headword`, `Idiom`, `Sense`
and `Example`.

## Creation profiles

| Profile | Use it for | Native scopes |
| --- | --- | --- |
| `GENERAL` / `General` | General dictionaries, glossaries and reference works | All generic scopes in the tables above |
| `KANWA` / `Kanwa` | Kanji entries with independently searchable jukugo | `Headword`, `Idiom`, `Sense`, `Example` |

Pass the profile to `ProductWriter.create()` / `ProductWriter::create()`.
General is the default. Kanwa selects the generic kanwa delegate and writes its
expected keystore names automatically. Product-specific delegates cannot be authored.

These choices do not generate keys from text or determine navigation membership.
Add [keys](../authoring/search_keys.md) and [navigation](../authoring/navigation.md)
explicitly. See [native search rules](../searching/dictionary_rules.md) when reading
existing dictionaries with specialised profiles.
