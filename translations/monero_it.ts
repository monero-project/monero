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
        <location filename="../src/wallet/api/address_book.cpp" line="60"/>
        <source>Payment ID supplied: this is obsolete</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Monero::PendingTransactionImpl</name>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="91"/>
        <source>Attempting to save transaction to file, but specified file(s) exist. Exiting to not risk overwriting. File:</source>
        <translation>Sto tentando di salvare la transazione nel file, ma il file specificato è già esistente. Sto uscendo per non rischiare di sovrascriverlo. File:</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="98"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Impossibile scrivere transazione/i su file</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="138"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>il daemon è impegnato. Prova più tardi.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="141"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>nessuna connessione con il daemon. Controlla che sia operativo.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="145"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transazione %s è stata respinta dal daemon con status: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="150"/>
        <source>. Reason: </source>
        <translation>. Motivo: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="152"/>
        <source>Unknown exception: </source>
        <translation>Eccezione sconosciuta: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="155"/>
        <source>Unhandled exception</source>
        <translation>Eccezione non gestita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="228"/>
        <source>Couldn&apos;t multisig sign data: </source>
        <translation>Impossibile firmare dati multifirma: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="250"/>
        <source>Couldn&apos;t sign multisig transaction: </source>
        <translation>Impossibile firmare la transazione multifirma: </translation>
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
        <translation>Caricate %lu transazioni, per %s, commissione %s, %s, %s, con ring size minimo %lu. %s</translation>
    </message>
</context>
<context>
    <name>Monero::WalletImpl</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1513"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1602"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>il daemon è impegnato. Riprova più tardi.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1515"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1604"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>nessuna connessione con il daemon. Accertati che sia operativo.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1517"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1606"/>
        <source>RPC error: </source>
        <translation>errore RPC: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1545"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1637"/>
        <source>not enough outputs for specified ring size</source>
        <translation>insufficiente numero di output per il ring size specificato</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1547"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1639"/>
        <source>found outputs to use</source>
        <translation>trovati output che possono essere usati</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1549"/>
        <source>Please sweep unmixable outputs.</source>
        <translation>Pulisci gli output non mixabili.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1523"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1613"/>
        <source>not enough money to transfer, available only %s, sent amount %s</source>
        <translation>non hai abbastanza fondi da trasferire, sono disponibili solo %s, ammontare inviato %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="589"/>
        <source>failed to parse address</source>
        <translation>parsing indirizzo fallito</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="600"/>
        <source>failed to parse secret spend key</source>
        <translation>impossibile effettuare il parsing della chiave segreta di spesa</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="623"/>
        <source>failed to parse secret view key</source>
        <translation>impossibile effettuare il parsing della chiave segreta di visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="632"/>
        <source>failed to verify secret spend key</source>
        <translation>impossibile verificare la chiave segreta di spesa</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="636"/>
        <source>spend key does not match address</source>
        <translation>la chiave di spesa non corrisponde all&apos;indirizzo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="642"/>
        <source>failed to verify secret view key</source>
        <translation>verifica chiave segreta di visualizzazione fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="646"/>
        <source>view key does not match address</source>
        <translation>la chiave di visualizzazione non corrisponde all&apos;indirizzo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="669"/>
        <location filename="../src/wallet/api/wallet.cpp" line="686"/>
        <source>failed to generate new wallet: </source>
        <translation>impossibile generare il nuovo portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="955"/>
        <source>Failed to send import wallet request</source>
        <translation>Impossibile inviare la richiesta di importazione portafoglio</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1125"/>
        <source>Failed to load unsigned transactions</source>
        <translation>Impossibile caricare transazioni non firmate</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1148"/>
        <source>Failed to load transaction from file</source>
        <translation>Impossibile caricare la transazione da file</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1164"/>
        <source>Wallet is view only</source>
        <translation>Il portafoglio è di tipo solo visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1172"/>
        <source>failed to save file </source>
        <translation>impossibile salvare il file </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1188"/>
        <source>Key images can only be imported with a trusted daemon</source>
        <translation>Le key image possono essere importate solo con un daemon fidato</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1201"/>
        <source>Failed to import key images: </source>
        <translation>Impossibile importare le key images: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1233"/>
        <source>Failed to get subaddress label: </source>
        <translation>Impossibile recuperare l&apos;etichetta del sottoindirizzo: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1246"/>
        <source>Failed to set subaddress label: </source>
        <translation>Impossibile assegnare l&apos;etichetta del sottoindirizzo: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="615"/>
        <source>Neither view key nor spend key supplied, cancelled</source>
        <translation>Nessuna chiave di visualizzazione né chiave di spesa fornita, operazione annullata</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="734"/>
        <source>Electrum seed is empty</source>
        <translation>Il seed Electrum è vuoto</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="743"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Verifica dell&apos;elenco parole in stile Electrum non riuscita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1263"/>
        <source>Failed to get multisig info: </source>
        <translation>Impossibile ottenere informazioni multifirma: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1280"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1294"/>
        <source>Failed to make multisig: </source>
        <translation>Impossibile eseguire multifirma: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1309"/>
        <source>Failed to finalize multisig wallet creation</source>
        <translation>Impossibile finalizzare la creazione di un portafoglio multifirma</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1312"/>
        <source>Failed to finalize multisig wallet creation: </source>
        <translation>Impossibile finalizzare la creazione di un portafoglio multifirma: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1328"/>
        <source>Failed to export multisig images: </source>
        <translation>Impossibile esportare immagini multifirma: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1346"/>
        <source>Failed to parse imported multisig images</source>
        <translation>Impossibile analizzare le immagini multifirma importate</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1356"/>
        <source>Failed to import multisig images: </source>
        <translation>Impossibile importare immagini multifirma: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1370"/>
        <source>Failed to check for partial multisig key images: </source>
        <translation>Impossibile controllare le immagini della chiave multifirma parziali: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1398"/>
        <source>Failed to restore multisig transaction: </source>
        <translation>Impossibile ripristinare la transazione multifirma: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1439"/>
        <source>Sending all requires one destination address</source>
        <translation>L&apos;invio di tutto richiede un indirizzo di destinazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1443"/>
        <source>Destinations and amounts are unequal</source>
        <translation>Le destinazioni e gli importi non corrispondono</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1451"/>
        <source>payment id has invalid format, expected 64 character hex string: </source>
        <translation>id pagamento ha un formato non valido, è prevista una stringa esadecimale di 64 caratteri:</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1459"/>
        <source>Invalid destination address</source>
        <translation>Indirizzo destinatario non valido</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1465"/>
        <source>a single transaction cannot use more than one payment id</source>
        <translation>una singola transazione non può utilizzare più di un id di pagamento</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1491"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>impossibile impostare id pagamento, anche se è stato decodificato correttamente</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1519"/>
        <source>failed to get outputs to mix: %s</source>
        <translation>impossibile ottenere gli output da mixare: %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1530"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1621"/>
        <source>not enough money to transfer, overall balance only %s, sent amount %s</source>
        <translation>fondi non sufficienti per il trasferimento, saldo totale %s, importo inviato %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1537"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1629"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>fondi non sufficienti per il trasferimento, disponibili solo %s, ammontare transazione %s = %s + %s (commissione)</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1547"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1639"/>
        <source>output amount</source>
        <translation>ammontare output</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1552"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1643"/>
        <source>transaction was not constructed</source>
        <translation>transazione non costruita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1555"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1646"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transazione %s è stata rifiutata dal daemon con status: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1560"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1651"/>
        <source>one of destinations is zero</source>
        <translation>una delle destinazioni è zero</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1562"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1653"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>impossibile trovare un modo per dividere le transazioni</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1564"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1655"/>
        <source>unknown transfer error: </source>
        <translation>errore trasferimento sconosciuto: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1566"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1657"/>
        <source>internal error: </source>
        <translation>errore interno: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1568"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1659"/>
        <source>unexpected error: </source>
        <translation>errore inaspettato: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1570"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1661"/>
        <source>unknown error</source>
        <translation>errore sconosciuto</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1608"/>
        <source>failed to get outputs to mix</source>
        <translation>impossibile ottenere gli output da mixare</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1749"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1785"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1833"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1861"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1889"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1910"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2386"/>
        <source>Failed to parse txid</source>
        <translation>Impossibile effettuare parsing del txid</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1769"/>
        <source>no tx keys found for this txid</source>
        <translation>nessuna chiave tx trovata per questo txid</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1793"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1802"/>
        <source>Failed to parse tx key</source>
        <translation>Impossibile effettuare parsing della chiave tx</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1811"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1840"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1868"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1949"/>
        <source>Failed to parse address</source>
        <translation>Impossibile effettuare parsing dell&apos;indirizzo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1954"/>
        <source>Address must not be a subaddress</source>
        <translation>L&apos;indirizzo non può essere un sottoindirizzo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1994"/>
        <source>The wallet must be in multisig ready state</source>
        <translation>Il portafoglio deve essere in stato multifirma</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2016"/>
        <source>Given string is not a key</source>
        <translation>La stringa inserita non è una chiave</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2258"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>&quot;Riscannerizza spesi&quot; può essere utilizzato solo da un daemon fidato</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2307"/>
        <source>Invalid output: </source>
        <translation>Output non valido: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2314"/>
        <source>Failed to mark outputs as spent</source>
        <translation>Impossibile contrassegnare gli outputs come spesi</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2325"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2347"/>
        <source>Failed to parse output amount</source>
        <translation>Impossibile analizzare la quantità di output</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2330"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2352"/>
        <source>Failed to parse output offset</source>
        <translation>Impossibile analizzare l&apos;offset di output</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2336"/>
        <source>Failed to mark output as spent</source>
        <translation>Impossibile contrassegnare l&apos;output come speso</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2358"/>
        <source>Failed to mark output as unspent</source>
        <translation>Impossibile contrassegnare l&apos;output come non speso</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2369"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2408"/>
        <source>Failed to parse key image</source>
        <translation>Impossibile analizzare l&apos;immagine della chiave</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2375"/>
        <source>Failed to get ring</source>
        <translation>Impossibile ottenere l&apos;anello</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2393"/>
        <source>Failed to get rings</source>
        <translation>Impossibile ottenere gli anelli</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2414"/>
        <source>Failed to set ring</source>
        <translation>Impossibile impostare l&apos;anello</translation>
    </message>
</context>
<context>
    <name>Wallet</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="344"/>
        <source>Failed to parse address</source>
        <translation>Impossibile effettuare parsing dell&apos;indirizzo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="351"/>
        <source>Failed to parse key</source>
        <translation>Impossibile effettuare parsing della chiave</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="359"/>
        <source>failed to verify key</source>
        <translation>impossibile effettuare la verifica della chiave</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="369"/>
        <source>key does not match address</source>
        <translation>la chiave non corrisponde all&apos;indirizzo</translation>
    </message>
</context>
<context>
    <name>command_line</name>
    <message>
        <location filename="../src/common/command_line.cpp" line="54"/>
        <source>yes</source>
        <translation>sì</translation>
    </message>
    <message>
        <location filename="../src/common/command_line.cpp" line="68"/>
        <source>no</source>
        <translation>no</translation>
    </message>
</context>
<context>
    <name>cryptonote::rpc_args</name>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="92"/>
        <source>Specify IP to bind RPC server</source>
        <translation>Specificare IP da associare al server RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="93"/>
        <source>Specify IPv6 address to bind RPC server</source>
        <translation>Specificare l&apos;indirizzo IPv6 da associare al server RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="94"/>
        <source>Allow IPv6 for RPC</source>
        <translation>Consenti IPv6 per RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="95"/>
        <source>Ignore unsuccessful IPv4 bind for RPC</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="96"/>
        <source>Specify username[:password] required for RPC server</source>
        <translation>Specifica username[:password] richiesta per server RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="97"/>
        <source>Confirm rpc-bind-ip value is NOT a loopback (local) IP</source>
        <translation>Conferma valore rpc-bind-ip NON è un loopback IP (locale)</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="98"/>
        <source>Specify a comma separated list of origins to allow cross origin resource sharing</source>
        <translation>Specificare una lista di origini i cui elementi sono separati da virgola al fine di consentire la condivisione incrociata fra le origini</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="99"/>
        <source>Enable SSL on RPC connections: enabled|disabled|autodetect</source>
        <translation>Abilita SSL su connessioni RPC: abilitato|disabilitato|rilevamento automatico</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="100"/>
        <source>Path to a PEM format private key</source>
        <translation>Percorso della chiave privata in formato PEM</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="101"/>
        <source>Path to a PEM format certificate</source>
        <translation>Percorso del certificato in formato PEM</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="102"/>
        <source>Path to file containing concatenated PEM format certificate(s) to replace system CA(s).</source>
        <translation>Percorso del file contenente i certificati concatenati in formato PEM per sostituire le CA di sistema.</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="103"/>
        <source>List of certificate fingerprints to allow</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="104"/>
        <source>Allow user (via --rpc-ssl-certificates) chain certificates</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="105"/>
        <source>Allow any peer certificate</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="146"/>
        <location filename="../src/rpc/rpc_args.cpp" line="174"/>
        <source>Invalid IP address given for --</source>
        <translation>Indirizzo IP non valido dato per --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="154"/>
        <location filename="../src/rpc/rpc_args.cpp" line="182"/>
        <source> permits inbound unencrypted external connections. Consider SSH tunnel or SSL proxy instead. Override with --</source>
        <translation>permette connessioni esterne non criptate in entrata. Considera in alternativa un tunnel SSH o un proxy SSL. Sovrascrivi con --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="205"/>
        <source>Username specified with --</source>
        <translation>Nome utente specificato con --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="205"/>
        <location filename="../src/rpc/rpc_args.cpp" line="215"/>
        <source> cannot be empty</source>
        <translation> non può essere vuoto</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="215"/>
        <source> requires RPC server password --</source>
        <translation> richiede la password del server RPC --</translation>
    </message>
</context>
<context>
    <name>cryptonote::simple_wallet</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="690"/>
        <source>Commands: </source>
        <translation>Comandi: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5065"/>
        <source>failed to read wallet password</source>
        <translation>impossibile leggere la password del portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4651"/>
        <source>invalid password</source>
        <translation>password non valida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3677"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation>imposta seed: richiede un argomento. opzioni disponibili: lingua</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3719"/>
        <source>set: unrecognized argument(s)</source>
        <translation>imposta: argomento/i non riconosciuto/i</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4904"/>
        <source>wallet file path not valid: </source>
        <translation>percorso file portafoglio non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3789"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation>Sto tentando di generare o ripristinare il portafoglio, ma i(l) file specificato/i esiste/esistono già. Sto uscendo per non rischiare di sovrascrivere.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3663"/>
        <source>needs an argument</source>
        <translation>ha bisogno di un argomento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3686"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3687"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3688"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3690"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3697"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3698"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3700"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3702"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3703"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3704"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3707"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3710"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3715"/>
        <source>0 or 1</source>
        <translation>0 o 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3695"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3699"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3706"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3717"/>
        <source>unsigned integer</source>
        <translation>intero senza segno</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3977"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation>specificare un parametro di ripristino con --electrum-seed=&quot;lista parole qui&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4584"/>
        <source>wallet failed to connect to daemon: </source>
        <translation>impossibile connettere il portafoglio al daemon: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4592"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation>Il daemon usa una versione principale RPC (%u) diversa da quella del portafoglio (%u): %s. Aggiorna una delle due, o usa --allow-mismatched-daemon-version.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4613"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation>Lista delle lingue disponibili per il seed del tuo portafoglio:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4699"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation>Hai usato una versione obsoleta del portafoglio. Per favore usa il nuovo seed che ti abbiamo fornito.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4715"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4789"/>
        <source>Generated new wallet: </source>
        <translation>Nuovo portafoglio generato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4724"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4794"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4838"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4893"/>
        <source>failed to generate new wallet: </source>
        <translation>impossibile generare nuovo portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4935"/>
        <source>Opened watch-only wallet</source>
        <translation>Portafoglio solo-vista aperto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4939"/>
        <source>Opened wallet</source>
        <translation>Portafoglio aperto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4957"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation>Stai utilizzando una versione disapprovata del portafoglio. Per favore procedi nell&apos;upgrade del portafoglio.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4972"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation>Stai utilizzando una versione disapprovata del portafoglio. Il formato del tuo portafoglio sta per essere aggiornato.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4980"/>
        <source>failed to load wallet: </source>
        <translation>impossibile caricare portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4997"/>
        <source>Use the &quot;help&quot; command to see the list of available commands.
