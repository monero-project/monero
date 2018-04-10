<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="it" sourcelanguage="en">
<context>
    <name>Monero::AddressBookImpl</name>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="53"/>
        <source>Invalid destination address</source>
        <translation>Indirizzo destinatario non valido</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="63"/>
        <source>Invalid payment ID. Short payment ID should only be used in an integrated address</source>
        <translation>ID pagamento non valido. Il pagamento ID corto dovrebbe essere usato solo in un indirizzo integrato</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="70"/>
        <source>Invalid payment ID</source>
        <translation>ID pagamento non valido</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="77"/>
        <source>Integrated address and long payment ID can&apos;t be used at the same time</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Monero::PendingTransactionImpl</name>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="90"/>
        <source>Attempting to save transaction to file, but specified file(s) exist. Exiting to not risk overwriting. File:</source>
        <translation>Sto tentando di salvare la transazione nel file, ma il file specificato esiste già. Sto uscendo per non rischiare di sovrascriverlo. File:</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="97"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Impossibile scrivere transazione/i su file</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="115"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>Il daemon è impegnato. Prova più tardi.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="118"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>Nessuna connessione con il daemon. Controlla che sia operativo.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="122"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>La transazione %s è stata respinta dal daemon con status: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="127"/>
        <source>. Reason: </source>
        <translation>. Motivo: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="129"/>
        <source>Unknown exception: </source>
        <translation>Eccezione sconosciuta: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="132"/>
        <source>Unhandled exception</source>
        <translation>Eccezione non gestita</translation>
    </message>
</context>
<context>
    <name>Monero::UnsignedTransactionImpl</name>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="75"/>
        <source>This is a watch only wallet</source>
        <translation>Questo è un portafoglio solo-vista</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="85"/>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="92"/>
        <source>Failed to sign transaction</source>
        <translation>Firma transazione fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="168"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>Il cambiamento richiesto non porta a un indirizzo pagato</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="174"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>Il cambiamento richiesto è più largo del pagamento all&apos;indirizzo di cambio</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="184"/>
        <source>Change goes to more than one address</source>
        <translation>Il cambiamento ha effetto su più di un indirizzo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="197"/>
        <source>sending %s to %s</source>
        <translation>inviando %s a %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="203"/>
        <source>with no destinations</source>
        <translation>senza destinazioni</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="209"/>
        <source>%s change to %s</source>
        <translation>%s cambia in %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="212"/>
        <source>no change</source>
        <translation>nessuna modifica</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="214"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu. %s</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Monero::WalletImpl</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1147"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>L&apos;id pagamento è in un formato invalido, dovrebbe essere una stringa hex di 16 o 64 caratteri: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1157"/>
        <source>Failed to add short payment id: </source>
        <translation>Impossibile aggiungere id pagamento corto</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1190"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1294"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>il daemon è impegnato. Prova più tardi</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1193"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1297"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>nessuna connessione con il daemon. Accertati che sia operativo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1196"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1300"/>
        <source>RPC error: </source>
        <translation>errore RPC: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1233"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1337"/>
        <source>not enough outputs for specified ring size</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1235"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1339"/>
        <source>found outputs to use</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1237"/>
        <source>Please sweep unmixable outputs.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1303"/>
        <source>failed to get random outputs to mix</source>
        <translation>impossibile recuperare outputs random da mixare</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1206"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1310"/>
        <source>not enough money to transfer, available only %s, sent amount %s</source>
        <translation>non hai abbastanza fondi da trasferire, sono disponibili solo %s, ammontare inviato %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="506"/>
        <source>failed to parse address</source>
        <translation>parsing indirizzo fallito</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="518"/>
        <source>failed to parse secret spend key</source>
        <translation>impossibile fare il parsing della chiave segreta di spesa</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="528"/>
        <source>No view key supplied, cancelled</source>
        <translation>Non è stata fornita nessuna chiave di visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="535"/>
        <source>failed to parse secret view key</source>
        <translation>impossibile fare il parsing della chiave segreta di visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="545"/>
        <source>failed to verify secret spend key</source>
        <translation>impossibile verificare chiave segreta di spesa</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="550"/>
        <source>spend key does not match address</source>
        <translation>la chiave di spesa non corrisponde all&apos;indirizzo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="556"/>
        <source>failed to verify secret view key</source>
        <translation>verifica chiave segreta di visualizzazione fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="561"/>
        <source>view key does not match address</source>
        <translation>la chiave di visualizzazione non corrisponde all&apos;indirizzo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="580"/>
        <source>failed to generate new wallet: </source>
        <translation>impossibile generare nuovo portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="809"/>
        <source>Failed to send import wallet request</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="955"/>
        <source>Failed to load unsigned transactions</source>
        <translation>Caricamento transazioni non firmate fallito</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="976"/>
        <source>Failed to load transaction from file</source>
        <translation>Caricamento transazione da file fallito</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="994"/>
        <source>Wallet is view only</source>
        <translation>Il portafoglio è di tipo solo visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1003"/>
        <source>failed to save file </source>
        <translation>impossibile salvare il file </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1022"/>
        <source>Key images can only be imported with a trusted daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1035"/>
        <source>Failed to import key images: </source>
        <translation>Impossibile importare immagini chiave: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1068"/>
        <source>Failed to get subaddress label: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1082"/>
        <source>Failed to set subaddress label: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1199"/>
        <source>failed to get random outputs to mix: %s</source>
        <translation>impossibile recuperare outputs casuali da mixare: %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1215"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1319"/>
        <source>not enough money to transfer, overall balance only %s, sent amount %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1224"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1328"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>non hai abbastanza fondi da trasferire, disponibili solo %s, ammontare transazione %s = %s + %s (commissione)</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1235"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1339"/>
        <source>output amount</source>
        <translation>ammontare output</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1241"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1344"/>
        <source>transaction was not constructed</source>
        <translation>transazione non costruita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1245"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1348"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transazione %s è stata rifiutata dal daemon con status: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1252"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1355"/>
        <source>one of destinations is zero</source>
        <translation>una delle destinazioni è zero</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1255"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1358"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>impossibile trovare un modo per dividere le transazioni</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1258"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1361"/>
        <source>unknown transfer error: </source>
        <translation>errore trasferimento sconosciuto: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1261"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1364"/>
        <source>internal error: </source>
        <translation>errore interno: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1264"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1367"/>
        <source>unexpected error: </source>
        <translation>errore inaspettato: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1267"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1370"/>
        <source>unknown error</source>
        <translation>errore sconosciuto</translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1466"/>
        <source>no tx keys found for this txid</source>
        <translation type="unfinished">nessuna chiave tx trovata per questo txid</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1486"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1496"/>
        <source>Failed to parse tx key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1506"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1538"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1569"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1657"/>
        <source>Failed to parse address</source>
        <translation type="unfinished">Parsing indirizzo fallito</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1663"/>
        <source>Address must not be a subaddress</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1886"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>&quot;Riscannerizza spesi&quot; può essere utilizzato solo da un daemon fidato</translation>
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
        <translation>Parsing indirizzo fallito</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="268"/>
        <source>Failed to parse key</source>
        <translation>Parsing chiave fallito</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="276"/>
        <source>failed to verify key</source>
        <translation>verifica chiave fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="286"/>
        <source>key does not match address</source>
        <translation>la chiave non corrisponde all&apos;indirizzo</translation>
    </message>
