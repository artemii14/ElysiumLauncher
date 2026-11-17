# Offline Russian catalog

`ru.ts` is from https://github.com/PrismLauncher/Translations at
61c44541defc9194f352e64a941f160e12ab0b48 (Apache-2.0, see LICENSE).
It supplies an offline fallback for the inherited Prism interface. Downloaded
catalogs override it; Elysium-specific strings use ElysiumTranslator.
The catalog is embedded as a Qt resource, so portable builds need no download.


Offline catalogs now include Russian, German, French, Spanish, Brazilian
Portuguese, Ukrainian and Simplified Chinese (zh). They are from the same
PrismLauncher/Translations repository and covered by the included Apache-2.0
license. English is the inherited source language. Historical translations are
retained because this ElyPrism base still uses their exact source strings.

`elysium.tsv` contains Elysium-specific translations (GPL-3.0-only, original
Elysium work). Columns: Russian canonical source, English, German, French,
Spanish, Brazilian Portuguese, Ukrainian, Simplified Chinese. Placeholders and
Minecraft IDs must not be translated. Unfinished upstream entries fall back to
source text; mod-provided metadata/error messages are not machine-translated.
