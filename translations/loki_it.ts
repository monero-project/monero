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
        <translation>ID pagamento non valido. L&apos;ID pagamento corto dovrebbe essere usato solo in un indirizzo integrato</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="70"/>
        <source>Invalid payment ID</source>
        <translation>ID pagamento non valido</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="77"/>
        <source>Integrated address and long payment ID can&apos;t be used at the same time</source>
        <translation>L&apos;indirizzo integrato e l&apos;ID pagamento lungo non possono essere utilizzati contemporaneamente</translation>
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
        <location filename="../src/wallet/api/wallet.cpp" line="1459"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>L&apos;id pagamento è in un formato invalido, dovrebbe essere una stringa esadecimale di 16 o 64 caratteri: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1468"/>
        <source>Failed to add short payment id: </source>
        <translation>Impossibile aggiungere id pagamento corto</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1510"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1592"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>il daemon è impegnato. Riprova più tardi.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1512"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1594"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>nessuna connessione con il daemon. Accertati che sia operativo.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1514"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1596"/>
        <source>RPC error: </source>
        <translation>errore RPC: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1542"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1627"/>
        <source>not enough outputs for specified ring size</source>
        <translation>insufficiente numero di output per il ring size specificato</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1544"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1629"/>
        <source>found outputs to use</source>
        <translation>trovati output che possono essere usati</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1546"/>
        <source>Please sweep unmixable outputs.</source>
        <translation>Pulisci gli output non mixabili.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1520"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1603"/>
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
        <location filename="../src/wallet/api/wallet.cpp" line="1144"/>
        <source>Failed to load transaction from file</source>
        <translation>Impossibile caricare la transazione da file</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1160"/>
        <source>Wallet is view only</source>
        <translation>Il portafoglio è di tipo solo visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1168"/>
        <source>failed to save file </source>
        <translation>impossibile salvare il file </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1184"/>
        <source>Key images can only be imported with a trusted daemon</source>
        <translation>Le key image possono essere importate solo con un daemon fidato</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1197"/>
        <source>Failed to import key images: </source>
        <translation>Impossibile importare le key images: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1229"/>
        <source>Failed to get subaddress label: </source>
        <translation>Impossibile recuperare l&apos;etichetta del sottoindirizzo: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1242"/>
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
        <location filename="../src/wallet/api/wallet.cpp" line="1259"/>
        <source>Failed to get multisig info: </source>
        <translation>Impossibile ottenere informazioni multifirma: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1276"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1290"/>
        <source>Failed to make multisig: </source>
        <translation>Impossibile eseguire multifirma: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1305"/>
        <source>Failed to finalize multisig wallet creation</source>
        <translation>Impossibile finalizzare la creazione di un portafoglio multifirma</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1308"/>
        <source>Failed to finalize multisig wallet creation: </source>
        <translation>Impossibile finalizzare la creazione di un portafoglio multifirma: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1324"/>
        <source>Failed to export multisig images: </source>
        <translation>Impossibile esportare immagini multifirma: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1342"/>
        <source>Failed to parse imported multisig images</source>
        <translation>Impossibile analizzare le immagini multifirma importate</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1352"/>
        <source>Failed to import multisig images: </source>
        <translation>Impossibile importare immagini multifirma: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1366"/>
        <source>Failed to check for partial multisig key images: </source>
        <translation>Impossibile controllare le immagini della chiave multifirma parziali: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1394"/>
        <source>Failed to restore multisig transaction: </source>
        <translation>Impossibile ripristinare la transazione multifirma: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1434"/>
        <source>Invalid destination address</source>
        <translation>Indirizzo destinatario non valido</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1516"/>
        <source>failed to get outputs to mix: %s</source>
        <translation>impossibile ottenere gli output da mixare: %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1527"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1611"/>
        <source>not enough money to transfer, overall balance only %s, sent amount %s</source>
        <translation>fondi non sufficienti per il trasferimento, saldo totale %s, importo inviato %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1534"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1619"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>fondi non sufficienti per il trasferimento, disponibili solo %s, ammontare transazione %s = %s + %s (commissione)</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1544"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1629"/>
        <source>output amount</source>
        <translation>ammontare output</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1549"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1633"/>
        <source>transaction was not constructed</source>
        <translation>transazione non costruita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1552"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1636"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transazione %s è stata rifiutata dal daemon con status: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1557"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1641"/>
        <source>one of destinations is zero</source>
        <translation>una delle destinazioni è zero</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1559"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1643"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>impossibile trovare un modo per dividere le transazioni</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1561"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1645"/>
        <source>unknown transfer error: </source>
        <translation>errore trasferimento sconosciuto: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1563"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1647"/>
        <source>internal error: </source>
        <translation>errore interno: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1565"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1649"/>
        <source>unexpected error: </source>
        <translation>errore inaspettato: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1567"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1651"/>
        <source>unknown error</source>
        <translation>errore sconosciuto</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1598"/>
        <source>failed to get outputs to mix</source>
        <translation>impossibile ottenere gli output da mixare</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1726"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1753"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1801"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1829"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1857"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1878"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2354"/>
        <source>Failed to parse txid</source>
        <translation>Impossibile effettuare parsing del txid</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1743"/>
        <source>no tx keys found for this txid</source>
        <translation>nessuna chiave tx trovata per questo txid</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1761"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1770"/>
        <source>Failed to parse tx key</source>
        <translation>Impossibile effettuare parsing della chiave tx</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1779"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1808"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1836"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1917"/>
        <source>Failed to parse address</source>
        <translation>Impossibile effettuare parsing dell&apos;indirizzo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1922"/>
        <source>Address must not be a subaddress</source>
        <translation>L&apos;indirizzo non può essere un sottoindirizzo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1962"/>
        <source>The wallet must be in multisig ready state</source>
        <translation>Il portafoglio deve essere in stato multifirma</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1984"/>
        <source>Given string is not a key</source>
        <translation>La stringa inserita non è una chiave</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2226"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>&quot;Riscannerizza spesi&quot; può essere utilizzato solo da un daemon fidato</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2275"/>
        <source>Invalid output: </source>
        <translation>Output non valido: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2282"/>
        <source>Failed to mark outputs as spent</source>
        <translation>Impossibile contrassegnare gli outputs come spesi</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2293"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2315"/>
        <source>Failed to parse output amount</source>
        <translation>Impossibile analizzare la quantità di output</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2298"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2320"/>
        <source>Failed to parse output offset</source>
        <translation>Impossibile analizzare l&apos;offset di output</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2304"/>
        <source>Failed to mark output as spent</source>
        <translation>Impossibile contrassegnare l&apos;output come speso</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2326"/>
        <source>Failed to mark output as unspent</source>
        <translation>Impossibile contrassegnare l&apos;output come non speso</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2337"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2376"/>
        <source>Failed to parse key image</source>
        <translation>Impossibile analizzare l&apos;immagine della chiave</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2343"/>
        <source>Failed to get ring</source>
        <translation>Impossibile ottenere l&apos;anello</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2361"/>
        <source>Failed to get rings</source>
        <translation>Impossibile ottenere gli anelli</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2382"/>
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
        <translation>Specificare una lista di origini i cui elementi sono separati da virgola al fine di consentire la condivisione incrociata fra le origini</translation>
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
        <location filename="../src/rpc/rpc_args.cpp" line="101"/>
        <source>Username specified with --</source>
        <translation>Nome utente specificato con --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="101"/>
        <location filename="../src/rpc/rpc_args.cpp" line="111"/>
        <source> cannot be empty</source>
        <translation> non può essere vuoto</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="111"/>
        <source> requires RPC server password --</source>
        <translation> richiede la password del server RPC --</translation>
    </message>
</context>
<context>
    <name>cryptonote::simple_wallet</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="666"/>
        <source>Commands: </source>
        <translation>Comandi: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4636"/>
        <source>failed to read wallet password</source>
        <translation>impossibile leggere la password del portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4231"/>
        <source>invalid password</source>
        <translation>password non valida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3283"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation>imposta seed: richiede un argomento. opzioni disponibili: lingua</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3319"/>
        <source>set: unrecognized argument(s)</source>
        <translation>imposta: argomento/i non riconosciuto/i</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4476"/>
        <source>wallet file path not valid: </source>
        <translation>percorso file portafoglio non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3389"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation>Sto tentando di generare o ripristinare il portafoglio, ma i(l) file specificato/i esiste/esistono già. Sto uscendo per non rischiare di sovrascrivere.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3269"/>
        <source>needs an argument</source>
        <translation>ha bisogno di un argomento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3292"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3293"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3294"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3296"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3299"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3304"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3305"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3307"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3309"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3310"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3311"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3314"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3315"/>
        <source>0 or 1</source>
        <translation>0 o 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3302"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3306"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3313"/>
        <source>unsigned integer</source>
        <translation>intero senza segno</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3577"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation>specificare un parametro di ripristino con --electrum-seed=&quot;lista parole qui&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4164"/>
        <source>wallet failed to connect to daemon: </source>
        <translation>impossibile connettere il portafoglio al daemon: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4172"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation>Il daemon usa una versione principale RPC (%u) diversa da quella del portafoglio (%u): %s. Aggiorna una delle due, o usa --allow-mismatched-daemon-version.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4193"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation>Lista delle lingue disponibili per il seed del tuo portafoglio:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4277"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation>Hai usato una versione obsoleta del portafoglio. Per favore usa il nuovo seed che ti abbiamo fornito.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4293"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4365"/>
        <source>Generated new wallet: </source>
        <translation>Nuovo portafoglio generato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4302"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4370"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4412"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4465"/>
        <source>failed to generate new wallet: </source>
        <translation>impossibile generare nuovo portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4507"/>
        <source>Opened watch-only wallet</source>
        <translation>Portafoglio solo-vista aperto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4511"/>
        <source>Opened wallet</source>
        <translation>Portafoglio aperto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4529"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation>Stai utilizzando una versione disapprovata del portafoglio. Per favore procedi nell&apos;upgrade del portafoglio.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4544"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation>Stai utilizzando una versione disapprovata del portafoglio. Il formato del tuo portafoglio sta per essere aggiornato.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4552"/>
        <source>failed to load wallet: </source>
        <translation>impossibile caricare portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4569"/>
        <source>Use the &quot;help&quot; command to see the list of available commands.
</source>
        <translation>Usa il comando &quot;help&quot; per visualizzare la lista dei comandi disponibili.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4614"/>
        <source>Wallet data saved</source>
        <translation>Dati del portafoglio salvati</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4819"/>
        <source>Mining started in daemon</source>
        <translation>Mining avviato nel daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4821"/>
        <source>mining has NOT been started: </source>
        <translation>il mining NON è stato avviato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4841"/>
        <source>Mining stopped in daemon</source>
        <translation>Mining nel daemon interrotto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4843"/>
        <source>mining has NOT been stopped: </source>
        <translation>il mining NON è stato interrotto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4925"/>
        <source>Blockchain saved</source>
        <translation>Blockchain salvata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4940"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4973"/>
        <source>Height </source>
        <translation>Blocco </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4975"/>
        <source>spent </source>
        <translation>speso/i </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5089"/>
        <source>Starting refresh...</source>
        <translation>Sto iniziando il refresh...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5115"/>
        <source>Refresh done, blocks received: </source>
        <translation>Refresh finito, blocchi ricevuti: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6349"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>l&apos;id pagamento ha un formato invalido, dovrebbe essere una stringa hex di 16 o 64 caratteri: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5704"/>
        <source>bad locked_blocks parameter:</source>
        <translation>parametro locked_blocks non corretto:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6369"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6639"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation>una singola transazione non può usare più di un id pagamento: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5806"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6378"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6607"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6647"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>impossibile impostare id pagamento, anche se è stato decodificado correttamente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5659"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6271"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6564"/>
        <source>ring size %u is too large, maximum is %u</source>
        <translation>la dimensione dell&apos;anello %u è troppo grande, il massimo è %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5690"/>
        <source>payment id failed to encode</source>
        <translation>codifica dell&apos;ID di pagamento non riuscita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5737"/>
        <source>failed to parse short payment ID from URI</source>
        <translation>impossibile analizzare l&apos;ID di pagamento breve dall&apos;URI</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5762"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5764"/>
        <source>Invalid last argument: </source>
        <translation>Ultimo argomento non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5782"/>
        <source>a single transaction cannot use more than one payment id</source>
        <translation>una singola transazione non può utilizzare più di un ID di pagamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5800"/>
        <source>failed to parse payment id, though it was detected</source>
        <translation>errore nell&apos;analisi dell&apos;ID di pagamento, anche se è stato rilevato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5823"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5903"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5991"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6139"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6392"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6450"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6661"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6706"/>
        <source>transaction cancelled.</source>
        <translation>transazione cancellata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5942"/>
        <source>Sending %s.  </source>
        <translation>Sto inviando %s. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5945"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation>La tua transazione deve essere divisa in %llu transazioni. Una commissione verrà applicata per ogni transazione, per un totale di %s commissioni</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5951"/>
        <source>The transaction fee is %s</source>
        <translation>La commissione per la transazione è %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5954"/>
        <source>, of which %s is dust from change</source>
        <translation>, della quale %s è polvere dovuta allo scambio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5955"/>
        <source>.</source>
        <translation>.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5955"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation>Un totale di %s in polvere verrà inviato all&apos;indirizzo della polvere</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5960"/>
        <source>.