</context>
<context>
    <name>command_line</name>
    <message>
        <location filename="../src/common/command_line.cpp" line="57"/>
        <source>yes</source>
        <translation>sì</translation>
    </message>
    <message>
        <location filename="../src/common/command_line.cpp" line="71"/>
        <source>no</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>cryptonote::rpc_args</name>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="40"/>
        <source>Specify IP to bind RPC server</source>
        <translation>Specificare IP da associare al server RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="41"/>
        <source>Specify username[:password] required for RPC server</source>
        <translation>Specifica username[:password] richiesta per server RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="42"/>
        <source>Confirm rpc-bind-ip value is NOT a loopback (local) IP</source>
        <translation>Conferma valore rpc-bind-ip NON è un loopback IP (locale)</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="43"/>
        <source>Specify a comma separated list of origins to allow cross origin resource sharing</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="70"/>
        <source>Invalid IP address given for --</source>
        <translation>Indirizzo IP non valido dato per --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="78"/>
        <source> permits inbound unencrypted external connections. Consider SSH tunnel or SSL proxy instead. Override with --</source>
        <translation>permette connessioni esterne non criptate in entrata. Considera in alternativa un tunnel SSH o un proxy SSL. Sovrascrivi con --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="95"/>
        <source>Username specified with --</source>
        <translation>Nome utente specificato con --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="95"/>
        <location filename="../src/rpc/rpc_args.cpp" line="105"/>
        <source> cannot be empty</source>
        <translation> non può essere vuoto</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="105"/>
        <source> requires RFC server password --</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>cryptonote::simple_wallet</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="544"/>
        <source>Commands: </source>
        <translation>Comandi: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3598"/>
        <source>failed to read wallet password</source>
        <translation>impossibile leggere la password del portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3223"/>
        <source>invalid password</source>
        <translation>password non valida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2395"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation>imposta seed: richiede una definizione. opzioni disponibili: lingua</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2427"/>
        <source>set: unrecognized argument(s)</source>
        <translation>imposta: definizione/i non riconosciuta/e</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3456"/>
        <source>wallet file path not valid: </source>
        <translation>percorso file portafoglio non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2481"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation>Sto tentando di generare o ripristinare il portafoglio, ma i(l) file specificato/i esiste/esistono già. Sto uscendo per non rischiare di sovrascrivere.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="744"/>
        <source>usage: payment_id</source>
        <translation>uso: payment_id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2381"/>
        <source>needs an argument</source>
        <translation>ha bisogno di un argomento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2404"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2405"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2406"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2408"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2411"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2412"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2416"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2417"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2419"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2421"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2422"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2423"/>
        <source>0 or 1</source>
        <translation>0 o 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2410"/>
        <source>0, 1, 2, 3, or 4</source>
        <translation>0, 1, 2, 3, o 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2414"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2418"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2425"/>
        <source>unsigned integer</source>
        <translation>integrale non firmato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2535"/>
        <source>NOTE: the following 25 words can be used to recover access to your wallet. Write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation>ATTENZIONE: le seguenti 25 parole possono essere usate per ripristinare il tuo portafoglio. Scrivile e conservale da qualche parte al sicuro.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2627"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation>specificare un parametro di ripristino con --electrum-seed=&quot;lista parole qui&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3158"/>
        <source>wallet failed to connect to daemon: </source>
        <translation>impossibile connettere il portafoglio al daemon: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3166"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation>Il daemon usa una versione principale RPC (%u) diversa da quella del portafoglio (%u): %s. Aggiorna una delle due, o usa --allow-mismatched-daemon-version.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3185"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation>Lista delle lingue disponibili per il seed del tuo portafoglio:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3195"/>
        <source>Enter the number corresponding to the language of your choice: </source>
        <translation>Inserisci il numero corrispondente al linguaggio da te scelto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3268"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation>Hai usato una versione obsoleta del portafoglio. Per favore usa il nuovo seed che ti abbiamo fornito.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3284"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3352"/>
        <source>Generated new wallet: </source>
        <translation>Nuovo portafoglio generato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3290"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3357"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3393"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3445"/>
        <source>failed to generate new wallet: </source>
        <translation>impossibile generare nuovo portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3474"/>
        <source>Opened watch-only wallet</source>
        <translation>Portafoglio solo-vista aperto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3478"/>
        <source>Opened wallet</source>
        <translation>Portafoglio aperto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3491"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation>Stai utilizzando una versione disapprovata del portafoglio. Per favore procedi nell&apos;upgrade del portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3506"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation>Stai utilizzando una versione disapprovata del portafoglio. Il formato del tuo portafoglio sta venendo aggiornato adesso.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3514"/>
        <source>failed to load wallet: </source>
        <translation>impossibile caricare portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3531"/>
        <source>Use the &quot;help&quot; command to see the list of available commands.
</source>
        <translation>Usa il comando &quot;help&quot; per visualizzare la lista dei comandi disponibili.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3576"/>
        <source>Wallet data saved</source>
        <translation>Dati del portafoglio salvati</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3672"/>
        <source>Mining started in daemon</source>
        <translation>Mining avviato nel daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3674"/>
        <source>mining has NOT been started: </source>
        <translation>il mining NON è stato avviato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3694"/>
        <source>Mining stopped in daemon</source>
        <translation>Mining nel daemon interrotto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3696"/>
        <source>mining has NOT been stopped: </source>
        <translation>il mining NON è stato interrotto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3751"/>
        <source>Blockchain saved</source>
        <translation>Blockchain salvata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3766"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3784"/>
        <source>Height </source>
        <translation>Blocco </translation>
    </message>
    <message>
        <source>transaction </source>
        <translation type="vanished">transazione </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3786"/>
        <source>spent </source>
        <translation>speso/i </translation>
    </message>
    <message>
        <source>unsupported transaction format</source>
        <translation type="vanished">formato transazione non supportato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3812"/>
        <source>Starting refresh...</source>
        <translation>Sto iniziando il refresh...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3825"/>
        <source>Refresh done, blocks received: </source>
        <translation>Refresh finito, blocchi ricevuti: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4362"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4834"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>l&apos;id pagamento ha un formato invalido, dovrebbe essere una stringa hex di 16 o 64 caratteri: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4377"/>
        <source>bad locked_blocks parameter:</source>
        <translation>parametro locked_blocks non corretto:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4405"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4852"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5066"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation>una singola transazione non può usare più di un id pagamento: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4414"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4861"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5034"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5074"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>impossibile impostare id pagamento, anche se è stato decodificado correttamente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4439"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4520"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4591"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4700"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4875"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4933"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5088"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5131"/>
        <source>transaction cancelled.</source>
        <translation>transazione cancellata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4559"/>
        <source>Sending %s.  </source>
        <translation>Sto inviando %s. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4562"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation>La tua transazione deve essere divisa in %llu transazioni. Una commissione verrà applicata per ogni transazione, per un totale di %s commissioni</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4568"/>
        <source>The transaction fee is %s</source>
        <translation>La commissione per la transazione è %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4571"/>
        <source>, of which %s is dust from change</source>
        <translation>, della quale %s è polvere dovuta allo scambio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4572"/>
        <source>.</source>
        <translation>.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4572"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation>Un totale di %s in polvere verrà inviato all&apos;indirizzo della polvere</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4577"/>
        <source>.