</source>
        <translation>Usa il comando &quot;help&quot; per visualizzare la lista dei comandi disponibili.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5043"/>
        <source>Wallet data saved</source>
        <translation>Dati del portafoglio salvati</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5248"/>
        <source>Mining started in daemon</source>
        <translation>Mining avviato nel daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5250"/>
        <source>mining has NOT been started: </source>
        <translation>il mining NON è stato avviato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5270"/>
        <source>Mining stopped in daemon</source>
        <translation>Mining nel daemon interrotto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5272"/>
        <source>mining has NOT been stopped: </source>
        <translation>il mining NON è stato interrotto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5409"/>
        <source>Blockchain saved</source>
        <translation>Blockchain salvata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5428"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5474"/>
        <source>Height </source>
        <translation>Blocco </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5476"/>
        <source>spent </source>
        <translation>speso/i </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5586"/>
        <source>Starting refresh...</source>
        <translation>Sto iniziando il refresh...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5612"/>
        <source>Refresh done, blocks received: </source>
        <translation>Refresh finito, blocchi ricevuti: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6933"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>l&apos;id pagamento ha un formato invalido, dovrebbe essere una stringa hex di 16 o 64 caratteri: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6303"/>
        <source>bad locked_blocks parameter:</source>
        <translation>parametro locked_blocks non corretto:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6953"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7211"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation>una singola transazione non può usare più di un id pagamento: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6403"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6962"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7179"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7219"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>impossibile impostare id pagamento, anche se è stato decodificado correttamente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6265"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6860"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7137"/>
        <source>ring size %u is too large, maximum is %u</source>
        <translation>la dimensione dell&apos;anello %u è troppo grande, il massimo è %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6289"/>
        <source>payment id failed to encode</source>
        <translation>codifica dell&apos;ID di pagamento non riuscita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6336"/>
        <source>failed to parse short payment ID from URI</source>
        <translation>impossibile analizzare l&apos;ID di pagamento breve dall&apos;URI</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6361"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6363"/>
        <source>Invalid last argument: </source>
        <translation>Ultimo argomento non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6381"/>
        <source>a single transaction cannot use more than one payment id</source>
        <translation>una singola transazione non può utilizzare più di un ID di pagamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6397"/>
        <source>failed to parse payment id, though it was detected</source>
        <translation>errore nell&apos;analisi dell&apos;ID di pagamento, anche se è stato rilevato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6486"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6495"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6582"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6731"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6983"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7026"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7262"/>
        <source>transaction cancelled.</source>
        <translation>transazione cancellata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6533"/>
        <source>Sending %s.  </source>
        <translation>Sto inviando %s. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6536"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation>La tua transazione deve essere divisa in %llu transazioni. Una commissione verrà applicata per ogni transazione, per un totale di %s commissioni</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6542"/>
        <source>The transaction fee is %s</source>
        <translation>La commissione per la transazione è %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6545"/>
        <source>, of which %s is dust from change</source>
        <translation>, della quale %s è polvere dovuta allo scambio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6546"/>
        <source>.</source>
        <translation>.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6546"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation>Un totale di %s in polvere verrà inviato all&apos;indirizzo della polvere</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6595"/>
        <source>Unsigned transaction(s) successfully written to MMS</source>
        <translation>Transazione(i) senza firma scritta(e) con successo su MMS</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6603"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6640"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6742"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6754"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7037"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7074"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7272"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7284"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Impossibile scrivere transazione/i su file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6608"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6645"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6746"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6758"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7041"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7078"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7276"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7288"/>
        <source>Unsigned transaction(s) successfully written to file: </source>
        <translation>Transazioni/e non firmata/e scritte/a con successo su file: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6617"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7053"/>
        <source>Failed to cold sign transaction with HW wallet</source>
        <translation>Impossibile firmare transazioni a freddo con il portafoglio hardware</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6701"/>
        <source>No unmixable outputs found</source>
        <translation>Nessun output non-mixabile trovato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6768"/>
        <source>Not enough money in unlocked balance</source>
        <translation>Fondi insufficienti in saldo sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6808"/>
        <source>No address given</source>
        <translation>Non è stato fornito nessun indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6869"/>
        <source>missing lockedblocks parameter</source>
        <translation>parametro lockedblocks mancante</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6879"/>
        <source>bad locked_blocks parameter</source>
        <translation>parametro locked_blocks errato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6904"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7146"/>
        <source>Failed to parse number of outputs</source>
        <translation>Impossibile analizzare il numero di outputs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6909"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7151"/>
        <source>Amount of outputs should be greater than 0</source>
        <translation>La quantità di outputs deve essere maggiore di 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7374"/>
        <source>Failed to parse donation address: </source>
        <translation>Impossibile analizzare l&apos;indirizzo di donazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7388"/>
        <source>Donating %s %s to The Monero Project (donate.getmonero.org or %s).</source>
        <translation>Donare %s %s a The Monero Project (donate.getmonero.org o %s).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7390"/>
        <source>Donating %s %s to %s.</source>
        <translation>Donare %s %s a %s.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7477"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>Il cambiamento richiesto non porta a un indirizzo pagato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7482"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>Il cambiamento richiesto è più largo del pagamento all&apos;indirizzo di cambio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7513"/>
        <source>sending %s to %s</source>
        <translation>sto mandando %s a %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7523"/>
        <source> dummy output(s)</source>
        <translation> output dummy</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7526"/>
        <source>with no destinations</source>
        <translation>senza destinazioni</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7567"/>
        <source>This is a multisig wallet, it can only sign with sign_multisig</source>
        <translation>Questo è un portafoglio multisig, può firmare solo con sign_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7590"/>
        <source>Failed to sign transaction</source>
        <translation>Impossibile firmare la transazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7596"/>
        <source>Failed to sign transaction: </source>
        <translation>Impossibile firmare la transazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7617"/>
        <source>Transaction raw hex data exported to </source>
        <translation>Dati esadecimali grezzi della transazione esportati su </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7638"/>
        <source>Failed to load transaction from file</source>
        <translation>Impossibile caricare la transazione da file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5634"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5969"/>
        <source>RPC error: </source>
        <translation>errore RPC: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="740"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation>il portafoglio è solo-vista e non ha una chiave di spesa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="890"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1064"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1117"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1184"/>
        <source>Your original password was incorrect.</source>
        <translation>La tua password originale era scorretta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="905"/>
        <source>Error with wallet rewrite: </source>
        <translation>Errore riscrittura wallet: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2627"/>
        <source>invalid unit</source>
        <translation>unità invalida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2645"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2707"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation>conteggio invalido: deve essere un intero senza segno</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2663"/>
        <source>invalid value</source>
        <translation>valore invalido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4421"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4441"/>
        <source>bad m_restore_height parameter: </source>
        <translation>parametro m_restore_height non corretto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4385"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4432"/>
        <source>Restore height is: </source>
        <translation>Ripristina altezza è: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5360"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Il daemon è locale, viene considerato fidato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5061"/>
        <source>Password for new watch-only wallet</source>
        <translation>Password per il nuovo portafoglio solo-vista</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5644"/>
        <source>internal error: </source>
        <translation>errore interno: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1652"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1993"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5649"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5974"/>
        <source>unexpected error: </source>
        <translation>errore inaspettato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1578"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1657"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5654"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5979"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6631"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6661"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6787"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7066"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7093"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7305"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7651"/>
        <source>unknown error</source>
        <translation>errore sconosciuto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5659"/>
        <source>refresh failed: </source>
        <translation>refresh fallito: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5659"/>
        <source>Blocks received: </source>
        <translation>Blocchi ricevuti: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5700"/>
        <source>unlocked balance: </source>
        <translation>bilancio sbloccato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3696"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3708"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3709"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5807"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5869"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8505"/>
        <source>amount</source>
        <translation>ammontare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="378"/>
        <source>false</source>
        <translation>falso</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="704"/>
        <source>Unknown command: </source>
        <translation>Comando sconosciuto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="711"/>
        <source>Command usage: </source>
        <translation>Uso del comando: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="714"/>
        <source>Command description: </source>
        <translation>Descrizione del comando: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="780"/>
        <source>wallet is multisig but not yet finalized</source>
        <translation>il portafoglio è multisig ma ancora non finalizzato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="813"/>
        <source>Failed to retrieve seed</source>
        <translation>Impossibile recuperare il seed</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="843"/>
        <source>wallet is multisig and has no seed</source>
        <translation>il portafoglio è multisig e non ha seed</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="942"/>
        <source>Error: failed to estimate backlog array size: </source>
        <translation>Errore: impossibile stimare la dimensione dell&apos;array di backlog: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="947"/>
        <source>Error: bad estimated backlog array size</source>
        <translation>Errore: errata stima della dimensione dell&apos;array di backlog</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="959"/>
        <source> (current)</source>
        <translation> (attuale)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="962"/>
        <source>%u block (%u minutes) backlog at priority %u%s</source>
        <translation>Backlog blocco %u (%u minuti) a priorità %u%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="964"/>
        <source>%u to %u block (%u to %u minutes) backlog at priority %u</source>
        <translation>Backlog blocco %u a %u (%u a %u minuti) a priorità %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="967"/>
        <source>No backlog at priority </source>
        <translation>Nessun backlog a priorità </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="987"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1032"/>
        <source>This wallet is already multisig</source>
        <translation>Questo portafoglio è già multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="992"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1037"/>
        <source>wallet is watch-only and cannot be made multisig</source>
        <translation>il portafoglio è sola-visualizzazione e non può essere reso multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="998"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1043"/>
        <source>This wallet has been used before, please use a new wallet to create a multisig wallet</source>
        <translation>Questo portafoglio è stato usato precedentmente, per cortesia utilizza un nuovo portafoglio per creare un portafoglio multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1006"/>
        <source>Send this multisig info to all other participants, then use make_multisig &lt;threshold&gt; &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Invia queste informazioni multisig a tutti gli altri partecipanti, poi utilizza make_multisig &lt;threshold&gt; &lt;info1&gt; [&lt;info2&gt;...] con le informazioni multisig degli altri</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1007"/>
        <source>This includes the PRIVATE view key, so needs to be disclosed only to that multisig wallet&apos;s participants </source>
        <translation>Questo include la chiave PRIVATA di visualizzazione, pertanto deve essere comunicata solo ai partecipanti di quel portafoglio multisig </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1057"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2797"/>
        <source>Invalid threshold</source>
        <translation>Soglia invalida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1077"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1199"/>
        <source>Another step is needed</source>
        <translation>Ancora un ultimo passo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1089"/>
        <source>Error creating multisig: </source>
        <translation>Impossibile creare multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1096"/>
        <source>Error creating multisig: new wallet is not multisig</source>
        <translation>Impossibile creare multisig: il nuovo portafoglio non è multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1099"/>
        <source> multisig address: </source>
        <translation> indirizzo multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1123"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1172"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1239"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1305"/>
        <source>This wallet is not multisig</source>
        <translation>Questo portafoglio non è multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1128"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1177"/>
        <source>This wallet is already finalized</source>
        <translation>Questo portafoglio è già finalizzato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1144"/>
        <source>Failed to finalize multisig</source>
        <translation>Impossibile finalizzare multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1150"/>
        <source>Failed to finalize multisig: </source>
        <translation>Impossibile finalizzare multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1211"/>
        <source>Multisig address: </source>
        <translation>Indirizzo multifirma: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1244"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1310"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1404"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1520"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1601"/>
        <source>This multisig wallet is not yet finalized</source>
        <translation>Questo portafoglio multisig non è ancora finalizzato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1280"/>
        <source>Error exporting multisig info: </source>
        <translation>Impossibile esportare informazioni sul multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1284"/>
        <source>Multisig info exported to </source>
        <translation>Informazioni sul multisig esportate su </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1350"/>
        <source>Multisig info imported</source>
        <translation>Informazioni su multisig importate</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1354"/>
        <source>Failed to import multisig info: </source>
        <translation>Impossibile importare informazioni sul multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1365"/>
        <source>Failed to update spent status after importing multisig info: </source>
        <translation>Impossibile aggiornare lo stato di spesa dopo aver importato le informazioni sul multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1371"/>
        <source>Untrusted daemon, spent status may be incorrect. Use a trusted daemon and run &quot;rescan_spent&quot;</source>
        <translation>Daemon non fidato, lo stato di spesa potrebbe non essere corretto. Usare un daemon fidato ed eseguire &quot;rescan_spent&quot; </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1399"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1515"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1596"/>
        <source>This is not a multisig wallet</source>
        <translation>Questo non è un portafoglio multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1449"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1458"/>
        <source>Failed to sign multisig transaction</source>
        <translation>Impossibile firmare la transazione multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1465"/>
        <source>Multisig error: </source>
        <translation>Errore multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1470"/>
        <source>Failed to sign multisig transaction: </source>
        <translation>Impossibile firmare la transazione multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1493"/>
        <source>It may be relayed to the network with submit_multisig</source>
        <translation>Potrebbe essere trasmesso alla rete con submit_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1552"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1622"/>
        <source>Failed to load multisig transaction from file</source>
        <translation>Impossibile caricare la transazione multisig da file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1558"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1627"/>
        <source>Multisig transaction signed by only %u signers, needs %u more signatures</source>
        <translation>Transazione multisig firmata da solo %u firmatari, necessita di altre %u firme</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1567"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10083"/>
        <source>Transaction successfully submitted, transaction </source>
        <translation>Transazione inviata con successo, transazione </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1568"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10084"/>
        <source>You can check its status by using the `show_transfers` command.</source>
        <translation>E&apos; possibile controllare il suo stato mediante il comando `show_transfers`.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1643"/>
        <source>Failed to export multisig transaction to file </source>
        <translation>Impossibile esportare la transazione multisig su file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1647"/>
        <source>Saved exported multisig transaction file(s): </source>
        <translation>Transazioni esportate salvate su(i) file: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1912"/>
        <source>Invalid key image or txid</source>
        <translation>Immagine della chiave o &apos;&apos;txid&apos;&apos; non valida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1921"/>
        <source>failed to unset ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2163"/>
        <source>usage: %s &lt;key_image&gt;|&lt;pubkey&gt;</source>
        <translation>utilizzo: %s &lt;key_image&gt;|&lt;pubkey&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2208"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2220"/>
        <source>Frozen: </source>
        <translation>Congelato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2222"/>
        <source>Not frozen: </source>
        <translation>Non Congelato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2236"/>
        <source> bytes sent</source>
        <translation> byte inviati</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2237"/>
        <source> bytes received</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2282"/>
        <source>Welcome to Monero, the private cryptocurrency.</source>
        <translation>Benvenuto/a in Monero, la cryptovaluta privata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2284"/>
        <source>Monero, like Bitcoin, is a cryptocurrency. That is, it is digital money.</source>
        <translation>Monero, come Bitcoin, è una criptovaluta. Cioè, è denaro digitale.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2288"/>
        <source>Monero protects your privacy on the blockchain, and while Monero strives to improve all the time,</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2289"/>
        <source>no privacy technology can be 100% perfect, Monero included.</source>
        <translation>nessuna tecnologia per la privacy può essere perfetta al 100%, incluso Monero.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2290"/>
        <source>Monero cannot protect you from malware, and it may not be as effective as we hope against powerful adversaries.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2291"/>
        <source>Flaws in Monero may be discovered in the future, and attacks may be developed to peek under some</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2292"/>
        <source>of the layers of privacy Monero provides. Be safe and practice defense in depth.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2444"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2450"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2476"/>
        <source>ring size must be an integer &gt;= </source>
        <translation>il ring size deve essere un intero &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2481"/>
        <source>could not change default ring size</source>
        <translation>impossibile modificare il ring size di default</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2741"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2859"/>
        <source>Invalid height</source>
        <translation>Altezza invalida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2890"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2909"/>
        <source>Invalid amount</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2966"/>
        <source>invalid argument: must be either 1/yes or 0/no</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3075"/>
        <source>Start mining in the daemon (bg_mining and ignore_battery are optional booleans).</source>
        <translation>Avvia il mining sul daemon (bg_mining e ignore_battery sono booleani opzionali).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3078"/>
        <source>Stop mining in the daemon.</source>
        <translation>Arresta il mining sul daemon.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3082"/>
        <source>Set another daemon to connect to.</source>
        <translation>Seleziona un altro daemon cui connettersi.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3085"/>
        <source>Save the current blockchain data.</source>
        <translation>Salva i dati blockchain correnti.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3088"/>
        <source>Synchronize the transactions and balance.</source>
        <translation>Sincronizza le transazioni ed il saldo.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3092"/>
        <source>Show the wallet&apos;s balance of the currently selected account.</source>
        <translation>Mostra il saldo del portafoglio del conto attualmente selezionato.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3096"/>
        <source>Show the incoming transfers, all or filtered by availability and address index.

