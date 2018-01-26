<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE TS []>
<TS version="2.1">
<context>
    <name>Monero::AddressBookImpl</name>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="55"/>
        <source>Invalid destination address</source>
        <translation>Ogiltig måladress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="65"/>
        <source>Invalid payment ID. Short payment ID should only be used in an integrated address</source>
        <translation>Ogiltigt betalnings-ID. Kort betalnings-ID ska endast användas i en integrerad adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="72"/>
        <source>Invalid payment ID</source>
        <translation>Ogiltigt betalnings-ID</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="79"/>
        <source>Integrated address and long payment id can&apos;t be used at the same time</source>
        <translation>Integrerad adress och långt betalnings-ID kan inte användas samtidigt</translation>
    </message>
</context>
<context>
    <name>Monero::PendingTransactionImpl</name>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="90"/>
        <source>Attempting to save transaction to file, but specified file(s) exist. Exiting to not risk overwriting. File:</source>
        <translation>Försöker spara transaktion till fil, men angiven fil finns redan. Avslutar för att inte riskera att skriva över någonting. Fil:</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="97"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Det gick inte att skriva transaktioner till fil</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="114"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>daemonen är upptagen. Försök igen senare.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="117"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>ingen anslutning till daemonen. Se till att daemonen körs.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="121"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>transaktionen %s avvisades av daemonen med status: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="126"/>
        <source>. Reason: </source>
        <translation>. Orsak: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="128"/>
        <source>Unknown exception: </source>
        <translation>Okänt undantag: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="131"/>
        <source>Unhandled exception</source>
        <translation>Ohanterat undantag</translation>
    </message>
</context>
<context>
    <name>Monero::UnsignedTransactionImpl</name>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="75"/>
        <source>This is a watch only wallet</source>
        <translation>Detta är en granskningsplånbok</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="85"/>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="92"/>
        <source>Failed to sign transaction</source>
        <translation>Det gick inte att signera transaktionen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="135"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>Begärd växel går inte till en betald adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="141"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>Begärd växel är större än betalning till växeladressen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="151"/>
        <source>Change goes to more than one address</source>
        <translation>Växel går till mer än en adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="164"/>
        <source>sending %s to %s</source>
        <translation>skickar %s till %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="170"/>
        <source>with no destinations</source>
        <translation>utan några mål</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="176"/>
        <source>%s change to %s</source>
        <translation>%s växel till %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="179"/>
        <source>no change</source>
        <translation>ingen växel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="181"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min mixin %lu. %s</source>
        <translation>Läste in %lu transaktioner, för %s, avgift %s, %s, %s, med minsta mixin %lu. %s</translation>
    </message>
</context>
<context>
    <name>Monero::WalletImpl</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="942"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>betalnings-ID har ogiltigt format. En 16- eller 64-teckens hex-sträng förväntades: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="952"/>
        <source>Failed to add short payment id: </source>
        <translation>Det gick inte att lägga till kort betalnings-ID: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="978"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1072"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>daemonen är upptagen. Försök igen senare.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="981"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1075"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>ingen anslutning till daemonen. Se till att daemonen körs.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="984"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1078"/>
        <source>RPC error: </source>
        <translation>RPC-fel: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1081"/>
        <source>failed to get random outputs to mix</source>
        <translation>det gick inte att hämta slumpmässiga utgångar att mixa</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="994"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1088"/>
        <source>not enough money to transfer, available only %s, sent amount %s</source>
        <translation>inte tillräckligt med pengar för överföring, endast tillgängligt %s, skickat belopp %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="403"/>
        <source>failed to parse address</source>
        <translation>det gick inte att parsa adressen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="415"/>
        <source>failed to parse secret spend key</source>
        <translation>det gick inte att parsa hemlig spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="425"/>
        <source>No view key supplied, cancelled</source>
        <translation>Ingen visningsnyckel angiven, avbruten</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="432"/>
        <source>failed to parse secret view key</source>
        <translation>det gick inte att parsa hemlig visningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="442"/>
        <source>failed to verify secret spend key</source>
        <translation>det gick inte att verifiera hemlig spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="447"/>
        <source>spend key does not match address</source>
        <translation>spendernyckel matchar inte adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="453"/>
        <source>failed to verify secret view key</source>
        <translation>det gick inte att verifiera hemlig visningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="458"/>
        <source>view key does not match address</source>
        <translation>visningsnyckel matchar inte adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="477"/>
        <source>failed to generate new wallet: </source>
        <translation>det gick inte att skapa ny plånbok: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="799"/>
        <source>Failed to load unsigned transactions</source>
        <translation>Det gick inte att läsa in osignerade transaktioner</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="820"/>
        <source>Failed to load transaction from file</source>
        <translation>Det gick inte att läsa in transaktion från fil</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="838"/>
        <source>Wallet is view only</source>
        <translation>Plånboken är endast för granskning</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="847"/>
        <source>failed to save file </source>
        <translation>det gick inte att spara fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="874"/>
        <source>Failed to import key images: </source>
        <translation>det gick inte att importera nyckelavbildningar: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="987"/>
        <source>failed to get random outputs to mix: %s</source>
        <translation>det gick inte att hitta slumpmässiga utgångar att mixa: %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1003"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1097"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>ej tillräckligt med pengar för överföring, endast tillgängligt %s, transaktionsbelopp %s = %s + %s (avgift)</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1012"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1106"/>
        <source>not enough outputs for specified mixin_count</source>
        <translation>inte tillräckligt många utgångar för angiven mixin_count</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1014"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1108"/>
        <source>output amount</source>
        <translation>utgångens belopp</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1014"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1108"/>
        <source>found outputs to mix</source>
        <translation>hittade utgångar att mixa</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1019"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1113"/>
        <source>transaction was not constructed</source>
        <translation>transaktionen konstruerades inte</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1023"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1117"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>transaktionen %s avvisades av daemonen med status: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1030"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1124"/>
        <source>one of destinations is zero</source>
        <translation>ett av målen är noll</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1033"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1127"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>det gick inte att hitta ett lämpligt sätt att dela upp transaktioner</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1036"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1130"/>
        <source>unknown transfer error: </source>
        <translation>okänt överföringsfel: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1039"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1133"/>
        <source>internal error: </source>
        <translation>internt fel: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1042"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1136"/>
        <source>unexpected error: </source>
        <translation>oväntat fel: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1045"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1139"/>
        <source>unknown error</source>
        <translation>okänt fel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1419"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>Genomsök efter spenderade kan endast användas med en betrodd daemon</translation>
    </message>
</context>
<context>
    <name>Monero::WalletManagerImpl</name>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="192"/>
        <source>failed to parse txid</source>
        <translation>det gick inte att parsa transaktions-id</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="199"/>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="206"/>
        <source>failed to parse tx key</source>
        <translation>det gick inte att parsa transaktionsnyckeln</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="217"/>
        <source>failed to parse address</source>
        <translation>det gick inte att parsa adressen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="227"/>
        <source>failed to get transaction from daemon</source>
        <translation>det gick inte att hämta transaktion från daemon</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="238"/>
        <source>failed to parse transaction from daemon</source>
        <translation>det gick inte att parsa transaktion från daemonen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="245"/>
        <source>failed to validate transaction from daemon</source>
        <translation>det gick inte att validera transaktion från daemon</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="250"/>
        <source>failed to get the right transaction from daemon</source>
        <translation>det gick inte att hämta rätt transaktion från daemonen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="257"/>
        <source>failed to generate key derivation from supplied parameters</source>
        <translation>det gick inte att skapa nyckelhärledning från angivna parametrar</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="313"/>
        <source>error: </source>
        <translation>fel: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="319"/>
        <source>received</source>
        <translation>mottaget</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="319"/>
        <source>in txid</source>
        <translation>i transaktions-id</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="323"/>
        <source>received nothing in txid</source>
        <translation>tog emot ingenting i transaktions-id</translation>
    </message>