This transaction will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation>.
Questa transazione verrà sbloccata al blocco %llu, in approssimativamente %s giorni (supponendo 2 minuti per blocco)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6004"/>
        <source>Unsigned transaction(s) successfully written to MMS</source>
        <translation>Transazione(i) senza firma scritta(e) con successo su MMS</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6012"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6049"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6150"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6162"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6461"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6498"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6716"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6728"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Impossibile scrivere transazione/i su file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6017"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6054"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6154"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6166"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6465"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6502"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6720"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6732"/>
        <source>Unsigned transaction(s) successfully written to file: </source>
        <translation>Transazioni/e non firmata/e scritte/a con successo su file: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6026"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6477"/>
        <source>Failed to cold sign transaction with HW wallet</source>
        <translation>Impossibile firmare transazioni a freddo con il portafoglio hardware</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6109"/>
        <source>No unmixable outputs found</source>
        <translation>Nessun output non-mixabile trovato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6176"/>
        <source>Not enough money in unlocked balance</source>
        <translation>Fondi insufficienti in saldo sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6216"/>
        <source>No address given</source>
        <translation>Non è stato fornito nessun indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6280"/>
        <source>missing lockedblocks parameter</source>
        <translation>parametro lockedblocks mancante</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6290"/>
        <source>bad locked_blocks parameter</source>
        <translation>parametro locked_blocks errato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6315"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6573"/>
        <source>Failed to parse number of outputs</source>
        <translation>Impossibile analizzare il numero di outputs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6320"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6578"/>
        <source>Amount of outputs should be greater than 0</source>
        <translation>La quantità di outputs deve essere maggiore di 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6816"/>
        <source>Failed to parse donation address: </source>
        <translation>Impossibile analizzare l&apos;indirizzo di donazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6830"/>
        <source>Donating %s %s to The Monero Project (donate.getmonero.org or %s).</source>
        <translation>Donare %s %s a The Monero Project (donate.getmonero.org o %s).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6832"/>
        <source>Donating %s %s to %s.</source>
        <translation>Donare %s %s a %s.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6919"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>Il cambiamento richiesto non porta a un indirizzo pagato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6924"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>Il cambiamento richiesto è più largo del pagamento all&apos;indirizzo di cambio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6955"/>
        <source>sending %s to %s</source>
        <translation>sto mandando %s a %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6965"/>
        <source> dummy output(s)</source>
        <translation> output dummy</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6968"/>
        <source>with no destinations</source>
        <translation>senza destinazioni</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7009"/>
        <source>This is a multisig wallet, it can only sign with sign_multisig</source>
        <translation>Questo è un portafoglio multisig, può firmare solo con sign_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7032"/>
        <source>Failed to sign transaction</source>
        <translation>Impossibile firmare la transazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7038"/>
        <source>Failed to sign transaction: </source>
        <translation>Impossibile firmare la transazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7059"/>
        <source>Transaction raw hex data exported to </source>
        <translation>Dati esadecimali grezzi della transazione esportati su </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7080"/>
        <source>Failed to load transaction from file</source>
        <translation>Impossibile caricare la transazione da file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5132"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5459"/>
        <source>RPC error: </source>
        <translation>errore RPC: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="716"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation>il portafoglio è solo-vista e non ha una chiave di spesa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="860"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1044"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1097"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1164"/>
        <source>Your original password was incorrect.</source>
        <translation>La tua password originale era scorretta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="875"/>
        <source>Error with wallet rewrite: </source>
        <translation>Errore riscrittura wallet: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2435"/>
        <source>invalid unit</source>
        <translation>unità invalida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2453"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2515"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation>conteggio invalido: deve essere un intero senza segno</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2471"/>
        <source>invalid value</source>
        <translation>valore invalido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4021"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4041"/>
        <source>bad m_restore_height parameter: </source>
        <translation>parametro m_restore_height non corretto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3985"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4032"/>
        <source>Restore height is: </source>
        <translation>Ripristina altezza è: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4897"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Il daemon è locale, viene considerato fidato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4632"/>
        <source>Password for new watch-only wallet</source>
        <translation>Password per il nuovo portafoglio solo-vista</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5142"/>
        <source>internal error: </source>
        <translation>errore interno: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1632"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5147"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5464"/>
        <source>unexpected error: </source>
        <translation>errore inaspettato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1558"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1637"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5152"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5469"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6040"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6070"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6195"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6490"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6517"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6749"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7093"/>
        <source>unknown error</source>
        <translation>errore sconosciuto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5157"/>
        <source>refresh failed: </source>
        <translation>refresh fallito: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5157"/>
        <source>Blocks received: </source>
        <translation>Blocchi ricevuti: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5195"/>
        <source>unlocked balance: </source>
        <translation>bilancio sbloccato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3303"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5302"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5364"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>amount</source>
        <translation>ammontare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="362"/>
        <source>false</source>
        <translation>falso</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="680"/>
        <source>Unknown command: </source>
        <translation>Comando sconosciuto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="687"/>
        <source>Command usage: </source>
        <translation>Uso del comando: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="690"/>
        <source>Command description: </source>
        <translation>Descrizione del comando: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="756"/>
        <source>wallet is multisig but not yet finalized</source>
        <translation>il portafoglio è multisig ma ancora non finalizzato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="789"/>
        <source>Failed to retrieve seed</source>
        <translation>Impossibile recuperare il seed</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="813"/>
        <source>wallet is multisig and has no seed</source>
        <translation>il portafoglio è multisig e non ha seed</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="922"/>
        <source>Error: failed to estimate backlog array size: </source>
        <translation>Errore: impossibile stimare la dimensione dell&apos;array di backlog: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="927"/>
        <source>Error: bad estimated backlog array size</source>
        <translation>Errore: errata stima della dimensione dell&apos;array di backlog</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="939"/>
        <source> (current)</source>
        <translation> (attuale)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="942"/>
        <source>%u block (%u minutes) backlog at priority %u%s</source>
        <translation>Backlog blocco %u (%u minuti) a priorità %u%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="944"/>
        <source>%u to %u block (%u to %u minutes) backlog at priority %u</source>
        <translation>Backlog blocco %u a %u (%u a %u minuti) a priorità %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="947"/>
        <source>No backlog at priority </source>
        <translation>Nessun backlog a priorità </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="967"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1012"/>
        <source>This wallet is already multisig</source>
        <translation>Questo portafoglio è già multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="972"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1017"/>
        <source>wallet is watch-only and cannot be made multisig</source>
        <translation>il portafoglio è sola-visualizzazione e non può essere reso multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="978"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1023"/>
        <source>This wallet has been used before, please use a new wallet to create a multisig wallet</source>
        <translation>Questo portafoglio è stato usato precedentmente, per cortesia utilizza un nuovo portafoglio per creare un portafoglio multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="986"/>
        <source>Send this multisig info to all other participants, then use make_multisig &lt;threshold&gt; &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Invia queste informazioni multisig a tutti gli altri partecipanti, poi utilizza make_multisig &lt;threshold&gt; &lt;info1&gt; [&lt;info2&gt;...] con le informazioni multisig degli altri</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="987"/>
        <source>This includes the PRIVATE view key, so needs to be disclosed only to that multisig wallet&apos;s participants </source>
        <translation>Questo include la chiave PRIVATA di visualizzazione, pertanto deve essere comunicata solo ai partecipanti di quel portafoglio multisig </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1037"/>
        <source>Invalid threshold</source>
        <translation>Soglia invalida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1057"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1179"/>
        <source>Another step is needed</source>
        <translation>Ancora un ultimo passo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1069"/>
        <source>Error creating multisig: </source>
        <translation>Impossibile creare multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1076"/>
        <source>Error creating multisig: new wallet is not multisig</source>
        <translation>Impossibile creare multisig: il nuovo portafoglio non è multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1079"/>
        <source> multisig address: </source>
        <translation> indirizzo multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1103"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1152"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1219"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1285"/>
        <source>This wallet is not multisig</source>
        <translation>Questo portafoglio non è multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1108"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1157"/>
        <source>This wallet is already finalized</source>
        <translation>Questo portafoglio è già finalizzato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1124"/>
        <source>Failed to finalize multisig</source>
        <translation>Impossibile finalizzare multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1130"/>
        <source>Failed to finalize multisig: </source>
        <translation>Impossibile finalizzare multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1191"/>
        <source>Multisig address: </source>
        <translation>Indirizzo multifirma: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1224"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1290"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1384"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1500"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1581"/>
        <source>This multisig wallet is not yet finalized</source>
        <translation>Questo portafoglio multisig non è ancora finalizzato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1260"/>
        <source>Error exporting multisig info: </source>
        <translation>Impossibile esportare informazioni sul multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1264"/>
        <source>Multisig info exported to </source>
        <translation>Informazioni sul multisig esportate su </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1330"/>
        <source>Multisig info imported</source>
        <translation>Informazioni su multisig importate</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1334"/>
        <source>Failed to import multisig info: </source>
        <translation>Impossibile importare informazioni sul multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1345"/>
        <source>Failed to update spent status after importing multisig info: </source>
        <translation>Impossibile aggiornare lo stato di spesa dopo aver importato le informazioni sul multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1351"/>
        <source>Untrusted daemon, spent status may be incorrect. Use a trusted daemon and run &quot;rescan_spent&quot;</source>
        <translation>Daemon non fidato, lo stato di spesa potrebbe non essere corretto. Usare un daemon fidato ed eseguire &quot;rescan_spent&quot; </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1379"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1495"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1576"/>
        <source>This is not a multisig wallet</source>
        <translation>Questo non è un portafoglio multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1429"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1438"/>
        <source>Failed to sign multisig transaction</source>
        <translation>Impossibile firmare la transazione multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1445"/>
        <source>Multisig error: </source>
        <translation>Errore multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1450"/>
        <source>Failed to sign multisig transaction: </source>
        <translation>Impossibile firmare la transazione multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1473"/>
        <source>It may be relayed to the network with submit_multisig</source>
        <translation>Potrebbe essere trasmesso alla rete con submit_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1532"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1602"/>
        <source>Failed to load multisig transaction from file</source>
        <translation>Impossibile caricare la transazione multisig da file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1538"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1607"/>
        <source>Multisig transaction signed by only %u signers, needs %u more signatures</source>
        <translation>Transazione multisig firmata da solo %u firmatari, necessita di altre %u firme</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1547"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9330"/>
        <source>Transaction successfully submitted, transaction </source>
        <translation>Transazione inviata con successo, transazione </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1548"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9331"/>
        <source>You can check its status by using the `show_transfers` command.</source>
        <translation>E&apos; possibile controllare il suo stato mediante il comando `show_transfers`.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1623"/>
        <source>Failed to export multisig transaction to file </source>
        <translation>Impossibile esportare la transazione multisig su file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1627"/>
        <source>Saved exported multisig transaction file(s): </source>
        <translation>Transazioni esportate salvate su(i) file: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1892"/>
        <source>Invalid key image or txid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1901"/>
        <source>failed to unset ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2072"/>
        <source>usage: %s &lt;key_image&gt;|&lt;pubkey&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2117"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2129"/>
        <source>Frozen: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2131"/>
        <source>Not frozen: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2138"/>
        <source> bytes sent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2139"/>
        <source> bytes received</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2145"/>
        <source>Welcome to Monero, the private cryptocurrency.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2147"/>
        <source>Monero, like Bitcoin, is a cryptocurrency. That is, it is digital money.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2148"/>
        <source>Unlike Bitcoin, your Monero transactions and balance stay private, and not visible to the world by default.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2149"/>
        <source>However, you have the option of making those available to select parties, if you choose to.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2151"/>
        <source>Monero protects your privacy on the blockchain, and while Monero strives to improve all the time,</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2152"/>
        <source>no privacy technology can be 100% perfect, Monero included.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2153"/>
        <source>Monero cannot protect you from malware, and it may not be as effective as we hope against powerful adversaries.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2154"/>
        <source>Flaws in Monero may be discovered in the future, and attacks may be developed to peek under some</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2155"/>
        <source>of the layers of privacy Monero provides. Be safe and practice defense in depth.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2157"/>
        <source>Welcome to Monero and financial privacy. For more information, see https://getmonero.org/</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2244"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2250"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2269"/>
        <source>ring size must be an integer &gt;= </source>
        <translation>il ring size deve essere un intero &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2274"/>
        <source>could not change default ring size</source>
        <translation>impossibile modificare il ring size di default</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2549"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2620"/>
        <source>Invalid height</source>
        <translation>Altezza invalida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2667"/>
        <source>invalid argument: must be either 1/yes or 0/no</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2738"/>
        <source>Start mining in the daemon (bg_mining and ignore_battery are optional booleans).</source>
        <translation>Avvia il mining sul daemon (bg_mining e ignore_battery sono booleani opzionali).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2741"/>
        <source>Stop mining in the daemon.</source>
        <translation>Arresta il mining sul daemon.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2745"/>
        <source>Set another daemon to connect to.</source>
        <translation>Seleziona un altro daemon cui connettersi.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2748"/>
        <source>Save the current blockchain data.</source>
        <translation>Salva i dati blockchain correnti.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2751"/>
        <source>Synchronize the transactions and balance.</source>
        <translation>Sincronizza le transazioni ed il saldo.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2755"/>
        <source>Show the wallet&apos;s balance of the currently selected account.</source>
        <translation>Mostra il saldo del portafoglio del conto attualmente selezionato.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2759"/>
        <source>Show the incoming transfers, all or filtered by availability and address index.