Output format:
Amount, Spent(&quot;T&quot;|&quot;F&quot;), &quot;frozen&quot;|&quot;locked&quot;|&quot;unlocked&quot;, RingCT, Global Index, Transaction Hash, Address Index, [Public Key, Key Image] </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3102"/>
        <source>Show the payments for the given payment IDs.</source>
        <translation>Mostra i pagamenti per gli id pagamento specificati.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3105"/>
        <source>Show the blockchain height.</source>
        <translation>Mostra l&apos;altezza della blockchain.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3119"/>
        <source>Send all unmixable outputs to yourself with ring_size 1</source>
        <translation>Invia tutti gli output non mixabili a te stesso usando ring_size 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3126"/>
        <source>Send all unlocked outputs below the threshold to an address.</source>
        <translation>Invia tutti gli output sbloccati sotto la soglia ad un indirizzo.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3130"/>
        <source>Send a single output of the given key image to an address without change.</source>
        <translation>Invia un singolo output della key image specificata ad un indirizzo senza modifica.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3134"/>
        <source>Donate &lt;amount&gt; to the development team (donate.getmonero.org).</source>
        <translation>Dona &lt;amount&gt; al team di sviluppo (donate.getmonero.org).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3141"/>
        <source>Submit a signed transaction from a file.</source>
        <translation>Invia una transazione firmata da file.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3145"/>
        <source>Change the current log detail (level must be &lt;0-4&gt;).</source>
        <translation>Modifica il dettaglio di log (il livello deve essere &lt;0-4&gt;).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3149"/>
        <source>If no arguments are specified, the wallet shows all the existing accounts along with their balances.
If the &quot;new&quot; argument is specified, the wallet creates a new account with its label initialized by the provided label text (which can be empty).
If the &quot;switch&quot; argument is specified, the wallet switches to the account specified by &lt;index&gt;.
If the &quot;label&quot; argument is specified, the wallet sets the label of the account specified by &lt;index&gt; to the provided label text.
If the &quot;tag&quot; argument is specified, a tag &lt;tag_name&gt; is assigned to the specified accounts &lt;account_index_1&gt;, &lt;account_index_2&gt;, ....
If the &quot;untag&quot; argument is specified, the tags assigned to the specified accounts &lt;account_index_1&gt;, &lt;account_index_2&gt; ..., are removed.
If the &quot;tag_description&quot; argument is specified, the tag &lt;tag_name&gt; is assigned an arbitrary text &lt;description&gt;.</source>
        <translation>Se non vengono specificati argomenti, il portafoglio mostra tutti gli account esistenti insieme ai loro saldi.
Se viene specificato l&apos;argomento &quot;new&quot;, il portafoglio crea un nuovo account con l&apos;etichetta inizializzata dal testo dell&apos;etichetta fornito (che può essere vuoto).
Se viene specificato l&apos;argomento &quot;switch&quot;, il portafoglio passa all&apos;account specificato da &lt;index&gt;.
Se viene specificato l&apos;argomento &quot;label&quot;, il portafoglio imposta l&apos;etichetta dell&apos;account specificato da &lt;index&gt; nel testo dell&apos;etichetta fornito.
Se l&apos;argomento &quot;tag&quot; è specificato, un tag &lt;tag_name&gt; viene assegnato agli account specificati &lt;account_index_1&gt;, &lt;account_index_2&gt;, ....
Se viene specificato l&apos;argomento &quot;untag&quot;, i tag assegnati agli account specificati &lt;account_index_1&gt;, &lt;account_index_2&gt; ..., vengono rimossi.
Se viene specificato l&apos;argomento &quot;tag_description&quot;, al tag &lt;tag_name&gt; viene assegnato un testo arbitrario &lt;description&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3163"/>
        <source>Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation>Codifica un ID di pagamento in un indirizzo integrato per l&apos;indirizzo pubblico del portafoglio corrente (nessun argomento utilizza un ID di pagamento casuale) o decodifica un indirizzo integrato per l&apos;indirizzo standard e l&apos;ID di pagamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3167"/>
        <source>Print all entries in the address book, optionally adding/deleting an entry to/from it.</source>
        <translation>Stampa tutte le voci nella rubrica, opzionalmente aggiungendo/eliminando una voce a/da esso.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3170"/>
        <source>Save the wallet data.</source>
        <translation>Salva i dati del portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3173"/>
        <source>Save a watch-only keys file.</source>
        <translation>Salva un file di chiavi in sola lettura.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3176"/>
        <source>Display the private view key.</source>
        <translation>Mostra la chiave privata di visualizzazione.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3179"/>
        <source>Display the private spend key.</source>
        <translation>Mostra la chiave di spesa privata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3182"/>
        <source>Display the Electrum-style mnemonic seed</source>
        <translation>Mostra il seed mnemonico di tipo Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3258"/>
        <source>Display the encrypted Electrum-style mnemonic seed.</source>
        <translation>Mostra il seed mnemonico criptato di tipo Electrum.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3261"/>
        <source>Rescan the blockchain for spent outputs.</source>
        <translation>Riscansiona la blockchain per gli outputs spesi.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3265"/>
        <source>Get the transaction key (r) for a given &lt;txid&gt;.</source>
        <translation>Ottieni la chiave di transazione (r) per il dato &lt;txid&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3273"/>
        <source>Check the amount going to &lt;address&gt; in &lt;txid&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3277"/>
        <source>Generate a signature proving funds sent to &lt;address&gt; in &lt;txid&gt;, optionally with a challenge string &lt;message&gt;, using either the transaction secret key (when &lt;address&gt; is not your wallet&apos;s address) or the view secret key (otherwise), which does not disclose the secret key.</source>
        <translation>Generare una firma che provi l&apos;invio dei fondi da &lt;address&gt; in &lt;txid&gt;, facoltativamente con una stringa di verifica &lt;message&gt;, utilizzando la chiave segreta della transazione (quando &lt;address&gt; non è l&apos;indirizzo del tuo portafoglio) o (in alternativa) la chiave di visualizzazione segreta, che non riveli la chiave segreta.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3281"/>
        <source>Check the proof for funds going to &lt;address&gt; in &lt;txid&gt; with the challenge string &lt;message&gt; if any.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3285"/>
        <source>Generate a signature proving that you generated &lt;txid&gt; using the spend secret key, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Genera una firma che dimostri che hai generato &lt;txid&gt; usando la chiave segreta di spesa, opzionalmente con una stringa di verifica &lt;message&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3289"/>
        <source>Check a signature proving that the signer generated &lt;txid&gt;, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Controlla la firma dimostrando che il firmatario ha generato &lt;txid&gt;, opzionalmente con una stringa di verifica &lt;message&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3293"/>
        <source>Generate a signature proving that you own at least this much, optionally with a challenge string &lt;message&gt;.
If &apos;all&apos; is specified, you prove the entire sum of all of your existing accounts&apos; balances.
Otherwise, you prove the reserve of the smallest possible amount above &lt;amount&gt; available in your current account.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3299"/>
        <source>Check a signature proving that the owner of &lt;address&gt; holds at least this much, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Controlla una firma che dimostri che il proprietario di &lt;address&gt; conserva questo valore, opzionalmente con una stringa di verifica &lt;message&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3319"/>
        <source>Show the unspent outputs of a specified address within an optional amount range.</source>
        <translation>Mostra gli outputs non spesi di uno specifico indirizzo all&apos;interno di un intervallo di quantità opzionale.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3323"/>
        <source>Rescan the blockchain from scratch. If &quot;hard&quot; is specified, you will lose any information which can not be recovered from the blockchain itself.</source>
        <translation>Scansiona la blockchain da zero. Se &quot;hard&quot; è specificato, perderai tutte le informazioni che non possono essere recuperate dalla blockchain stessa.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3327"/>
        <source>Set an arbitrary string note for a &lt;txid&gt;.</source>
        <translation>Imposta una nota arbitraria per un &lt;txid&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3331"/>
        <source>Get a string note for a txid.</source>
        <translation>Ottieni una nota di stringa per un txid.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3335"/>
        <source>Set an arbitrary description for the wallet.</source>
        <translation>Imposta una descrizione arbitraria per il portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3339"/>
        <source>Get the description of the wallet.</source>
        <translation>Ottieni la descrizione del portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3342"/>
        <source>Show the wallet&apos;s status.</source>
        <translation>Mostra lo stato del portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3345"/>
        <source>Show the wallet&apos;s information.</source>
        <translation>Mostra le informazioni del portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3349"/>
        <source>Sign the contents of a file.</source>
        <translation>Firma il contenuto di un file.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3353"/>
        <source>Verify a signature on the contents of a file.</source>
        <translation>Verifica una firma sul contenuto di un file.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3361"/>
        <source>Import a signed key images list and verify their spent status.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3373"/>
        <source>Export a set of outputs owned by this wallet.</source>
        <translation>Esporta un set di output posseduto da questo portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3377"/>
        <source>Import a set of outputs owned by this wallet.</source>
        <translation>Importa un set di outputs posseduto da questo portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3381"/>
        <source>Show information about a transfer to/from this address.</source>
        <translation>Mostra informazioni su un trasferimento da/verso questo indirizzo.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3384"/>
        <source>Change the wallet&apos;s password.</source>
        <translation>Cambia la password del portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3391"/>
        <source>Print the information about the current fee and transaction backlog.</source>
        <translation>Stampa le informazioni sulla commissione corrente e sul backlog della transazione.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3393"/>
        <source>Export data needed to create a multisig wallet</source>
        <translation>Esportare i dati necessari per creare un portafoglio multifirma</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3396"/>
        <source>Turn this wallet into a multisig wallet</source>
        <translation>Trasforma questo portafoglio in un portafoglio multifirma</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3400"/>
        <source>Turn this wallet into a multisig wallet, extra step for N-1/N wallets</source>
        <translation>Trasforma questo portafoglio in un portafoglio multifirma, passo aggiuntivo per i portafogli N-1/N</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3408"/>
        <source>Export multisig info for other participants</source>
        <translation>Esporta informazioni multifirma per altri partecipanti</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3412"/>
        <source>Import multisig info from other participants</source>
        <translation>Importa informazioni multifirma da altri partecipanti</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3416"/>
        <source>Sign a multisig transaction from a file</source>
        <translation>Firma una transazione multifirma da un file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3420"/>
        <source>Submit a signed multisig transaction from a file</source>
        <translation>Invia una transazione multifirma firmata da un file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3424"/>
        <source>Export a signed multisig transaction to a file</source>
        <translation>Esporta una transazione multifirma firmata in un file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3521"/>
        <source>Unsets the ring used for a given key image or transaction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3541"/>
        <source>Freeze a single output by key image so it will not be used</source>
        <translation>Congela un singolo output per immagine chiave in modo che non venga utilizzato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3545"/>
        <source>Thaw a single output by key image so it may be used again</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3549"/>
        <source>Checks whether a given output is currently frozen by key image</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3557"/>
        <source>Prints simple network stats</source>
        <translation>Stampa semplici statistiche di rete</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3565"/>
        <source>Prints basic info about Monero for first time users</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3585"/>
        <source>Show the help section or the documentation about a &lt;command&gt;.</source>
        <translation>Mostra la sezione di aiuto o la documentazione su un &lt;command&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3689"/>
        <source>integer &gt;= </source>
        <translation>intero &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3701"/>
        <source>block height</source>
        <translation>altezza del blocco</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3712"/>
        <source>1/yes or 0/no</source>
        <translation>1/si o 0/no</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3814"/>
        <source>No wallet found with that name. Confirm creation of new wallet named: </source>
        <translation>Nessun portafoglio trovato con quel nome. Conferma la creazione di un nuovo portafoglio chiamato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3940"/>
        <source>can&apos;t specify both --restore-deterministic-wallet or --restore-multisig-wallet and --non-deterministic</source>
        <translation>non è possibile specificare sia --restore-deterministic-wallet o --restore-multisig-wallet e --non-deterministic</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3946"/>
        <source>--restore-multisig-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-multisig-wallet utilizza --generate-new-wallet, non --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3962"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;multisig seed here&quot;</source>
        <translation>specifica un parametro di recupero con --electrum-seed=&quot; seed multifirma qui&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3991"/>
        <source>Multisig seed failed verification</source>
        <translation>Verifica seed mulitfirma fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4041"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4118"/>
        <source>This address is a subaddress which cannot be used here.</source>
        <translation>Questo indirizzo è un sottoindirizzo che non può essere utilizzato qui.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4196"/>
        <source>Error: expected M/N, but got: </source>
        <translation>Errore: atteso M/N, ma ottenuto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4201"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4206"/>
        <source>Error: M/N is currently unsupported. </source>
        <translation>Errore: M/N non è attualmente supportato. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4209"/>
        <source>Generating master wallet from %u of %u multisig wallet keys</source>
        <translation>Generazione del portafoglio principale da %u di %u chiavi del portafoglio multifirma</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4238"/>
        <source>failed to parse secret view key</source>
        <translation>impossibile fare l&apos;analisi  (parsing) della chiave segreta di visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4246"/>
        <source>failed to verify secret view key</source>
        <translation>verifica chiave segreta di visualizzazione fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4289"/>
        <source>Error: M/N is currently unsupported</source>
        <translation>Errore: M/N non è attualmente supportato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4455"/>
        <source>Restore height </source>
        <translation>Altezza di ripristino </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4521"/>
        <source>If you are new to Monero, type &quot;welcome&quot; for a brief overview.</source>
        <translation>Se non conosci Monero, digita &quot;welcome&quot; per una breve panoramica.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4585"/>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or change the daemon address using the &apos;set_daemon&apos; command.</source>
        <translation>Il daemon non è stato avviato o la porta è errata. Assicurati che il daemon sia in esecuzione oppure cambia l&apos;indirizzo del daemon usando il comando &apos;set_daemon&apos;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4623"/>
        <source>Enter the number corresponding to the language of your choice</source>
        <translation>Inserire il numero corrispondente alla lingua desiderata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4662"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4759"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4807"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4850"/>
        <source>Error creating wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4735"/>
        <source>Your wallet has been generated!