</context>
<context>
    <name>Wallet</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="212"/>
        <source>Failed to parse address</source>
        <translation>Det gick inte att parsa adressen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="219"/>
        <source>Failed to parse key</source>
        <translation>Det gick inte att parsa nyckeln</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="227"/>
        <source>failed to verify key</source>
        <translation>det gick inte att verifiera nyckeln</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="237"/>
        <source>key does not match address</source>
        <translation>nyckeln matchar inte adressen</translation>
    </message>
</context>
<context>
    <name>command_line</name>
    <message>
        <location filename="../src/common/command_line.cpp" line="76"/>
        <source>yes</source>
        <translation>ja</translation>
    </message>
</context>
<context>
    <name>cryptonote::rpc_args</name>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="38"/>
        <source>Specify ip to bind rpc server</source>
        <translation>Ange IP-adress för att binda till RPC-server</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="39"/>
        <source>Specify username[:password] required for RPC server</source>
        <translation>Ange användarnamn[:lösenord] som krävs av RPC-servern</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="40"/>
        <source>Confirm rpc-bind-ip value is NOT a loopback (local) IP</source>
        <translation>Bekräftelsevärde för rpc-bind-ip är INTE ett lokalt (loopback) IP</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="66"/>
        <source>Invalid IP address given for --</source>
        <translation>Ogiltig IP-adress angiven för --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="74"/>
        <source> permits inbound unencrypted external connections. Consider SSH tunnel or SSL proxy instead. Override with --</source>
        <translation> tillåter inkommande okrypterade externa anslutningar. Överväg att använda SSH-tunnel eller SSL-proxy istället. Åsidosätt med --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="89"/>
        <source>Username specified with --</source>
        <translation>Användarnamn angivet med --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="89"/>
        <source> cannot be empty</source>
        <translation> kan inte vara tomt</translation>
    </message>
</context>
<context>
    <name>cryptonote::simple_wallet</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="290"/>
        <source>Commands: </source>
        <translation>Kommandon: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1557"/>
        <source>failed to read wallet password</source>
        <translation>det gick inte att läsa lösenord för plånboken</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1325"/>
        <source>invalid password</source>
        <translation>ogiltigt lösenord</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="697"/>
        <source>start_mining [&lt;number_of_threads>] - Start mining in daemon</source>
        <translation>start_mining [&lt;antal_trådar>] - Starta brytning i daemonen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="698"/>
        <source>Stop mining in daemon</source>
        <translation>Avbryt brytning i daemonen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="699"/>
        <source>Save current blockchain data</source>
        <translation>Spara aktuella blockkedje-data</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="701"/>
        <source>Show current wallet balance</source>
        <translation>Visa aktuellt saldo för plånboken</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="704"/>
        <source>Show blockchain height</source>
        <translation>Visa blockkedjans höjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="715"/>
        <source>Show current wallet public address</source>
        <translation>Visa plånbokens aktuella öppna adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="744"/>
        <source>Show this help</source>
        <translation>Visa denna hjälp</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="788"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation>set seed: behöver ett argument. tillgängliga alternativ: språk</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="811"/>
        <source>set: unrecognized argument(s)</source>
        <translation>set: okända argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1442"/>
        <source>wallet file path not valid: </source>
        <translation>ogiltig sökväg till plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="863"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation>Försöker skapa eller återställa plånbok, men angivna filer existerar.  Avslutar för att inte riskera att skriva över någonting.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="416"/>
        <source>usage: payment_id</source>
        <translation>användning: payment_id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="710"/>
        <source>sweep_below &lt;amount_threshold> [mixin] address [payment_id] - Send all unlocked outputs below the threshold to an address</source>
        <translation>sweep_below &lt;tröskelbelopp> [mixin] &lt;adress> [&lt;betalnings_ID>] - Skicka alla upplåsta utgångar under tröskelbeloppet till en adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="743"/>
        <source>Generate a new random full size payment id - these will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids</source>
        <translation>Skapa ett nytt slumpmässigt betalnings-ID av full storlek - dessa kommer att vara okrypterade på blockkedjan, se integrated_address för krypterade korta betalnings-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="774"/>
        <source>needs an argument</source>
        <translation>kräver ett argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="797"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="798"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="799"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="801"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="804"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="805"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="809"/>
        <source>0 or 1</source>
        <translation>0 eller 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="800"/>
        <source>integer >= 2</source>
        <translation>heltal >= 2</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="803"/>
        <source>0, 1, 2, 3, or 4</source>
        <translation>0, 1, 2, 3 eller 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="807"/>
        <source>unsigned integer</source>
        <translation>positivt heltal</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="912"/>
        <source>NOTE: the following 25 words can be used to recover access to your wallet. Write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation>OBSERVERA: följande 25 ord kan användas för att återfå tillgång till din plånbok. Skriv ner och spara dem på ett säkert ställe. Spara dem inte i din e-post eller på något lagringsutrymme som du inte har direkt kontroll över.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="958"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-deterministic-wallet använder --generate-new-wallet, inte --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="973"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation>ange en återställningsparameter med --electrum-seed=&quot;ordlista här&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1123"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation>ange sökväg till en plånbok med --generate-new-wallet (inte --wallet-file)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1261"/>
        <source>wallet failed to connect to daemon: </source>
        <translation>plånboken kunde inte ansluta till daemonen: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1269"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation>Daemonen använder en högre version av RPC (%u) än plånboken (%u): %s. Antingen uppdatera en av dem, eller använd --allow-mismatched-daemon-version.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1288"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation>Lista över tillgängliga språk för din plånboks frö:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1297"/>
        <source>Enter the number corresponding to the language of your choice: </source>
        <translation>Ange det tal som motsvarar det språk du vill använda: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1354"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation>Du hade använt en inaktuell version av plånboken. Använd det nya frö som vi tillhandahåller.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1368"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1425"/>
        <source>Generated new wallet: </source>
        <translation>Ny plånbok skapad: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1374"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1430"/>
        <source>failed to generate new wallet: </source>
        <translation>det gick inte att skapa ny plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1457"/>
        <source>Opened watch-only wallet</source>
        <translation>Öppnade plånbok för granskning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1457"/>
        <source>Opened wallet</source>
        <translation>Öppnade plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1466"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation>Du hade använt en inaktuell version av plånboken. Fortsätt för att uppgradera din plånbok.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1481"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation>Du hade använt en inaktuell version av plånboken. Plånbokens filformat kommer nu att uppgraderas.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1489"/>
        <source>failed to load wallet: </source>
        <translation>det gick inte att läsa in plånboken: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1497"/>
        <source>Use the &quot;help&quot; command to see the list of available commands.
</source>
        <translation>Använd kommandot &quot;help&quot; för att se en lista över tillgängliga kommandon.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1541"/>
        <source>Wallet data saved</source>
        <translation>Plånboksdata sparades</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1613"/>
        <source>Mining started in daemon</source>
        <translation>Brytning startad i daemonen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1615"/>
        <source>mining has NOT been started: </source>
        <translation>brytning har INTE startats: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1634"/>
        <source>Mining stopped in daemon</source>
        <translation>Brytning stoppad i daemonen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1636"/>
        <source>mining has NOT been stopped: </source>
        <translation>brytning har INTE stoppats: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1655"/>
        <source>Blockchain saved</source>
        <translation>Blockkedjan sparad</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1670"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1687"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1699"/>
        <source>Height </source>
        <translation>Höjd </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1671"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1688"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1700"/>
        <source>transaction </source>
        <translation>transaktion </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1672"/>
        <source>received </source>
        <translation>mottaget </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1689"/>
        <source>spent </source>
        <translation>spenderad </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1701"/>
        <source>unsupported transaction format</source>
        <translation>transaktionsformatet stöds inte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1718"/>
        <source>Starting refresh...</source>
        <translation>Startar uppdatering …</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1731"/>
        <source>Refresh done, blocks received: </source>
        <translation>Uppdatering färdig, mottagna block: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2186"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2701"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>betalnings-ID har ogiltigt format. En 16- eller 64-teckens hex-sträng förväntades: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2201"/>
        <source>bad locked_blocks parameter:</source>
        <translation>dålig parameter för locked_blocks:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2228"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2726"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation>en enstaka transaktion kan inte använda mer än ett betalnings-ID: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2237"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2735"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>det gick inte att upprätta betalnings-ID, trots att det avkodades korrekt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2262"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2355"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2533"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2749"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2794"/>
        <source>transaction cancelled.</source>
        <translation>transaktion avbröts.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2323"/>
        <source>Sending %s.  </source>
        <translation>Skickar %s.  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2326"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation>Transaktionen behöver delas upp i %llu transaktioner.  Detta kommer att göra att en transaktionsavgift läggs till varje transaktion, med ett totalt belopp på %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2332"/>
        <source>The transaction fee is %s</source>
        <translation>Transaktionsavgiften är %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2335"/>
        <source>, of which %s is dust from change</source>
        <translation>, varav %s är damm från växel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2336"/>
        <source>.</source>
        <translation>.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2336"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation>Ett total belopp på %s från växeldamm kommer att skickas till damm-adressen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2341"/>
        <source>.
