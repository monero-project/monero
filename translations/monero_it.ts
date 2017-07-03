<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="it" sourcelanguage="en">
<context>
    <name>Monero::AddressBookImpl</name>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="55"/>
        <source>Invalid destination address</source>
        <translation>Indirizzo destinatario invalido</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="65"/>
        <source>Invalid payment ID. Short payment ID should only be used in an integrated address</source>
        <translation>ID pagamento invalido. Il pagamento id corto dovrebbe essere usato solo in un indirizzo integrato</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="72"/>
        <source>Invalid payment ID</source>
        <translation>ID pagamento invalido</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="79"/>
        <source>Integrated address and long payment id can&apos;t be used at the same time</source>
        <translation>Indirizzo integrato e ID pagamento lungo non possono essere usati nello stesso momento</translation>
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
        <location filename="../src/wallet/api/pending_transaction.cpp" line="114"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>Il daemon è impegnato. Prova più tardi.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="117"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>Nessuna connessione con il daemon. Controlla che sia operativo.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="121"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>La transazione %s è stata respinta dal daemon con status: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="126"/>
        <source>. Reason: </source>
        <translation>Motivo: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="128"/>
        <source>Unknown exception: </source>
        <translation>Eccezione sconosciuta: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="131"/>
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
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="135"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>Il cambiamento richiesto non porta a un indirizzo pagato</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="141"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>Il cambiamento richiesto è più largo del pagamento all&apos;indirizzo di cambio</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="151"/>
        <source>Change goes to more than one address</source>
        <translation>Il cambiamento ha effetto su più di un in indirizzo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="164"/>
        <source>sending %s to %s</source>
        <translation>Inviando %s a %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="170"/>
        <source>with no destinations</source>
        <translation>senza destinazioni</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="176"/>
        <source>%s change to %s</source>
        <translation>%s cambia in %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="179"/>
        <source>no change</source>
        <translation>nessun cambiamento</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="181"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min mixin %lu. %s</source>
        <translation>Caricato %lu transazioni, per %s, tassa %s, %s, %s, %s, con mixin %lu. %s</translation>
    </message>
</context>
<context>
    <name>Monero::WalletImpl</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="942"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>L&apos;id pagamento è in un formato invalido, dovrebbe essere una stringa hex di 16 o 64 caratteri: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="952"/>
        <source>Failed to add short payment id: </source>
        <translation>Impossibile aggiungere id pagamento corto</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="978"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1072"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>il daemon è impegnato. Prova più tardi</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="981"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1075"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>nessuna connessione con il daemon. Accertati che sia operativo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="984"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1078"/>
        <source>RPC error: </source>
        <translation>errore RPC: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1081"/>
        <source>failed to get random outputs to mix</source>
        <translation>Impossibile raccogliere outputs random da mixare</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="994"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1088"/>
        <source>not enough money to transfer, available only %s, sent amount %s</source>
        <translation>non hai abbastanza soldi da trasferire, sono disponibili solo %s, ammontare inviato %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="403"/>
        <source>failed to parse address</source>
        <translation>Analisi(parse) indirizzo fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="415"/>
        <source>failed to parse secret spend key</source>
        <translation>Impossibile analizzare(parse) la chiave segreta spendibile</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="425"/>
        <source>No view key supplied, cancelled</source>
        <translation>Non è stata fornita nessuna chiave per visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="432"/>
        <source>failed to parse secret view key</source>
        <translation>Impossibile analizzare(parse) la chiave segreta per visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="442"/>
        <source>failed to verify secret spend key</source>
        <translation>impossibile verificare chiave segreta spendibile</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="447"/>
        <source>spend key does not match address</source>
        <translation>la chiave spendibile non corrisponde all&apos;indirizzo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="453"/>
        <source>failed to verify secret view key</source>
        <translation>verifica chiave segreta per visualizzazione fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="458"/>
        <source>view key does not match address</source>
        <translation>La chiave per visualizzazione non corrisponde all&apos;indirizzo</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="477"/>
        <source>failed to generate new wallet: </source>
        <translation>impossibile generare nuovo portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="799"/>
        <source>Failed to load unsigned transactions</source>
        <translation>Caricamento transazioni non firmate fallito</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="820"/>
        <source>Failed to load transaction from file</source>
        <translation>Caricamento transazione da file fallito</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="838"/>
        <source>Wallet is view only</source>
        <translation>Il portafoglio è di tipo solo visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="847"/>
        <source>failed to save file </source>
        <translation>impossibile salvare il file </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="874"/>
        <source>Failed to import key images: </source>
        <translation>Impossibile importare immagini chiave: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="987"/>
        <source>failed to get random outputs to mix: %s</source>
        <translation>impossibile recuperare outputs casuali da mixare: %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1003"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1097"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>Non hai abbastanza ssoldi da trasferire, disponibili solo %s, ammontare transazione %s = %s + %s (tassa)</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1012"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1106"/>
        <source>not enough outputs for specified mixin_count</source>
        <translation>outputs insufficienti per il numero di mixaggi specificato </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1014"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1108"/>
        <source>output amount</source>
        <translation>ammontare output</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1014"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1108"/>
        <source>found outputs to mix</source>
        <translation>trovati outputs da mixare</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1019"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1113"/>
        <source>transaction was not constructed</source>
        <translation>transazione non costruita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1023"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1117"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transazione %s è stata rifiutata dal daemon con status: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1030"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1124"/>
        <source>one of destinations is zero</source>
        <translation>una delle destinazioni è zero</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1033"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1127"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>impossibile trovare un modo per dividere le transazioni</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1036"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1130"/>
        <source>unknown transfer error: </source>
        <translation>errore trasferimento sconosciuto: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1039"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1133"/>
        <source>internal error: </source>
        <translation>errore interno: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1042"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1136"/>
        <source>unexpected error: </source>
        <translation>errore insaspettato: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1045"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1139"/>
        <source>unknown error</source>
        <translation>errore sconosciuto</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1419"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>&quot;riscannerizza spesi&quot; può essere utilizzato solo da un daemon fidato</translation>
    </message>
</context>
<context>
    <name>Monero::WalletManagerImpl</name>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="192"/>
        <source>failed to parse txid</source>
        <translation>analisi(parse) txid fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="199"/>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="206"/>
        <source>failed to parse tx key</source>
        <translation>analisi(parse) chiave tx fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="217"/>
        <source>failed to parse address</source>
        <translation>analisi(parse) indirizzo fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="227"/>
        <source>failed to get transaction from daemon</source>
        <translation>impossibile recuperare transazione da daemon</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="238"/>
        <source>failed to parse transaction from daemon</source>
        <translation>impossibile analizzare(parse) transazione dal daemon</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="245"/>
        <source>failed to validate transaction from daemon</source>
        <translation>convalida transazione da daemon fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="250"/>
        <source>failed to get the right transaction from daemon</source>
        <translation>Impossibile recuperare la giusta transazione dal daemon</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="257"/>
        <source>failed to generate key derivation from supplied parameters</source>
        <translation>impossibile generare derivazione chiave dai parametri forniti</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="313"/>
        <source>error: </source>
        <translation>errore: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="319"/>
        <source>received</source>
        <translation>ricevuto</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="319"/>
        <source>in txid</source>
        <translation>in txid</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="323"/>
        <source>received nothing in txid</source>
        <translation>ricevuto nulla in txid</translation>
    </message>