Output format:
Amount, Spent(&quot;T&quot;|&quot;F&quot;), &quot;frozen&quot;|&quot;locked&quot;|&quot;unlocked&quot;, RingCT, Global Index, Transaction Hash, Address Index, [Public Key, Key Image] </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2765"/>
        <source>Show the payments for the given payment IDs.</source>
        <translation>Mostra i pagamenti per gli id pagamento specificati.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2768"/>
        <source>Show the blockchain height.</source>
        <translation>Mostra l&apos;altezza della blockchain.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2782"/>
        <source>Send all unmixable outputs to yourself with ring_size 1</source>
        <translation>Invia tutti gli output non mixabili a te stesso usando ring_size 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2789"/>
        <source>Send all unlocked outputs below the threshold to an address.</source>
        <translation>Invia tutti gli output sbloccati sotto la soglia ad un indirizzo.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2793"/>
        <source>Send a single output of the given key image to an address without change.</source>
        <translation>Invia un singolo output della key image specificata ad un indirizzo senza modifica.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2797"/>
        <source>Donate &lt;amount&gt; to the development team (donate.getmonero.org).</source>
        <translation>Dona &lt;amount&gt; al team di sviluppo (donate.getmonero.org).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2804"/>
        <source>Submit a signed transaction from a file.</source>
        <translation>Invia una transazione firmata da file.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2808"/>
        <source>Change the current log detail (level must be &lt;0-4&gt;).</source>
        <translation>Modifica il dettaglio di log (il livello deve essere &lt;0-4&gt;).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2812"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="2826"/>
        <source>Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation>Codifica un ID di pagamento in un indirizzo integrato per l&apos;indirizzo pubblico del portafoglio corrente (nessun argomento utilizza un ID di pagamento casuale) o decodifica un indirizzo integrato per l&apos;indirizzo standard e l&apos;ID di pagamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2830"/>
        <source>Print all entries in the address book, optionally adding/deleting an entry to/from it.</source>
        <translation>Stampa tutte le voci nella rubrica, opzionalmente aggiungendo/eliminando una voce a/da esso.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2833"/>
        <source>Save the wallet data.</source>
        <translation>Salva i dati del portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2836"/>
        <source>Save a watch-only keys file.</source>
        <translation>Salva un file di chiavi in sola lettura.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2839"/>
        <source>Display the private view key.</source>
        <translation>Mostra la chiave privata di visualizzazione.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2842"/>
        <source>Display the private spend key.</source>
        <translation>Mostra la chiave di spesa privata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2845"/>
        <source>Display the Electrum-style mnemonic seed</source>
        <translation>Mostra il seed mnemonico di tipo Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2849"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="2897"/>
        <source>Display the encrypted Electrum-style mnemonic seed.</source>
        <translation>Mostra il seed mnemonico criptato di tipo Electrum.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2900"/>
        <source>Rescan the blockchain for spent outputs.</source>
        <translation>Riscansiona la blockchain per gli outputs spesi.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2904"/>
        <source>Get the transaction key (r) for a given &lt;txid&gt;.</source>
        <translation>Ottieni la chiave di transazione (r) per il dato &lt;txid&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2912"/>
        <source>Check the amount going to &lt;address&gt; in &lt;txid&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2916"/>
        <source>Generate a signature proving funds sent to &lt;address&gt; in &lt;txid&gt;, optionally with a challenge string &lt;message&gt;, using either the transaction secret key (when &lt;address&gt; is not your wallet&apos;s address) or the view secret key (otherwise), which does not disclose the secret key.</source>
        <translation>Generare una firma che provi l&apos;invio dei fondi da &lt;address&gt; in &lt;txid&gt;, facoltativamente con una stringa di verifica &lt;message&gt;, utilizzando la chiave segreta della transazione (quando &lt;address&gt; non è l&apos;indirizzo del tuo portafoglio) o (in alternativa) la chiave di visualizzazione segreta, che non riveli la chiave segreta.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2920"/>
        <source>Check the proof for funds going to &lt;address&gt; in &lt;txid&gt; with the challenge string &lt;message&gt; if any.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2924"/>
        <source>Generate a signature proving that you generated &lt;txid&gt; using the spend secret key, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Genera una firma che dimostri che hai generato &lt;txid&gt; usando la chiave segreta di spesa, opzionalmente con una stringa di verifica &lt;message&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2928"/>
        <source>Check a signature proving that the signer generated &lt;txid&gt;, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Controlla la firma dimostrando che il firmatario ha generato &lt;txid&gt;, opzionalmente con una stringa di verifica &lt;message&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2932"/>
        <source>Generate a signature proving that you own at least this much, optionally with a challenge string &lt;message&gt;.
If &apos;all&apos; is specified, you prove the entire sum of all of your existing accounts&apos; balances.
Otherwise, you prove the reserve of the smallest possible amount above &lt;amount&gt; available in your current account.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2938"/>
        <source>Check a signature proving that the owner of &lt;address&gt; holds at least this much, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Controlla una firma che dimostri che il proprietario di &lt;address&gt; conserva questo valore, opzionalmente con una stringa di verifica &lt;message&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2958"/>
        <source>Show the unspent outputs of a specified address within an optional amount range.</source>
        <translation>Mostra gli outputs non spesi di uno specifico indirizzo all&apos;interno di un intervallo di quantità opzionale.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2962"/>
        <source>Rescan the blockchain from scratch. If &quot;hard&quot; is specified, you will lose any information which can not be recovered from the blockchain itself.</source>
        <translation>Scansiona la blockchain da zero. Se &quot;hard&quot; è specificato, perderai tutte le informazioni che non possono essere recuperate dalla blockchain stessa.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2966"/>
        <source>Set an arbitrary string note for a &lt;txid&gt;.</source>
        <translation>Imposta una nota arbitraria per un &lt;txid&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2970"/>
        <source>Get a string note for a txid.</source>
        <translation>Ottieni una nota di stringa per un txid.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2974"/>
        <source>Set an arbitrary description for the wallet.</source>
        <translation>Imposta una descrizione arbitraria per il portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2978"/>
        <source>Get the description of the wallet.</source>
        <translation>Ottieni la descrizione del portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2981"/>
        <source>Show the wallet&apos;s status.</source>
        <translation>Mostra lo stato del portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2984"/>
        <source>Show the wallet&apos;s information.</source>
        <translation>Mostra le informazioni del portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2988"/>
        <source>Sign the contents of a file.</source>
        <translation>Firma il contenuto di un file.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2992"/>
        <source>Verify a signature on the contents of a file.</source>
        <translation>Verifica una firma sul contenuto di un file.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3000"/>
        <source>Import a signed key images list and verify their spent status.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3012"/>
        <source>Export a set of outputs owned by this wallet.</source>
        <translation>Esporta un set di output posseduto da questo portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3016"/>
        <source>Import a set of outputs owned by this wallet.</source>
        <translation>Importa un set di outputs posseduto da questo portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3020"/>
        <source>Show information about a transfer to/from this address.</source>
        <translation>Mostra informazioni su un trasferimento da/verso questo indirizzo.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3023"/>
        <source>Change the wallet&apos;s password.</source>
        <translation>Cambia la password del portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3030"/>
        <source>Print the information about the current fee and transaction backlog.</source>
        <translation>Stampa le informazioni sulla commissione corrente e sul backlog della transazione.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3032"/>
        <source>Export data needed to create a multisig wallet</source>
        <translation>Esportare i dati necessari per creare un portafoglio multifirma</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3035"/>
        <source>Turn this wallet into a multisig wallet</source>
        <translation>Trasforma questo portafoglio in un portafoglio multifirma</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3039"/>
        <source>Turn this wallet into a multisig wallet, extra step for N-1/N wallets</source>
        <translation>Trasforma questo portafoglio in un portafoglio multifirma, passo aggiuntivo per i portafogli N-1/N</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3047"/>
        <source>Export multisig info for other participants</source>
        <translation>Esporta informazioni multifirma per altri partecipanti</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3051"/>
        <source>Import multisig info from other participants</source>
        <translation>Importa informazioni multifirma da altri partecipanti</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3055"/>
        <source>Sign a multisig transaction from a file</source>
        <translation>Firma una transazione multifirma da un file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3059"/>
        <source>Submit a signed multisig transaction from a file</source>
        <translation>Invia una transazione multifirma firmata da un file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3063"/>
        <source>Export a signed multisig transaction to a file</source>
        <translation>Esporta una transazione multifirma firmata in un file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3160"/>
        <source>Unsets the ring used for a given key image or transaction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3180"/>
        <source>Freeze a single output by key image so it will not be used</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3184"/>
        <source>Thaw a single output by key image so it may be used again</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3188"/>
        <source>Checks whether a given output is currently frozen by key image</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3192"/>
        <source>Prints simple network stats</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3196"/>
        <source>Prints basic info about Monero for first time users</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3204"/>
        <source>Show the help section or the documentation about a &lt;command&gt;.</source>
        <translation>Mostra la sezione di aiuto o la documentazione su un &lt;command&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3258"/>
        <source> (set this to support the network and to get a chance to receive new monero)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3295"/>
        <source>integer &gt;= </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3308"/>
        <source>block height</source>
        <translation>altezza del blocco</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3316"/>
        <source>1/yes or 0/no</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3414"/>
        <source>No wallet found with that name. Confirm creation of new wallet named: </source>
        <translation>Nessun portafoglio trovato con quel nome. Conferma la creazione di un nuovo portafoglio chiamato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3540"/>
        <source>can&apos;t specify both --restore-deterministic-wallet or --restore-multisig-wallet and --non-deterministic</source>
        <translation>non è possibile specificare sia --restore-deterministic-wallet o --restore-multisig-wallet e --non-deterministic</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3546"/>
        <source>--restore-multisig-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-multisig-wallet utilizza --generate-new-wallet, non --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3562"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;multisig seed here&quot;</source>
        <translation>specifica un parametro di recupero con --electrum-seed=&quot; seed multifirma qui&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3591"/>
        <source>Multisig seed failed verification</source>
        <translation>Verifica seed mulitfirma fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3641"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3718"/>
        <source>This address is a subaddress which cannot be used here.</source>
        <translation>Questo indirizzo è un sottoindirizzo che non può essere utilizzato qui.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3796"/>
        <source>Error: expected M/N, but got: </source>
        <translation>Errore: atteso M/N, ma ottenuto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3801"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3806"/>
        <source>Error: M/N is currently unsupported. </source>
        <translation>Errore: M/N non è attualmente supportato. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3809"/>
        <source>Generating master wallet from %u of %u multisig wallet keys</source>
        <translation>Generazione del portafoglio principale da %u di %u chiavi del portafoglio multifirma</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3838"/>
        <source>failed to parse secret view key</source>
        <translation type="unfinished">impossibile fare il parsing della chiave segreta di visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3846"/>
        <source>failed to verify secret view key</source>
        <translation type="unfinished">verifica chiave segreta di visualizzazione fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3889"/>
        <source>Error: M/N is currently unsupported</source>
        <translation>Errore: M/N non è attualmente supportato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4055"/>
        <source>Restore height </source>
        <translation>Altezza di ripristino </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4083"/>
        <source>Warning: using an untrusted daemon at %s, privacy will be lessened</source>
        <translation>Attenzione: sto utilizzando un daemon non fidato su %s, la privacy sarà ridotta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4093"/>
        <source>If you are new to Monero, type &quot;welcome&quot; for a brief overview.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4098"/>
        <source>WARNING: obsolete long payment IDs are enabled. Sending transactions with those payment IDs are bad for your privacy.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4100"/>
        <source>It is recommended that you do not use them, and ask recipients who ask for one to not endanger your privacy.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4165"/>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or change the daemon address using the &apos;set_daemon&apos; command.</source>
        <translation>Il daemon non è stato avviato o la porta è errata. Assicurati che il daemon sia in esecuzione oppure cambia l&apos;indirizzo del daemon usando il comando &apos;set_daemon&apos;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4203"/>
        <source>Enter the number corresponding to the language of your choice</source>
        <translation>Inserire il numero corrispondente alla lingua desiderata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4313"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="4457"/>
        <source>failed to generate new mutlisig wallet</source>
        <translation>impossibile generare un nuovo portafoglio multifirma</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4460"/>
        <source>Generated new %u/%u multisig wallet: </source>
        <translation>Generato nuovo portafoglio %u/%u multifirma: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4509"/>
        <source>Opened %u/%u multisig wallet%s</source>
        <translation>Aperto %u/%u portafoglio multifirma%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4570"/>
        <source>Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