To start synchronizing with the daemon, use the &quot;refresh&quot; command.
Use the &quot;help&quot; command to see the list of available commands.
Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
Always use the &quot;exit&quot; command when closing monero-wallet-cli to save 
your current session&apos;s state. Otherwise, you might need to synchronize 
your wallet again (your wallet keys are NOT at risk in any case).
</source>
        <translation>Il tuo portafoglio è stato generato!
Per avviare la sincronizzazione con il daemon, utilizzare il comando &quot;refresh&quot;.
Utilizzare il comando &quot;help&quot; per visualizzare l&apos;elenco dei comandi disponibili.
Usa &quot;help &lt;command&gt;&quot; per vedere la documentazione di un comando.
Usa sempre il comando &quot;exit&quot; quando chiudi monero-wallet-cli per salvare lo 
stato della sessione corrente. In caso contrario, potrebbe essere necessario sincronizzare 
di nuovo il tuo portafoglio (le chiavi del tuo portafoglio NON sono in nessun caso a rischio).
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4885"/>
        <source>failed to generate new mutlisig wallet</source>
        <translation>impossibile generare un nuovo portafoglio multifirma</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4888"/>
        <source>Generated new %u/%u multisig wallet: </source>
        <translation>Generato nuovo portafoglio %u/%u multifirma: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4937"/>
        <source>Opened %u/%u multisig wallet%s</source>
        <translation>Aperto %u/%u portafoglio multifirma%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4998"/>
        <source>Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
</source>
        <translation>Usa &quot;help &lt;command&gt;&quot; per vedere la documentazione di un comando.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5057"/>
        <source>wallet is multisig and cannot save a watch-only version</source>
        <translation>il portafoglio è multifirma e non è possibile salvarne una versione in sola lettura</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5093"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5125"/>
        <source>Failed to query mining status: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5108"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5136"/>
        <source>Failed to setup background mining: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5112"/>
        <source>Background mining enabled. Thank you for supporting the Monero network.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5140"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5148"/>
        <source>Background mining not enabled. Run &quot;set setup-background-mining 1&quot; to change.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5154"/>
        <source>Using an untrusted daemon, skipping background mining check</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5179"/>
        <source>The daemon is not set up to background mine.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5181"/>
        <source>Enabling this supports the network you are using, and makes you eligible for receiving new monero</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5186"/>
        <source>Background mining not enabled. Set setup-background-mining to 1 to change.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5327"/>
        <source>Unexpected array length - Exited simple_wallet::set_daemon()</source>
        <translation>Lunghezza array imprevista - Exited simple_wallet:: set_daemon ()</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5389"/>
        <source>This does not seem to be a valid daemon URL.</source>
        <translation>Questo URL del daemon non sembra essere valido.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5429"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5475"/>
        <source>txid </source>
        <translation>txid </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5431"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5477"/>
        <source>idx </source>
        <translation>idx </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5455"/>
        <source>NOTE: This transaction is locked, see details with: show_transfer </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5605"/>
        <source>New transfer received since rescan was started. Key images are incomplete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5628"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5959"/>
        <source>payment required.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5688"/>
        <source> (Some owned outputs have partial key images - import_multisig_info needed)</source>
        <translation> (Alcuni output contengono immagini chiave parziali - import_multisig_info necessario)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5691"/>
        <source>Currently selected account: [</source>
        <translation>Account attualmente selezionato: [</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5691"/>
        <source>] </source>
        <translation>] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5693"/>
        <source>Tag: </source>
        <translation>Etichetta: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5693"/>
        <source>(No tag assigned)</source>
        <translation>(Nessuna etichetta assegnata)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5705"/>
        <source>Balance per address:</source>
        <translation>Saldo per indirizzo:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5706"/>
        <source>Address</source>
        <translation>Indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5706"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9157"/>
        <source>Balance</source>
        <translation>Saldo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5706"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9157"/>
        <source>Unlocked balance</source>
        <translation>Saldo sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5706"/>
        <source>Outputs</source>
        <translation>Outputs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5706"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9157"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10319"/>
        <source>Label</source>
        <translation>Etichetta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5714"/>
        <source>%8u %6s %21s %21s %7u %21s</source>
        <translation>%8u %6s %21s %21s %7u %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5807"/>
        <source>spent</source>
        <translation>spesi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5807"/>
        <source>global index</source>
        <translation>indice globale</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5807"/>
        <source>tx id</source>
        <translation>tx id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5807"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5869"/>
        <source>addr index</source>
        <translation>indice indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5817"/>
        <source>Used at heights: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5823"/>
        <source>[frozen]</source>
        <translation>[congelato]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5837"/>
        <source>No incoming transfers</source>
        <translation>Nessun trasferimento in entrata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5841"/>
        <source>No incoming available transfers</source>
        <translation>Nessun trasferimento in entrata disponibile</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5845"/>
        <source>No incoming unavailable transfers</source>
        <translation>Nessun trasferimento indisponibile in entrata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5869"/>
        <source>payment</source>
        <translation>pagamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5869"/>
        <source>transaction</source>
        <translation>transazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5869"/>
        <source>height</source>
        <translation>altezza</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5869"/>
        <source>unlock time</source>
        <translation>tempo sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5881"/>
        <source>No payments with id </source>
        <translation>Nessun pagamento con id </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5929"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6024"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6426"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6891"/>
        <source>failed to get blockchain height: </source>
        <translation>impossibile recuperare altezza blockchain: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6032"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation>
Transazione %llu/%llu: txid=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6070"/>
        <source>failed to get output: </source>
        <translation>impossibile recuperare output: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6078"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation>l&apos;altezza del blocco di origine della chiave di output non dovrebbe essere più alta dell&apos;altezza della blockchain</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6082"/>
        <source>
Originating block heights: </source>
        <translation>
Originando blocchi: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6094"/>
        <source>
|</source>
        <translation>
|</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6094"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8693"/>
        <source>|
</source>
        <translation>|
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6111"/>
        <source>
Warning: Some input keys being spent are from </source>
        <translation>
Avviso: alcune chiavi di input spese vengono da </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6113"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation>, che potrebbe compromettere l&apos;anonimità della ring signature. Assicurati di farlo intenzionalmente!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6144"/>
        <source>Transaction spends more than one very old output. Privacy would be better if they were sent separately.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6145"/>
        <source>Spend them now anyway?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6248"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6843"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7120"/>
        <source>Ring size must not be 0</source>
        <translation>Il ring size non può essere 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6260"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6855"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7132"/>
        <source>ring size %u is too small, minimum is %u</source>
        <translation>il ring size %u è troppo piccolo, il minimo è %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6272"/>
        <source>wrong number of arguments</source>
        <translation>numero di argomenti errato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6442"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6977"/>
        <source>No outputs found, or daemon is not ready</source>
        <translation>Nessun output trovato, o il daemon non è pronto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6551"/>
        <source>.