</context>
<context>
    <name>Wallet</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="212"/>
        <source>Failed to parse address</source>
        <translation>Analisi(parse) indirizzo fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="219"/>
        <source>Failed to parse key</source>
        <translation>Analisi(parse) key fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="227"/>
        <source>failed to verify key</source>
        <translation>verifica key fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="237"/>
        <source>key does not match address</source>
        <translation>la chiave non corrisponde all&apos;indirizzo</translation>
    </message>
</context>
<context>
    <name>command_line</name>
    <message>
        <location filename="../src/common/command_line.cpp" line="76"/>
        <source>yes</source>
        <translation>sì</translation>
    </message>
</context>
<context>
    <name>cryptonote::rpc_args</name>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="38"/>
        <source>Specify ip to bind rpc server</source>
        <translation>Specificare ip da associare al server rpc</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="39"/>
        <source>Specify username[:password] required for RPC server</source>
        <translation>Specifica username[:password] richiesta per server RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="40"/>
        <source>Confirm rpc-bind-ip value is NOT a loopback (local) IP</source>
        <translation>Conferma valore rpc-bind-ip NON è un loopback IP (locale)</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="66"/>
        <source>Invalid IP address given for --</source>
        <translation>Invalido indirizzo IP dato per --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="74"/>
        <source> permits inbound unencrypted external connections. Consider SSH tunnel or SSL proxy instead. Override with --</source>
        <translation>permette connessioni esterne non criptate in entrata. Considera in alternativa un tunnel SSH o un proxy SSL. Sovrascrivi con --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="89"/>
        <source>Username specified with --</source>
        <translation>Nome utente specificato con --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="89"/>
        <source> cannot be empty</source>
        <translation> non puoò essere vuoto</translation>
    </message>
</context>
<context>
    <name>cryptonote::simple_wallet</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="290"/>
        <source>Commands: </source>
        <translation>Comandi: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1557"/>
        <source>failed to read wallet password</source>
        <translation>impossibile leggere password portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1325"/>
        <source>invalid password</source>
        <translation>password non valida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="697"/>
        <source>start_mining [&lt;number_of_threads&gt;] - Start mining in daemon</source>
        <translation>inizia a minare [&lt;number_of_threads&gt;] - Inizia a minare nel daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="698"/>
        <source>Stop mining in daemon</source>
        <translation>interrompi mining nel daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="699"/>
        <source>Save current blockchain data</source>
        <translation>Salva dati correnti blockchain</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="701"/>
        <source>Show current wallet balance</source>
        <translation>Mostra bilancio portafoglio attuale</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="704"/>
        <source>Show blockchain height</source>
        <translation>Mostra &quot;altezza&quot; blockchain</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="715"/>
        <source>Show current wallet public address</source>
        <translation>Mostra indirizzo pubblico del corrente portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="744"/>
        <source>Show this help</source>
        <translation>Mostra questo aiuto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="788"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation>imposta seme: richiede una definizione. opzioni disponibili: lingua</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="811"/>
        <source>set: unrecognized argument(s)</source>
        <translation>imposta: definizione/i non riconosciuta/e</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1442"/>
        <source>wallet file path not valid: </source>
        <translation>percorso file portafoglio non valido: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="863"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation>Sto tentando di generare o ripristinare il portafoglio, ma i(l) file specificato/i esiste/esistono già. Sto uscendo per non rischiare di sovrascrivere.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="416"/>
        <source>usage: payment_id</source>
        <translation>uso: payment_id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="710"/>
        <source>sweep_below &lt;amount_threshold&gt; [mixin] address [payment_id] - Send all unlocked outputs below the threshold to an address</source>
        <translation>sweep_below &lt;amount_threshold&gt; [mixin] address [payment_id] - Invia tutti gli outputs sbloccati sotto la soglia specificata a un indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="727"/>
        <source>Generate a signature to prove payment to &lt;address&gt; in &lt;txid&gt; using the transaction secret key (r) without revealing it</source>
        <translation>Genera una firma per provare un pagamento a &lt;address&gt; in &lt;txid&gt; usando la chiave di transazione (r) senza rivelarla</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="728"/>
        <source>Check tx proof for payment going to &lt;address&gt; in &lt;txid&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="743"/>
        <source>Generate a new random full size payment id - these will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="774"/>
        <source>needs an argument</source>
        <translation>ha bisogno di un argomento</translation>
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
        <translation>0 o 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="800"/>
        <source>integer &gt;= 2</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="803"/>
        <source>0, 1, 2, 3, or 4</source>
        <translation>0, 1, 2, 3, o 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="807"/>
        <source>unsigned integer</source>
        <translation>integrale non firmato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="912"/>
        <source>PLEASE NOTE: the following 25 words can be used to recover access to your wallet. Please write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation>ATTENZIONE: le seguenti 25 parole possono essere usate per ripristinare il tuo portafoglio. Scrivile e conservale da qualche parte al sicuro.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="973"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation>Specificare un parametro di ripristino con --electrum-seed=&quot;lista parole qui&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1261"/>
        <source>wallet failed to connect to daemon: </source>
        <translation>impossibile connettere portafoglio a daemon: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1269"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation>Il daemon usa una versione principale RPC (%u) diversa da quella del portafoglio (%u): %s. Aggiorna una delle due, o usa --allow-mismatched-daemon-version.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1288"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation>Lista delle lingue disponibili per il seme del tuo portafoglio:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1297"/>
        <source>Enter the number corresponding to the language of your choice: </source>
        <translation>Inserisci il numero corrispondente al linguaggio da te scelto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1354"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation>Hai usato una versione obsoleta del portafoglio. Per favore usa il nuovo seme che ti abbiamo fornito.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1368"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1425"/>
        <source>Generated new wallet: </source>
        <translation>Nuovo portafoglio generato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1374"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1430"/>
        <source>failed to generate new wallet: </source>
        <translation>impossibile generare nuovo portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1457"/>
        <source>Opened watch-only wallet</source>
        <translation>Portafoglio solo-vista aperto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1457"/>
        <source>Opened wallet</source>
        <translation>Portafoglio aperto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1466"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation>Stai utilizzando una versione disapprovata del portafoglio. Per favore procedi nell&apos;upgrade del portafoglio.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1481"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation>Stai utilizzando una versione disapprovata del portafoglio. Il formato del tuo portafoglio sta venendo aggiornato adesso.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1489"/>
        <source>failed to load wallet: </source>
        <translation>impossibile caricare portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1497"/>
        <source>Use &quot;help&quot; command to see the list of available commands.