</source>
        <translation>Usa &quot;help &lt;command&gt;&quot; per vedere la documentazione di un comando.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4628"/>
        <source>wallet is multisig and cannot save a watch-only version</source>
        <translation>il portafoglio è multifirma e non è possibile salvarne una versione in sola lettura</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4664"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4696"/>
        <source>Failed to query mining status: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4679"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4707"/>
        <source>Failed to setup background mining: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4683"/>
        <source>Background mining enabled. Thank you for supporting the Monero network.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4711"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4719"/>
        <source>Background mining not enabled. Run &quot;set setup-background-mining 1&quot; to change.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4725"/>
        <source>Using an untrusted daemon, skipping background mining check</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4750"/>
        <source>The daemon is not set up to background mine.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4751"/>
        <source>With background mining enabled, the daemon will mine when idle and not on batttery.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4752"/>
        <source>Enabling this supports the network you are using, and makes you eligible for receiving new monero</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4757"/>
        <source>Background mining not enabled. Set setup-background-mining to 1 to change.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4864"/>
        <source>Unexpected array length - Exited simple_wallet::set_daemon()</source>
        <translation>Lunghezza array imprevista - Exited simple_wallet:: set_daemon ()</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4905"/>
        <source>This does not seem to be a valid daemon URL.</source>
        <translation>Questo URL del daemon non sembra essere valido.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4941"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4974"/>
        <source>txid </source>
        <translation>txid </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4943"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4976"/>
        <source>idx </source>
        <translation>idx </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4956"/>
        <source>WARNING: this transaction uses an unencrypted payment ID: consider using subaddresses instead.</source>
        <translation>ATTENZIONE: questa transazione utilizza un ID di pagamento non criptato: considera l&apos;utilizzo di sottoindirizzi.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4956"/>
        <source>WARNING: this transaction uses an unencrypted payment ID: these are obsolete. Support will be withdrawn in the future. Use subaddresses instead.</source>
        <translation>ATTENZIONE: questa transazione utilizza un ID di pagamento non criptato: questi sono obsoleti. Il supporto ad essi verrà sospeso in futuro. Utilizzare sottoindirizzi.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5108"/>
        <source>New transfer received since rescan was started. Key images are incomplete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5183"/>
        <source> (Some owned outputs have partial key images - import_multisig_info needed)</source>
        <translation> (Alcuni output contengono immagini chiave parziali - import_multisig_info necessario)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5186"/>
        <source>Currently selected account: [</source>
        <translation>Account attualmente selezionato: [</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5186"/>
        <source>] </source>
        <translation>] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5188"/>
        <source>Tag: </source>
        <translation>Etichetta: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5188"/>
        <source>(No tag assigned)</source>
        <translation>(Nessuna etichetta assegnata)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5200"/>
        <source>Balance per address:</source>
        <translation>Saldo per indirizzo:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5201"/>
        <source>Address</source>
        <translation>Indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5201"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8451"/>
        <source>Balance</source>
        <translation>Saldo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5201"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8451"/>
        <source>Unlocked balance</source>
        <translation>Saldo sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5201"/>
        <source>Outputs</source>
        <translation>Outputs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5201"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8451"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9564"/>
        <source>Label</source>
        <translation>Etichetta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5209"/>
        <source>%8u %6s %21s %21s %7u %21s</source>
        <translation>%8u %6s %21s %21s %7u %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5302"/>
        <source>spent</source>
        <translation>spesi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5302"/>
        <source>global index</source>
        <translation>indice globale</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5302"/>
        <source>tx id</source>
        <translation>tx id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5302"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5364"/>
        <source>addr index</source>
        <translation>indice indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5312"/>
        <source>Used at heights: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5318"/>
        <source>[frozen]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5332"/>
        <source>No incoming transfers</source>
        <translation>Nessun trasferimento in entrata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5336"/>
        <source>No incoming available transfers</source>
        <translation>Nessun trasferimento in entrata disponibile</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5340"/>
        <source>No incoming unavailable transfers</source>
        <translation>Nessun trasferimento indisponibile in entrata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5364"/>
        <source>payment</source>
        <translation>pagamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5364"/>
        <source>transaction</source>
        <translation>transazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5364"/>
        <source>height</source>
        <translation>altezza</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5364"/>
        <source>unlock time</source>
        <translation>tempo sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5376"/>
        <source>No payments with id </source>
        <translation>Nessun pagamento con id </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5424"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5514"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5843"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6302"/>
        <source>failed to get blockchain height: </source>
        <translation>impossibile recuperare altezza blockchain: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5522"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation>
Transazione %llu/%llu: txid=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5559"/>
        <source>failed to get output: </source>
        <translation>impossibile recuperare output: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5567"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation>l&apos;altezza del blocco di origine della chiave di output non dovrebbe essere più alta dell&apos;altezza della blockchain</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5571"/>
        <source>
Originating block heights: </source>
        <translation>
Originando blocchi: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5583"/>
        <source>
|</source>
        <translation>
|</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5583"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8110"/>
        <source>|
</source>
        <translation>|
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5600"/>
        <source>
Warning: Some input keys being spent are from </source>
        <translation>
Avviso: alcune chiavi di input spese vengono da </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5602"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation>, che potrebbe compromettere l&apos;anonimità della ring signature. Assicurati di farlo intenzionalmente!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5642"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6254"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6547"/>
        <source>Ring size must not be 0</source>
        <translation>Il ring size non può essere 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5654"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6266"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6559"/>
        <source>ring size %u is too small, minimum is %u</source>
        <translation>il ring size %u è troppo piccolo, il minimo è %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5666"/>
        <source>wrong number of arguments</source>
        <translation>numero di argomenti errato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5686"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5796"/>
        <source>Warning: Unencrypted payment IDs will harm your privacy: ask the recipient to use subaddresses instead</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5859"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6407"/>
        <source>No outputs found, or daemon is not ready</source>
        <translation>Nessun output trovato, o il daemon non è pronto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7164"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7175"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7182"/>
        <source>failed to parse tx_key</source>
        <translation>impossibile analizzare tx_key</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7191"/>
        <source>Tx key successfully stored.</source>
        <translation>Tx key memorizzata con successo.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7195"/>
        <source>Failed to store tx key: </source>
        <translation>Impossibile memorizzare la chiave tx: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7696"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>block</source>
        <translation>blocco</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7844"/>
        <source>usage: show_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation>utilizzo: show_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7897"/>
        <source>usage: export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;path&gt;]</source>
        <translation>utilizzo: export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;path&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>direction</source>
        <translation>direzione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>timestamp</source>
        <translation>timestamp</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>running balance</source>
        <translation>bilancio corrente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>hash</source>
        <translation>hash</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>payment ID</source>
        <translation>ID di pagamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>fee</source>
        <translation>tassa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>destination</source>
        <translation>destinazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>index</source>
        <translation>indice</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>note</source>
        <translation>nota</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7976"/>
        <source>CSV exported to </source>
        <translation>CSV esportato in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8159"/>
        <source>Warning: this will lose any information which can not be recovered from the blockchain.</source>
        <translation>Attenzione: questo farà perdere ogni informazione che non può essere recuperata dalla blockchain.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8160"/>
        <source>This includes destination addresses, tx secret keys, tx notes, etc</source>
        <translation>Questo include gli indirizzi di destinazione, le chiavi segrete di tx, le note di tx, ecc</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8172"/>
        <source>Warning: your restore height is higher than wallet restore height: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8173"/>
        <source>Rescan anyway ? (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8192"/>
        <source>MMS received new message</source>
        <translation>ricevuto un nuovo messaggio MMS</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8832"/>
        <source>Network type: </source>
        <translation>Tipo di rete: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8833"/>
        <source>Testnet</source>
        <translation>Testnet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8834"/>
        <source>Stagenet</source>
        <translation>Stagenet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8834"/>
        <source>Mainnet</source>
        <translation>Mainnet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8999"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9045"/>
        <source>command only supported by HW wallet</source>
        <translation>comando supportato solo dal portafoglio HW</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9004"/>
        <source>hw wallet does not support cold KI sync</source>
        <translation>Il portafoglio hardware non supporta la sincronizzazione cold KI</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9016"/>
        <source>Please confirm the key image sync on the device</source>
        <translation>Si prega di confermare la sincronizzazione della chiave immagine sul dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9022"/>
        <source>Key images synchronized to height </source>
        <translation>Immagini della chiave sincronizzate all&apos;altezza </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9025"/>
        <source>Running untrusted daemon, cannot determine which transaction output is spent. Use a trusted daemon with --trusted-daemon and run rescan_spent</source>
        <translation>L&apos;esecuzione di un daemon non fidato non può determinare quale output di transazione viene speso. Utilizzare un daemon fidato con --trusted-daemon ed eseguire rescan_spent</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9028"/>
        <source> spent, </source>
        <translation> speso, </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9028"/>
        <source> unspent</source>
        <translation> non speso</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9032"/>
        <source>Failed to import key images</source>
        <translation>Impossibile importare le immagini della chiave</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9037"/>
        <source>Failed to import key images: </source>
        <translation type="unfinished">Impossibile importare le key images: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9054"/>
        <source>Failed to reconnect device</source>
        <translation>Impossibile ricollegare il dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9059"/>
        <source>Failed to reconnect device: </source>
        <translation>Impossibile ricollegare il dispositivo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9323"/>
        <source>Transaction successfully saved to </source>
        <translation>Transazione salvata correttamente in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9323"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9325"/>
        <source>, txid </source>
        <translation>, txid </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9325"/>
        <source>Failed to save transaction to </source>
        <translation>Impossibile salvare la transazione in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7014"/>
        <source>This is a watch only wallet</source>
        <translation>Questo è un portafoglio solo-vista</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9253"/>
        <source>Double spend seen on the network: this transaction may or may not end up being mined</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9288"/>
        <source>Transaction ID not found</source>
        <translation>ID transazione non trovato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="357"/>
        <source>true</source>
        <translation>vero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="410"/>
        <source>failed to parse refresh type</source>
        <translation>impossibile fare il parsing del tipo di refresh</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="742"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="808"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="962"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1007"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1090"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1147"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1214"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1280"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1374"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1490"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1571"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7004"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7068"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7105"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7410"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7494"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8842"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8919"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8962"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9070"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9110"/>
        <source>command not supported by HW wallet</source>
        <translation>comando non supportato dal portafoglio HW</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="747"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="818"/>
        <source>wallet is watch-only and has no seed</source>
        <translation>il portafoglio è solo-vista e non possiede un seed</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="765"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="828"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation>il portafoglio è non-deterministico e non possiede un seed</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="772"/>
        <source>Enter optional seed offset passphrase, empty to see raw seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="838"/>
        <source>Incorrect password</source>
        <translation>Password errata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="906"/>
        <source>Current fee is %s %s per %s</source>
        <translation>La commissione attuale è %s %s per %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1059"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1181"/>
        <source>Send this multisig info to all other participants, then use exchange_multisig_keys &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Invia queste informazioni multifirma a tutti i partecipanti, quindi utilizza exchange_multisig_keys &lt;info1&gt; [&lt;info2&gt;...] con le informazioni multifirma degli altri partecipanti</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1190"/>
        <source>Multisig wallet has been successfully created. Current wallet type: </source>
        <translation>Il portafoglio multifirma è stato creato con successo. Tipo di portafoglio corrente: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1196"/>
        <source>Failed to perform multisig keys exchange: </source>
        <translation>Impossibile eseguire lo scambio di chiavi multifirma: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1523"/>
        <source>Failed to load multisig transaction from MMS</source>
        <translation>Impossibile caricare la transazione multifirma da MMS</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1655"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1812"/>
        <source>Invalid key image</source>
        <translation>Immagine della chiave non valida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1661"/>
        <source>Invalid txid</source>
        <translation>Txid non valido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1673"/>
        <source>Key image either not spent, or spent with mixin 0</source>
        <translation>Immagine della chiave non spesa o spesa con mixin 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1688"/>
        <source>Failed to get key image ring: </source>
        <translation>Impossibile ottenere l&apos;anello della chiave immagine: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1703"/>
        <source>File doesn&apos;t exist</source>
        <translation>Il file non esiste</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1725"/>
        <source>Invalid ring specification: </source>
        <translation>Specifica dell&apos;anello non valida: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1733"/>
        <source>Invalid key image: </source>
        <translation>Immagine della chiave non valida: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1738"/>
        <source>Invalid ring type, expected relative or abosolute: </source>
        <translation>Tipo di anello non valido, previsto relativo o assoluto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1744"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1756"/>
        <source>Error reading line: </source>
        <translation>Errore durante la lettura della riga: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1767"/>
        <source>Invalid ring: </source>
        <translation>Anello non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1776"/>
        <source>Invalid relative ring: </source>
        <translation>Anello relativo non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1788"/>
        <source>Invalid absolute ring: </source>
        <translation>Anello assoluto non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1797"/>
        <source>Failed to set ring for key image: </source>
        <translation>Impossibile impostare l&apos;anello per l&apos;immagine della chiave: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1797"/>
        <source>Continuing.</source>
        <translation>Continuando.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1827"/>
        <source>Missing absolute or relative keyword</source>
        <translation>Parola chiave assoluta o relativa mancante</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1837"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1844"/>
        <source>invalid index: must be a strictly positive unsigned integer</source>
        <translation>indice non valido: deve essere un numero intero senza segno e positivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1852"/>
        <source>invalid index: indices wrap</source>
        <translation>indice non valido: indici wrap</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1862"/>
        <source>invalid index: indices should be in strictly ascending order</source>
        <translation>indice non valido: gli indici devono essere rigorosamente in ordine crescente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1869"/>
        <source>failed to set ring</source>
        <translation>impossibile impostare l&apos;anello</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1946"/>
        <source>First line is not an amount</source>
        <translation>La prima riga non è un importo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1960"/>
        <source>Invalid output: </source>
        <translation>Output non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1970"/>
        <source>Bad argument: </source>
        <translation>Argomento non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1970"/>
        <source>should be &quot;add&quot;</source>
        <translation>dovrebbe essere &quot;add&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1979"/>
        <source>Failed to open file</source>
        <translation>Impossibile aprire il file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1985"/>
        <source>Invalid output key, and file doesn&apos;t exist</source>
        <translation>Chiave di output non valida, il file non esiste</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1991"/>
        <source>Failed to mark output spent: </source>
        <translation>Impossibile contrassegnare l&apos;output come speso: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2008"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2035"/>
        <source>Invalid output</source>
        <translation>Output non valido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2018"/>
        <source>Failed to mark output unspent: </source>
        <translation>Impossibile contrassegnare l&apos;output non speso: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2042"/>
        <source>Spent: </source>
        <translation>Speso: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2044"/>
        <source>Not spent: </source>
        <translation>Non speso: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2048"/>
        <source>Failed to check whether output is spent: </source>
        <translation>Impossibile verificare se l&apos;output è stato speso: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2063"/>
        <source>Failed to save known rings: </source>
        <translation>Impossibile salvare gli anelli conosciuti: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2171"/>
        <source>Please confirm the transaction on the device</source>
        <translation>Si prega di confermare la transazione sul dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2218"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2237"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation>il portafoglio è solo-vista e non può eseguire trasferimenti</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2255"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5982"/>
        <source>WARNING: this is a non default ring size, which may harm your privacy. Default is recommended.</source>
        <translation>ATTENZIONE: impostazione dell&apos;anello non default, la tua privacy potrebbe essere compromessa. Si consiglia di utilizzare l&apos;impostazione predefinita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2257"/>
        <source>WARNING: from v8, ring size will be fixed and this setting will be ignored.</source>
        <translation>ATTENZIONE: dalla v8, la dimensione dell&apos;anello sarà fissa e questa impostazione verrà ignorata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2286"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2309"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2325"/>
        <source>priority must be either 0, 1, 2, 3, or 4, or one of: </source>
        <translation>la priorità deve essere 0, 1, 2, 3 o 4 o una di: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2330"/>
        <source>could not change default priority</source>
        <translation>impossibile cambiare priorità standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2400"/>
        <source>invalid argument: must be either 0/never, 1/action, or 2/encrypt/decrypt</source>
        <translation>argomento non valido: deve essere 0/never, 1/action, or 2/encrypt/decrypt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2686"/>
        <source>Device name not specified</source>
        <translation>Nome del dispositivo non specificato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2695"/>
        <source>Device reconnect failed</source>
        <translation>Riconnessione del dispositivo fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2700"/>
        <source>Device reconnect failed: </source>
        <translation>Riconnessione del dispositivo fallita: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2771"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt;. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Trasferisci &lt;amount&gt; in &lt;address&gt;. Se viene specificato il parametro &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; il portafoglio utilizza le uscite ricevute dagli indirizzi di tali indici. Se omesso, il portafoglio sceglie casualmente gli indici dell&apos;indirizzo da utilizzare. In ogni caso, fa del suo meglio per non combinare gli output su più indirizzi. &lt;priority&gt; è la priorità della transazione. Maggiore è la priorità, maggiore è la commissione di transazione. I valori validi in ordine di priorità (dal più basso al più alto) sono: non importante, normale, elevata, priorità. Se omesso, viene utilizzato il valore predefinito (consultare il comando &quot;set priority&quot;). &lt;ring_size&gt; è il numero di input da includere per la non tracciabilità. È possibile effettuare pagamenti multipli contemporaneamente aggiungendo URI_2 o &lt;address_2&gt; &lt;amount_2&gt; eccetera (prima dell&apos;ID pagamento, se questo è incluso)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2775"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Trasferisci &lt;amount&gt; in &lt;address&gt; e bloccalo per &lt;lockblocks&gt; (massimo 1000000). Se viene specificato il parametro &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot;, il portafoglio utilizza le uscite ricevute dagli indirizzi di tali indici. Se omesso, il portafoglio sceglie casualmente gli indici dell&apos;indirizzo da utilizzare. In ogni caso, fa del suo meglio per non combinare gli output su più indirizzi. &lt;priority&gt; è la priorità della transazione. Maggiore è la priorità, maggiore è la commissione di transazione. I valori validi in ordine di priorità (dal più basso al più alto) sono: non importante, normale, elevata, priorità. Se omesso, viene utilizzato il valore predefinito (consultare il comando &quot;set priority&quot;). &lt;ring_size&gt; è il numero di input da includere per la non tracciabilità. È possibile effettuare pagamenti multipli contemporaneamente aggiungendo URI_2 o &lt;address_2&gt; &lt;amount_2&gt; eccetera (prima dell&apos;ID pagamento, se questo è incluso)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2779"/>
        <source>Send all unlocked balance to an address and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. &lt;priority&gt; is the priority of the sweep. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability.</source>
        <translation>Invia tutto il saldo sbloccato a un indirizzo e bloccalo per &lt;lockblocks&gt; (massimo 1000000). Se viene specificato il parametro &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot;, le uscite sweep del portafoglio vengono ricevute da tali indici dell&apos;indirizzo. Se omesso, il portafoglio sceglie casualmente un indice dell&apos;indirizzo da utilizzare. &lt;priority&gt; è la priorità dello sweep. Maggiore è la priorità, maggiore è la commissione di transazione. I valori validi in ordine di priorità (dal più basso al più alto) sono: non importante, normale, elevata, priorità. Se omesso, viene utilizzato il valore predefinito (consultare il comando &quot;set priority&quot;). &lt;ring_size&gt; è il numero di input da includere per la non tracciabilità.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2785"/>
        <source>Send all unlocked balance to an address. If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. If the parameter &quot;outputs=&lt;N&gt;&quot; is specified and  N &gt; 0, wallet splits the transaction into N even outputs.</source>
        <translation>Invia tutto il saldo sbloccato a un indirizzo. Se viene specificato il parametro &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot;, le uscite sweep del portafoglio vengono ricevute da tali indici dell&apos;indirizzo. Se omesso, il portafoglio sceglie casualmente un indice dell&apos;indirizzo da utilizzare. Se il parametro &quot;outputs=&lt;N&gt;&quot; è specificato e  N &gt; 0, il portafoglio divide la transazione in N uscite pari.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2801"/>
        <source>Sign a transaction from a file. If the parameter &quot;export_raw&quot; is specified, transaction raw hex data suitable for the daemon RPC /sendrawtransaction is exported.</source>
        <translation>Firma una transazione da un file. Se viene specificato il parametro &quot;export_raw&quot;, vengono esportati i dati esadecimali grezzi della transazione adatti al daemon RPC /sendrawtransaction.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2822"/>
        <source>If no arguments are specified or &lt;index&gt; is specified, the wallet shows the default or specified address. If &quot;all&quot; is specified, the wallet shows all the existing addresses in the currently selected account. If &quot;new &quot; is specified, the wallet creates a new address with the provided label text (which can be empty). If &quot;label&quot; is specified, the wallet sets the label of the address specified by &lt;index&gt; to the provided label text.</source>
        <translation>Se non viene specificato alcun argomento o viene specificato &lt;index&gt;, il portafoglio mostra l&apos;indirizzo predefinito o specificato. Se è specificato &quot;all&quot;, il portafoglio mostra tutti gli indirizzi esistenti nell&apos;account attualmente selezionato. Se viene specificato &quot;new&quot;, il portafoglio crea un nuovo indirizzo con il testo dell&apos;etichetta fornito (che può essere vuoto). Se si specifica &quot;label&quot;, il portafoglio imposta l&apos;etichetta dell&apos;indirizzo specificato da &lt;index&gt; sul testo dell&apos;etichetta fornito.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2908"/>
        <source>Set the transaction key (r) for a given &lt;txid&gt; in case the tx was made by some other device or 3rd party wallet.</source>
        <translation>Imposta la chiave di transazione (r) per un dato &lt;txid&gt; nel caso in cui il tx sia stato creato da qualche altro dispositivo o portafoglio di terze parti.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2943"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="2953"/>
        <source>export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;filepath&gt;]</source>
        <translation>export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;filepath&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2954"/>
        <source>Export to CSV the incoming/outgoing transfers within an optional height range.</source>
        <translation>Esporta in CSV i trasferimenti in entrata/uscita all&apos;interno di un intervallo di altezza opzionale.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2996"/>
        <source>Export a signed set of key images to a &lt;filename&gt;.</source>
        <translation>Esporta un set firmato di immagini chiave in un &lt;filename&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3004"/>
        <source>Synchronizes key images with the hw wallet.</source>
        <translation>Sincronizza le immagini chiave con il portafoglio hw.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3008"/>
        <source>Attempts to reconnect HW wallet.</source>
        <translation>Tenta di ricollegare il portafoglio HW.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3027"/>
        <source>Generate a new random full size payment id (obsolete). These will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids.</source>
        <translation>Genera un nuovo ID di pagamento lungo casuale (obsoleto). Questi non saranno crittati sulla blockchain, vedere integrated_address per gli ID di pagamento brevi crittografati.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3043"/>
        <source>Performs extra multisig keys exchange rounds. Needed for arbitrary M/N multisig wallets</source>
        <translation>Esegue scambi extra di chiavi multifirma. Necessario per portafogli multisig M/N arbitrari</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3067"/>
        <source>Interface with the MMS (Multisig Messaging System)