This transaction will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation>.
Denna transaktion kommer att låsas upp vid block %llu, om ungefär %s dagar (förutsatt en blocktid på 2 minuter)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2367"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2544"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2805"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Det gick inte att skriva transaktioner till fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2371"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2548"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2809"/>
        <source>Unsigned transaction(s) successfully written to file: </source>
        <translation>Osignerade transaktioner skrevs till fil: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2406"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2583"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2844"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3157"/>
        <source>Not enough money in unlocked balance</source>
        <translation>Inte tillräckligt med pengar i upplåst saldo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2415"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2592"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2853"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3166"/>
        <source>Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees</source>
        <translation>Det gick inte att hitta något sätt att skapa transaktioner. Detta beror vanligtvis på damm som är så litet att det inte kan betala för sig självt i avgifter, eller ett försök att skicka mer pengar än upplåst saldo, eller att inte lämna tillräckligt för att täcka avgifterna</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2435"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2612"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2873"/>
        <source>Reason: </source>
        <translation>Orsak: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2447"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2624"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2885"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>det gick inte att hitta ett lämpligt sätt att dela upp transaktioner</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2503"/>
        <source>No unmixable outputs found</source>
        <translation>Inga omixbara utgångar kunde hittas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2709"/>
        <source>No address given</source>
        <translation>Ingen adress har angivits</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2921"/>
        <source>missing amount threshold</source>
        <translation>beloppströskel saknas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2926"/>
        <source>invalid amount threshold</source>
        <translation>ogiltig beloppströskel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3008"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>Begärd växel går inte till en betald adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3013"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>Begärd växel är större än betalning till växeladressen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3035"/>
        <source>sending %s to %s</source>
        <translation>skickar %s till %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3041"/>
        <source>with no destinations</source>
        <translation>utan några mål</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3088"/>
        <source>Failed to sign transaction</source>
        <translation>Det gick inte att signera transaktionen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3094"/>
        <source>Failed to sign transaction: </source>
        <translation>Det gick inte att signera transaktion: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3120"/>
        <source>Failed to load transaction from file</source>
        <translation>Det gick inte att läsa in transaktion från fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3137"/>
        <source>daemon is busy. Please try later</source>
        <translation>daemonen är upptagen. Försök senare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1745"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1995"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2395"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2572"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2833"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3146"/>
        <source>RPC error: </source>
        <translation>RPC-fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="312"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation>plånboken är enbart för granskning och har ingen spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="390"/>
        <source>Your original password was incorrect.</source>
        <translation>Ditt ursprungliga lösenord var fel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="404"/>
        <source>Error with wallet rewrite: </source>
        <translation>Fel vid återskrivning av plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="513"/>
        <source>priority must be 0, 1, 2, 3, or 4 </source>
        <translation>prioritet måste vara 0, 1, 2, 3 eller 4 </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="525"/>
        <source>priority must be 0, 1, 2, 3,or 4</source>
        <translation>prioritet måste vara 0, 1, 2, 3 eller 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="540"/>
        <source>priority must be 0, 1, 2 3,or 4</source>
        <translation>prioritet måste vara 0, 1, 2, 3 eller 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="623"/>
        <source>invalid unit</source>
        <translation>ogiltig enhet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="641"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation>ogiltigt värde för count: måste vara ett heltal utan tecken</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="659"/>
        <source>invalid value</source>
        <translation>ogiltigt värde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="705"/>
        <source>Same as transfer, but using an older transaction building algorithm</source>
        <translation>Samma som transfer, men använder en äldre algoritm för att bygga transaktionen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="709"/>
        <source>sweep_all [mixin] address [payment_id] - Send all unlocked balance to an address</source>
        <translation>sweep_all [mixin] adress [betalnings_id] - Skicka allt upplåst saldo till en adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="711"/>
        <source>donate [&lt;mixin_count>] &lt;amount> [payment_id] - Donate &lt;amount> to the development team (donate.getmonero.org)</source>
        <translation>donate [&lt;mixin_antal>] &lt;belopp> [&lt;betalnings_id>] - Donera &lt;belopp> till utvecklingsteamet (donate.getmonero.org)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="714"/>
        <source>set_log &lt;level>|&lt;categories> - Change current log detail (level must be &lt;0-4>)</source>
        <translation>set_log &lt;nivå>|&lt;kategorier> - Ändra detaljnivån för aktuell logg (nivå måste vara 0-4)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="717"/>
        <source>address_book [(add (&lt;address> [pid &lt;long or short payment id>])|&lt;integrated address> [&lt;description possibly with whitespaces>])|(delete &lt;index>)] - Print all entries in the address book, optionally adding/deleting an entry to/from it</source>
        <translation>address_book [(add (&lt;adress> [pid &lt;långt eller kort betalnings-ID>])|&lt;integrerad adress> [&lt;beskrivning eventuellt med blanktecken>])|(delete &lt;index>)] - Skriv ut alla poster i adressboken, eventuellt lägg till/ta bort en post till/från den</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="729"/>
        <source>show_transfers [in|out|pending|failed|pool] [&lt;min_height> [&lt;max_height>]] - Show incoming/outgoing transfers within an optional height range</source>
        <translation>show_transfers [in|out|pending|failed|pool] [&lt;min_höjd> [&lt;max_höjd>]] - Visa inkommande/utgående överföringar inom ett valfritt höjdintervall</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="741"/>
        <source>Show information about a transfer to/from this address</source>
        <translation>Visa information om en överföring till/från denna adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="742"/>
        <source>Change wallet password</source>
        <translation>Ändra lösenord för plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="820"/>
        <source>usage: set_log &lt;log_level_number_0-4> | &lt;categories></source>
        <translation>användning: set_log &lt;loggnivå_nummer_0-4> | &lt;kategorier></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="886"/>
        <source>(Y/Yes/N/No): </source>
        <translation>(J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1157"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1184"/>
        <source>bad m_restore_height parameter: </source>
        <translation>dålig parameter för m_restore_height: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1162"/>
        <source>date format must be YYYY-MM-DD</source>
        <translation>datumformat måste vara ÅÅÅÅ-MM-DD</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1175"/>
        <source>Restore height is: </source>
        <translation>Återställningshöjd är: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1176"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2348"/>
        <source>Is this okay?  (Y/Yes/N/No): </source>
        <translation>Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1212"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Daemonen är lokal, utgår från att den är betrodd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1553"/>
        <source>Password for new watch-only wallet</source>
        <translation>Lösenord för ny granskningsplånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1604"/>
        <source>invalid arguments. Please use start_mining [&lt;number_of_threads>] [do_bg_mining] [ignore_battery], &lt;number_of_threads> should be from 1 to </source>
        <translation>ogiltiga argument. Använd start_mining [&lt;antal_trådar>] [do_bg_mining] [ignore_battery], &lt;antal_trådar> ska vara från 1 till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1755"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2457"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2634"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2895"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3205"/>
        <source>internal error: </source>
        <translation>internt fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1760"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2000"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2462"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2639"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2900"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3210"/>
        <source>unexpected error: </source>
        <translation>oväntat fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1765"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2005"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2467"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2644"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2905"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3215"/>
        <source>unknown error</source>
        <translation>okänt fel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1770"/>
        <source>refresh failed: </source>
        <translation>det gick inte att att uppdatera: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1770"/>
        <source>Blocks received: </source>
        <translation>Mottagna block: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1795"/>
        <source>unlocked balance: </source>
        <translation>upplåst saldo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="808"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>amount</source>
        <translation>belopp</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <source>spent</source>
        <translation>spenderad</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <source>global index</source>
        <translation>globalt index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <source>tx id</source>
        <translation>tx-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1868"/>
        <source>No incoming transfers</source>
        <translation>Inga inkommande överföringar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1872"/>
        <source>No incoming available transfers</source>
        <translation>Inga inkommande tillgängliga överföringar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1876"/>
        <source>No incoming unavailable transfers</source>
        <translation>Inga inkommande otillgängliga överföringar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1887"/>
        <source>expected at least one payment_id</source>
        <translation>åtminstone ett payment_id förväntades</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>payment</source>
        <translation>betalning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>transaction</source>
        <translation>transaktion</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>height</source>
        <translation>höjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>unlock time</source>
        <translation>upplåsningstid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1908"/>
        <source>No payments with id </source>
        <translation>Inga betalningar med ID </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1960"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2026"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2280"/>
        <source>failed to get blockchain height: </source>
        <translation>det gick inte att hämta blockkedjans höjd: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2016"/>
        <source>failed to connect to the daemon</source>
        <translation>det gick inte att ansluta till daemonen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2034"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation>
Transaktion %llu/%llu: txid=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2044"/>
        <source>
Input %llu/%llu: amount=%s</source>
        <translation>
Ingång %llu/%llu: belopp=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2060"/>
        <source>failed to get output: </source>
        <translation>det gick inte att hämta utgång: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2068"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation>utgångsnyckelns ursprungsblockhöjd får inte vara högre än blockkedjans höjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2072"/>
        <source>
Originating block heights: </source>
        <translation>
Ursprungsblockhöjder: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2087"/>
        <source>
|</source>
        <translation>
|</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2087"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3915"/>
        <source>|
</source>
        <translation>|
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2104"/>
        <source>
Warning: Some input keys being spent are from </source>
        <translation>
Varning: Några ingångsnycklar som spenderas kommer från </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2106"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation>, vilket kan bryta ringsignaturens anonymitet. Se till att detta är avsiktligt!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2152"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2937"/>
        <source>wrong number of arguments</source>
        <translation>fel antal argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2257"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2744"/>
        <source>No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): </source>
        <translation>Inget betalnings-ID har inkluderats med denna transaktion. Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2298"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2762"/>
        <source>No outputs found, or daemon is not ready</source>
        <translation>Inga utgångar hittades, eller så är daemonen inte redo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2399"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2576"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2837"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3150"/>
        <source>failed to get random outputs to mix: </source>
        <translation>det gick inte att hitta slumpmässiga utgångar att mixa: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2518"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2779"/>
        <source>Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Sveper upp %s i %llu transaktioner för en total avgift på %s.  Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2524"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Sveper upp %s för en total avgift på %s.  Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2969"/>
        <source>Donating </source>
        <translation>Donerar </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3053"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min mixin %lu. %sIs this okay? (Y/Yes/N/No): </source>
        <translation>Läste in %lu transaktioner, för %s, avgift %s, %s, %s, med minsta mixin %lu. %sÄr detta okej? (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3077"/>
        <source>This is a watch only wallet</source>
        <translation>Detta är en granskningsplånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4443"/>
        <source>usage: show_transfer &lt;txid></source>
        <translation>användning: show_transfer &lt;transaktions-ID></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4557"/>
        <source>Transaction ID not found</source>
        <translation>Transaktions-ID kunde inte hittas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="237"/>
        <source>true</source>
        <translation>sant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="266"/>
        <source>failed to parse refresh type</source>
        <translation>det gick inte att parsa uppdateringstyp</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="330"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="362"/>
        <source>wallet is watch-only and has no seed</source>
        <translation>plånboken är enbart för granskning och saknar frö</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="353"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="367"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation>plånboken är icke-deterministisk och saknar frö</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="450"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="467"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation>plånboken är enbart för granskning och kan inte göra överföringar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="474"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="480"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="496"/>
        <source>mixin must be an integer >= 2</source>
        <translation>mixin måste vara ett heltal >= 2</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="501"/>
        <source>could not change default mixin</source>
        <translation>det gick inte att ändra standardinställning för mixin</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="545"/>
        <source>could not change default priority</source>
        <translation>Det gick inte att ändra standardinställning för prioritet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="700"/>
        <source>Synchronize transactions and balance</source>
        <translation>Synkronisera transaktioner och saldo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="702"/>
        <source>incoming_transfers [available|unavailable] - Show incoming transfers, all or filtered by availability</source>
        <translation>incoming_transfers [available|unavailable] - Visa inkommande överföringar, alla eller filtrerade efter tillgänglighet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="703"/>
        <source>payments &lt;PID_1> [&lt;PID_2> ... &lt;PID_N>] - Show payments for given payment ID[s]</source>
        <translation>payments &lt;B_ID_1> [&lt;B_ID_2> … &lt;B_ID_N>] - Visa betalningar för angivna betalnings-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="706"/>
        <source>transfer [&lt;priority>] [&lt;mixin_count>] &lt;address> &lt;amount> [&lt;payment_id>] - Transfer &lt;amount> to &lt;address>. &lt;priority> is the priority of the transaction. The higher the priority, the higher the fee of the transaction. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;mixin_count> is the number of extra inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2> &lt;amount_2> etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>transfer [&lt;prioritet>] [&lt;mixin_antal>] &lt;adress> &lt;belopp> [&lt;betalnings_id>] - Överför &lt;belopp> till &lt;adress>. &lt;prioritet> är transaktionens prioritet. Ju högre prioritet, desto högre transaktionsavgift. Giltiga värden i prioritetsordning (från lägsta till högsta) är: unimportant, normal, elevated, priority. Om det utelämnas kommer standardvärdet (se kommandot &quot;set priority&quot;) att användas. &lt;mixin_antal> är det antal extra ingångar som ska inkluderas för att uppnå ospårbarhet. Flera betalningar kan göras på en gång genom att lägga till &lt;adress_2> &lt;belopp_2> etcetera (före betalnings-ID, om det inkluderas)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="707"/>
        <source>locked_transfer [&lt;mixin_count>] &lt;addr> &lt;amount> &lt;lockblocks>(Number of blocks to lock the transaction for, max 1000000) [&lt;payment_id>]</source>
        <translation>locked_transfer [&lt;mixin_antal>] &lt;adress> &lt;belopp> &lt;låsblock>(Antal block som transaktionen ska låsas för, max 1000000) [&lt;betalnings_id>]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="708"/>
        <source>Send all unmixable outputs to yourself with mixin 0</source>
        <translation>Skicka alla omixbara utgångar till dig själv med mixin 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="712"/>
        <source>Sign a transaction from a file</source>
        <translation>Signera en transaktion från en fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="713"/>
        <source>Submit a signed transaction from a file</source>
        <translation>Skicka en signerad transaktion från en fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="716"/>
        <source>integrated_address [PID] - Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation>integrated_address [PID] - Koda ett betalnings-ID till en integrerad adress för den aktuella plånbokens öppna adress (utan argument används ett slumpmässigt betalnings-ID), eller avkoda en integrerad adress till standardadress och betalnings-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="718"/>
        <source>Save wallet data</source>
        <translation>Spara plånboksdata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="719"/>
        <source>Save a watch-only keys file</source>
        <translation>Spara en fil med granskningsnycklar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="720"/>
        <source>Display private view key</source>
        <translation>Visa privat visningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="721"/>
        <source>Display private spend key</source>
        <translation>Visa privat spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="722"/>
        <source>Display Electrum-style mnemonic seed</source>
        <translation>Visa minnesfrö (Electrum-typ)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="723"/>
        <source>Available options: seed language - set wallet seed language; always-confirm-transfers &lt;1|0> - whether to confirm unsplit txes; print-ring-members &lt;1|0> - whether to print detailed information about ring members during confirmation; store-tx-info &lt;1|0> - whether to store outgoing tx info (destination address, payment ID, tx secret key) for future reference; default-mixin &lt;n> - set default mixin (default is 4); auto-refresh &lt;1|0> - whether to automatically sync new blocks from the daemon; refresh-type &lt;full|optimize-coinbase|no-coinbase|default> - set wallet refresh behaviour; priority [0|1|2|3|4] - default/unimportant/normal/elevated/priority fee; confirm-missing-payment-id &lt;1|0>; ask-password &lt;1|0>; unit &lt;monero|millinero|micronero|nanonero|piconero> - set default monero (sub-)unit; min-outputs-count [n] - try to keep at least that many outputs of value at least min-outputs-value; min-outputs-value [n] - try to keep at least min-outputs-count outputs of at least that value; merge-destinations &lt;1|0> - whether to merge multiple payments to the same destination address</source>
        <translation>Tillgängliga alternativ: seed language - ange språk för plånbokens frö; always-confirm-transfers &lt;1|0> - huruvida ej delade transaktioner ska bekräftas; print-ring-members &lt;1|0> - huruvida detaljerad information om ringmedlemmar ska skrivas ut vid bekräftelse; store-tx-info &lt;1|0> - huruvida info om utgående transaktioner ska sparas (måladress, betalnings-ID, hemlig transaktionsnyckel) för framtida referens; default-mixin &lt;n> - ange standardvärde för mixin (standard är 4); auto-refresh &lt;1|0> - huruvida nya block från daemonen ska synkas automatiskt; refresh-type &lt;full|optimize-coinbase|no-coinbase|default> - ange uppdateringsbeteende för plånbok; priority [0|1|2|3|4] - standard/oviktigt/normal/förhöjd/prioritetsavgift; confirm-missing-payment-id &lt;1|0>; ask-password &lt;1|0>; unit &lt;monero|millinero|micronero|nanonero|piconero> - ange standardvärde för monero-(under-)enhet; min-outputs-count [n] - försök behålla åtminstone så många utgångar med ett värde på åtminstone min-outputs-value; min-outputs-value [n] - försök behålla åtminstone min-outputs-count utgångar av åtminstone detta värde; merge-destinations &lt;1|0> - huruvida flera betalningar till samma måladress ska slås samman</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="724"/>
        <source>Rescan blockchain for spent outputs</source>
        <translation>Genomsök blockkedjan igen för spenderade utgångar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="725"/>
        <source>Get transaction key (r) for a given &lt;txid></source>
        <translation>Hämta transaktionsnyckel (r) för ett givet &lt;transaktions-ID></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="726"/>
        <source>Check amount going to &lt;address> in &lt;txid></source>
        <translation>Kontrollera belopp som går till &lt;adress> i &lt;transaktions-ID></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="727"/>
        <source>Generate a signature to prove payment to &lt;address> in &lt;txid> using the transaction secret key (r) without revealing it</source>
        <translation>Skapa en signatur för att bevisa betalning till &lt;adress> i &lt;transaktions-ID> genom att använda hemlig nyckel för transaktion (r) utan att avslöja den</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="728"/>
        <source>Check tx proof for payment going to &lt;address> in &lt;txid></source>
        <translation>Kontrollera transaktionsbevis för betalning som går till &lt;adress> i &lt;transaktions-ID></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="730"/>
        <source>unspent_outputs [&lt;min_amount> &lt;max_amount>] - Show unspent outputs within an optional amount range</source>
        <translation>unspent_outputs [&lt;min_belopp> &lt;max_belopp>] - Visa ej spenderade utgångar inom ett valfritt beloppsintervall</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="731"/>
        <source>Rescan blockchain from scratch</source>
        <translation>Genomsök blockkedjan från början</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="732"/>
        <source>Set an arbitrary string note for a txid</source>
        <translation>Ange en godtycklig sträng som anteckning för ett transaktions-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="733"/>
        <source>Get a string note for a txid</source>
        <translation>Hämta en stränganteckning för ett transaktions-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="734"/>
        <source>Show wallet status information</source>
        <translation>Visa statusinformation för plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="735"/>
        <source>Sign the contents of a file</source>
        <translation>Signera innehållet i en fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="736"/>
        <source>Verify a signature on the contents of a file</source>
        <translation>Verifera signaturen för innehållet i en fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="737"/>
        <source>Export a signed set of key images</source>
        <translation>Exportera en signerad uppsättning nyckelavbildningar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="738"/>
        <source>Import signed key images list and verify their spent status</source>
        <translation>Importera lista med signerade nyckelavbildningar och verifera deras spenderingsstatus</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="739"/>
        <source>Export a set of outputs owned by this wallet</source>
        <translation>Exportera en uppsättning utgångar som ägs av denna plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="740"/>
        <source>Import set of outputs owned by this wallet</source>
        <translation>Importera en uppsättning utgångar som ägs av denna plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="802"/>
        <source>full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)</source>
        <translation>full (långsammast, inga antaganden); optimize-coinbase (snabb, antar att hela coinbase-transaktionen betalas till en enda adress); no-coinbase (snabbast, antar att ingen coinbase-transaktion tas emot), default (samma som optimize-coinbase)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="806"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation>monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="851"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation>Plånbokens namn ej giltigt. Försök igen eller använd Ctrl-C för att avsluta.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="868"/>
        <source>Wallet and key files found, loading...</source>
        <translation>Plånbok och nyckelfil hittades, läser in …</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="874"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation>Nyckelfil hittades men inte plånboksfilen. Återskapar …</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="880"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation>Nyckelfilen kunde inte hittas. Det gick inte att öppna plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="894"/>
        <source>Generating new wallet...</source>
        <translation>Skapar ny plånbok …</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="937"/>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-keys=&quot;wallet_name&quot;</source>
        <translation>det går inte att ange mer än en av --generate-new-wallet=&quot;plånboksnamn&quot;, --wallet-file=&quot;plånboksnamn&quot;, --generate-from-view-key=&quot;plånboksnamn&quot;, --generate-from-json=&quot;json-filnamn&quot; och --generate-from-keys=&quot;plånboksnamn&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="953"/>
        <source>can&apos;t specify both --restore-deterministic-wallet and --non-deterministic</source>
        <translation>det går inte att ange både --restore-deterministic-wallet och --non-deterministic</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="982"/>
        <source>Electrum-style word list failed verification</source>
        <translation>det gick inte att verifiera ordlista av Electrum-typ</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="994"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1011"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1046"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1063"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1079"/>
        <source>No data supplied, cancelled</source>
        <translation>Ingen data angiven, avbryter</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1002"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1054"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2220"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2718"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3276"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3378"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3530"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4048"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4239"/>
        <source>failed to parse address</source>
        <translation>det gick inte att parsa adressen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1017"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1085"/>
        <source>failed to parse view key secret key</source>
        <translation>det gick inte att parsa hemlig visningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1027"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1103"/>
        <source>failed to verify view key secret key</source>
        <translation>det gick inte att verifiera hemlig visningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1031"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1107"/>
        <source>view key does not match standard address</source>
        <translation>visningsnyckel matchar inte standardadress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1036"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1111"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1128"/>
        <source>account creation failed</source>
        <translation>det gick inte att skapa konto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1069"/>
        <source>failed to parse spend key secret key</source>
        <translation>det gick inte att parsa spendernyckel hemlig nyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1095"/>
        <source>failed to verify spend key secret key</source>
        <translation>det gick inte att verifiera spendernyckel hemlig nyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1099"/>
        <source>spend key does not match standard address</source>
        <translation>spendernyckel matchar inte standardadress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1199"/>
        <source>failed to open account</source>
        <translation>det gick inte att öppna konto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1203"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1579"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1626"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1647"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3348"/>
        <source>wallet is null</source>
        <translation>plånbok är null</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1262"/>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or restart the wallet with the correct daemon address.</source>
        <translation>Antingen har daemonen inte startat eller så angavs fel port. Se till att daemonen kör eller starta om plånboken med korrekt daemonadress.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1306"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1311"/>
        <source>invalid language choice passed. Please try again.