</source>
        <translation>Usa il comando &quot;help&quot; per visualizzare la lista dei comandi disponibili.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1541"/>
        <source>Wallet data saved</source>
        <translation>Dati del portafoglio salvati</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1613"/>
        <source>Mining started in daemon</source>
        <translation>Mining partito nel daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1615"/>
        <source>mining has NOT been started: </source>
        <translation>il mining NON è partito: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1634"/>
        <source>Mining stopped in daemon</source>
        <translation>Mining nel daemon interrotto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1636"/>
        <source>mining has NOT been stopped: </source>
        <translation>Il mining NON è stato interrotto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1655"/>
        <source>Blockchain saved</source>
        <translation>Blockchain salvata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1670"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1687"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1699"/>
        <source>Height </source>
        <translation>Blocco </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1671"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1688"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1700"/>
        <source>transaction </source>
        <translation>transazione </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1672"/>
        <source>received </source>
        <translation>ricevuto/i </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1689"/>
        <source>spent </source>
        <translation>speso/i</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1701"/>
        <source>unsupported transaction format</source>
        <translation>formato transazione non supportato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1718"/>
        <source>Starting refresh...</source>
        <translation>Iniziando refresh...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1731"/>
        <source>Refresh done, blocks received: </source>
        <translation>Refresh finito, blocchi ricevuti: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2186"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2701"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>l&apos;id pagamento ha un formato invalido, dovrebbe essere una stringa hex di 16 o 64 caratteri: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2201"/>
        <source>bad locked_blocks parameter:</source>
        <translation>parametro locked_blocks difettoso:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2228"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2726"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation>una singola transazione non può usare più di un id pagamento: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2237"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2735"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>impossibile impostare id pagamento, anche se è stato decodificado correttamente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2262"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2355"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2533"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2749"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2794"/>
        <source>transaction cancelled.</source>
        <translation>transazione cancellata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2323"/>
        <source>Sending %s.  </source>
        <translation>Inviando %s. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2326"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation>La tua transazione deve essere divisa in %llu transazioni. Una tassa verrà applicata per ogni transazione, per un totale di %s tasse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2332"/>
        <source>The transaction fee is %s</source>
        <translation>la tassa per la transazione è %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2335"/>
        <source>, of which %s is dust from change</source>
        <translation>, della quale %s è polvere dovuta allo scambio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2336"/>
        <source>.</source>
        <translation>.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2336"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation>Un totale di %s in polvere verrà inviato all&apos;indirizzo della polvere</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2341"/>
        <source>.