This transaction will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation>.
Questa transazione verrà sbloccata al blocco %llu, in approssimativamente %s giorni (supponendo 2 minuti per blocco)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4603"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4615"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4711"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4723"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4944"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4956"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5141"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5153"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Impossibile scrivere transazione/i su file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4607"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4619"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4715"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4727"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4948"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4960"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5145"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5157"/>
        <source>Unsigned transaction(s) successfully written to file: </source>
        <translation>Transazioni/e non firmata/e scritte/a con successo su file: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4670"/>
        <source>No unmixable outputs found</source>
        <translation>Nessun output non-mixabile trovato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4753"/>
        <source>No address given</source>
        <translation>Non è stato fornito nessun indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5306"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>Il cambiamento richiesto non porta a un indirizzo pagato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5311"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>Il cambiamento richiesto è più largo del pagamento all&apos;indirizzo di cambio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5342"/>
        <source>sending %s to %s</source>
        <translation>sto mandando %s a %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5352"/>
        <source> dummy output(s)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5355"/>
        <source>with no destinations</source>
        <translation>senza destinazioni</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5367"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu, %s. %sIs this okay? (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5396"/>
        <source>This is a multisig wallet, it can only sign with sign_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5406"/>
        <source>usage: sign_transfer [export]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5418"/>
        <source>Failed to sign transaction</source>
        <translation>Impossibile firmare transazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5424"/>
        <source>Failed to sign transaction: </source>
        <translation>Impossibile firmare transazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5445"/>
        <source>Transaction raw hex data exported to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5466"/>
        <source>Failed to load transaction from file</source>
        <translation>Caricamento transazione da file fallito</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3841"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4144"/>
        <source>RPC error: </source>
        <translation>errore RPC: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="591"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation>il portafoglio è solo-vista e non ha una chiave di spesa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="716"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="872"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="945"/>
        <source>Your original password was incorrect.</source>
        <translation>La tua password originale era scorretta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="732"/>
        <source>Error with wallet rewrite: </source>
        <translation>Errore riscrittura wallet: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1680"/>
        <source>priority must be 0, 1, 2, 3, or 4 </source>
        <translation>la priorità deve essere 0, 1, 2, 3, or 4 </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1692"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1707"/>
        <source>priority must be 0, 1, 2, 3, or 4</source>
        <translation>la priorità deve essere 0, 1, 2, 3, or 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1795"/>
        <source>invalid unit</source>
        <translation>unità invalida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1813"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1875"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation>conteggio invalido: deve essere un integrale non firmato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1831"/>
        <source>invalid value</source>
        <translation>valore invalido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2436"/>
        <source>usage: set_log &lt;log_level_number_0-4&gt; | &lt;categories&gt;</source>
        <translation>uso: set_log &lt;log_level_number_0-4&gt; | &lt;categories&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2507"/>
        <source>(Y/Yes/N/No): </source>
        <translation>(S/Sì/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3024"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3051"/>
        <source>bad m_restore_height parameter: </source>
        <translation>parametro m_restore_height non corretto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3029"/>
        <source>date format must be YYYY-MM-DD</source>
        <translation>il formato della data deve essere YYYY-MM-DD</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3042"/>
        <source>Restore height is: </source>
        <translation>Ripristina altezza è: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3043"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4584"/>
        <source>Is this okay?  (Y/Yes/N/No): </source>
        <translation>Va bene? (S/Sì/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3095"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Il daemon è locale, viene considerato fidato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3594"/>
        <source>Password for new watch-only wallet</source>
        <translation>Password per il nuovo portafoglio solo-vista</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3663"/>
        <source>invalid arguments. Please use start_mining [&lt;number_of_threads&gt;] [do_bg_mining] [ignore_battery], &lt;number_of_threads&gt; should be from 1 to </source>
        <translation>argomenti invalidi. Usa start_mining [&lt;number_of_threads&gt;] [do_bg_mining] [ignore_battery], &lt;number_of_threads&gt; dovrebbe risultare da 1 a </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3851"/>
        <source>internal error: </source>
        <translation>errore interno: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1307"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3856"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4149"/>
        <source>unexpected error: </source>
        <translation>errore inaspettato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1236"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1312"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3861"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4154"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4634"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4742"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4975"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5174"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5479"/>
        <source>unknown error</source>
        <translation>errore sconosciuto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3866"/>
        <source>refresh failed: </source>
        <translation>refresh fallito: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3866"/>
        <source>Blocks received: </source>
        <translation>Blocchi ricevuti: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3897"/>
        <source>unlocked balance: </source>
        <translation>bilancio sbloccato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2415"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3993"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4044"/>
        <source>amount</source>
        <translation>ammontare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="253"/>
        <source>false</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="558"/>
        <source>Unknown command: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="565"/>
        <source>Command usage: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="568"/>
        <source>Command description: </source>
        <translation type="unfinished"></translation>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="5391"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5454"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5491"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5531"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5742"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5832"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6943"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7017"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7059"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7101"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7151"/>
        <source>command not supported by HW wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="629"/>
        <source>wallet is multisig but not yet finalized</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="642"/>
        <source>Enter optional seed encryption passphrase, empty to see raw seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="659"/>
        <source>Failed to retrieve seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="683"/>
        <source>wallet is multisig and has no seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="756"/>
        <source>Cannot connect to daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="761"/>
        <source>Current fee is %s monero per kB</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="777"/>
        <source>Error: failed to estimate backlog array size: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="782"/>
        <source>Error: bad estimated backlog array size</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="794"/>
        <source> (current)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="797"/>
        <source>%u block (%u minutes) backlog at priority %u%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="799"/>
        <source>%u to %u block (%u to %u minutes) backlog at priority %u</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="802"/>
        <source>No backlog at priority </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="816"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="854"/>
        <source>This wallet is already multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="821"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="859"/>
        <source>wallet is watch-only and cannot be made multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="827"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="865"/>
        <source>This wallet has been used before, please use a new wallet to create a multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="834"/>
        <source>Your password is incorrect.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="840"/>
        <source>Send this multisig info to all other participants, then use make_multisig &lt;threshold&gt; &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="841"/>
        <source>This includes the PRIVATE view key, so needs to be disclosed only to that multisig wallet&apos;s participants </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="878"/>
        <source>usage: make_multisig &lt;threshold&gt; &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="886"/>
        <source>Invalid threshold</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="899"/>
        <source>Another step is needed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="901"/>
        <source>Send this multisig info to all other participants, then use finalize_multisig &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="907"/>
        <source>Error creating multisig: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="914"/>
        <source>Error creating multisig: new wallet is not multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="917"/>
        <source> multisig address: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="933"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="982"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1034"/>
        <source>This wallet is not multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="938"/>
        <source>This wallet is already finalized</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="951"/>
        <source>usage: finalize_multisig &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="959"/>
        <source>Failed to finalize multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="965"/>
        <source>Failed to finalize multisig: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="987"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1039"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1118"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1191"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1258"/>
        <source>This multisig wallet is not yet finalized</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="992"/>
        <source>usage: export_multisig_info &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1015"/>
        <source>Error exporting multisig info: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1019"/>
        <source>Multisig info exported to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1044"/>
        <source>usage: import_multisig_info &lt;filename1&gt; [&lt;filename2&gt;...] - one for each other participant</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1072"/>
        <source>Multisig info imported</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1076"/>
        <source>Failed to import multisig info: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1087"/>
        <source>Failed to update spent status after importing multisig info: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1092"/>
        <source>Untrusted daemon, spent status may be incorrect. Use a trusted daemon and run &quot;rescan_spent&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1113"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1186"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1253"/>
        <source>This is not a multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1123"/>
        <source>usage: sign_multisig &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1136"/>
        <source>Failed to sign multisig transaction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1142"/>
        <source>Multisig error: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1147"/>
        <source>Failed to sign multisig transaction: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1170"/>
        <source>It may be relayed to the network with submit_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1196"/>
        <source>usage: submit_multisig &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1211"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1277"/>
        <source>Failed to load multisig transaction from file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1216"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1282"/>
        <source>Multisig transaction signed by only %u signers, needs %u more signatures</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1225"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7414"/>
        <source>Transaction successfully submitted, transaction </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1226"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7415"/>
        <source>You can check its status by using the `show_transfers` command.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1263"/>
        <source>usage: export_raw_multisig &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1298"/>
        <source>Failed to export multisig transaction to file </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1302"/>
        <source>Saved exported multisig transaction file(s): </source>
        <translation type="unfinished"></translation>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="1663"/>
        <source>ring size must be an integer &gt;= </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1668"/>
        <source>could not change default ring size</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1909"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1980"/>
        <source>Invalid height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2013"/>
        <source>start_mining [&lt;number_of_threads&gt;] [bg_mining] [ignore_battery]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2014"/>
        <source>Start mining in the daemon (bg_mining and ignore_battery are optional booleans).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2017"/>
        <source>Stop mining in the daemon.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2020"/>
        <source>set_daemon &lt;host&gt;[:&lt;port&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2021"/>
        <source>Set another daemon to connect to.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2024"/>
        <source>Save the current blockchain data.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2027"/>
        <source>Synchronize the transactions and balance.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2030"/>
        <source>balance [detail]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2031"/>
        <source>Show the wallet&apos;s balance of the currently selected account.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2034"/>
        <source>incoming_transfers [available|unavailable] [verbose] [index=&lt;N1&gt;[,&lt;N2&gt;[,...]]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2035"/>
        <source>Show the incoming transfers, all or filtered by availability and address index.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2038"/>
        <source>payments &lt;PID_1&gt; [&lt;PID_2&gt; ... &lt;PID_N&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2039"/>
        <source>Show the payments for the given payment IDs.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2042"/>
        <source>Show the blockchain height.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2045"/>
        <source>transfer_original [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2046"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; using an older transaction building algorithm. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2048"/>
        <source>transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2049"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt;. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2052"/>
        <source>locked_transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;addr&gt; &lt;amount&gt; &lt;lockblocks&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2053"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2056"/>
        <source>Send all unmixable outputs to yourself with ring_size 1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2058"/>
        <source>sweep_all [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2059"/>
        <source>Send all unlocked balance to an address. If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2062"/>
        <source>sweep_below &lt;amount_threshold&gt; [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2063"/>
        <source>Send all unlocked outputs below the threshold to an address.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2066"/>
        <source>sweep_single [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;key_image&gt; &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2067"/>
        <source>Send a single output of the given key image to an address without change.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2070"/>
        <source>donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2071"/>
        <source>Donate &lt;amount&gt; to the development team (donate.getmonero.org).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2074"/>
        <source>sign_transfer &lt;file&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2075"/>
        <source>Sign a transaction from a &lt;file&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2078"/>
        <source>Submit a signed transaction from a file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2081"/>
        <source>set_log &lt;level&gt;|{+,-,}&lt;categories&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2082"/>
        <source>Change the current log detail (level must be &lt;0-4&gt;).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2085"/>
        <source>account
  account new &lt;label text with white spaces allowed&gt;
  account switch &lt;index&gt; 
  account label &lt;index&gt; &lt;label text with white spaces allowed&gt;
  account tag &lt;tag_name&gt; &lt;account_index_1&gt; [&lt;account_index_2&gt; ...]
  account untag &lt;account_index_1&gt; [&lt;account_index_2&gt; ...]
  account tag_description &lt;tag_name&gt; &lt;description&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2092"/>
        <source>If no arguments are specified, the wallet shows all the existing accounts along with their balances.
If the &quot;new&quot; argument is specified, the wallet creates a new account with its label initialized by the provided label text (which can be empty).
If the &quot;switch&quot; argument is specified, the wallet switches to the account specified by &lt;index&gt;.
If the &quot;label&quot; argument is specified, the wallet sets the label of the account specified by &lt;index&gt; to the provided label text.
If the &quot;tag&quot; argument is specified, a tag &lt;tag_name&gt; is assigned to the specified accounts &lt;account_index_1&gt;, &lt;account_index_2&gt;, ....
If the &quot;untag&quot; argument is specified, the tags assigned to the specified accounts &lt;account_index_1&gt;, &lt;account_index_2&gt; ..., are removed.
If the &quot;tag_description&quot; argument is specified, the tag &lt;tag_name&gt; is assigned an arbitrary text &lt;description&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2101"/>
        <source>address [ new &lt;label text with white spaces allowed&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;label text with white spaces allowed&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2105"/>
        <source>integrated_address [&lt;payment_id&gt; | &lt;address&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2106"/>
        <source>Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2109"/>
        <source>address_book [(add ((&lt;address&gt; [pid &lt;id&gt;])|&lt;integrated address&gt;) [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2110"/>
        <source>Print all entries in the address book, optionally adding/deleting an entry to/from it.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2113"/>
        <source>Save the wallet data.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2116"/>
        <source>Save a watch-only keys file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2119"/>
        <source>Display the private view key.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2122"/>
        <source>Display the private spend key.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2125"/>
        <source>Display the Electrum-style mnemonic seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2128"/>
        <source>set &lt;option&gt; [&lt;value&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2175"/>
        <source>Display the encrypted Electrum-style mnemonic seed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2178"/>
        <source>Rescan the blockchain for spent outputs.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2181"/>
        <source>get_tx_key &lt;txid&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2182"/>
        <source>Get the transaction key (r) for a given &lt;txid&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2185"/>
        <source>check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2186"/>
        <source>Check the amount going to &lt;address&gt; in &lt;txid&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2189"/>
        <source>get_tx_proof &lt;txid&gt; &lt;address&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2190"/>
        <source>Generate a signature proving funds sent to &lt;address&gt; in &lt;txid&gt;, optionally with a challenge string &lt;message&gt;, using either the transaction secret key (when &lt;address&gt; is not your wallet&apos;s address) or the view secret key (otherwise), which does not disclose the secret key.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2193"/>
        <source>check_tx_proof &lt;txid&gt; &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2194"/>
        <source>Check the proof for funds going to &lt;address&gt; in &lt;txid&gt; with the challenge string &lt;message&gt; if any.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2197"/>
        <source>get_spend_proof &lt;txid&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2198"/>
        <source>Generate a signature proving that you generated &lt;txid&gt; using the spend secret key, optionally with a challenge string &lt;message&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2201"/>
        <source>check_spend_proof &lt;txid&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2202"/>
        <source>Check a signature proving that the signer generated &lt;txid&gt;, optionally with a challenge string &lt;message&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2205"/>
        <source>get_reserve_proof (all|&lt;amount&gt;) [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2206"/>
        <source>Generate a signature proving that you own at least this much, optionally with a challenge string &lt;message&gt;.
If &apos;all&apos; is specified, you prove the entire sum of all of your existing accounts&apos; balances.
Otherwise, you prove the reserve of the smallest possible amount above &lt;amount&gt; available in your current account.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2211"/>
        <source>check_reserve_proof &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2212"/>
        <source>Check a signature proving that the owner of &lt;address&gt; holds at least this much, optionally with a challenge string &lt;message&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2215"/>
        <source>show_transfers [in|out|pending|failed|pool] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2216"/>
        <source>Show the incoming/outgoing transfers within an optional height range.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2219"/>
        <source>unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_amount&gt; [&lt;max_amount&gt;]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2220"/>
        <source>Show the unspent outputs of a specified address within an optional amount range.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2223"/>
        <source>Rescan the blockchain from scratch.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2226"/>
        <source>set_tx_note &lt;txid&gt; [free text note]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2227"/>
        <source>Set an arbitrary string note for a &lt;txid&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2230"/>
        <source>get_tx_note &lt;txid&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2231"/>
        <source>Get a string note for a txid.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2234"/>
        <source>set_description [free text note]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2235"/>
        <source>Set an arbitrary description for the wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2238"/>
        <source>Get the description of the wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2241"/>
        <source>Show the wallet&apos;s status.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2244"/>
        <source>Show the wallet&apos;s information.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2247"/>
        <source>sign &lt;file&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2248"/>
        <source>Sign the contents of a file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2251"/>
        <source>verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2252"/>
        <source>Verify a signature on the contents of a file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2255"/>
        <source>export_key_images &lt;file&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2256"/>
        <source>Export a signed set of key images to a &lt;file&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2259"/>
        <source>import_key_images &lt;file&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2260"/>
        <source>Import a signed key images list and verify their spent status.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2263"/>
        <source>export_outputs &lt;file&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2264"/>
        <source>Export a set of outputs owned by this wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2267"/>
        <source>import_outputs &lt;file&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2268"/>
        <source>Import a set of outputs owned by this wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2271"/>
        <source>show_transfer &lt;txid&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2272"/>
        <source>Show information about a transfer to/from this address.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2275"/>
        <source>Change the wallet&apos;s password.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2278"/>
        <source>Generate a new random full size payment id. These will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2281"/>
        <source>Print the information about the current fee and transaction backlog.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2283"/>
        <source>Export data needed to create a multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2285"/>
        <source>make_multisig &lt;threshold&gt; &lt;string1&gt; [&lt;string&gt;...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2286"/>
        <source>Turn this wallet into a multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2289"/>
        <source>finalize_multisig &lt;string&gt; [&lt;string&gt;...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2290"/>
        <source>Turn this wallet into a multisig wallet, extra step for N-1/N wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2293"/>
        <source>export_multisig_info &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2294"/>
        <source>Export multisig info for other participants</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2297"/>
        <source>import_multisig_info &lt;filename&gt; [&lt;filename&gt;...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2298"/>
        <source>Import multisig info from other participants</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2301"/>
        <source>sign_multisig &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2302"/>
        <source>Sign a multisig transaction from a file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2305"/>
        <source>submit_multisig &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2306"/>
        <source>Submit a signed multisig transaction from a file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2309"/>
        <source>export_raw_multisig_tx &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2310"/>
        <source>Export a signed multisig transaction to a file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2313"/>
        <source>print_ring &lt;key_image&gt; | &lt;txid&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2314"/>
        <source>Print the ring(s) used to spend a given key image or transaction (if the ring size is &gt; 1)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2317"/>
        <source>set_ring &lt;key_image&gt; absolute|relative &lt;index&gt; [&lt;index&gt;...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2318"/>
        <source>Set the ring used for a given key image, so it can be reused in a fork</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2321"/>
        <source>save_known_rings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2322"/>
        <source>Save known rings to the shared rings database</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2325"/>
        <source>blackball &lt;output public key&gt; | &lt;filename&gt; [add]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2326"/>
        <source>Blackball output(s) so they never get selected as fake outputs in a ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2329"/>
        <source>unblackball &lt;output public key&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2330"/>
        <source>Unblackballs an output so it may get selected as a fake output in a ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2333"/>
        <source>blackballed &lt;output public key&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2334"/>
        <source>Checks whether an output is blackballed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2337"/>
        <source>help [&lt;command&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2338"/>
        <source>Show the help section or the documentation about a &lt;command&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2407"/>
        <source>integer &gt;= </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2420"/>
        <source>block height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2424"/>
        <source>&lt;major&gt;:&lt;minor&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2506"/>
        <source>No wallet found with that name. Confirm creation of new wallet named: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2590"/>
        <source>can&apos;t specify both --restore-deterministic-wallet or --restore-multisig-wallet and --non-deterministic</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2596"/>
        <source>--restore-multisig-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2612"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;multisig seed here&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2639"/>
        <source>Multisig seed failed verification</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2652"/>
        <source>Enter seed encryption passphrase, empty if none</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2688"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2762"/>
        <source>This address is a subaddress which cannot be used here.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2840"/>
        <source>Error: expected M/N, but got: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2845"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2850"/>
        <source>Error: M/N is currently unsupported. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2853"/>
        <source>Generating master wallet from %u of %u multisig wallet keys</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2882"/>
        <source>failed to parse secret view key</source>
        <translation type="unfinished">impossibile fare il parsing della chiave segreta di visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2891"/>
        <source>failed to verify secret view key</source>
        <translation type="unfinished">verifica chiave segreta di visualizzazione fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2911"/>
        <source>Secret spend key (%u of %u):</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2935"/>
        <source>Error: M/N is currently unsupported</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3065"/>
        <source>Restore height </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3066"/>
        <source>Still apply restore height?  (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3078"/>
        <source>can&apos;t specify --subaddress-lookahead and --wallet-file at the same time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3102"/>
        <source>Warning: using an untrusted daemon at %s, privacy will be lessened</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3159"/>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or change the daemon address using the &apos;set_daemon&apos; command.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3186"/>
        <source>If your display freezes, exit blind with ^C, then run again with --use-english-language-names</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3301"/>
        <source>Your wallet has been generated!
To start synchronizing with the daemon, use the &quot;refresh&quot; command.
Use the &quot;help&quot; command to see the list of available commands.
Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
Always use the &quot;exit&quot; command when closing monero-wallet-cli to save 
your current session&apos;s state. Otherwise, you might need to synchronize 
your wallet again (your wallet keys are NOT at risk in any case).
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3388"/>
        <source>Generated new on device wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3437"/>
        <source>failed to generate new mutlisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3440"/>
        <source>Generated new %u/%u multisig wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3476"/>
        <source>Opened %u/%u multisig wallet%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3532"/>
        <source>Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3590"/>
        <source>wallet is multisig and cannot save a watch-only version</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3606"/>
        <source>Watch only wallet saved as: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3610"/>
        <source>Failed to save watch only wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3706"/>
        <source>missing daemon URL argument</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3717"/>
        <source>Unexpected array length - Exited simple_wallet::set_daemon()</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3731"/>
        <source>This does not seem to be a valid daemon URL.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3767"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3785"/>
        <source>txid </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3769"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3787"/>
        <source>idx </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3892"/>
        <source> (Some owned outputs have partial key images - import_multisig_info needed)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3893"/>
        <source>Currently selected account: [</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3893"/>
        <source>] </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3895"/>
        <source>Tag: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3895"/>
        <source>(No tag assigned)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3902"/>
        <source>Balance per address:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3903"/>
        <source>Address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3903"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6554"/>
        <source>Balance</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3903"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6554"/>
        <source>Unlocked balance</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3903"/>
        <source>Outputs</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3903"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6554"/>
        <source>Label</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3911"/>
        <source>%8u %6s %21s %21s %7u %21s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3920"/>
        <source>usage: balance [detail]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3932"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3974"/>
        <source>usage: incoming_transfers [available|unavailable] [verbose] [index=&lt;N&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3993"/>
        <source>spent</source>
        <translation>spesi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3993"/>
        <source>global index</source>
        <translation>indice globale</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3993"/>
        <source>tx id</source>
        <translation>tx id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3993"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4044"/>
        <source>addr index</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4016"/>
        <source>No incoming transfers</source>
        <translation>Nessun trasferimento in entrata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4020"/>
        <source>No incoming available transfers</source>
        <translation>Nessun trasferimento in entrata disponibile</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4024"/>
        <source>No incoming unavailable transfers</source>
        <translation>Nessun trasferimento indisponibile in entrata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4035"/>
        <source>expected at least one payment ID</source>
        <translation>deve esserci almeno un payment ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4044"/>
        <source>payment</source>
        <translation>pagamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4044"/>
        <source>transaction</source>
        <translation>transazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4044"/>
        <source>height</source>
        <translation>altezza</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4044"/>
        <source>unlock time</source>
        <translation>tempo sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4056"/>
        <source>No payments with id </source>
        <translation>Nessun pagamento con id </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4109"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4175"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4457"/>
        <source>failed to get blockchain height: </source>
        <translation>impossibile recuperare altezza blockchain: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4165"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5765"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5803"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5860"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5893"/>
        <source>failed to connect to the daemon</source>
        <translation>impossibile connettersi al daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4183"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation>
Transazione %llu/%llu: txid=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4199"/>
        <source>failed to find construction data for tx input</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4204"/>
        <source>
Input %llu/%llu: amount=%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4220"/>
        <source>failed to get output: </source>
        <translation>impossibile recuperare output: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4228"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation>l&apos;altezza del blocco di origine della chiave di output non dovrebbe essere più alta dell&apos;altezza della blockchain</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4232"/>
        <source>
Originating block heights: </source>
        <translation>
Originando blocchi: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4247"/>
        <source>
|</source>
        <translation>
|</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4247"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6285"/>
        <source>|
</source>
        <translation>|
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4264"/>
        <source>
Warning: Some input keys being spent are from </source>
        <translation>
Avviso: alcune chiavi di input spese vengono da </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4266"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation>, che potrebbe compromettere l&apos;anonimità della ring signature. Assicurati di farlo intenzionalmente!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4309"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4788"/>
        <source>Ring size must not be 0</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4321"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4800"/>
        <source>ring size %u is too small, minimum is %u</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4328"/>
        <source>wrong number of arguments</source>
        <translation>numero di argomenti errato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4434"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4870"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5083"/>
        <source>No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): </source>
        <translation>Nessun id pagamento è incluso in questa transazione. Questo è corretto? (S/Sì/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4476"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4890"/>
        <source>No outputs found, or daemon is not ready</source>
        <translation>Nessun output trovato, o il daemon non è pronto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5205"/>
        <source>donations are not enabled on the testnet or on the stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6933"/>
        <source>Network type: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6934"/>
        <source>Testnet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6935"/>
        <source>Stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6935"/>
        <source>Mainnet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7407"/>
        <source>Transaction successfully saved to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7407"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7409"/>
        <source>, txid </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7409"/>
        <source>Failed to save transaction to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4685"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4918"/>
        <source>Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Sto eseguendo lo sweep di %s nelle transazioni %llu per un totale commissioni di %s.  Va bene?  (S/Sì/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4691"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4924"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5123"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Sto eseguendo lo sweep di %s per un totale commissioni di %s.  Va bene?  (S/Sì/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5234"/>
        <source>Donating </source>
        <translation>Donando </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5401"/>
        <source>This is a watch only wallet</source>
        <translation>Questo è un portafoglio solo-vista</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7235"/>
        <source>usage: show_transfer &lt;txid&gt;</source>
        <translation>uso: show_transfer &lt;txid&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7337"/>
        <source>Double spend seen on the network: this transaction may or may not end up being mined</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7372"/>
        <source>Transaction ID not found</source>
        <translation>ID transazione non trovato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="248"/>
        <source>true</source>
        <translation>vero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="301"/>
        <source>failed to parse refresh type</source>
        <translation>impossibile fare il parsing del tipo di refresh</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="619"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="688"/>
        <source>wallet is watch-only and has no seed</source>
        <translation>il portafoglio è solo-vista e non possiede un seed</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="635"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="693"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation>il portafoglio è non-deterministico e non possiede un seed</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1617"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1636"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation>il portafoglio è solo-vista e non può eseguire trasferimenti</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1712"/>
        <source>could not change default priority</source>
        <translation>impossibile cambiare priorità standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2102"/>
        <source>If no arguments are specified or &lt;index&gt; is specified, the wallet shows the default or specified address. If &quot;all&quot; is specified, the wallet shows all the existing addresses in the currently selected account. If &quot;new &quot; is specified, the wallet creates a new address with the provided label text (which can be empty). If &quot;label&quot; is specified, the wallet sets the label of the address specified by &lt;index&gt; to the provided label text.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2129"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="2409"/>
        <source>full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)</source>
        <translation>completo (più lento, nessuna ipotesi); optimize-coinbase (veloce, ipotizza che l&apos;intero coinbase viene pagato ad un indirizzo singolo); no-coinbase (il più veloce, ipotizza di non ricevere una transazione coinbase), default (come optimize-coinbase)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2413"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation>monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2469"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation>Nome del portafoglio non valido. Prova di nuovo o usa Ctrl-C per uscire</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2486"/>
        <source>Wallet and key files found, loading...</source>
        <translation>Portafoglio e chiavi trovate, sto caricando...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2492"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation>Ho trovato la chiave ma non il portafoglio. Sto rigenerando...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2498"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation>Chiave non trovata. Impossibile aprire portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2517"/>
        <source>Generating new wallet...</source>
        <translation>Sto generando un nuovo portafoglio...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2559"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2571"/>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-device=&quot;wallet_name&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2598"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-deterministic-wallet usa --generate-new-wallet, non --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2647"/>
        <source>Electrum-style word list failed verification</source>
        <translation>La lista di parole stile Electrum ha fallito la verifica</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2677"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2697"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2732"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2751"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2771"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2787"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2835"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2860"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2876"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2916"/>
        <source>No data supplied, cancelled</source>
        <translation>Nessun dato fornito, cancellato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2683"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2757"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2866"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4395"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4844"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5058"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5550"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5618"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5682"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5900"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6739"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6997"/>
        <source>failed to parse address</source>
        <translation>impossibile fare il parsing dell&apos;indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2703"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2793"/>
        <source>failed to parse view key secret key</source>
        <translation>impossibile fare il parsing chiave di visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2713"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2811"/>
        <source>failed to verify view key secret key</source>
        <translation>impossibile verificare chiave di visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2717"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2815"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2896"/>
        <source>view key does not match standard address</source>
        <translation>la chiave di visualizzazione non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2722"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2741"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2819"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2953"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2976"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2990"/>
        <source>account creation failed</source>
        <translation>creazione dell&apos;account fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2737"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2777"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2921"/>
        <source>failed to parse spend key secret key</source>
        <translation>impossibile fare il parsing chiave di spesa chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2803"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2942"/>
        <source>failed to verify spend key secret key</source>
        <translation>impossibile verificare chiave di spesa chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2807"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2947"/>
        <source>spend key does not match standard address</source>
        <translation>la chiave di spesa non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2981"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation>specifica un percorso per il portafoglio con --generate-new-wallet (non --wallet-file)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3082"/>
        <source>failed to open account</source>
        <translation>impossibile aprire account</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3086"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3630"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3685"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3743"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5586"/>
        <source>wallet is null</source>
        <translation>il portafoglio è nullo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3204"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3209"/>
        <source>invalid language choice entered. Please try again.
</source>
        <translation>linguaggio selezionato scorretto. Prova di nuovo.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3286"/>
        <source>View key: </source>
        <translation>Chiave di visualizzazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3525"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation>Potresti voler rimuovere il file &quot;%s&quot; e provare di nuovo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3553"/>
        <source>failed to deinitialize wallet</source>
        <translation>deinizializzazione portafoglio fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3621"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4117"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7064"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation>questo comando richiede un daemon fidato. Abilita questa opzione con --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3753"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation>impossibile salvare la blockchain: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3832"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4131"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>il daemon è impegnato. Prova più tardi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3836"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4135"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>nessuna connessione con il daemon. Assicurati che sia in funzione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3846"/>
        <source>refresh error: </source>
        <translation>errore refresh: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3896"/>
        <source>Balance: </source>
        <translation>Bilancio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3992"/>
        <source>pubkey</source>
        <translation>pubkey</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3992"/>
        <source>key image</source>
        <translation>immagine chiave</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3993"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4003"/>
        <source>unlocked</source>
        <translation>sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3993"/>
        <source>ringct</source>
        <translation>ringct</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4002"/>
        <source>T</source>
        <translation>T</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4002"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4003"/>
        <source>locked</source>
        <translation>bloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4004"/>
        <source>RingCT</source>
        <translation>RingCT</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4004"/>
        <source>-</source>
        <translation>-</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4078"/>
        <source>payment ID has invalid format, expected 16 or 64 character hex string: </source>
        <translation>l&apos;id pagamento è in un formato invalido, dovrebbe essere una stringa hex di 16 o 64 caratteri</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4139"/>
        <source>failed to get spent status</source>
        <translation>impossibile recuperare status spesi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4265"/>
        <source>the same transaction</source>
        <translation>la stessa transazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4265"/>
        <source>blocks that are temporally very close</source>
        <translation>i blocchi che sono temporalmente molto vicini</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4382"/>
        <source>Locked blocks too high, max 1000000 (˜4 yrs)</source>
        <translation>I blocchi bloccati sono troppo alti, max 1000000 (˜4 anni)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4499"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4509"/>
        <source>Is this okay anyway?  (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4504"/>
        <source>There is currently a %u block backlog at that fee level. Is this okay?  (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4509"/>
        <source>Failed to check for backlog: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4550"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4906"/>
        <source>
Transaction </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4555"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4911"/>
        <source>Spending from address index %d
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4557"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4913"/>
        <source>WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5028"/>
        <source>failed to parse Payment ID</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5044"/>
        <source>usage: sweep_single [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;key_image&gt; &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5051"/>
        <source>failed to parse key image</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5103"/>
        <source>No outputs found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5108"/>
        <source>Multiple transactions are created, which is not supposed to happen</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5113"/>
        <source>The transaction uses multiple or no inputs, which is not supposed to happen</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5190"/>
        <source>missing threshold amount</source>
        <translation>manca la soglia massima dell&apos;ammontare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5195"/>
        <source>invalid amount threshold</source>
        <translation>ammontare soglia invalido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5212"/>
        <source>usage: donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5320"/>
        <source>Change goes to more than one address</source>
        <translation>Il cambiamento va a più di un indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5701"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5817"/>
        <source>Good signature</source>
        <translation>Firma valida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5728"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5819"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5927"/>
        <source>Bad signature</source>
        <translation>Firma invalida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6679"/>
        <source>usage: integrated_address [payment ID]</source>
        <translation>uso: integrated_address [ID pagamento]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6715"/>
        <source>Standard address: </source>
        <translation>Indirizzo standard: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6720"/>
        <source>failed to parse payment ID or address</source>
        <translation>impossibile fare il parsing di ID pagamento o indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6731"/>
        <source>usage: address_book [(add (&lt;address&gt; [pid &lt;long or short payment id&gt;])|&lt;integrated address&gt; [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</source>
        <translation>uso: address_book [(add (&lt;address&gt; [pid &lt;long or short payment id&gt;])|&lt;integrated address&gt; [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6761"/>
        <source>failed to parse payment ID</source>
        <translation>impossibile fare il parsing di ID pagamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6779"/>
        <source>failed to parse index</source>
        <translation>impossibile fare il parsing dell&apos;indice</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6787"/>
        <source>Address book is empty.</source>
        <translation>La rubrica è vuota.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6793"/>
        <source>Index: </source>
        <translation>Indice: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6794"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6924"/>
        <source>Address: </source>
        <translation>Indirizzo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6795"/>
        <source>Payment ID: </source>
        <translation>ID Pagamento: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6796"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6923"/>
        <source>Description: </source>
        <translation>Descrizione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6806"/>
        <source>usage: set_tx_note [txid] free text note</source>
        <translation>uso: set_tx_note [txid] free text note</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6834"/>
        <source>usage: get_tx_note [txid]</source>
        <translation>uso: get_tx_note [txid]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6948"/>
        <source>usage: sign &lt;filename&gt;</source>
        <translation>uso: sign &lt;filename&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6953"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation>il portafoglio è solo-vista e non può firmare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1058"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6967"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6990"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7165"/>
        <source>failed to read file </source>
        <translation>impossibile leggere il file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5663"/>
        <source>usage: check_tx_proof &lt;txid&gt; &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5690"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5810"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5912"/>
        <source>failed to load signature file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5746"/>
        <source>usage: get_spend_proof &lt;txid&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5752"/>
        <source>wallet is watch-only and cannot generate the proof</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5790"/>
        <source>usage: check_spend_proof &lt;txid&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5836"/>
        <source>usage: get_reserve_proof (all|&lt;amount&gt;) [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5842"/>
        <source>The reserve proof can be generated only by a full wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5887"/>
        <source>usage: check_reserve_proof &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5905"/>
        <source>Address must not be a subaddress</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5923"/>
        <source>Good signature -- total: %s, spent: %s, unspent: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5987"/>
        <source>usage: show_transfers [in|out|all|pending|failed] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6124"/>
        <source>[Double spend seen on the network: this transaction may or may not end up being mined] </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6160"/>
        <source>usage: unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_amount&gt; [&lt;max_amount&gt;]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6220"/>
        <source>There is no unspent output in the specified address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6332"/>
        <source> (no daemon)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6334"/>
        <source> (out of sync)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6391"/>
        <source>(Untitled account)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6404"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6422"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6447"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6470"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6623"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6646"/>
        <source>failed to parse index: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6409"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6628"/>
        <source>specify an index between 0 and </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6506"/>
        <source>usage:
  account
  account new &lt;label text with white spaces allowed&gt;
  account switch &lt;index&gt;
  account label &lt;index&gt; &lt;label text with white spaces allowed&gt;
  account tag &lt;tag_name&gt; &lt;account_index_1&gt; [&lt;account_index_2&gt; ...]
  account untag &lt;account_index_1&gt; [&lt;account_index_2&gt; ...]
  account tag_description &lt;tag_name&gt; &lt;description&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6534"/>
        <source>
Grand total:
  Balance: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6534"/>
        <source>, unlocked balance: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6542"/>
        <source>Untagged accounts:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6548"/>
        <source>Tag %s is unregistered.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6551"/>
        <source>Accounts with tag: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6552"/>
        <source>Tag&apos;s description: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6554"/>
        <source>Account</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6560"/>
        <source> %c%8u %6s %21s %21s %21s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6570"/>
        <source>----------------------------------------------------------------------------------</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6571"/>
        <source>%15s %21s %21s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6594"/>
        <source>Primary address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6594"/>
        <source>(used)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6615"/>
        <source>(Untitled address)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6655"/>
        <source>&lt;index_min&gt; is already out of bound</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6660"/>
        <source>&lt;index_max&gt; exceeds the bound</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6668"/>
        <source>usage: address [ new &lt;label text with white spaces allowed&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;label text with white spaces allowed&gt; ]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6686"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6698"/>
        <source>Integrated addresses can only be created for account 0</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6710"/>
        <source>Integrated address: %s, payment ID: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6715"/>
        <source>Subaddress: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6875"/>
        <source>usage: get_description</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6881"/>
        <source>no description found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6883"/>
        <source>description found: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6922"/>
        <source>Filename: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6927"/>
        <source>Watch only</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6929"/>
        <source>%u/%u multisig%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6931"/>
        <source>Normal</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6932"/>
        <source>Type: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6958"/>
        <source>This wallet is multisig and cannot sign</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6979"/>
        <source>usage: verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation>uso: verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7004"/>
        <source>Bad signature from </source>
        <translation>Firma non valida da </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7008"/>
        <source>Good signature from </source>
        <translation>Firma valida da </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7022"/>
        <source>usage: export_key_images &lt;filename&gt;</source>
        <translation>uso: export_key_images &lt;filename&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7027"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation>il portafoglio è solo-vista e non può esportare immagini chiave</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1008"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7040"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7132"/>
        <source>failed to save file </source>
        <translation>impossibile salvare file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7051"/>
        <source>Signed key images exported to </source>
        <translation>Chiave immagine firmata esportata in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7070"/>
        <source>usage: import_key_images &lt;filename&gt;</source>
        <translation>uso: import_key_images &lt;filename&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7106"/>
        <source>usage: export_outputs &lt;filename&gt;</source>
        <translation>uso: export_outputs &lt;filename&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7143"/>
        <source>Outputs exported to </source>
        <translation>Outputs esportati in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7156"/>
        <source>usage: import_outputs &lt;filename&gt;</source>
        <translation>uso: import_outputs &lt;filename&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4423"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5853"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6179"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6187"/>
        <source>amount is wrong: </source>
        <translation>l&apos;ammontare non è corretto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4424"/>
        <source>expected number from 0 to </source>
        <translation>deve essere un numero da 0 a </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4683"/>
        <source>Sweeping </source>
        <translation>Eseguendo lo sweeping </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5163"/>
        <source>Money successfully sent, transaction: </source>
        <translation>Fondi inviati con successo, transazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5361"/>
        <source>%s change to %s</source>
        <translation>%s cambia in %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5364"/>
        <source>no change</source>
        <translation>nessun cambiamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1156"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1169"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5435"/>
        <source>Transaction successfully signed to file </source>
        <translation>Transazione firmata con successo nel file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5495"/>
        <source>usage: get_tx_key &lt;txid&gt;</source>
        <translation>uso: get_tx_key &lt;txid&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5503"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5543"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5592"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5674"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5759"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5797"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6813"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6841"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7242"/>
        <source>failed to parse txid</source>
        <translation>parsing txid fallito</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5517"/>
        <source>Tx key: </source>
        <translation>Chiave Tx: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5522"/>
        <source>no tx keys found for this txid</source>
        <translation>nessuna chiave tx trovata per questo txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5536"/>
        <source>usage: get_tx_proof &lt;txid&gt; &lt;address&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5561"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5776"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5873"/>
        <source>signature file saved to: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5563"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5778"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5875"/>
        <source>failed to save signature file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5577"/>
        <source>usage: check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</source>
        <translation>uso: check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5600"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5609"/>
        <source>failed to parse tx key</source>
        <translation>impossibile fare il parsing della chiave tx</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5567"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5655"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5733"/>
        <source>error: </source>
        <translation>errore: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5631"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5704"/>
        <source>received</source>
        <translation>ricevuto/i</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5631"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5704"/>
        <source>in txid</source>
        <translation>in txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5650"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5723"/>
        <source>received nothing in txid</source>
        <translation>nulla ricevuto in txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5634"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5707"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation>AVVISO: questa transazione non è ancora inclusa nella blockchain!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5640"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5713"/>
        <source>This transaction has %u confirmations</source>
        <translation>Questa transazione ha %u conferme</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5644"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5717"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation>AVVISO: impossibile determinare il numero di conferme!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6035"/>
        <source>bad min_height parameter:</source>
        <translation>parametro min_height non corretto:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6047"/>
        <source>bad max_height parameter:</source>
        <translation>parametro max_height non corretto:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6107"/>
        <source>in</source>
        <translation>in</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6107"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6148"/>
        <source>out</source>
        <translation>out</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6148"/>
        <source>failed</source>
        <translation>fallito</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6148"/>
        <source>pending</source>
        <translation>in attesa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6194"/>
        <source>&lt;min_amount&gt; should be smaller than &lt;max_amount&gt;</source>
        <translation>&lt;min_amount&gt; dovrebbe essere più piccolo di &lt;max_amount&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6226"/>
        <source>
Amount: </source>
        <translation>
Ammontare: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6226"/>
        <source>, number of keys: </source>
        <translation>, numero di chiavi: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6231"/>
        <source> </source>
        <translation> </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6236"/>
        <source>
Min block height: </source>
        <translation>
Altezza minima blocco: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6237"/>
        <source>
Max block height: </source>
        <translation>
Altezza massima blocco: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6238"/>
        <source>
Min amount found: </source>
        <translation>
Ammontare minimo trovato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6239"/>
        <source>
Max amount found: </source>
        <translation>
Ammontare massimo trovato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6240"/>
        <source>
Total count: </source>
        <translation>
Conto totale: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6280"/>
        <source>
Bin size: </source>
        <translation>
Dimensione Bin: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6281"/>
        <source>
Outputs per *: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6283"/>
        <source>count
  ^
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6285"/>
        <source>  |</source>
        <translation>  |</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6287"/>
        <source>  +</source>
        <translation>  +</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6287"/>
        <source>+--&gt; block height
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6288"/>
        <source>   ^</source>
        <translation>   ^</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6288"/>
        <source>^
</source>
        <translation>^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6289"/>
        <source>  </source>
        <translation>  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6330"/>
        <source>wallet</source>
        <translation>portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="748"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6690"/>
        <source>Random payment ID: </source>
        <translation>ID pagamento casuale: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6691"/>
        <source>Matching integrated address: </source>
        <translation>Indirizzo integrato corrispondente: </translation>
    </message>
</context>
<context>
    <name>genms</name>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="70"/>
        <source>Base filename (-1, -2, etc suffixes will be appended as needed)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="71"/>
        <source>Give threshold and participants at once as M/N</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="72"/>
        <source>How many participants will share parts of the multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="73"/>
        <source>How many signers are required to sign a valid transaction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="74"/>
        <source>Create testnet multisig wallets</source>
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
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="83"/>
        <source>Generating %u %u/%u multisig wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="140"/>
        <source>Error verifying multisig extra info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="148"/>
        <source>Error finalizing multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="155"/>
        <source>Generated multisig wallets for address </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="159"/>
        <source>Error creating multisig wallets: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="180"/>
        <source>This program generates a set of multisig wallets - use this simpler scheme only if all the participants trust each other</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="197"/>
        <source>Error: Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="204"/>
        <source>Error: expected N/M, but got: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="212"/>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="221"/>
        <source>Error: either --scheme or both of --threshold and --participants may be given</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="228"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got N==%u and M==%d</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="237"/>
        <source>Error: --filename-base is required</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="243"/>
        <source>Error: unsupported scheme: only N/N and N-1/N are supported</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>sw</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="120"/>
        <source>Generate new wallet and save it to &lt;arg&gt;</source>
        <translation>Genera un nuovo portafoglio e salvalo in &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="121"/>
        <source>Generate new wallet from device and save it to &lt;arg&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="122"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>Genera un portafoglio solo-ricezione da chiave di visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="123"/>
        <source>Generate deterministic wallet from spend key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="124"/>
        <source>Generate wallet from private keys</source>
        <translation>Genera portafoglio da chiavi private</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="125"/>
        <source>Generate a master wallet from multisig wallet keys</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="127"/>
        <source>Language for mnemonic</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="128"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>Specifica il seed stile Electrum per recuperare/creare il portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="129"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Recupera portafoglio usando il seed mnemonico stile-Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="130"/>
        <source>Recover multisig wallet using Electrum-style mnemonic seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="131"/>
        <source>Generate non-deterministic view and spend keys</source>
        <translation>Crea chiavi di visualizzione e chiavi di spesa non-deterministiche</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="132"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation>Abilita comandi dipendenti da un daemon fidato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="133"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>Permetti comunicazioni con un daemon che usa una versione RPC differente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="134"/>
        <source>Restore from specific blockchain height</source>
        <translation>Ripristina da specifico blocco</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="135"/>
        <source>The newly created transaction will not be relayed to the monero network</source>
        <translation type="unfinished"></translation>
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
        <translation>il daemon è occupato. Prova più tardi.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="214"/>
        <source>possibly lost connection to daemon</source>
        <translation>possibile perdita di connessione con il daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="231"/>
        <source>Error: </source>
        <translation>Errore: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7461"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7475"/>
        <source>Failed to initialize wallet</source>
        <translation>Inizializzazione wallet fallita</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="135"/>
        <source>Use daemon instance at &lt;host&gt;:&lt;port&gt;</source>
        <translation>Usa instanza daemon in &lt;host&gt;:&lt;port&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="136"/>
        <source>Use daemon instance at host &lt;arg&gt; instead of localhost</source>
        <translation>Usa istanza daemon all&apos;host &lt;arg&gt; invece che localhost</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="138"/>
        <source>Wallet password file</source>
        <translation>File password portafoglio</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="139"/>
        <source>Use daemon instance at port &lt;arg&gt; instead of 18081</source>
        <translation>Usa istanza daemon alla porta &lt;arg&gt; invece che alla 18081</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="141"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>Per testnet. Il daemon può anche essere lanciato con la flag --testnet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="142"/>
        <source>For stagenet. Daemon must also be launched with --stagenet flag</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="143"/>
        <source>Restricts to view-only commands</source>
        <translation>Restringi a comandi di tipo solo-vista</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="145"/>
        <source>Set shared ring database path</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="202"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>non puoi specificare la porta o l&apos;host del daemon più di una volta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="240"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>non puoi specificare più di un --password e --password-file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="253"/>
        <source>the password file specified could not be read</source>
        <translation>il file password specificato non può essere letto</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="278"/>
        <source>Failed to load file </source>
        <translation>Impossibile caricare file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="360"/>
        <source>At least one of either an Electrum-style word list, private view key, or private spend key must be specified</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="137"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation>Wallet password (escape/quote se necessario)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="140"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>Specificare username[:password] per client del daemon RPC</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="260"/>
        <source>no password specified; use --prompt-for-password to prompt for a password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="284"/>
        <source>Failed to parse JSON</source>
        <translation>Impossibile fare il parsing di JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="291"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation>La versione %u è troppo recente, possiamo comprendere solo fino alla versione %u</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="307"/>
        <source>failed to parse view key secret key</source>
        <translation>impossibile fare il parsing di chiave di visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="312"/>
        <location filename="../src/wallet/wallet2.cpp" line="380"/>
        <location filename="../src/wallet/wallet2.cpp" line="421"/>
        <source>failed to verify view key secret key</source>
        <translation>impossibile verificare chiave di visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="323"/>
        <source>failed to parse spend key secret key</source>
        <translation>impossibile fare il parsing chiave di spesa chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="328"/>
        <location filename="../src/wallet/wallet2.cpp" line="390"/>
        <location filename="../src/wallet/wallet2.cpp" line="446"/>
        <source>failed to verify spend key secret key</source>
        <translation>impossibile verificare chiave di spesa chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="340"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Verifica lista di parole stile-Electrum fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="364"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Specificate entrambe lista parole stile-Electrum e chiave/i privata/e </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="374"/>
        <source>invalid address</source>
        <translation>indirizzo invalido</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="383"/>
        <source>view key does not match standard address</source>
        <translation>la chiave di visualizzazione non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="393"/>
        <source>spend key does not match standard address</source>
        <translation>la chiave di spesa non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="401"/>
        <source>Cannot generate deprecated wallets from JSON</source>
        <translation>Impossibile creare portafogli disapprovati da JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="433"/>
        <source>failed to parse address: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="439"/>
        <source>Address must be specified in order to create watch-only wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="454"/>
        <source>failed to generate new wallet: </source>
        <translation>impossibile generare nuovo portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="2970"/>
        <location filename="../src/wallet/wallet2.cpp" line="3027"/>
        <location filename="../src/wallet/wallet2.cpp" line="3120"/>
        <location filename="../src/wallet/wallet2.cpp" line="3170"/>
        <location filename="../src/wallet/wallet2.cpp" line="3210"/>
        <location filename="../src/wallet/wallet2.cpp" line="3306"/>
        <location filename="../src/wallet/wallet2.cpp" line="3408"/>
        <location filename="../src/wallet/wallet2.cpp" line="3808"/>
        <location filename="../src/wallet/wallet2.cpp" line="4185"/>
        <source>Primary account</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="8598"/>
        <source>No funds received in this tx.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="9289"/>
        <source>failed to read file </source>
        <translation>lettura file fallita</translation>
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
        <translation>Il daemon è locale, viene considerato fidato</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="176"/>
        <source>Failed to create directory </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="178"/>
        <source>Failed to create directory %s: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="189"/>
        <source>Cannot specify --</source>
        <translation>Impossibile specificare --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="189"/>
        <source> and --</source>
        <translation> e --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="208"/>
        <source>Failed to create file </source>
        <translation>Impossibile creare file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="208"/>
        <source>. Check permissions or remove file</source>
        <translation>. Controlla permessi o rimuovi il file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="218"/>
        <source>Error writing to file </source>
        <translation>Errore durante scrittura su file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="221"/>
        <source>RPC username/password is stored in file </source>
        <translation>Username/password RPC conservato nel file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="442"/>
        <source>Tag %s is unregistered.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2466"/>
        <source>Transaction not possible. Available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2901"/>
        <source>This is the RPC monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2920"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2932"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>Non puoi specificare più di un --wallet-file e --generate-from-json</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2944"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>Devi specificare --wallet-file o --generate-from-json o --wallet-dir</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2948"/>
        <source>Loading wallet...</source>
        <translation>Sto caricando il portafoglio...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2981"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3014"/>
        <source>Saving wallet...</source>
        <translation>Sto salvando il portafoglio...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2983"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3016"/>
        <source>Successfully saved</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2986"/>
        <source>Successfully loaded</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2990"/>
        <source>Wallet initialization failed: </source>
        <translation>Inizializzazione portafoglio fallita: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2997"/>
        <source>Failed to initialize wallet RPC server</source>
        <translation>Inizializzazione server RPC portafoglio fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3001"/>
        <source>Starting wallet RPC server</source>
        <translation>Server RPC portafoglio in avvio</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3008"/>
        <source>Failed to run wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3011"/>
        <source>Stopped wallet RPC server</source>
        <translation>Server RPC portafoglio arrestato</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3020"/>
        <source>Failed to save wallet: </source>
        <translation>Impossibile salvare portafoglio: </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7430"/>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="168"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2887"/>
        <source>Wallet options</source>
        <translation>Opzioni portafoglio</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="73"/>
        <source>Generate wallet from JSON format file</source>
        <translation>Genera portafoglio da file JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="77"/>
        <source>Use wallet &lt;arg&gt;</source>
        <translation>Usa portafoglio &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="104"/>
        <source>Max number of threads to use for a parallel job</source>
        <translation>Numero massimo di threads da utilizzare per un lavoro parallelo</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="105"/>
        <source>Specify log file</source>
        <translation>Specificare file di log</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="106"/>
        <source>Config file</source>
        <translation>File configurazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="115"/>
        <source>General options</source>
        <translation>Opzioni generali</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="138"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="161"/>
        <source>Can&apos;t find config file </source>
        <translation>Impossibile trovare file configurazione </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="195"/>
        <source>Logging to: </source>
        <translation>Sto salvando il Log in: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="197"/>
        <source>Logging to %s</source>
        <translation>Sto salvando il Log in %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="140"/>
        <source>Usage:</source>
        <translation>Uso:</translation>
    </message>
</context>
</TS>
