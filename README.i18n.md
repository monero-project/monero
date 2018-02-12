Monero daemon internationalization
==================================

The Monero command line tools can be translated in various languages. If you wish to contribute and need help/support, contact the [Monero Localization Workgroup on Taiga](https://taiga.getmonero.org/project/erciccione-monero-localization/) or come chat on `#monero-translations` (Freenode/IRC, riot/matrix, MatterMost)

In order to use the same translation workflow as the [Monero Core GUI](https://github.com/monero-project/monero-core), they use Qt Linguist translation files.  However, to avoid the dependencies on Qt this normally implies, they use a custom loader to read those files at runtime.

### Tools for translators

In order to create, update or build translations files, you need to have Qt tools installed. For translating, you need either the **Qt Linguist GUI** ([part of QT Creator](https://www.qt.io/download-open-source/#allDownloadsDiv-9) or a [3rd-party standalone version](https://github.com/lelegard/qtlinguist-installers/releases)), or another tool that supports Qt ts files, such as Transifex.  The files are XML, so they can be edited in any plain text editor if needed.

### Creating / modifying translations

You do not need anything from Qt in order to use the final translations.

To update ts files after changing source code:

    ./utils/translations/update-translations.sh

To add a new language, eg Spanish (ISO code es):

    cp translations/monero.ts translations/monero_es.ts

To edit translations for Spanish:

    linguist translations/monero_es.ts

To build translations after modifying them:

    ./utils/translations/build-translations.sh

To test a translation:

    LANG=es ./build/release/bin/monero-wallet-cli

To add new translatable strings in the source code:

Use the `tr(string)` function if possible. If the code is in a class, and this class doesn't already have a `tr()` static function, add one, which uses a context named after what `lupdate` uses for the context, usually the fully qualified class name (eg, `cryptonote::simple_wallet`).  If you need to use `tr()` in code that's not in a class, you can use the fully qualified version (eg, `simple_wallet::tr`) of the one matching the context you want. Use `QT_TRANSLATE_NOOP(string)` if you want to specify a context manually.

If you're getting messages of the form:

    Class 'cryptonote::simple_wallet' lacks Q_OBJECT macro

all is fine, we don't actually need that here.
