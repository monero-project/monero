<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.0">
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
        <location filename="../src/wallet/api/pending_transaction.cpp" line="91"/>
        <source>Attempting to save transaction to file, but specified file(s) exist. Exiting to not risk overwriting. File:</source>
        <translation>Försöker spara transaktion till fil, men angiven fil finns redan. Avslutar för att inte riskera att skriva över någonting. Fil:</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="98"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Det gick inte att skriva transaktioner till fil</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="121"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>daemonen är upptagen. Försök igen senare.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="124"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>ingen anslutning till daemonen. Se till att daemonen körs.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="128"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>transaktionen %s avvisades av daemonen med status: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="133"/>
        <source>. Reason: </source>
        <translation>. Orsak: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="135"/>
        <source>Unknown exception: </source>
        <translation>Okänt undantag: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="138"/>
        <source>Unhandled exception</source>
        <translation>Ohanterat undantag</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="211"/>
        <source>Couldn&apos;t multisig sign data: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="233"/>
        <source>Couldn&apos;t sign multisig transaction: </source>
        <translation type="unfinished"></translation>
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
        <location filename="../src/wallet/api/wallet.cpp" line="1383"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>betalnings-ID har ogiltigt format. En hexadecimal sträng med 16 eller 64 tecken förväntades: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1392"/>
        <source>Failed to add short payment id: </source>
        <translation>Det gick inte att lägga till kort betalnings-ID: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1428"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1510"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>daemonen är upptagen. Försök igen senare.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1430"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1512"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>ingen anslutning till daemonen. Se till att daemonen körs.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1432"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1514"/>
        <source>RPC error: </source>
        <translation>RPC-fel: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1460"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1545"/>
        <source>not enough outputs for specified ring size</source>
        <translation>inte tillräckligt med utgångar för angiven ringstorlek</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1462"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1547"/>
        <source>found outputs to use</source>
        <translation>hittade utgångar att använda</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1464"/>
        <source>Please sweep unmixable outputs.</source>
        <translation>Svep upp omixbara utgångar.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1438"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1521"/>
        <source>not enough money to transfer, available only %s, sent amount %s</source>
        <translation>inte tillräckligt med pengar för överföring, endast tillgängligt %s, skickat belopp %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="541"/>
        <source>failed to parse address</source>
        <translation>det gick inte att parsa adressen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="552"/>
        <source>failed to parse secret spend key</source>
        <translation>det gick inte att parsa hemlig spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="575"/>
        <source>failed to parse secret view key</source>
        <translation>det gick inte att parsa hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="584"/>
        <source>failed to verify secret spend key</source>
        <translation>det gick inte att verifiera hemlig spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="588"/>
        <source>spend key does not match address</source>
        <translation>spendernyckel matchar inte adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="594"/>
        <source>failed to verify secret view key</source>
        <translation>det gick inte att verifiera hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="598"/>
        <source>view key does not match address</source>
        <translation>granskningsnyckel matchar inte adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="621"/>
        <location filename="../src/wallet/api/wallet.cpp" line="638"/>
        <source>failed to generate new wallet: </source>
        <translation>det gick inte att skapa ny plånbok: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="885"/>
        <source>Failed to send import wallet request</source>
        <translation>Det gick inte att skicka begäran om att importera plånbok</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1049"/>
        <source>Failed to load unsigned transactions</source>
        <translation>Det gick inte att läsa in osignerade transaktioner</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1068"/>
        <source>Failed to load transaction from file</source>
        <translation>Det gick inte att läsa in transaktion från fil</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1084"/>
        <source>Wallet is view only</source>
        <translation>Plånboken är endast för granskning</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1092"/>
        <source>failed to save file </source>
        <translation>det gick inte att spara fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1108"/>
        <source>Key images can only be imported with a trusted daemon</source>
        <translation>Nyckelavbildningar kan bara importeras med en betrodd daemon</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1121"/>
        <source>Failed to import key images: </source>
        <translation>Det gick inte att importera nyckelavbildningar: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1153"/>
        <source>Failed to get subaddress label: </source>
        <translation>Det gick inte att hämta etikett för underadress: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1166"/>
        <source>Failed to set subaddress label: </source>
        <translation>Det gick inte att ange etikett för underadress: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="567"/>
        <source>Neither view key nor spend key supplied, cancelled</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="686"/>
        <source>Electrum seed is empty</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="695"/>
        <source>Electrum-style word list failed verification</source>
        <translation type="unfinished">Det gick inte att verifiera ordlista av Electrum-typ</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1183"/>
        <source>Failed to get multisig info: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1200"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1214"/>
        <source>Failed to make multisig: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1229"/>
        <source>Failed to finalize multisig wallet creation</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1232"/>
        <source>Failed to finalize multisig wallet creation: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1248"/>
        <source>Failed to export multisig images: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1266"/>
        <source>Failed to parse imported multisig images</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1276"/>
        <source>Failed to import multisig images: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1290"/>
        <source>Failed to check for partial multisig key images: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1318"/>
        <source>Failed to restore multisig transaction: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1358"/>
        <source>Invalid destination address</source>
        <translation type="unfinished">Ogiltig måladress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1434"/>
        <source>failed to get outputs to mix: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1445"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1529"/>
        <source>not enough money to transfer, overall balance only %s, sent amount %s</source>
        <translation>inte tillräckligt med pengar för överföring, totalt saldo är bara %s, skickat belopp %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1452"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1537"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>ej tillräckligt med pengar för överföring, endast tillgängligt %s, transaktionsbelopp %s = %s + %s (avgift)</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1462"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1547"/>
        <source>output amount</source>
        <translation>utgångens belopp</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1467"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1551"/>
        <source>transaction was not constructed</source>
        <translation>transaktionen konstruerades inte</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1470"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1554"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>transaktionen %s avvisades av daemonen med status: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1475"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1559"/>
        <source>one of destinations is zero</source>
        <translation>ett av målen är noll</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1477"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1561"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>det gick inte att hitta ett lämpligt sätt att dela upp transaktioner</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1479"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1563"/>
        <source>unknown transfer error: </source>
        <translation>okänt överföringsfel: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1481"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1565"/>
        <source>internal error: </source>
        <translation>internt fel: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1483"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1567"/>
        <source>unexpected error: </source>
        <translation>oväntat fel: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1485"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1569"/>
        <source>unknown error</source>
        <translation>okänt fel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1516"/>
        <source>failed to get outputs to mix</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1644"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1671"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1719"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1747"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1775"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1796"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2258"/>
        <source>Failed to parse txid</source>
        <translation>Det gick inte att parsa txid</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1661"/>
        <source>no tx keys found for this txid</source>
        <translation>inga tx-nycklar kunde hittas för detta txid</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1679"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1688"/>
        <source>Failed to parse tx key</source>
        <translation>Det gick inte att parsa txnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1697"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1726"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1754"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1835"/>
        <source>Failed to parse address</source>
        <translation>Det gick inte att parsa adressen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1840"/>
        <source>Address must not be a subaddress</source>
        <translation>Adressen får inte vara en underadress</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1880"/>
        <source>The wallet must be in multisig ready state</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1902"/>
        <source>Given string is not a key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2130"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>Genomsök efter spenderade kan endast användas med en betrodd daemon</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2179"/>
        <source>Invalid output: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2186"/>
        <source>Failed to mark outputs as spent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2197"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2219"/>
        <source>Failed to parse output amount</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2202"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2224"/>
        <source>Failed to parse output offset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2208"/>
        <source>Failed to mark output as spent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2230"/>
        <source>Failed to mark output as unspent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2241"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2280"/>
        <source>Failed to parse key image</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2247"/>
        <source>Failed to get ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2265"/>
        <source>Failed to get rings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2286"/>
        <source>Failed to set ring</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Wallet</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="301"/>
        <source>Failed to parse address</source>
        <translation>Det gick inte att parsa adressen</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="308"/>
        <source>Failed to parse key</source>
        <translation>Det gick inte att parsa nyckeln</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="316"/>
        <source>failed to verify key</source>
        <translation>det gick inte att verifiera nyckeln</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="326"/>
        <source>key does not match address</source>
        <translation>nyckeln matchar inte adressen</translation>
    </message>
</context>
<context>
    <name>command_line</name>
    <message>
        <location filename="../src/common/command_line.cpp" line="54"/>
        <source>yes</source>
        <translation>ja</translation>
    </message>
    <message>
        <location filename="../src/common/command_line.cpp" line="68"/>
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
        <location filename="../src/rpc/rpc_args.cpp" line="101"/>
        <source>Username specified with --</source>
        <translation>Användarnamn angivet med --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="101"/>
        <location filename="../src/rpc/rpc_args.cpp" line="111"/>
        <source> cannot be empty</source>
        <translation> får inte vara tomt</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="111"/>
        <source> requires RPC server password --</source>
        <translation> kräver lösenord till RPC-server --</translation>
    </message>
</context>
<context>
    <name>cryptonote::simple_wallet</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="645"/>
        <source>Commands: </source>
        <translation>Kommandon: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4359"/>
        <source>failed to read wallet password</source>
        <translation>det gick inte att läsa lösenord för plånboken</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3954"/>
        <source>invalid password</source>
        <translation>ogiltigt lösenord</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3073"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation>set seed: kräver ett argument. tillgängliga alternativ: språk</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3108"/>
        <source>set: unrecognized argument(s)</source>
        <translation>set: okända argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4199"/>
        <source>wallet file path not valid: </source>
        <translation>ogiltig sökväg till plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3178"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation>Försöker skapa eller återställa plånbok, men angivna filer finns redan.  Avslutar för att inte riskera att skriva över någonting.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3059"/>
        <source>needs an argument</source>
        <translation>kräver ett argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3082"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3083"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3084"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3086"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3089"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3094"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3095"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3097"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3099"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3100"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3101"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3104"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3105"/>
        <source>0 or 1</source>
        <translation>0 eller 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3092"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3096"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3103"/>
        <source>unsigned integer</source>
        <translation>positivt heltal</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3312"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-deterministic-wallet använder --generate-new-wallet, inte --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3341"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation>ange en återställningsparameter med --electrum-seed=&quot;ordlista här&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3717"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation>ange sökväg till en plånbok med --generate-new-wallet (inte --wallet-file)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3887"/>
        <source>wallet failed to connect to daemon: </source>
        <translation>plånboken kunde inte ansluta till daemonen: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3895"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation>Daemonen använder en högre version av RPC (%u) än plånboken (%u): %s. Antingen uppdatera en av dem, eller använd --allow-mismatched-daemon-version.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3916"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation>Lista över tillgängliga språk för din plånboks startvärde:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3926"/>
        <source>Enter the number corresponding to the language of your choice: </source>
        <translation>Ange det tal som motsvarar det språk du vill använda: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4000"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation>Du hade använt en inaktuell version av plånboken. Använd det nya startvärde som tillhandahålls.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4016"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4088"/>
        <source>Generated new wallet: </source>
        <translation>Ny plånbok skapad: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4025"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4093"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4135"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4188"/>
        <source>failed to generate new wallet: </source>
        <translation>det gick inte att skapa ny plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4230"/>
        <source>Opened watch-only wallet</source>
        <translation>Öppnade plånbok för granskning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4234"/>
        <source>Opened wallet</source>
        <translation>Öppnade plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4252"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation>Du hade använt en inaktuell version av plånboken. Fortsätt för att uppgradera din plånbok.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4267"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation>Du hade använt en inaktuell version av plånboken. Plånbokens filformat kommer nu att uppgraderas.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4275"/>
        <source>failed to load wallet: </source>
        <translation>det gick inte att läsa in plånboken: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4292"/>
        <source>Use the &quot;help&quot; command to see the list of available commands.