This transaction (including %s change) will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7722"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7733"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7740"/>
        <source>failed to parse tx_key</source>
        <translation>impossibile analizzare tx_key</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7749"/>
        <source>Tx key successfully stored.</source>
        <translation>Tx key memorizzata con successo.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7753"/>
        <source>Failed to store tx key: </source>
        <translation>Impossibile memorizzare la chiave tx: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8256"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8505"/>
        <source>block</source>
        <translation>blocco</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8427"/>
        <source>usage: show_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation>utilizzo: show_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8480"/>
        <source>usage: export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;path&gt;]</source>
        <translation>utilizzo: export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;path&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8505"/>
        <source>direction</source>
        <translation>direzione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8505"/>
        <source>timestamp</source>
        <translation>timestamp</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8505"/>
        <source>running balance</source>
        <translation>bilancio corrente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8505"/>
        <source>hash</source>
        <translation>hash</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8505"/>
        <source>payment ID</source>
        <translation>ID di pagamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8505"/>
        <source>fee</source>
        <translation>tassa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8505"/>
        <source>destination</source>
        <translation>destinazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8505"/>
        <source>index</source>
        <translation>indice</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8505"/>
        <source>note</source>
        <translation>nota</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8559"/>
        <source>CSV exported to </source>
        <translation>CSV esportato in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8742"/>
        <source>Warning: this will lose any information which can not be recovered from the blockchain.</source>
        <translation>Attenzione: questo farà perdere ogni informazione che non può essere recuperata dalla blockchain.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8743"/>
        <source>This includes destination addresses, tx secret keys, tx notes, etc</source>
        <translation>Questo include gli indirizzi di destinazione, le chiavi segrete di tx, le note di tx, ecc</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8755"/>
        <source>Warning: your restore height is higher than wallet restore height: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8756"/>
        <source>Rescan anyway ? (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8775"/>
        <source>MMS received new message</source>
        <translation>ricevuto un nuovo messaggio MMS</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9284"/>
        <source>&lt;index&gt; is out of bounds</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9562"/>
        <source>Network type: </source>
        <translation>Tipo di rete: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9563"/>
        <source>Testnet</source>
        <translation>Testnet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9564"/>
        <source>Stagenet</source>
        <translation>Stagenet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9564"/>
        <source>Mainnet</source>
        <translation>Mainnet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9739"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9785"/>
        <source>command only supported by HW wallet</source>
        <translation>comando supportato solo dal portafoglio HW</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9744"/>
        <source>hw wallet does not support cold KI sync</source>
        <translation>Il portafoglio hardware non supporta la sincronizzazione cold KI</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9756"/>
        <source>Please confirm the key image sync on the device</source>
        <translation>Si prega di confermare la sincronizzazione della chiave immagine sul dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9762"/>
        <source>Key images synchronized to height </source>
        <translation>Immagini della chiave sincronizzate all&apos;altezza </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9765"/>
        <source>Running untrusted daemon, cannot determine which transaction output is spent. Use a trusted daemon with --trusted-daemon and run rescan_spent</source>
        <translation>L&apos;esecuzione di un daemon non fidato non può determinare quale output di transazione viene speso. Utilizzare un daemon fidato con --trusted-daemon ed eseguire rescan_spent</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9768"/>
        <source> spent, </source>
        <translation> speso, </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9768"/>
        <source> unspent</source>
        <translation> non speso</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9772"/>
        <source>Failed to import key images</source>
        <translation>Impossibile importare le immagini della chiave</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9777"/>
        <source>Failed to import key images: </source>
        <translation>Impossibile importare le key images: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9794"/>
        <source>Failed to reconnect device</source>
        <translation>Impossibile ricollegare il dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9799"/>
        <source>Failed to reconnect device: </source>
        <translation>Impossibile ricollegare il dispositivo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10076"/>
        <source>Transaction successfully saved to </source>
        <translation>Transazione salvata correttamente in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10076"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10078"/>
        <source>, txid </source>
        <translation>, txid </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10078"/>
        <source>Failed to save transaction to </source>
        <translation>Impossibile salvare la transazione in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7572"/>
        <source>This is a watch only wallet</source>
        <translation>Questo è un portafoglio solo-vista</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10006"/>
        <source>Double spend seen on the network: this transaction may or may not end up being mined</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10041"/>
        <source>Transaction ID not found</source>
        <translation>ID transazione non trovato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="373"/>
        <source>true</source>
        <translation>vero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="426"/>
        <source>failed to parse refresh type</source>
        <translation>impossibile fare il parsing del tipo di refresh</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="766"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="838"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="982"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1027"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1110"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1167"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1234"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1300"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1394"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1510"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1591"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7562"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7626"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7663"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7968"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8052"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9572"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9649"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9702"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9810"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9859"/>
        <source>command not supported by HW wallet</source>
        <translation>comando non supportato dal portafoglio HW</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="771"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="848"/>
        <source>wallet is watch-only and has no seed</source>
        <translation>il portafoglio è solo-vista e non possiede un seed</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="789"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="858"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation>il portafoglio è non-deterministico e non possiede un seed</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="796"/>
        <source>Enter optional seed offset passphrase, empty to see raw seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="868"/>
        <source>Incorrect password</source>
        <translation>Password errata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="926"/>
        <source>Current fee is %s %s per %s</source>
        <translation>La commissione attuale è %s %s per %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1079"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1201"/>
        <source>Send this multisig info to all other participants, then use exchange_multisig_keys &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Invia queste informazioni multifirma a tutti i partecipanti, quindi utilizza exchange_multisig_keys &lt;info1&gt; [&lt;info2&gt;...] con le informazioni multifirma degli altri partecipanti</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1210"/>
        <source>Multisig wallet has been successfully created. Current wallet type: </source>
        <translation>Il portafoglio multifirma è stato creato con successo. Tipo di portafoglio corrente: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1216"/>
        <source>Failed to perform multisig keys exchange: </source>
        <translation>Impossibile eseguire lo scambio di chiavi multifirma: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1543"/>
        <source>Failed to load multisig transaction from MMS</source>
        <translation>Impossibile caricare la transazione multifirma da MMS</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1675"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1832"/>
        <source>Invalid key image</source>
        <translation>Immagine della chiave non valida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1681"/>
        <source>Invalid txid</source>
        <translation>Txid non valido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1693"/>
        <source>Key image either not spent, or spent with mixin 0</source>
        <translation>Immagine della chiave non spesa o spesa con mixin 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1708"/>
        <source>Failed to get key image ring: </source>
        <translation>Impossibile ottenere l&apos;anello della chiave immagine: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1723"/>
        <source>File doesn&apos;t exist</source>
        <translation>Il file non esiste</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1745"/>
        <source>Invalid ring specification: </source>
        <translation>Specifica dell&apos;anello non valida: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1753"/>
        <source>Invalid key image: </source>
        <translation>Immagine della chiave non valida: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1758"/>
        <source>Invalid ring type, expected relative or abosolute: </source>
        <translation>Tipo di anello non valido, previsto relativo o assoluto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1764"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1776"/>
        <source>Error reading line: </source>
        <translation>Errore durante la lettura della riga: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1787"/>
        <source>Invalid ring: </source>
        <translation>Anello non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1796"/>
        <source>Invalid relative ring: </source>
        <translation>Anello relativo non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1808"/>
        <source>Invalid absolute ring: </source>
        <translation>Anello assoluto non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1817"/>
        <source>Failed to set ring for key image: </source>
        <translation>Impossibile impostare l&apos;anello per l&apos;immagine della chiave: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1817"/>
        <source>Continuing.</source>
        <translation>Continuando.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1847"/>
        <source>Missing absolute or relative keyword</source>
        <translation>Parola chiave assoluta o relativa mancante</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1857"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1864"/>
        <source>invalid index: must be a strictly positive unsigned integer</source>
        <translation>indice non valido: deve essere un numero intero senza segno e positivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1872"/>
        <source>invalid index: indices wrap</source>
        <translation>indice non valido: indici wrap</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1882"/>
        <source>invalid index: indices should be in strictly ascending order</source>
        <translation>indice non valido: gli indici devono essere rigorosamente in ordine crescente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1889"/>
        <source>failed to set ring</source>
        <translation>impossibile impostare l&apos;anello</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2037"/>
        <source>First line is not an amount</source>
        <translation>La prima riga non è un importo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2051"/>
        <source>Invalid output: </source>
        <translation>Output non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2061"/>
        <source>Bad argument: </source>
        <translation>Argomento non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2061"/>
        <source>should be &quot;add&quot;</source>
        <translation>dovrebbe essere &quot;add&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2070"/>
        <source>Failed to open file</source>
        <translation>Impossibile aprire il file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2076"/>
        <source>Invalid output key, and file doesn&apos;t exist</source>
        <translation>Chiave di output non valida, il file non esiste</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2082"/>
        <source>Failed to mark output spent: </source>
        <translation>Impossibile contrassegnare l&apos;output come speso: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2099"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2126"/>
        <source>Invalid output</source>
        <translation>Output non valido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2109"/>
        <source>Failed to mark output unspent: </source>
        <translation>Impossibile contrassegnare l&apos;output non speso: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2133"/>
        <source>Spent: </source>
        <translation>Speso: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2135"/>
        <source>Not spent: </source>
        <translation>Non speso: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2139"/>
        <source>Failed to check whether output is spent: </source>
        <translation>Impossibile verificare se l&apos;output è stato speso: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2154"/>
        <source>Failed to save known rings: </source>
        <translation>Impossibile salvare gli anelli conosciuti: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2285"/>
        <source>Unlike Bitcoin, your Monero transactions and balance stay private and are not visible to the world by default.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2286"/>
        <source>However, you have the option of making those available to select parties if you choose to.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2294"/>
        <source>Welcome to Monero and financial privacy. For more information see https://GetMonero.org</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2308"/>
        <source>Unknown command &apos;%s&apos;, try &apos;help&apos;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2327"/>
        <source>Please confirm the transaction on the device</source>
        <translation>Si prega di confermare la transazione sul dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2418"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2437"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation>il portafoglio è solo-vista e non può eseguire trasferimenti</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2462"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6573"/>
        <source>WARNING: this is a non default ring size, which may harm your privacy. Default is recommended.</source>
        <translation>ATTENZIONE: impostazione dell&apos;anello non default, la tua privacy potrebbe essere compromessa. Si consiglia di utilizzare l&apos;impostazione predefinita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2458"/>
        <source>WARNING: from v8, ring size will be fixed and this setting will be ignored.</source>
        <translation>ATTENZIONE: dalla v8, la dimensione dell&apos;anello sarà fissa e questa impostazione verrà ignorata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="523"/>
        <source>Payment required, see the &apos;rpc_payment_info&apos; command</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1944"/>
        <source>RPC client ID: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1945"/>
        <source>RPC client secret key: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1948"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2358"/>
        <source>Failed to query daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1956"/>
        <source>Using daemon: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1957"/>
        <source>Payments required for node use, current credits: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1958"/>
        <source>Credits target: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1961"/>
        <source>Credits spent this session: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1963"/>
        <source>Credit discrepancy this session: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1965"/>
        <source>Difficulty: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1965"/>
        <source>credits per hash found, </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1965"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2261"/>
        <source>credits/hash</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1973"/>
        <source>Mining for payment at %.1f H/s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1978"/>
        <source>Estimated time till %u credits target mined: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1982"/>
        <source>Mining for payment</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1985"/>
        <source>Not mining</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1988"/>
        <source>No payment needed for node use</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2249"/>
        <source>No known public nodes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2261"/>
        <source>address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2261"/>
        <source>last_seen</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2267"/>
        <source>never</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2275"/>
        <source>Error retrieving public node list: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2363"/>
        <source>Daemon does not require payment for RPC access</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2371"/>
        <source>Starting mining for RPC access: diff %llu, %f credits/hash%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2372"/>
        <source>Run stop_mining_for_rpc to stop</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2493"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2516"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2532"/>
        <source>priority must be either 0, 1, 2, 3, or 4, or one of: </source>
        <translation>la priorità deve essere 0, 1, 2, 3 o 4 o una di: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2537"/>
        <source>could not change default priority</source>
        <translation>impossibile cambiare priorità standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2592"/>
        <source>invalid argument: must be either 0/never, 1/action, or 2/encrypt/decrypt</source>
        <translation>argomento non valido: deve essere 0/never, 1/action, or 2/encrypt/decrypt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2814"/>
        <source>Invalid target</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2934"/>
        <source>Inactivity lock timeout disabled on Windows</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2948"/>
        <source>Invalid number of seconds</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2985"/>
        <source>Device name not specified</source>
        <translation>Nome del dispositivo non specificato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2994"/>
        <source>Device reconnect failed</source>
        <translation>Riconnessione del dispositivo fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2999"/>
        <source>Device reconnect failed: </source>
        <translation>Riconnessione del dispositivo fallita: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3010"/>
        <source>Export format not specified</source>
        <translation>Formato di esportazione non specificato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3024"/>
        <source>Export format not recognized.</source>
        <translation>Formato di esportazione non riconosciuto.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3108"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt;. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Trasferisci &lt;amount&gt; in &lt;address&gt;. Se viene specificato il parametro &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; il portafoglio utilizza le uscite ricevute dagli indirizzi di tali indici. Se omesso, il portafoglio sceglie casualmente gli indici dell&apos;indirizzo da utilizzare. In ogni caso, fa del suo meglio per non combinare gli output su più indirizzi. &lt;priority&gt; è la priorità della transazione. Maggiore è la priorità, maggiore è la commissione di transazione. I valori validi in ordine di priorità (dal più basso al più alto) sono: non importante, normale, elevata, priorità. Se omesso, viene utilizzato il valore predefinito (consultare il comando &quot;set priority&quot;). &lt;ring_size&gt; è il numero di input da includere per la non tracciabilità. È possibile effettuare pagamenti multipli contemporaneamente aggiungendo URI_2 o &lt;address_2&gt; &lt;amount_2&gt; eccetera (prima dell&apos;ID pagamento, se questo è incluso)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3112"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Trasferisci &lt;amount&gt; in &lt;address&gt; e bloccalo per &lt;lockblocks&gt; (massimo 1000000). Se viene specificato il parametro &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot;, il portafoglio utilizza le uscite ricevute dagli indirizzi di tali indici. Se omesso, il portafoglio sceglie casualmente gli indici dell&apos;indirizzo da utilizzare. In ogni caso, fa del suo meglio per non combinare gli output su più indirizzi. &lt;priority&gt; è la priorità della transazione. Maggiore è la priorità, maggiore è la commissione di transazione. I valori validi in ordine di priorità (dal più basso al più alto) sono: non importante, normale, elevata, priorità. Se omesso, viene utilizzato il valore predefinito (consultare il comando &quot;set priority&quot;). &lt;ring_size&gt; è il numero di input da includere per la non tracciabilità. È possibile effettuare pagamenti multipli contemporaneamente aggiungendo URI_2 o &lt;address_2&gt; &lt;amount_2&gt; eccetera (prima dell&apos;ID pagamento, se questo è incluso)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3116"/>
        <source>Send all unlocked balance to an address and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. &lt;priority&gt; is the priority of the sweep. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability.</source>
        <translation>Invia tutto il saldo sbloccato a un indirizzo e bloccalo per &lt;lockblocks&gt; (massimo 1000000). Se viene specificato il parametro &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot;, le uscite sweep del portafoglio vengono ricevute da tali indici dell&apos;indirizzo. Se omesso, il portafoglio sceglie casualmente un indice dell&apos;indirizzo da utilizzare. &lt;priority&gt; è la priorità dello sweep. Maggiore è la priorità, maggiore è la commissione di transazione. I valori validi in ordine di priorità (dal più basso al più alto) sono: non importante, normale, elevata, priorità. Se omesso, viene utilizzato il valore predefinito (consultare il comando &quot;set priority&quot;). &lt;ring_size&gt; è il numero di input da includere per la non tracciabilità.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3122"/>
        <source>Send all unlocked balance to an address. If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. If the parameter &quot;outputs=&lt;N&gt;&quot; is specified and  N &gt; 0, wallet splits the transaction into N even outputs.</source>
        <translation>Invia tutto il saldo sbloccato a un indirizzo. Se viene specificato il parametro &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot;, le uscite sweep del portafoglio vengono ricevute da tali indici dell&apos;indirizzo. Se omesso, il portafoglio sceglie casualmente un indice dell&apos;indirizzo da utilizzare. Se il parametro &quot;outputs=&lt;N&gt;&quot; è specificato e  N &gt; 0, il portafoglio divide la transazione in N uscite pari.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3138"/>
        <source>Sign a transaction from a file. If the parameter &quot;export_raw&quot; is specified, transaction raw hex data suitable for the daemon RPC /sendrawtransaction is exported.</source>
        <translation>Firma una transazione da un file. Se viene specificato il parametro &quot;export_raw&quot;, vengono esportati i dati esadecimali grezzi della transazione adatti al daemon RPC /sendrawtransaction.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3159"/>
        <source>If no arguments are specified or &lt;index&gt; is specified, the wallet shows the default or specified address. If &quot;all&quot; is specified, the wallet shows all the existing addresses in the currently selected account. If &quot;new &quot; is specified, the wallet creates a new address with the provided label text (which can be empty). If &quot;label&quot; is specified, the wallet sets the label of the address specified by &lt;index&gt; to the provided label text.</source>
        <translation>Se non viene specificato alcun argomento o viene specificato &lt;index&gt;, il portafoglio mostra l&apos;indirizzo predefinito o specificato. Se è specificato &quot;all&quot;, il portafoglio mostra tutti gli indirizzi esistenti nell&apos;account attualmente selezionato. Se viene specificato &quot;new&quot;, il portafoglio crea un nuovo indirizzo con il testo dell&apos;etichetta fornito (che può essere vuoto). Se si specifica &quot;label&quot;, il portafoglio imposta l&apos;etichetta dell&apos;indirizzo specificato da &lt;index&gt; sul testo dell&apos;etichetta fornito.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3185"/>
        <source>Display the restore height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3189"/>
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
 confirm-missing-payment-id &lt;1|0&gt; (obsolete)
 ask-password &lt;0|1|2   (or never|action|decrypt)&gt;
   action: ask the password before many actions such as transfer, etc
   decrypt: same as action, but keeps the spend key encrypted in memory when not needed
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
 confirm-export-overwrite &lt;1|0&gt;
   Whether to warn if the file to be exported already exists.
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
 segregation-height &lt;n&gt;
   Set to the height of a key reusing fork you want to use, 0 to use default.
 ignore-fractional-outputs &lt;1|0&gt;
   Whether to ignore fractional outputs that result in net loss when spending due to fee.
 ignore-outputs-above &lt;amount&gt;
   Ignore outputs of amount above this threshold when spending. Value 0 is translated to the maximum value (18 million) which disables this filter.
 ignore-outputs-below &lt;amount&gt;
   Ignore outputs of amount below this threshold when spending.
 track-uses &lt;1|0&gt;
   Whether to keep track of owned outputs uses.
 setup-background-mining &lt;1|0&gt;
   Whether to enable background mining. Set this to support the network and to get a chance to receive new monero.
 device-name &lt;device_name[:device_spec]&gt;
   Device name for hardware wallet.
 export-format &lt;&quot;binary&quot;|&quot;ascii&quot;&gt;
   Save all exported files as binary (cannot be copied and pasted) or ascii (can be).
 persistent-client-id &lt;1|0&gt;
   Whether to keep using the same client id for RPC payment over wallet restarts.
auto-mine-for-rpc-payment-threshold &lt;float&gt;
   Whether to automatically start mining for RPC payment if the daemon requires it.
credits-target &lt;unsigned int&gt;
  The RPC payment credits balance to target (0 for default).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3269"/>
        <source>Set the transaction key (r) for a given &lt;txid&gt; in case the tx was made by some other device or 3rd party wallet.</source>
        <translation>Imposta la chiave di transazione (r) per un dato &lt;txid&gt; nel caso in cui il tx sia stato creato da qualche altro dispositivo o portafoglio di terze parti.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3304"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="3314"/>
        <source>export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;filepath&gt;]</source>
        <translation>export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;filepath&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3315"/>
        <source>Export to CSV the incoming/outgoing transfers within an optional height range.</source>
        <translation>Esporta in CSV i trasferimenti in entrata/uscita all&apos;interno di un intervallo di altezza opzionale.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3357"/>
        <source>Export a signed set of key images to a &lt;filename&gt;.</source>
        <translation>Esporta un set firmato di immagini chiave in un &lt;filename&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3365"/>
        <source>Synchronizes key images with the hw wallet.</source>
        <translation>Sincronizza le immagini chiave con il portafoglio hw.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3369"/>
        <source>Attempts to reconnect HW wallet.</source>
        <translation>Tenta di ricollegare il portafoglio HW.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3388"/>
        <source>Generate a new random full size payment id (obsolete). These will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids.</source>
        <translation>Genera un nuovo ID di pagamento lungo casuale (obsoleto). Questi non saranno crittati sulla blockchain, vedere integrated_address per gli ID di pagamento brevi crittografati.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3404"/>
        <source>Performs extra multisig keys exchange rounds. Needed for arbitrary M/N multisig wallets</source>
        <translation>Esegue scambi extra di chiavi multifirma. Necessario per portafogli multisig M/N arbitrari</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3428"/>
        <source>Interface with the MMS (Multisig Messaging System)
&lt;subcommand&gt; is one of:
  init, info, signer, list, next, sync, transfer, delete, send, receive, export, note, show, set, help
  send_signer_config, start_auto_config, stop_auto_config, auto_config
Get help about a subcommand with: help mms &lt;subcommand&gt;, or mms help &lt;subcommand&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3436"/>
        <source>Initialize and configure the MMS for M/N = number of required signers/number of authorized signers multisig</source>
        <translation>Inizializza e configura MMS per M/N = numero di firmatari richiesti/numero di multifirma dei firmatari autorizzati</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3440"/>
        <source>Display current MMS configuration</source>
        <translation>Mostra la configurazione MMS corrente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3444"/>
        <source>Set or modify authorized signer info (single-word label, transport address, Monero address), or list all signers</source>
        <translation>Imposta o modifica le informazioni del firmatario autorizzato (etichetta con una sola parola, indirizzo di trasporto, indirizzo Monero) oppure elenca tutti i firmatari</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3448"/>
        <source>List all messages</source>
        <translation>Elenca tutti i messaggi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3452"/>
        <source>Evaluate the next possible multisig-related action(s) according to wallet state, and execute or offer for choice
By using &apos;sync&apos; processing of waiting messages with multisig sync info can be forced regardless of wallet state</source>
        <translation>Valuta le azioni multifirma successive secondo lo stato del portafoglio, esegui o proponi una scelta
Utilizzando l&apos;elaborazione &apos;sync&apos; dei messaggi in attesa con le informazioni di sincronizzazione multifirma, è possibile forzare indipendentemente dallo stato del wallet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3457"/>
        <source>Force generation of multisig sync info regardless of wallet state, to recover from special situations like &quot;stale data&quot; errors</source>
        <translation>Generazione forzata di informazioni di sincronizzazione multifirma indipendente dallo stato del wallet, per il ripristino da situazioni speciali come errori di &quot;stale data&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3461"/>
        <source>Initiate transfer with MMS support; arguments identical to normal &apos;transfer&apos; command arguments, for info see there</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3465"/>
        <source>Delete a single message by giving its id, or delete all messages by using &apos;all&apos;</source>
        <translation>Elimina un singolo messaggio inserendo il suo id, o cancella tutti i messaggi usando &apos;all&apos;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3469"/>
        <source>Send a single message by giving its id, or send all waiting messages</source>
        <translation>Invia un singolo messaggio inserendo il suo id o inviando tutti i messaggi in attesa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3473"/>
        <source>Check right away for new messages to receive</source>
        <translation>Controlla subito i nuovi messaggi da ricevere</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3477"/>
        <source>Write the content of a message to a file &quot;mms_message_content&quot;</source>
        <translation>Scrivi il contenuto di un messaggio in un file &quot;mms_message_content&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3481"/>
        <source>Send a one-line message to an authorized signer, identified by its label, or show any waiting unread notes</source>
        <translation>Invia un messaggio di una linea a un firmatario autorizzato, identificato dalla sua etichetta, o mostra eventuali note non lette in attesa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3485"/>
        <source>Show detailed info about a single message</source>
        <translation>Mostra informazioni dettagliate su un singolo messaggio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3489"/>
        <source>Available options:
 auto-send &lt;1|0&gt;
   Whether to automatically send newly generated messages right away.
 </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3495"/>
        <source>Send completed signer config to all other authorized signers</source>
        <translation>Invia la configurazione completa del firmatario a tutti gli altri firmatari autorizzati</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3499"/>
        <source>Start auto-config at the auto-config manager&apos;s wallet by issuing auto-config tokens and optionally set others&apos; labels</source>
        <translation>Avvia auto-config sul portafoglio del gestore automatico di configurazione emettendo i token di auto-configurazione e opzionalmente imposta le etichette degli altri</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3503"/>
        <source>Delete any auto-config tokens and abort a auto-config process</source>
        <translation>Elimina tutti i token di configurazione automatica e interrompi il processo di configurazione automatica</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3507"/>
        <source>Start auto-config by using the token received from the auto-config manager</source>
        <translation>Avviare la configurazione automatica utilizzando il token ricevuto dal gestore di configurazione automatica</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3511"/>
        <source>Print the ring(s) used to spend a given key image or transaction (if the ring size is &gt; 1)

