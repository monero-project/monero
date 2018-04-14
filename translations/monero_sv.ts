<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1">
<context>
    <name>Monero::AddressBookImpl</name>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="53"/>
        <source>Invalid destination address</source>
        <translation>Ogiltig måladress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="63"/>
        <source>Invalid payment ID. Short payment ID should only be used in an integrated address</source>
        <translation>Ogiltigt betalnings-ID. Kort betalnings-ID ska endast användas i en integrerad adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="70"/>
        <source>Invalid payment ID</source>
        <translation>Ogiltigt betalnings-ID</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="77"/>
        <source>Integrated address and long payment ID can&apos;t be used at the same time</source>
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
        <location filename="../src/wallet/api/pending_transaction.cpp" line="115"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>daemonen är upptagen. Försök igen senare.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="118"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>ingen anslutning till daemonen. Se till att daemonen körs.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="122"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>transaktionen %s avvisades av daemonen med status: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="127"/>
        <source>. Reason: </source>
        <translation>. Orsak: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="129"/>
        <source>Unknown exception: </source>
        <translation>Okänt undantag: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="132"/>
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
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="168"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>Begärd växel går inte till en betald adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="174"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>Begärd växel är större än betalning till växeladressen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="184"/>
        <source>Change goes to more than one address</source>
        <translation>Växel går till fler än en adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="197"/>
        <source>sending %s to %s</source>
        <translation>skickar %s till %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="203"/>
        <source>with no destinations</source>
        <translation>utan några mål</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="209"/>
        <source>%s change to %s</source>
        <translation>%s växel till %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="212"/>
        <source>no change</source>
        <translation>ingen växel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="214"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu. %s</source>
        <translation>Läste in %lu transaktioner, för %s, avgift %s, %s, %s, med minsta ringstorlek %lu. %s</translation>
    </message>
</context>
<context>
    <name>Monero::WalletImpl</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1147"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>betalnings-ID har ogiltigt format. En hexadecimal sträng med 16 eller 64 tecken förväntades: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1157"/>
        <source>Failed to add short payment id: </source>
        <translation>Det gick inte att lägga till kort betalnings-ID: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1190"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1294"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>daemonen är upptagen. Försök igen senare.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1193"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1297"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>ingen anslutning till daemonen. Se till att daemonen körs.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1196"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1300"/>
        <source>RPC error: </source>
        <translation>RPC-fel: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1233"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1337"/>
        <source>not enough outputs for specified ring size</source>
        <translation>inte tillräckligt med utgångar för angiven ringstorlek</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1235"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1339"/>
        <source>found outputs to use</source>
        <translation>hittade utgångar att använda</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1237"/>
        <source>Please sweep unmixable outputs.</source>
        <translation>Svep upp omixbara utgångar.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1303"/>
        <source>failed to get random outputs to mix</source>
        <translation>det gick inte att hämta slumpmässiga utgångar att mixa</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1206"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1310"/>
        <source>not enough money to transfer, available only %s, sent amount %s</source>
        <translation>inte tillräckligt med pengar för överföring, endast tillgängligt %s, skickat belopp %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="506"/>
        <source>failed to parse address</source>
        <translation>det gick inte att parsa adressen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="518"/>
        <source>failed to parse secret spend key</source>
        <translation>det gick inte att parsa hemlig spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="528"/>
        <source>No view key supplied, cancelled</source>
        <translation>Ingen granskningsnyckel angiven, avbruten</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="535"/>
        <source>failed to parse secret view key</source>
        <translation>det gick inte att parsa hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="545"/>
        <source>failed to verify secret spend key</source>
        <translation>det gick inte att verifiera hemlig spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="550"/>
        <source>spend key does not match address</source>
        <translation>spendernyckel matchar inte adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="556"/>
        <source>failed to verify secret view key</source>
        <translation>det gick inte att verifiera hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="561"/>
        <source>view key does not match address</source>
        <translation>granskningsnyckel matchar inte adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="580"/>
        <source>failed to generate new wallet: </source>
        <translation>det gick inte att skapa ny plånbok: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="809"/>
        <source>Failed to send import wallet request</source>
        <translation>Det gick inte att skicka begäran om att importera plånbok</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="955"/>
        <source>Failed to load unsigned transactions</source>
        <translation>Det gick inte att läsa in osignerade transaktioner</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="976"/>
        <source>Failed to load transaction from file</source>
        <translation>Det gick inte att läsa in transaktion från fil</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="994"/>
        <source>Wallet is view only</source>
        <translation>Plånboken är endast för granskning</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1003"/>
        <source>failed to save file </source>
        <translation>det gick inte att spara fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1022"/>
        <source>Key images can only be imported with a trusted daemon</source>
        <translation>Nyckelavbildningar kan bara importeras med en betrodd daemon</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1035"/>
        <source>Failed to import key images: </source>
        <translation>Det gick inte att importera nyckelavbildningar: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1068"/>
        <source>Failed to get subaddress label: </source>
        <translation>Det gick inte att hämta etikett för underadress: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1082"/>
        <source>Failed to set subaddress label: </source>
        <translation>Det gick inte att ange etikett för underadress: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1199"/>
        <source>failed to get random outputs to mix: %s</source>
        <translation>det gick inte att hitta slumpmässiga utgångar att mixa: %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1215"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1319"/>
        <source>not enough money to transfer, overall balance only %s, sent amount %s</source>
        <translation>inte tillräckligt med pengar för överföring, totalt saldo är bara %s, skickat belopp %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1224"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1328"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>ej tillräckligt med pengar för överföring, endast tillgängligt %s, transaktionsbelopp %s = %s + %s (avgift)</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1235"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1339"/>
        <source>output amount</source>
        <translation>utgångens belopp</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1241"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1344"/>
        <source>transaction was not constructed</source>
        <translation>transaktionen konstruerades inte</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1245"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1348"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>transaktionen %s avvisades av daemonen med status: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1252"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1355"/>
        <source>one of destinations is zero</source>
        <translation>ett av målen är noll</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1255"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1358"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>det gick inte att hitta ett lämpligt sätt att dela upp transaktioner</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1258"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1361"/>
        <source>unknown transfer error: </source>
        <translation>okänt överföringsfel: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1261"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1364"/>
        <source>internal error: </source>
        <translation>internt fel: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1264"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1367"/>
        <source>unexpected error: </source>
        <translation>oväntat fel: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1267"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1370"/>
        <source>unknown error</source>
        <translation>okänt fel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1448"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1477"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1530"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1561"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1592"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1615"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1980"/>
        <source>Failed to parse txid</source>
        <translation>Det gick inte att parsa txid</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1466"/>
        <source>no tx keys found for this txid</source>
        <translation>inga tx-nycklar kunde hittas för detta txid</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1486"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1496"/>
        <source>Failed to parse tx key</source>
        <translation>Det gick inte att parsa txnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1506"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1538"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1569"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1657"/>
        <source>Failed to parse address</source>
        <translation>Det gick inte att parsa adressen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1663"/>
        <source>Address must not be a subaddress</source>
        <translation>Adressen får inte vara en underadress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1886"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>Genomsök efter spenderade kan endast användas med en betrodd daemon</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1921"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1942"/>
        <source>Failed to parse output public key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1930"/>
        <source>Failed to set blackballed outputs</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1949"/>
        <source>Failed to unblackball output</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1961"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2004"/>
        <source>Failed to parse key image</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1968"/>
        <source>Failed to get ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1988"/>
        <source>Failed to get rings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2011"/>
        <source>Failed to set ring</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Wallet</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="261"/>
        <source>Failed to parse address</source>
        <translation>Det gick inte att parsa adressen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="268"/>
        <source>Failed to parse key</source>
        <translation>Det gick inte att parsa nyckeln</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="276"/>
        <source>failed to verify key</source>
        <translation>det gick inte att verifiera nyckeln</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="286"/>
        <source>key does not match address</source>
        <translation>nyckeln matchar inte adressen</translation>
    </message>
</context>
<context>
    <name>command_line</name>
    <message>
        <location filename="../src/common/command_line.cpp" line="57"/>
        <source>yes</source>
        <translation>ja</translation>
    </message>
    <message>
        <location filename="../src/common/command_line.cpp" line="71"/>
        <source>no</source>
        <translation>nej</translation>
    </message>
</context>
<context>
    <name>cryptonote::rpc_args</name>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="40"/>
        <source>Specify IP to bind RPC server</source>
        <translation>Ange IP-adress för att binda till RPC-server</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="41"/>
        <source>Specify username[:password] required for RPC server</source>
        <translation>Ange användarnamn[:lösenord] som krävs av RPC-servern</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="42"/>
        <source>Confirm rpc-bind-ip value is NOT a loopback (local) IP</source>
        <translation>Bekräftelsevärde för rpc-bind-ip är INTE en lokal IP-adress (loopback)</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="43"/>
        <source>Specify a comma separated list of origins to allow cross origin resource sharing</source>
        <translation>Ange en kommaseparerad lista av ursprung för att tillåta resursdelning för korsande ursprung (Cross-origin resource sharing)</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="70"/>
        <source>Invalid IP address given for --</source>
        <translation>Ogiltig IP-adress angiven för --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="78"/>
        <source> permits inbound unencrypted external connections. Consider SSH tunnel or SSL proxy instead. Override with --</source>
        <translation> tillåter inkommande okrypterade externa anslutningar. Överväg att använda SSH-tunnel eller SSL-proxy istället. Åsidosätt med --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="95"/>
        <source>Username specified with --</source>
        <translation>Användarnamn angivet med --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="95"/>
        <location filename="../src/rpc/rpc_args.cpp" line="105"/>
        <source> cannot be empty</source>
        <translation> får inte vara tomt</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="105"/>
        <source> requires RFC server password --</source>
        <translation> kräver lösenord till RPC-server --</translation>
    </message>
</context>
<context>
    <name>cryptonote::simple_wallet</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="544"/>
        <source>Commands: </source>
        <translation>Kommandon: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3620"/>
        <source>failed to read wallet password</source>
        <translation>det gick inte att läsa lösenord för plånboken</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3245"/>
        <source>invalid password</source>
        <translation>ogiltigt lösenord</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2398"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation>set seed: kräver ett argument. tillgängliga alternativ: språk</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2430"/>
        <source>set: unrecognized argument(s)</source>
        <translation>set: okända argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3478"/>
        <source>wallet file path not valid: </source>
        <translation>ogiltig sökväg till plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2484"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation>Försöker skapa eller återställa plånbok, men angivna filer finns redan.  Avslutar för att inte riskera att skriva över någonting.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="744"/>
        <source>usage: payment_id</source>
        <translation>användning: payment_id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2384"/>
        <source>needs an argument</source>
        <translation>kräver ett argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2407"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2408"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2409"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2411"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2414"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2415"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2419"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2420"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2422"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2424"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2425"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2426"/>
        <source>0 or 1</source>
        <translation>0 eller 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2413"/>
        <source>0, 1, 2, 3, or 4</source>
        <translation>0, 1, 2, 3 eller 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2417"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2421"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2428"/>
        <source>unsigned integer</source>
        <translation>positivt heltal</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2538"/>
        <source>NOTE: the following 25 words can be used to recover access to your wallet. Write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation>OBS: följande 25 ord kan användas för att återställa åtkomst till din plånbok. Skriv ner och spara dem på ett säkert ställe. Spara dem inte i din e-post eller på något lagringsutrymme som du inte har direkt kontroll över.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2601"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-deterministic-wallet använder --generate-new-wallet, inte --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2630"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation>ange en återställningsparameter med --electrum-seed=&quot;ordlista här&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3000"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation>ange sökväg till en plånbok med --generate-new-wallet (inte --wallet-file)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3180"/>
        <source>wallet failed to connect to daemon: </source>
        <translation>plånboken kunde inte ansluta till daemonen: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3188"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation>Daemonen använder en högre version av RPC (%u) än plånboken (%u): %s. Antingen uppdatera en av dem, eller använd --allow-mismatched-daemon-version.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3207"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation>Lista över tillgängliga språk för din plånboks startvärde:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3217"/>
        <source>Enter the number corresponding to the language of your choice: </source>
        <translation>Ange det tal som motsvarar det språk du vill använda: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3290"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation>Du hade använt en inaktuell version av plånboken. Använd det nya startvärde som tillhandahålls.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3306"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3374"/>
        <source>Generated new wallet: </source>
        <translation>Ny plånbok skapad: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3312"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3379"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3415"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3467"/>
        <source>failed to generate new wallet: </source>
        <translation>det gick inte att skapa ny plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3496"/>
        <source>Opened watch-only wallet</source>
        <translation>Öppnade plånbok för granskning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3500"/>
        <source>Opened wallet</source>
        <translation>Öppnade plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3513"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation>Du hade använt en inaktuell version av plånboken. Fortsätt för att uppgradera din plånbok.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3528"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation>Du hade använt en inaktuell version av plånboken. Plånbokens filformat kommer nu att uppgraderas.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3536"/>
        <source>failed to load wallet: </source>
        <translation>det gick inte att läsa in plånboken: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3553"/>
        <source>Use the &quot;help&quot; command to see the list of available commands.