</source>
        <translation>Använd kommandot &quot;help&quot; för att visa en lista över tillgängliga kommandon.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4337"/>
        <source>Wallet data saved</source>
        <translation>Plånboksdata sparades</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4431"/>
        <source>Mining started in daemon</source>
        <translation>Brytning startad i daemonen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4433"/>
        <source>mining has NOT been started: </source>
        <translation>brytning har INTE startats: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4453"/>
        <source>Mining stopped in daemon</source>
        <translation>Brytning stoppad i daemonen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4455"/>
        <source>mining has NOT been stopped: </source>
        <translation>brytning har INTE stoppats: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4537"/>
        <source>Blockchain saved</source>
        <translation>Blockkedjan sparades</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4552"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4589"/>
        <source>Height </source>
        <translation>Höjd </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4591"/>
        <source>spent </source>
        <translation>spenderat </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4698"/>
        <source>Starting refresh...</source>
        <translation>Startar uppdatering …</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4712"/>
        <source>Refresh done, blocks received: </source>
        <translation>Uppdatering färdig, mottagna block: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5958"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>betalnings-ID har ogiltigt format. En hexadecimal sträng med 16 eller 64 tecken förväntades: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5307"/>
        <source>bad locked_blocks parameter:</source>
        <translation>felaktig parameter för locked_blocks:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5978"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6251"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation>en enda transaktion kan inte använda fler än ett betalnings-ID: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5405"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5987"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6219"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6259"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>det gick inte att upprätta betalnings-ID, trots att det avkodades korrekt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5251"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5870"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6173"/>
        <source>ring size %u is too large, maximum is %u</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5276"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5395"/>
        <source>Unencrypted payment IDs are bad for privacy: ask the recipient to use subaddresses instead</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5293"/>
        <source>payment id failed to encode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5312"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5894"/>
        <source>Locked blocks too high, max 1000000 (Ë4 yrs)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5340"/>
        <source>failed to parse short payment ID from URI</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5363"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5365"/>
        <source>Invalid last argument: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5382"/>
        <source>a single transaction cannot use more than one payment id</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5399"/>
        <source>failed to parse payment id, though it was detected</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5422"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5502"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5590"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5738"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6001"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6059"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6273"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6318"/>
        <source>transaction cancelled.</source>
        <translation>transaktion avbruten.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5481"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5491"/>
        <source>Is this okay anyway?  (Y/Yes/N/No): </source>
        <translation>Är detta okej ändå?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5486"/>
        <source>There is currently a %u block backlog at that fee level. Is this okay?  (Y/Yes/N/No): </source>
        <translation>Det finns för närvarande en %u blocks eftersläpning på den avgiftsnivån. Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5491"/>
        <source>Failed to check for backlog: </source>
        <translation>Det gick inte att kontrollera eftersläpning: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5532"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6032"/>
        <source>
Transaction </source>
        <translation>
Transaktion </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5537"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6037"/>
        <source>Spending from address index %d
</source>
        <translation>Spendera från adressindex %d
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5539"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6039"/>
        <source>WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.
</source>
        <translation>VARNING: Utgångar från flera adresser används tillsammans, vilket möjligen kan kompromettera din sekretess.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5541"/>
        <source>Sending %s.  </source>
        <translation>Skickar %s.  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5544"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation>Transaktionen behöver delas upp i %llu transaktioner.  Detta gör att en transaktionsavgift läggs till varje transaktion, med ett totalbelopp på %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5550"/>
        <source>The transaction fee is %s</source>
        <translation>Transaktionsavgiften är %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5553"/>
        <source>, of which %s is dust from change</source>
        <translation>, varav %s är damm från växel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5554"/>
        <source>.</source>
        <translation>.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5554"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation>Ett totalt belopp på %s från växeldamm skickas till damm-adressen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5559"/>
        <source>.