&lt;subcommand&gt; is one of:
  init, info, signer, list, next, sync, transfer, delete, send, receive, export, note, show, set, help
  send_signer_config, start_auto_config, stop_auto_config, auto_config
Get help about a subcommand with: help mms &lt;subcommand&gt;, or mms help &lt;subcommand&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3075"/>
        <source>Initialize and configure the MMS for M/N = number of required signers/number of authorized signers multisig</source>
        <translation>Inizializza e configura MMS per M/N = numero di firmatari richiesti/numero di multifirma dei firmatari autorizzati</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3079"/>
        <source>Display current MMS configuration</source>
        <translation>Mostra la configurazione MMS corrente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3083"/>
        <source>Set or modify authorized signer info (single-word label, transport address, Monero address), or list all signers</source>
        <translation>Imposta o modifica le informazioni del firmatario autorizzato (etichetta con una sola parola, indirizzo di trasporto, indirizzo Monero) oppure elenca tutti i firmatari</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3087"/>
        <source>List all messages</source>
        <translation>Elenca tutti i messaggi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3091"/>
        <source>Evaluate the next possible multisig-related action(s) according to wallet state, and execute or offer for choice
By using &apos;sync&apos; processing of waiting messages with multisig sync info can be forced regardless of wallet state</source>
        <translation>Valuta le azioni multifirma successive secondo lo stato del portafoglio, esegui o proponi una scelta
Utilizzando l&apos;elaborazione &apos;sync&apos; dei messaggi in attesa con le informazioni di sincronizzazione multifirma, è possibile forzare indipendentemente dallo stato del wallet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3096"/>
        <source>Force generation of multisig sync info regardless of wallet state, to recover from special situations like &quot;stale data&quot; errors</source>
        <translation>Generazione forzata di informazioni di sincronizzazione multifirma indipendente dallo stato del wallet, per il ripristino da situazioni speciali come errori di &quot;stale data&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3100"/>
        <source>Initiate transfer with MMS support; arguments identical to normal &apos;transfer&apos; command arguments, for info see there</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3104"/>
        <source>Delete a single message by giving its id, or delete all messages by using &apos;all&apos;</source>
        <translation>Elimina un singolo messaggio inserendo il suo id, o cancella tutti i messaggi usando &apos;all&apos;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3108"/>
        <source>Send a single message by giving its id, or send all waiting messages</source>
        <translation>Invia un singolo messaggio inserendo il suo id o inviando tutti i messaggi in attesa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3112"/>
        <source>Check right away for new messages to receive</source>
        <translation>Controlla subito i nuovi messaggi da ricevere</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3116"/>
        <source>Write the content of a message to a file &quot;mms_message_content&quot;</source>
        <translation>Scrivi il contenuto di un messaggio in un file &quot;mms_message_content&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3120"/>
        <source>Send a one-line message to an authorized signer, identified by its label, or show any waiting unread notes</source>
        <translation>Invia un messaggio di una linea a un firmatario autorizzato, identificato dalla sua etichetta, o mostra eventuali note non lette in attesa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3124"/>
        <source>Show detailed info about a single message</source>
        <translation>Mostra informazioni dettagliate su un singolo messaggio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3128"/>
        <source>Available options:
 auto-send &lt;1|0&gt;
   Whether to automatically send newly generated messages right away.
 </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3134"/>
        <source>Send completed signer config to all other authorized signers</source>
        <translation>Invia la configurazione completa del firmatario a tutti gli altri firmatari autorizzati</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3138"/>
        <source>Start auto-config at the auto-config manager&apos;s wallet by issuing auto-config tokens and optionally set others&apos; labels</source>
        <translation>Avvia auto-config sul portafoglio del gestore automatico di configurazione emettendo i token di auto-configurazione e opzionalmente imposta le etichette degli altri</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3142"/>
        <source>Delete any auto-config tokens and abort a auto-config process</source>
        <translation>Elimina tutti i token di configurazione automatica e interrompi il processo di configurazione automatica</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3146"/>
        <source>Start auto-config by using the token received from the auto-config manager</source>
        <translation>Avviare la configurazione automatica utilizzando il token ricevuto dal gestore di configurazione automatica</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3150"/>
        <source>Print the ring(s) used to spend a given key image or transaction (if the ring size is &gt; 1)