This transaction will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation>Questa transazione verrà sbloccata al blocco %llu, in approssimativamente %s giorni (supponendo 2 minuti per blocco)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2367"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2544"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2805"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Impossibile scrivere transazione/i su file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2371"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2548"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2809"/>
        <source>Unsigned transaction(s) successfully written to file: </source>
        <translation>Transazioni/e non firmata/e scritte/a con successo su file: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2406"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2583"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2844"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3157"/>
        <source>Not enough money in unlocked balance</source>
        <translation>Non hai abbastanza fondi nel bilancio sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2415"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2592"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2853"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3166"/>
        <source>Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees</source>
        <translation>Impossibile creare transazioni. Questo succede di solito perchè l&apos;ammontare di polvere è così piccolo da non poter pagare le proprie tasse, oppure stai provando a mandare più soldi di quelli che possiedi nel bilancio sbloccato, o non hai aggiunto abbastanza tasse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2435"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2612"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2873"/>
        <source>Reason: </source>
        <translation>Motivo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2447"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2624"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2885"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>Impossibile trovare un modo adatto per dividere le transazioni</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2503"/>
        <source>No unmixable outputs found</source>
        <translation>Nessun output non-mixabile trovato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2709"/>
        <source>No address given</source>
        <translation>Non è stato fornito nessun indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3008"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>Il cambiamento richiesto non porta a un indirizzo pagato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3013"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>Il cambiamento richiesto è più largo del pagamento all&apos;indirizzo di cambio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3035"/>
        <source>sending %s to %s</source>
        <translation>mandando %s a %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3041"/>
        <source>with no destinations</source>
        <translation>senza destinazioni</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3088"/>
        <source>Failed to sign transaction</source>
        <translation>Impossibile firmare transazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3094"/>
        <source>Failed to sign transaction: </source>
        <translation>Impossibile firmare transazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3120"/>
        <source>Failed to load transaction from file</source>
        <translation>caricamento transazione da file fallito</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3137"/>
        <source>daemon is busy. Please try later</source>
        <translation>il daemon è occupato. Prova più tardi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1745"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1995"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2395"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2572"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2833"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3146"/>
        <source>RPC error: </source>
        <translation>errore RPC: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="312"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation>il portafoglio è solo-vista e non ha una chiave spendibile</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="390"/>
        <source>Your original password was incorrect.</source>
        <translation>La tua password originale era scorretta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="404"/>
        <source>Error with wallet rewrite: </source>
        <translation>Errore riscrittura wallet: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="513"/>
        <source>priority must be 0, 1, 2, 3, or 4 </source>
        <translation>la priorità deve essere 0, 1, 2, 3, or 4 </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="525"/>
        <source>priority must be 0, 1, 2, 3,or 4</source>
        <translation>la priorità deve essere 0, 1, 2, 3,or 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="540"/>
        <source>priority must be 0, 1, 2 3,or 4</source>
        <translation>la priorità deve essere 0, 1, 2 3, or 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="623"/>
        <source>invalid unit</source>
        <translation>unità invalida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="641"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation>conteggio invalido: deve essere un integrale non firmato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="659"/>
        <source>invalid value</source>
        <translation>valore invalido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="705"/>
        <source>Same as transfer, but using an older transaction building algorithm</source>
        <translation>Lo stesso di transfer, ma usando un vecchio algoritmo per costruire la transazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="709"/>
        <source>sweep_all [mixin] address [payment_id] - Send all unlocked balance to an address</source>
        <translation>sweep_all [mixin] address [payment_id] - Manda tutto il bilancio sbloccato ad un indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="711"/>
        <source>donate [&lt;mixin_count&gt;] &lt;amount&gt; [payment_id] - Donate &lt;amount&gt; to the development team (donate.getmonero.org)</source>
        <translation>donate [&lt;mixin_count&gt;] &lt;amount&gt; [payment_id] - Dona &lt;amount&gt; al team di sviluppo (donate.getmonero.org)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="714"/>
        <source>set_log &lt;level&gt;|&lt;categories&gt; - Change current log detail (level must be &lt;0-4&gt;)</source>
        <translation>set_log &lt;level&gt;|&lt;categories&gt; - Cambia livello dettaglio log (il livello deve essere &lt;0-4&gt;)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="717"/>
        <source>address_book [(add (&lt;address&gt; [pid &lt;long or short payment id&gt;])|&lt;integrated address&gt; [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)] - Print all entries in the address book, optionally adding/deleting an entry to/from it</source>
        <translation>address_book [(add (&lt;address&gt; [pid &lt;long or short payment id&gt;])|&lt;integrated address&gt; [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)] - Mostra tutte le voci nella rubrica, hai l&apos;opzione di aggiungere/eliminare delle voci</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="729"/>
        <source>show_transfers [in|out|pending|failed|pool] [&lt;min_height&gt; [&lt;max_height&gt;]] - Show incoming/outgoing transfers within an optional height range</source>
        <translation>show_transfers [in|out|pending|failed|pool] [&lt;min_height&gt; [&lt;max_height&gt;]] - Mostra trasferimenti in entrata/uscita entro un altezza del blocco opzionale</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="741"/>
        <source>Show information about a transfer to/from this address</source>
        <translation>Mostra informazioni riguardo un trasferimento da/per questo indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="742"/>
        <source>Change wallet password</source>
        <translation>Cambia password portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="820"/>
        <source>usage: set_log &lt;log_level_number_0-4&gt; | &lt;categories&gt;</source>
        <translation>uso: set_log &lt;log_level_number_0-4&gt; | &lt;categories&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="886"/>
        <source>(Y/Yes/N/No): </source>
        <translation>(S/Sì/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1157"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1184"/>
        <source>bad m_restore_height parameter: </source>
        <translation>parametro m_restore_height scorretto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1162"/>
        <source>date format must be YYYY-MM-DD</source>
        <translation>il formato della data deve essere YYYY-MM-DD</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1175"/>
        <source>Restore height is: </source>
        <translation>ripristina altezza è: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1176"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2348"/>
        <source>Is this okay?  (Y/Yes/N/No): </source>
        <translation>va bene? (S/Sì/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1212"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Il daemon è locale, assunto per fidato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1553"/>
        <source>Password for new watch-only wallet</source>
        <translation>Password per il nuovo portafoglio solo-vista</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1604"/>
        <source>invalid arguments. Please use start_mining [&lt;number_of_threads&gt;] [do_bg_mining] [ignore_battery], &lt;number_of_threads&gt; should be from 1 to </source>
        <translation>argomenti invalidi. Usa start_mining [&lt;number_of_threads&gt;] [do_bg_mining] [ignore_battery], &lt;number_of_threads&gt; dovrebbe risultare da 1 a </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1755"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2457"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2634"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2895"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3205"/>
        <source>internal error: </source>
        <translation>errore interno: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1760"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2000"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2462"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2639"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2900"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3210"/>
        <source>unexpected error: </source>
        <translation>errore inaspettato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1765"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2005"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2467"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2644"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2905"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3215"/>
        <source>unknown error</source>
        <translation>errore sconosciuto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1770"/>
        <source>refresh failed: </source>
        <translation>refresh fallito: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1770"/>
        <source>Blocks received: </source>
        <translation>Blocchi ricevuti: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1795"/>
        <source>unlocked balance: </source>
        <translation>bilancio sbloccato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="808"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>amount</source>
        <translation>ammontare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <source>spent</source>
        <translation>spesi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <source>global index</source>
        <translation>indice globale</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <source>tx id</source>
        <translation>tx id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1868"/>
        <source>No incoming transfers</source>
        <translation>nessun trasferimento in entrata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1872"/>
        <source>No incoming available transfers</source>
        <translation>nessun trasferimento in entrata disponibile</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1876"/>
        <source>No incoming unavailable transfers</source>
        <translation>Nessun trasferimento indisponibile in entrata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1887"/>
        <source>expected at least one payment_id</source>
        <translation>deve esserci almeno un payment_id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>payment</source>
        <translation>pagamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>transaction</source>
        <translation>transazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>height</source>
        <translation>blocco</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>unlock time</source>
        <translation>tempo sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1908"/>
        <source>No payments with id </source>
        <translation>nessun pagamento con id </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1960"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2026"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2280"/>
        <source>failed to get blockchain height: </source>
        <translation>impossibile recuperare dalla </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2016"/>
        <source>failed to connect to the daemon</source>
        <translation>impossibile connettersi al daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2034"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation>Transazione %llu/%llu: txid=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2044"/>
        <source>
Input %llu/%llu: amount=%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2060"/>
        <source>failed to get output: </source>
        <translation>impossibile recuperare output: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2068"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2072"/>
        <source>
Originating block heights: </source>
        <translation>Originando blocchi: </translation>
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
        <translation>Avvertimento: alcune chiavi di input spese vengono da </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2106"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation>, che potrebbe rempere l&apos;anonimità delle firme ad anello. Assicurati di farlo intenzionalmente!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2152"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2937"/>
        <source>wrong number of arguments</source>
        <translation>errato numero di argomenti</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2257"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2744"/>
        <source>No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): </source>
        <translation>Nessun id pagamento incluso in questa transazione. Questo è corretto? (S/Sì/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2298"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2762"/>
        <source>No outputs found, or daemon is not ready</source>
        <translation>Nessun output trovato, o il daemon non è pronto</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2399"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2576"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2837"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3150"/>
        <source>failed to get random outputs to mix: </source>
        <translation>impossibile recuperare output casuali da mixare: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2518"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2779"/>
        <source>Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Eseguendo lo sweep di %s nelle transazioni %llu per un totale di tasse di %s.  Va bene?  (S/Sì/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2524"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Eseguendo lo sweep di %s per un totale di tasse di %s.  Va bene?  (S/Sì/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2969"/>
        <source>Donating </source>
        <translation>Donando </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3053"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min mixin %lu. %sIs this okay? (Y/Yes/N/No): </source>
        <translation>Caricate %lu transazioni, per %s, tasse %s, %s, %s, con mixaggio %lu. %sQuesto è corretto? (S/Sì/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3077"/>
        <source>This is a watch only wallet</source>
        <translation>questo è un portafoglio solo-vista</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4443"/>
        <source>usage: show_transfer &lt;txid&gt;</source>
        <translation>uso: show_transfer &lt;txid&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4557"/>
        <source>Transaction ID not found</source>
        <translation>ID transazione non trovato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="237"/>
        <source>true</source>
        <translation>vero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="266"/>
        <source>failed to parse refresh type</source>
        <translation>impossibile analizzare (parse) tipo di refresh</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="330"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="362"/>
        <source>wallet is watch-only and has no seed</source>
        <translation>il portafoglio è solo-vista e non possiede un seme</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="353"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="367"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation>il portafoglio è non-deterministico e non possiede un seme</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="450"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="467"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation>il portafoglio è solo-vista e non può eseguire trasferimenti</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="474"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="480"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="496"/>
        <source>mixin must be an integer &gt;= 2</source>
        <translation>mixin deve essere un integrale &gt;= 2</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="501"/>
        <source>could not change default mixin</source>
        <translation>impossibile cambiare mixxaggio standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="545"/>
        <source>could not change default priority</source>
        <translation>impossibile cambiare priorità standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="700"/>
        <source>Synchronize transactions and balance</source>
        <translation>Sincronizzare transazioni e bilancio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="702"/>
        <source>incoming_transfers [available|unavailable] - Show incoming transfers, all or filtered by availability</source>
        <translation>incoming_transfers [available|unavailable] - Mostra trasferimenti in entrata, tutti o filtrati per disponibilità</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="703"/>
        <source>payments &lt;PID_1&gt; [&lt;PID_2&gt; ... &lt;PID_N&gt;] - Show payments for given payment ID[s]</source>
        <translation>payments &lt;PID_1&gt; [&lt;PID_2&gt; ... &lt;PID_N&gt;] - Mostra pagamenti per gli/l&apos; ID fornito/i</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="706"/>
        <source>transfer [&lt;priority&gt;] [&lt;mixin_count&gt;] &lt;address&gt; &lt;amount&gt; [&lt;payment_id&gt;] - Transfer &lt;amount&gt; to &lt;address&gt;. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the fee of the transaction. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;mixin_count&gt; is the number of extra inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>transfer [&lt;priority&gt;] [&lt;mixin_count&gt;] &lt;address&gt; &lt;amount&gt; [&lt;payment_id&gt;] - Transfer &lt;amount&gt; to &lt;address&gt;. &lt;priority&gt; è la priorità della transazione. Maggiore è la priorità, maggiori saranno le tasse per la transazione. Valori validi in ordini di priorità (dal più basso al più alto) sono:unimportant, normal, elevated, priority. se omesso, verrà usato il valore standard (vedi il comando &quot;set priority&quot;). &lt;mixin_count&gt; è il numero di inputs extra da inludere per intracciabilità. Puoi eseguire pagamenti multipli in una volta aggiungendo &lt;address_2&gt; &lt;amount_2&gt; etcetera (prima dell&apos; ID pagamento, se incluso)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="707"/>
        <source>locked_transfer [&lt;mixin_count&gt;] &lt;addr&gt; &lt;amount&gt; &lt;lockblocks&gt;(Number of blocks to lock the transaction for, max 1000000) [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="708"/>
        <source>Send all unmixable outputs to yourself with mixin 0</source>
        <translation>Manda tutti gli outputs non spendibili a te stesso con mixaggio 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="712"/>
        <source>Sign a transaction from a file</source>
        <translation>Firma una transazione da file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="713"/>
        <source>Submit a signed transaction from a file</source>
        <translation>Invia una transazione dirmata da file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="716"/>
        <source>integrated_address [PID] - Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="718"/>
        <source>Save wallet data</source>
        <translation>Salva i dati del portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="719"/>
        <source>Save a watch-only keys file</source>
        <translation>Salva una chiave solo-vista</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="720"/>
        <source>Display private view key</source>
        <translation>Visualizza chiave di visualizzazione privata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="721"/>
        <source>Display private spend key</source>
        <translation>Visualizza chiave privata spendibile</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="722"/>
        <source>Display Electrum-style mnemonic seed</source>
        <translation>Visualizza il seme mnemonico in stile Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="723"/>
        <source>Available options: seed language - set wallet seed language; always-confirm-transfers &lt;1|0&gt; - whether to confirm unsplit txes; print-ring-members &lt;1|0&gt; - whether to print detailed information about ring members during confirmation; store-tx-info &lt;1|0&gt; - whether to store outgoing tx info (destination address, payment ID, tx secret key) for future reference; default-mixin &lt;n&gt; - set default mixin (default is 4); auto-refresh &lt;1|0&gt; - whether to automatically sync new blocks from the daemon; refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt; - set wallet refresh behaviour; priority [0|1|2|3|4] - default/unimportant/normal/elevated/priority fee; confirm-missing-payment-id &lt;1|0&gt;; ask-password &lt;1|0&gt;; unit &lt;monero|millinero|micronero|nanonero|piconero&gt; - set default monero (sub-)unit; min-outputs-count [n] - try to keep at least that many outputs of value at least min-outputs-value; min-outputs-value [n] - try to keep at least min-outputs-count outputs of at least that value; merge-destinations &lt;1|0&gt; - whether to merge multiple payments to the same destination address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="724"/>
        <source>Rescan blockchain for spent outputs</source>
        <translation>Riscannerizza blockchain in cerca di outputs non spesi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="725"/>
        <source>Get transaction key (r) for a given &lt;txid&gt;</source>
        <translation>Ricevi una chiave transazione (r) per ogni &lt;txid&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="726"/>
        <source>Check amount going to &lt;address&gt; in &lt;txid&gt;</source>
        <translation>Controlla l&apos;ammontare che va da &lt;address&gt; a &lt;txid&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="730"/>
        <source>unspent_outputs [&lt;min_amount&gt; &lt;max_amount&gt;] - Show unspent outputs within an optional amount range</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="731"/>
        <source>Rescan blockchain from scratch</source>
        <translation>Riscannerizza blockchain dal principio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="732"/>
        <source>Set an arbitrary string note for a txid</source>
        <translation>Imposta una stringa arbitraria per un txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="733"/>
        <source>Get a string note for a txid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="734"/>
        <source>Show wallet status information</source>
        <translation>Visualizza informazioni sulla stato del portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="735"/>
        <source>Sign the contents of a file</source>
        <translation>Firma il contenuto di un file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="736"/>
        <source>Verify a signature on the contents of a file</source>
        <translation>Verifica firma sul contesto del file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="737"/>
        <source>Export a signed set of key images</source>
        <translation>Esporta un set di chiavi firmate</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="738"/>
        <source>Import signed key images list and verify their spent status</source>
        <translation>Importa immagine firmata della lista chiavi e verifica lo status degli spesi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="739"/>
        <source>Export a set of outputs owned by this wallet</source>
        <translation>Esporta un set di outputs posseduti da questo portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="740"/>
        <source>Import set of outputs owned by this wallet</source>
        <translation>Importa un set di outputs posseduti da questo portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="802"/>
        <source>full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="806"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="851"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation>Nome del portafoglio non valido. Prova di nuovo o usa Ctrl-C per uscire</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="868"/>
        <source>Wallet and key files found, loading...</source>
        <translation>Portafoglio e chiavi trovatr, sto caricando...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="874"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation>Ho trovato la chiave ma non il portafoglio. Rigenerando...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="880"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation>Chiave non trovata. Impossibile aprire portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="894"/>
        <source>Generating new wallet...</source>
        <translation>Sto generando un nuovo portafoglio...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="937"/>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-keys=&quot;wallet_name&quot;</source>
        <translation>non puoi specificare più di un --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; e --generate-from-keys=&quot;wallet_name&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="953"/>
        <source>can&apos;t specify both --restore-deterministic-wallet and --non-deterministic</source>
        <translation>impossibile specificare sia --restore-deterministic-wallet che --non-deterministic</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="958"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-deterministic-wallet usa --generate-new-wallet, non --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="982"/>
        <source>Electrum-style word list failed verification</source>
        <translation>La lista di parole stile Electrum ha fallito la verifica</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="994"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1011"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1046"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1063"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1079"/>
        <source>No data supplied, cancelled</source>
        <translation>Nessun dato fornito, cancellato</translation>
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
        <translation>impossibile analizzare(parse) indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1017"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1085"/>
        <source>failed to parse view key secret key</source>
        <translation>impossibile analizzare(parse) chiave per visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1027"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1103"/>
        <source>failed to verify view key secret key</source>
        <translation>impossibile verificare chiave segreta vista</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1031"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1107"/>
        <source>view key does not match standard address</source>
        <translation>la chiave per visualizzazione non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1036"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1111"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1128"/>
        <source>account creation failed</source>
        <translation>creazione dell&apos;account fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1069"/>
        <source>failed to parse spend key secret key</source>
        <translation>impossibile analizzare (parse) chiave spendibile chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1095"/>
        <source>failed to verify spend key secret key</source>
        <translation>impossibile verificare chiave spendibile chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1099"/>
        <source>spend key does not match standard address</source>
        <translation>la chiave spendibile non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1123"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation>specifica un nuovo percorso per il portafoglio con --generate-new-wallet (non --wallet-file)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1199"/>
        <source>failed to open account</source>
        <translation>impossibile aprire account</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1203"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1579"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1626"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1647"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3348"/>
        <source>wallet is null</source>
        <translation>il portafoglio è nullo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1262"/>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or restart the wallet with the correct daemon address.</source>
        <translation>Il daemon non è partito o è settato con la porta sbagliata. Assicurati che il daemon stia funzionando o fai ripartire il wallet con l&apos;indirizzo corretto del daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1306"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1311"/>
        <source>invalid language choice passed. Please try again.
</source>
        <translation>linguaggio selezionato scorretto. Prova di nuovo.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1370"/>
        <source>View key: </source>
        <translation>Chiave per visualizzazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1385"/>
        <source>Your wallet has been generated!
To start synchronizing with the daemon, use &quot;refresh&quot; command.
Use &quot;help&quot; command to see the list of available commands.
Always use &quot;exit&quot; command when closing monero-wallet-cli to save your
current session&apos;s state. Otherwise, you might need to synchronize 
your wallet again (your wallet keys are NOT at risk in any case).
</source>
        <translation>Il tuo portafoglio è stato generato!
Per iniziare a sincronizzarlo con il daemon, usa il comando &quot;refresh&quot;.
Usa il comando &quot;help&quot; per vedere la lista dei comandi disponibili.
Usa sempre il comando &quot;exit&quot; quando chiudi monero-wallet-cli per salvare
lo stato della tua sessione &apos;s corrente. Altrimenti potresti dover sincronizzare 
di nuovo il tuo portafoglio (le chiavi del tuo portafoglio NON sono a rischio in sessun caso).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1492"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation>Potresti voler rimuovere il file &quot;%s&quot; e provare di nuovo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1518"/>
        <source>failed to deinitialize wallet</source>
        <translation>deinizializzazione portafoglio fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1570"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1968"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation>questo comando richiede un daemon fidato. Abilita questa opzione con --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1657"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation>impossibile salvare blockchain: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1736"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1982"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2386"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2563"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2824"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>il daemon è impegnato. Prova più tardi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1740"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1986"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2390"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2567"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2828"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>nessuna connessione con il daemon. Assicurati che sia in funzione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1750"/>
        <source>refresh error: </source>
        <translation>refresh errore: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1794"/>
        <source>Balance: </source>
        <translation>Bilancio: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1845"/>
        <source>pubkey</source>
        <translation>pubkey</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1845"/>
        <source>key image</source>
        <translation>immagine chiave</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1856"/>
        <source>unlocked</source>
        <translation>sbloccato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <source>ringct</source>
        <translation>ringct</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1855"/>
        <source>T</source>
        <translation>T</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1855"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1856"/>
        <source>locked</source>
        <translation>bloccato</translation>
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
        <translation>l&apos;id pagamento è in un formato invalido, dovrebbe essere una stringa hex di 16 o 64 caratteri</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1990"/>
        <source>failed to get spent status</source>
        <translation>impossibile recuperare status spesi</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2105"/>
        <source>the same transaction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2105"/>
        <source>blocks that are temporally very close</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2206"/>
        <source>Locked blocks too high, max 1000000 (˜4 yrs)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2921"/>
        <source>missing amount threshold</source>
        <translation>manca la soglia massima dell&apos;ammontare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2926"/>
        <source>invalid amount threshold</source>
        <translation>ammontare soglia invalido</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3022"/>
        <source>Change goes to more than one address</source>
        <translation>Il cambiamento va a più di un indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3258"/>
        <source>usage: get_tx_proof &lt;txid&gt; &lt;dest_address&gt; [&lt;tx_key&gt;]</source>
        <translation>uso: get_tx_proof &lt;txid&gt; &lt;dest_address&gt; [&lt;tx_key&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3289"/>
        <source>failed to parse tx_key</source>
        <translation>impossibile analizzare (parse) tx_key</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3298"/>
        <source>Tx secret key was found for the given txid, but you&apos;ve also provided another tx secret key which doesn&apos;t match the found one.</source>
        <translation>Una chiave tx segreta è stata trovata per il txid fornito, ma hai anche fornito un&apos;altra chiave segreta tx che non corrsisponde a quella trovata.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3306"/>
        <source>Tx secret key wasn&apos;t found in the wallet file. Provide it as the optional third parameter if you have it elsewhere.</source>
        <translation>Chiave segreta tx non trovata nel file del portafoglio. Forniscila come terzo parametro opzionale se la conservi da qualche altra parte.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3330"/>
        <source>Signature: </source>
        <translation>Firma: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3508"/>
        <source>usage: check_tx_proof &lt;txid&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation>uso: check_tx_proof &lt;txid&gt; &lt;address&gt; &lt;signature&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3539"/>
        <source>Signature header check error</source>
        <translation>errore controllo firma intestazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3550"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3555"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3560"/>
        <source>Signature decoding error</source>
        <translation>Errore decodificazione firma</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3602"/>
        <source>Tx pubkey was not found</source>
        <translation>Chiave pubblica tx non trovata</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3609"/>
        <source>Good signature</source>
        <translation>Firma valida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3613"/>
        <source>Bad signature</source>
        <translation>Firma invalida</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3621"/>
        <source>failed to generate key derivation</source>
        <translation>generazione derivazione chiave fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3994"/>
        <source>usage: integrated_address [payment ID]</source>
        <translation>uso: integrated_address [ID pagamento]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4017"/>
        <source>Integrated address: account %s, payment ID %s</source>
        <translation>Indirizzo integrato: account %s, ID pagamento %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4022"/>
        <source>Standard address: </source>
        <translation>Indirizzo standard: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4027"/>
        <source>failed to parse payment ID or address</source>
        <translation>impossibile analizzare (parse) ID pagamento o indirizzo</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4038"/>
        <source>usage: address_book [(add (&lt;address&gt; [pid &lt;long or short payment id&gt;])|&lt;integrated address&gt; [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</source>
        <translation>uso: address_book [(add (&lt;address&gt; [pid &lt;long or short payment id&gt;])|&lt;integrated address&gt; [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4070"/>
        <source>failed to parse payment ID</source>
        <translation>impossibile analizzare (parse) ID pagamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4088"/>
        <source>failed to parse index</source>
        <translation>impossibile analizzare (parse) indice</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4096"/>
        <source>Address book is empty.</source>
        <translation>La rubrica è vuota.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4102"/>
        <source>Index: </source>
        <translation>Indice: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4103"/>
        <source>Address: </source>
        <translation>Indirizzo: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4104"/>
        <source>Payment ID: </source>
        <translation>ID Pagamento: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4105"/>
        <source>Description: </source>
        <translation>Descrizione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4115"/>
        <source>usage: set_tx_note [txid] free text note</source>
        <translation>uso: set_tx_note [txid] free text note</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4143"/>
        <source>usage: get_tx_note [txid]</source>
        <translation>uso: get_tx_note [txid]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4193"/>
        <source>usage: sign &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4198"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation>il portafoglio è solo-vista e non può firmare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4207"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4230"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4374"/>
        <source>failed to read file </source>
        <translation>impossibile leggere file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4219"/>
        <source>usage: verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation>uso: verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4246"/>
        <source>Bad signature from </source>
        <translation>Firma invalida da </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4250"/>
        <source>Good signature from </source>
        <translation>Firma valida da </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4259"/>
        <source>usage: export_key_images &lt;filename&gt;</source>
        <translation>uso: export_key_images &lt;filename&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4264"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation>il portafoglio è solo-vista e non può esportare immagini chiave</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4274"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4346"/>
        <source>failed to save file </source>
        <translation>impossibile salvare file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4285"/>
        <source>Signed key images exported to </source>
        <translation>Chiave immagine firmata esportata in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4293"/>
        <source>usage: import_key_images &lt;filename&gt;</source>
        <translation>uso: import_key_images &lt;filename&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4323"/>
        <source>usage: export_outputs &lt;filename&gt;</source>
        <translation>usage: export_outputs &lt;filename&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4357"/>
        <source>Outputs exported to </source>
        <translation>Outputs esportati in </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4365"/>
        <source>usage: import_outputs &lt;filename&gt;</source>
        <translation>uso: import_outputs &lt;filename&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2246"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3818"/>
        <source>amount is wrong: </source>
        <translation>l&apos;ammontare è scorretto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2247"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3819"/>
        <source>expected number from 0 to </source>
        <translation>deve essere un numero da 0 a </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2378"/>
        <source>Money successfully sent, transaction </source>
        <translation>Fondi inviati con successo, transazione </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3141"/>
        <source>no connection to daemon. Please, make sure daemon is running.</source>
        <translation>nessuna connessione con il daemon, assicurati che stia funzionando.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2420"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2597"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2858"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3171"/>
        <source>not enough outputs for specified mixin_count</source>
        <translation>non ci sono abbastanza output per lo specificato mixin_count</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2423"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2600"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2861"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3174"/>
        <source>output amount</source>
        <translation>ammontare output</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2423"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2600"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2861"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3174"/>
        <source>found outputs to mix</source>
        <translation>trovati outputs da mixare</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2428"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2605"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2866"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3179"/>
        <source>transaction was not constructed</source>
        <translation>la transazione non è stata costruita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2432"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2609"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2870"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3183"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transazione %s è stata respinta dal daemon con status: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2443"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2620"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2881"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3191"/>
        <source>one of destinations is zero</source>
        <translation>una delle destinazioni è zero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3195"/>
        <source>Failed to find a suitable way to split transactions</source>
        <translation>Impossibile trovare un modo corretto per dividere transazioni</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2452"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2629"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2890"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3200"/>
        <source>unknown transfer error: </source>
        <translation>errore trasferimento sconosciuto: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2516"/>
        <source>Sweeping </source>
        <translation>Pulendo (sweeping) </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2785"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No)</source>
        <translation>Eseguendo lo sweeping di %s per un totale di tasse di %s. Va bene? (S/Sì/N/No)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2555"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2816"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3129"/>
        <source>Money successfully sent, transaction: </source>
        <translation>Soldi inviati con successo, transazione: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3047"/>
        <source>%s change to %s</source>
        <translation>%s cambia in %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3050"/>
        <source>no change</source>
        <translation>nessun cambiamento</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3105"/>
        <source>Transaction successfully signed to file </source>
        <translation>Transazione firmata con successo nel file </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3226"/>
        <source>usage: get_tx_key &lt;txid&gt;</source>
        <translation>uso: get_tx_key &lt;txid&gt;</translation>
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
        <translation>analisi(parse) txid fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3245"/>
        <source>Tx key: </source>
        <translation>Chiave Tx: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3250"/>
        <source>no tx keys found for this txid</source>
        <translation>nessuna chiave tx trovata per questo txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3339"/>
        <source>usage: check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</source>
        <translation>uso: check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3361"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3368"/>
        <source>failed to parse tx key</source>
        <translation>impossibile analizzare (parse) chiave tx</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3400"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3573"/>
        <source>failed to get transaction from daemon</source>
        <translation>impossibil recuperare transazione dal daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3411"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3584"/>
        <source>failed to parse transaction from daemon</source>
        <translation>impossibile analizzare (parse) la transazione dal daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3418"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3591"/>
        <source>failed to validate transaction from daemon</source>
        <translation>convalida transazione dal daemon fallita</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3423"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3596"/>
        <source>failed to get the right transaction from daemon</source>
        <translation>impossibile recuperare la corretta transazione dal daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3385"/>
        <source>failed to generate key derivation from supplied parameters</source>
        <translation>impossibile generare chiave derivazione dai parametri forniti</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3471"/>
        <source>error: </source>
        <translation>errore: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3477"/>
        <source>received</source>
        <translation>ricevuto/i</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3477"/>
        <source>in txid</source>
        <translation>in txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3481"/>
        <source>received nothing in txid</source>
        <translation>ricevuto niente in txid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3485"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation>AVVERTIMENTO: questa transazione non è ancora inclusa nella blockchain!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3494"/>
        <source>This transaction has %u confirmations</source>
        <translation>Questa transazione ha %u conferme</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3498"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation>AVVERTIMENTO: impossibile determinare numero di conferme!</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3661"/>
        <source>usage: show_transfers [in|out|all|pending|failed] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation>uso: show_transfers [in|out|all|pending|failed] [&lt;min_height&gt; [&lt;max_height&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3700"/>
        <source>bad min_height parameter:</source>
        <translation>parametro min_height scorretto:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3712"/>
        <source>bad max_height parameter:</source>
        <translation>parametro max_height scorretto:</translation>
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
        <translation>out</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3798"/>
        <source>failed</source>
        <translation>fallito</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3798"/>
        <source>pending</source>
        <translation>in attesa</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3809"/>
        <source>usage: unspent_outputs [&lt;min_amount&gt; &lt;max_amount&gt;]</source>
        <translation>uso: unspent_outputs [&lt;min_amount&gt; &lt;max_amount&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3824"/>
        <source>&lt;min_amount&gt; should be smaller than &lt;max_amount&gt;</source>
        <translation>&lt;min_amount&gt; dovrebbe essere più piccolo di &lt;max_amount&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3856"/>
        <source>
Amount: </source>
        <translation>Ammontare: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3856"/>
        <source>, number of keys: </source>
        <translation>, numero di chiavi: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3861"/>
        <source> </source>
        <translation> </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3866"/>
        <source>
Min block height: </source>
        <translation>Altezza minima blocco: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3867"/>
        <source>
Max block height: </source>
        <translation>Altezza massima blocco: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3868"/>
        <source>
Min amount found: </source>
        <translation>Ammontare minimo trovato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3869"/>
        <source>
Max amount found: </source>
        <translation>Ammontare massimo trovato: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3870"/>
        <source>
Total count: </source>
        <translation>Conto totale: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3910"/>
        <source>
Bin size: </source>
        <translation>Dimensione Bin</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3911"/>
        <source>
Outputs per *: </source>
        <translation></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3913"/>
        <source>count
  ^
</source>
        <translation type="unfinished"></translation>
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
        <source>+--&gt; block height
</source>
        <translation type="unfinished"></translation>
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
        <translation>  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3969"/>
        <source>wallet</source>
        <translation>portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="420"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4000"/>
        <source>Random payment ID: </source>
        <translation>ID pagamento casuale: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4001"/>
        <source>Matching integrated address: </source>
        <translation>Indirizzo integrato corrispondente: </translation>
    </message>
</context>
<context>
    <name>sw</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="116"/>
        <source>Generate new wallet and save it to &lt;arg&gt;</source>
        <translation>Genera un nuovo portafoglio e salvalo in &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="117"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>Genera un portafoglio solo-ricezione da chiave per visualizzazione</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="118"/>
        <source>Generate wallet from private keys</source>
        <translation>Genera portafoglio da chiavi private</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="120"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>Specifica il seme stile Electrum per recuperare/creare il portafoglio</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="121"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Recupera portafoglio usando il seme mnemonico stile-Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="122"/>
        <source>Create non-deterministic view and spend keys</source>
        <translation>Crea chiavi per visualizzione e chiavi spendibili non-deterministiche</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="123"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation>Abilita comandi dipendenti da un daemon fidato</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="124"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>Permetti comunicazioni con un daemon che usa una versione RPC differente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="125"/>
        <source>Restore from specific blockchain height</source>
        <translation>Ripristina da specifico blocco</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="136"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>il daemon è occupato. Prova più tardi.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="145"/>
        <source>possibly lost connection to daemon</source>
        <translation>possibile perdita di connessione con il daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="226"/>
        <source>Error: </source>
        <translation>Errore: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4614"/>
        <source>Failed to initialize wallet</source>
        <translation>Inizializzazione wallet fallita</translation>
    </message>
</context>
<context>
    <name>tools::dns_utils</name>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="430"/>
        <source>DNSSEC validation passed</source>
        <translation>Convalida DNSSEC passata</translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="434"/>
        <source>WARNING: DNSSEC validation was unsuccessful, this address may not be correct!</source>
        <translation>AVVERTIMENTO: convalida DNSSEC fallita, questo indirizzo potrebbe non essere corretto!</translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="437"/>
        <source>For URL: </source>
        <translation>Per URL: </translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="439"/>
        <source> Monero Address = </source>
        <translation>Indirizzo Monero = </translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="441"/>
        <source>Is this OK? (Y/n) </source>
        <translation>Va bene? (S/n) </translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="451"/>
        <source>you have cancelled the transfer request</source>
        <translation>hai cancelliato la richiesta di transferimento</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="106"/>
        <source>Use daemon instance at &lt;host&gt;:&lt;port&gt;</source>
        <translation>Usa instanza daemon in &lt;host&gt;:&lt;port&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="107"/>
        <source>Use daemon instance at host &lt;arg&gt; instead of localhost</source>
        <translation>Usa istanza daemon all&apos;host &lt;arg&gt; invece che localhost</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="460"/>
        <source>Wallet password</source>
        <translation>Password portafoglio</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="109"/>
        <source>Wallet password file</source>
        <translation>File password portafoglio</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="110"/>
        <source>Use daemon instance at port &lt;arg&gt; instead of 18081</source>
        <translation>Usa istanza daemon alla porta &lt;arg&gt; invece che alla 18081</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="112"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>Per testnet. Il Daemon può anche essere lanciato con la flag --testnet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="113"/>
        <source>Restricts to view-only commands</source>
        <translation>Restringi i comandi a solo-vista</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="152"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>non puoi specificare la porta o l&apos;host del daemon più di una volta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="188"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>non puoi specificare più di un --password e --password-file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="204"/>
        <source>the password file specified could not be read</source>
        <translation>il file password specificato non può essere letto</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="460"/>
        <source>Enter new wallet password</source>
        <translation>Inserisci una nuova password per il portafoglio</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="464"/>
        <source>failed to read wallet password</source>
        <translation>impossibile leggere password portafoglio</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="227"/>
        <source>Failed to load file </source>
        <translation>Impossibile caricare file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="108"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="111"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>Specificare username[:password] per client del daemon RPC</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="233"/>
        <source>Failed to parse JSON</source>
        <translation>Impossibile analizzare (parse) JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="240"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="258"/>
        <source>failed to parse view key secret key</source>
        <translation>impossibile analizzare (parse) chiave per visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="264"/>
        <location filename="../src/wallet/wallet2.cpp" line="331"/>
        <location filename="../src/wallet/wallet2.cpp" line="373"/>
        <source>failed to verify view key secret key</source>
        <translation>impossibile verificare chiave per visualizzazione chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="276"/>
        <source>failed to parse spend key secret key</source>
        <translation>impossibile analizzare (parse) chiave spendibile chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="282"/>
        <location filename="../src/wallet/wallet2.cpp" line="343"/>
        <location filename="../src/wallet/wallet2.cpp" line="394"/>
        <source>failed to verify spend key secret key</source>
        <translation>impossibile verificare chiave spendibile chiave segreta</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="295"/>
        <source>Electrum-style word list failed verification</source>
        <translation>verifica lista di parole stile-Electrum fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="306"/>
        <source>At least one of Electrum-style word list and private view key must be specified</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="311"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Specificate entrambe lista parole stile-Electrum e chiave/i privata/e </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="324"/>
        <source>invalid address</source>
        <translation>indirizzo invalido</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="335"/>
        <source>view key does not match standard address</source>
        <translation>la chiave per visualizzazione non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="347"/>
        <source>spend key does not match standard address</source>
        <translation>la chiave spendibile non corrisponde all&apos;indirizzo standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="356"/>
        <source>Cannot create deprecated wallets from JSON</source>
        <translation>Impossibile creare portafogli disapprovati da JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="403"/>
        <source>failed to generate new wallet: </source>
        <translation>impossibile generare nuovo portafoglio: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="5205"/>
        <source>failed to read file </source>
        <translation>lettura file fallita</translation>
    </message>
</context>
<context>
    <name>tools::wallet_rpc_server</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="151"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Il daemon è locale, assunto per fidato</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="171"/>
        <source>Cannot specify --</source>
        <translation>Impossibile specificare --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="171"/>
        <source> and --</source>
        <translation> e --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="198"/>
        <source>Failed to create file </source>
        <translation>Impossibile creare file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="198"/>
        <source>. Check permissions or remove file</source>
        <translation>. Controlla permessi o rimuovi il file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="209"/>
        <source>Error writing to file </source>
        <translation>Errore durante scrittura su file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="212"/>
        <source>RPC username/password is stored in file </source>
        <translation>Username/password RPC conservate nel file </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1748"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>Non puoi specificare più di un --wallet-file e --generate-from-json</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1760"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>Devi specificare --wallet-file o --generate-from-json o --wallet-dir</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1764"/>
        <source>Loading wallet...</source>
        <translation>Caricando il portafoglio...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1789"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1814"/>
        <source>Storing wallet...</source>
        <translation>Conservando il portafoglio...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1791"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1816"/>
        <source>Stored ok</source>
        <translation>Conservato con successo</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1794"/>
        <source>Loaded ok</source>
        <translation>Caricato con successo</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1798"/>
        <source>Wallet initialization failed: </source>
        <translation>Inizializzazione portafoglio fallita: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1805"/>
        <source>Failed to initialize wallet rpc server</source>
        <translation>Inizializzazione server rpc portafoglio fallita</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1809"/>
        <source>Starting wallet rpc server</source>
        <translation>Server RPC portafoglio in partenza</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1811"/>
        <source>Stopped wallet rpc server</source>
        <translation>Server RPC portafoglio arrestato</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1820"/>
        <source>Failed to store wallet: </source>
        <translation>Impossibile conservare portafoglio: </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1715"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4580"/>
        <source>Wallet options</source>
        <translation>Opzioni portafoglio</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="59"/>
        <source>Generate wallet from JSON format file</source>
        <translation>Genera portafoglio da file JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="63"/>
        <source>Use wallet &lt;arg&gt;</source>
        <translation>Usa portafoglio &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="87"/>
        <source>Max number of threads to use for a parallel job</source>
        <translation>Numero massimo di threads da utilizzare per un lavoro parallelo</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="88"/>
        <source>Specify log file</source>
        <translation>Specificare log file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="89"/>
        <source>Config file</source>
        <translation>File configurazione</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="98"/>
        <source>General options</source>
        <translation>Opzioni generali</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="128"/>
        <source>Can&apos;t find config file </source>
        <translation>Impossibile trovare file configurazione </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="172"/>
        <source>Logging to: </source>
        <translation>Loggando in: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="173"/>
        <source>Logging to %s</source>
        <translation>Loggando in %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="153"/>
        <source>Usage:</source>
        <translation>Uso:</translation>
    </message>
</context>
</TS>