Output format:
Key Image, &quot;absolute&quot;, list of rings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3517"/>
        <source>Set the ring used for a given key image, so it can be reused in a fork</source>
        <translation>Imposta l&apos;anello utilizzato per una determinata immagine chiave, in modo che possa essere riutilizzato in un fork</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3525"/>
        <source>Save known rings to the shared rings database</source>
        <translation>Salva gli anelli noti nel database degli anelli condivisi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3529"/>
        <source>Mark output(s) as spent so they never get selected as fake outputs in a ring</source>
        <translation>Contrassegna gli output come spesi in modo che non vengano mai selezionati come output falsi in un anello</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3533"/>
        <source>Marks an output as unspent so it may get selected as a fake output in a ring</source>
        <translation>Contrassegna un output come non speso in modo che possa essere selezionato come un falso output in un anello</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3537"/>
        <source>Checks whether an output is marked as spent</source>
        <translation>Controlla se un output è contrassegnato come speso</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3553"/>
        <source>Lock the wallet console, requiring the wallet password to continue</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3561"/>
        <source>Lists known public nodes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3569"/>
        <source>Returns version information</source>
        <translation>Restituisce informazioni sulla versione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3573"/>
        <source>Get info about RPC payments to current node</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3577"/>
        <source>Start mining to pay for RPC access</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3581"/>
        <source>Stop mining to pay for RPC access</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3691"/>
        <source>full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)</source>
        <translation>completo (più lento, nessuna ipotesi); optimize-coinbase (veloce, ipotizza che l&apos;intero coinbase viene pagato ad un indirizzo singolo); no-coinbase (il più veloce, ipotizza di non ricevere una transazione coinbase), default (come optimize-coinbase)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3692"/>
        <source>0, 1, 2, 3, or 4, or one of </source>
        <translation>0, 1, 2, 3 o 4 o uno di </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3693"/>
        <source>0|1|2 (or never|action|decrypt)</source>
        <translation>0|1|2 (o never|action|decrypt)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3694"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation>monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3705"/>
        <source>&lt;major&gt;:&lt;minor&gt;</source>
        <translation>&lt;major&gt;:&lt;minor&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3711"/>
        <source>unsigned integer (seconds, 0 to disable)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3713"/>
        <source>&lt;device_name[:device_spec]&gt;</source>
        <translation>&lt;device_name[:device_spec]&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3714"/>
        <source>&quot;binary&quot; or &quot;ascii&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3716"/>
        <source>floating point &gt;= 0</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3738"/>
        <source>wrong number range, use: %s</source>
        <translation>intervallo di numeri errato, utilizzare: %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3777"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation>Nome del portafoglio non valido. Prova di nuovo o usa Ctrl-C per uscire</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3794"/>
        <source>Wallet and key files found, loading...</source>
        <translation>Portafoglio e chiavi trovate, sto caricando...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3800"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation>Ho trovato la chiave ma non il portafoglio. Sto rigenerando...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3806"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation>Chiave non trovata. Impossibile aprire portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3825"/>
        <source>Generating new wallet...</source>
        <translation>Sto generando un nuovo portafoglio...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3843"/>
        <source>NOTE: the following %s can be used to recover access to your wallet. Write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation>NOTA: il seguente %s può essere utilizzato per recuperare l&apos;accesso al tuo portafoglio. Scrivili e salvali in un posto sicuro e protetto. Si prega di non archiviarli nella propria e-mail o in servizi di archiviazione di file al di fuori del proprio immediato controllo.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3845"/>
        <source>string</source>
        <translation>stringa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3845"/>
        <source>25 words</source>
        <translation>25 parole</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3906"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation>Non è possibile specificare più di un --testnet e --stagenet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3921"/>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-device=&quot;wallet_name&quot;</source>
        <translation>non è possibile specificare più di un --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-device=&quot;wallet_name&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3948"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-deterministic-wallet usa --generate-new-wallet, non --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4000"/>
        <source>Electrum-style word list failed verification</source>
        <translation>La lista di parole stile Electrum ha fallito la verifica</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4005"/>
        <source>Enter seed offset passphrase, empty if none</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4030"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4050"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4086"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4107"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4127"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4142"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4191"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4216"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4232"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4271"/>
        <source>No data supplied, cancelled</source>
        <translation>Nessun dato fornito, cancellato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4036"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4113"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4222"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6369"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6944"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7203"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7776"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7844"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7908"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8112"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9385"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9629"/>
        <source>failed to parse address</source>
        <translation>impossibile fare il parsing dell&apos;indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4056"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4148"/>
        <source>failed to parse view key secret key</source>
        <translation>impossibile fare il parsing chiave di visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4065"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4165"/>
        <source>failed to verify view key secret key</source>
        <translation>impossibile verificare chiave di visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4069"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4169"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4251"/>
        <source>view key does not match standard address</source>
        <translation>la chiave di visualizzazione non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4074"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4095"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4173"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4307"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4334"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4366"/>
        <source>account creation failed</source>
        <translation>creazione dell&apos;account fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4091"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4133"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4276"/>
        <source>failed to parse spend key secret key</source>
        <translation>impossibile fare il parsing chiave di spesa chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4157"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4296"/>
        <source>failed to verify spend key secret key</source>
        <translation>impossibile verificare chiave di spesa chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4161"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4301"/>
        <source>spend key does not match standard address</source>
        <translation>la chiave di spesa non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4266"/>
        <source>Secret spend key (%u of %u)</source>
        <translation>Chiave di spesa segreta (%u di %u)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4341"/>
        <source>No restore height is specified.</source>
        <translation>Nessuna altezza di ripristino specificata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4342"/>
        <source>Assumed you are creating a new account, restore will be done from current estimated blockchain height.</source>
        <translation>Supponendo che si stia creando un nuovo account, il ripristino verrà effettuato dall&apos;attuale altezza stimata sulla blockchain.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4347"/>
        <source>account creation aborted</source>
        <translation>creazione account interrotta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4357"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation>specifica un percorso per il portafoglio con --generate-new-wallet (non --wallet-file)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4469"/>
        <source>can&apos;t specify --subaddress-lookahead and --wallet-file at the same time</source>
        <translation>non è possibile specificare --subaddress-lookahead e --wallet-file nello stesso momento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4473"/>
        <source>failed to open account</source>
        <translation>impossibile aprire account</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4478"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5208"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5261"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5401"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7812"/>
        <source>wallet is null</source>
        <translation>il portafoglio è nullo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4487"/>
        <source>RPC client secret key should be 32 byte in hex format</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4495"/>
        <source>Warning: using an untrusted daemon at %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4496"/>
        <source>Using a third party daemon can be detrimental to your security and privacy</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4499"/>
        <source>Using your own without SSL exposes your RPC traffic to monitoring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4500"/>
        <source>You are strongly encouraged to connect to the Monero network using your own daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4501"/>
        <source>If you or someone you trust are operating this daemon, you can use --trusted-daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4508"/>
        <source>Moreover, a daemon is also less secure when running in bootstrap mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4512"/>
        <source>Failed to initialize ring database: privacy enhancing features will be inactive</source>
        <translation>Impossibile inizializzare il database degli anelli: le funzionalità di miglioramento della privacy non saranno attive</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4614"/>
        <source>If your display freezes, exit blind with ^C, then run again with --use-english-language-names</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4632"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4637"/>
        <source>invalid language choice entered. Please try again.