</source>
        <translation>Använd kommandot &quot;help&quot; för att visa en lista över tillgängliga kommandon.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3598"/>
        <source>Wallet data saved</source>
        <translation>Plånboksdata sparades</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3694"/>
        <source>Mining started in daemon</source>
        <translation>Brytning startad i daemonen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3696"/>
        <source>mining has NOT been started: </source>
        <translation>brytning har INTE startats: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3716"/>
        <source>Mining stopped in daemon</source>
        <translation>Brytning stoppad i daemonen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3718"/>
        <source>mining has NOT been stopped: </source>
        <translation>brytning har INTE stoppats: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3773"/>
        <source>Blockchain saved</source>
        <translation>Blockkedjan sparades</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3788"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3806"/>
        <source>Height </source>
        <translation>Höjd </translation>
    </message>
    <message>
        <source>transaction </source>
        <translation type="vanished">transaktion </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3808"/>
        <source>spent </source>
        <translation>spenderat </translation>
    </message>
    <message>
        <source>unsupported transaction format</source>
        <translation type="vanished">transaktionsformatet stöds inte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3834"/>
        <source>Starting refresh...</source>
        <translation>Startar uppdatering …</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3847"/>
        <source>Refresh done, blocks received: </source>
        <translation>Uppdatering färdig, mottagna block: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4384"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4873"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>betalnings-ID har ogiltigt format. En hexadecimal sträng med 16 eller 64 tecken förväntades: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4399"/>
        <source>bad locked_blocks parameter:</source>
        <translation>felaktig parameter för locked_blocks:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4427"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4891"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5105"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation>en enda transaktion kan inte använda fler än ett betalnings-ID: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4436"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4900"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5073"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5113"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>det gick inte att upprätta betalnings-ID, trots att det avkodades korrekt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4461"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4542"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4630"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4739"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4914"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4972"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5127"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5170"/>
        <source>transaction cancelled.</source>
        <translation>transaktion avbruten.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4521"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4531"/>
        <source>Is this okay anyway?  (Y/Yes/N/No): </source>
        <translation>Är detta okej ändå?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4526"/>
        <source>There is currently a %u block backlog at that fee level. Is this okay?  (Y/Yes/N/No): </source>
        <translation>Det finns för närvarande en %u blocks eftersläpning på den avgiftsnivån. Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4531"/>
        <source>Failed to check for backlog: </source>
        <translation>Det gick inte att kontrollera eftersläpning: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4572"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4945"/>
        <source>
Transaction </source>
        <translation>
Transaktion </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4577"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4950"/>
        <source>Spending from address index %d
</source>
        <translation>Spendera från adressindex %d
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4579"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4952"/>
        <source>WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.
</source>
        <translation>VARNING: Utgångar från flera adresser används tillsammans, vilket möjligen kan kompromettera din sekretess.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4581"/>
        <source>Sending %s.  </source>
        <translation>Skickar %s.  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4584"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation>Transaktionen behöver delas upp i %llu transaktioner.  Detta gör att en transaktionsavgift läggs till varje transaktion, med ett totalbelopp på %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4590"/>
        <source>The transaction fee is %s</source>
        <translation>Transaktionsavgiften är %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4593"/>
        <source>, of which %s is dust from change</source>
        <translation>, varav %s är damm från växel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4594"/>
        <source>.</source>
        <translation>.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4594"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation>Ett totalt belopp på %s från växeldamm skickas till damm-adressen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4599"/>
        <source>.