This transaction will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation>.
Denna transaktion låses upp vid block %llu, om ungefär %s dagar (förutsatt en blocktid på 2 minuter)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5603"/>
        <source>Unsigned transaction(s) successfully written to MMS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5611"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5648"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5749"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5761"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6070"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6107"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6328"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6340"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Det gick inte att skriva transaktioner till fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5616"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5653"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5753"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5765"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6074"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6111"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6332"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6344"/>
        <source>Unsigned transaction(s) successfully written to file: </source>
        <translation>Osignerade transaktioner skrevs till fil: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5625"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6086"/>
        <source>Failed to cold sign transaction with HW wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5708"/>
        <source>No unmixable outputs found</source>
        <translation>Inga omixbara utgångar kunde hittas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5775"/>
        <source>Not enough money in unlocked balance</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5776"/>
        <source>Discarding %s of unmixable outputs that cannot be spent, which can be undone by &quot;rescan_spent&quot;.  Is this okay?  (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5815"/>
        <source>No address given</source>
        <translation>Ingen adress har angivits</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5879"/>
        <source>missing lockedblocks parameter</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5889"/>
        <source>bad locked_blocks parameter</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5914"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6182"/>
        <source>Failed to parse number of outputs</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5919"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6187"/>
        <source>Amount of outputs should be greater than 0</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6213"/>
        <source>failed to parse Payment ID</source>
        <translation>det gick inte att parsa betalnings-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6236"/>
        <source>failed to parse key image</source>
        <translation>det gick inte att parsa nyckelavbildning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6290"/>
        <source>No outputs found</source>
        <translation>Inga utgångar kunde hittas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6295"/>
        <source>Multiple transactions are created, which is not supposed to happen</source>
        <translation>Flera transaktioner skapas, vilket inte ska kunna inträffa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6300"/>
        <source>The transaction uses multiple or no inputs, which is not supposed to happen</source>
        <translation>Transaktionen använder flera eller inga ingångar, vilket inte ska kunna inträffa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6377"/>
        <source>missing threshold amount</source>
        <translation>tröskelbelopp saknas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6382"/>
        <source>invalid amount threshold</source>
        <translation>ogiltigt tröskelbelopp</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6516"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>Begärd växel går inte till en betald adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6521"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>Begärd växel är större än betalning till växeladressen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6552"/>
        <source>sending %s to %s</source>
        <translation>skickar %s till %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6562"/>
        <source> dummy output(s)</source>
        <translation> dummy-utgångar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6565"/>
        <source>with no destinations</source>
        <translation>utan några mål</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6577"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu, %s. %sIs this okay? (Y/Yes/N/No): </source>
        <translation>Läste in %lu transaktioner, för %s, avgift %s, %s, %s, med minsta ringstorlek %lu, %s. %sÄr detta okej? (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6606"/>
        <source>This is a multisig wallet, it can only sign with sign_multisig</source>
        <translation>Detta är en multisig-plånbok, som endast kan signera med sign_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6629"/>
        <source>Failed to sign transaction</source>
        <translation>Det gick inte att signera transaktionen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6635"/>
        <source>Failed to sign transaction: </source>
        <translation>Det gick inte att signera transaktionen: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6656"/>
        <source>Transaction raw hex data exported to </source>
        <translation>Hexadecimala rådata för transaktionen exporterades till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6677"/>
        <source>Failed to load transaction from file</source>
        <translation>Det gick inte att läsa in transaktion från fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4729"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5051"/>
        <source>RPC error: </source>
        <translation>RPC-fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="695"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation>plånboken är enbart för granskning och har ingen spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="839"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1021"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1074"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1141"/>
        <source>Your original password was incorrect.</source>
        <translation>Ditt ursprungliga lösenord var fel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="854"/>
        <source>Error with wallet rewrite: </source>
        <translation>Ett fel uppstod vid återskrivning av plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2284"/>
        <source>invalid unit</source>
        <translation>ogiltig enhet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2302"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2364"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation>ogiltigt värde för count: måste vara ett heltal utan tecken</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2320"/>
        <source>invalid value</source>
        <translation>ogiltigt värde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3204"/>
        <source>(Y/Yes/N/No): </source>
        <translation>(J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3761"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3788"/>
        <source>bad m_restore_height parameter: </source>
        <translation>felaktig parameter för m_restore_height: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3766"/>
        <source>date format must be YYYY-MM-DD</source>
        <translation>datumformat måste vara ÅÅÅÅ-MM-DD</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3779"/>
        <source>Restore height is: </source>
        <translation>Återställningshöjd är: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3705"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3780"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5583"/>
        <source>Is this okay?  (Y/Yes/N/No): </source>
        <translation>Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4509"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Daemonen är lokal, utgår från att den är betrodd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4355"/>
        <source>Password for new watch-only wallet</source>
        <translation>Lösenord för ny granskningsplånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4739"/>
        <source>internal error: </source>
        <translation>internt fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1608"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4744"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5056"/>
        <source>unexpected error: </source>
        <translation>oväntat fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1534"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1613"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4749"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5061"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5639"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5669"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5794"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6099"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6126"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6361"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6690"/>
        <source>unknown error</source>
        <translation>okänt fel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4754"/>
        <source>refresh failed: </source>
        <translation>det gick inte att uppdatera: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4754"/>
        <source>Blocks received: </source>
        <translation>Mottagna block: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4787"/>
        <source>unlocked balance: </source>
        <translation>upplåst saldo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3093"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4894"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4956"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7518"/>
        <source>amount</source>
        <translation>belopp</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="341"/>
        <source>false</source>
        <translation>falskt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="659"/>
        <source>Unknown command: </source>
        <translation>Okänt kommando: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="666"/>
        <source>Command usage: </source>
        <translation>Användning av kommando: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="669"/>
        <source>Command description: </source>
        <translation>Beskrivning av kommando: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="735"/>
        <source>wallet is multisig but not yet finalized</source>
        <translation>plånboken är multisig men är ännu inte slutförd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="768"/>
        <source>Failed to retrieve seed</source>
        <translation>Det gick inte att hämta startvärde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="792"/>
        <source>wallet is multisig and has no seed</source>
        <translation>plånboken är multisig och har inget startvärde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="899"/>
        <source>Error: failed to estimate backlog array size: </source>
        <translation>Fel: det gick inte att uppskatta eftersläpningsmatrisens storlek: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="904"/>
        <source>Error: bad estimated backlog array size</source>
        <translation>Fel: felaktigt uppskattat värde för eftersläpningsmatrisens storlek</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="916"/>
        <source> (current)</source>
        <translation> (aktuellt)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="919"/>
        <source>%u block (%u minutes) backlog at priority %u%s</source>
        <translation>%u blocks (%u minuters) eftersläpning vid prioritet %u%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="921"/>
        <source>%u to %u block (%u to %u minutes) backlog at priority %u</source>
        <translation>%u till %u blocks (%u till %u minuters) eftersläpning vid prioritet %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="924"/>
        <source>No backlog at priority </source>
        <translation>Ingen eftersläpning vid prioritet </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="944"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="989"/>
        <source>This wallet is already multisig</source>
        <translation>Denna plånbok är redan multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="949"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="994"/>
        <source>wallet is watch-only and cannot be made multisig</source>
        <translation>plånboken är enbart för granskning och kan inte göras om till multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="955"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1000"/>
        <source>This wallet has been used before, please use a new wallet to create a multisig wallet</source>
        <translation>Denna plånbok har använts tidigare. Använd en ny plånbok för att skapa en multisig-plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="963"/>
        <source>Send this multisig info to all other participants, then use make_multisig &lt;threshold&gt; &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Skicka denna multisig-info till alla andra deltagare och använd sedan make_multisig &lt;tröskelvärde&gt; &lt;info1&gt; [&lt;info2&gt;…] med de andras multisig-info</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="964"/>
        <source>This includes the PRIVATE view key, so needs to be disclosed only to that multisig wallet&apos;s participants </source>
        <translation>Detta innefattar den PRIVATA granskningsnyckeln, så den behöver endast lämnas ut till den multisig-plånbokens deltagare </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1014"/>
        <source>Invalid threshold</source>
        <translation>Ogiltigt tröskelvärde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1034"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1156"/>
        <source>Another step is needed</source>
        <translation>Ytterligare ett steg krävs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1046"/>
        <source>Error creating multisig: </source>
        <translation>Ett fel uppstod när multisig skapades: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1053"/>
        <source>Error creating multisig: new wallet is not multisig</source>
        <translation>Ett fel uppstod när multisig skapades: den nya plånboken är inte multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1056"/>
        <source> multisig address: </source>
        <translation> multisig-adress: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1080"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1129"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1195"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1261"/>
        <source>This wallet is not multisig</source>
        <translation>Denna plånbok är inte multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1085"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1134"/>
        <source>This wallet is already finalized</source>
        <translation>Denna plånbok är redan slutförd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1101"/>
        <source>Failed to finalize multisig</source>
        <translation>Det gick inte att slutföra multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1107"/>
        <source>Failed to finalize multisig: </source>
        <translation>Det gick inte att slutföra multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1200"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1266"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1360"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1476"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1557"/>
        <source>This multisig wallet is not yet finalized</source>
        <translation>Denna multisig-plånbok är inte slutförd ännu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1236"/>
        <source>Error exporting multisig info: </source>
        <translation>Ett fel uppstod när multisig-info exporterades: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1240"/>
        <source>Multisig info exported to </source>
        <translation>Multisig-info exporterades till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1306"/>
        <source>Multisig info imported</source>
        <translation>Multisig-info importerades</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1310"/>
        <source>Failed to import multisig info: </source>
        <translation>Det gick inte att importera multisig-info: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1321"/>
        <source>Failed to update spent status after importing multisig info: </source>
        <translation>Det gick inte att uppdatera spenderstatus efter import av multisig-info: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1327"/>
        <source>Untrusted daemon, spent status may be incorrect. Use a trusted daemon and run &quot;rescan_spent&quot;</source>
        <translation>Ej betrodd daemon. Spenderstatus kan vara felaktig. Använd en betrodd daemon och kör &quot;rescan_spent&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1355"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1471"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1552"/>
        <source>This is not a multisig wallet</source>
        <translation>Detta är inte en multisig-plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1405"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1414"/>
        <source>Failed to sign multisig transaction</source>
        <translation>Det gick inte att signera multisig-transaktion</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1421"/>
        <source>Multisig error: </source>
        <translation>Multisig-fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1426"/>
        <source>Failed to sign multisig transaction: </source>
        <translation>Det gick inte att signera multisig-transaktion: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1449"/>
        <source>It may be relayed to the network with submit_multisig</source>
        <translation>Den kan skickas vidare till nätverket med submit_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1508"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1578"/>
        <source>Failed to load multisig transaction from file</source>
        <translation>Det gick inte att läsa in multisig-transaktion från fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1514"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1583"/>
        <source>Multisig transaction signed by only %u signers, needs %u more signatures</source>
        <translation>Multisig-transaktion har signerats av bara %u signerare. Den behöver %u ytterligare signaturer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1523"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8890"/>
        <source>Transaction successfully submitted, transaction </source>
        <translation>Transaktionen skickades, transaktion </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1524"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8891"/>
        <source>You can check its status by using the `show_transfers` command.</source>
        <translation>Du kan kontrollera dess status genom att använda kommandot &apos;show_transfers&apos;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1599"/>
        <source>Failed to export multisig transaction to file </source>
        <translation>Det gick inte att exportera multisig-transaktion till fil </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1603"/>
        <source>Saved exported multisig transaction file(s): </source>
        <translation>Sparade filer med exporterade multisig-transaktioner: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2095"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2101"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2120"/>
        <source>ring size must be an integer &gt;= </source>
        <translation>ringstorlek måste vara ett heltal &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2125"/>
        <source>could not change default ring size</source>
        <translation>det gick inte att ändra standardinställning för ringstorlek</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2398"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2469"/>
        <source>Invalid height</source>
        <translation>Ogiltig höjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2562"/>
        <source>Start mining in the daemon (bg_mining and ignore_battery are optional booleans).</source>
        <translation>Starta brytning i daemonen (bgbrytning och ignorera_batteri är valfri booleska värden).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2565"/>
        <source>Stop mining in the daemon.</source>
        <translation>Stoppa brytning i daemonen.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2569"/>
        <source>Set another daemon to connect to.</source>
        <translation>Ange en annan daemon att ansluta till.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2572"/>
        <source>Save the current blockchain data.</source>
        <translation>Spara aktuella blockkedjedata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2575"/>
        <source>Synchronize the transactions and balance.</source>
        <translation>Synkronisera transaktionerna och saldot.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2579"/>
        <source>Show the wallet&apos;s balance of the currently selected account.</source>
        <translation>Visa plånbokens saldo för det aktiva kontot.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2589"/>
        <source>Show the payments for the given payment IDs.</source>
        <translation>Visa betalningar för givna betalnings-ID.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2592"/>
        <source>Show the blockchain height.</source>
        <translation>Visa blockkedjans höjd.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2606"/>
        <source>Send all unmixable outputs to yourself with ring_size 1</source>
        <translation>Skicka alla omixbara utgångar till dig själv med ringstorlek 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2613"/>
        <source>Send all unlocked outputs below the threshold to an address.</source>
        <translation>Skicka alla upplåsta utgångar under tröskelvärdet till en adress.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2617"/>
        <source>Send a single output of the given key image to an address without change.</source>
        <translation>Skicka en enda utgång hos den givna nyckelavbildningen till en adress utan växel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2621"/>
        <source>Donate &lt;amount&gt; to the development team (donate.getmonero.org).</source>
        <translation>Donera &lt;belopp&gt; till utvecklingsteamet (donate.getmonero.org).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2628"/>
        <source>Submit a signed transaction from a file.</source>
        <translation>Skicka en signerad transaktion från en fil.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2632"/>
        <source>Change the current log detail (level must be &lt;0-4&gt;).</source>
        <translation>Ändra detaljnivån för aktuell logg (nivå måste vara 0-4).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2636"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="2650"/>
        <source>Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation>Koda ett betalnings-ID till en integrerad adress för den aktuella plånbokens publika adress (om inget argument anges används ett slumpmässigt betalnings-ID), eller avkoda en integrerad adress till en standardadress och ett betalnings-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2654"/>
        <source>Print all entries in the address book, optionally adding/deleting an entry to/from it.</source>
        <translation>Skriv ut alla poster i adressboken, och valfritt lägg till/ta bort en post i den.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2657"/>
        <source>Save the wallet data.</source>
        <translation>Spara plånboksdata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2660"/>
        <source>Save a watch-only keys file.</source>
        <translation>Spara en fil med granskningsnycklar.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2663"/>
        <source>Display the private view key.</source>
        <translation>Visa privat granskningsnyckel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2666"/>
        <source>Display the private spend key.</source>
        <translation>Visa privat spendernyckel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2669"/>
        <source>Display the Electrum-style mnemonic seed</source>
        <translation>Visa det minnesbaserade startvärdet (Electrum-typ)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2719"/>
        <source>Display the encrypted Electrum-style mnemonic seed.</source>
        <translation>Visa det krypterade minnesbaserade startvärdet (Electrum-typ).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2722"/>
        <source>Rescan the blockchain for spent outputs.</source>
        <translation>Genomsök blockkedjan efter spenderade utgångar.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2726"/>
        <source>Get the transaction key (r) for a given &lt;txid&gt;.</source>
        <translation>Hämta transaktionsnyckel (r) för ett givet &lt;txid&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2734"/>
        <source>Check the amount going to &lt;address&gt; in &lt;txid&gt;.</source>
        <translation>Kontrollera belopp som går till &lt;adress&gt; i &lt;txid&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2738"/>
        <source>Generate a signature proving funds sent to &lt;address&gt; in &lt;txid&gt;, optionally with a challenge string &lt;message&gt;, using either the transaction secret key (when &lt;address&gt; is not your wallet&apos;s address) or the view secret key (otherwise), which does not disclose the secret key.</source>
        <translation>Skapa en signatur som bevisar att pengar skickades till &lt;adress&gt; i &lt;txid&gt;, valfritt med kontrollsträngen &lt;meddelande&gt;, genom att använda antingen transaktionens hemliga nyckel (när &lt;adress&gt; inte är din plånboks adress) eller den hemliga granskningsnyckeln (annars), vilket inte lämnar ut den hemliga nyckeln.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2742"/>
        <source>Check the proof for funds going to &lt;address&gt; in &lt;txid&gt; with the challenge string &lt;message&gt; if any.</source>
        <translation>Kontrollera beviset för pengar som skickats till &lt;adress&gt; i &lt;txid&gt; med kontrollsträngen &lt;meddelande&gt;, om den angivits.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2746"/>
        <source>Generate a signature proving that you generated &lt;txid&gt; using the spend secret key, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Skapa en signatur som bevisar att du skapade &lt;txid&gt; genom att använda den hemliga spendernyckeln, valfritt med kontrollsträngen &lt;meddelande&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2750"/>
        <source>Check a signature proving that the signer generated &lt;txid&gt;, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Kontrollera en signatur som bevisar att signeraren skapade &lt;txid&gt;, valfritt med kontrollsträngen &lt;meddelande&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2754"/>
        <source>Generate a signature proving that you own at least this much, optionally with a challenge string &lt;message&gt;.
If &apos;all&apos; is specified, you prove the entire sum of all of your existing accounts&apos; balances.
Otherwise, you prove the reserve of the smallest possible amount above &lt;amount&gt; available in your current account.</source>
        <translation>Skapa en signatur som bevisar att du äger åtminstone så här mycket, valfritt med kontrollsträngen &lt;meddelande&gt;.
Om &apos;all&apos; anges, bevisar du totalsumman av alla dina befintliga kontons saldo.
Annars bevisar du reserven för det minsta möjliga belopp över &lt;belopp&gt; som är tillgängligt på ditt aktuella konto.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2760"/>
        <source>Check a signature proving that the owner of &lt;address&gt; holds at least this much, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Kontrollera en signatur som bevisar att ägaren till &lt;adress&gt; har åtminstone så här mycket, valfritt med kontrollsträngen &lt;meddelande&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2780"/>
        <source>Show the unspent outputs of a specified address within an optional amount range.</source>
        <translation>Visa de ej spenderade utgångarna hos en angiven adress inom ett valfritt beloppsintervall.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2788"/>
        <source>Set an arbitrary string note for a &lt;txid&gt;.</source>
        <translation>Ange en godtycklig stränganteckning för &lt;txid&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2792"/>
        <source>Get a string note for a txid.</source>
        <translation>Hämta en stränganteckning för ett txid.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2796"/>
        <source>Set an arbitrary description for the wallet.</source>
        <translation>Ange en godtycklig beskrivning av plånboken.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2800"/>
        <source>Get the description of the wallet.</source>
        <translation>Hämta plånbokens beskrivning.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2803"/>
        <source>Show the wallet&apos;s status.</source>
        <translation>Visa plånbokens status.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2806"/>
        <source>Show the wallet&apos;s information.</source>
        <translation>Visa information om plånboken.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2810"/>
        <source>Sign the contents of a file.</source>
        <translation>Signera innehållet i en fil.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2814"/>
        <source>Verify a signature on the contents of a file.</source>
        <translation>Verifiera en signatur av innehållet in en fil.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2822"/>
        <source>Import a signed key images list and verify their spent status.</source>
        <translation>Importera en signerad lista av nyckelavbildningar och verifiera deras spenderstatus.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2834"/>
        <source>Export a set of outputs owned by this wallet.</source>
        <translation>Exportera en uppsättning utgångar som ägs av denna plånbok.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2838"/>
        <source>Import a set of outputs owned by this wallet.</source>
        <translation>Importera en uppsättning utgångar som ägs av denna plånbok.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2842"/>
        <source>Show information about a transfer to/from this address.</source>
        <translation>Visa information om en transktion till/från denna adress.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2845"/>
        <source>Change the wallet&apos;s password.</source>
        <translation>Ändra plånbokens lösenord.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2849"/>
        <source>Generate a new random full size payment id. These will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids.</source>
        <translation>Skapa ett nytt slumpmässigt betalnings-ID av normalstorlek. Dessa kommer att vara okrypterade på blockkedjan. Se integrated_address för krypterade korta betalnings-ID.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2852"/>
        <source>Print the information about the current fee and transaction backlog.</source>
        <translation>Skriv ut information om aktuell avgift och transaktionseftersläpning.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2854"/>
        <source>Export data needed to create a multisig wallet</source>
        <translation>Exportera data som krävs för att skapa en multisig-plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2857"/>
        <source>Turn this wallet into a multisig wallet</source>
        <translation>Gör denna plånbok till en multisig-plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2861"/>
        <source>Turn this wallet into a multisig wallet, extra step for N-1/N wallets</source>
        <translation>Gör denna plånbok till en multisig-plånbok, extra steg för plånböcker med N-1/N</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2869"/>
        <source>Export multisig info for other participants</source>
        <translation>Exportera multisig-info för andra deltagare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2873"/>
        <source>Import multisig info from other participants</source>
        <translation>Importera multisig-info från andra deltagare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2877"/>
        <source>Sign a multisig transaction from a file</source>
        <translation>Signera en a multisig-transaktion från en fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2881"/>
        <source>Submit a signed multisig transaction from a file</source>
        <translation>Skicka en signerad multisig-transaktion från en fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2885"/>
        <source>Export a signed multisig transaction to a file</source>
        <translation>Exportera en signerad multisig-transaktion till en fil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3002"/>
        <source>Show the help section or the documentation about a &lt;command&gt;.</source>
        <translation>Visa hjälpavsnittet eller dokumentationen för &lt;kommando&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3085"/>
        <source>integer &gt;= </source>
        <translation>heltal &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3098"/>
        <source>block height</source>
        <translation>blockhöjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3203"/>
        <source>No wallet found with that name. Confirm creation of new wallet named: </source>
        <translation>Ingen plånbok med det namnet kunde hittas. Bekräfta skapande av ny plånbok med namn: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3304"/>
        <source>can&apos;t specify both --restore-deterministic-wallet or --restore-multisig-wallet and --non-deterministic</source>
        <translation>det går inte att ange både --restore-deterministic-wallet eller --restore-multisig-wallet och --non-deterministic</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3310"/>
        <source>--restore-multisig-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-multisig-wallet använder --generate-new-wallet, inte --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3326"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;multisig seed here&quot;</source>
        <translation>ange en återställningsparameter med --electrum-seed=&quot;startvärde för multisig&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3355"/>
        <source>Multisig seed failed verification</source>
        <translation>Startvärde för multisig kunde inte verifieras</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3406"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3481"/>
        <source>This address is a subaddress which cannot be used here.</source>
        <translation>Denna adress är en underadress som inte kan användas här.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3558"/>
        <source>Error: expected M/N, but got: </source>
        <translation>Fel: förväntade M/N, men fick: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3563"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got: </source>
        <translation>Fel: förväntade N &gt; 1 och N &lt;= M, men fick: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3568"/>
        <source>Error: M/N is currently unsupported. </source>
        <translation>Fel: M/N stöds för närvarande inte. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3571"/>
        <source>Generating master wallet from %u of %u multisig wallet keys</source>
        <translation>Skapar huvudplånbok från %u av %u multisig-plånboksnycklar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3600"/>
        <source>failed to parse secret view key</source>
        <translation>det gick inte att parsa hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3608"/>
        <source>failed to verify secret view key</source>
        <translation>det gick inte att verifiera hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3628"/>
        <source>Secret spend key (%u of %u):</source>
        <translation>Hemlig spendernyckel (%u av %u):</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3651"/>
        <source>Error: M/N is currently unsupported</source>
        <translation>Fel: M/N stöds för närvarande inte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3802"/>
        <source>Restore height </source>
        <translation>Återställningshöjd </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3803"/>
        <source>Still apply restore height?  (Y/Yes/N/No): </source>
        <translation>Ska återställningshöjd fortfarande appliceras?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3829"/>
        <source>Warning: using an untrusted daemon at %s, privacy will be lessened</source>
        <translation>Varning: använder en ej betrodd daemon på %s; sekretessen kommer att vara mindre</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3888"/>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or change the daemon address using the &apos;set_daemon&apos; command.</source>
        <translation>Antingen har daemonen inte startat eller så angavs fel port. Se till att daemonen körs eller byt daemonadress med kommandot &apos;set_daemon&apos;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4036"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="4180"/>
        <source>failed to generate new mutlisig wallet</source>
        <translation>det gick inte att skapa ny multisig-plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4183"/>
        <source>Generated new %u/%u multisig wallet: </source>
        <translation>Skapa ny %u/%u-multisig-plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4232"/>
        <source>Opened %u/%u multisig wallet%s</source>
        <translation>Öppnade %u/%u-multisig-plånbok%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4293"/>
        <source>Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
</source>
        <translation>Använd &quot;help &lt;kommando&gt;&quot; för att visa dokumentation för kommandot.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4351"/>
        <source>wallet is multisig and cannot save a watch-only version</source>
        <translation>plånboken är multisig och kan inte spara en granskningsversion</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4476"/>
        <source>Unexpected array length - Exited simple_wallet::set_daemon()</source>
        <translation>Oväntad matrislängd - Lämnade simple_wallet::set_daemon()</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4517"/>
        <source>This does not seem to be a valid daemon URL.</source>
        <translation>Detta verkar inte vara en giltig daemon-URL.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4553"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4590"/>
        <source>txid </source>
        <translation>txid </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4555"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4592"/>
        <source>idx </source>
        <translation>idx </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4780"/>
        <source> (Some owned outputs have partial key images - import_multisig_info needed)</source>
        <translation> (Några ägda utgångar har partiella nyckelavbildningar - import_multisig_info krävs)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4783"/>
        <source>Currently selected account: [</source>
        <translation>Aktuellt valt konto: [</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4783"/>
        <source>] </source>
        <translation>] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4785"/>
        <source>Tag: </source>
        <translation>Tagg: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4785"/>
        <source>(No tag assigned)</source>
        <translation>(Ingen tagg tilldelad)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4792"/>
        <source>Balance per address:</source>
        <translation>Saldo per adress:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4793"/>
        <source>Address</source>
        <translation>Adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4793"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8008"/>
        <source>Balance</source>
        <translation>Saldo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4793"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8008"/>
        <source>Unlocked balance</source>
        <translation>Upplåst saldo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4793"/>
        <source>Outputs</source>
        <translation>Utgångar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4793"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8008"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9122"/>
        <source>Label</source>
        <translation>Etikett</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4801"/>
        <source>%8u %6s %21s %21s %7u %21s</source>
        <translation>%8u %6s %21s %21s %7u %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4894"/>
        <source>spent</source>
        <translation>spenderat</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4894"/>
        <source>global index</source>
        <translation>globalt index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4894"/>
        <source>tx id</source>
        <translation>tx-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4894"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4956"/>
        <source>addr index</source>
        <translation>addr index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4924"/>
        <source>No incoming transfers</source>
        <translation>Inga inkommande överföringar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4928"/>
        <source>No incoming available transfers</source>
        <translation>Inga inkommande tillgängliga överföringar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4932"/>
        <source>No incoming unavailable transfers</source>
        <translation>Inga inkommande otillgängliga överföringar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4956"/>
        <source>payment</source>
        <translation>betalning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4956"/>
        <source>transaction</source>
        <translation>transaktion</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4956"/>
        <source>height</source>
        <translation>höjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4956"/>
        <source>unlock time</source>
        <translation>upplåsningstid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4968"/>
        <source>No payments with id </source>
        <translation>Inga betalningar med ID </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5016"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5106"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5442"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5901"/>
        <source>failed to get blockchain height: </source>
        <translation>det gick inte att hämta blockkedjans höjd: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5114"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation>
Transaktion %llu/%llu: txid=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5135"/>
        <source>
Input %llu/%llu: amount=%s</source>
        <translation>
Ingång %llu/%llu: belopp=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5151"/>
        <source>failed to get output: </source>
        <translation>det gick inte att hämta utgång: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5159"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation>utgångsnyckelns ursprungsblockhöjd får inte vara högre än blockkedjans höjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5163"/>
        <source>
Originating block heights: </source>
        <translation>
Ursprungsblockhöjder: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5175"/>
        <source>
|</source>
        <translation>
|</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5175"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7706"/>
        <source>|
</source>
        <translation>|
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5192"/>
        <source>
Warning: Some input keys being spent are from </source>
        <translation>
Varning: Några ingångsnycklar som spenderas kommer från </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5194"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation>, vilket kan bryta ringsignaturens anonymitet. Se till att detta är avsiktligt!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5234"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5853"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6156"/>
        <source>Ring size must not be 0</source>
        <translation>Ringstorlek för inte vara 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5246"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5865"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6168"/>
        <source>ring size %u is too small, minimum is %u</source>
        <translation>ringstorlek %uär för liten, minimum är %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5258"/>
        <source>wrong number of arguments</source>
        <translation>fel antal argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5417"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5996"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6268"/>
        <source>No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): </source>
        <translation>Inget betalnings-ID har inkluderats med denna transaktion. Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5458"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6016"/>
        <source>No outputs found, or daemon is not ready</source>
        <translation>Inga utgångar hittades, eller så är daemonen inte klar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6428"/>
        <source>Failed to parse donation address: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6442"/>
        <source>Donating %s %s to The Monero Project (donate.getmonero.org or %s).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6444"/>
        <source>Donating %s %s to %s.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6759"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6770"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6777"/>
        <source>failed to parse tx_key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6786"/>
        <source>Tx key successfully stored.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6790"/>
        <source>Failed to store tx key: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7296"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7518"/>
        <source>block</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7440"/>
        <source>usage: show_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7493"/>
        <source>usage: export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;path&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7518"/>
        <source>direction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7518"/>
        <source>timestamp</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7518"/>
        <source>running balance</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7518"/>
        <source>hash</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7518"/>
        <source>payment ID</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7518"/>
        <source>fee</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7518"/>
        <source>destination</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7518"/>
        <source>index</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7518"/>
        <source>note</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7572"/>
        <source>CSV exported to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7730"/>
        <source>Warning: this will lose any information which can not be recovered from the blockchain.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7731"/>
        <source>This includes destination addresses, tx secret keys, tx notes, etc</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7732"/>
        <source>Rescan anyway ? (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7750"/>
        <source>MMS received new message</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8387"/>
        <source>Network type: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8388"/>
        <source>Testnet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8389"/>
        <source>Stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8389"/>
        <source>Mainnet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8559"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8605"/>
        <source>command only supported by HW wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8564"/>
        <source>hw wallet does not support cold KI sync</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8576"/>
        <source>Please confirm the key image sync on the device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8582"/>
        <source>Key images synchronized to height </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8585"/>
        <source>Running untrusted daemon, cannot determine which transaction output is spent. Use a trusted daemon with --trusted-daemon and run rescan_spent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8588"/>
        <source> spent, </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8588"/>
        <source> unspent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8592"/>
        <source>Failed to import key images</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8597"/>
        <source>Failed to import key images: </source>
        <translation type="unfinished">Det gick inte att importera nyckelavbildningar: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8614"/>
        <source>Failed to reconnect device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8619"/>
        <source>Failed to reconnect device: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8883"/>
        <source>Transaction successfully saved to </source>
        <translation>Transaktionen sparades till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8883"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8885"/>
        <source>, txid </source>
        <translation>, txid </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8885"/>
        <source>Failed to save transaction to </source>
        <translation>Det gick inte att spara transaktion till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5723"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6044"/>
        <source>Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Sveper upp %s i %llu transaktioner för en total avgift på %s.  Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5729"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6050"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6310"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Sveper upp %s för en total avgift på %s.  Är detta okej?  (J/Ja/N/Nej): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6611"/>
        <source>This is a watch only wallet</source>
        <translation>Detta är en granskningsplånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8813"/>
        <source>Double spend seen on the network: this transaction may or may not end up being mined</source>
        <translation>En dubbelspendering upptäcktes på nätverket: denna transaktion kanske aldrig blir verifierad</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8848"/>
        <source>Transaction ID not found</source>
        <translation>Transaktions-ID kunde inte hittas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="336"/>
        <source>true</source>
        <translation>sant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="389"/>
        <source>failed to parse refresh type</source>
        <translation>det gick inte att parsa uppdateringstyp</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="721"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="787"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="939"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="984"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1067"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1124"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1190"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1256"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1350"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1466"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1547"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6601"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6665"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6702"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6799"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7010"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7094"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8397"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8474"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8517"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8630"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8670"/>
        <source>command not supported by HW wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="726"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="797"/>
        <source>wallet is watch-only and has no seed</source>
        <translation>plånboken är enbart för granskning och har inget startvärde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="744"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="807"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation>plånboken är icke-deterministisk och har inget startvärde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="751"/>
        <source>Enter optional seed offset passphrase, empty to see raw seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="817"/>
        <source>Incorrect password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="883"/>
        <source>Current fee is %s %s per %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1036"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1158"/>
        <source>Send this multisig info to all other participants, then use exchange_multisig_keys &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1167"/>
        <source>Multisig wallet has been successfully created. Current wallet type: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1172"/>
        <source>Failed to perform multisig keys exchange: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1499"/>
        <source>Failed to load multisig transaction from MMS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1631"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1788"/>
        <source>Invalid key image</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1637"/>
        <source>Invalid txid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1649"/>
        <source>Key image either not spent, or spent with mixin 0</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1664"/>
        <source>Failed to get key image ring: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1679"/>
        <source>File doesn&apos;t exist</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1701"/>
        <source>Invalid ring specification: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1709"/>
        <source>Invalid key image: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1714"/>
        <source>Invalid ring type, expected relative or abosolute: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1720"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1732"/>
        <source>Error reading line: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1743"/>
        <source>Invalid ring: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1752"/>
        <source>Invalid relative ring: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1764"/>
        <source>Invalid absolute ring: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1773"/>
        <source>Failed to set ring for key image: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1773"/>
        <source>Continuing.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1803"/>
        <source>Missing absolute or relative keyword</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1813"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1820"/>
        <source>invalid index: must be a strictly positive unsigned integer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1828"/>
        <source>invalid index: indices wrap</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1838"/>
        <source>invalid index: indices should be in strictly ascending order</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1845"/>
        <source>failed to set ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1890"/>
        <source>First line is not an amount</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1904"/>
        <source>Invalid output: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1914"/>
        <source>Bad argument: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1914"/>
        <source>should be &quot;add&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1923"/>
        <source>Failed to open file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1929"/>
        <source>Invalid output key, and file doesn&apos;t exist</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1935"/>
        <source>Failed to mark output spent: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1952"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1979"/>
        <source>Invalid output</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1962"/>
        <source>Failed to mark output unspent: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1986"/>
        <source>Spent: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1988"/>
        <source>Not spent: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1992"/>
        <source>Failed to check whether output is spent: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2007"/>
        <source>Failed to save known rings: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2022"/>
        <source>Please confirm the transaction on the device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2069"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2088"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation>plånboken är enbart för granskning och kan inte göra överföringar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2106"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5581"/>
        <source>WARNING: this is a non default ring size, which may harm your privacy. Default is recommended.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2108"/>
        <source>WARNING: from v8, ring size will be fixed and this setting will be ignored.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2137"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2160"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2176"/>
        <source>priority must be either 0, 1, 2, 3, or 4, or one of: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2181"/>
        <source>could not change default priority</source>
        <translation>det gick inte att ändra standardinställning för prioritet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2249"/>
        <source>invalid argument: must be either 0/never, 1/action, or 2/encrypt/decrypt</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2510"/>
        <source>Device name not specified</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2519"/>
        <source>Device reconnect failed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2524"/>
        <source>Device reconnect failed: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2583"/>
        <source>Show the incoming transfers, all or filtered by availability and address index.

Output format:
Amount, Spent(&quot;T&quot;|&quot;F&quot;), &quot;locked&quot;|&quot;unlocked&quot;, RingCT, Global Index, Transaction Hash, Address Index, [Public Key, Key Image] </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2595"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt;. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2599"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2603"/>
        <source>Send all unlocked balance to an address and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. &lt;priority&gt; is the priority of the sweep. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2609"/>
        <source>Send all unlocked balance to an address. If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. If the parameter &quot;outputs=&lt;N&gt;&quot; is specified and  N &gt; 0, wallet splits the transaction into N even outputs.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2625"/>
        <source>Sign a transaction from a file. If the parameter &quot;export_raw&quot; is specified, transaction raw hex data suitable for the daemon RPC /sendrawtransaction is exported.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2646"/>
        <source>If no arguments are specified or &lt;index&gt; is specified, the wallet shows the default or specified address. If &quot;all&quot; is specified, the wallet shows all the existing addresses in the currently selected account. If &quot;new &quot; is specified, the wallet creates a new address with the provided label text (which can be empty). If &quot;label&quot; is specified, the wallet sets the label of the address specified by &lt;index&gt; to the provided label text.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2673"/>
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
   Set the default ring size (obsolete).
 auto-refresh &lt;1|0&gt;
   Whether to automatically synchronize new blocks from the daemon.
 refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt;
   Set the wallet&apos;s refresh behaviour.
 priority [0|1|2|3|4]
   Set the fee to default/unimportant/normal/elevated/priority.
 confirm-missing-payment-id &lt;1|0&gt;
 ask-password &lt;0|1|2   (or never|action|decrypt)&gt;
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="2730"/>
        <source>Set the transaction key (r) for a given &lt;txid&gt; in case the tx was made by some other device or 3rd party wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2765"/>
        <source>Show the incoming/outgoing transfers within an optional height range.

Output format:
In or Coinbase:    Block Number, &quot;block&quot;|&quot;in&quot;,              Time, Amount,  Transaction Hash, Payment ID, Subaddress Index,                     &quot;-&quot;, Note
Out:               Block Number, &quot;out&quot;,                     Time, Amount*, Transaction Hash, Payment ID, Fee, Destinations, Input addresses**, &quot;-&quot;, Note
Pool:                            &quot;pool&quot;, &quot;in&quot;,              Time, Amount,  Transaction Hash, Payment Id, Subaddress Index,                     &quot;-&quot;, Note, Double Spend Note
Pending or Failed:               &quot;failed&quot;|&quot;pending&quot;, &quot;out&quot;, Time, Amount*, Transaction Hash, Payment ID, Fee, Input addresses**,               &quot;-&quot;, Note

* Excluding change and fee.
** Set of address indices used as inputs in this transfer.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2775"/>
        <source>export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;filepath&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2776"/>
        <source>Export to CSV the incoming/outgoing transfers within an optional height range.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2784"/>
        <source>Rescan the blockchain from scratch, losing any information which can not be recovered from the blockchain itself.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2818"/>
        <source>Export a signed set of key images to a &lt;filename&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2826"/>
        <source>Synchronizes key images with the hw wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2830"/>
        <source>Attempts to reconnect HW wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2865"/>
        <source>Performs extra multisig keys exchange rounds. Needed for arbitrary M/N multisig wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2889"/>
        <source>Interface with the MMS (Multisig Messaging System)
&lt;subcommand&gt; is one of:
  init, info, signer, list, next, sync, transfer, delete, send, receive, export, note, show, set, help
  send_signer_config, start_auto_config, stop_auto_config, auto_config
Get help about a subcommand with: help mms &lt;subcommand&gt;, or mms help &lt;subcommand&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2897"/>
        <source>Initialize and configure the MMS for M/N = number of required signers/number of authorized signers multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2901"/>
        <source>Display current MMS configuration</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2905"/>
        <source>Set or modify authorized signer info (single-word label, transport address, Monero address), or list all signers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2909"/>
        <source>List all messages</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2913"/>
        <source>Evaluate the next possible multisig-related action(s) according to wallet state, and execute or offer for choice
By using &apos;sync&apos; processing of waiting messages with multisig sync info can be forced regardless of wallet state</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2918"/>
        <source>Force generation of multisig sync info regardless of wallet state, to recover from special situations like &quot;stale data&quot; errors</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2922"/>
        <source>Initiate transfer with MMS support; arguments identical to normal &apos;transfer&apos; command arguments, for info see there</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2926"/>
        <source>Delete a single message by giving its id, or delete all messages by using &apos;all&apos;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2930"/>
        <source>Send a single message by giving its id, or send all waiting messages</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2934"/>
        <source>Check right away for new messages to receive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2938"/>
        <source>Write the content of a message to a file &quot;mms_message_content&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2942"/>
        <source>Send a one-line message to an authorized signer, identified by its label, or show any waiting unread notes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2946"/>
        <source>Show detailed info about a single message</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2950"/>
        <source>Available options:
 auto-send &lt;1|0&gt;
   Whether to automatically send newly generated messages right away.
 </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2956"/>
        <source>Send completed signer config to all other authorized signers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2960"/>
        <source>Start auto-config at the auto-config manager&apos;s wallet by issuing auto-config tokens and optionally set others&apos; labels</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2964"/>
        <source>Delete any auto-config tokens and abort a auto-config process</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2968"/>
        <source>Start auto-config by using the token received from the auto-config manager</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2972"/>
        <source>Print the ring(s) used to spend a given key image or transaction (if the ring size is &gt; 1)

Output format:
Key Image, &quot;absolute&quot;, list of rings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2978"/>
        <source>Set the ring used for a given key image, so it can be reused in a fork</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2982"/>
        <source>Save known rings to the shared rings database</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2986"/>
        <source>Mark output(s) as spent so they never get selected as fake outputs in a ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2990"/>
        <source>Marks an output as unspent so it may get selected as a fake output in a ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2994"/>
        <source>Checks whether an output is marked as spent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2998"/>
        <source>Returns version information</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3087"/>
        <source>full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)</source>
        <translation>full (långsammast, inga antaganden); optimize-coinbase (snabb, antar att hela coinbase-transaktionen betalas till en enda adress); no-coinbase (snabbast, antar att ingen coinbase-transaktion tas emot), default (samma som optimize-coinbase)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3088"/>
        <source>0, 1, 2, 3, or 4, or one of </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3090"/>
        <source>0|1|2 (or never|action|decrypt)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3091"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation>monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3102"/>
        <source>&lt;major&gt;:&lt;minor&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3106"/>
        <source>&lt;device_name[:device_spec]&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3127"/>
        <source>wrong number range, use: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3166"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation>Plånbokens namn ej giltigt. Försök igen eller använd Ctrl-C för att avsluta.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3183"/>
        <source>Wallet and key files found, loading...</source>
        <translation>Plånbok och nyckelfil hittades, läser in …</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3189"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation>Nyckelfilen hittades men inte plånboksfilen. Återskapar …</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3195"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation>Nyckelfilen kunde inte hittas. Det gick inte att öppna plånbok: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3214"/>
        <source>Generating new wallet...</source>
        <translation>Skapar ny plånbok …</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3232"/>
        <source>NOTE: the following %s can be used to recover access to your wallet. Write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3234"/>
        <source>string</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3234"/>
        <source>25 words</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3273"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3285"/>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-device=&quot;wallet_name&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3364"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Det gick inte att verifiera ordlista av Electrum-typ</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3369"/>
        <source>Enter seed offset passphrase, empty if none</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3395"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3415"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3450"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3470"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3490"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3505"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3553"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3578"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3594"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3633"/>
        <source>No data supplied, cancelled</source>
        <translation>Inga data angivna, avbryter</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3401"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3476"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3584"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5371"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5969"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6243"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6818"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6886"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6950"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7154"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8193"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8454"/>
        <source>failed to parse address</source>
        <translation>det gick inte att parsa adressen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3421"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3511"/>
        <source>failed to parse view key secret key</source>
        <translation>det gick inte att parsa hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3430"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3528"/>
        <source>failed to verify view key secret key</source>
        <translation>det gick inte att verifiera hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3434"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3532"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3613"/>
        <source>view key does not match standard address</source>
        <translation>granskningsnyckel matchar inte standardadress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3439"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3459"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3536"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3669"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3695"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3726"/>
        <source>account creation failed</source>
        <translation>det gick inte att skapa konto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3455"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3496"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3638"/>
        <source>failed to parse spend key secret key</source>
        <translation>det gick inte att parsa spendernyckel hemlig nyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3520"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3658"/>
        <source>failed to verify spend key secret key</source>
        <translation>det gick inte att verifiera spendernyckel hemlig nyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3524"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3663"/>
        <source>spend key does not match standard address</source>
        <translation>spendernyckel matchar inte standardadress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3701"/>
        <source>No restore height is specified.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3702"/>
        <source>Assumed you are creating a new account, restore will be done from current estimated blockchain height.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3703"/>
        <source>Use --restore-height if you want to restore an already setup account from a specific height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3707"/>
        <source>account creation aborted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3816"/>
        <source>can&apos;t specify --subaddress-lookahead and --wallet-file at the same time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3820"/>
        <source>failed to open account</source>
        <translation>det gick inte att öppna konto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3824"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4391"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4444"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4529"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6854"/>
        <source>wallet is null</source>
        <translation>plånbok är null</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3832"/>
        <source>Failed to initialize ring database: privacy enhancing features will be inactive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3917"/>
        <source>If your display freezes, exit blind with ^C, then run again with --use-english-language-names</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3935"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3940"/>
        <source>invalid language choice entered. Please try again.
</source>
        <translation>ogiltigt språkval har angivits. Försök igen.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4019"/>
        <source>View key: </source>
        <translation>Granskningsnyckel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4130"/>
        <source>Generated new wallet on hw device: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4209"/>
        <source>Key file not found. Failed to open wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4286"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation>Du kan också prova att bort filen &quot;%s&quot; och försöka igen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4314"/>
        <source>failed to deinitialize wallet</source>
        <translation>det gick inte att avinitiera plånboken</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4367"/>
        <source>Watch only wallet saved as: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4371"/>
        <source>Failed to save watch only wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4382"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5024"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8522"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation>detta kommando kräver en betrodd daemon. Aktivera med --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4498"/>
        <source>Expected trusted or untrusted, got </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4515"/>
        <source>trusted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4515"/>
        <source>untrusted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4539"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation>blockkedjan kan inte sparas: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4569"/>
        <source>NOTE: this transaction uses an encrypted payment ID: consider using subaddresses instead</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4572"/>
        <source>WARNING: this transaction uses an unencrypted payment ID: consider using subaddresses instead</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4608"/>
        <source>Password needed (%s) - use the refresh command</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4616"/>
        <source>Enter password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4631"/>
        <source>Device requires attention</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4639"/>
        <source>Enter device PIN</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4641"/>
        <source>Failed to read device PIN</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4648"/>
        <source>Please enter the device passphrase on the device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4655"/>
        <source>Enter device passphrase</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4657"/>
        <source>Failed to read device passphrase</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4673"/>
        <source>The first refresh has finished for the HW-based wallet with received money. hw_key_images_sync is needed. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4675"/>
        <source>Do you want to do it now? (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4677"/>
        <source>hw_key_images_sync skipped. Run command manually before a transfer.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4720"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5038"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>daemonen är upptagen. Försök igen senare.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4724"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5042"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>ingen anslutning till daemonen. Se till att daemonen körs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4734"/>
        <source>refresh error: </source>
        <translation>fel vid uppdatering: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4782"/>
        <source> (Some owned outputs have missing key images - import_key_images needed)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4786"/>
        <source>Balance: </source>
        <translation>Saldo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4855"/>
        <source>Invalid keyword: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4893"/>
        <source>pubkey</source>
        <translation>publik nyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4893"/>
        <source>key image</source>
        <translation>nyckelavbildning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4894"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4910"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7518"/>
        <source>unlocked</source>
        <translation>upplåst</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4894"/>
        <source>ringct</source>
        <translation>ringct</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4904"/>
        <source>Heights: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4909"/>
        <source>T</source>
        <translation>S</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4909"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4910"/>
        <source>locked</source>
        <translation>låst</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4911"/>
        <source>RingCT</source>
        <translation>RingCT</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4911"/>
        <source>-</source>
        <translation>-</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4990"/>
        <source>payment ID has invalid format, expected 16 or 64 character hex string: </source>
        <translation>betalnings-ID har ogiltigt format. En hexadecimal sträng med 16 eller 64 tecken förväntades: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5046"/>
        <source>failed to get spent status</source>
        <translation>det gick inte att hämta spenderstatus</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5130"/>
        <source>failed to find construction data for tx input</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5193"/>
        <source>the same transaction</source>
        <translation>samma transaktion</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5193"/>
        <source>blocks that are temporally very close</source>
        <translation>block som ligger väldigt nära varandra i tiden</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9015"/>
        <source> (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9042"/>
        <source>Choose processing:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9051"/>
        <source>Sign tx</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9059"/>
        <source>Send the tx for submission to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9063"/>
        <source>Send the tx for signing to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9070"/>
        <source>Submit tx</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9073"/>
        <source>unknown</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9079"/>
        <source>Choice: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9091"/>
        <source>Wrong choice</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9098"/>
        <source>Id</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9098"/>
        <source>I/O</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9098"/>
        <source>Authorized Signer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9099"/>
        <source>Message Type</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9099"/>
        <source>Height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9099"/>
        <source>R</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9099"/>
        <source>Message State</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9099"/>
        <source>Since</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9116"/>
        <source> ago</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9122"/>
        <source>#</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9122"/>
        <source>Transport Address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9123"/>
        <source>Auto-Config Token</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9123"/>
        <source>Monero Address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9127"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9135"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9137"/>
        <source>&lt;not set&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9178"/>
        <source>Message </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9179"/>
        <source>In/out: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9181"/>
        <source>State: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9181"/>
        <source>%s since %s, %s ago</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9185"/>
        <source>Sent: Never</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9189"/>
        <source>Sent: %s, %s ago</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9192"/>
        <source>Authorized signer: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9193"/>
        <source>Content size: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9193"/>
        <source> bytes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9194"/>
        <source>Content: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9194"/>
        <source>(binary data)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9224"/>
        <source>Send these messages now?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9234"/>
        <source>Queued for sending.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9254"/>
        <source>Invalid message id</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9263"/>
        <source>usage: mms init &lt;required_signers&gt;/&lt;authorized_signers&gt; &lt;own_label&gt; &lt;own_transport_address&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9269"/>
        <source>The MMS is already initialized. Re-initialize by deleting all signer info and messages?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9284"/>
        <source>Error in the number of required signers and/or authorized signers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9301"/>
        <source>The MMS is not active.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9324"/>
        <source>Invalid signer number </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9329"/>
        <source>mms signer [&lt;number&gt; &lt;label&gt; [&lt;transport_address&gt; [&lt;monero_address&gt;]]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9348"/>
        <source>Invalid Monero address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9355"/>
        <source>Wallet state does not allow changing Monero addresses anymore</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9367"/>
        <source>Usage: mms list</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9380"/>
        <source>Usage: mms next [sync]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9405"/>
        <source>No next step: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9415"/>
        <source>prepare_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9421"/>
        <source>make_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9436"/>
        <source>exchange_multisig_keys</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9451"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9571"/>
        <source>export_multisig_info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9460"/>
        <source>import_multisig_info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9473"/>
        <source>sign_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9483"/>
        <source>submit_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9493"/>
        <source>Send tx</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9504"/>
        <source>Process signer config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9516"/>
        <source>Replace current signer config with the one displayed above?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9530"/>
        <source>Process auto config data</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9544"/>
        <source>Nothing ready to process</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9564"/>
        <source>Usage: mms sync</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9588"/>
        <source>Usage: mms delete (&lt;message_id&gt; | all)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9595"/>
        <source>Delete all messages?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9621"/>
        <source>Usage: mms send [&lt;message_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9638"/>
        <source>Usage: mms receive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9655"/>
        <source>Usage: mms export &lt;message_id&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9667"/>
        <source>Message content saved to: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9671"/>
        <source>Failed to to save message content</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9695"/>
        <source>Usage: mms note [&lt;label&gt; &lt;text&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9702"/>
        <source>No signer found with label </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9724"/>
        <source>Usage: mms show &lt;message_id&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9743"/>
        <source>Usage: mms set &lt;option_name&gt; [&lt;option_value&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9760"/>
        <source>Wrong option value</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9765"/>
        <source>Auto-send is on</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9765"/>
        <source>Auto-send is off</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9770"/>
        <source>Unknown option</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9778"/>
        <source>Usage: mms help [&lt;subcommand&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9794"/>
        <source>Usage: mms send_signer_config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9800"/>
        <source>Signer config not yet complete</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9815"/>
        <source>Usage: mms start_auto_config [&lt;label&gt; &lt;label&gt; ...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9820"/>
        <source>There are signers without a label set. Complete labels before auto-config or specify them as parameters here.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9826"/>
        <source>Auto-config is already running. Cancel and restart?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9850"/>
        <source>Usage: mms stop_auto_config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9853"/>
        <source>Delete any auto-config tokens and stop auto-config?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9866"/>
        <source>Usage: mms auto_config &lt;auto_config_token&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9873"/>
        <source>Invalid auto-config token</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9879"/>
        <source>Auto-config already running. Cancel and restart?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9911"/>
        <source>The MMS is not active. Activate using the &quot;mms init&quot; command</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9988"/>
        <source>Invalid MMS subcommand</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9993"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9997"/>
        <source>Error in MMS command: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6969"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7079"/>
        <source>Good signature</source>
        <translation>Godkänd signatur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6996"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7081"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7181"/>
        <source>Bad signature</source>
        <translation>Felaktig signatur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8169"/>
        <source>Standard address: </source>
        <translation>Standardadress: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8174"/>
        <source>failed to parse payment ID or address</source>
        <translation>det gick inte att parsa betalnings-ID eller adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8215"/>
        <source>failed to parse payment ID</source>
        <translation>det gick inte att parsa betalnings-ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8233"/>
        <source>failed to parse index</source>
        <translation>det gick inte att parsa index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8241"/>
        <source>Address book is empty.</source>
        <translation>Adressboken är tom.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8247"/>
        <source>Index: </source>
        <translation>Index: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8248"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8378"/>
        <source>Address: </source>
        <translation>Adress: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8249"/>
        <source>Payment ID: </source>
        <translation>Betalnings-ID: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8250"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8377"/>
        <source>Description: </source>
        <translation>Beskrivning: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8407"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation>plånboken är enbart för granskning och kan inte signera</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1289"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8421"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8447"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8684"/>
        <source>failed to read file </source>
        <translation>det gick inte att läsa filen </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6958"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7072"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7166"/>
        <source>failed to load signature file</source>
        <translation>det gick inte att läsa in signaturfil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7020"/>
        <source>wallet is watch-only and cannot generate the proof</source>
        <translation>plånboken är enbart för granskning och kan inte skapa beviset</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7104"/>
        <source>The reserve proof can be generated only by a full wallet</source>
        <translation>Beviset på reserv kan endast skapas av en standardplånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7159"/>
        <source>Address must not be a subaddress</source>
        <translation>Adressen får inte vara en underadress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7177"/>
        <source>Good signature -- total: %s, spent: %s, unspent: %s</source>
        <translation>Godkänd signatur -- summa: %s, spenderat: %s, ej spenderat: %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7365"/>
        <source>[Double spend seen on the network: this transaction may or may not end up being mined] </source>
        <translation>[En dubbelspendering upptäcktes på nätverket: denna transaktion kanske aldrig blir verifierad] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7641"/>
        <source>There is no unspent output in the specified address</source>
        <translation>Det finns ingen ej spenderad utgång i den angivna adressen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7799"/>
        <source> (no daemon)</source>
        <translation> (ingen daemon)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7801"/>
        <source> (out of sync)</source>
        <translation> (inte synkroniserad)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7852"/>
        <source>(Untitled account)</source>
        <translation>(Ej namngivet konto)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7865"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7883"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7908"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7931"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8077"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8100"/>
        <source>failed to parse index: </source>
        <translation>det gick inte att parsa index: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7870"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8082"/>
        <source>specify an index between 0 and </source>
        <translation>ange ett index mellan 0 och </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7988"/>
        <source>
Grand total:
  Balance: </source>
        <translation>
Totalsumma:
  Saldo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7988"/>
        <source>, unlocked balance: </source>
        <translation>, upplåst saldo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7996"/>
        <source>Untagged accounts:</source>
        <translation>Otaggade konton:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8002"/>
        <source>Tag %s is unregistered.</source>
        <translation>Taggen %s har inte registrerats.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8005"/>
        <source>Accounts with tag: </source>
        <translation>Konton med tagg: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8006"/>
        <source>Tag&apos;s description: </source>
        <translation>Taggens beskrivning: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8008"/>
        <source>Account</source>
        <translation>Konto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8014"/>
        <source> %c%8u %6s %21s %21s %21s</source>
        <translation> %c%8u %6s %21s %21s %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8024"/>
        <source>----------------------------------------------------------------------------------</source>
        <translation>----------------------------------------------------------------------------------</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8025"/>
        <source>%15s %21s %21s</source>
        <translation>%15s %21s %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8048"/>
        <source>Primary address</source>
        <translation>Primär adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8048"/>
        <source>(used)</source>
        <translation>(används)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8069"/>
        <source>(Untitled address)</source>
        <translation>(Ej namngiven adress)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8109"/>
        <source>&lt;index_min&gt; is already out of bound</source>
        <translation>&lt;index_min&gt; är redan utanför tillåtet intervall</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8114"/>
        <source>&lt;index_max&gt; exceeds the bound</source>
        <translation>&lt;index_max&gt; är utanför tillåtet intervall</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8140"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8152"/>
        <source>Integrated addresses can only be created for account 0</source>
        <translation>Integrerade adresser kan bara skapas för konto 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8164"/>
        <source>Integrated address: %s, payment ID: %s</source>
        <translation>Integrerad adress: %s, betalnings-ID: %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8169"/>
        <source>Subaddress: </source>
        <translation>Underadress: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8335"/>
        <source>no description found</source>
        <translation>ingen beskrivning hittades</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8337"/>
        <source>description found: </source>
        <translation>beskrivning hittades: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8376"/>
        <source>Filename: </source>
        <translation>Filnamn: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8381"/>
        <source>Watch only</source>
        <translation>Endast granskning</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8383"/>
        <source>%u/%u multisig%s</source>
        <translation>%u/%u multisig%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8385"/>
        <source>Normal</source>
        <translation>Normal</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8386"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9180"/>
        <source>Type: </source>
        <translation>Typ: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8412"/>
        <source>This wallet is multisig and cannot sign</source>
        <translation>Plånboken är multisig och kan inte signera</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8461"/>
        <source>Bad signature from </source>
        <translation>Felaktig signatur från </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8465"/>
        <source>Good signature from </source>
        <translation>Godkänd signatur från </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8484"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation>plånboken är enbart för granskning och kan inte exportera nyckelavbildningar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1228"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8498"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8651"/>
        <source>failed to save file </source>
        <translation>det gick inte att spara fil </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8509"/>
        <source>Signed key images exported to </source>
        <translation>Signerade nyckelavbildningar exporterades till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8662"/>
        <source>Outputs exported to </source>
        <translation>Utgångar exporterades till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5354"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6417"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7115"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7600"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7608"/>
        <source>amount is wrong: </source>
        <translation>beloppet är fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5355"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6417"/>
        <source>expected number from 0 to </source>
        <translation>förväntades: ett tal från 0 till </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5721"/>
        <source>Sweeping </source>
        <translation>Sveper upp </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6350"/>
        <source>Money successfully sent, transaction: </source>
        <translation>Pengar skickades, transaktion: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6530"/>
        <source>Change goes to more than one address</source>
        <translation>Växel går till fler än en adress</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6571"/>
        <source>%s change to %s</source>
        <translation>%s växel till %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6574"/>
        <source>no change</source>
        <translation>ingen växel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1435"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1448"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6646"/>
        <source>Transaction successfully signed to file </source>
        <translation>Transaktionen signerades till fil </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6713"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6749"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6811"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6860"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6942"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7027"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7062"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8267"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8295"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8714"/>
        <source>failed to parse txid</source>
        <translation>det gick inte att parsa txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6727"/>
        <source>Tx key: </source>
        <translation>Tx-nyckel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6732"/>
        <source>no tx keys found for this txid</source>
        <translation>inga tx-nycklar kunde hittas för detta txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6829"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7041"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7130"/>
        <source>signature file saved to: </source>
        <translation>signaturfilen sparades till: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6831"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7043"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7132"/>
        <source>failed to save signature file</source>
        <translation>det gick inte att spara signaturfilen</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6868"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6877"/>
        <source>failed to parse tx key</source>
        <translation>det gick inte att parsa txnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6835"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6923"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7001"/>
        <source>error: </source>
        <translation>fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6899"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6972"/>
        <source>received</source>
        <translation>mottaget</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6899"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6972"/>
        <source>in txid</source>
        <translation>i txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6918"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6991"/>
        <source>received nothing in txid</source>
        <translation>tog emot ingenting i txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6902"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6975"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation>VARNING: denna transaktion är ännu inte inkluderad i blockkedjan!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6908"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6981"/>
        <source>This transaction has %u confirmations</source>
        <translation>Denna transaktion har %u bekräftelser</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6912"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6985"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation>VARNING: det gick inte att bestämma antal bekräftelser!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7266"/>
        <source>bad min_height parameter:</source>
        <translation>felaktig parameter för min_höjd:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7278"/>
        <source>bad max_height parameter:</source>
        <translation>felaktig parameter för max_höjd:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7296"/>
        <source>in</source>
        <translation>in</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7615"/>
        <source>&lt;min_amount&gt; should be smaller than &lt;max_amount&gt;</source>
        <translation>&lt;min_belopp&gt; måste vara mindre än &lt;max_belopp&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7647"/>
        <source>
Amount: </source>
        <translation>
Belopp: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7647"/>
        <source>, number of keys: </source>
        <translation>, antal nycklar: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7652"/>
        <source> </source>
        <translation></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7657"/>
        <source>
Min block height: </source>
        <translation>
Minblockhöjd: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7658"/>
        <source>
Max block height: </source>
        <translation>
Maxblockhöjd: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7659"/>
        <source>
Min amount found: </source>
        <translation>
Minbelopp funnet: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7660"/>
        <source>
Max amount found: </source>
        <translation>
Maxbelopp funnet: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7661"/>
        <source>
Total count: </source>
        <translation>
Totalt antal: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7701"/>
        <source>
Bin size: </source>
        <translation>
Storlek för binge: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7702"/>
        <source>
Outputs per *: </source>
        <translation>
Utgångar per *: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7704"/>
        <source>count
  ^
</source>
        <translation>antal
  ^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7706"/>
        <source>  |</source>
        <translation>  |</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7708"/>
        <source>  +</source>
        <translation>  +</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7708"/>
        <source>+--&gt; block height
</source>
        <translation>+--&gt; blockhöjd
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7709"/>
        <source>   ^</source>
        <translation>   ^</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7709"/>
        <source>^
</source>
        <translation>^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7710"/>
        <source>  </source>
        <translation></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7797"/>
        <source>wallet</source>
        <translation>plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="870"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8144"/>
        <source>Random payment ID: </source>
        <translation>Slumpmässigt betalnings-ID: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8145"/>
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
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="83"/>
        <source>Generating %u %u/%u multisig wallets</source>
        <translation>Skapar %u %u/%u multisig-plånböcker</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="142"/>
        <source>Error verifying multisig extra info</source>
        <translation>Ett fel uppstod när extra multisig-info verifierades</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="72"/>
        <source>How many participants will share parts of the multisig wallet</source>
        <translation type="unfinished"></translation>
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
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="107"/>
        <source>Failed to verify multisig info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="153"/>
        <source>Generated multisig wallets for address </source>
        <translation>Skapade multisig-plånböcker för adress </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="157"/>
        <source>Error creating multisig wallets: </source>
        <translation>Ett fel uppstod när multisig-plånböcker skapades: </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="182"/>
        <source>This program generates a set of multisig wallets - use this simpler scheme only if all the participants trust each other</source>
        <translation>Programmet skapar en uppsättning multisig-plånböcker - använd endast detta enklare system om alla deltagare litar på varandra</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="201"/>
        <source>Error: Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="208"/>
        <source>Error: expected N/M, but got: </source>
        <translation>Fel: förväntade N/M, men fick: </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="216"/>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="225"/>
        <source>Error: either --scheme or both of --threshold and --participants may be given</source>
        <translation>Fel: antingen --scheme eller både --threshold och --participants får anges</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="232"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got N==%u and M==%d</source>
        <translation>Fel: förväntade N &gt; 1 och N &lt;= M, men fick N=%u och M=%d</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="241"/>
        <source>Error: --filename-base is required</source>
        <translation>Fel: --filename-base måste anges</translation>
    </message>
</context>
<context>
    <name>mms::message_store</name>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="69"/>
        <source>Use PyBitmessage instance at URL &lt;arg&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="70"/>
        <source>Specify &lt;arg&gt; as username:password for PyBitmessage API</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="832"/>
        <source>Auto-config cannot proceed because auto config data from other signers is not complete</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="857"/>
        <source>The signer config is not complete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="909"/>
        <source>Wallet can&apos;t go multisig because key sets from other signers are missing or not complete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="951"/>
        <source>Wallet can&apos;t start another key exchange round because key sets from other signers are missing or not complete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1015"/>
        <source>Syncing not done because multisig sync data from other signers are missing or not complete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1129"/>
        <source>There are waiting messages, but nothing is ready to process under normal circumstances</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1132"/>
        <source>
Use &quot;mms next sync&quot; if you want to force processing of the waiting sync data</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1136"/>
        <source>
Use &quot;mms note&quot; to display the waiting notes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1141"/>
        <source>There are no messages waiting to be processed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1359"/>
        <source>key set</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1361"/>
        <source>additional key set</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1363"/>
        <source>multisig sync data</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1365"/>
        <source>partially signed tx</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1367"/>
        <source>fully signed tx</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1369"/>
        <source>note</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1371"/>
        <source>signer config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1373"/>
        <source>auto-config data</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1375"/>
        <source>unknown message type</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1384"/>
        <source>in</source>
        <translation type="unfinished">in</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1386"/>
        <source>out</source>
        <translation type="unfinished">ut</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1388"/>
        <source>unknown message direction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1397"/>
        <source>ready to send</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1399"/>
        <source>sent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1401"/>
        <source>waiting</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1403"/>
        <source>processed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1405"/>
        <source>cancelled</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1407"/>
        <source>unknown message state</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>sw</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="125"/>
        <source>Generate new wallet and save it to &lt;arg&gt;</source>
        <translation>Skapa ny plånbok och spara den till &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="126"/>
        <source>Generate new wallet from device and save it to &lt;arg&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="127"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>Skapa granskningsplånbok från granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="128"/>
        <source>Generate deterministic wallet from spend key</source>
        <translation>Skapa deterministisk plånbok från spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="129"/>
        <source>Generate wallet from private keys</source>
        <translation>Skapa plånbok från privata nycklar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="130"/>
        <source>Generate a master wallet from multisig wallet keys</source>
        <translation>Skapa en huvudplånbok från multisig-plånboksnycklar</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="132"/>
        <source>Language for mnemonic</source>
        <translation>Språk för minnesbaserat startvärde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="133"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>Ange Electrum-startvärde för att återställa/skapa plånbok</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="134"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Återställ plånbok genom att använda minnesbaserat startvärde (Electrum-typ)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="135"/>
        <source>Recover multisig wallet using Electrum-style mnemonic seed</source>
        <translation>Återställ multisig-plånbok genom att använda minnesbaserat startvärde (Electrum-typ)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="136"/>
        <source>Generate non-deterministic view and spend keys</source>
        <translation>Skapa icke-deterministisk granskningsnyckel och spendernyckel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="361"/>
        <source>invalid argument: must be either 0/1, true/false, y/n, yes/no</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="417"/>
        <source>DNSSEC validation passed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="421"/>
        <source>WARNING: DNSSEC validation was unsuccessful, this address may not be correct!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="424"/>
        <source>For URL: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="426"/>
        <source> Monero Address = </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="428"/>
        <source>Is this OK? (Y/n) </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="438"/>
        <source>you have cancelled the transfer request</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="459"/>
        <source>failed to parse index: </source>
        <translation type="unfinished">det gick inte att parsa index: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="472"/>
        <source>invalid format for subaddress lookahead; must be &lt;major&gt;:&lt;minor&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="489"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation type="unfinished">ingen anslutning till daemonen. Se till att daemonen körs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="494"/>
        <source>RPC error: </source>
        <translation type="unfinished">RPC-fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="498"/>
        <source>failed to get random outputs to mix: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="505"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="513"/>
        <source>Not enough money in unlocked balance</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="523"/>
        <source>Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="529"/>
        <source>not enough outputs for specified ring size</source>
        <translation type="unfinished">inte tillräckligt med utgångar för angiven ringstorlek</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="532"/>
        <source>output amount</source>
        <translation type="unfinished">utgångens belopp</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="532"/>
        <source>found outputs to use</source>
        <translation type="unfinished">hittade utgångar att använda</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="534"/>
        <source>Please use sweep_unmixable.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="538"/>
        <source>transaction was not constructed</source>
        <translation type="unfinished">transaktionen konstruerades inte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="543"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation type="unfinished">transaktionen %s avvisades av daemonen med status: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="546"/>
        <source>Reason: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="555"/>
        <source>one of destinations is zero</source>
        <translation type="unfinished">ett av målen är noll</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="560"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation type="unfinished">det gick inte att hitta ett lämpligt sätt att dela upp transaktioner</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="566"/>
        <source>unknown transfer error: </source>
        <translation type="unfinished">okänt överföringsfel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="571"/>
        <source>Multisig error: </source>
        <translation type="unfinished">Multisig-fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="577"/>
        <source>internal error: </source>
        <translation type="unfinished">internt fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="582"/>
        <source>unexpected error: </source>
        <translation type="unfinished">oväntat fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="586"/>
        <source>There was an error, which could mean the node may be trying to get you to retry creating a transaction, and zero in on which outputs you own. Or it could be a bona fide error. It may be prudent to disconnect from this node, and not try to send a transaction immediately. Alternatively, connect to another node so the original node cannot correlate information.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="596"/>
        <source>File %s likely stores wallet private keys! Use a different file name.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="599"/>
        <source>File %s already exists. Are you sure to overwrite it? (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7195"/>
        <source> seconds</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7197"/>
        <source> minutes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7199"/>
        <source> hours</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7201"/>
        <source> days</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7203"/>
        <source> months</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7204"/>
        <source>a long time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8940"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.
WARNING: Do not reuse your Monero keys on another fork, UNLESS this fork has key reuse mitigations built in. Doing so will harm your privacy.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8965"/>
        <source>Unknown command: </source>
        <translation type="unfinished">Okänt kommando: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="137"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>Tillåt kommunikation med en daemon som använder en annan version av RPC</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="138"/>
        <source>Restore from specific blockchain height</source>
        <translation>Återställ från angiven blockkedjehöjd</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="139"/>
        <source>The newly created transaction will not be relayed to the monero network</source>
        <translation>Den nyss skapade transaktionen kommer inte att skickas vidare till monero-nätverket</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="140"/>
        <source>Create an address file for new wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="142"/>
        <source>Display English language names</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="276"/>
        <source>failed to read wallet password</source>
        <translation type="unfinished">det gick inte att läsa lösenord för plånboken</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="283"/>
        <source>Enter a new password for the wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="283"/>
        <source>Wallet password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="293"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="485"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>daemonen är upptagen. Försök igen senare.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="302"/>
        <source>possibly lost connection to daemon</source>
        <translation>anslutning till daemonen kan ha förlorats</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="319"/>
        <source>Error: </source>
        <translation>Fel: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8959"/>
        <source>Failed to initialize wallet</source>
        <translation>Det gick inte att initiera plånbok</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="201"/>
        <source>Use daemon instance at &lt;host&gt;:&lt;port&gt;</source>
        <translation>Använd daemonen på &lt;värddator&gt;:&lt;port&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="202"/>
        <source>Use daemon instance at host &lt;arg&gt; instead of localhost</source>
        <translation>Använd daemonen på värddatorn &lt;arg&gt; istället för localhost</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="206"/>
        <source>Wallet password file</source>
        <translation>Lösenordsfil för plånboken</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="207"/>
        <source>Use daemon instance at port &lt;arg&gt; instead of 18081</source>
        <translation>Använd daemonen på port &lt;arg&gt; istället för 18081</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="209"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>För testnet. Daemonen måste också startas med flaggan --testnet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="282"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>det går inte ange värd eller port för daemonen mer än en gång</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="355"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>det går inte att ange fler än en av --password och --password-file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="368"/>
        <source>the password file specified could not be read</source>
        <translation>det gick inte att läsa angiven lösenordsfil</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="394"/>
        <source>Failed to load file </source>
        <translation>Det gick inte att läsa in fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="205"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation>Lösenord för plånboken (använd escape-sekvenser eller citattecken efter behov)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="203"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation type="unfinished">Aktivera kommandon som kräver en betrodd daemon</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="204"/>
        <source>Disable commands which rely on a trusted daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="208"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>Ange användarnamn[:lösenord] för RPC-klient till daemonen</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="210"/>
        <source>For stagenet. Daemon must also be launched with --stagenet flag</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="212"/>
        <source>Set shared ring database path</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="223"/>
        <source>Number of rounds for the key derivation function</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="224"/>
        <source>HW device to use</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="225"/>
        <source>HW device wallet derivation path (e.g., SLIP-10)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="313"/>
        <source>--trusted-daemon and --untrusted-daemon are both seen, assuming untrusted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="323"/>
        <source>Daemon is local, assuming trusted</source>
        <translation type="unfinished">Daemonen är lokal, utgår från att den är betrodd</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="375"/>
        <source>no password specified; use --prompt-for-password to prompt for a password</source>
        <translation>inget lösenord har angivits; använd --prompt-for-password för att fråga efter lösenord</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="377"/>
        <source>Enter a new password for the wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="377"/>
        <source>Wallet password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="400"/>
        <source>Failed to parse JSON</source>
        <translation>Det gick inte att parsa JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="407"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation>Version %u är för ny, vi förstår bara upp till %u</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="423"/>
        <source>failed to parse view key secret key</source>
        <translation>det gick inte att parsa hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="428"/>
        <location filename="../src/wallet/wallet2.cpp" line="496"/>
        <location filename="../src/wallet/wallet2.cpp" line="539"/>
        <source>failed to verify view key secret key</source>
        <translation>det gick inte att verifiera hemlig granskningsnyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="439"/>
        <source>failed to parse spend key secret key</source>
        <translation>det gick inte att parsa spendernyckel hemlig nyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="444"/>
        <location filename="../src/wallet/wallet2.cpp" line="506"/>
        <location filename="../src/wallet/wallet2.cpp" line="565"/>
        <source>failed to verify spend key secret key</source>
        <translation>det gick inte att verifiera spendernyckel hemlig nyckel</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="456"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Det gick inte att verifiera ordlista av Electrum-typ</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="476"/>
        <source>At least one of either an Electrum-style word list, private view key, or private spend key must be specified</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="480"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Både ordlista av Electrum-typ och privat nyckel har angivits</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="490"/>
        <source>invalid address</source>
        <translation>ogiltig adress</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="499"/>
        <source>view key does not match standard address</source>
        <translation>granskningsnyckel matchar inte standardadress</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="509"/>
        <source>spend key does not match standard address</source>
        <translation>spendernyckel matchar inte standardadress</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="517"/>
        <source>Cannot generate deprecated wallets from JSON</source>
        <translation>Det går inte att skapa inaktuella plånböcker från JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="551"/>
        <source>failed to parse address: </source>
        <translation>det gick inte att parsa adressen: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="557"/>
        <source>Address must be specified in order to create watch-only wallet</source>
        <translation>Adress måste anges för att kunna skapa granskningsplånbok</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="574"/>
        <source>failed to generate new wallet: </source>
        <translation>det gick inte att skapa ny plånbok: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="1382"/>
        <source>Password is needed to compute key image for incoming monero</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="1383"/>
        <source>Invalid password: password is needed to compute key image for incoming monero</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="3770"/>
        <location filename="../src/wallet/wallet2.cpp" line="4374"/>
        <location filename="../src/wallet/wallet2.cpp" line="4926"/>
        <source>Primary account</source>
        <translation>Primärt konto</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="10157"/>
        <source>No funds received in this tx.</source>
        <translation>Inga pengar togs emot i denna tx.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="10899"/>
        <source>failed to read file </source>
        <translation>det gick inte att läsa filen </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="141"/>
        <source>Set subaddress lookahead sizes to &lt;major&gt;:&lt;minor&gt;</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>tools::wallet_rpc_server</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="180"/>
        <source>Failed to create directory </source>
        <translation>Det gick inte att skapa mapp </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="182"/>
        <source>Failed to create directory %s: %s</source>
        <translation>Det gick inte att skapa mapp %s: %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="193"/>
        <source>Cannot specify --</source>
        <translation>Det går inte att ange --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="193"/>
        <source> and --</source>
        <translation> och --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="212"/>
        <source>Failed to create file </source>
        <translation>Det gick inte att skapa fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="212"/>
        <source>. Check permissions or remove file</source>
        <translation>. Kontrollera behörigheter eller ta bort filen</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="222"/>
        <source>Error writing to file </source>
        <translation>Ett fel uppstod vid skrivning till fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="225"/>
        <source>RPC username/password is stored in file </source>
        <translation>Användarnamn/lösenord för RPC har sparats i fil </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="479"/>
        <source>Tag %s is unregistered.</source>
        <translation>Taggen %s har inte registrerats.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3081"/>
        <source>Transaction not possible. Available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>Transaktion är inte möjlig. Endast tillgängligt %s, transaktionsbelopp %s = %s + %s (avgift)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3947"/>
        <source>This is the RPC monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation>Detta är RPC-plånboken för monero. Den måste ansluta till en Monero-
daemon för att fungera korrekt.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3788"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>Det går inte att ange fler än en av --wallet-file och --generate-from-json</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3773"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3800"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>Måste ange --wallet-file eller --generate-from-json eller --wallet-dir</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3804"/>
        <source>Loading wallet...</source>
        <translation>Läser in plånbok …</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3838"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3870"/>
        <source>Saving wallet...</source>
        <translation>Sparar plånbok …</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3840"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3872"/>
        <source>Successfully saved</source>
        <translation>Plånboken sparades</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3843"/>
        <source>Successfully loaded</source>
        <translation>Plånboken lästes in</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3847"/>
        <source>Wallet initialization failed: </source>
        <translation>Det gick inte att initiera plånbok: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3853"/>
        <source>Failed to initialize wallet RPC server</source>
        <translation>Det gick inte att initiera RPC-servern för plånbok</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3857"/>
        <source>Starting wallet RPC server</source>
        <translation>Startar RPC-server för plånboken</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3864"/>
        <source>Failed to run wallet: </source>
        <translation>Det gick inte att köra plånboken: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3867"/>
        <source>Stopped wallet RPC server</source>
        <translation>Stoppade RPC-server för plånboken</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3876"/>
        <source>Failed to save wallet: </source>
        <translation>Det gick inte spara plånboken: </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="168"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8908"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3928"/>
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
        <location filename="../src/wallet/wallet_args.cpp" line="105"/>
        <source>Max number of threads to use for a parallel job</source>
        <translation>Max antal trådar att använda för ett parallellt jobb</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="106"/>
        <source>Specify log file</source>
        <translation>Ange loggfil</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="107"/>
        <source>Config file</source>
        <translation>Konfigurationsfil</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="119"/>
        <source>General options</source>
        <translation>Allmänna alternativ</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="144"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation>Detta är kommandoradsplånboken för Monero. Den måste ansluta till en Monero-
daemon för att fungera korrekt.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="169"/>
        <source>Can&apos;t find config file </source>
        <translation>Det gick inte att hitta konfigurationsfilen </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="210"/>
        <source>Logging to: </source>
        <translation>Loggar till: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="212"/>
        <source>Logging to %s</source>
        <translation>Loggar till %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="216"/>
        <source>WARNING: You may not have a high enough lockable memory limit</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="218"/>
        <source>see ulimit -l</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="146"/>
        <source>Usage:</source>
        <translation>Användning:</translation>
    </message>
</context>
</TS>