</source>
        <translation>la lingua selezionata non è corretta. Prova di nuovo.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4718"/>
        <source>View key: </source>
        <translation>Chiave di visualizzazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4833"/>
        <source>Generated new wallet on hw device: </source>
        <translation>Generato un nuovo portafoglio sul dispositivo hw: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4914"/>
        <source>Key file not found. Failed to open wallet</source>
        <translation>File della chiave non trovato. Impossibile aprire il portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4991"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation>Potresti voler rimuovere il file &quot;%s&quot; e provare di nuovo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5020"/>
        <source>failed to deinitialize wallet</source>
        <translation>deinizializzazione portafoglio fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5073"/>
        <source>Watch only wallet saved as: </source>
        <translation>Portafoglio solo-lettura salvato come: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5077"/>
        <source>Failed to save watch only wallet: </source>
        <translation>Impossibile salvare il portafoglio in sola lettura: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5180"/>
        <source>With background mining enabled, the daemon will mine when idle and not on battery.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5199"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5937"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9707"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation>questo comando richiede un daemon fidato. Abilita questa opzione con --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5297"/>
        <source>Error checking daemon RPC access prices</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5303"/>
        <source>Error checking daemon RPC access prices: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5349"/>
        <source>Expected trusted or untrusted, got </source>
        <translation>Previsto fidato o non fidato, ottenuto </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5369"/>
        <source>Failed to connect to daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5373"/>
        <source>trusted</source>
        <translation>fidato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5373"/>
        <source>untrusted</source>
        <translation>non fidato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5381"/>
        <source>Daemon RPC credits/hash is less than was claimed. Either this daemon is cheating, or it changed its setup recently.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5382"/>
        <source>Claimed: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5383"/>
        <source>Actual: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5411"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation>impossibile salvare la blockchain: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5447"/>
        <source>NOTE: this transaction uses an encrypted payment ID: consider using subaddresses instead</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5451"/>
        <source>WARNING: this transaction uses an unencrypted payment ID: these are obsolete and ignored. Use subaddresses instead.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5497"/>
        <source>Password needed (%s) - use the refresh command</source>
        <translation>Password necessaria (%s) - usa il comando refresh</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5503"/>
        <source>Enter password</source>
        <translation>Inserisci la password</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5518"/>
        <source>Device requires attention</source>
        <translation>Il dispositivo richiede attenzione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5524"/>
        <source>Enter device PIN</source>
        <translation>Inserisci il PIN del dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5526"/>
        <source>Failed to read device PIN</source>
        <translation>Impossibile leggere il PIN del dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5533"/>
        <source>Please enter the device passphrase on the device</source>
        <translation>Inserisci la passphrase del dispositivo sul dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5538"/>
        <source>Enter device passphrase</source>
        <translation>Immettere la passphrase del dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5540"/>
        <source>Failed to read device passphrase</source>
        <translation>Impossibile leggere la passphrase del dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5556"/>
        <source>The first refresh has finished for the HW-based wallet with received money. hw_key_images_sync is needed. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5182"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5558"/>
        <source>Do you want to do it now? (Y/Yes/N/No): </source>
        <translation>Vuoi procedere adesso? (Y/Yes/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5560"/>
        <source>hw_key_images_sync skipped. Run command manually before a transfer.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="532"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5620"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5951"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>il daemon è impegnato. Prova più tardi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5624"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5955"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>nessuna connessione con il daemon. Assicurati che sia in funzione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5639"/>
        <source>refresh error: </source>
        <translation>errore refresh: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5690"/>
        <source> (Some owned outputs have missing key images - import_key_images needed)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5699"/>
        <source>Balance: </source>
        <translation>Bilancio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5768"/>
        <source>Invalid keyword: </source>
        <translation>Parola chiave non valida: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5806"/>
        <source>pubkey</source>
        <translation>pubkey</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5806"/>
        <source>key image</source>
        <translation>immagine chiave</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5807"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5823"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8505"/>
        <source>unlocked</source>
        <translation>sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5807"/>
        <source>ringct</source>
        <translation>ringct</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5822"/>
        <source>T</source>
        <translation>T</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5822"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5823"/>
        <source>locked</source>
        <translation>bloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5824"/>
        <source>RingCT</source>
        <translation>RingCT</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5824"/>
        <source>-</source>
        <translation>-</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5903"/>
        <source>payment ID has invalid format, expected 16 or 64 character hex string: </source>
        <translation>l&apos;id pagamento è in un formato invalido, dovrebbe essere una stringa hex di 16 o 64 caratteri</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5964"/>
        <source>failed to get spent status</source>
        <translation>impossibile recuperare status spesi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6048"/>
        <source>failed to find construction data for tx input</source>
        <translation>impossibile trovare i dati di costruzione per l&apos;input tx</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6053"/>
        <source>
Input %llu/%llu (%s): amount=%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6112"/>
        <source>the same transaction</source>
        <translation>la stessa transazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6112"/>
        <source>blocks that are temporally very close</source>
        <translation>i blocchi che sono temporalmente molto vicini</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6168"/>
        <source>I locked your Monero wallet to protect you while you were away</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6188"/>
        <source>Locked due to inactivity. The wallet password is required to unlock the console.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6308"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6884"/>
        <source>Locked blocks too high, max 1000000 (˜4 yrs)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6465"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6475"/>
        <source>Is this okay anyway?</source>
        <translation>Vuoi procedere comunque?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6470"/>
        <source>There is currently a %u block backlog at that fee level. Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6716"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7011"/>
        <source>Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6722"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7017"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7254"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6769"/>
        <source>Discarding %s of unmixable outputs that cannot be spent, which can be undone by &quot;rescan_spent&quot;.  Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7538"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu, %s. %sIs this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8744"/>
        <source>Rescan anyway?</source>
        <translation>Ripetere comunque la scansione?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8944"/>
        <source>locked due to inactivity</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10212"/>
        <source> (Y/Yes/N/No): </source>
        <translation> (Y/Yes/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10239"/>
        <source>Choose processing:</source>
        <translation>Scegli l&apos;elaborazione:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10248"/>
        <source>Sign tx</source>
        <translation>Firma tx</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10256"/>
        <source>Send the tx for submission to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10260"/>
        <source>Send the tx for signing to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10267"/>
        <source>Submit tx</source>
        <translation>Invia tx</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10270"/>
        <source>unknown</source>
        <translation>sconosciuto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10276"/>
        <source>Choice: </source>
        <translation>Scelta: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10288"/>
        <source>Wrong choice</source>
        <translation>Scelta errata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10295"/>
        <source>Id</source>
        <translation>Id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10295"/>
        <source>I/O</source>
        <translation>I/O</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10295"/>
        <source>Authorized Signer</source>
        <translation>Firmatario autorizzato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10296"/>
        <source>Message Type</source>
        <translation>Tipo di messaggio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10296"/>
        <source>Height</source>
        <translation>Altezza</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10296"/>
        <source>R</source>
        <translation>R</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10296"/>
        <source>Message State</source>
        <translation>Stato del messaggio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10296"/>
        <source>Since</source>
        <translation>Da</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10313"/>
        <source> ago</source>
        <translation> fa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10319"/>
        <source>#</source>
        <translation>#</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10319"/>
        <source>Transport Address</source>
        <translation>Indirizzo di trasporto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10320"/>
        <source>Auto-Config Token</source>
        <translation>Auto-Config Token</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10320"/>
        <source>Monero Address</source>
        <translation>Indirizzo Monero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10324"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10332"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10334"/>
        <source>&lt;not set&gt;</source>
        <translation>&lt;not set&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10375"/>
        <source>Message </source>
        <translation>Messaggio </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10376"/>
        <source>In/out: </source>
        <translation>In/out: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10378"/>
        <source>State: </source>
        <translation>Stato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10378"/>
        <source>%s since %s, %s ago</source>
        <translation>%s da %s, %s fa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10382"/>
        <source>Sent: Never</source>
        <translation>Inviati: mai</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10386"/>
        <source>Sent: %s, %s ago</source>
        <translation>inviato/i: %s, %s fa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10389"/>
        <source>Authorized signer: </source>
        <translation>Firmatario autorizzato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10390"/>
        <source>Content size: </source>
        <translation>Dimensione del contenuto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10390"/>
        <source> bytes</source>
        <translation> bytes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10391"/>
        <source>Content: </source>
        <translation>Contenuto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10391"/>
        <source>(binary data)</source>
        <translation>(dati binari)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10421"/>
        <source>Send these messages now?</source>
        <translation>Vuoi inviare questi messaggi adesso?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10431"/>
        <source>Queued for sending.</source>
        <translation>In coda per l&apos;invio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10451"/>
        <source>Invalid message id</source>
        <translation>ID messaggio non valido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10460"/>
        <source>usage: mms init &lt;required_signers&gt;/&lt;authorized_signers&gt; &lt;own_label&gt; &lt;own_transport_address&gt;</source>
        <translation>utilizzo: mms init &lt;required_signers&gt;/&lt;authorized_signers&gt; &lt;own_label&gt; &lt;own_transport_address&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10466"/>
        <source>The MMS is already initialized. Re-initialize by deleting all signer info and messages?</source>
        <translation>L&apos;MMS è già inizializzato. Reinizializzare eliminando tutte le informazioni e i messaggi del firmatario?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10481"/>
        <source>Error in the number of required signers and/or authorized signers</source>
        <translation>Errore nel numero di firmatari richiesti e/o firmatari autorizzati</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10498"/>
        <source>The MMS is not active.</source>
        <translation>L&apos;MMS non è attivo.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10521"/>
        <source>Invalid signer number </source>
        <translation>Numero del firmatario non valido </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10526"/>
        <source>mms signer [&lt;number&gt; &lt;label&gt; [&lt;transport_address&gt; [&lt;monero_address&gt;]]]</source>
        <translation>mms signer [&lt;number&gt; &lt;label&gt; [&lt;transport_address&gt; [&lt;monero_address&gt;]]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10545"/>
        <source>Invalid Monero address</source>
        <translation>Indirizzo Monero non valido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10552"/>
        <source>Wallet state does not allow changing Monero addresses anymore</source>
        <translation>Lo stato del portafoglio non consente più di modificare gli indirizzi Monero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10564"/>
        <source>Usage: mms list</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10577"/>
        <source>Usage: mms next [sync]</source>
        <translation>Utilizzo: mms next [sync]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10602"/>
        <source>No next step: </source>
        <translation>Nessun passo successivo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10612"/>
        <source>prepare_multisig</source>
        <translation>prepare_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10618"/>
        <source>make_multisig</source>
        <translation>make_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10633"/>
        <source>exchange_multisig_keys</source>
        <translation>exchange_multisig_keys</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10648"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10768"/>
        <source>export_multisig_info</source>
        <translation>export_multisig_info</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10657"/>
        <source>import_multisig_info</source>
        <translation>import_multisig_info</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10670"/>
        <source>sign_multisig</source>
        <translation>sign_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10680"/>
        <source>submit_multisig</source>
        <translation>submit_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10690"/>
        <source>Send tx</source>
        <translation>Invia tx</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10701"/>
        <source>Process signer config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10713"/>
        <source>Replace current signer config with the one displayed above?</source>
        <translation>Sostituire la configurazione attuale del firmatario con quella visualizzata sopra?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10727"/>
        <source>Process auto config data</source>
        <translation>Elabora dati di configurazione automatica</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10741"/>
        <source>Nothing ready to process</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10761"/>
        <source>Usage: mms sync</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10785"/>
        <source>Usage: mms delete (&lt;message_id&gt; | all)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10792"/>
        <source>Delete all messages?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10818"/>
        <source>Usage: mms send [&lt;message_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10835"/>
        <source>Usage: mms receive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10852"/>
        <source>Usage: mms export &lt;message_id&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10864"/>
        <source>Message content saved to: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10868"/>
        <source>Failed to to save message content</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10892"/>
        <source>Usage: mms note [&lt;label&gt; &lt;text&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10899"/>
        <source>No signer found with label </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10921"/>
        <source>Usage: mms show &lt;message_id&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10940"/>
        <source>Usage: mms set &lt;option_name&gt; [&lt;option_value&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10957"/>
        <source>Wrong option value</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10962"/>
        <source>Auto-send is on</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10962"/>
        <source>Auto-send is off</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10967"/>
        <source>Unknown option</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10975"/>
        <source>Usage: mms help [&lt;subcommand&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10991"/>
        <source>Usage: mms send_signer_config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10997"/>
        <source>Signer config not yet complete</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="11012"/>
        <source>Usage: mms start_auto_config [&lt;label&gt; &lt;label&gt; ...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="11017"/>
        <source>There are signers without a label set. Complete labels before auto-config or specify them as parameters here.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="11023"/>
        <source>Auto-config is already running. Cancel and restart?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="11047"/>
        <source>Usage: mms stop_auto_config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="11050"/>
        <source>Delete any auto-config tokens and stop auto-config?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="11063"/>
        <source>Usage: mms auto_config &lt;auto_config_token&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="11070"/>
        <source>Invalid auto-config token</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="11076"/>
        <source>Auto-config already running. Cancel and restart?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="11094"/>
        <source>MMS not available in this wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="11118"/>
        <source>The MMS is not active. Activate using the &quot;mms init&quot; command</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="11195"/>
        <source>Invalid MMS subcommand</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="11200"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="11204"/>
        <source>Error in MMS command: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6475"/>
        <source>Failed to check for backlog: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6524"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6999"/>
        <source>
Transaction </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6529"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7004"/>
        <source>Spending from address index %d
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6531"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7006"/>
        <source>WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7173"/>
        <source>failed to parse Payment ID</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2169"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2216"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7196"/>
        <source>failed to parse key image</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7234"/>
        <source>No outputs found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7239"/>
        <source>Multiple transactions are created, which is not supposed to happen</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7244"/>
        <source>The transaction uses multiple or no inputs, which is not supposed to happen</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7322"/>
        <source>missing threshold amount</source>
        <translation>manca la soglia massima dell&apos;ammontare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7327"/>
        <source>invalid amount threshold</source>
        <translation>ammontare soglia invalido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7491"/>
        <source>Change goes to more than one address</source>
        <translation>Il cambiamento va a più di un indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7927"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8037"/>
        <source>Good signature</source>
        <translation>Firma valida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7954"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8039"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8139"/>
        <source>Bad signature</source>
        <translation>Firma invalida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9361"/>
        <source>Standard address: </source>
        <translation>Indirizzo standard: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9366"/>
        <source>failed to parse payment ID or address</source>
        <translation>impossibile fare il parsing di ID pagamento o indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9403"/>
        <source>failed to parse index</source>
        <translation>impossibile fare il parsing dell&apos;indice</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9411"/>
        <source>Address book is empty.</source>
        <translation>La rubrica è vuota.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9417"/>
        <source>Index: </source>
        <translation>Indice: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9423"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9553"/>
        <source>Address: </source>
        <translation>Indirizzo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9424"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9552"/>
        <source>Description: </source>
        <translation>Descrizione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9582"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation>il portafoglio è di tipo solo-visualizzazione e non può firmare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1333"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9596"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9622"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9873"/>
        <source>failed to read file </source>
        <translation>impossibile leggere il file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4343"/>
        <source>Use --restore-height or --restore-date if you want to restore an already setup account from a specific height.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4345"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4433"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6575"/>
        <source>Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4456"/>
        <source>Still apply restore height?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7916"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8030"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8124"/>
        <source>failed to load signature file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7978"/>
        <source>wallet is watch-only and cannot generate the proof</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8062"/>
        <source>The reserve proof can be generated only by a full wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8117"/>
        <source>Address must not be a subaddress</source>
        <translation>L&apos;indirizzo non può essere un sottoindirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8135"/>
        <source>Good signature -- total: %s, spent: %s, unspent: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8350"/>
        <source>[Double spend seen on the network: this transaction may or may not end up being mined] </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8628"/>
        <source>There is no unspent output in the specified address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8901"/>
        <source>Daemon requests payment at diff %llu, with %f credits/hash%s. Run start_mining_for_rpc to start mining to pay for RPC access, or use another daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8928"/>
        <source>Error mining to daemon: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8934"/>
        <source>Failed to start mining for RPC payment</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8948"/>
        <source> (no daemon)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8950"/>
        <source> (out of sync)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9001"/>
        <source>(Untitled account)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9014"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9032"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9057"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9080"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9228"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9251"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9279"/>
        <source>failed to parse index: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9019"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9233"/>
        <source>specify an index between 0 and </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9137"/>
        <source>
Grand total:
  Balance: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9137"/>
        <source>, unlocked balance: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9145"/>
        <source>Untagged accounts:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9151"/>
        <source>Tag %s is unregistered.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9154"/>
        <source>Accounts with tag: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9155"/>
        <source>Tag&apos;s description: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9157"/>
        <source>Account</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9163"/>
        <source> %c%8u %6s %21s %21s %21s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9173"/>
        <source>----------------------------------------------------------------------------------</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9174"/>
        <source>%15s %21s %21s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9198"/>
        <source>Primary address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9198"/>
        <source>(used)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9219"/>
        <source>(Untitled address)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9260"/>
        <source>&lt;index_min&gt; is already out of bound</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9265"/>
        <source>&lt;index_max&gt; exceeds the bound</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9329"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9342"/>
        <source>Integrated addresses can only be created for account 0</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9355"/>
        <source>Integrated address: %s, payment ID: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9361"/>
        <source>Subaddress: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9509"/>
        <source>no description found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9511"/>
        <source>description found: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9551"/>
        <source>Filename: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9556"/>
        <source>Watch only</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9558"/>
        <source>%u/%u multisig%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9560"/>
        <source>Normal</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9561"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10377"/>
        <source>Type: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9587"/>
        <source>This wallet is multisig and cannot sign</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9636"/>
        <source>Bad signature from </source>
        <translation>Firma non valida da </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9640"/>
        <source>Good signature from </source>
        <translation>Firma valida da </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9656"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation>il portafoglio è solo-vista e non può esportare immagini chiave</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1272"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9683"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9840"/>
        <source>failed to save file </source>
        <translation>impossibile salvare file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9694"/>
        <source>Signed key images exported to </source>
        <translation>Chiave immagine firmata esportata in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9851"/>
        <source>Outputs exported to </source>
        <translation>Outputs esportati in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6351"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7363"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8073"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8587"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8595"/>
        <source>amount is wrong: </source>
        <translation>l&apos;ammontare non è corretto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6352"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7363"/>
        <source>expected number from 0 to </source>
        <translation>deve essere un numero da 0 a </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6714"/>
        <source>Sweeping </source>
        <translation>Eseguendo lo sweeping </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7294"/>
        <source>Money successfully sent, transaction: </source>
        <translation>Fondi inviati con successo, transazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7532"/>
        <source>%s change to %s</source>
        <translation>%s cambia in %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7535"/>
        <source>no change</source>
        <translation>nessun cambiamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1479"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1492"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7607"/>
        <source>Transaction successfully signed to file </source>
        <translation>Transazione firmata con successo nel file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7674"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7712"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7769"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7818"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7900"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7985"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8020"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9441"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9469"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9903"/>
        <source>failed to parse txid</source>
        <translation>parsing txid fallito</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7690"/>
        <source>Tx key: </source>
        <translation>Chiave Tx: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7695"/>
        <source>no tx keys found for this txid</source>
        <translation>nessuna chiave tx trovata per questo txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7787"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7999"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8088"/>
        <source>signature file saved to: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7789"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8001"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8090"/>
        <source>failed to save signature file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7826"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7835"/>
        <source>failed to parse tx key</source>
        <translation>impossibile fare il parsing della chiave tx</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7793"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7881"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7959"/>
        <source>error: </source>
        <translation>errore: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7857"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7930"/>
        <source>received</source>
        <translation>ricevuto/i</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7857"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7930"/>
        <source>in txid</source>
        <translation>in txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7876"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7949"/>
        <source>received nothing in txid</source>
        <translation>nulla ricevuto in txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7860"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7933"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation>AVVISO: questa transazione non è ancora inclusa nella blockchain!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7866"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7939"/>
        <source>This transaction has %u confirmations</source>
        <translation>Questa transazione ha %u conferme</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7870"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7943"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation>AVVISO: impossibile determinare il numero di conferme!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8224"/>
        <source>bad min_height parameter:</source>
        <translation>parametro min_height non corretto:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8236"/>
        <source>bad max_height parameter:</source>
        <translation>parametro max_height non corretto:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8256"/>
        <source>in</source>
        <translation>in</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8602"/>
        <source>&lt;min_amount&gt; should be smaller than &lt;max_amount&gt;</source>
        <translation>&lt;min_amount&gt; dovrebbe essere più piccolo di &lt;max_amount&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8634"/>
        <source>
Amount: </source>
        <translation>
Ammontare: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8634"/>
        <source>, number of keys: </source>
        <translation>, numero di chiavi: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8639"/>
        <source> </source>
        <translation> </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8644"/>
        <source>
Min block height: </source>
        <translation>
Altezza minima blocco: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8645"/>
        <source>
Max block height: </source>
        <translation>
Altezza massima blocco: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8646"/>
        <source>
Min amount found: </source>
        <translation>
Ammontare minimo trovato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8647"/>
        <source>
Max amount found: </source>
        <translation>
Ammontare massimo trovato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8648"/>
        <source>
Total count: </source>
        <translation>
Conto totale: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8688"/>
        <source>
Bin size: </source>
        <translation>
Dimensione Bin: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8689"/>
        <source>
Outputs per *: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8691"/>
        <source>count
  ^
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8693"/>
        <source>  |</source>
        <translation>  |</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8695"/>
        <source>  +</source>
        <translation>  +</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8695"/>
        <source>+--&gt; block height
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8696"/>
        <source>   ^</source>
        <translation>   ^</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8696"/>
        <source>^
</source>
        <translation>^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8697"/>
        <source>  </source>
        <translation>  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8946"/>
        <source>wallet</source>
        <translation>portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9333"/>
        <source>Random payment ID: </source>
        <translation>ID pagamento casuale: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9334"/>
        <source>Matching integrated address: </source>
        <translation>Indirizzo integrato corrispondente: </translation>
    </message>
</context>
<context>
    <name>gencert</name>
    <message>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="61"/>
        <source>Filename to save the certificate</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="62"/>
        <source>Filename to save the private key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="63"/>
        <source>Passphrase with which to encrypt the private key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="64"/>
        <source>File containing the passphrase with which to encrypt the private key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="65"/>
        <source>Prompt for a passphrase with which to encrypt the private key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="137"/>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="143"/>
        <source>Argument is needed: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="153"/>
        <source>Failed to read passphrase</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="188"/>
        <source>Failed to create certificate</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="198"/>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="206"/>
        <source>Failed to write certificate: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="218"/>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="226"/>
        <source>Failed to write private key: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="237"/>
        <source>Failed to save certificate file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_ssl_cert/gen_ssl_cert.cpp" line="243"/>
        <source>Failed to save private key file</source>
        <translation type="unfinished"></translation>
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
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="83"/>
        <source>Generating %u %u/%u multisig wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="142"/>
        <source>Error verifying multisig extra info</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="157"/>
        <source>Error creating multisig wallets: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="182"/>
        <source>This program generates a set of multisig wallets - use this simpler scheme only if all the participants trust each other</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="201"/>
        <source>Error: Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="208"/>
        <source>Error: expected N/M, but got: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="216"/>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="225"/>
        <source>Error: either --scheme or both of --threshold and --participants may be given</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="232"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got N==%u and M==%d</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="241"/>
        <source>Error: --filename-base is required</source>
        <translation type="unfinished"></translation>
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
        <location filename="../src/wallet/message_store.cpp" line="824"/>
        <source>Auto-config cannot proceed because auto config data from other signers is not complete</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="849"/>
        <source>The signer config is not complete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="901"/>
        <source>Wallet can&apos;t go multisig because key sets from other signers are missing or not complete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="943"/>
        <source>Wallet can&apos;t start another key exchange round because key sets from other signers are missing or not complete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1007"/>
        <source>Syncing not done because multisig sync data from other signers are missing or not complete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1121"/>
        <source>There are waiting messages, but nothing is ready to process under normal circumstances</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1124"/>
        <source>
Use &quot;mms next sync&quot; if you want to force processing of the waiting sync data</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1128"/>
        <source>
Use &quot;mms note&quot; to display the waiting notes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1133"/>
        <source>There are no messages waiting to be processed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1351"/>
        <source>key set</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1353"/>
        <source>additional key set</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1355"/>
        <source>multisig sync data</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1357"/>
        <source>partially signed tx</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1359"/>
        <source>fully signed tx</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1361"/>
        <source>note</source>
        <translation>nota</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1363"/>
        <source>signer config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1365"/>
        <source>auto-config data</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1367"/>
        <source>unknown message type</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1376"/>
        <source>in</source>
        <translation>in</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1378"/>
        <source>out</source>
        <translation>out</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1380"/>
        <source>unknown message direction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1389"/>
        <source>ready to send</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1391"/>
        <source>sent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1393"/>
        <source>waiting</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1395"/>
        <source>processed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1397"/>
        <source>cancelled</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1399"/>
        <source>unknown message state</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>sw</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="152"/>
        <source>Generate new wallet and save it to &lt;arg&gt;</source>
        <translation>Genera un nuovo portafoglio e salvalo in &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="153"/>
        <source>Generate new wallet from device and save it to &lt;arg&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="154"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>Genera un portafoglio solo-ricezione da chiave di visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="155"/>
        <source>Generate deterministic wallet from spend key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="156"/>
        <source>Generate wallet from private keys</source>
        <translation>Genera portafoglio da chiavi private</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="157"/>
        <source>Generate a master wallet from multisig wallet keys</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="159"/>
        <source>Language for mnemonic</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="160"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>Specifica il seed stile Electrum per recuperare/creare il portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="161"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Recupera portafoglio usando il seed mnemonico stile-Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="162"/>
        <source>alias for --restore-deterministic-wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="163"/>
        <source>Recover multisig wallet using Electrum-style mnemonic seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="164"/>
        <source>Generate non-deterministic view and spend keys</source>
        <translation>Crea chiavi di visualizzione e chiavi di spesa non-deterministiche</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="167"/>
        <source>Restore from estimated blockchain height on specified date</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="398"/>
        <source>invalid argument: must be either 0/1, true/false, y/n, yes/no</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="454"/>
        <source>DNSSEC validation passed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="458"/>
        <source>WARNING: DNSSEC validation was unsuccessful, this address may not be correct!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="461"/>
        <source>For URL: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="463"/>
        <source> Monero Address = </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="475"/>
        <source>you have cancelled the transfer request</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="496"/>
        <source>failed to parse index: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="509"/>
        <source>invalid format for subaddress lookahead; must be &lt;major&gt;:&lt;minor&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="528"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="537"/>
        <source>RPC error: </source>
        <translation>errore RPC: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="541"/>
        <source>failed to get random outputs to mix: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="548"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="556"/>
        <source>Not enough money in unlocked balance</source>
        <translation>Fondi insufficienti in saldo sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="566"/>
        <source>Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="572"/>
        <source>not enough outputs for specified ring size</source>
        <translation>insufficiente numero di output per il ring size specificato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="575"/>
        <source>output amount</source>
        <translation>ammontare output</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="575"/>
        <source>found outputs to use</source>
        <translation>trovati output che possono essere usati</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="577"/>
        <source>Please use sweep_unmixable.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="581"/>
        <source>transaction was not constructed</source>
        <translation>transazione non costruita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="589"/>
        <source>Reason: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="598"/>
        <source>one of destinations is zero</source>
        <translation>una delle destinazioni è zero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="603"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>impossibile trovare un modo per dividere le transazioni</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="609"/>
        <source>unknown transfer error: </source>
        <translation>errore trasferimento sconosciuto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="614"/>
        <source>Multisig error: </source>
        <translation>Errore multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="620"/>
        <source>internal error: </source>
        <translation>errore interno: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="625"/>
        <source>unexpected error: </source>
        <translation>errore inaspettato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="629"/>
        <source>There was an error, which could mean the node may be trying to get you to retry creating a transaction, and zero in on which outputs you own. Or it could be a bona fide error. It may be prudent to disconnect from this node, and not try to send a transaction immediately. Alternatively, connect to another node so the original node cannot correlate information.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="641"/>
        <source>File %s likely stores wallet private keys! Use a different file name.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8153"/>
        <source> seconds</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8155"/>
        <source> minutes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8157"/>
        <source> hours</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8159"/>
        <source> days</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8161"/>
        <source> months</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8162"/>
        <source>a long time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10137"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.
WARNING: Do not reuse your Monero keys on another fork, UNLESS this fork has key reuse mitigations built in. Doing so will harm your privacy.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10162"/>
        <source>Unknown command: </source>
        <translation>Comando sconosciuto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="165"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>Permetti comunicazioni con un daemon che usa una versione RPC differente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="166"/>
        <source>Restore from specific blockchain height</source>
        <translation>Ripristina da specifico blocco</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="168"/>
        <source>The newly created transaction will not be relayed to the monero network</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="169"/>
        <source>Create an address file for new wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="171"/>
        <source>Display English language names</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="313"/>
        <source>failed to read wallet password</source>
        <translation>impossibile leggere la password del portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="320"/>
        <source>Enter a new password for the wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="320"/>
        <source>Wallet password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="330"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>il daemon è occupato. Prova più tardi.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="339"/>
        <source>possibly lost connection to daemon</source>
        <translation>possibile perdita di connessione con il daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="356"/>
        <source>Error: </source>
        <translation>Errore: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="465"/>
        <source>Is this OK?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="586"/>
        <source>transaction %s was rejected by daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="644"/>
        <source>File %s already exists. Are you sure to overwrite it?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10156"/>
        <source>Failed to initialize wallet</source>
        <translation>Inizializzazione wallet fallita</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="253"/>
        <source>Use daemon instance at &lt;host&gt;:&lt;port&gt;</source>
        <translation>Usa instanza daemon in &lt;host&gt;:&lt;port&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="254"/>
        <source>Use daemon instance at host &lt;arg&gt; instead of localhost</source>
        <translation>Usa istanza daemon all&apos;host &lt;arg&gt; invece che localhost</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="259"/>
        <source>Wallet password file</source>
        <translation>File password portafoglio</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="260"/>
        <source>Use daemon instance at port &lt;arg&gt; instead of 18081</source>
        <translation>Usa istanza daemon alla porta &lt;arg&gt; invece che alla 18081</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="269"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>Per testnet. Il daemon può anche essere lanciato con la flag --testnet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="386"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>non puoi specificare la porta o l&apos;host del daemon più di una volta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="522"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>non puoi specificare più di un --password e --password-file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="535"/>
        <source>the password file specified could not be read</source>
        <translation>il file password specificato non può essere letto</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="561"/>
        <source>Failed to load file </source>
        <translation>Impossibile caricare file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="258"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation>Wallet password (escape/quote se necessario)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="255"/>
        <source>[&lt;ip&gt;:]&lt;port&gt; socks proxy to use for daemon connections</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="256"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation>Abilita comandi dipendenti da un daemon fidato</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="257"/>
        <source>Disable commands which rely on a trusted daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="261"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>Specificare username[:password] per client del daemon RPC</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="262"/>
        <source>Enable SSL on daemon RPC connections: enabled|disabled|autodetect</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="266"/>
        <source>List of valid fingerprints of allowed RPC servers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="267"/>
        <source>Allow any SSL certificate from the daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="268"/>
        <source>Allow user (via --daemon-ssl-ca-certificates) chain certificates</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="270"/>
        <source>For stagenet. Daemon must also be launched with --stagenet flag</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="272"/>
        <source>Set shared ring database path</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="283"/>
        <source>Number of rounds for the key derivation function</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="284"/>
        <source>HW device to use</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="285"/>
        <source>HW device wallet derivation path (e.g., SLIP-10)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="287"/>
        <source>Do not use DNS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="288"/>
        <source>Do not connect to a daemon, nor use DNS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="289"/>
        <source>File containing extra entropy to initialize the PRNG (any data, aim for 256 bits of entropy to be useful, wihch typically means more than 256 bits of data)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="378"/>
        <source>Invalid argument for </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="430"/>
        <source>Enabling --</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="430"/>
        <source> requires --</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="431"/>
        <source> or --</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="431"/>
        <source> or use of a .onion/.i2p domain</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="465"/>
        <source>--trusted-daemon and --untrusted-daemon are both seen, assuming untrusted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="475"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Il daemon è locale, viene considerato fidato</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="542"/>
        <source>no password specified; use --prompt-for-password to prompt for a password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="544"/>
        <source>Enter a new password for the wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="544"/>
        <source>Wallet password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="567"/>
        <source>Failed to parse JSON</source>
        <translation>Impossibile fare il parsing di JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="574"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation>La versione %u è troppo recente, possiamo comprendere solo fino alla versione %u</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="590"/>
        <source>failed to parse view key secret key</source>
        <translation>impossibile fare il parsing di chiave di visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="595"/>
        <location filename="../src/wallet/wallet2.cpp" line="663"/>
        <location filename="../src/wallet/wallet2.cpp" line="708"/>
        <source>failed to verify view key secret key</source>
        <translation>impossibile verificare chiave di visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="606"/>
        <source>failed to parse spend key secret key</source>
        <translation>impossibile fare il parsing chiave di spesa chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="611"/>
        <location filename="../src/wallet/wallet2.cpp" line="673"/>
        <location filename="../src/wallet/wallet2.cpp" line="734"/>
        <source>failed to verify spend key secret key</source>
        <translation>impossibile verificare chiave di spesa chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="623"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Verifica lista di parole stile-Electrum fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="643"/>
        <source>At least one of either an Electrum-style word list, private view key, or private spend key must be specified</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="647"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Specificate entrambe lista parole stile-Electrum e chiave/i privata/e </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="657"/>
        <source>invalid address</source>
        <translation>indirizzo invalido</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="666"/>
        <source>view key does not match standard address</source>
        <translation>la chiave di visualizzazione non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="676"/>
        <source>spend key does not match standard address</source>
        <translation>la chiave di spesa non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="684"/>
        <source>Cannot generate deprecated wallets from JSON</source>
        <translation>Impossibile creare portafogli disapprovati da JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="720"/>
        <source>failed to parse address: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="726"/>
        <source>Address must be specified in order to create watch-only wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="743"/>
        <source>failed to generate new wallet: </source>
        <translation>impossibile generare nuovo portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="1762"/>
        <source>Password is needed to compute key image for incoming monero</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="1763"/>
        <source>Invalid password: password is needed to compute key image for incoming monero</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="4378"/>
        <location filename="../src/wallet/wallet2.cpp" line="4968"/>
        <location filename="../src/wallet/wallet2.cpp" line="5572"/>
        <source>Primary account</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="11253"/>
        <source>No funds received in this tx.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="12034"/>
        <source>failed to read file </source>
        <translation>lettura file fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="170"/>
        <source>Set subaddress lookahead sizes to &lt;major&gt;:&lt;minor&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="263"/>
        <source>Path to a PEM format private key</source>
        <translation type="unfinished">Percorso della chiave privata in formato PEM</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="264"/>
        <source>Path to a PEM format certificate</source>
        <translation type="unfinished">Percorso del certificato in formato PEM</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="265"/>
        <source>Path to file containing concatenated PEM format certificate(s) to replace system CA(s).</source>
        <translation type="unfinished">Percorso del file contenente i certificati concatenati in formato PEM per sostituire le CA di sistema.</translation>
    </message>
</context>
<context>
    <name>tools::wallet_rpc_server</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="188"/>
        <source>Failed to create directory </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="190"/>
        <source>Failed to create directory %s: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="201"/>
        <source>Cannot specify --</source>
        <translation>Impossibile specificare --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="201"/>
        <source> and --</source>
        <translation> e --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="220"/>
        <source>Failed to create file </source>
        <translation>Impossibile creare file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="220"/>
        <source>. Check permissions or remove file</source>
        <translation>. Controlla permessi o rimuovi il file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="230"/>
        <source>Error writing to file </source>
        <translation>Errore durante scrittura su file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="233"/>
        <source>RPC username/password is stored in file </source>
        <translation>Username/password RPC conservato nel file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="612"/>
        <source>Tag %s is unregistered.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3291"/>
        <source>Transaction not possible. Available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4492"/>
        <source>This is the RPC monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4320"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>Non puoi specificare più di un --wallet-file e --generate-from-json</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4304"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation>Non è possibile specificare più di un --testnet e --stagenet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4332"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>Devi specificare --wallet-file o --generate-from-json o --wallet-dir</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4336"/>
        <source>Loading wallet...</source>
        <translation>Sto caricando il portafoglio...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4381"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4413"/>
        <source>Saving wallet...</source>
        <translation>Sto salvando il portafoglio...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4383"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4415"/>
        <source>Successfully saved</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4386"/>
        <source>Successfully loaded</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4390"/>
        <source>Wallet initialization failed: </source>
        <translation>Inizializzazione portafoglio fallita: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4396"/>
        <source>Failed to initialize wallet RPC server</source>
        <translation>Inizializzazione server RPC portafoglio fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4400"/>
        <source>Starting wallet RPC server</source>
        <translation>Server RPC portafoglio in avvio</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4407"/>
        <source>Failed to run wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4410"/>
        <source>Stopped wallet RPC server</source>
        <translation>Server RPC portafoglio arrestato</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4419"/>
        <source>Failed to save wallet: </source>
        <translation>Impossibile salvare portafoglio: </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="168"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4472"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10102"/>
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
        <location filename="../src/wallet/wallet_args.cpp" line="81"/>
        <source>Set RPC client secret key for RPC payments</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="109"/>
        <source>Max number of threads to use for a parallel job</source>
        <translation>Numero massimo di threads da utilizzare per un lavoro parallelo</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="110"/>
        <source>Specify log file</source>
        <translation>Specificare file di log</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="111"/>
        <source>Config file</source>
        <translation>File configurazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="123"/>
        <source>General options</source>
        <translation>Opzioni generali</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="148"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="173"/>
        <source>Can&apos;t find config file </source>
        <translation>Impossibile trovare file configurazione </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="217"/>
        <source>Logging to: </source>
        <translation>Sto salvando il Log in: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="219"/>
        <source>Logging to %s</source>
        <translation>Sto salvando il Log in %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="223"/>
        <source>WARNING: You may not have a high enough lockable memory limit</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="225"/>
        <source>see ulimit -l</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="150"/>
        <source>Usage:</source>
        <translation>Uso:</translation>
    </message>
</context>
</TS>