Output format:
Key Image, &quot;absolute&quot;, list of rings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3156"/>
        <source>Set the ring used for a given key image, so it can be reused in a fork</source>
        <translation>Imposta l&apos;anello utilizzato per una determinata immagine chiave, in modo che possa essere riutilizzato in un fork</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3164"/>
        <source>Save known rings to the shared rings database</source>
        <translation>Salva gli anelli noti nel database degli anelli condivisi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3168"/>
        <source>Mark output(s) as spent so they never get selected as fake outputs in a ring</source>
        <translation>Contrassegna gli output come spesi in modo che non vengano mai selezionati come output falsi in un anello</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3172"/>
        <source>Marks an output as unspent so it may get selected as a fake output in a ring</source>
        <translation>Contrassegna un output come non speso in modo che possa essere selezionato come un falso output in un anello</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3176"/>
        <source>Checks whether an output is marked as spent</source>
        <translation>Controlla se un output è contrassegnato come speso</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3200"/>
        <source>Returns version information</source>
        <translation>Restituisce informazioni sulla versione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3297"/>
        <source>full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)</source>
        <translation>completo (più lento, nessuna ipotesi); optimize-coinbase (veloce, ipotizza che l&apos;intero coinbase viene pagato ad un indirizzo singolo); no-coinbase (il più veloce, ipotizza di non ricevere una transazione coinbase), default (come optimize-coinbase)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3298"/>
        <source>0, 1, 2, 3, or 4, or one of </source>
        <translation>0, 1, 2, 3 o 4 o uno di </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3300"/>
        <source>0|1|2 (or never|action|decrypt)</source>
        <translation>0|1|2 (o never|action|decrypt)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3301"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation>monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3312"/>
        <source>&lt;major&gt;:&lt;minor&gt;</source>
        <translation>&lt;major&gt;:&lt;minor&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3317"/>
        <source>&lt;device_name[:device_spec]&gt;</source>
        <translation>&lt;device_name[:device_spec]&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3338"/>
        <source>wrong number range, use: %s</source>
        <translation>intervallo di numeri errato, utilizzare: %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3377"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation>Nome del portafoglio non valido. Prova di nuovo o usa Ctrl-C per uscire</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3394"/>
        <source>Wallet and key files found, loading...</source>
        <translation>Portafoglio e chiavi trovate, sto caricando...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3400"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation>Ho trovato la chiave ma non il portafoglio. Sto rigenerando...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3406"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation>Chiave non trovata. Impossibile aprire portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3425"/>
        <source>Generating new wallet...</source>
        <translation>Sto generando un nuovo portafoglio...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3443"/>
        <source>NOTE: the following %s can be used to recover access to your wallet. Write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation>NOTA: il seguente %s può essere utilizzato per recuperare l&apos;accesso al tuo portafoglio. Scrivili e salvali in un posto sicuro e protetto. Si prega di non archiviarli nella propria e-mail o in servizi di archiviazione di file al di fuori del proprio immediato controllo.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3445"/>
        <source>string</source>
        <translation>stringa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3445"/>
        <source>25 words</source>
        <translation>25 parole</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3506"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation>Non è possibile specificare più di un --testnet e --stagenet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3521"/>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-device=&quot;wallet_name&quot;</source>
        <translation>non è possibile specificare più di un --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-device=&quot;wallet_name&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3548"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-deterministic-wallet usa --generate-new-wallet, non --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3600"/>
        <source>Electrum-style word list failed verification</source>
        <translation>La lista di parole stile Electrum ha fallito la verifica</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3605"/>
        <source>Enter seed offset passphrase, empty if none</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3630"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3650"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3686"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3707"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3727"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3742"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3791"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3816"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3832"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3871"/>
        <source>No data supplied, cancelled</source>
        <translation>Nessun dato fornito, cancellato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3636"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3713"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3822"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5770"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6360"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6631"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7218"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7286"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7350"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7554"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8636"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8899"/>
        <source>failed to parse address</source>
        <translation>impossibile fare il parsing dell&apos;indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3656"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3748"/>
        <source>failed to parse view key secret key</source>
        <translation>impossibile fare il parsing chiave di visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3665"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3765"/>
        <source>failed to verify view key secret key</source>
        <translation>impossibile verificare chiave di visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3669"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3769"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3851"/>
        <source>view key does not match standard address</source>
        <translation>la chiave di visualizzazione non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3674"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3695"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3773"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3907"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3934"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3966"/>
        <source>account creation failed</source>
        <translation>creazione dell&apos;account fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3691"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3733"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3876"/>
        <source>failed to parse spend key secret key</source>
        <translation>impossibile fare il parsing chiave di spesa chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3757"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3896"/>
        <source>failed to verify spend key secret key</source>
        <translation>impossibile verificare chiave di spesa chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3761"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3901"/>
        <source>spend key does not match standard address</source>
        <translation>la chiave di spesa non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3866"/>
        <source>Secret spend key (%u of %u)</source>
        <translation>Chiave di spesa segreta (%u di %u)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3941"/>
        <source>No restore height is specified.</source>
        <translation>Nessuna altezza di ripristino specificata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3942"/>
        <source>Assumed you are creating a new account, restore will be done from current estimated blockchain height.</source>
        <translation>Supponendo che si stia creando un nuovo account, il ripristino verrà effettuato dall&apos;attuale altezza stimata sulla blockchain.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3947"/>
        <source>account creation aborted</source>
        <translation>creazione account interrotta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3957"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation>specifica un percorso per il portafoglio con --generate-new-wallet (non --wallet-file)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4069"/>
        <source>can&apos;t specify --subaddress-lookahead and --wallet-file at the same time</source>
        <translation>non è possibile specificare --subaddress-lookahead e --wallet-file nello stesso momento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4073"/>
        <source>failed to open account</source>
        <translation>impossibile aprire account</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4078"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4779"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4832"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4917"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7254"/>
        <source>wallet is null</source>
        <translation>il portafoglio è nullo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4086"/>
        <source>Failed to initialize ring database: privacy enhancing features will be inactive</source>
        <translation>Impossibile inizializzare il database degli anelli: le funzionalità di miglioramento della privacy non saranno attive</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4194"/>
        <source>If your display freezes, exit blind with ^C, then run again with --use-english-language-names</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4212"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4217"/>
        <source>invalid language choice entered. Please try again.