This transaction will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation>.
Denna transaktion låses upp vid block %llu, om ungefär %s dagar (förutsatt en blocktid på 2 minuter)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4642"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4654"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4750"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4762"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4983"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4995"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5180"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5192"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Det gick inte att skriva transaktioner till fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4646"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4658"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4754"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4766"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4987"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4999"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5184"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5196"/>
        <source>Unsigned transaction(s) successfully written to file: </source>
        <translation>Osignerade transaktioner skrevs till fil: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4709"/>
        <source>No unmixable outputs found</source>
        <translation>Inga omixbara utgångar kunde hittas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4792"/>
        <source>No address given</source>
        <translation>Ingen adress har angivits</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5067"/>
        <source>failed to parse Payment ID</source>
        <translation>det gick inte att parsa betalnings-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5083"/>
        <source>usage: sweep_single [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;key_image&gt; &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation>användning: sweep_single [&lt;prioritet&gt;] [&lt;ringstorlek&gt;] &lt;nyckelavbildning&gt; &lt;adress&gt; [&lt;betalnings_id&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5090"/>
        <source>failed to parse key image</source>
        <translation>det gick inte att parsa nyckelavbildning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5142"/>
        <source>No outputs found</source>
        <translation>Inga utgångar kunde hittas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5147"/>
        <source>Multiple transactions are created, which is not supposed to happen</source>
        <translation>Flera transaktioner skapas, vilket inte ska kunna inträffa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5152"/>
        <source>The transaction uses multiple or no inputs, which is not supposed to happen</source>
        <translation>Transaktionen använder flera eller inga ingångar, vilket inte ska kunna inträffa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5229"/>
        <source>missing threshold amount</source>
        <translation>tröskelbelopp saknas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5234"/>
        <source>invalid amount threshold</source>
        <translation>ogiltigt tröskelbelopp</translation>
    </message>
    <message>
        <source>donations are not enabled on the testnet</source>
        <translation type="vanished">donationer är inte aktiverade på testnet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5251"/>
        <source>usage: donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation>användning: donate [index=&lt;N1&gt;[, &lt;N2&gt;, …]] [&lt;prioritet&gt;] [&lt;ringstorlek&gt;] &lt;belopp&gt; [&lt;betalnings_id&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5345"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>Begärd växel går inte till en betald adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5350"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>Begärd växel är större än betalning till växeladressen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5381"/>
        <source>sending %s to %s</source>
        <translation>skickar %s till %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5391"/>
        <source> dummy output(s)</source>
        <translation> dummy-utgångar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5394"/>
        <source>with no destinations</source>
        <translation>utan några mål</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5406"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu, %s. %sIs this okay? (Y/Yes/N/No): </source>
        <translation>Läste in %lu transaktioner, för %s, avgift %s, %s, %s, med minsta ringstorlek %lu, %s. %sÄr detta okej? (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5435"/>
        <source>This is a multisig wallet, it can only sign with sign_multisig</source>
        <translation>Detta är en multisig-plånbok, som endast kan signera med sign_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5445"/>
        <source>usage: sign_transfer [export]</source>
        <translation>användning: sign_transfer [export]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5457"/>
        <source>Failed to sign transaction</source>
        <translation>Det gick inte att signera transaktionen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5463"/>
        <source>Failed to sign transaction: </source>
        <translation>Det gick inte att signera transaktionen: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5484"/>
        <source>Transaction raw hex data exported to </source>
        <translation>Hexadecimala rådata för transaktionen exporterades till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5505"/>
        <source>Failed to load transaction from file</source>
        <translation>Det gick inte att läsa in transaktion från fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3863"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4166"/>
        <source>RPC error: </source>
        <translation>RPC-fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="591"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation>plånboken är enbart för granskning och har ingen spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="716"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="872"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="945"/>
        <source>Your original password was incorrect.</source>
        <translation>Ditt ursprungliga lösenord var fel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="732"/>
        <source>Error with wallet rewrite: </source>
        <translation>Ett fel uppstod vid återskrivning av plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1683"/>
        <source>priority must be 0, 1, 2, 3, or 4 </source>
        <translation>prioritet måste vara 0, 1, 2, 3 eller 4 </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1695"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1710"/>
        <source>priority must be 0, 1, 2, 3, or 4</source>
        <translation>prioritet måste vara 0, 1, 2, 3 eller 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1798"/>
        <source>invalid unit</source>
        <translation>ogiltig enhet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1816"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1878"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation>ogiltigt värde för count: måste vara ett heltal utan tecken</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1834"/>
        <source>invalid value</source>
        <translation>ogiltigt värde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2439"/>
        <source>usage: set_log &lt;log_level_number_0-4&gt; | &lt;categories&gt;</source>
        <translation>användning: set_log &lt;loggnivå_nummer_0-4&gt; | &lt;kategorier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2510"/>
        <source>(Y/Yes/N/No): </source>
        <translation>(J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3043"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3070"/>
        <source>bad m_restore_height parameter: </source>
        <translation>felaktig parameter för m_restore_height: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3048"/>
        <source>date format must be YYYY-MM-DD</source>
        <translation>datumformat måste vara ÅÅÅÅ-MM-DD</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3061"/>
        <source>Restore height is: </source>
        <translation>Återställningshöjd är: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2988"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3062"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4623"/>
        <source>Is this okay?  (Y/Yes/N/No): </source>
        <translation>Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3114"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Daemonen är lokal, utgår från att den är betrodd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3616"/>
        <source>Password for new watch-only wallet</source>
        <translation>Lösenord för ny granskningsplånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3685"/>
        <source>invalid arguments. Please use start_mining [&lt;number_of_threads&gt;] [do_bg_mining] [ignore_battery], &lt;number_of_threads&gt; should be from 1 to </source>
        <translation>ogiltiga argument. Använd start_mining [&lt;antal_trådar&gt;] [do_bg_mining] [ignore_battery], &lt;antal_trådar&gt; ska vara från 1 till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3873"/>
        <source>internal error: </source>
        <translation>internt fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1307"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3878"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4171"/>
        <source>unexpected error: </source>
        <translation>oväntat fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1236"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1312"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3883"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4176"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4673"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4781"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5014"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5213"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5518"/>
        <source>unknown error</source>
        <translation>okänt fel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3888"/>
        <source>refresh failed: </source>
        <translation>det gick inte att uppdatera: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3888"/>
        <source>Blocks received: </source>
        <translation>Mottagna block: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3919"/>
        <source>unlocked balance: </source>
        <translation>upplåst saldo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2418"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4066"/>
        <source>amount</source>
        <translation>belopp</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="253"/>
        <source>false</source>
        <translation>falskt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="558"/>
        <source>Unknown command: </source>
        <translation>Okänt kommando: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="565"/>
        <source>Command usage: </source>
        <translation>Användning av kommando: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="568"/>
        <source>Command description: </source>
        <translation>Beskrivning av kommando: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="614"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="678"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="811"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="849"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="928"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="977"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1029"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1108"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1181"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1248"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5430"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5493"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5530"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5570"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5781"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5871"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6982"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7056"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7098"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7140"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7190"/>
        <source>command not supported by HW wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="629"/>
        <source>wallet is multisig but not yet finalized</source>
        <translation>plånboken är multisig men är ännu inte slutförd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="642"/>
        <source>Enter optional seed encryption passphrase, empty to see raw seed</source>
        <translation>Ange valfri lösenfras för kryptering av startvärdet, lämna tomt för att se rådata för startvärdet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="659"/>
        <source>Failed to retrieve seed</source>
        <translation>Det gick inte att hämta startvärde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="683"/>
        <source>wallet is multisig and has no seed</source>
        <translation>plånboken är multisig och har inget startvärde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="756"/>
        <source>Cannot connect to daemon</source>
        <translation>Det går inte att ansluta till daemonen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="761"/>
        <source>Current fee is %s monero per kB</source>
        <translation>Aktuell avgift är %s monero per kB</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="777"/>
        <source>Error: failed to estimate backlog array size: </source>
        <translation>Fel: det gick inte att uppskatta eftersläpningsmatrisens storlek: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="782"/>
        <source>Error: bad estimated backlog array size</source>
        <translation>Fel: felaktigt uppskattat värde för eftersläpningsmatrisens storlek</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="794"/>
        <source> (current)</source>
        <translation> (aktuellt)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="797"/>
        <source>%u block (%u minutes) backlog at priority %u%s</source>
        <translation>%u blocks (%u minuters) eftersläpning vid prioritet %u%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="799"/>
        <source>%u to %u block (%u to %u minutes) backlog at priority %u</source>
        <translation>%u till %u blocks (%u till %u minuters) eftersläpning vid prioritet %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="802"/>
        <source>No backlog at priority </source>
        <translation>Ingen eftersläpning vid prioritet </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="816"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="854"/>
        <source>This wallet is already multisig</source>
        <translation>Denna plånbok är redan multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="821"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="859"/>
        <source>wallet is watch-only and cannot be made multisig</source>
        <translation>plånboken är enbart för granskning och kan inte göras om till multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="827"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="865"/>
        <source>This wallet has been used before, please use a new wallet to create a multisig wallet</source>
        <translation>Denna plånbok har använts tidigare. Använd en ny plånbok för att skapa en multisig-plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="834"/>
        <source>Your password is incorrect.</source>
        <translation>Ditt lösenord är fel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="840"/>
        <source>Send this multisig info to all other participants, then use make_multisig &lt;threshold&gt; &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Skicka denna multisig-info till alla andra deltagare och använd sedan make_multisig &lt;tröskelvärde&gt; &lt;info1&gt; [&lt;info2&gt;…] med de andras multisig-info</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="841"/>
        <source>This includes the PRIVATE view key, so needs to be disclosed only to that multisig wallet&apos;s participants </source>
        <translation>Detta innefattar den PRIVATA granskningsnyckeln, så den behöver endast lämnas ut till den multisig-plånbokens deltagare </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="878"/>
        <source>usage: make_multisig &lt;threshold&gt; &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</source>
        <translation>användning: make_multisig &lt;tröskelvärde&gt; &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;…]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="886"/>
        <source>Invalid threshold</source>
        <translation>Ogiltigt tröskelvärde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="899"/>
        <source>Another step is needed</source>
        <translation>Ytterligare ett steg krävs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="901"/>
        <source>Send this multisig info to all other participants, then use finalize_multisig &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Skicka denna multisig-info till alla andra deltagare, använd sedan finalize_multisig &lt;info1&gt; [&lt;info2&gt;…] med de andras multisig-info</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="907"/>
        <source>Error creating multisig: </source>
        <translation>Ett fel uppstod när multisig skapades: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="914"/>
        <source>Error creating multisig: new wallet is not multisig</source>
        <translation>Ett fel uppstod när multisig skapades: den nya plånboken är inte multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="917"/>
        <source> multisig address: </source>
        <translation> multisig-adress: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="933"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="982"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1034"/>
        <source>This wallet is not multisig</source>
        <translation>Denna plånbok är inte multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="938"/>
        <source>This wallet is already finalized</source>
        <translation>Denna plånbok är redan slutförd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="951"/>
        <source>usage: finalize_multisig &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</source>
        <translation>användning: finalize_multisig &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;…]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="959"/>
        <source>Failed to finalize multisig</source>
        <translation>Det gick inte att slutföra multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="965"/>
        <source>Failed to finalize multisig: </source>
        <translation>Det gick inte att slutföra multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="987"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1039"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1118"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1191"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1258"/>
        <source>This multisig wallet is not yet finalized</source>
        <translation>Denna multisig-plånbok är inte slutförd ännu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="992"/>
        <source>usage: export_multisig_info &lt;filename&gt;</source>
        <translation>användning: export_multisig_info &lt;filnamn&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1015"/>
        <source>Error exporting multisig info: </source>
        <translation>Ett fel uppstod när multisig-info exporterades: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1019"/>
        <source>Multisig info exported to </source>
        <translation>Multisig-info exporterades till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1044"/>
        <source>usage: import_multisig_info &lt;filename1&gt; [&lt;filename2&gt;...] - one for each other participant</source>
        <translation>användning: import_multisig_info &lt;filename1&gt; [&lt;filename2&gt;…] - en för varje annan deltagare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1072"/>
        <source>Multisig info imported</source>
        <translation>Multisig-info importerades</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1076"/>
        <source>Failed to import multisig info: </source>
        <translation>Det gick inte att importera multisig-info: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1087"/>
        <source>Failed to update spent status after importing multisig info: </source>
        <translation>Det gick inte att uppdatera spenderstatus efter import av multisig-info: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1092"/>
        <source>Untrusted daemon, spent status may be incorrect. Use a trusted daemon and run &quot;rescan_spent&quot;</source>
        <translation>Ej betrodd daemon. Spenderstatus kan vara felaktig. Använd en betrodd daemon och kör &quot;rescan_spent&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1113"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1186"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1253"/>
        <source>This is not a multisig wallet</source>
        <translation>Detta är inte en multisig-plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1123"/>
        <source>usage: sign_multisig &lt;filename&gt;</source>
        <translation>användning: sign_multisig &lt;filnamn&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1136"/>
        <source>Failed to sign multisig transaction</source>
        <translation>Det gick inte att signera multisig-transaktion</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1142"/>
        <source>Multisig error: </source>
        <translation>Multisig-fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1147"/>
        <source>Failed to sign multisig transaction: </source>
        <translation>Det gick inte att signera multisig-transaktion: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1170"/>
        <source>It may be relayed to the network with submit_multisig</source>
        <translation>Den kan skickas vidare till nätverket med submit_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1196"/>
        <source>usage: submit_multisig &lt;filename&gt;</source>
        <translation>användning: submit_multisig &lt;filnamn&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1211"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1277"/>
        <source>Failed to load multisig transaction from file</source>
        <translation>Det gick inte att läsa in multisig-transaktion från fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1216"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1282"/>
        <source>Multisig transaction signed by only %u signers, needs %u more signatures</source>
        <translation>Multisig-transaktion har signerats av bara %u signerare. Den behöver %u ytterligare signaturer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1225"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7453"/>
        <source>Transaction successfully submitted, transaction </source>
        <translation>Transaktionen skickades, transaktion </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1226"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7454"/>
        <source>You can check its status by using the `show_transfers` command.</source>
        <translation>Du kan kontrollera dess status genom att använda kommandot &apos;show_transfers&apos;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1263"/>
        <source>usage: export_raw_multisig &lt;filename&gt;</source>
        <translation>användning: export_raw_multisig &lt;filnamn&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1298"/>
        <source>Failed to export multisig transaction to file </source>
        <translation>Det gick inte att exportera multisig-transaktion till fil </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1302"/>
        <source>Saved exported multisig transaction file(s): </source>
        <translation>Sparade filer med exporterade multisig-transaktioner: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1324"/>
        <source>usage: print_ring &lt;key_image|txid&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1330"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1380"/>
        <source>Invalid key image</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1336"/>
        <source>Invalid txid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1348"/>
        <source>Key image either not spent, or spent with mixin 0</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1363"/>
        <source>Failed to get key image ring: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1374"/>
        <source>usage: set_ring &lt;key_image&gt; absolute|relative &lt;index&gt; [&lt;index&gt;...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1395"/>
        <source>Missing absolute or relative keyword</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1405"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1412"/>
        <source>invalid index: must be a strictly positive unsigned integer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1420"/>
        <source>invalid index: indices wrap</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1430"/>
        <source>invalid index: indices should be in strictly ascending order</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1437"/>
        <source>failed to set ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1449"/>
        <source>usage: blackball &lt;output_public_key&gt; | &lt;filename&gt; [add]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1479"/>
        <source>Invalid public key: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1489"/>
        <source>Bad argument: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1489"/>
        <source>should be &quot;add&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1498"/>
        <source>Failed to open file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1504"/>
        <source>Invalid public key, and file doesn&apos;t exist</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1510"/>
        <source>Failed to blackball output: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1521"/>
        <source>usage: unblackball &lt;output_public_key&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1527"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1554"/>
        <source>Invalid public key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1537"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1567"/>
        <source>Failed to unblackball output: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1548"/>
        <source>usage: blackballed &lt;output_public_key&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1561"/>
        <source>Blackballed: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1563"/>
        <source>not blackballed: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1582"/>
        <source>Failed to save known rings: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1643"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1649"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1666"/>
        <source>ring size must be an integer &gt;= </source>
        <translation>ringstorlek måste vara ett heltal &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1654"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4621"/>
        <source>WARNING: this is a non default ring size, which may harm your privacy. Default is recommended.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1671"/>
        <source>could not change default ring size</source>
        <translation>det gick inte att ändra standardinställning för ringstorlek</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1912"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1983"/>
        <source>Invalid height</source>
        <translation>Ogiltig höjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2016"/>
        <source>start_mining [&lt;number_of_threads&gt;] [bg_mining] [ignore_battery]</source>
        <translation>start_mining [&lt;antal_trådar&gt;] [&lt;bgbrytning&gt;] [&lt;ignorera_batteri&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2017"/>
        <source>Start mining in the daemon (bg_mining and ignore_battery are optional booleans).</source>
        <translation>Starta brytning i daemonen (bgbrytning och ignorera_batteri är valfri booleska värden).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2020"/>
        <source>Stop mining in the daemon.</source>
        <translation>Stoppa brytning i daemonen.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2023"/>
        <source>set_daemon &lt;host&gt;[:&lt;port&gt;]</source>
        <translation>set_daemon &lt;värddator&gt;[:&lt;port&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2024"/>
        <source>Set another daemon to connect to.</source>
        <translation>Ange en annan daemon att ansluta till.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2027"/>
        <source>Save the current blockchain data.</source>
        <translation>Spara aktuella blockkedjedata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2030"/>
        <source>Synchronize the transactions and balance.</source>
        <translation>Synkronisera transaktionerna och saldot.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2033"/>
        <source>balance [detail]</source>
        <translation>balance [detail]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2034"/>
        <source>Show the wallet&apos;s balance of the currently selected account.</source>
        <translation>Visa plånbokens saldo för det aktiva kontot.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2037"/>
        <source>incoming_transfers [available|unavailable] [verbose] [index=&lt;N1&gt;[,&lt;N2&gt;[,...]]]</source>
        <translation>incoming_transfers [available|unavailable] [verbose] [index=&lt;N1&gt;[, &lt;N2&gt;[, …]]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2038"/>
        <source>Show the incoming transfers, all or filtered by availability and address index.</source>
        <translation>Visa inkommande överföringar: alla eller filtrerade efter tillgänglighet och adressindex.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2041"/>
        <source>payments &lt;PID_1&gt; [&lt;PID_2&gt; ... &lt;PID_N&gt;]</source>
        <translation>payments &lt;BID_1&gt; [&lt;BID_2&gt; … &lt;BID_N&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2042"/>
        <source>Show the payments for the given payment IDs.</source>
        <translation>Visa betalningar för givna betalnings-ID.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2045"/>
        <source>Show the blockchain height.</source>
        <translation>Visa blockkedjans höjd.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2048"/>
        <source>transfer_original [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation>transfer_original [index=&lt;N1&gt;[, &lt;N2&gt;, …]] [&lt;prioritet&gt;] [&lt;ringstorlek&gt;] &lt;adress&gt; &lt;belopp&gt; [&lt;betalnings_id&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2049"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; using an older transaction building algorithm. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Överför &lt;belopp&gt; till &lt;adress&gt; genom att använda en äldre algoritm för att bygga transaktioner. Om parametern &quot;index=&lt;N1&gt;[, &lt;N2&gt;, …]&quot; anges använder plånboken utgångar som tagits emot av adresser vid dessa index. Om parametern utelämnas väljer plånboken slumpmässigt adressindex att använda. Oavsett vilket kommer den att göra sitt bästa för att inte kombinera utgångar från flera adresser. &lt;prioritet&gt; är transaktionens prioritet. Ju högre prioritet, desto högre transaktionsavgift. Giltiga värden i prioritetsordning (från lägsta till högsta) är: unimportant, normal, elevated, priority. Om värdet utelämnas kommer standardvärdet att användas (se kommandot &quot;set priority&quot;). &lt;ringstorlek&gt; är det antal ingångar som ska inkluderas för att uppnå ospårbarhet. Flera betalningar kan göras på en gång genom att lägga till &lt;adress_2&gt; &lt;belopp_2&gt; osv (före betalnings-ID, om det inkluderas)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2051"/>
        <source>transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation>transfer [index=&lt;N1&gt;[, &lt;N2&gt;, …]] [&lt;prioritet&gt;] [&lt;ringstorlek&gt;] &lt;adress&gt; &lt;belopp&gt; [&lt;betalnings_id&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2052"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt;. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Överför &lt;belopp&gt; till &lt;adress&gt;. Om parametern &quot;index=&lt;N1&gt;[, &lt;N2&gt;, …]&quot; anges använder plånboken utgångar som tagits emot av adresser vid dessa index. Om parametern utelämnas väljer plånboken slumpmässigt adressindex att använda. Oavsett vilket kommer den att göra sitt bästa för att inte kombinera utgångar från flera adresser. &lt;prioritet&gt; är transaktionens prioritet. Ju högre prioritet, desto högre transaktionsavgift. Giltiga värden i prioritetsordning (från lägsta till högsta) är: unimportant, normal, elevated, priority. Om värdet utelämnas kommer standardvärdet att användas (se kommandot &quot;set priority&quot;). &lt;ringstorlek&gt; är det antal ingångar som ska inkluderas för att uppnå ospårbarhet. Flera betalningar kan göras på en gång genom att lägga till &lt;adress_2&gt; &lt;belopp_2&gt; osv (före betalnings-ID, om det inkluderas)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2055"/>
        <source>locked_transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;addr&gt; &lt;amount&gt; &lt;lockblocks&gt; [&lt;payment_id&gt;]</source>
        <translation>locked_transfer [index=&lt;N1&gt;[, &lt;N2&gt;, …]] [&lt;prioritet&gt;] [&lt;ringstorlek&gt;] &lt;adress&gt; &lt;belopp&gt; &lt;låsblock&gt; [&lt;betalnings_id&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2056"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Överför &lt;belopp&gt; till &lt;adress&gt; och lås det i &lt;låsblock&gt; (max. 1000000). Om parametern &quot;index=&lt;N1&gt;[, &lt;N2&gt;, …]&quot; anges använder plånboken utgångar som tagits emot av adresser vid dessa index. Om parametern utelämnas väljer plånboken slumpmässigt adressindex att använda. Oavsett vilket kommer den att göra sitt bästa för att inte kombinera utgångar från flera adresser. &lt;prioritet&gt; är transaktionens prioritet. Ju högre prioritet, desto högre transaktionsavgift. Giltiga värden i prioritetsordning (från lägsta till högsta) är: unimportant, normal, elevated, priority. Om värdet utelämnas kommer standardvärdet att användas (se kommandot &quot;set priority&quot;). &lt;ringstorlek&gt; är det antal ingångar som ska inkluderas för att uppnå ospårbarhet. Flera betalningar kan göras på en gång genom att lägga till &lt;adress_2&gt; &lt;belopp_2&gt; osv (före betalnings-ID, om det inkluderas)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2059"/>
        <source>Send all unmixable outputs to yourself with ring_size 1</source>
        <translation>Skicka alla omixbara utgångar till dig själv med ringstorlek 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2061"/>
        <source>sweep_all [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation>sweep_all [index=&lt;N1&gt;[, &lt;N2&gt;, …]] [&lt;prioritet&gt;] [&lt;ringstorlek&gt;] &lt;adress&gt; [&lt;betalnings_id&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2062"/>
        <source>Send all unlocked balance to an address. If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used.</source>
        <translation>Skicka allt upplåst saldo till en adress. Om parametern &quot;index&lt;N1&gt;[, &lt;N2&gt;, …]&quot; anges sveper plånboken upp utgångar som tagits emot av adresserna vid dessa index. Om parametern utelämnas väljer plånboken slumpmässigt ett adressindex att använda.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2065"/>
        <source>sweep_below &lt;amount_threshold&gt; [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation>sweep_below &lt;tröskelbelopp&gt; [index=&lt;N1&gt;[, &lt;N2&gt;, …]] [&lt;prioritet&gt;] [&lt;ringstorlek&gt;] &lt;adress&gt; [&lt;betalnings_id&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2066"/>
        <source>Send all unlocked outputs below the threshold to an address.</source>
        <translation>Skicka alla upplåsta utgångar under tröskelvärdet till en adress.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2069"/>
        <source>sweep_single [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;key_image&gt; &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation>sweep_single [&lt;prioritet&gt;] [&lt;ringstorlek&gt;] &lt;nyckelavbildning&gt; &lt;adress&gt; [&lt;betalnings_id&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2070"/>
        <source>Send a single output of the given key image to an address without change.</source>
        <translation>Skicka en enda utgång hos den givna nyckelavbildningen till en adress utan växel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2073"/>
        <source>donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation>donate [index=&lt;N1&gt;[, &lt;N2&gt;, …]] [&lt;prioritet&gt;] [&lt;ringstorlek&gt;] &lt;belopp&gt; [&lt;betalnings_id&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2074"/>
        <source>Donate &lt;amount&gt; to the development team (donate.getmonero.org).</source>
        <translation>Donera &lt;belopp&gt; till utvecklingsteamet (donate.getmonero.org).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2077"/>
        <source>sign_transfer &lt;file&gt;</source>
        <translation>sign_transfer &lt;fil&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2078"/>
        <source>Sign a transaction from a &lt;file&gt;.</source>
        <translation>Signera en transaktion från &lt;fil&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2081"/>
        <source>Submit a signed transaction from a file.</source>
        <translation>Skicka en signerad transaktion från en fil.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2084"/>
        <source>set_log &lt;level&gt;|{+,-,}&lt;categories&gt;</source>
        <translation>set_log &lt;nivå&gt;|{+,-,}&lt;kategorier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2085"/>
        <source>Change the current log detail (level must be &lt;0-4&gt;).</source>
        <translation>Ändra detaljnivån för aktuell logg (nivå måste vara 0-4).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2088"/>
        <source>account
  account new &lt;label text with white spaces allowed&gt;
  account switch &lt;index&gt; 
  account label &lt;index&gt; &lt;label text with white spaces allowed&gt;
  account tag &lt;tag_name&gt; &lt;account_index_1&gt; [&lt;account_index_2&gt; ...]
  account untag &lt;account_index_1&gt; [&lt;account_index_2&gt; ...]
  account tag_description &lt;tag_name&gt; &lt;description&gt;</source>
        <translation>account
  account new &lt;etikettext med blanktecken tillåtna&gt;
  account switch &lt;index&gt; 
  account label &lt;index&gt; &lt;etikettext med blanktecken tillåtna&gt;
  account tag &lt;taggnamn&gt; &lt;kontoindex_1&gt; [&lt;kontoindex_2&gt; …]
  account untag &lt;kontoindex_1&gt; [&lt;kontoindex_2&gt; …]
  account tag_description &lt;taggnamn&gt; &lt;beskrivning&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2095"/>
        <source>If no arguments are specified, the wallet shows all the existing accounts along with their balances.
If the &quot;new&quot; argument is specified, the wallet creates a new account with its label initialized by the provided label text (which can be empty).
If the &quot;switch&quot; argument is specified, the wallet switches to the account specified by &lt;index&gt;.
If the &quot;label&quot; argument is specified, the wallet sets the label of the account specified by &lt;index&gt; to the provided label text.
If the &quot;tag&quot; argument is specified, a tag &lt;tag_name&gt; is assigned to the specified accounts &lt;account_index_1&gt;, &lt;account_index_2&gt;, ....
If the &quot;untag&quot; argument is specified, the tags assigned to the specified accounts &lt;account_index_1&gt;, &lt;account_index_2&gt; ..., are removed.
If the &quot;tag_description&quot; argument is specified, the tag &lt;tag_name&gt; is assigned an arbitrary text &lt;description&gt;.</source>
        <translation>Om inga argument anges visas plånbokens samtliga befintliga konton, tillsammans med deras respektive saldo.
Om argumentet &quot;new&quot; anges, skapar plånboken ett nytt konto med etiketten satt till med den angivna etikettexten (som kan vara tom).
Om argumentet &quot;switch&quot; anges, växlar plånboken till det konto som anges av &lt;index&gt;.
Om argumentet &quot;label&quot; anges, sätter plånboken etiketten för kontot som anges av &lt;index&gt; till den angivna etikettexten.
Om argumentet &quot;tag&quot; anges, så tilldelas taggen &lt;taggnamn&gt; till det angivna kontona &lt;kontoindex_1&gt;, &lt;kontoindex_2&gt;, …
Om argumentet &quot;untag&quot; anges, tas tilldelade taggar bort från de angivna kontona &lt;kontoindex_1&gt;, &lt;kontoindex_2&gt; …
Om argumentet &quot;tag_description&quot; anges, så tilldelas taggen &lt;taggnamn&gt; den godtyckliga texten &lt;beskrivning&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2104"/>
        <source>address [ new &lt;label text with white spaces allowed&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;label text with white spaces allowed&gt;]</source>
        <translation>address [new &lt;etikettext med blanktecken tillåtna&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;etikettext med blanktecken tillåtna&gt;]</translation>
    </message>
    <message>
        <source>If no arguments are specified or &lt;index&gt; is specified, the wallet shows the default or specified address. If &quot;all&quot; is specified, the walllet shows all the existing addresses in the currently selected account. If &quot;new &quot; is specified, the wallet creates a new address with the provided label text (which can be empty). If &quot;label&quot; is specified, the wallet sets the label of the address specified by &lt;index&gt; to the provided label text.</source>
        <translation type="vanished">Om inga argument anges, eller om &lt;index&gt; anges, visar plånboken standardadressen eller den angivna adressen. Om argumentet &quot;all&quot; anges visar plånboken samtliga befintliga adresser i det aktiva kontot. Om argumentet &quot;new &quot; anges skapar plånboken en ny adress med den angivna etikettexten (som kan vara tom). Om argumentet &quot;label&quot; anges sätter plånboken etiketten för adressen som anges av &lt;index&gt; till den angivna etikettexten.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2108"/>
        <source>integrated_address [&lt;payment_id&gt; | &lt;address&gt;]</source>
        <translation>integrated_address [&lt;betalnings-id&gt; | &lt;adress&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2109"/>
        <source>Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation>Koda ett betalnings-ID till en integrerad adress för den aktuella plånbokens publika adress (om inget argument anges används ett slumpmässigt betalnings-ID), eller avkoda en integrerad adress till en standardadress och ett betalnings-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2112"/>
        <source>address_book [(add ((&lt;address&gt; [pid &lt;id&gt;])|&lt;integrated address&gt;) [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</source>
        <translation>address_book [(add ((&lt;adress&gt; [pid &lt;id&gt;])|&lt;integrerad adress&gt;) [&lt;beskrivning eventuellt med blanktecken&gt;])|(delete &lt;index&gt;)]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2113"/>
        <source>Print all entries in the address book, optionally adding/deleting an entry to/from it.</source>
        <translation>Skriv ut alla poster i adressboken, och valfritt lägg till/ta bort en post i den.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2116"/>
        <source>Save the wallet data.</source>
        <translation>Spara plånboksdata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2119"/>
        <source>Save a watch-only keys file.</source>
        <translation>Spara en fil med granskningsnycklar.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2122"/>
        <source>Display the private view key.</source>
        <translation>Visa privat granskningsnyckel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2125"/>
        <source>Display the private spend key.</source>
        <translation>Visa privat spendernyckel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2128"/>
        <source>Display the Electrum-style mnemonic seed</source>
        <translation>Visa det minnesbaserade startvärdet (Electrum-typ)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2131"/>
        <source>set &lt;option&gt; [&lt;value&gt;]</source>
        <translation>set &lt;alternativ&gt; [&lt;värde&gt;]</translation>
    </message>
    <message>
        <source>Available options:
 seed language
   Set the wallet&apos;s seed language.
 always-confirm-transfers &lt;1|0&gt;
   Whether to confirm unsplit txes.
 print-ring-members &lt;1|0&gt;
   Whether to print detailed information about ring members during confirmation.
 store-tx-info &lt;1|0&gt;
   Whether to store outgoing tx info (destination address, payment ID, tx secret key) for future reference.
 default-ring-size &lt;n&gt;
   Set the default ring size (default and minimum is 5).
 auto-refresh &lt;1|0&gt;
   Whether to automatically synchronize new blocks from the daemon.
 refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt;
   Set the wallet&apos;s refresh behaviour.
 priority [0|1|2|3|4]
   Set the fee too default/unimportant/normal/elevated/priority.
 confirm-missing-payment-id &lt;1|0&gt;
 ask-password &lt;1|0&gt;
 unit &lt;monero|millinero|micronero|nanonero|piconero&gt;
   Set the default monero (sub-)unit.
 min-outputs-count [n]
   Try to keep at least that many outputs of value at least min-outputs-value.
 min-outputs-value [n]
   Try to keep at least min-outputs-count outputs of at least that value.
 merge-destinations &lt;1|0&gt;
   Whether to merge multiple payments to the same destination address.
 confirm-backlog &lt;1|0&gt;
   Whether to warn if there is transaction backlog.
 confirm-backlog-threshold [n]
   Set a threshold for confirm-backlog to only warn if the transaction backlog is greater than n blocks.
 refresh-from-block-height [n]
   Set the height before which to ignore blocks.
 auto-low-priority &lt;1|0&gt;
   Whether to automatically use the low priority fee level when it&apos;s safe to do so.</source>
        <translation type="vanished">Tillgängliga alternativ:
 språk för startvärde
   Ange plånbokens språk för startvärde.
 always-confirm-transfers &lt;1|0&gt;
   Om ej delade transaktioner ska bekräftas.
 print-ring-members &lt;1|0&gt;
   Om detaljerad information om ringmedlemmar ska skrivas ut vid bekräftelse.
 store-tx-info &lt;1|0&gt;
   Om information om utgående transaktion ska sparas (måladress, betalnings-ID, hemlig tx-nyckel) som referens.
 default-ring-size &lt;n&gt;
   Ange standardinställning för ringstorlek (standard och minimum är 5).
 auto-refresh &lt;1|0&gt;
   Om nya block ska synkas automatiskt från daemonen.
 refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt;
   Ange plånbokens uppdateringsbeteende.
 priority [0|1|2|3|4]
   Sätt avgiften till default/unimportant/normal/elevated/priority.
 confirm-missing-payment-id &lt;1|0&gt;
 ask-password &lt;1|0&gt;
 unit &lt;monero|millinero|micronero|nanonero|piconero&gt;
   Ange standardvärde för moneroenhet.
 min-outputs-count [n]
   Försök att behålla åtminstone så många utgångar med åtminstone värdet min-outputs-value.
 min-outputs-value [n]
   Försök att behålla åtminstone min-outputs-count utgångar med åtminstone det värdet.
 merge-destinations &lt;1|0&gt;
   Om flera betalningar till samma måladress ska sammanslås.
 confirm-backlog &lt;1|0&gt;
   Om en varning ska visas om det föreligger transaktionseftersläpning.
 confirm-backlog-threshold [n]
   Ange ett tröskelvärde för confirm-backlog för att endast varna om transaktionseftersläpningen är större än n block.
 refresh-from-block-height [n]
   Ange höjden upp till vilken block ska ignoreras.
 auto-low-priority &lt;1|0&gt;
   Om avgiftsnivån för låg prioritet automatiskt ska användas när detta är säkert att göra.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2178"/>
        <source>Display the encrypted Electrum-style mnemonic seed.</source>
        <translation>Visa det krypterade minnesbaserade startvärdet (Electrum-typ).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2181"/>
        <source>Rescan the blockchain for spent outputs.</source>
        <translation>Genomsök blockkedjan efter spenderade utgångar.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2184"/>
        <source>get_tx_key &lt;txid&gt;</source>
        <translation>get_tx_key &lt;txid&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2185"/>
        <source>Get the transaction key (r) for a given &lt;txid&gt;.</source>
        <translation>Hämta transaktionsnyckel (r) för ett givet &lt;txid&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2188"/>
        <source>check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</source>
        <translation>check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;adress&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2189"/>
        <source>Check the amount going to &lt;address&gt; in &lt;txid&gt;.</source>
        <translation>Kontrollera belopp som går till &lt;adress&gt; i &lt;txid&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2192"/>
        <source>get_tx_proof &lt;txid&gt; &lt;address&gt; [&lt;message&gt;]</source>
        <translation>get_tx_proof &lt;txid&gt; &lt;adress&gt; [&lt;meddelande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2193"/>
        <source>Generate a signature proving funds sent to &lt;address&gt; in &lt;txid&gt;, optionally with a challenge string &lt;message&gt;, using either the transaction secret key (when &lt;address&gt; is not your wallet&apos;s address) or the view secret key (otherwise), which does not disclose the secret key.</source>
        <translation>Skapa en signatur som bevisar att pengar skickades till &lt;adress&gt; i &lt;txid&gt;, valfritt med kontrollsträngen &lt;meddelande&gt;, genom att använda antingen transaktionens hemliga nyckel (när &lt;adress&gt; inte är din plånboks adress) eller den hemliga granskningsnyckeln (annars), vilket inte lämnar ut den hemliga nyckeln.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2196"/>
        <source>check_tx_proof &lt;txid&gt; &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>check_tx_proof &lt;txid&gt; &lt;adress&gt; &lt;signaturfil&gt; [&lt;meddelande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2197"/>
        <source>Check the proof for funds going to &lt;address&gt; in &lt;txid&gt; with the challenge string &lt;message&gt; if any.</source>
        <translation>Kontrollera beviset för pengar som skickats till &lt;adress&gt; i &lt;txid&gt; med kontrollsträngen &lt;meddelande&gt;, om den angivits.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2200"/>
        <source>get_spend_proof &lt;txid&gt; [&lt;message&gt;]</source>
        <translation>get_spend_proof &lt;txid&gt; [&lt;meddelande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2201"/>
        <source>Generate a signature proving that you generated &lt;txid&gt; using the spend secret key, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Skapa en signatur som bevisar att du skapade &lt;txid&gt; genom att använda den hemliga spendernyckeln, valfritt med kontrollsträngen &lt;meddelande&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2204"/>
        <source>check_spend_proof &lt;txid&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>check_spend_proof &lt;txid&gt; &lt;signaturfil&gt; [&lt;meddelande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2205"/>
        <source>Check a signature proving that the signer generated &lt;txid&gt;, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Kontrollera en signatur som bevisar att signeraren skapade &lt;txid&gt;, valfritt med kontrollsträngen &lt;meddelande&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2208"/>
        <source>get_reserve_proof (all|&lt;amount&gt;) [&lt;message&gt;]</source>
        <translation>get_reserve_proof (all|&lt;belopp&gt;) [&lt;meddelande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2209"/>
        <source>Generate a signature proving that you own at least this much, optionally with a challenge string &lt;message&gt;.
If &apos;all&apos; is specified, you prove the entire sum of all of your existing accounts&apos; balances.
Otherwise, you prove the reserve of the smallest possible amount above &lt;amount&gt; available in your current account.</source>
        <translation>Skapa en signatur som bevisar att du äger åtminstone så här mycket, valfritt med kontrollsträngen &lt;meddelande&gt;.
Om &apos;all&apos; anges, bevisar du totalsumman av alla dina befintliga kontons saldo.
Annars bevisar du reserven för det minsta möjliga belopp över &lt;belopp&gt; som är tillgängligt på ditt aktuella konto.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2214"/>
        <source>check_reserve_proof &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>check_reserve_proof &lt;adress&gt; &lt;signaturfil&gt; [&lt;meddelande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2215"/>
        <source>Check a signature proving that the owner of &lt;address&gt; holds at least this much, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Kontrollera en signatur som bevisar att ägaren till &lt;adress&gt; har åtminstone så här mycket, valfritt med kontrollsträngen &lt;meddelande&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2218"/>
        <source>show_transfers [in|out|pending|failed|pool] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation>show_transfers [in|out|pending|failed|pool] [index=&lt;N1&gt;[, &lt;N2&gt;, …]] [&lt;min_höjd&gt; [&lt;max_höjd&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2219"/>
        <source>Show the incoming/outgoing transfers within an optional height range.</source>
        <translation>Visa inkommande/utgående överföringar inom ett valfritt höjdintervall.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2222"/>
        <source>unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_amount&gt; [&lt;max_amount&gt;]]</source>
        <translation>unspent_outputs [index=&lt;N1&gt;[, &lt;N2&gt;, …]] [&lt;min_belopp&gt; [&lt;max_belopp&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2223"/>
        <source>Show the unspent outputs of a specified address within an optional amount range.</source>
        <translation>Visa de ej spenderade utgångarna hos en angiven adress inom ett valfritt beloppsintervall.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2226"/>
        <source>Rescan the blockchain from scratch.</source>
        <translation>Genomsök blockkedjan från början.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2229"/>
        <source>set_tx_note &lt;txid&gt; [free text note]</source>
        <translation>set_tx_note &lt;txid&gt; [&lt;fritextanteckning&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2230"/>
        <source>Set an arbitrary string note for a &lt;txid&gt;.</source>
        <translation>Ange en godtycklig stränganteckning för &lt;txid&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2233"/>
        <source>get_tx_note &lt;txid&gt;</source>
        <translation>get_tx_note &lt;txid&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2234"/>
        <source>Get a string note for a txid.</source>
        <translation>Hämta en stränganteckning för ett txid.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2237"/>
        <source>set_description [free text note]</source>
        <translation>set_description [&lt;fritextanteckning&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2238"/>
        <source>Set an arbitrary description for the wallet.</source>
        <translation>Ange en godtycklig beskrivning av plånboken.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2241"/>
        <source>Get the description of the wallet.</source>
        <translation>Hämta plånbokens beskrivning.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2244"/>
        <source>Show the wallet&apos;s status.</source>
        <translation>Visa plånbokens status.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2247"/>
        <source>Show the wallet&apos;s information.</source>
        <translation>Visa information om plånboken.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2250"/>
        <source>sign &lt;file&gt;</source>
        <translation>sign &lt;fil&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2251"/>
        <source>Sign the contents of a file.</source>
        <translation>Signera innehållet i en fil.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2254"/>
        <source>verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation>verify &lt;filnamn&gt; &lt;adress&gt; &lt;signatur&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2255"/>
        <source>Verify a signature on the contents of a file.</source>
        <translation>Verifiera en signatur av innehållet in en fil.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2258"/>
        <source>export_key_images &lt;file&gt;</source>
        <translation>export_key_images &lt;fil&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2259"/>
        <source>Export a signed set of key images to a &lt;file&gt;.</source>
        <translation>Exportera en signerad uppsättning nyckelavbildningar till &lt;fil&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2262"/>
        <source>import_key_images &lt;file&gt;</source>
        <translation>import_key_images &lt;fil&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2263"/>
        <source>Import a signed key images list and verify their spent status.</source>
        <translation>Importera en signerad lista av nyckelavbildningar och verifiera deras spenderstatus.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2266"/>
        <source>export_outputs &lt;file&gt;</source>
        <translation>export_outputs &lt;fil&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2267"/>
        <source>Export a set of outputs owned by this wallet.</source>
        <translation>Exportera en uppsättning utgångar som ägs av denna plånbok.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2270"/>
        <source>import_outputs &lt;file&gt;</source>
        <translation>import_outputs &lt;fil&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2271"/>
        <source>Import a set of outputs owned by this wallet.</source>
        <translation>Importera en uppsättning utgångar som ägs av denna plånbok.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2274"/>
        <source>show_transfer &lt;txid&gt;</source>
        <translation>show_transfer &lt;txid&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2275"/>
        <source>Show information about a transfer to/from this address.</source>
        <translation>Visa information om en transktion till/från denna adress.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2278"/>
        <source>Change the wallet&apos;s password.</source>
        <translation>Ändra plånbokens lösenord.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2281"/>
        <source>Generate a new random full size payment id. These will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids.</source>
        <translation>Skapa ett nytt slumpmässigt betalnings-ID av normalstorlek. Dessa kommer att vara okrypterade på blockkedjan. Se integrated_address för krypterade korta betalnings-ID.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2284"/>
        <source>Print the information about the current fee and transaction backlog.</source>
        <translation>Skriv ut information om aktuell avgift och transaktionseftersläpning.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2286"/>
        <source>Export data needed to create a multisig wallet</source>
        <translation>Exportera data som krävs för att skapa en multisig-plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2288"/>
        <source>make_multisig &lt;threshold&gt; &lt;string1&gt; [&lt;string&gt;...]</source>
        <translation>make_multisig &lt;tröskelvärde&gt; &lt;string1&gt; [&lt;sträng&gt;…]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2289"/>
        <source>Turn this wallet into a multisig wallet</source>
        <translation>Gör denna plånbok till en multisig-plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2292"/>
        <source>finalize_multisig &lt;string&gt; [&lt;string&gt;...]</source>
        <translation>finalize_multisig &lt;sträng&gt; [&lt;sträng&gt;…]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2293"/>
        <source>Turn this wallet into a multisig wallet, extra step for N-1/N wallets</source>
        <translation>Gör denna plånbok till en multisig-plånbok, extra steg för plånböcker med N-1/N</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2296"/>
        <source>export_multisig_info &lt;filename&gt;</source>
        <translation>export_multisig_info &lt;filnamn&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2297"/>
        <source>Export multisig info for other participants</source>
        <translation>Exportera multisig-info för andra deltagare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2300"/>
        <source>import_multisig_info &lt;filename&gt; [&lt;filename&gt;...]</source>
        <translation>import_multisig_info &lt;filnamn&gt; [&lt;filnamn&gt;…]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2301"/>
        <source>Import multisig info from other participants</source>
        <translation>Importera multisig-info från andra deltagare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2304"/>
        <source>sign_multisig &lt;filename&gt;</source>
        <translation>sign_multisig &lt;filnamn&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2305"/>
        <source>Sign a multisig transaction from a file</source>
        <translation>Signera en a multisig-transaktion från en fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2308"/>
        <source>submit_multisig &lt;filename&gt;</source>
        <translation>submit_multisig &lt;filnamn&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2309"/>
        <source>Submit a signed multisig transaction from a file</source>
        <translation>Skicka en signerad multisig-transaktion från en fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2312"/>
        <source>export_raw_multisig_tx &lt;filename&gt;</source>
        <translation>export_raw_multisig_tx &lt;filnamn&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2313"/>
        <source>Export a signed multisig transaction to a file</source>
        <translation>Exportera en signerad multisig-transaktion till en fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2316"/>
        <source>print_ring &lt;key_image&gt; | &lt;txid&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2317"/>
        <source>Print the ring(s) used to spend a given key image or transaction (if the ring size is &gt; 1)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2320"/>
        <source>set_ring &lt;key_image&gt; absolute|relative &lt;index&gt; [&lt;index&gt;...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2321"/>
        <source>Set the ring used for a given key image, so it can be reused in a fork</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2324"/>
        <source>save_known_rings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2325"/>
        <source>Save known rings to the shared rings database</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2328"/>
        <source>blackball &lt;output public key&gt; | &lt;filename&gt; [add]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2329"/>
        <source>Blackball output(s) so they never get selected as fake outputs in a ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2332"/>
        <source>unblackball &lt;output public key&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2333"/>
        <source>Unblackballs an output so it may get selected as a fake output in a ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2336"/>
        <source>blackballed &lt;output public key&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2337"/>
        <source>Checks whether an output is blackballed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2340"/>
        <source>help [&lt;command&gt;]</source>
        <translation>help [&lt;kommando&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2341"/>
        <source>Show the help section or the documentation about a &lt;command&gt;.</source>
        <translation>Visa hjälpavsnittet eller dokumentationen för &lt;kommando&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2410"/>
        <source>integer &gt;= </source>
        <translation>heltal &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2423"/>
        <source>block height</source>
        <translation>blockhöjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2427"/>
        <source>&lt;major&gt;:&lt;minor&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2509"/>
        <source>No wallet found with that name. Confirm creation of new wallet named: </source>
        <translation>Ingen plånbok med det namnet kunde hittas. Bekräfta skapande av ny plånbok med namn: </translation>
    </message>
    <message>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot; and --generate-from-json=&quot;jsonfilename&quot;</source>
        <translation type="vanished">det går inte att ange fler än en av --generate-new-wallet=&quot;plånboksnamn&quot;, --wallet-file=&quot;plånboksnamn&quot;, --generate-from-view-key=&quot;plånboksnamn&quot;, --generate-from-spend-key=&quot;plånboksnamn&quot;, --generate-from-keys=&quot;plånboksnamn&quot;, --generate-from-multisig-keys=&quot;plånboksnamn&quot; och --generate-from-json=&quot;json-filnamn&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2593"/>
        <source>can&apos;t specify both --restore-deterministic-wallet or --restore-multisig-wallet and --non-deterministic</source>
        <translation>det går inte att ange både --restore-deterministic-wallet eller --restore-multisig-wallet och --non-deterministic</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2599"/>
        <source>--restore-multisig-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-multisig-wallet använder --generate-new-wallet, inte --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2615"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;multisig seed here&quot;</source>
        <translation>ange en återställningsparameter med --electrum-seed=&quot;startvärde för multisig&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2642"/>
        <source>Multisig seed failed verification</source>
        <translation>Startvärde för multisig kunde inte verifieras</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2655"/>
        <source>Enter seed encryption passphrase, empty if none</source>
        <translation>Ange lösenfras för kryptering av startvärde, lämna tomt om ingen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2691"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2765"/>
        <source>This address is a subaddress which cannot be used here.</source>
        <translation>Denna adress är en underadress som inte kan användas här.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2843"/>
        <source>Error: expected M/N, but got: </source>
        <translation>Fel: förväntade M/N, men fick: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2848"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got: </source>
        <translation>Fel: förväntade N &gt; 1 och N &lt;= M, men fick: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2853"/>
        <source>Error: M/N is currently unsupported. </source>
        <translation>Fel: M/N stöds för närvarande inte. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2856"/>
        <source>Generating master wallet from %u of %u multisig wallet keys</source>
        <translation>Skapar huvudplånbok från %u av %u multisig-plånboksnycklar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2885"/>
        <source>failed to parse secret view key</source>
        <translation>det gick inte att parsa hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2894"/>
        <source>failed to verify secret view key</source>
        <translation>det gick inte att verifiera hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2914"/>
        <source>Secret spend key (%u of %u):</source>
        <translation>Hemlig spendernyckel (%u av %u):</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2938"/>
        <source>Error: M/N is currently unsupported</source>
        <translation>Fel: M/N stöds för närvarande inte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3084"/>
        <source>Restore height </source>
        <translation>Återställningshöjd </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3085"/>
        <source>Still apply restore height?  (Y/Yes/N/No): </source>
        <translation>Ska återställningshöjd fortfarande appliceras?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3097"/>
        <source>can&apos;t specify --subaddress-lookahead and --wallet-file at the same time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3121"/>
        <source>Warning: using an untrusted daemon at %s, privacy will be lessened</source>
        <translation>Varning: använder en ej betrodd daemon på %s; sekretessen kommer att vara mindre</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3181"/>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or change the daemon address using the &apos;set_daemon&apos; command.</source>
        <translation>Antingen har daemonen inte startat eller så angavs fel port. Se till att daemonen körs eller byt daemonadress med kommandot &apos;set_daemon&apos;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3208"/>
        <source>If your display freezes, exit blind with ^C, then run again with --use-english-language-names</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3323"/>
        <source>Your wallet has been generated!
To start synchronizing with the daemon, use the &quot;refresh&quot; command.
Use the &quot;help&quot; command to see the list of available commands.
Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
Always use the &quot;exit&quot; command when closing monero-wallet-cli to save 
your current session&apos;s state. Otherwise, you might need to synchronize 
your wallet again (your wallet keys are NOT at risk in any case).
</source>
        <translation>Din plånbok har skapats!
Använd kommandot &quot;refresh&quot; för att starta synkronisering med daemonen.
Använd kommandot &quot;help&quot; för att visa en lista över tillgängliga kommandon.
Använd &quot;help &lt;kommando&gt;&quot; för att visa dokumentation för kommandot.
Använd alltid kommandot &quot;exit&quot; när du stänger monero-wallet-cli så att ditt aktuella sessionstillstånd sparas. Annars kan du bli tvungen att synkronisera
din plånbok igen (din plånboks nycklar är dock INTE hotade i vilket fall som helst).
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3459"/>
        <source>failed to generate new mutlisig wallet</source>
        <translation>det gick inte att skapa ny multisig-plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3462"/>
        <source>Generated new %u/%u multisig wallet: </source>
        <translation>Skapa ny %u/%u-multisig-plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3498"/>
        <source>Opened %u/%u multisig wallet%s</source>
        <translation>Öppnade %u/%u-multisig-plånbok%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3554"/>
        <source>Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
</source>
        <translation>Använd &quot;help &lt;kommando&gt;&quot; för att visa dokumentation för kommandot.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3612"/>
        <source>wallet is multisig and cannot save a watch-only version</source>
        <translation>plånboken är multisig och kan inte spara en granskningsversion</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3628"/>
        <source>Watch only wallet saved as: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3632"/>
        <source>Failed to save watch only wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3728"/>
        <source>missing daemon URL argument</source>
        <translation>argument för URL till daemon saknas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3739"/>
        <source>Unexpected array length - Exited simple_wallet::set_daemon()</source>
        <translation>Oväntad matrislängd - Lämnade simple_wallet::set_daemon()</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3753"/>
        <source>This does not seem to be a valid daemon URL.</source>
        <translation>Detta verkar inte vara en giltig daemon-URL.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3789"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3807"/>
        <source>txid </source>
        <translation>txid </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3791"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3809"/>
        <source>idx </source>
        <translation>idx </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3914"/>
        <source> (Some owned outputs have partial key images - import_multisig_info needed)</source>
        <translation> (Några ägda utgångar har partiella nyckelavbildningar - import_multisig_info krävs)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3915"/>
        <source>Currently selected account: [</source>
        <translation>Aktuellt valt konto: [</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3915"/>
        <source>] </source>
        <translation>] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3917"/>
        <source>Tag: </source>
        <translation>Tagg: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3917"/>
        <source>(No tag assigned)</source>
        <translation>(Ingen tagg tilldelad)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3924"/>
        <source>Balance per address:</source>
        <translation>Saldo per adress:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3925"/>
        <source>Address</source>
        <translation>Adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3925"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6593"/>
        <source>Balance</source>
        <translation>Saldo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3925"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6593"/>
        <source>Unlocked balance</source>
        <translation>Upplåst saldo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3925"/>
        <source>Outputs</source>
        <translation>Utgångar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3925"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6593"/>
        <source>Label</source>
        <translation>Etikett</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3933"/>
        <source>%8u %6s %21s %21s %7u %21s</source>
        <translation>%8u %6s %21s %21s %7u %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3942"/>
        <source>usage: balance [detail]</source>
        <translation>användning: balance [detail]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3954"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3996"/>
        <source>usage: incoming_transfers [available|unavailable] [verbose] [index=&lt;N&gt;]</source>
        <translation>användning: incoming_transfers [available|unavailable] [verbose] [index=&lt;N&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <source>spent</source>
        <translation>spenderat</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <source>global index</source>
        <translation>globalt index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <source>tx id</source>
        <translation>tx-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4066"/>
        <source>addr index</source>
        <translation>addr index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4038"/>
        <source>No incoming transfers</source>
        <translation>Inga inkommande överföringar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4042"/>
        <source>No incoming available transfers</source>
        <translation>Inga inkommande tillgängliga överföringar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4046"/>
        <source>No incoming unavailable transfers</source>
        <translation>Inga inkommande otillgängliga överföringar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4057"/>
        <source>expected at least one payment ID</source>
        <translation>åtminstone ett betalnings-ID förväntades</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4066"/>
        <source>payment</source>
        <translation>betalning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4066"/>
        <source>transaction</source>
        <translation>transaktion</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4066"/>
        <source>height</source>
        <translation>höjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4066"/>
        <source>unlock time</source>
        <translation>upplåsningstid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4078"/>
        <source>No payments with id </source>
        <translation>Inga betalningar med ID </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4131"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4197"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4479"/>
        <source>failed to get blockchain height: </source>
        <translation>det gick inte att hämta blockkedjans höjd: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4187"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5804"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5842"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5899"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5932"/>
        <source>failed to connect to the daemon</source>
        <translation>det gick inte att ansluta till daemonen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4205"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation>
Transaktion %llu/%llu: txid=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4221"/>
        <source>failed to find construction data for tx input</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4226"/>
        <source>
Input %llu/%llu: amount=%s</source>
        <translation>
Ingång %llu/%llu: belopp=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4242"/>
        <source>failed to get output: </source>
        <translation>det gick inte att hämta utgång: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4250"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation>utgångsnyckelns ursprungsblockhöjd får inte vara högre än blockkedjans höjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4254"/>
        <source>
Originating block heights: </source>
        <translation>
Ursprungsblockhöjder: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4269"/>
        <source>
|</source>
        <translation>
|</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4269"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6324"/>
        <source>|
</source>
        <translation>|
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4286"/>
        <source>
Warning: Some input keys being spent are from </source>
        <translation>
Varning: Några ingångsnycklar som spenderas kommer från </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4288"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation>, vilket kan bryta ringsignaturens anonymitet. Se till att detta är avsiktligt!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4331"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4827"/>
        <source>Ring size must not be 0</source>
        <translation>Ringstorlek för inte vara 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4343"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4839"/>
        <source>ring size %u is too small, minimum is %u</source>
        <translation>ringstorlek %uär för liten, minimum är %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4350"/>
        <source>wrong number of arguments</source>
        <translation>fel antal argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4456"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4909"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5122"/>
        <source>No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): </source>
        <translation>Inget betalnings-ID har inkluderats med denna transaktion. Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4498"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4929"/>
        <source>No outputs found, or daemon is not ready</source>
        <translation>Inga utgångar hittades, eller så är daemonen inte klar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5244"/>
        <source>donations are not enabled on the testnet or on the stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6972"/>
        <source>Network type: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6973"/>
        <source>Testnet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6974"/>
        <source>Stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6974"/>
        <source>Mainnet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7446"/>
        <source>Transaction successfully saved to </source>
        <translation>Transaktionen sparades till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7446"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7448"/>
        <source>, txid </source>
        <translation>, txid </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7448"/>
        <source>Failed to save transaction to </source>
        <translation>Det gick inte att spara transaktion till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4724"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4957"/>
        <source>Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Sveper upp %s i %llu transaktioner för en total avgift på %s.  Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4730"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4963"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5162"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Sveper upp %s för en total avgift på %s.  Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5273"/>
        <source>Donating </source>
        <translation>Donerar </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5440"/>
        <source>This is a watch only wallet</source>
        <translation>Detta är en granskningsplånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7274"/>
        <source>usage: show_transfer &lt;txid&gt;</source>
        <translation>användning: show_transfer &lt;txid&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7376"/>
        <source>Double spend seen on the network: this transaction may or may not end up being mined</source>
        <translation>En dubbelspendering upptäcktes på nätverket: denna transaktion kanske aldrig blir verifierad</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7411"/>
        <source>Transaction ID not found</source>
        <translation>Transaktions-ID kunde inte hittas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="248"/>
        <source>true</source>
        <translation>sant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="301"/>
        <source>failed to parse refresh type</source>
        <translation>det gick inte att parsa uppdateringstyp</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="619"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="688"/>
        <source>wallet is watch-only and has no seed</source>
        <translation>plånboken är enbart för granskning och har inget startvärde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="635"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="693"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation>plånboken är icke-deterministisk och har inget startvärde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1617"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1636"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation>plånboken är enbart för granskning och kan inte göra överföringar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1715"/>
        <source>could not change default priority</source>
        <translation>det gick inte att ändra standardinställning för prioritet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2105"/>
        <source>If no arguments are specified or &lt;index&gt; is specified, the wallet shows the default or specified address. If &quot;all&quot; is specified, the wallet shows all the existing addresses in the currently selected account. If &quot;new &quot; is specified, the wallet creates a new address with the provided label text (which can be empty). If &quot;label&quot; is specified, the wallet sets the label of the address specified by &lt;index&gt; to the provided label text.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2132"/>
        <source>Available options:
 seed language
   Set the wallet&apos;s seed language.
 always-confirm-transfers &lt;1|0&gt;
   Whether to confirm unsplit txes.
 print-ring-members &lt;1|0&gt;
   Whether to print detailed information about ring members during confirmation.
 store-tx-info &lt;1|0&gt;
   Whether to store outgoing tx info (destination address, payment ID, tx secret key) for future reference.
 default-ring-size &lt;n&gt;
   Set the default ring size (default and minimum is 5).
 auto-refresh &lt;1|0&gt;
   Whether to automatically synchronize new blocks from the daemon.
 refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt;
   Set the wallet&apos;s refresh behaviour.
 priority [0|1|2|3|4]
   Set the fee too default/unimportant/normal/elevated/priority.
 confirm-missing-payment-id &lt;1|0&gt;
 ask-password &lt;1|0&gt;
 unit &lt;monero|millinero|micronero|nanonero|piconero&gt;
   Set the default monero (sub-)unit.
 min-outputs-count [n]
   Try to keep at least that many outputs of value at least min-outputs-value.
 min-outputs-value [n]
   Try to keep at least min-outputs-count outputs of at least that value.
 merge-destinations &lt;1|0&gt;
   Whether to merge multiple payments to the same destination address.
 confirm-backlog &lt;1|0&gt;
   Whether to warn if there is transaction backlog.
 confirm-backlog-threshold [n]
   Set a threshold for confirm-backlog to only warn if the transaction backlog is greater than n blocks.
 refresh-from-block-height [n]
   Set the height before which to ignore blocks.
 auto-low-priority &lt;1|0&gt;
   Whether to automatically use the low priority fee level when it&apos;s safe to do so.
 segregate-pre-fork-outputs &lt;1|0&gt;
   Set this if you intend to spend outputs on both Monero AND a key reusing fork.
 key-reuse-mitigation2 &lt;1|0&gt;
   Set this if you are not sure whether you will spend on a key reusing Monero fork later.
subaddress-lookahead &lt;major&gt;:&lt;minor&gt;
   Set the lookahead sizes for the subaddress hash table.
   Set this if you are not sure whether you will spend on a key reusing Monero fork later.
 segregation-height &lt;n&gt;
   Set to the height of a key reusing fork you want to use, 0 to use default.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2412"/>
        <source>full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)</source>
        <translation>full (långsammast, inga antaganden); optimize-coinbase (snabb, antar att hela coinbase-transaktionen betalas till en enda adress); no-coinbase (snabbast, antar att ingen coinbase-transaktion tas emot), default (samma som optimize-coinbase)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2416"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation>monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2472"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation>Plånbokens namn ej giltigt. Försök igen eller använd Ctrl-C för att avsluta.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2489"/>
        <source>Wallet and key files found, loading...</source>
        <translation>Plånbok och nyckelfil hittades, läser in …</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2495"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation>Nyckelfilen hittades men inte plånboksfilen. Återskapar …</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2501"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation>Nyckelfilen kunde inte hittas. Det gick inte att öppna plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2520"/>
        <source>Generating new wallet...</source>
        <translation>Skapar ny plånbok …</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2562"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2574"/>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-device=&quot;wallet_name&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2650"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Det gick inte att verifiera ordlista av Electrum-typ</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2680"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2700"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2735"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2754"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2774"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2790"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2838"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2863"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2879"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2919"/>
        <source>No data supplied, cancelled</source>
        <translation>Inga data angivna, avbryter</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2686"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2760"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2869"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4417"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4883"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5097"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5589"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5657"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5721"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5939"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6778"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7036"/>
        <source>failed to parse address</source>
        <translation>det gick inte att parsa adressen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2706"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2796"/>
        <source>failed to parse view key secret key</source>
        <translation>det gick inte att parsa hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2716"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2814"/>
        <source>failed to verify view key secret key</source>
        <translation>det gick inte att verifiera hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2720"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2818"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2899"/>
        <source>view key does not match standard address</source>
        <translation>granskningsnyckel matchar inte standardadress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2725"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2744"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2822"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2956"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2979"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3009"/>
        <source>account creation failed</source>
        <translation>det gick inte att skapa konto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2740"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2780"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2924"/>
        <source>failed to parse spend key secret key</source>
        <translation>det gick inte att parsa spendernyckel hemlig nyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2806"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2945"/>
        <source>failed to verify spend key secret key</source>
        <translation>det gick inte att verifiera spendernyckel hemlig nyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2810"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2950"/>
        <source>spend key does not match standard address</source>
        <translation>spendernyckel matchar inte standardadress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2984"/>
        <source>No restore height is specified.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2985"/>
        <source>Assumed you are creating a new account, restore will be done from current estimated blockchain height.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2986"/>
        <source>Use --restore-height if you want to restore an already setup account from a specific height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2990"/>
        <source>account creation aborted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3101"/>
        <source>failed to open account</source>
        <translation>det gick inte att öppna konto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3105"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3652"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3707"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3765"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5625"/>
        <source>wallet is null</source>
        <translation>plånbok är null</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3124"/>
        <source>Failed to initialize ring database: privacy enhancing features will be inactive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3226"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3231"/>
        <source>invalid language choice entered. Please try again.
</source>
        <translation>ogiltigt språkval har angivits. Försök igen.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3308"/>
        <source>View key: </source>
        <translation>Granskningsnyckel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3410"/>
        <source>Generated new wallet on hw device: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3547"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation>Du kan också prova att bort filen &quot;%s&quot; och försöka igen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3575"/>
        <source>failed to deinitialize wallet</source>
        <translation>det gick inte att avinitiera plånboken</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3643"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4139"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7103"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation>detta kommando kräver en betrodd daemon. Aktivera med --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3775"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation>blockkedjan kan inte sparas: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3854"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4153"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>daemonen är upptagen. Försök igen senare.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3858"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4157"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>ingen anslutning till daemonen. Se till att daemonen körs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3868"/>
        <source>refresh error: </source>
        <translation>fel vid uppdatering: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3918"/>
        <source>Balance: </source>
        <translation>Saldo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4014"/>
        <source>pubkey</source>
        <translation>publik nyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4014"/>
        <source>key image</source>
        <translation>nyckelavbildning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4025"/>
        <source>unlocked</source>
        <translation>upplåst</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <source>ringct</source>
        <translation>ringct</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4024"/>
        <source>T</source>
        <translation>S</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4024"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4025"/>
        <source>locked</source>
        <translation>låst</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4026"/>
        <source>RingCT</source>
        <translation>RingCT</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4026"/>
        <source>-</source>
        <translation>-</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4100"/>
        <source>payment ID has invalid format, expected 16 or 64 character hex string: </source>
        <translation>betalnings-ID har ogiltigt format. En hexadecimal sträng med 16 eller 64 tecken förväntades: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4161"/>
        <source>failed to get spent status</source>
        <translation>det gick inte att hämta spenderstatus</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4287"/>
        <source>the same transaction</source>
        <translation>samma transaktion</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4287"/>
        <source>blocks that are temporally very close</source>
        <translation>block som ligger väldigt nära varandra i tiden</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4404"/>
        <source>Locked blocks too high, max 1000000 (˜4 yrs)</source>
        <translation>Låsta block för högt, max 1000000 (˜~4 år)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5740"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5856"/>
        <source>Good signature</source>
        <translation>Godkänd signatur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5767"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5858"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5966"/>
        <source>Bad signature</source>
        <translation>Felaktig signatur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6718"/>
        <source>usage: integrated_address [payment ID]</source>
        <translation>användning: integrated_address [betalnings-ID]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6754"/>
        <source>Standard address: </source>
        <translation>Standardadress: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6759"/>
        <source>failed to parse payment ID or address</source>
        <translation>det gick inte att parsa betalnings-ID eller adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6770"/>
        <source>usage: address_book [(add (&lt;address&gt; [pid &lt;long or short payment id&gt;])|&lt;integrated address&gt; [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</source>
        <translation>användning: address_book [(add (&lt;adress&gt; [pid &lt;långt eller kort betalnings-ID&gt;])|&lt;integrerad adress&gt; [&lt;beskrivning eventuellt med blanktecken&gt;])|(delete &lt;index&gt;)]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6800"/>
        <source>failed to parse payment ID</source>
        <translation>det gick inte att parsa betalnings-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6818"/>
        <source>failed to parse index</source>
        <translation>det gick inte att parsa index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6826"/>
        <source>Address book is empty.</source>
        <translation>Adressboken är tom.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6832"/>
        <source>Index: </source>
        <translation>Index: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6833"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6963"/>
        <source>Address: </source>
        <translation>Adress: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6834"/>
        <source>Payment ID: </source>
        <translation>Betalnings-ID: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6835"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6962"/>
        <source>Description: </source>
        <translation>Beskrivning: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6845"/>
        <source>usage: set_tx_note [txid] free text note</source>
        <translation>användning: set_tx_note [txid] fritextanteckning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6873"/>
        <source>usage: get_tx_note [txid]</source>
        <translation>användning: get_tx_note [txid]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6987"/>
        <source>usage: sign &lt;filename&gt;</source>
        <translation>användning: sign &lt;filnamn&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6992"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation>plånboken är enbart för granskning och kan inte signera</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1058"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7006"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7029"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7204"/>
        <source>failed to read file </source>
        <translation>det gick inte att läsa filen </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5702"/>
        <source>usage: check_tx_proof &lt;txid&gt; &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>användning: check_tx_proof &lt;txid&gt; &lt;adress&gt; &lt;signaturfil&gt; [&lt;meddelande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5729"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5849"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5951"/>
        <source>failed to load signature file</source>
        <translation>det gick inte att läsa in signaturfil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5785"/>
        <source>usage: get_spend_proof &lt;txid&gt; [&lt;message&gt;]</source>
        <translation>användning: get_spend_proof &lt;txid&gt; [&lt;meddelande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5791"/>
        <source>wallet is watch-only and cannot generate the proof</source>
        <translation>plånboken är enbart för granskning och kan inte skapa beviset</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5829"/>
        <source>usage: check_spend_proof &lt;txid&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>användning: check_spend_proof &lt;txid&gt; &lt;signaturfil&gt; [&lt;meddelande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5875"/>
        <source>usage: get_reserve_proof (all|&lt;amount&gt;) [&lt;message&gt;]</source>
        <translation>användning: get_reserve_proof (all|&lt;belopp&gt;) [&lt;meddelande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5881"/>
        <source>The reserve proof can be generated only by a full wallet</source>
        <translation>Beviset på reserv kan endast skapas av en standardplånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5926"/>
        <source>usage: check_reserve_proof &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>användning: check_reserve_proof &lt;adress&gt; &lt;signaturfil&gt; [&lt;meddelande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5944"/>
        <source>Address must not be a subaddress</source>
        <translation>Adressen får inte vara en underadress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5962"/>
        <source>Good signature -- total: %s, spent: %s, unspent: %s</source>
        <translation>Godkänd signatur -- summa: %s, spenderat: %s, ej spenderat: %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6026"/>
        <source>usage: show_transfers [in|out|all|pending|failed] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation>användning: show_transfers [in|out|all|pending|failed] [index=&lt;N1&gt;[, &lt;N2&gt;, …]] [&lt;minhöjd&gt; [&lt;maxhöjd&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6163"/>
        <source>[Double spend seen on the network: this transaction may or may not end up being mined] </source>
        <translation>[En dubbelspendering upptäcktes på nätverket: denna transaktion kanske aldrig blir verifierad] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6199"/>
        <source>usage: unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_amount&gt; [&lt;max_amount&gt;]]</source>
        <translation>användning: unspent_outputs [index=&lt;N1&gt;[, &lt;N2&gt;, …]] [&lt;min_belopp&gt; [&lt;max_belopp&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6259"/>
        <source>There is no unspent output in the specified address</source>
        <translation>Det finns ingen ej spenderad utgång i den angivna adressen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6371"/>
        <source> (no daemon)</source>
        <translation> (ingen daemon)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6373"/>
        <source> (out of sync)</source>
        <translation> (inte synkroniserad)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6430"/>
        <source>(Untitled account)</source>
        <translation>(Ej namngivet konto)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6443"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6461"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6486"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6509"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6662"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6685"/>
        <source>failed to parse index: </source>
        <translation>det gick inte att parsa index: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6448"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6667"/>
        <source>specify an index between 0 and </source>
        <translation>ange ett index mellan 0 och </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6545"/>
        <source>usage:
  account
  account new &lt;label text with white spaces allowed&gt;
  account switch &lt;index&gt;
  account label &lt;index&gt; &lt;label text with white spaces allowed&gt;
  account tag &lt;tag_name&gt; &lt;account_index_1&gt; [&lt;account_index_2&gt; ...]
  account untag &lt;account_index_1&gt; [&lt;account_index_2&gt; ...]
  account tag_description &lt;tag_name&gt; &lt;description&gt;</source>
        <translation>användning:
  account
  account new &lt;etikettext med blanktecken tillåtna&gt;
  account switch &lt;index&gt;
  account label &lt;index&gt; &lt;etikettext med blanktecken tillåtna&gt;
  account tag &lt;taggnamn&gt; &lt;kontoindex_1&gt; [&lt;kontoindex_2&gt; …]
  account untag &lt;kontoindex_1&gt; [&lt;kontoindex_2&gt; …]
  account tag_description &lt;taggnamn&gt; &lt;beskrivning&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6573"/>
        <source>
Grand total:
  Balance: </source>
        <translation>
Totalsumma:
  Saldo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6573"/>
        <source>, unlocked balance: </source>
        <translation>, upplåst saldo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6581"/>
        <source>Untagged accounts:</source>
        <translation>Otaggade konton:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6587"/>
        <source>Tag %s is unregistered.</source>
        <translation>Taggen %s har inte registrerats.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6590"/>
        <source>Accounts with tag: </source>
        <translation>Konton med tagg: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6591"/>
        <source>Tag&apos;s description: </source>
        <translation>Taggens beskrivning: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6593"/>
        <source>Account</source>
        <translation>Konto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6599"/>
        <source> %c%8u %6s %21s %21s %21s</source>
        <translation> %c%8u %6s %21s %21s %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6609"/>
        <source>----------------------------------------------------------------------------------</source>
        <translation>----------------------------------------------------------------------------------</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6610"/>
        <source>%15s %21s %21s</source>
        <translation>%15s %21s %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6633"/>
        <source>Primary address</source>
        <translation>Primär adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6633"/>
        <source>(used)</source>
        <translation>(används)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6654"/>
        <source>(Untitled address)</source>
        <translation>(Ej namngiven adress)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6694"/>
        <source>&lt;index_min&gt; is already out of bound</source>
        <translation>&lt;index_min&gt; är redan utanför tillåtet intervall</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6699"/>
        <source>&lt;index_max&gt; exceeds the bound</source>
        <translation>&lt;index_max&gt; är utanför tillåtet intervall</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6707"/>
        <source>usage: address [ new &lt;label text with white spaces allowed&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;label text with white spaces allowed&gt; ]</source>
        <translation>användning: address [new &lt;etikettext med blanktecken tillåtna&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;etikettext med blanktecken tillåtna&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6725"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6737"/>
        <source>Integrated addresses can only be created for account 0</source>
        <translation>Integrerade adresser kan bara skapas för konto 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6749"/>
        <source>Integrated address: %s, payment ID: %s</source>
        <translation>Integrerad adress: %s, betalnings-ID: %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6754"/>
        <source>Subaddress: </source>
        <translation>Underadress: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6914"/>
        <source>usage: get_description</source>
        <translation>användning: get_description</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6920"/>
        <source>no description found</source>
        <translation>ingen beskrivning hittades</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6922"/>
        <source>description found: </source>
        <translation>beskrivning hittades: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6961"/>
        <source>Filename: </source>
        <translation>Filnamn: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6966"/>
        <source>Watch only</source>
        <translation>Endast granskning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6968"/>
        <source>%u/%u multisig%s</source>
        <translation>%u/%u multisig%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6970"/>
        <source>Normal</source>
        <translation>Normal</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6971"/>
        <source>Type: </source>
        <translation>Typ: </translation>
    </message>
    <message>
        <source>Testnet: </source>
        <translation type="vanished">Testnet: </translation>
    </message>
    <message>
        <source>Yes</source>
        <translation type="vanished">Ja</translation>
    </message>
    <message>
        <source>No</source>
        <translation type="vanished">Nej</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6997"/>
        <source>This wallet is multisig and cannot sign</source>
        <translation>Plånboken är multisig och kan inte signera</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7018"/>
        <source>usage: verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation>användning: verify &lt;filnamn&gt; &lt;adress&gt; &lt;signatur&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7043"/>
        <source>Bad signature from </source>
        <translation>Felaktig signatur från </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7047"/>
        <source>Good signature from </source>
        <translation>Godkänd signatur från </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7061"/>
        <source>usage: export_key_images &lt;filename&gt;</source>
        <translation>användning: export_key_images &lt;filnamn&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7066"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation>plånboken är enbart för granskning och kan inte exportera nyckelavbildningar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1008"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7079"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7171"/>
        <source>failed to save file </source>
        <translation>det gick inte att spara fil </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7090"/>
        <source>Signed key images exported to </source>
        <translation>Signerade nyckelavbildningar exporterades till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7109"/>
        <source>usage: import_key_images &lt;filename&gt;</source>
        <translation>användning: import_key_images &lt;filnamn&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7145"/>
        <source>usage: export_outputs &lt;filename&gt;</source>
        <translation>användning: export_outputs &lt;filnamn&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7182"/>
        <source>Outputs exported to </source>
        <translation>Utgångar exporterades till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7195"/>
        <source>usage: import_outputs &lt;filename&gt;</source>
        <translation>användning: import_outputs &lt;filnamn&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4445"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5892"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6218"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6226"/>
        <source>amount is wrong: </source>
        <translation>beloppet är fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4446"/>
        <source>expected number from 0 to </source>
        <translation>förväntades: ett tal från 0 till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4722"/>
        <source>Sweeping </source>
        <translation>Sveper upp </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5202"/>
        <source>Money successfully sent, transaction: </source>
        <translation>Pengar skickades, transaktion: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5359"/>
        <source>Change goes to more than one address</source>
        <translation>Växel går till fler än en adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5400"/>
        <source>%s change to %s</source>
        <translation>%s växel till %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5403"/>
        <source>no change</source>
        <translation>ingen växel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1156"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1169"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5474"/>
        <source>Transaction successfully signed to file </source>
        <translation>Transaktionen signerades till fil </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5534"/>
        <source>usage: get_tx_key &lt;txid&gt;</source>
        <translation>användning: get_tx_key &lt;txid&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5542"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5582"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5631"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5713"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5798"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5836"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6852"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6880"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7281"/>
        <source>failed to parse txid</source>
        <translation>det gick inte att parsa txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5556"/>
        <source>Tx key: </source>
        <translation>Tx-nyckel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5561"/>
        <source>no tx keys found for this txid</source>
        <translation>inga tx-nycklar kunde hittas för detta txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5575"/>
        <source>usage: get_tx_proof &lt;txid&gt; &lt;address&gt; [&lt;message&gt;]</source>
        <translation>användning: get_tx_proof &lt;txid&gt; &lt;adress&gt; [&lt;meddelande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5600"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5815"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5912"/>
        <source>signature file saved to: </source>
        <translation>signaturfilen sparades till: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5602"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5817"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5914"/>
        <source>failed to save signature file</source>
        <translation>det gick inte att spara signaturfilen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5616"/>
        <source>usage: check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</source>
        <translation>användning: check_tx_key &lt;txid&gt; &lt;txnyckel&gt; &lt;adress&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5639"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5648"/>
        <source>failed to parse tx key</source>
        <translation>det gick inte att parsa txnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5606"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5694"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5772"/>
        <source>error: </source>
        <translation>fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5670"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5743"/>
        <source>received</source>
        <translation>mottaget</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5670"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5743"/>
        <source>in txid</source>
        <translation>i txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5689"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5762"/>
        <source>received nothing in txid</source>
        <translation>tog emot ingenting i txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5673"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5746"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation>VARNING: denna transaktion är ännu inte inkluderad i blockkedjan!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5679"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5752"/>
        <source>This transaction has %u confirmations</source>
        <translation>Denna transaktion har %u bekräftelser</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5683"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5756"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation>VARNING: det gick inte att bestämma antal bekräftelser!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6074"/>
        <source>bad min_height parameter:</source>
        <translation>felaktig parameter för min_höjd:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6086"/>
        <source>bad max_height parameter:</source>
        <translation>felaktig parameter för max_höjd:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6146"/>
        <source>in</source>
        <translation>in</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6146"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6187"/>
        <source>out</source>
        <translation>ut</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6187"/>
        <source>failed</source>
        <translation>misslyckades</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6187"/>
        <source>pending</source>
        <translation>väntande</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6233"/>
        <source>&lt;min_amount&gt; should be smaller than &lt;max_amount&gt;</source>
        <translation>&lt;min_belopp&gt; måste vara mindre än &lt;max_belopp&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6265"/>
        <source>
Amount: </source>
        <translation>
Belopp: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6265"/>
        <source>, number of keys: </source>
        <translation>, antal nycklar: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6270"/>
        <source> </source>
        <translation></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6275"/>
        <source>
Min block height: </source>
        <translation>
Minblockhöjd: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6276"/>
        <source>
Max block height: </source>
        <translation>
Maxblockhöjd: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6277"/>
        <source>
Min amount found: </source>
        <translation>
Minbelopp funnet: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6278"/>
        <source>
Max amount found: </source>
        <translation>
Maxbelopp funnet: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6279"/>
        <source>
Total count: </source>
        <translation>
Totalt antal: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6319"/>
        <source>
Bin size: </source>
        <translation>
Storlek för binge: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6320"/>
        <source>
Outputs per *: </source>
        <translation>
Utgångar per *: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6322"/>
        <source>count
  ^
</source>
        <translation>antal
  ^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6324"/>
        <source>  |</source>
        <translation>  |</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6326"/>
        <source>  +</source>
        <translation>  +</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6326"/>
        <source>+--&gt; block height
</source>
        <translation>+--&gt; blockhöjd
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6327"/>
        <source>   ^</source>
        <translation>   ^</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6327"/>
        <source>^
</source>
        <translation>^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6328"/>
        <source>  </source>
        <translation></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6369"/>
        <source>wallet</source>
        <translation>plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="748"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6729"/>
        <source>Random payment ID: </source>
        <translation>Slumpmässigt betalnings-ID: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6730"/>
        <source>Matching integrated address: </source>
        <translation>Matchande integrerad adress: </translation>
    </message>
</context>
<context>
    <name>genms</name>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="70"/>
        <source>Base filename (-1, -2, etc suffixes will be appended as needed)</source>
        <translation>Basfilnamn (suffix -1, -2, osv. läggs till efter behov)</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="71"/>
        <source>Give threshold and participants at once as M/N</source>
        <translation>Ange tröskelvärde och deltagare på en gång som M/N</translation>
    </message>
    <message>
        <source>How many participants wil share parts of the multisig wallet</source>
        <translation type="vanished">Hur många deltagare kommer att dela delar av multisig-plånboken</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="72"/>
        <source>How many participants will share parts of the multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="73"/>
        <source>How many signers are required to sign a valid transaction</source>
        <translation>Hur många signerare krävs för att signera en giltig transaktion</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="74"/>
        <source>Create testnet multisig wallets</source>
        <translation>Skapa multisig-plånböcker för testnet</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="75"/>
        <source>Create stagenet multisig wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="76"/>
        <source>Create an address file for new wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="83"/>
        <source>Generating %u %u/%u multisig wallets</source>
        <translation>Skapar %u %u/%u multisig-plånböcker</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="140"/>
        <source>Error verifying multisig extra info</source>
        <translation>Ett fel uppstod när extra multisig-info verifierades</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="148"/>
        <source>Error finalizing multisig</source>
        <translation>Ett fel uppstod vid slutförande av multisig</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="155"/>
        <source>Generated multisig wallets for address </source>
        <translation>Skapade multisig-plånböcker för adress </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="159"/>
        <source>Error creating multisig wallets: </source>
        <translation>Ett fel uppstod när multisig-plånböcker skapades: </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="180"/>
        <source>This program generates a set of multisig wallets - use this simpler scheme only if all the participants trust each other</source>
        <translation>Programmet skapar en uppsättning multisig-plånböcker - använd endast detta enklare system om alla deltagare litar på varandra</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="197"/>
        <source>Error: Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="204"/>
        <source>Error: expected N/M, but got: </source>
        <translation>Fel: förväntade N/M, men fick: </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="212"/>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="221"/>
        <source>Error: either --scheme or both of --threshold and --participants may be given</source>
        <translation>Fel: antingen --scheme eller både --threshold och --participants får anges</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="228"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got N==%u and M==%d</source>
        <translation>Fel: förväntade N &gt; 1 och N &lt;= M, men fick N=%u och M=%d</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="237"/>
        <source>Error: --filename-base is required</source>
        <translation>Fel: --filename-base måste anges</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="243"/>
        <source>Error: unsupported scheme: only N/N and N-1/N are supported</source>
        <translation>Fel: systemet stöds inte: bara N/N och N-1/N stöds</translation>
    </message>
</context>
<context>
    <name>sw</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="120"/>
        <source>Generate new wallet and save it to &lt;arg&gt;</source>
        <translation>Skapa ny plånbok och spara den till &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="121"/>
        <source>Generate new wallet from device and save it to &lt;arg&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="122"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>Skapa granskningsplånbok från granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="123"/>
        <source>Generate deterministic wallet from spend key</source>
        <translation>Skapa deterministisk plånbok från spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="124"/>
        <source>Generate wallet from private keys</source>
        <translation>Skapa plånbok från privata nycklar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="125"/>
        <source>Generate a master wallet from multisig wallet keys</source>
        <translation>Skapa en huvudplånbok från multisig-plånboksnycklar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="127"/>
        <source>Language for mnemonic</source>
        <translation>Språk för minnesbaserat startvärde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="128"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>Ange Electrum-startvärde för att återställa/skapa plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="129"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Återställ plånbok genom att använda minnesbaserat startvärde (Electrum-typ)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="130"/>
        <source>Recover multisig wallet using Electrum-style mnemonic seed</source>
        <translation>Återställ multisig-plånbok genom att använda minnesbaserat startvärde (Electrum-typ)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="131"/>
        <source>Generate non-deterministic view and spend keys</source>
        <translation>Skapa icke-deterministisk granskningsnyckel och spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="132"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation>Aktivera kommandon som kräver en betrodd daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="133"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>Tillåt kommunikation med en daemon som använder en annan version av RPC</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="134"/>
        <source>Restore from specific blockchain height</source>
        <translation>Återställ från angiven blockkedjehöjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="135"/>
        <source>The newly created transaction will not be relayed to the monero network</source>
        <translation>Den nyss skapade transaktionen kommer inte att skickas vidare till monero-nätverket</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="136"/>
        <source>Create an address file for new wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="138"/>
        <source>Display English language names</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="205"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>daemonen är upptagen. Försök igen senare.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="214"/>
        <source>possibly lost connection to daemon</source>
        <translation>anslutning till daemonen kan ha förlorats</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="231"/>
        <source>Error: </source>
        <translation>Fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7500"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.
WARNING: Do not reuse your Monero keys on an another fork, UNLESS this fork has key reuse mitigations built in. Doing so will harm your privacy.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation type="vanished">Detta är kommandoradsplånboken för Monero. Den måste ansluta till en Monero-
daemon för att fungera korrekt.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7514"/>
        <source>Failed to initialize wallet</source>
        <translation>Det gick inte att initiera plånbok</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="135"/>
        <source>Use daemon instance at &lt;host&gt;:&lt;port&gt;</source>
        <translation>Använd daemonen på &lt;värddator&gt;:&lt;port&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="136"/>
        <source>Use daemon instance at host &lt;arg&gt; instead of localhost</source>
        <translation>Använd daemonen på värddatorn &lt;arg&gt; istället för localhost</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="138"/>
        <source>Wallet password file</source>
        <translation>Lösenordsfil för plånboken</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="139"/>
        <source>Use daemon instance at port &lt;arg&gt; instead of 18081</source>
        <translation>Använd daemonen på port &lt;arg&gt; istället för 18081</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="141"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>För testnet. Daemonen måste också startas med flaggan --testnet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="142"/>
        <source>For stagenet. Daemon must also be launched with --stagenet flag</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="143"/>
        <source>Restricts to view-only commands</source>
        <translation>Begränsar till granskningskommandon</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="145"/>
        <source>Set shared ring database path</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="202"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>det går inte ange värd eller port för daemonen mer än en gång</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="240"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>det går inte att ange fler än en av --password och --password-file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="253"/>
        <source>the password file specified could not be read</source>
        <translation>det gick inte att läsa angiven lösenordsfil</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="278"/>
        <source>Failed to load file </source>
        <translation>Det gick inte att läsa in fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="360"/>
        <source>At least one of either an Electrum-style word list, private view key, or private spend key must be specified</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="137"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation>Lösenord för plånboken (använd escape-sekvenser eller citattecken efter behov)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="140"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>Ange användarnamn[:lösenord] för RPC-klient till daemonen</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="260"/>
        <source>no password specified; use --prompt-for-password to prompt for a password</source>
        <translation>inget lösenord har angivits; använd --prompt-for-password för att fråga efter lösenord</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="284"/>
        <source>Failed to parse JSON</source>
        <translation>Det gick inte att parsa JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="291"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation>Version %u är för ny, vi förstår bara upp till %u</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="307"/>
        <source>failed to parse view key secret key</source>
        <translation>det gick inte att parsa hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="312"/>
        <location filename="../src/wallet/wallet2.cpp" line="380"/>
        <location filename="../src/wallet/wallet2.cpp" line="421"/>
        <source>failed to verify view key secret key</source>
        <translation>det gick inte att verifiera hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="323"/>
        <source>failed to parse spend key secret key</source>
        <translation>det gick inte att parsa spendernyckel hemlig nyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="328"/>
        <location filename="../src/wallet/wallet2.cpp" line="390"/>
        <location filename="../src/wallet/wallet2.cpp" line="446"/>
        <source>failed to verify spend key secret key</source>
        <translation>det gick inte att verifiera spendernyckel hemlig nyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="340"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Det gick inte att verifiera ordlista av Electrum-typ</translation>
    </message>
    <message>
        <source>At least one of Electrum-style word list and private view key and private spend key must be specified</source>
        <translation type="vanished">Åtminstone en av ordlista av Electrum-typ och privat granskningsnyckel och privat spendernyckel måste anges</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="364"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Både ordlista av Electrum-typ och privat nyckel har angivits</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="374"/>
        <source>invalid address</source>
        <translation>ogiltig adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="383"/>
        <source>view key does not match standard address</source>
        <translation>granskningsnyckel matchar inte standardadress</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="393"/>
        <source>spend key does not match standard address</source>
        <translation>spendernyckel matchar inte standardadress</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="401"/>
        <source>Cannot generate deprecated wallets from JSON</source>
        <translation>Det går inte att skapa inaktuella plånböcker från JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="433"/>
        <source>failed to parse address: </source>
        <translation>det gick inte att parsa adressen: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="439"/>
        <source>Address must be specified in order to create watch-only wallet</source>
        <translation>Adress måste anges för att kunna skapa granskningsplånbok</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="454"/>
        <source>failed to generate new wallet: </source>
        <translation>det gick inte att skapa ny plånbok: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="3023"/>
        <location filename="../src/wallet/wallet2.cpp" line="3080"/>
        <location filename="../src/wallet/wallet2.cpp" line="3173"/>
        <location filename="../src/wallet/wallet2.cpp" line="3223"/>
        <location filename="../src/wallet/wallet2.cpp" line="3263"/>
        <location filename="../src/wallet/wallet2.cpp" line="3359"/>
        <location filename="../src/wallet/wallet2.cpp" line="3461"/>
        <location filename="../src/wallet/wallet2.cpp" line="3861"/>
        <location filename="../src/wallet/wallet2.cpp" line="4238"/>
        <source>Primary account</source>
        <translation>Primärt konto</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="8703"/>
        <source>No funds received in this tx.</source>
        <translation>Inga pengar togs emot i denna tx.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="9398"/>
        <source>failed to read file </source>
        <translation>det gick inte att läsa filen </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="137"/>
        <source>Set subaddress lookahead sizes to &lt;major&gt;:&lt;minor&gt;</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>tools::wallet_rpc_server</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="161"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Daemonen är lokal, utgår från att den är betrodd</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="176"/>
        <source>Failed to create directory </source>
        <translation>Det gick inte att skapa mapp </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="178"/>
        <source>Failed to create directory %s: %s</source>
        <translation>Det gick inte att skapa mapp %s: %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="189"/>
        <source>Cannot specify --</source>
        <translation>Det går inte att ange --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="189"/>
        <source> and --</source>
        <translation> och --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="208"/>
        <source>Failed to create file </source>
        <translation>Det gick inte att skapa fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="208"/>
        <source>. Check permissions or remove file</source>
        <translation>. Kontrollera behörigheter eller ta bort filen</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="218"/>
        <source>Error writing to file </source>
        <translation>Ett fel uppstod vid skrivning till fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="221"/>
        <source>RPC username/password is stored in file </source>
        <translation>Användarnamn/lösenord för RPC har sparats i fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="442"/>
        <source>Tag %s is unregistered.</source>
        <translation>Taggen %s har inte registrerats.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2466"/>
        <source>Transaction not possible. Available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>Transaktion är inte möjlig. Endast tillgängligt %s, transaktionsbelopp %s = %s + %s (avgift)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2901"/>
        <source>This is the RPC monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation>Detta är RPC-plånboken för monero. Den måste ansluta till en Monero-
daemon för att fungera korrekt.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2920"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2932"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>Det går inte att ange fler än en av --wallet-file och --generate-from-json</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2944"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>Måste ange --wallet-file eller --generate-from-json eller --wallet-dir</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2948"/>
        <source>Loading wallet...</source>
        <translation>Läser in plånbok …</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2981"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3014"/>
        <source>Saving wallet...</source>
        <translation>Sparar plånbok …</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2983"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3016"/>
        <source>Successfully saved</source>
        <translation>Plånboken sparades</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2986"/>
        <source>Successfully loaded</source>
        <translation>Plånboken lästes in</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2990"/>
        <source>Wallet initialization failed: </source>
        <translation>Det gick inte att initiera plånbok: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2997"/>
        <source>Failed to initialize wallet RPC server</source>
        <translation>Det gick inte att initiera RPC-servern för plånbok</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3001"/>
        <source>Starting wallet RPC server</source>
        <translation>Startar RPC-server för plånboken</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3008"/>
        <source>Failed to run wallet: </source>
        <translation>Det gick inte att köra plånboken: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3011"/>
        <source>Stopped wallet RPC server</source>
        <translation>Stoppade RPC-server för plånboken</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3020"/>
        <source>Failed to save wallet: </source>
        <translation>Det gick inte spara plånboken: </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7469"/>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="168"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2887"/>
        <source>Wallet options</source>
        <translation>Alternativ för plånbok</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="73"/>
        <source>Generate wallet from JSON format file</source>
        <translation>Skapa plånbok från fil i JSON-format</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="77"/>
        <source>Use wallet &lt;arg&gt;</source>
        <translation>Använd plånbok &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="104"/>
        <source>Max number of threads to use for a parallel job</source>
        <translation>Max antal trådar att använda för ett parallellt jobb</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="105"/>
        <source>Specify log file</source>
        <translation>Ange loggfil</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="106"/>
        <source>Config file</source>
        <translation>Konfigurationsfil</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="115"/>
        <source>General options</source>
        <translation>Allmänna alternativ</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="138"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation>Detta är kommandoradsplånboken för Monero. Den måste ansluta till en Monero-
daemon för att fungera korrekt.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="161"/>
        <source>Can&apos;t find config file </source>
        <translation>Det gick inte att hitta konfigurationsfilen </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="195"/>
        <source>Logging to: </source>
        <translation>Loggar till: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="197"/>
        <source>Logging to %s</source>
        <translation>Loggar till %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="140"/>
        <source>Usage:</source>
        <translation>Användning:</translation>
    </message>
</context>
</TS>