</source>
        <translation>ogiltigt språkval angavs. Försök igen.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1370"/>
        <source>View key: </source>
        <translation>Visningsnyckel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1385"/>
        <source>Your wallet has been generated!
To start synchronizing with the daemon, use the &quot;refresh&quot; command.
Use the &quot;help&quot; command to see the list of available commands.
Always use the &quot;exit&quot; command when closing monero-wallet-cli to save 
your current session&apos;s state. Otherwise, you might need to synchronize 
your wallet again (your wallet keys are NOT at risk in any case).
</source>
        <translation>Din plånbok har skapats!
För att starta synkronisering med daemonen, använd kommandot &quot;refresh&quot;.
Använd kommandot &quot;help&quot; för att se en lista över tillgängliga kommandon.
Använd alltid kommandot &quot;exit&quot; när du stänger monero-wallet-cli så att ditt aktuella sessionstillstånd sparas. Annars kan du bli tvungen att synkronisera
din plånbok igen (din plånboks nycklar är dock INTE hotade i vilket fall som helst).
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1492"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation>Du kan också vilja ta bort filen &quot;%s&quot; och försöka igen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1518"/>
        <source>failed to deinitialize wallet</source>
        <translation>det gick inte att avinitiera plånboken</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1570"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1968"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation>detta kommando kräver en betrodd daemon. Aktivera med --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1657"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation>blockkedjan kan inte sparas: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1736"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1982"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2386"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2563"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2824"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>daemonen är upptagen. Försök igen senare.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1740"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1986"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2390"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2567"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2828"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>ingen anslutning till daemonen. Se till att daemonen körs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1750"/>
        <source>refresh error: </source>
        <translation>fel vid uppdatering: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1794"/>
        <source>Balance: </source>
        <translation>Saldo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1845"/>
        <source>pubkey</source>
        <translation>öppen nyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1845"/>
        <source>key image</source>
        <translation>nyckelavbildning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1856"/>
        <source>unlocked</source>
        <translation>upplåst</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <source>ringct</source>
        <translation>ringct</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1855"/>
        <source>T</source>
        <translation>S</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1855"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1856"/>
        <source>locked</source>
        <translation>låst</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1857"/>
        <source>RingCT</source>
        <translation>RingCT</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1857"/>
        <source>-</source>
        <translation>-</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1929"/>
        <source>payment ID has invalid format, expected 16 or 64 character hex string: </source>
        <translation>betalnings-ID har ogiltigt format, en 16- eller 64-teckens hex-sträng förväntades: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1990"/>
        <source>failed to get spent status</source>
        <translation>det gick inte att hämta spenderingsstatus</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2105"/>
        <source>the same transaction</source>
        <translation>samma transaktion</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2105"/>
        <source>blocks that are temporally very close</source>
        <translation>block som ligger väldigt nära varandra i tiden</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2206"/>
        <source>Locked blocks too high, max 1000000 (˜4 yrs)</source>
        <translation>Låsta block för högt, max 1000000 (˜~4 år)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3258"/>
        <source>usage: get_tx_proof &lt;txid> &lt;dest_address> [&lt;tx_key>]</source>
        <translation>användning: get_tx_proof &lt;txid> &lt;måladress> [&lt;tx_key>]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3289"/>
        <source>failed to parse tx_key</source>
        <translation>det gick inte att parsa tx_nyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3298"/>
        <source>Tx secret key was found for the given txid, but you&apos;ve also provided another tx secret key which doesn&apos;t match the found one.</source>
        <translation>Hemlig transaktionsnyckel hittades för det givna txid, men du har också angivit en annan hemlig transaktionsnyckel som inte matchar den hittade nyckeln.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3306"/>
        <source>Tx secret key wasn&apos;t found in the wallet file. Provide it as the optional third parameter if you have it elsewhere.</source>
        <translation>Den hemliga transaktionsnyckeln kunde inte hittas i plånboksfilen. Ange den som den valfria tredje parametern om du har den någon annanstans.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3330"/>
        <source>Signature: </source>
        <translation>Signatur: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3508"/>
        <source>usage: check_tx_proof &lt;txid> &lt;address> &lt;signature></source>
        <translation>användning: check_tx_proof &lt;txid> &lt;adress> &lt;signatur></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3539"/>
        <source>Signature header check error</source>
        <translation>Fel vid kontroll av signaturhuvud</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3550"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3555"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3560"/>
        <source>Signature decoding error</source>
        <translation>nFel vid avkodning av signatur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3602"/>
        <source>Tx pubkey was not found</source>
        <translation>Transaktionens öppna nyckel kunde inte hittas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3609"/>
        <source>Good signature</source>
        <translation>Bra signatur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3613"/>
        <source>Bad signature</source>
        <translation>Dålig signatur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3621"/>
        <source>failed to generate key derivation</source>
        <translation>det gick inte att skapa nyckelhärledning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3994"/>
        <source>usage: integrated_address [payment ID]</source>
        <translation>användning: integrated_address [betalnings-ID]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4017"/>
        <source>Integrated address: account %s, payment ID %s</source>
        <translation>Integrerad adress: konto %s, betalnings-ID %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4022"/>
        <source>Standard address: </source>
        <translation>Standardadress: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4027"/>
        <source>failed to parse payment ID or address</source>
        <translation>det gick inte att parsa betalnings-ID eller adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4038"/>
        <source>usage: address_book [(add (&lt;address> [pid &lt;long or short payment id>])|&lt;integrated address> [&lt;description possibly with whitespaces>])|(delete &lt;index>)]</source>
        <translation>användning: address_book [(add (&lt;adress> [pid &lt;långt eller kort betalnings-ID>])|&lt;Integrerad adress> [&lt;beskrivning eventuellt med blanktecken>])|(delete &lt;index>)]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4070"/>
        <source>failed to parse payment ID</source>
        <translation>det gick inte att parsa betalnings-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4088"/>
        <source>failed to parse index</source>
        <translation>det gick inte att parsa index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4096"/>
        <source>Address book is empty.</source>
        <translation>Adressboken är tom.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4102"/>
        <source>Index: </source>
        <translation>Index: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4103"/>
        <source>Address: </source>
        <translation>Adress: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4104"/>
        <source>Payment ID: </source>
        <translation>Betalnings-ID: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4105"/>
        <source>Description: </source>
        <translation>Beskrivning: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4115"/>
        <source>usage: set_tx_note [txid] free text note</source>
        <translation>användning: set_tx_note [txid] fri textanteckning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4143"/>
        <source>usage: get_tx_note [txid]</source>
        <translation>användning: get_tx_note [txid]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4193"/>
        <source>usage: sign &lt;filename></source>
        <translation>användning: sign &lt;filnamn></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4198"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation>plånboken är enbart för granskning och kan inte signera</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4207"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4230"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4374"/>
        <source>failed to read file </source>
        <translation>det gick inte att läsa filen </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4219"/>
        <source>usage: verify &lt;filename> &lt;address> &lt;signature></source>
        <translation>användning: verify &lt;filnamn> &lt;adress> &lt;signatur></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4246"/>
        <source>Bad signature from </source>
        <translation>Dålig signatur från </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4250"/>
        <source>Good signature from </source>
        <translation>Bra signatur från </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4259"/>
        <source>usage: export_key_images &lt;filename></source>
        <translation>användning: export_key_images &lt;filnamn></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4264"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation>plånboken är enbart för granskning och kan inte exportera nyckelavbildningar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4274"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4346"/>
        <source>failed to save file </source>
        <translation>det gick inte att spara fil </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4285"/>
        <source>Signed key images exported to </source>
        <translation>Signerade nyckelavbildningar exporterades till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4293"/>
        <source>usage: import_key_images &lt;filename></source>
        <translation>användning: import_key_images &lt;filnamn></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4323"/>
        <source>usage: export_outputs &lt;filename></source>
        <translation>användning: export_outputs &lt;filnamn></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4357"/>
        <source>Outputs exported to </source>
        <translation>Utgångar exporterades till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4365"/>
        <source>usage: import_outputs &lt;filename></source>
        <translation>användning: import_outputs &lt;filnamn></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2246"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3818"/>
        <source>amount is wrong: </source>
        <translation>beloppet är fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2247"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3819"/>
        <source>expected number from 0 to </source>
        <translation>förväntades: ett tal från 0 till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2378"/>
        <source>Money successfully sent, transaction </source>
        <translation>Pengar skickades, transaktion </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3141"/>
        <source>no connection to daemon. Please, make sure daemon is running.</source>
        <translation>ingen anslutning till daemonen. Se till att daemonen körs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2420"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2597"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2858"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3171"/>
        <source>not enough outputs for specified mixin_count</source>
        <translation>inte tillräckligt många utgångar för angiven mixin_count</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2423"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2600"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2861"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3174"/>
        <source>output amount</source>
        <translation>utgångens belopp</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2423"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2600"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2861"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3174"/>
        <source>found outputs to mix</source>
        <translation>hittade utgångar att mixa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2428"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2605"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2866"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3179"/>
        <source>transaction was not constructed</source>
        <translation>transaktionen konstruerades inte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2432"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2609"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2870"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3183"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>transaktionen %s avvisades av daemonen med status: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2443"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2620"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2881"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3191"/>
        <source>one of destinations is zero</source>
        <translation>ett av målen är noll</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3195"/>
        <source>Failed to find a suitable way to split transactions</source>
        <translation>Det gick inte att hitta ett passande sätt att dela upp transaktioner</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2452"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2629"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2890"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3200"/>
        <source>unknown transfer error: </source>
        <translation>okänt överföringsfel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2516"/>
        <source>Sweeping </source>
        <translation>Sveper upp </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2785"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No)</source>
        <translation>Sveper upp %s för en total avgift på %s.  Är detta okej?  (J/Ja/N/Nej)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2555"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2816"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3129"/>
        <source>Money successfully sent, transaction: </source>
        <translation>Pengar skickades, transaktion: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3022"/>
        <source>Change goes to more than one address</source>
        <translation>Växel går till mer än en adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3047"/>
        <source>%s change to %s</source>
        <translation>%s växel till %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3050"/>
        <source>no change</source>
        <translation>ingen växel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3105"/>
        <source>Transaction successfully signed to file </source>
        <translation>Transaktionen signerades till fil </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3226"/>
        <source>usage: get_tx_key &lt;txid></source>
        <translation>användning: get_tx_key &lt;transaktions-ID></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3234"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3266"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3354"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3519"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4122"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4150"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4450"/>
        <source>failed to parse txid</source>
        <translation>det gick inte att parsa transaktions-id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3245"/>
        <source>Tx key: </source>
        <translation>Tx-nyckel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3250"/>
        <source>no tx keys found for this txid</source>
        <translation>inga tx-nycklar kunde hittas för detta txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3339"/>
        <source>usage: check_tx_key &lt;txid> &lt;txkey> &lt;address></source>
        <translation>användning: check_tx_key &lt;txid> &lt;txnyckel> &lt;adress></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3361"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3368"/>
        <source>failed to parse tx key</source>
        <translation>det gick inte att parsa transaktionsnyckeln</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3400"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3573"/>
        <source>failed to get transaction from daemon</source>
        <translation>det gick inte att hämta transaktion från daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3411"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3584"/>
        <source>failed to parse transaction from daemon</source>
        <translation>det gick inte att parsa transaktion från daemonen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3418"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3591"/>
        <source>failed to validate transaction from daemon</source>
        <translation>det gick inte att validera transaktion från daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3423"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3596"/>
        <source>failed to get the right transaction from daemon</source>
        <translation>det gick inte att hämta rätt transaktion från daemonen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3385"/>
        <source>failed to generate key derivation from supplied parameters</source>
        <translation>det gick inte att skapa nyckelhärledning från angivna parametrar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3471"/>
        <source>error: </source>
        <translation>fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3477"/>
        <source>received</source>
        <translation>mottaget</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3477"/>
        <source>in txid</source>
        <translation>i transaktions-id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3481"/>
        <source>received nothing in txid</source>
        <translation>tog emot ingenting i transaktions-id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3485"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation>VARNING: denna transaktion är ännu inte inkluderad i blockkedjan!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3494"/>
        <source>This transaction has %u confirmations</source>
        <translation>Transaktionen har %u bekräftelser</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3498"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation>VARNING: det gick inte att avgöra antal bekräftelser!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3661"/>
        <source>usage: show_transfers [in|out|all|pending|failed] [&lt;min_height> [&lt;max_height>]]</source>
        <translation>användning: show_transfers [in|out|all|pending|failed] [&lt;min_höjd> [&lt;max_höjd>]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3700"/>
        <source>bad min_height parameter:</source>
        <translation>dålig parameter för min_height:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3712"/>
        <source>bad max_height parameter:</source>
        <translation>dålig parameter för max_height:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3760"/>
        <source>in</source>
        <translation>in</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3760"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3798"/>
        <source>out</source>
        <translation>ut</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3798"/>
        <source>failed</source>
        <translation>misslyckades</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3798"/>
        <source>pending</source>
        <translation>väntar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3809"/>
        <source>usage: unspent_outputs [&lt;min_amount> &lt;max_amount>]</source>
        <translation>användning: unspent_outputs [&lt;min_belopp> &lt;max_belopp>]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3824"/>
        <source>&lt;min_amount> should be smaller than &lt;max_amount></source>
        <translation>&lt;min_belopp> måste vara mindre än &lt;max_belopp></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3856"/>
        <source>
Amount: </source>
        <translation>
Belopp: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3856"/>
        <source>, number of keys: </source>
        <translation>, antal nycklar: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3861"/>
        <source> </source>
        <translation></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3866"/>
        <source>
Min block height: </source>
        <translation>
Minblockhöjd: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3867"/>
        <source>
Max block height: </source>
        <translation>
Maxblockhöjd: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3868"/>
        <source>
Min amount found: </source>
        <translation>
Minbelopp funnet: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3869"/>
        <source>
Max amount found: </source>
        <translation>
Maxbelopp funnet: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3870"/>
        <source>
Total count: </source>
        <translation>
Totalt antal: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3910"/>
        <source>
Bin size: </source>
        <translation>
Storlek för binge: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3911"/>
        <source>
Outputs per *: </source>
        <translation>
Utgångar per *: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3913"/>
        <source>count
  ^
</source>
        <translation>antal
  ^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3915"/>
        <source>  |</source>
        <translation>  |</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3917"/>
        <source>  +</source>
        <translation>  +</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3917"/>
        <source>+--> block height
</source>
        <translation>+--> blockhöjd
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3918"/>
        <source>   ^</source>
        <translation>   ^</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3918"/>
        <source>^
</source>
        <translation>^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3919"/>
        <source>  </source>
        <translation></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3969"/>
        <source>wallet</source>
        <translation>plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="420"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4000"/>
        <source>Random payment ID: </source>
        <translation>Slumpmässigt betalnings-ID: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4001"/>
        <source>Matching integrated address: </source>
        <translation>Matchande integrerad adress: </translation>
    </message>
</context>
<context>
    <name>sw</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="116"/>
        <source>Generate new wallet and save it to &lt;arg></source>
        <translation>Skapa ny plånbok och spara den till &lt;arg></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="117"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>Skapa granskningsplånbok från visningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="118"/>
        <source>Generate wallet from private keys</source>
        <translation>Skapa plånbok från privata nycklar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="120"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>Ange Electrum-frö för att återställa/skapa plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="121"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Återställ plånbok genom användning av minnesfrö (Electrum-typ)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="122"/>
        <source>Create non-deterministic view and spend keys</source>
        <translation>Skapa non-deterministic visnings- och spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="123"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation>Aktivera kommandon som kräver en betrodd daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="124"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>Tillåt kommunikation med en daemon som använder en annan version av RPC</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="125"/>
        <source>Restore from specific blockchain height</source>
        <translation>Återställ från angiven blockkedjehöjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="136"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>daemonen är upptagen. Försök igen senare.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="145"/>
        <source>possibly lost connection to daemon</source>
        <translation>anslutning till daemonen kan ha tappats</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="226"/>
        <source>Error: </source>
        <translation>Fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4614"/>
        <source>Failed to initialize wallet</source>
        <translation>Det gick inte att initiera plånbok</translation>
    </message>
</context>
<context>
    <name>tools::dns_utils</name>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="430"/>
        <source>DNSSEC validation passed</source>
        <translation>DNSSEC-validering godkänd</translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="434"/>
        <source>WARNING: DNSSEC validation was unsuccessful, this address may not be correct!</source>
        <translation>VARNING: DNSSEC-verifiering misslyckades, denna adress kanske inte är korrekt!</translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="437"/>
        <source>For URL: </source>
        <translation>För URL: </translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="439"/>
        <source> Monero Address = </source>
        <translation> Monero-adress = </translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="441"/>
        <source>Is this OK? (Y/n) </source>
        <translation>är det OK? (J/n) </translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="451"/>
        <source>you have cancelled the transfer request</source>
        <translation>du har avbrutit överföringsbegäran</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="106"/>
        <source>Use daemon instance at &lt;host>:&lt;port></source>
        <translation>Använd daemoninstans på &lt;värddator>:&lt;port></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="107"/>
        <source>Use daemon instance at host &lt;arg> instead of localhost</source>
        <translation>Använd daemon-instansen på värddator &lt;arg> istället för localhost</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="460"/>
        <source>Wallet password</source>
        <translation>Lösenord för plånboken</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="109"/>
        <source>Wallet password file</source>
        <translation>Lösenordsfil för plånboken</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="110"/>
        <source>Use daemon instance at port &lt;arg> instead of 18081</source>
        <translation>Använd daemon-instansen på port &lt;arg> istället för 18081</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="112"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>För testnet. Daemonen måste också startas med flaggan --testnet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="113"/>
        <source>Restricts to view-only commands</source>
        <translation>Begränsar till granskningskommandon</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="152"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>det går inte ange värd eller port för daemonen mer än en gång</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="188"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>det går inte att ange mer än en av --password och --password-file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="204"/>
        <source>the password file specified could not be read</source>
        <translation>det gick inte att läsa angiven lösenordsfil</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="460"/>
        <source>Enter new wallet password</source>
        <translation>Ange nytt lösenord för plånboken</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="464"/>
        <source>failed to read wallet password</source>
        <translation>det gick inte att läsa lösenord för plånboken</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="227"/>
        <source>Failed to load file </source>
        <translation>Det gick inte att läsa in fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="108"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation>Lösenord för plånboken (använd escape-sekvenser eller citattecken efter behov)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="111"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>Ange användarnamn[:lösenord] för RPC-klient till daemonen</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="233"/>
        <source>Failed to parse JSON</source>
        <translation>Det gick inte att parsa JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="240"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation>Version %u är för ny, vi förstår bara upp till %u</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="258"/>
        <source>failed to parse view key secret key</source>
        <translation>det gick inte att parsa hemlig visningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="264"/>
        <location filename="../src/wallet/wallet2.cpp" line="331"/>
        <location filename="../src/wallet/wallet2.cpp" line="373"/>
        <source>failed to verify view key secret key</source>
        <translation>det gick inte att verifiera hemlig visningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="276"/>
        <source>failed to parse spend key secret key</source>
        <translation>det gick inte att parsa spendernyckel hemlig nyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="282"/>
        <location filename="../src/wallet/wallet2.cpp" line="343"/>
        <location filename="../src/wallet/wallet2.cpp" line="394"/>
        <source>failed to verify spend key secret key</source>
        <translation>det gick inte att verifiera spendernyckel hemlig nyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="295"/>
        <source>Electrum-style word list failed verification</source>
        <translation>det gick inte att verifiera ordlista av Electrum-typ</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="306"/>
        <source>At least one of Electrum-style word list and private view key must be specified</source>
        <translation>Åtminstone en av ordlista av Electrum-typ och privat visningsnyckel måste anges</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="311"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Både ordlista av Electrum-typ och privat nyckel har angivits</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="324"/>
        <source>invalid address</source>
        <translation>ogiltig adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="335"/>
        <source>view key does not match standard address</source>
        <translation>visningsnyckel matchar inte standardadress</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="347"/>
        <source>spend key does not match standard address</source>
        <translation>spendernyckel matchar inte standardadress</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="356"/>
        <source>Cannot create deprecated wallets from JSON</source>
        <translation>Det går inte att skapa inaktuella plånböcker från JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="403"/>
        <source>failed to generate new wallet: </source>
        <translation>det gick inte att skapa ny plånbok: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="5205"/>
        <source>failed to read file </source>
        <translation>det gick inte att läsa filen </translation>
    </message>
</context>
<context>
    <name>tools::wallet_rpc_server</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="151"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Daemonen är lokal, utgår från att den är betrodd</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="171"/>
        <source>Cannot specify --</source>
        <translation>Det går inte att ange --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="171"/>
        <source> and --</source>
        <translation> och --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="198"/>
        <source>Failed to create file </source>
        <translation>Det gick inte att skapa fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="198"/>
        <source>. Check permissions or remove file</source>
        <translation>. Kontrollera behörigheter eller ta bort filen</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="209"/>
        <source>Error writing to file </source>
        <translation>Fel vid skrivning till fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="212"/>
        <source>RPC username/password is stored in file </source>
        <translation>Användarnamn/lösenord för RPC har sparats i fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1748"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>Det går inte att ange mer än en av --wallet-file och --generate-from-json</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1760"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>Måste ange --wallet-file eller --generate-from-json eller --wallet-dir</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1764"/>
        <source>Loading wallet...</source>
        <translation>Läser in plånbok …</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1789"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1814"/>
        <source>Storing wallet...</source>
        <translation>Sparar plånbok …</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1791"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1816"/>
        <source>Stored ok</source>
        <translation>Sparad ok</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1794"/>
        <source>Loaded ok</source>
        <translation>Inläst ok</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1798"/>
        <source>Wallet initialization failed: </source>
        <translation>Det gick inte att initiera plånbok: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1805"/>
        <source>Failed to initialize wallet rpc server</source>
        <translation>Det gick inte att initiera RPC-servern för plånbok</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1809"/>
        <source>Starting wallet rpc server</source>
        <translation>Startar RPC-servern för plånbok</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1811"/>
        <source>Stopped wallet rpc server</source>
        <translation>Stoppade RPC-servern för plånbok</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1820"/>
        <source>Failed to store wallet: </source>
        <translation>Det gick inte att spara plånbok: </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1715"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4580"/>
        <source>Wallet options</source>
        <translation>Alternativ för plånbok</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="59"/>
        <source>Generate wallet from JSON format file</source>
        <translation>Skapa plånbok från fil i JSON-format</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="63"/>
        <source>Use wallet &lt;arg></source>
        <translation>Använd plånbok &lt;arg></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="87"/>
        <source>Max number of threads to use for a parallel job</source>
        <translation>Max antal trådar att använda för ett parallellt jobb</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="88"/>
        <source>Specify log file</source>
        <translation>Ange loggfil</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="89"/>
        <source>Config file</source>
        <translation>Konfigurationsfil</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="98"/>
        <source>General options</source>
        <translation>Allmänna alternativ</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="128"/>
        <source>Can&apos;t find config file </source>
        <translation>Det gick inte att hitta konfigurationsfilen </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="172"/>
        <source>Logging to: </source>
        <translation>Loggar till: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="173"/>
        <source>Logging to %s</source>
        <translation>Loggar till %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="153"/>
        <source>Usage:</source>
        <translation>Användning:</translation>
    </message>
</context>
</TS>