</source>
        <translation>la lingua selezionata non è corretta. Prova di nuovo.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4296"/>
        <source>View key: </source>
        <translation>Chiave di visualizzazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4407"/>
        <source>Generated new wallet on hw device: </source>
        <translation>Generato un nuovo portafoglio sul dispositivo hw: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4486"/>
        <source>Key file not found. Failed to open wallet</source>
        <translation>File della chiave non trovato. Impossibile aprire il portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4563"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation>Potresti voler rimuovere il file &quot;%s&quot; e provare di nuovo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4591"/>
        <source>failed to deinitialize wallet</source>
        <translation>deinizializzazione portafoglio fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4644"/>
        <source>Watch only wallet saved as: </source>
        <translation>Portafoglio solo-lettura salvato come: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4648"/>
        <source>Failed to save watch only wallet: </source>
        <translation>Impossibile salvare il portafoglio in sola lettura: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4770"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5432"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8967"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation>questo comando richiede un daemon fidato. Abilita questa opzione con --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4886"/>
        <source>Expected trusted or untrusted, got </source>
        <translation>Previsto fidato o non fidato, ottenuto </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4903"/>
        <source>trusted</source>
        <translation>fidato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4903"/>
        <source>untrusted</source>
        <translation>non fidato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4927"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation>impossibile salvare la blockchain: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4992"/>
        <source>Password needed (%s) - use the refresh command</source>
        <translation>Password necessaria (%s) - usa il comando refresh</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5000"/>
        <source>Enter password</source>
        <translation>Inserisci la password</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5015"/>
        <source>Device requires attention</source>
        <translation>Il dispositivo richiede attenzione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5023"/>
        <source>Enter device PIN</source>
        <translation>Inserisci il PIN del dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5025"/>
        <source>Failed to read device PIN</source>
        <translation>Impossibile leggere il PIN del dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5032"/>
        <source>Please enter the device passphrase on the device</source>
        <translation>Inserisci la passphrase del dispositivo sul dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5039"/>
        <source>Enter device passphrase</source>
        <translation>Immettere la passphrase del dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5041"/>
        <source>Failed to read device passphrase</source>
        <translation>Impossibile leggere la passphrase del dispositivo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5057"/>
        <source>The first refresh has finished for the HW-based wallet with received money. hw_key_images_sync is needed. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4753"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5059"/>
        <source>Do you want to do it now? (Y/Yes/N/No): </source>
        <translation>Vuoi procedere adesso? (Y/Yes/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5061"/>
        <source>hw_key_images_sync skipped. Run command manually before a transfer.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5123"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5446"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>il daemon è impegnato. Prova più tardi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5127"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5450"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>nessuna connessione con il daemon. Assicurati che sia in funzione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5137"/>
        <source>refresh error: </source>
        <translation>errore refresh: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5185"/>
        <source> (Some owned outputs have missing key images - import_key_images needed)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5194"/>
        <source>Balance: </source>
        <translation>Bilancio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5263"/>
        <source>Invalid keyword: </source>
        <translation>Parola chiave non valida: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5301"/>
        <source>pubkey</source>
        <translation>pubkey</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5301"/>
        <source>key image</source>
        <translation>immagine chiave</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5302"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5318"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>unlocked</source>
        <translation>sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5302"/>
        <source>ringct</source>
        <translation>ringct</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5317"/>
        <source>T</source>
        <translation>T</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5317"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5318"/>
        <source>locked</source>
        <translation>bloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5319"/>
        <source>RingCT</source>
        <translation>RingCT</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5319"/>
        <source>-</source>
        <translation>-</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5398"/>
        <source>payment ID has invalid format, expected 16 or 64 character hex string: </source>
        <translation>l&apos;id pagamento è in un formato invalido, dovrebbe essere una stringa hex di 16 o 64 caratteri</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5454"/>
        <source>failed to get spent status</source>
        <translation>impossibile recuperare status spesi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5538"/>
        <source>failed to find construction data for tx input</source>
        <translation>impossibile trovare i dati di costruzione per l&apos;input tx</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5543"/>
        <source>
Input %llu/%llu (%s): amount=%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5601"/>
        <source>the same transaction</source>
        <translation>la stessa transazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5601"/>
        <source>blocks that are temporally very close</source>
        <translation>i blocchi che sono temporalmente molto vicini</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5709"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6295"/>
        <source>Locked blocks too high, max 1000000 (˜4 yrs)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5818"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6387"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6656"/>
        <source>No payment id is included with this transaction. Is this okay?</source>
        <translation>Nessun ID di pagamento è incluso in questa transazione. Vuoi procedere comunque?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5882"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5892"/>
        <source>Is this okay anyway?</source>
        <translation>Vuoi procedere comunque?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5887"/>
        <source>There is currently a %u block backlog at that fee level. Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6124"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6435"/>
        <source>Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6130"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6441"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6698"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6177"/>
        <source>Discarding %s of unmixable outputs that cannot be spent, which can be undone by &quot;rescan_spent&quot;.  Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6980"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu, %s. %sIs this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8161"/>
        <source>Rescan anyway?</source>
        <translation>Ripetere comunque la scansione?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8654"/>
        <source>Short payment IDs are to be used within an integrated address only</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9457"/>
        <source> (Y/Yes/N/No): </source>
        <translation> (Y/Yes/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9484"/>
        <source>Choose processing:</source>
        <translation>Scegli l&apos;elaborazione:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9493"/>
        <source>Sign tx</source>
        <translation>Firma tx</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9501"/>
        <source>Send the tx for submission to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9505"/>
        <source>Send the tx for signing to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9512"/>
        <source>Submit tx</source>
        <translation>Invia tx</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9515"/>
        <source>unknown</source>
        <translation>sconosciuto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9521"/>
        <source>Choice: </source>
        <translation>Scelta: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9533"/>
        <source>Wrong choice</source>
        <translation>Scelta errata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9540"/>
        <source>Id</source>
        <translation>Id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9540"/>
        <source>I/O</source>
        <translation>I/O</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9540"/>
        <source>Authorized Signer</source>
        <translation>Firmatario autorizzato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9541"/>
        <source>Message Type</source>
        <translation>Tipo di messaggio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9541"/>
        <source>Height</source>
        <translation>Altezza</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9541"/>
        <source>R</source>
        <translation>R</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9541"/>
        <source>Message State</source>
        <translation>Stato del messaggio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9541"/>
        <source>Since</source>
        <translation>Da</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9558"/>
        <source> ago</source>
        <translation> fa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9564"/>
        <source>#</source>
        <translation>#</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9564"/>
        <source>Transport Address</source>
        <translation>Indirizzo di trasporto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9565"/>
        <source>Auto-Config Token</source>
        <translation>Auto-Config Token</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9565"/>
        <source>Monero Address</source>
        <translation>Indirizzo Monero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9569"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9577"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9579"/>
        <source>&lt;not set&gt;</source>
        <translation>&lt;not set&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9620"/>
        <source>Message </source>
        <translation>Messaggio </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9621"/>
        <source>In/out: </source>
        <translation>In/out: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9623"/>
        <source>State: </source>
        <translation>Stato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9623"/>
        <source>%s since %s, %s ago</source>
        <translation>%s da %s, %s fa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9627"/>
        <source>Sent: Never</source>
        <translation>Inviati: mai</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9631"/>
        <source>Sent: %s, %s ago</source>
        <translation>inviato/i: %s, %s fa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9634"/>
        <source>Authorized signer: </source>
        <translation>Firmatario autorizzato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9635"/>
        <source>Content size: </source>
        <translation>Dimensione del contenuto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9635"/>
        <source> bytes</source>
        <translation> bytes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9636"/>
        <source>Content: </source>
        <translation>Contenuto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9636"/>
        <source>(binary data)</source>
        <translation>(dati binari)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9666"/>
        <source>Send these messages now?</source>
        <translation>Vuoi inviare questi messaggi adesso?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9676"/>
        <source>Queued for sending.</source>
        <translation>In coda per l&apos;invio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9696"/>
        <source>Invalid message id</source>
        <translation>ID messaggio non valido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9705"/>
        <source>usage: mms init &lt;required_signers&gt;/&lt;authorized_signers&gt; &lt;own_label&gt; &lt;own_transport_address&gt;</source>
        <translation>utilizzo: mms init &lt;required_signers&gt;/&lt;authorized_signers&gt; &lt;own_label&gt; &lt;own_transport_address&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9711"/>
        <source>The MMS is already initialized. Re-initialize by deleting all signer info and messages?</source>
        <translation>L&apos;MMS è già inizializzato. Reinizializzare eliminando tutte le informazioni e i messaggi del firmatario?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9726"/>
        <source>Error in the number of required signers and/or authorized signers</source>
        <translation>Errore nel numero di firmatari richiesti e/o firmatari autorizzati</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9743"/>
        <source>The MMS is not active.</source>
        <translation>L&apos;MMS non è attivo.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9766"/>
        <source>Invalid signer number </source>
        <translation>Numero del firmatario non valido </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9771"/>
        <source>mms signer [&lt;number&gt; &lt;label&gt; [&lt;transport_address&gt; [&lt;monero_address&gt;]]]</source>
        <translation>mms signer [&lt;number&gt; &lt;label&gt; [&lt;transport_address&gt; [&lt;monero_address&gt;]]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9790"/>
        <source>Invalid Monero address</source>
        <translation>Indirizzo Monero non valido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9797"/>
        <source>Wallet state does not allow changing Monero addresses anymore</source>
        <translation>Lo stato del portafoglio non consente più di modificare gli indirizzi Monero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9809"/>
        <source>Usage: mms list</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9822"/>
        <source>Usage: mms next [sync]</source>
        <translation>Utilizzo: mms next [sync]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9847"/>
        <source>No next step: </source>
        <translation>Nessun passo successivo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9857"/>
        <source>prepare_multisig</source>
        <translation>prepare_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9863"/>
        <source>make_multisig</source>
        <translation>make_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9878"/>
        <source>exchange_multisig_keys</source>
        <translation>exchange_multisig_keys</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9893"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10013"/>
        <source>export_multisig_info</source>
        <translation>export_multisig_info</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9902"/>
        <source>import_multisig_info</source>
        <translation>import_multisig_info</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9915"/>
        <source>sign_multisig</source>
        <translation>sign_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9925"/>
        <source>submit_multisig</source>
        <translation>submit_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9935"/>
        <source>Send tx</source>
        <translation>Invia tx</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9946"/>
        <source>Process signer config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9958"/>
        <source>Replace current signer config with the one displayed above?</source>
        <translation>Sostituire la configurazione attuale del firmatario con quella visualizzata sopra?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9972"/>
        <source>Process auto config data</source>
        <translation>Elabora dati di configurazione automatica</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9986"/>
        <source>Nothing ready to process</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10006"/>
        <source>Usage: mms sync</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10030"/>
        <source>Usage: mms delete (&lt;message_id&gt; | all)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10037"/>
        <source>Delete all messages?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10063"/>
        <source>Usage: mms send [&lt;message_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10080"/>
        <source>Usage: mms receive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10097"/>
        <source>Usage: mms export &lt;message_id&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10109"/>
        <source>Message content saved to: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10113"/>
        <source>Failed to to save message content</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10137"/>
        <source>Usage: mms note [&lt;label&gt; &lt;text&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10144"/>
        <source>No signer found with label </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10166"/>
        <source>Usage: mms show &lt;message_id&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10185"/>
        <source>Usage: mms set &lt;option_name&gt; [&lt;option_value&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10202"/>
        <source>Wrong option value</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10207"/>
        <source>Auto-send is on</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10207"/>
        <source>Auto-send is off</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10212"/>
        <source>Unknown option</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10220"/>
        <source>Usage: mms help [&lt;subcommand&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10236"/>
        <source>Usage: mms send_signer_config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10242"/>
        <source>Signer config not yet complete</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10257"/>
        <source>Usage: mms start_auto_config [&lt;label&gt; &lt;label&gt; ...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10262"/>
        <source>There are signers without a label set. Complete labels before auto-config or specify them as parameters here.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10268"/>
        <source>Auto-config is already running. Cancel and restart?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10292"/>
        <source>Usage: mms stop_auto_config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10295"/>
        <source>Delete any auto-config tokens and stop auto-config?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10308"/>
        <source>Usage: mms auto_config &lt;auto_config_token&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10315"/>
        <source>Invalid auto-config token</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10321"/>
        <source>Auto-config already running. Cancel and restart?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10339"/>
        <source>MMS not available in this wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10363"/>
        <source>The MMS is not active. Activate using the &quot;mms init&quot; command</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10440"/>
        <source>Invalid MMS subcommand</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10445"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10449"/>
        <source>Error in MMS command: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5892"/>
        <source>Failed to check for backlog: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5933"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6423"/>
        <source>
Transaction </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5938"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6428"/>
        <source>Spending from address index %d
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5940"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6430"/>
        <source>WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6601"/>
        <source>failed to parse Payment ID</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2078"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2125"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6624"/>
        <source>failed to parse key image</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6678"/>
        <source>No outputs found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6683"/>
        <source>Multiple transactions are created, which is not supposed to happen</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6688"/>
        <source>The transaction uses multiple or no inputs, which is not supposed to happen</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6765"/>
        <source>missing threshold amount</source>
        <translation>manca la soglia massima dell&apos;ammontare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6770"/>
        <source>invalid amount threshold</source>
        <translation>ammontare soglia invalido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6933"/>
        <source>Change goes to more than one address</source>
        <translation>Il cambiamento va a più di un indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7369"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7479"/>
        <source>Good signature</source>
        <translation>Firma valida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7396"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7481"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7581"/>
        <source>Bad signature</source>
        <translation>Firma invalida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8612"/>
        <source>Standard address: </source>
        <translation>Indirizzo standard: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8617"/>
        <source>failed to parse payment ID or address</source>
        <translation>impossibile fare il parsing di ID pagamento o indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8659"/>
        <source>failed to parse payment ID</source>
        <translation>impossibile fare il parsing di ID pagamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8677"/>
        <source>failed to parse index</source>
        <translation>impossibile fare il parsing dell&apos;indice</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8685"/>
        <source>Address book is empty.</source>
        <translation>La rubrica è vuota.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8691"/>
        <source>Index: </source>
        <translation>Indice: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8692"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8823"/>
        <source>Address: </source>
        <translation>Indirizzo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8693"/>
        <source>Payment ID: </source>
        <translation>ID Pagamento: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8694"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8822"/>
        <source>Description: </source>
        <translation>Descrizione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8852"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation>il portafoglio è di tipo solo-visualizzazione e non può firmare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1313"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8866"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8892"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9124"/>
        <source>failed to read file </source>
        <translation>impossibile leggere il file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3943"/>
        <source>Use --restore-height or --restore-date if you want to restore an already setup account from a specific height.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3945"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4033"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5984"/>
        <source>Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4056"/>
        <source>Still apply restore height?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7358"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7472"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7566"/>
        <source>failed to load signature file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7420"/>
        <source>wallet is watch-only and cannot generate the proof</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7504"/>
        <source>The reserve proof can be generated only by a full wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7559"/>
        <source>Address must not be a subaddress</source>
        <translation type="unfinished">L&apos;indirizzo non può essere un sottoindirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7577"/>
        <source>Good signature -- total: %s, spent: %s, unspent: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7767"/>
        <source>[Double spend seen on the network: this transaction may or may not end up being mined] </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8045"/>
        <source>There is no unspent output in the specified address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8242"/>
        <source> (no daemon)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8244"/>
        <source> (out of sync)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8295"/>
        <source>(Untitled account)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8308"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8326"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8351"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8374"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8520"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8543"/>
        <source>failed to parse index: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8313"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8525"/>
        <source>specify an index between 0 and </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8431"/>
        <source>
Grand total:
  Balance: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8431"/>
        <source>, unlocked balance: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8439"/>
        <source>Untagged accounts:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8445"/>
        <source>Tag %s is unregistered.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8448"/>
        <source>Accounts with tag: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8449"/>
        <source>Tag&apos;s description: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8451"/>
        <source>Account</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8457"/>
        <source> %c%8u %6s %21s %21s %21s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8467"/>
        <source>----------------------------------------------------------------------------------</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8468"/>
        <source>%15s %21s %21s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8491"/>
        <source>Primary address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8491"/>
        <source>(used)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8512"/>
        <source>(Untitled address)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8552"/>
        <source>&lt;index_min&gt; is already out of bound</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8557"/>
        <source>&lt;index_max&gt; exceeds the bound</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8583"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8595"/>
        <source>Integrated addresses can only be created for account 0</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8607"/>
        <source>Integrated address: %s, payment ID: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8612"/>
        <source>Subaddress: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8779"/>
        <source>no description found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8781"/>
        <source>description found: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8821"/>
        <source>Filename: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8826"/>
        <source>Watch only</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8828"/>
        <source>%u/%u multisig%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8830"/>
        <source>Normal</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8831"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9622"/>
        <source>Type: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8857"/>
        <source>This wallet is multisig and cannot sign</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8906"/>
        <source>Bad signature from </source>
        <translation>Firma non valida da </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8910"/>
        <source>Good signature from </source>
        <translation>Firma valida da </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8929"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation>il portafoglio è solo-vista e non può esportare immagini chiave</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1252"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8943"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9091"/>
        <source>failed to save file </source>
        <translation>impossibile salvare file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8954"/>
        <source>Signed key images exported to </source>
        <translation>Chiave immagine firmata esportata in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9102"/>
        <source>Outputs exported to </source>
        <translation>Outputs esportati in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5752"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6805"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7515"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8004"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8012"/>
        <source>amount is wrong: </source>
        <translation>l&apos;ammontare non è corretto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5753"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6805"/>
        <source>expected number from 0 to </source>
        <translation>deve essere un numero da 0 a </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6122"/>
        <source>Sweeping </source>
        <translation>Eseguendo lo sweeping </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6738"/>
        <source>Money successfully sent, transaction: </source>
        <translation>Fondi inviati con successo, transazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6974"/>
        <source>%s change to %s</source>
        <translation>%s cambia in %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6977"/>
        <source>no change</source>
        <translation>nessun cambiamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1459"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1472"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7049"/>
        <source>Transaction successfully signed to file </source>
        <translation>Transazione firmata con successo nel file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7116"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7154"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7211"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7260"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7342"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7427"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7462"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8711"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8739"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9154"/>
        <source>failed to parse txid</source>
        <translation>parsing txid fallito</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7132"/>
        <source>Tx key: </source>
        <translation>Chiave Tx: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7137"/>
        <source>no tx keys found for this txid</source>
        <translation>nessuna chiave tx trovata per questo txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7229"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7441"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7530"/>
        <source>signature file saved to: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7231"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7443"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7532"/>
        <source>failed to save signature file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7268"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7277"/>
        <source>failed to parse tx key</source>
        <translation>impossibile fare il parsing della chiave tx</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7235"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7323"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7401"/>
        <source>error: </source>
        <translation>errore: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7299"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7372"/>
        <source>received</source>
        <translation>ricevuto/i</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7299"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7372"/>
        <source>in txid</source>
        <translation>in txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7318"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7391"/>
        <source>received nothing in txid</source>
        <translation>nulla ricevuto in txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7302"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7375"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation>AVVISO: questa transazione non è ancora inclusa nella blockchain!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7308"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7381"/>
        <source>This transaction has %u confirmations</source>
        <translation>Questa transazione ha %u conferme</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7312"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7385"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation>AVVISO: impossibile determinare il numero di conferme!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7666"/>
        <source>bad min_height parameter:</source>
        <translation>parametro min_height non corretto:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7678"/>
        <source>bad max_height parameter:</source>
        <translation>parametro max_height non corretto:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7696"/>
        <source>in</source>
        <translation>in</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8019"/>
        <source>&lt;min_amount&gt; should be smaller than &lt;max_amount&gt;</source>
        <translation>&lt;min_amount&gt; dovrebbe essere più piccolo di &lt;max_amount&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8051"/>
        <source>
Amount: </source>
        <translation>
Ammontare: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8051"/>
        <source>, number of keys: </source>
        <translation>, numero di chiavi: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8056"/>
        <source> </source>
        <translation> </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8061"/>
        <source>
Min block height: </source>
        <translation>
Altezza minima blocco: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8062"/>
        <source>
Max block height: </source>
        <translation>
Altezza massima blocco: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8063"/>
        <source>
Min amount found: </source>
        <translation>
Ammontare minimo trovato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8064"/>
        <source>
Max amount found: </source>
        <translation>
Ammontare massimo trovato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8065"/>
        <source>
Total count: </source>
        <translation>
Conto totale: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8105"/>
        <source>
Bin size: </source>
        <translation>
Dimensione Bin: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8106"/>
        <source>
Outputs per *: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8108"/>
        <source>count
  ^
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8110"/>
        <source>  |</source>
        <translation>  |</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8112"/>
        <source>  +</source>
        <translation>  +</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8112"/>
        <source>+--&gt; block height
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8113"/>
        <source>   ^</source>
        <translation>   ^</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8113"/>
        <source>^
</source>
        <translation>^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8114"/>
        <source>  </source>
        <translation>  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8240"/>
        <source>wallet</source>
        <translation>portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="893"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8587"/>
        <source>Random payment ID: </source>
        <translation>ID pagamento casuale: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8588"/>
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
        <translation type="unfinished">nota</translation>
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
        <translation type="unfinished">out</translation>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="135"/>
        <source>Generate new wallet and save it to &lt;arg&gt;</source>
        <translation>Genera un nuovo portafoglio e salvalo in &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="136"/>
        <source>Generate new wallet from device and save it to &lt;arg&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="137"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>Genera un portafoglio solo-ricezione da chiave di visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="138"/>
        <source>Generate deterministic wallet from spend key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="139"/>
        <source>Generate wallet from private keys</source>
        <translation>Genera portafoglio da chiavi private</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="140"/>
        <source>Generate a master wallet from multisig wallet keys</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="142"/>
        <source>Language for mnemonic</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="143"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>Specifica il seed stile Electrum per recuperare/creare il portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="144"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Recupera portafoglio usando il seed mnemonico stile-Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="145"/>
        <source>Recover multisig wallet using Electrum-style mnemonic seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="146"/>
        <source>Generate non-deterministic view and spend keys</source>
        <translation>Crea chiavi di visualizzione e chiavi di spesa non-deterministiche</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="149"/>
        <source>Restore from estimated blockchain height on specified date</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="382"/>
        <source>invalid argument: must be either 0/1, true/false, y/n, yes/no</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="438"/>
        <source>DNSSEC validation passed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="442"/>
        <source>WARNING: DNSSEC validation was unsuccessful, this address may not be correct!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="445"/>
        <source>For URL: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="447"/>
        <source> Monero Address = </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="459"/>
        <source>you have cancelled the transfer request</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="480"/>
        <source>failed to parse index: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="493"/>
        <source>invalid format for subaddress lookahead; must be &lt;major&gt;:&lt;minor&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="510"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="515"/>
        <source>RPC error: </source>
        <translation type="unfinished">errore RPC: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="519"/>
        <source>failed to get random outputs to mix: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="526"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="534"/>
        <source>Not enough money in unlocked balance</source>
        <translation type="unfinished">Fondi insufficienti in saldo sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="544"/>
        <source>Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="550"/>
        <source>not enough outputs for specified ring size</source>
        <translation type="unfinished">insufficiente numero di output per il ring size specificato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="553"/>
        <source>output amount</source>
        <translation type="unfinished">ammontare output</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="553"/>
        <source>found outputs to use</source>
        <translation type="unfinished">trovati output che possono essere usati</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="555"/>
        <source>Please use sweep_unmixable.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="559"/>
        <source>transaction was not constructed</source>
        <translation type="unfinished">transazione non costruita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="567"/>
        <source>Reason: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="576"/>
        <source>one of destinations is zero</source>
        <translation type="unfinished">una delle destinazioni è zero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="581"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation type="unfinished">impossibile trovare un modo per dividere le transazioni</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="587"/>
        <source>unknown transfer error: </source>
        <translation type="unfinished">errore trasferimento sconosciuto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="592"/>
        <source>Multisig error: </source>
        <translation type="unfinished">Errore multisig: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="598"/>
        <source>internal error: </source>
        <translation type="unfinished">errore interno: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="603"/>
        <source>unexpected error: </source>
        <translation type="unfinished">errore inaspettato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="607"/>
        <source>There was an error, which could mean the node may be trying to get you to retry creating a transaction, and zero in on which outputs you own. Or it could be a bona fide error. It may be prudent to disconnect from this node, and not try to send a transaction immediately. Alternatively, connect to another node so the original node cannot correlate information.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="617"/>
        <source>File %s likely stores wallet private keys! Use a different file name.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7595"/>
        <source> seconds</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7597"/>
        <source> minutes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7599"/>
        <source> hours</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7601"/>
        <source> days</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7603"/>
        <source> months</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7604"/>
        <source>a long time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9382"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.
WARNING: Do not reuse your Monero keys on another fork, UNLESS this fork has key reuse mitigations built in. Doing so will harm your privacy.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9407"/>
        <source>Unknown command: </source>
        <translation type="unfinished">Comando sconosciuto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="147"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>Permetti comunicazioni con un daemon che usa una versione RPC differente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="148"/>
        <source>Restore from specific blockchain height</source>
        <translation>Ripristina da specifico blocco</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="150"/>
        <source>The newly created transaction will not be relayed to the monero network</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="151"/>
        <source>Create an address file for new wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="153"/>
        <source>Display English language names</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="154"/>
        <source>Support obsolete long (unencrypted) payment ids (using them harms your privacy)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="297"/>
        <source>failed to read wallet password</source>
        <translation type="unfinished">impossibile leggere la password del portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="304"/>
        <source>Enter a new password for the wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="304"/>
        <source>Wallet password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="314"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="506"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>il daemon è occupato. Prova più tardi.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="323"/>
        <source>possibly lost connection to daemon</source>
        <translation>possibile perdita di connessione con il daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="340"/>
        <source>Error: </source>
        <translation>Errore: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="449"/>
        <source>Is this OK?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="564"/>
        <source>transaction %s was rejected by daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="620"/>
        <source>File %s already exists. Are you sure to overwrite it?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9401"/>
        <source>Failed to initialize wallet</source>
        <translation>Inizializzazione wallet fallita</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="234"/>
        <source>Use daemon instance at &lt;host&gt;:&lt;port&gt;</source>
        <translation>Usa instanza daemon in &lt;host&gt;:&lt;port&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="235"/>
        <source>Use daemon instance at host &lt;arg&gt; instead of localhost</source>
        <translation>Usa istanza daemon all&apos;host &lt;arg&gt; invece che localhost</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="240"/>
        <source>Wallet password file</source>
        <translation>File password portafoglio</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="241"/>
        <source>Use daemon instance at port &lt;arg&gt; instead of 18081</source>
        <translation>Usa istanza daemon alla porta &lt;arg&gt; invece che alla 18081</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="250"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>Per testnet. Il daemon può anche essere lanciato con la flag --testnet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="361"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>non puoi specificare la porta o l&apos;host del daemon più di una volta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="480"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>non puoi specificare più di un --password e --password-file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="493"/>
        <source>the password file specified could not be read</source>
        <translation>il file password specificato non può essere letto</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="519"/>
        <source>Failed to load file </source>
        <translation>Impossibile caricare file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="239"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation>Wallet password (escape/quote se necessario)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="236"/>
        <source>[&lt;ip&gt;:]&lt;port&gt; socks proxy to use for daemon connections</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="237"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation type="unfinished">Abilita comandi dipendenti da un daemon fidato</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="238"/>
        <source>Disable commands which rely on a trusted daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="242"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>Specificare username[:password] per client del daemon RPC</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="243"/>
        <source>Enable SSL on daemon RPC connections: enabled|disabled|autodetect</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="247"/>
        <source>List of valid fingerprints of allowed RPC servers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="248"/>
        <source>Allow any SSL certificate from the daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="249"/>
        <source>Allow user (via --daemon-ssl-ca-certificates) chain certificates</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="251"/>
        <source>For stagenet. Daemon must also be launched with --stagenet flag</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="253"/>
        <source>Set shared ring database path</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="264"/>
        <source>Number of rounds for the key derivation function</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="265"/>
        <source>HW device to use</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="266"/>
        <source>HW device wallet derivation path (e.g., SLIP-10)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="268"/>
        <source>Do not use DNS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="269"/>
        <source>Do not connect to a daemon, nor use DNS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="353"/>
        <source>Invalid argument for </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="397"/>
        <source>Enabling --</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="397"/>
        <source> requires --</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="398"/>
        <source> or --</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="398"/>
        <source> or use of a .onion/.i2p domain</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="432"/>
        <source>--trusted-daemon and --untrusted-daemon are both seen, assuming untrusted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="442"/>
        <source>Daemon is local, assuming trusted</source>
        <translation type="unfinished">Il daemon è locale, viene considerato fidato</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="500"/>
        <source>no password specified; use --prompt-for-password to prompt for a password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="502"/>
        <source>Enter a new password for the wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="502"/>
        <source>Wallet password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="525"/>
        <source>Failed to parse JSON</source>
        <translation>Impossibile fare il parsing di JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="532"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation>La versione %u è troppo recente, possiamo comprendere solo fino alla versione %u</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="548"/>
        <source>failed to parse view key secret key</source>
        <translation>impossibile fare il parsing di chiave di visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="553"/>
        <location filename="../src/wallet/wallet2.cpp" line="621"/>
        <location filename="../src/wallet/wallet2.cpp" line="666"/>
        <source>failed to verify view key secret key</source>
        <translation>impossibile verificare chiave di visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="564"/>
        <source>failed to parse spend key secret key</source>
        <translation>impossibile fare il parsing chiave di spesa chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="569"/>
        <location filename="../src/wallet/wallet2.cpp" line="631"/>
        <location filename="../src/wallet/wallet2.cpp" line="692"/>
        <source>failed to verify spend key secret key</source>
        <translation>impossibile verificare chiave di spesa chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="581"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Verifica lista di parole stile-Electrum fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="601"/>
        <source>At least one of either an Electrum-style word list, private view key, or private spend key must be specified</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="605"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Specificate entrambe lista parole stile-Electrum e chiave/i privata/e </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="615"/>
        <source>invalid address</source>
        <translation>indirizzo invalido</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="624"/>
        <source>view key does not match standard address</source>
        <translation>la chiave di visualizzazione non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="634"/>
        <source>spend key does not match standard address</source>
        <translation>la chiave di spesa non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="642"/>
        <source>Cannot generate deprecated wallets from JSON</source>
        <translation>Impossibile creare portafogli disapprovati da JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="678"/>
        <source>failed to parse address: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="684"/>
        <source>Address must be specified in order to create watch-only wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="701"/>
        <source>failed to generate new wallet: </source>
        <translation>impossibile generare nuovo portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="1625"/>
        <source>Password is needed to compute key image for incoming monero</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="1626"/>
        <source>Invalid password: password is needed to compute key image for incoming monero</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="4122"/>
        <location filename="../src/wallet/wallet2.cpp" line="4712"/>
        <location filename="../src/wallet/wallet2.cpp" line="5308"/>
        <source>Primary account</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="10885"/>
        <source>No funds received in this tx.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="11645"/>
        <source>failed to read file </source>
        <translation>lettura file fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="152"/>
        <source>Set subaddress lookahead sizes to &lt;major&gt;:&lt;minor&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="68"/>
        <source>Enable SSL on wallet RPC connections: enabled|disabled|autodetect</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="69"/>
        <location filename="../src/wallet/wallet2.cpp" line="244"/>
        <source>Path to a PEM format private key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="70"/>
        <location filename="../src/wallet/wallet2.cpp" line="245"/>
        <source>Path to a PEM format certificate</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="71"/>
        <location filename="../src/wallet/wallet2.cpp" line="246"/>
        <source>Path to file containing concatenated PEM format certificate(s) to replace system CA(s).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="72"/>
        <source>List of certificate fingerprints to allow</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>tools::wallet_rpc_server</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="192"/>
        <source>Failed to create directory </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="194"/>
        <source>Failed to create directory %s: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="205"/>
        <source>Cannot specify --</source>
        <translation>Impossibile specificare --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="205"/>
        <source> and --</source>
        <translation> e --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="224"/>
        <source>Failed to create file </source>
        <translation>Impossibile creare file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="224"/>
        <source>. Check permissions or remove file</source>
        <translation>. Controlla permessi o rimuovi il file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="234"/>
        <source>Error writing to file </source>
        <translation>Errore durante scrittura su file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="237"/>
        <source>RPC username/password is stored in file </source>
        <translation>Username/password RPC conservato nel file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="613"/>
        <source>Tag %s is unregistered.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3242"/>
        <source>Transaction not possible. Available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4409"/>
        <source>This is the RPC monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4245"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>Non puoi specificare più di un --wallet-file e --generate-from-json</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4230"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished">Non è possibile specificare più di un --testnet e --stagenet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4257"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>Devi specificare --wallet-file o --generate-from-json o --wallet-dir</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4261"/>
        <source>Loading wallet...</source>
        <translation>Sto caricando il portafoglio...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4295"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4327"/>
        <source>Saving wallet...</source>
        <translation>Sto salvando il portafoglio...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4297"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4329"/>
        <source>Successfully saved</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4300"/>
        <source>Successfully loaded</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4304"/>
        <source>Wallet initialization failed: </source>
        <translation>Inizializzazione portafoglio fallita: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4310"/>
        <source>Failed to initialize wallet RPC server</source>
        <translation>Inizializzazione server RPC portafoglio fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4314"/>
        <source>Starting wallet RPC server</source>
        <translation>Server RPC portafoglio in avvio</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4321"/>
        <source>Failed to run wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4324"/>
        <source>Stopped wallet RPC server</source>
        <translation>Server RPC portafoglio arrestato</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4333"/>
        <source>Failed to save wallet: </source>
        <translation>Impossibile salvare portafoglio: </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="168"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4385"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9348"/>
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
        <location filename="../src/wallet/wallet_args.cpp" line="105"/>
        <source>Max number of threads to use for a parallel job</source>
        <translation>Numero massimo di threads da utilizzare per un lavoro parallelo</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="106"/>
        <source>Specify log file</source>
        <translation>Specificare file di log</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="107"/>
        <source>Config file</source>
        <translation>File configurazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="119"/>
        <source>General options</source>
        <translation>Opzioni generali</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="144"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="169"/>
        <source>Can&apos;t find config file </source>
        <translation>Impossibile trovare file configurazione </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="210"/>
        <source>Logging to: </source>
        <translation>Sto salvando il Log in: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="212"/>
        <source>Logging to %s</source>
        <translation>Sto salvando il Log in %s</translation>
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
        <translation>Uso:</translation>
    </message>
</context>
</TS>
