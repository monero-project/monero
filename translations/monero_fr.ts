<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="fr_FR">
<context>
    <name>Monero::AddressBookImpl</name>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="53"/>
        <source>Invalid destination address</source>
        <translation>Adresse de destination invalide</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="63"/>
        <source>Invalid payment ID. Short payment ID should only be used in an integrated address</source>
        <translation>ID de paiement invalide. L&apos;identifiant de paiement court devrait seulement être utilisé dans une adresse intégrée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="70"/>
        <source>Invalid payment ID</source>
        <translation>ID de paiement invalide</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="77"/>
        <source>Integrated address and long payment ID can&apos;t be used at the same time</source>
        <translation>Adresse intégrée et ID de paiement long ne peuvent pas être utilisés en même temps</translation>
    </message>
</context>
<context>
    <name>Monero::PendingTransactionImpl</name>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="91"/>
        <source>Attempting to save transaction to file, but specified file(s) exist. Exiting to not risk overwriting. File:</source>
        <translation>Tentative d&apos;enregistrement d&apos;une transaction dans un fichier, mais le fichier spécifié existe déjà. Sortie pour ne pas risquer de l&apos;écraser. Fichier&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="98"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Échec de l&apos;écriture de(s) transaction(s) dans le fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="121"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="124"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="128"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transaction %s a été rejetée par le démon avec le statut&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="133"/>
        <source>. Reason: </source>
        <translation>. Raison&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="135"/>
        <source>Unknown exception: </source>
        <translation>Exception inconnue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="138"/>
        <source>Unhandled exception</source>
        <translation>Exception non gérée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="211"/>
        <source>Couldn&apos;t multisig sign data: </source>
        <translation>Signature multisig des données impossible : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="233"/>
        <source>Couldn&apos;t sign multisig transaction: </source>
        <translation>Signature multisig de la transaction impossible : </translation>
    </message>
</context>
<context>
    <name>Monero::UnsignedTransactionImpl</name>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="75"/>
        <source>This is a watch only wallet</source>
        <translation>Ceci est un portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="85"/>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="92"/>
        <source>Failed to sign transaction</source>
        <translation>Échec de signature de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="168"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>La monnaie réclamée ne va pas à une adresse payée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="174"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>La monnaie réclamée est supérieure au paiement à l&apos;adresse de monnaie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="184"/>
        <source>Change goes to more than one address</source>
        <translation>La monnaie rendue va à plus d&apos;une adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="197"/>
        <source>sending %s to %s</source>
        <translation>envoi de %s à %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="203"/>
        <source>with no destinations</source>
        <translation>sans destination</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="209"/>
        <source>%s change to %s</source>
        <translation>%s de monnaie rendue à %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="212"/>
        <source>no change</source>
        <translation>sans monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="214"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu. %s</source>
        <translation>%lu transactions chargées, pour %s, frais %s, %s, %s, taille de cercle minimum %lu, %s</translation>
    </message>
</context>
<context>
    <name>Monero::WalletImpl</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1354"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1363"/>
        <source>Failed to add short payment id: </source>
        <translation>Échec de l&apos;ajout de l&apos;ID de paiement court&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1399"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1481"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1401"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1483"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1403"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1485"/>
        <source>RPC error: </source>
        <translation>Erreur RPC&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1431"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1516"/>
        <source>not enough outputs for specified ring size</source>
        <translation>pas assez de sorties pour la taille de cercle spécifiée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1433"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1518"/>
        <source>found outputs to use</source>
        <translation>sorties à utiliser trouvées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1435"/>
        <source>Please sweep unmixable outputs.</source>
        <translation>Veuillez balayer les sorties non mélangeables.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1409"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1492"/>
        <source>not enough money to transfer, available only %s, sent amount %s</source>
        <translation>pas assez de fonds pour le transfert, montant disponible %s, montant envoyé %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="540"/>
        <source>failed to parse address</source>
        <translation>échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="551"/>
        <source>failed to parse secret spend key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="574"/>
        <source>failed to parse secret view key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="583"/>
        <source>failed to verify secret spend key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="587"/>
        <source>spend key does not match address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="593"/>
        <source>failed to verify secret view key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="597"/>
        <source>view key does not match address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="620"/>
        <location filename="../src/wallet/api/wallet.cpp" line="637"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="884"/>
        <source>Failed to send import wallet request</source>
        <translation>Échec de l&apos;envoi de la requête d&apos;importation de portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1034"/>
        <source>Failed to load unsigned transactions</source>
        <translation>Échec du chargement des transaction non signées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1053"/>
        <source>Failed to load transaction from file</source>
        <translation>Échec du chargement de la transaction du fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1069"/>
        <source>Wallet is view only</source>
        <translation>Portefeuille d&apos;audit uniquement</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1077"/>
        <source>failed to save file </source>
        <translation>échec de l&apos;enregistrement du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1093"/>
        <source>Key images can only be imported with a trusted daemon</source>
        <translation>Les images de clé ne peuvent être importées qu&apos;avec un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1106"/>
        <source>Failed to import key images: </source>
        <translation>Échec de l&apos;importation des images de clé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1138"/>
        <source>Failed to get subaddress label: </source>
        <translation>Échec de la récupération de l&apos;étiquette de sous-adresse&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1151"/>
        <source>Failed to set subaddress label: </source>
        <translation>Échec de l&apos;affectation de l&apos;étiquette de sous-adresse&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="566"/>
        <source>Neither view key nor spend key supplied, cancelled</source>
        <translation>Ni clé d&apos;audit ni clé de dépense fournie, annulation</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="685"/>
        <source>Electrum seed is empty</source>
        <translation>La phrase Electrum est vide</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="694"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1168"/>
        <source>Failed to get multisig info: </source>
        <translation>Échec de la récupération des infos multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1185"/>
        <source>Failed to make multisig: </source>
        <translation>Échec de la création multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1200"/>
        <source>Failed to finalize multisig wallet creation</source>
        <translation>Échec de la finalisation de la création du portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1203"/>
        <source>Failed to finalize multisig wallet creation: </source>
        <translation>Échec de la finalisation de la création du portefeuille multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1219"/>
        <source>Failed to export multisig images: </source>
        <translation>Échec de l&apos;exportation des images multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1237"/>
        <source>Failed to parse imported multisig images</source>
        <translation>Échec de l&apos;analyse des images multisig importées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1247"/>
        <source>Failed to import multisig images: </source>
        <translation>Échec de l&apos;importation des images multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1261"/>
        <source>Failed to check for partial multisig key images: </source>
        <translation>Échec de la vérification des images de clé multisig partielles : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1289"/>
        <source>Failed to restore multisig transaction: </source>
        <translation>Échec de la restauration de la transaction multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1329"/>
        <source>Invalid destination address</source>
        <translation>Adresse de destination invalide</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1405"/>
        <source>failed to get outputs to mix: %s</source>
        <translation>échec de la récupération de sorties à mélanger : %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1416"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1500"/>
        <source>not enough money to transfer, overall balance only %s, sent amount %s</source>
        <translation>pas assez de fonds pour le transfer, solde global disponible %s, montant envoyé %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1423"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1508"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>pas assez de fonds pour le transfert, montant disponible %s, montant envoyé %s = %s + %s (frais)</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1433"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1518"/>
        <source>output amount</source>
        <translation>montant de la sortie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1438"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1522"/>
        <source>transaction was not constructed</source>
        <translation>la transaction n&apos;a pas été construite</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1441"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1525"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transaction %s a été rejetée par le démon avec le statut&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1446"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1530"/>
        <source>one of destinations is zero</source>
        <translation>une des destinations est zéro</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1448"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1532"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>échec de la recherche d&apos;une façon adéquate de scinder les transactions</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1450"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1534"/>
        <source>unknown transfer error: </source>
        <translation>erreur de transfert inconnue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1452"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1536"/>
        <source>internal error: </source>
        <translation>erreur interne&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1454"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1538"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1456"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1540"/>
        <source>unknown error</source>
        <translation>erreur inconnue</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1487"/>
        <source>failed to get outputs to mix</source>
        <translation>échec de la récupération de sorties à mélanger</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1615"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1642"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1690"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1718"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1746"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1767"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2222"/>
        <source>Failed to parse txid</source>
        <translation>Échec de l&apos;analyse de l&apos;ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1632"/>
        <source>no tx keys found for this txid</source>
        <translation>aucune clé de transaction trouvée pour cet ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1650"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1659"/>
        <source>Failed to parse tx key</source>
        <translation>Échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1668"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1697"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1725"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1806"/>
        <source>Failed to parse address</source>
        <translation>Échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1811"/>
        <source>Address must not be a subaddress</source>
        <translation>L&apos;adresse ne doit pas être une sous-adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1851"/>
        <source>The wallet must be in multisig ready state</source>
        <translation>Le portefeuille doit être multisig et prêt</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1873"/>
        <source>Given string is not a key</source>
        <translation>La chaîne entrée n&apos;est pas une clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2094"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>Réexaminer les dépenses ne peut se faire qu&apos;avec un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2143"/>
        <source>Invalid output: </source>
        <translation>Sortie invalide : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2150"/>
        <source>Failed to set blackballed outputs</source>
        <translation>Échec de l&apos;affectation des sorties blackboulées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2161"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2183"/>
        <source>Failed to parse output amount</source>
        <translation>Échec de l&apos;analyse du montant de la sortie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2166"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2188"/>
        <source>Failed to parse output offset</source>
        <translation>Échec de l&apos;analyse de l&apos;offset de la sortie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2172"/>
        <source>Failed to blackball output</source>
        <translation>Échec du blackboulage de la sortie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2194"/>
        <source>Failed to unblackball output</source>
        <translation>Échec du déblackboulage de la sortie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2205"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2244"/>
        <source>Failed to parse key image</source>
        <translation>Échec de l&apos;analyse de l&apos;image de clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2211"/>
        <source>Failed to get ring</source>
        <translation>Échec de la récupération du cercle</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2229"/>
        <source>Failed to get rings</source>
        <translation>Échec de la récupération des cercles</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2250"/>
        <source>Failed to set ring</source>
        <translation>Échec de l&apos;affectation du cercle</translation>
    </message>
</context>
<context>
    <name>Wallet</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="301"/>
        <source>Failed to parse address</source>
        <translation>Échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="308"/>
        <source>Failed to parse key</source>
        <translation>Échec de l&apos;analyse de la clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="316"/>
        <source>failed to verify key</source>
        <translation>Échec de la vérification de la clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="326"/>
        <source>key does not match address</source>
        <translation>la clé ne correspond pas à l&apos;adresse</translation>
    </message>
</context>
<context>
    <name>command_line</name>
    <message>
        <location filename="../src/common/command_line.cpp" line="57"/>
        <source>yes</source>
        <translation>oui</translation>
    </message>
    <message>
        <location filename="../src/common/command_line.cpp" line="71"/>
        <source>no</source>
        <translation>non</translation>
    </message>
</context>
<context>
    <name>cryptonote::rpc_args</name>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="40"/>
        <source>Specify IP to bind RPC server</source>
        <translation>Spécifier l&apos;IP à laquelle lier le serveur RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="41"/>
        <source>Specify username[:password] required for RPC server</source>
        <translation>Spécifier le nom_utilisateur[:mot_de_passe] requis pour le serveur RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="42"/>
        <source>Confirm rpc-bind-ip value is NOT a loopback (local) IP</source>
        <translation>Confirmer que la valeur de rpc-bind-ip n&apos;est PAS une IP de bouclage (locale)</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="43"/>
        <source>Specify a comma separated list of origins to allow cross origin resource sharing</source>
        <translation>Spécifier une liste d&apos;origines séparées par des virgules pour autoriser le partage de ressource de différentes origines (CORS)</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="70"/>
        <source>Invalid IP address given for --</source>
        <translation>Adresse IP invalide fournie pour --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="78"/>
        <source> permits inbound unencrypted external connections. Consider SSH tunnel or SSL proxy instead. Override with --</source>
        <translation> autorise les connexions entrantes non cryptées venant de l&apos;extérieur. Considérez plutôt un tunnel SSH ou un proxy SSL. Outrepasser avec --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="101"/>
        <source>Username specified with --</source>
        <translation>Le nom d&apos;utilisateur spécifié avec --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="101"/>
        <location filename="../src/rpc/rpc_args.cpp" line="111"/>
        <source> cannot be empty</source>
        <translation> ne peut pas être vide</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="111"/>
        <source> requires RPC server password --</source>
        <translation> nécessite le mot de passe du serveur RPC --</translation>
    </message>
</context>
<context>
    <name>cryptonote::simple_wallet</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="552"/>
        <source>Commands: </source>
        <translation>Commandes&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3911"/>
        <source>failed to read wallet password</source>
        <translation>échec de la lecture du mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3511"/>
        <source>invalid password</source>
        <translation>mot de passe invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2632"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation>set seed&#xa0;: requiert un argument. options disponibles&#xa0;: language</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2665"/>
        <source>set: unrecognized argument(s)</source>
        <translation>set&#xa0;: argument(s) non reconnu(s)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3753"/>
        <source>wallet file path not valid: </source>
        <translation>chemin du fichier portefeuille non valide&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2735"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation>Tentative de génération ou de restauration d&apos;un portefeuille, mais le fichier spécifié existe déjà. Sortie pour ne pas risquer de l&apos;écraser.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="772"/>
        <source>usage: payment_id</source>
        <translation>usage&#xa0;: payment_id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2618"/>
        <source>needs an argument</source>
        <translation>requiert un argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2641"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2642"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2643"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2645"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2648"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2653"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2654"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2656"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2658"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2659"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2660"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2663"/>
        <source>0 or 1</source>
        <translation>0 ou 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2651"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2655"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2662"/>
        <source>unsigned integer</source>
        <translation>entier non signé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2789"/>
        <source>NOTE: the following 25 words can be used to recover access to your wallet. Write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation>VEUILLEZ NOTER&#xa0;: les 25 mots suivants peuvent être utilisés pour restaurer votre portefeuille. Veuillez les écrire sur papier et les garder dans un endroit sûr. Ne les gardez pas dans un courriel ou dans un service de stockage de fichiers hors de votre contrôle.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2869"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-deterministic-wallet utilise --generate-new-wallet, pas --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2898"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation>spécifiez un paramètre de récupération avec --electrum-seed=&quot;liste de mots ici&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3274"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation>spécifiez un chemin de portefeuille avec --generate-new-wallet (pas --wallet-file)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3444"/>
        <source>wallet failed to connect to daemon: </source>
        <translation>échec de la connexion du portefeuille au démon&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3452"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation>Le démon utilise une version majeure de RPC (%u) différente de celle du portefeuille (%u)&#xa0;: %s. Mettez l&apos;un des deux à jour, ou utilisez --allow-mismatched-daemon-version.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3473"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation>Liste des langues disponibles pour la phrase mnémonique de votre portefeuille&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3483"/>
        <source>Enter the number corresponding to the language of your choice: </source>
        <translation>Entrez le nombre correspondant à la langue de votre choix&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3557"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Veuillez dorénavant utiliser la nouvelle phrase mnémonique que nous fournissons.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3573"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3645"/>
        <source>Generated new wallet: </source>
        <translation>Nouveau portefeuille généré&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3582"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3650"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3689"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3742"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3782"/>
        <source>Opened watch-only wallet</source>
        <translation>Ouverture du portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3786"/>
        <source>Opened wallet</source>
        <translation>Ouverture du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3804"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Veuillez procéder à la mise à jour de votre portefeuille.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3819"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Le format de votre fichier portefeuille est en cours de mise à jour.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3827"/>
        <source>failed to load wallet: </source>
        <translation>échec du chargement du portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3844"/>
        <source>Use the &quot;help&quot; command to see the list of available commands.
</source>
        <translation>Utilisez la commande &quot;help&quot; pour voir la liste des commandes disponibles.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3889"/>
        <source>Wallet data saved</source>
        <translation>Données du portefeuille sauvegardées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3983"/>
        <source>Mining started in daemon</source>
        <translation>La mine a démarré dans le démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3985"/>
        <source>mining has NOT been started: </source>
        <translation>la mine n&apos;a PAS démarré&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4005"/>
        <source>Mining stopped in daemon</source>
        <translation>La mine a été stoppée dans le démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4007"/>
        <source>mining has NOT been stopped: </source>
        <translation>la mine n&apos;a PAS été stoppée&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4089"/>
        <source>Blockchain saved</source>
        <translation>Chaîne de blocs sauvegardée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4104"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4141"/>
        <source>Height </source>
        <translation>Hauteur </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4143"/>
        <source>spent </source>
        <translation>dépensé </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4195"/>
        <source>Starting refresh...</source>
        <translation>Démarrage du rafraîchissement...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4208"/>
        <source>Refresh done, blocks received: </source>
        <translation>Rafraîchissement effectué, blocs reçus&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5362"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4761"/>
        <source>bad locked_blocks parameter:</source>
        <translation>mauvais paramètre locked_blocks&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5382"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5631"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation>une unique transaction ne peut pas utiliser plus d&apos;un ID de paiement&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4857"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5391"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5599"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5639"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>échec de la définition de l&apos;ID de paiement, bien qu&apos;il ait été décodé correctement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4874"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4952"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5040"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5149"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5405"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5463"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5653"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5696"/>
        <source>transaction cancelled.</source>
        <translation>transaction annulée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4931"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4941"/>
        <source>Is this okay anyway?  (Y/Yes/N/No): </source>
        <translation>Est-ce correct quand même ?  (Y/Yes/Oui/N/No/Non)&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4936"/>
        <source>There is currently a %u block backlog at that fee level. Is this okay?  (Y/Yes/N/No): </source>
        <translation>Il y a actuellement un arriéré de %u blocs à ce niveau de frais. Est-ce correct quand même ?  (Y/Yes/Oui/N/No/Non)&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4941"/>
        <source>Failed to check for backlog: </source>
        <translation>Échec de la vérification du backlog&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4982"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5436"/>
        <source>
Transaction </source>
        <translation>
Transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4987"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5441"/>
        <source>Spending from address index %d
</source>
        <translation>Dépense depuis l&apos;adresse d&apos;index %d
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4989"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5443"/>
        <source>WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.
</source>
        <translation>ATTENTION&#xa0;: Des sorties de multiples adresses sont utilisées ensemble, ce qui pourrait potentiellement compromettre votre confidentialité.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4991"/>
        <source>Sending %s.  </source>
        <translation>Envoi de %s.  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4994"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation>Votre transaction doit être scindée en %llu transactions. Il en résulte que des frais de transaction doivent être appliqués à chaque transaction, pour un total de %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5000"/>
        <source>The transaction fee is %s</source>
        <translation>Les frais de transaction sont de %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5003"/>
        <source>, of which %s is dust from change</source>
        <translation>, dont %s est de la poussière de monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5004"/>
        <source>.</source>
        <translation>.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5004"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation>Un total de %s de poussière de monnaie rendue sera envoyé à une adresse de poussière</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5009"/>
        <source>.
This transaction will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation>.
Cette transaction sera déverrouillée au bloc %llu, dans approximativement %s jours (en supposant 2 minutes par bloc)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5052"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5064"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5160"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5172"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5474"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5486"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5706"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5718"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Échec de l&apos;écriture de(s) transaction(s) dans le fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5056"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5068"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5164"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5176"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5478"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5490"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5710"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5722"/>
        <source>Unsigned transaction(s) successfully written to file: </source>
        <translation>Transaction(s) non signée(s) écrite(s) dans le fichier avec succès&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5119"/>
        <source>No unmixable outputs found</source>
        <translation>Aucune sortie non mélangeable trouvée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5219"/>
        <source>No address given</source>
        <translation>Aucune adresse fournie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5593"/>
        <source>failed to parse Payment ID</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5616"/>
        <source>failed to parse key image</source>
        <translation>échec de l&apos;analyse de l&apos;image de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5668"/>
        <source>No outputs found</source>
        <translation>Pas de sorties trouvées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5673"/>
        <source>Multiple transactions are created, which is not supposed to happen</source>
        <translation>De multiples transactions sont crées, ce qui n&apos;est pas supposé arriver</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5678"/>
        <source>The transaction uses multiple or no inputs, which is not supposed to happen</source>
        <translation>La transaction utilise aucune ou de multiples entrées, ce qui n&apos;est pas supposé arriver</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5755"/>
        <source>missing threshold amount</source>
        <translation>montant seuil manquant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5760"/>
        <source>invalid amount threshold</source>
        <translation>montant seuil invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5777"/>
        <source>usage: donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation>usage&#xa0;: donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] &lt;montant&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5871"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>La monnaie réclamée ne va pas à une adresse payée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5876"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>La monnaie réclamée est supérieure au paiement à l&apos;adresse de monnaie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5907"/>
        <source>sending %s to %s</source>
        <translation>envoi de %s à %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5917"/>
        <source> dummy output(s)</source>
        <translation> sortie(s) factice(s)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5920"/>
        <source>with no destinations</source>
        <translation>sans destination</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5932"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu, %s. %sIs this okay? (Y/Yes/N/No): </source>
        <translation>%lu transactions chargées, pour %s, frais %s, %s, %s, taille de cercle minimum %lu, %s. %sEst-ce correct ? (Y/Yes/Oui/N/No/Non)&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5961"/>
        <source>This is a multisig wallet, it can only sign with sign_multisig</source>
        <translation>Ceci est un portefeuille multisig, il ne peut signer qu&apos;avec sign_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5984"/>
        <source>Failed to sign transaction</source>
        <translation>Échec de signature de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5990"/>
        <source>Failed to sign transaction: </source>
        <translation>Échec de signature de transaction&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6011"/>
        <source>Transaction raw hex data exported to </source>
        <translation>Données brutes hex de la transaction exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6032"/>
        <source>Failed to load transaction from file</source>
        <translation>Échec du chargement de la transaction du fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4224"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4524"/>
        <source>RPC error: </source>
        <translation>Erreur RPC&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="602"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il n&apos;a pas de clé de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="745"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="912"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="961"/>
        <source>Your original password was incorrect.</source>
        <translation>Votre mot de passe original est incorrect.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="760"/>
        <source>Error with wallet rewrite: </source>
        <translation>Erreur avec la réécriture du portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1994"/>
        <source>invalid unit</source>
        <translation>unité invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2012"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2074"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation>nombre invalide&#xa0;: un entier non signé est attendu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2030"/>
        <source>invalid value</source>
        <translation>valeur invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2674"/>
        <source>usage: set_log &lt;log_level_number_0-4&gt; | &lt;categories&gt;</source>
        <translation>usage&#xa0;: set_log &lt;niveau_de_journalisation_0-4&gt; | &lt;catégories&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2761"/>
        <source>(Y/Yes/N/No): </source>
        <translation>(Y/Yes/Oui/N/No/Non)&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3318"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3345"/>
        <source>bad m_restore_height parameter: </source>
        <translation>mauvais paramètre m_restore_height&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3323"/>
        <source>date format must be YYYY-MM-DD</source>
        <translation>le format de date doit être AAAA-MM-JJ</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3336"/>
        <source>Restore height is: </source>
        <translation>La hauteur de restauration est&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3262"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3337"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5033"/>
        <source>Is this okay?  (Y/Yes/N/No): </source>
        <translation>Est-ce correct ? (Y/Yes/Oui/N/No/Non)&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4061"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Le démon est local, supposons qu&apos;il est de confiance</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3907"/>
        <source>Password for new watch-only wallet</source>
        <translation>Mot de passe pour le nouveau portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4234"/>
        <source>internal error: </source>
        <translation>erreur interne&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1339"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4239"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4529"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1267"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1344"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4244"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4534"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5083"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5205"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5505"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5739"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6045"/>
        <source>unknown error</source>
        <translation>erreur inconnue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4249"/>
        <source>refresh failed: </source>
        <translation>échec du rafraîchissement&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4249"/>
        <source>Blocks received: </source>
        <translation>Blocs reçus&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4282"/>
        <source>unlocked balance: </source>
        <translation>solde débloqué&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2652"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4378"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4429"/>
        <source>amount</source>
        <translation>montant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="248"/>
        <source>false</source>
        <translation>faux</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="566"/>
        <source>Unknown command: </source>
        <translation>Commande inconnue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="573"/>
        <source>Command usage: </source>
        <translation>Usage de la commande&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="576"/>
        <source>Command description: </source>
        <translation>Description de la commande&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="644"/>
        <source>wallet is multisig but not yet finalized</source>
        <translation>le portefeuille est multisig mais pas encore finalisé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="674"/>
        <source>Failed to retrieve seed</source>
        <translation>Échec de la récupération de la phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="698"/>
        <source>wallet is multisig and has no seed</source>
        <translation>le portefeuille est multisig et n&apos;a pas de phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="784"/>
        <source>Cannot connect to daemon</source>
        <translation>Impossible de se connecter au démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="808"/>
        <source>Error: failed to estimate backlog array size: </source>
        <translation>Erreur&#xa0;: échec de l&apos;estimation de la taille du tableau d&apos;arriéré&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="813"/>
        <source>Error: bad estimated backlog array size</source>
        <translation>Erreur&#xa0;: mauvaise estimation de la taille du tableau d&apos;arriéré</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="825"/>
        <source> (current)</source>
        <translation> (actuel)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="828"/>
        <source>%u block (%u minutes) backlog at priority %u%s</source>
        <translation>arriéré de %u bloc(s) (%u minutes) à la priorité %u%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="830"/>
        <source>%u to %u block (%u to %u minutes) backlog at priority %u</source>
        <translation>arriéré de %u à %u bloc(s) (%u à %u minutes) à la priorité %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="833"/>
        <source>No backlog at priority </source>
        <translation>Pas d&apos;arriéré à la priorité </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="847"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="880"/>
        <source>This wallet is already multisig</source>
        <translation>Le portefeuille est déjà multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="852"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="885"/>
        <source>wallet is watch-only and cannot be made multisig</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas être tranformé en multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="858"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="891"/>
        <source>This wallet has been used before, please use a new wallet to create a multisig wallet</source>
        <translation>Ce portefeuille a été utilisé auparavant, veuillez utiliser un nouveau portefeuille pour créer un portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="866"/>
        <source>Send this multisig info to all other participants, then use make_multisig &lt;threshold&gt; &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Envoyez ces infos multisig à tous les autres participants, ensuite utilisez make_multisig &lt;seuil&gt; &lt;info1&gt; [&lt;info2&gt;...] avec les infos multisig des autres</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="867"/>
        <source>This includes the PRIVATE view key, so needs to be disclosed only to that multisig wallet&apos;s participants </source>
        <translation>Ceci inclut la clé PRIVÉE d&apos;audit, donc ne doit être divulgué qu&apos;aux participants de ce portefeuille multisig </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="897"/>
        <source>usage: make_multisig &lt;threshold&gt; &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</source>
        <translation>usage&#xa0;: make_multisig &lt;seuil&gt; &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="905"/>
        <source>Invalid threshold</source>
        <translation>Seuil invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="925"/>
        <source>Another step is needed</source>
        <translation>Une autre étape est nécessaire</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="927"/>
        <source>Send this multisig info to all other participants, then use finalize_multisig &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Envoyez ces infos multisig à tous les autres participants, ensuite utilisez finalize_multisig &lt;info1&gt; [&lt;info2&gt;...] avec les infos multisig des autres</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="933"/>
        <source>Error creating multisig: </source>
        <translation>Erreur de création multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="940"/>
        <source>Error creating multisig: new wallet is not multisig</source>
        <translation>Erreur de création multisig&#xa0;: le nouveau portefeuille n&apos;est pas multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="943"/>
        <source> multisig address: </source>
        <translation> adresse multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="967"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1011"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1063"/>
        <source>This wallet is not multisig</source>
        <translation>Ce portefeuille n&apos;est pas multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="972"/>
        <source>This wallet is already finalized</source>
        <translation>Ce portefeuille est déjà finalisé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="980"/>
        <source>usage: finalize_multisig &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</source>
        <translation>usage&#xa0;: finalize_multisig &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="988"/>
        <source>Failed to finalize multisig</source>
        <translation>Échec de finalisation multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="994"/>
        <source>Failed to finalize multisig: </source>
        <translation>Échec de finalisation multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1016"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1068"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1147"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1221"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1289"/>
        <source>This multisig wallet is not yet finalized</source>
        <translation>Ce portefeuille multisig n&apos;est pas encore finalisé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1021"/>
        <source>usage: export_multisig_info &lt;filename&gt;</source>
        <translation>usage&#xa0;: export_multisig_info &lt;nom_fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1044"/>
        <source>Error exporting multisig info: </source>
        <translation>Erreur d&apos;importation des infos multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1048"/>
        <source>Multisig info exported to </source>
        <translation>Infos multisig exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1073"/>
        <source>usage: import_multisig_info &lt;filename1&gt; [&lt;filename2&gt;...] - one for each other participant</source>
        <translation>usage&#xa0;: import_multisig_info &lt;nom_fichier1&gt; [&lt;nom_fichier2&gt;...] - un pour chaque autre participant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1101"/>
        <source>Multisig info imported</source>
        <translation>Infos multisig importées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1105"/>
        <source>Failed to import multisig info: </source>
        <translation>Échec de l&apos;importation des infos multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1116"/>
        <source>Failed to update spent status after importing multisig info: </source>
        <translation>Échec de la mise à jour de l&apos;état des dépenses après l&apos;importation des infos multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1121"/>
        <source>Untrusted daemon, spent status may be incorrect. Use a trusted daemon and run &quot;rescan_spent&quot;</source>
        <translation>Pas un démon de confiance, l&apos;état des dépenses peut être incorrect. Utilisez un démon de confiance et executez &quot;rescan_spent&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1142"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1216"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1284"/>
        <source>This is not a multisig wallet</source>
        <translation>Ceci n&apos;est pas un portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1152"/>
        <source>usage: sign_multisig &lt;filename&gt;</source>
        <translation>usage&#xa0;: sign_multisig &lt;nom_fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1166"/>
        <source>Failed to sign multisig transaction</source>
        <translation>Échec de la signature de la transaction multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1172"/>
        <source>Multisig error: </source>
        <translation>Erreur multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1177"/>
        <source>Failed to sign multisig transaction: </source>
        <translation>Échec de la signature de la transaction multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1200"/>
        <source>It may be relayed to the network with submit_multisig</source>
        <translation>Elle peut être transmise au réseau avec submit_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1226"/>
        <source>usage: submit_multisig &lt;filename&gt;</source>
        <translation>usage&#xa0;: submit_multisig &lt;nom_fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1242"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1309"/>
        <source>Failed to load multisig transaction from file</source>
        <translation>Échec du chargement de la transaction multisig du fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1247"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1314"/>
        <source>Multisig transaction signed by only %u signers, needs %u more signatures</source>
        <translation>Transaction multisig signée par %u signataire(s) seulement, nécessite %u signature(s) de plus</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1256"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8024"/>
        <source>Transaction successfully submitted, transaction </source>
        <translation>Transaction transmise avec succès, transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1257"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8025"/>
        <source>You can check its status by using the `show_transfers` command.</source>
        <translation>Vous pouvez vérifier son statut en utilisant la commane &apos;show_transfers&apos;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1294"/>
        <source>usage: export_raw_multisig &lt;filename&gt;</source>
        <translation>usage&#xa0;: export_raw_multisig &lt;nom_fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1330"/>
        <source>Failed to export multisig transaction to file </source>
        <translation>Échec de l&apos;exportation de la transaction multisig vers le fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1334"/>
        <source>Saved exported multisig transaction file(s): </source>
        <translation>Transaction multisig enregistrée dans le(s) fichier(s)&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1805"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1811"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1830"/>
        <source>ring size must be an integer &gt;= </source>
        <translation>la taille de cercle doit être un nombre entier &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1835"/>
        <source>could not change default ring size</source>
        <translation>échec du changement de la taille de cercle par défaut</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2108"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2179"/>
        <source>Invalid height</source>
        <translation>Hauteur invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2225"/>
        <source>start_mining [&lt;number_of_threads&gt;] [bg_mining] [ignore_battery]</source>
        <translation>start_mining [&lt;nombre_de_threads&gt;] [mine_arrière_plan] [ignorer_batterie]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2226"/>
        <source>Start mining in the daemon (bg_mining and ignore_battery are optional booleans).</source>
        <translation>Démarrer la mine dans le démon (mine_arrière_plan et ignorer_batterie sont des booléens facultatifs).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2229"/>
        <source>Stop mining in the daemon.</source>
        <translation>Arrêter la mine dans le démon.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2233"/>
        <source>Set another daemon to connect to.</source>
        <translation>Spécifier un autre démon auquel se connecter.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2236"/>
        <source>Save the current blockchain data.</source>
        <translation>Sauvegarder les données actuelles de la châine de blocs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2239"/>
        <source>Synchronize the transactions and balance.</source>
        <translation>Synchroniser les transactions et le solde.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2242"/>
        <source>balance [detail]</source>
        <translation>solde [détail]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2243"/>
        <source>Show the wallet&apos;s balance of the currently selected account.</source>
        <translation>Afficher le solde du compte actuellement sélectionné.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2246"/>
        <source>incoming_transfers [available|unavailable] [verbose] [index=&lt;N1&gt;[,&lt;N2&gt;[,...]]]</source>
        <translation>incoming_transfers [available|unavailable] [verbose] [index=&lt;N1&gt;[,&lt;N2&gt;[,...]]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2247"/>
        <source>Show the incoming transfers, all or filtered by availability and address index.</source>
        <translation>Afficher les transferts entrants, tous ou filtrés par disponibilité et index d&apos;adresse.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2250"/>
        <source>payments &lt;PID_1&gt; [&lt;PID_2&gt; ... &lt;PID_N&gt;]</source>
        <translation>payments &lt;PID_1&gt; [&lt;PID_2&gt; ... &lt;PID_N&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2251"/>
        <source>Show the payments for the given payment IDs.</source>
        <translation>Afficher les paiements pour les IDs de paiement donnés.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2254"/>
        <source>Show the blockchain height.</source>
        <translation>Afficher la hauteur de la chaîne de blocs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2268"/>
        <source>Send all unmixable outputs to yourself with ring_size 1</source>
        <translation>Envoyer toutes les sorties non mélangeables à vous-même avec une taille de cercle de 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2274"/>
        <source>sweep_below &lt;amount_threshold&gt; [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation>sweep_below &lt;montant_seuil&gt; [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] &lt;adresse&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2275"/>
        <source>Send all unlocked outputs below the threshold to an address.</source>
        <translation>Envoyer toutes les sorties débloquées d&apos;un montant inférieur au seuil à une adresse.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2279"/>
        <source>Send a single output of the given key image to an address without change.</source>
        <translation>Envoyer une unique sortie ayant une image de clé donnée à une adresse sans rendu de monnaie.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2282"/>
        <source>donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation>donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] &lt;montant&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2283"/>
        <source>Donate &lt;amount&gt; to the development team (donate.getmonero.org).</source>
        <translation>Donner &lt;montant&gt; à l&apos;équipe de développement (donate.getmonero.org).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2290"/>
        <source>Submit a signed transaction from a file.</source>
        <translation>Transmettre une transaction signée d&apos;un fichier.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2293"/>
        <source>set_log &lt;level&gt;|{+,-,}&lt;categories&gt;</source>
        <translation>set_log &lt;niveau&gt;|{+,-,}&lt;catégories&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2294"/>
        <source>Change the current log detail (level must be &lt;0-4&gt;).</source>
        <translation>Changer le niveau de détail du journal (le niveau doit être &lt;0-4&gt;).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2297"/>
        <source>account
  account new &lt;label text with white spaces allowed&gt;
  account switch &lt;index&gt; 
  account label &lt;index&gt; &lt;label text with white spaces allowed&gt;
  account tag &lt;tag_name&gt; &lt;account_index_1&gt; [&lt;account_index_2&gt; ...]
  account untag &lt;account_index_1&gt; [&lt;account_index_2&gt; ...]
  account tag_description &lt;tag_name&gt; &lt;description&gt;</source>
        <translation>account
  account new &lt;texte étiquette avec espaces autorisés&gt;
  account switch &lt;index&gt; 
  account label &lt;index&gt; &lt;texte étiquette avec espaces autorisés&gt;
  account tag &lt;mot_clé&gt; &lt;index_compte_1&gt; [&lt;index_compte_2&gt; ...]
  account untag &lt;index_compte_1&gt; [&lt;index_compte_2&gt; ...]
  account tag_description &lt;mot_clé&gt; &lt;description&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2304"/>
        <source>If no arguments are specified, the wallet shows all the existing accounts along with their balances.
If the &quot;new&quot; argument is specified, the wallet creates a new account with its label initialized by the provided label text (which can be empty).
If the &quot;switch&quot; argument is specified, the wallet switches to the account specified by &lt;index&gt;.
If the &quot;label&quot; argument is specified, the wallet sets the label of the account specified by &lt;index&gt; to the provided label text.
If the &quot;tag&quot; argument is specified, a tag &lt;tag_name&gt; is assigned to the specified accounts &lt;account_index_1&gt;, &lt;account_index_2&gt;, ....
If the &quot;untag&quot; argument is specified, the tags assigned to the specified accounts &lt;account_index_1&gt;, &lt;account_index_2&gt; ..., are removed.
If the &quot;tag_description&quot; argument is specified, the tag &lt;tag_name&gt; is assigned an arbitrary text &lt;description&gt;.</source>
        <translation>Si aucun argument n&apos;est spécifié, le portefeuille affiche tous les comptes existants ainsi que leurs soldes.
Si l&apos;argument &quot;new&quot; est spécifié, le portefeuille crée un nouveau compte avec son étiquette initialisée par le texte fourni (qui peut être vide).
Si l&apos;argument &quot;switch&quot; est spécifié, le portefeuille passe au compte spécifié par &lt;index&gt;.
Si l&apos;argument &quot;label&quot; est spécifié, le portefeuille affecte le texte fourni à l&apos;étiquette du compte spécifié par &lt;index&gt;.
Si l&apos;argument &quot;tag&quot; est spécifié, un mot clé &lt;mot_clé&gt; est assigné aux comptes spécifiés &lt;account_index_1&gt;, &lt;account_index_2&gt;, ....
Si l&apos;argument &quot;untag&quot; est spécifié, les mots clés assignés aux comptes spécifiés &lt;account_index_1&gt;, &lt;account_index_2&gt; ..., sont supprimés.
Si l&apos;argument &quot;tag_description&quot; est spécifié, le texte arbitraire &lt;description&gt; est assigné au mot clé &lt;mot_clé&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2313"/>
        <source>address [ new &lt;label text with white spaces allowed&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;label text with white spaces allowed&gt;]</source>
        <translation>address [ new &lt;texte étiquette avec espaces autorisés&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;texte étiquette avec espaces autorisés&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2317"/>
        <source>integrated_address [&lt;payment_id&gt; | &lt;address&gt;]</source>
        <translation>integrated_address [&lt;ID_paiement&gt; | &lt;adresse&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2318"/>
        <source>Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation>Encoder un ID de paiement dans une adresse intégrée pour l&apos;adresse publique du portefeuille actuel (en l&apos;absence d&apos;argument un ID de paiement aléatoire est utilisé), ou décoder une adresse intégrée en une adresse standard et un ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2321"/>
        <source>address_book [(add ((&lt;address&gt; [pid &lt;id&gt;])|&lt;integrated address&gt;) [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</source>
        <translation>address_book [(add ((&lt;adresse&gt; [pid &lt;id&gt;])|&lt;adresse intégrée&gt;) [&lt;description avec éventuellement des espaces&gt;])|(delete &lt;index&gt;)]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2322"/>
        <source>Print all entries in the address book, optionally adding/deleting an entry to/from it.</source>
        <translation>Afficher toutes les entrées du carnet d&apos;adresses, optionnellement en y ajoutant/supprimant une entrée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2325"/>
        <source>Save the wallet data.</source>
        <translation>Sauvegarder les données du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2328"/>
        <source>Save a watch-only keys file.</source>
        <translation>Sauvegarder un fichier de clés d&apos;audit.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2331"/>
        <source>Display the private view key.</source>
        <translation>Afficher la clé privée d&apos;audit.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2334"/>
        <source>Display the private spend key.</source>
        <translation>Afficher la clé privée de dépense.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2337"/>
        <source>Display the Electrum-style mnemonic seed</source>
        <translation>Afficher la phrase mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2340"/>
        <source>set &lt;option&gt; [&lt;value&gt;]</source>
        <translation>set &lt;option&gt; [&lt;valeur&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2387"/>
        <source>Display the encrypted Electrum-style mnemonic seed.</source>
        <translation>Afficher la phrase mnémonique de style Electrum chiffrée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2390"/>
        <source>Rescan the blockchain for spent outputs.</source>
        <translation>Rescanner la chaîne de blocs pour trouver les sorties dépensées.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2393"/>
        <source>get_tx_key &lt;txid&gt;</source>
        <translation>get_tx_key &lt;ID_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2394"/>
        <source>Get the transaction key (r) for a given &lt;txid&gt;.</source>
        <translation>Obtenir la clé de transaction (r) pour un &lt;ID_transaction&gt; donné.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2401"/>
        <source>check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</source>
        <translation>check_tx_key &lt;ID_transaction&gt; &lt;clé_transaction&gt; &lt;adresse&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2402"/>
        <source>Check the amount going to &lt;address&gt; in &lt;txid&gt;.</source>
        <translation>Vérifier le montant allant à &lt;adresse&gt; dans &lt;ID_transaction&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2405"/>
        <source>get_tx_proof &lt;txid&gt; &lt;address&gt; [&lt;message&gt;]</source>
        <translation>get_tx_proof &lt;ID_transaction&gt; &lt;adresse&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2406"/>
        <source>Generate a signature proving funds sent to &lt;address&gt; in &lt;txid&gt;, optionally with a challenge string &lt;message&gt;, using either the transaction secret key (when &lt;address&gt; is not your wallet&apos;s address) or the view secret key (otherwise), which does not disclose the secret key.</source>
        <translation>Générer une signature prouvant l&apos;envoi de fonds à &lt;adresse&gt; dans &lt;ID_transaction&gt;, optionnellement avec un &lt;message&gt; comme challenge, en utilisant soit la clé secrète de transaction (quand &lt;adresse&gt; n&apos;est pas l&apos;adresse de votre portefeuille) soit la clé secrète d&apos;audit (dans le cas contraire), tout en ne divulgant pas la clé secrète.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2409"/>
        <source>check_tx_proof &lt;txid&gt; &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>check_tx_proof &lt;ID_transaction&gt; &lt;adresse&gt; &lt;fichier_signature&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2410"/>
        <source>Check the proof for funds going to &lt;address&gt; in &lt;txid&gt; with the challenge string &lt;message&gt; if any.</source>
        <translation>Vérifier la validité de la preuve de fonds allant à &lt;adresse&gt; dans &lt;ID_transaction&gt; avec le &lt;message&gt; de challenge s&apos;il y en a un.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2413"/>
        <source>get_spend_proof &lt;txid&gt; [&lt;message&gt;]</source>
        <translation>get_spend_proof &lt;ID_transaction&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2414"/>
        <source>Generate a signature proving that you generated &lt;txid&gt; using the spend secret key, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Générer une signature prouvant que vous avez créé &lt;ID_transaction&gt; en utilisant la clé secrète de dépense, optionnellement avec un &lt;message&gt; comme challenge.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2417"/>
        <source>check_spend_proof &lt;txid&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>check_spend_proof &lt;ID_transaction&gt; &lt;fichier_signature&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2418"/>
        <source>Check a signature proving that the signer generated &lt;txid&gt;, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Vérifier la validité de la preuve que le signataire a créé &lt;ID_transaction&gt;, optionnellement avec un &lt;message&gt; comme challenge.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2421"/>
        <source>get_reserve_proof (all|&lt;amount&gt;) [&lt;message&gt;]</source>
        <translation>get_reserve_proof (all|&lt;montant&gt;) [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2422"/>
        <source>Generate a signature proving that you own at least this much, optionally with a challenge string &lt;message&gt;.
If &apos;all&apos; is specified, you prove the entire sum of all of your existing accounts&apos; balances.
Otherwise, you prove the reserve of the smallest possible amount above &lt;amount&gt; available in your current account.</source>
        <translation>Générer une signature prouvant que vous possédez au moins ce montant, optionnellement avec un &lt;message&gt; comme challenge.
Si &apos;all&apos; est spécifié, vous prouvez la somme totale des soldes de tous vos comptes existants.
Sinon, vous prouvez le plus petit solde supérieur à &lt;montant&gt; dans votre compte actuel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2427"/>
        <source>check_reserve_proof &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>check_reserve_proof &lt;adresse&gt; &lt;fichier_signature&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2428"/>
        <source>Check a signature proving that the owner of &lt;address&gt; holds at least this much, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Vérifier la validité d&apos;une signature prouvant que le propriétaire d&apos;une &lt;adresse&gt; détient au moins un montant, optionnellement avec un &lt;message&gt; comme challenge.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2432"/>
        <source>Show the incoming/outgoing transfers within an optional height range.</source>
        <translation>Afficher les transferts entrants/sortants dans un interval de hauteurs facultatif.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2435"/>
        <source>unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_amount&gt; [&lt;max_amount&gt;]]</source>
        <translation>unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;montant_min&gt; [&lt;montant_max&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2436"/>
        <source>Show the unspent outputs of a specified address within an optional amount range.</source>
        <translation>Afficher les sorties non dépensées d&apos;une adresse spécifique dans un interval de montants facultatif.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2442"/>
        <source>set_tx_note &lt;txid&gt; [free text note]</source>
        <translation>set_tx_note &lt;ID_transaction&gt; [texte de la note]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2443"/>
        <source>Set an arbitrary string note for a &lt;txid&gt;.</source>
        <translation>Définir un texte arbitraire comme note pour &lt;ID_transaction&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2446"/>
        <source>get_tx_note &lt;txid&gt;</source>
        <translation>get_tx_note &lt;ID_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2447"/>
        <source>Get a string note for a txid.</source>
        <translation>Obtenir le texte de la note pour &lt;ID_transaction&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2450"/>
        <source>set_description [free text note]</source>
        <translation>set_description [texte]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2451"/>
        <source>Set an arbitrary description for the wallet.</source>
        <translation>Définir un texte arbitraire comme description du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2454"/>
        <source>Get the description of the wallet.</source>
        <translation>Obtenir la description du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2457"/>
        <source>Show the wallet&apos;s status.</source>
        <translation>Afficher l&apos;état du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2460"/>
        <source>Show the wallet&apos;s information.</source>
        <translation>Afficher les informations du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2463"/>
        <source>sign &lt;file&gt;</source>
        <translation>sign &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2464"/>
        <source>Sign the contents of a file.</source>
        <translation>Signer le contenu d&apos;un fichier.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2467"/>
        <source>verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation>verify &lt;fichier&gt; &lt;adresse&gt; &lt;signature&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2468"/>
        <source>Verify a signature on the contents of a file.</source>
        <translation>Vérifier la signature du contenu d&apos;un fichier.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2471"/>
        <source>export_key_images &lt;file&gt;</source>
        <translation>export_key_images &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2472"/>
        <source>Export a signed set of key images to a &lt;file&gt;.</source>
        <translation>Exported un ensemble signé d&apos;images de clé vers un &lt;fichier&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2475"/>
        <source>import_key_images &lt;file&gt;</source>
        <translation>import_key_images &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2476"/>
        <source>Import a signed key images list and verify their spent status.</source>
        <translation>Importer un ensemble signé d&apos;images de clé et vérifier si elles correspondent à des dépenses.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2483"/>
        <source>export_outputs &lt;file&gt;</source>
        <translation>export_outputs &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2484"/>
        <source>Export a set of outputs owned by this wallet.</source>
        <translation>Exporter un ensemble de sorties possédées par ce portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2487"/>
        <source>import_outputs &lt;file&gt;</source>
        <translation>import_outputs &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2488"/>
        <source>Import a set of outputs owned by this wallet.</source>
        <translation>Importer un ensemble de sorties possédées par ce portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2491"/>
        <source>show_transfer &lt;txid&gt;</source>
        <translation>show_transfer &lt;ID_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2492"/>
        <source>Show information about a transfer to/from this address.</source>
        <translation>Afficher les information à propos d&apos;un transfert vers/depuis cette adresse.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2495"/>
        <source>Change the wallet&apos;s password.</source>
        <translation>Changer le mot de passe du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2498"/>
        <source>Generate a new random full size payment id. These will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids.</source>
        <translation>Générer un nouvel ID de paiement long aléatoire. Ceux-ci sont en clair dans la chaîne de blocs, voir integrated_address pour les IDs de paiement courts cryptés.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2501"/>
        <source>Print the information about the current fee and transaction backlog.</source>
        <translation>Afficher les informations à propos des frais et arriéré de transactions actuels.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2503"/>
        <source>Export data needed to create a multisig wallet</source>
        <translation>Exporter les données nécessaires pour créer un portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2505"/>
        <source>make_multisig &lt;threshold&gt; &lt;string1&gt; [&lt;string&gt;...]</source>
        <translation>make_multisig &lt;seuil&gt; &lt;chaîne_caractères1&gt; [&lt;chaîne_caractères&gt;...]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2506"/>
        <source>Turn this wallet into a multisig wallet</source>
        <translation>Transformer ce portefeuille en portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2509"/>
        <source>finalize_multisig &lt;string&gt; [&lt;string&gt;...]</source>
        <translation>finalize_multisig &lt;chaîne_caractères&gt; [&lt;chaîne_caractères&gt;...]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2510"/>
        <source>Turn this wallet into a multisig wallet, extra step for N-1/N wallets</source>
        <translation>Transformer ce portefeuille en portefeuille multisig, étape supplémentaire pour les portefeuilles N-1/N</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2513"/>
        <source>export_multisig_info &lt;filename&gt;</source>
        <translation>export_multisig_info &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2514"/>
        <source>Export multisig info for other participants</source>
        <translation>Exporter les infos multisig pour les autres participants</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2517"/>
        <source>import_multisig_info &lt;filename&gt; [&lt;filename&gt;...]</source>
        <translation>import_multisig_info &lt;fichier&gt; [&lt;fichier&gt;...]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2518"/>
        <source>Import multisig info from other participants</source>
        <translation>Importer les infos multisig des autres participants</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2521"/>
        <source>sign_multisig &lt;filename&gt;</source>
        <translation>sign_multisig &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2522"/>
        <source>Sign a multisig transaction from a file</source>
        <translation>Signer une transaction multisig d&apos;un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2525"/>
        <source>submit_multisig &lt;filename&gt;</source>
        <translation>submit_multisig &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2526"/>
        <source>Submit a signed multisig transaction from a file</source>
        <translation>Transmettre une transaction multisig signée d&apos;un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2529"/>
        <source>export_raw_multisig_tx &lt;filename&gt;</source>
        <translation>export_raw_multisig_tx &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2530"/>
        <source>Export a signed multisig transaction to a file</source>
        <translation>Exporter une transaction multisig signée vers un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2561"/>
        <source>help [&lt;command&gt;]</source>
        <translation>help [&lt;commande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2562"/>
        <source>Show the help section or the documentation about a &lt;command&gt;.</source>
        <translation>Afficher la section d&apos;aide ou la documentation d&apos;une &lt;commande&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2644"/>
        <source>integer &gt;= </source>
        <translation>entier &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2657"/>
        <source>block height</source>
        <translation>hauteur de bloc</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2760"/>
        <source>No wallet found with that name. Confirm creation of new wallet named: </source>
        <translation>Aucun portefeuille avec ce nom trouvé. Confirmer la création d&apos;un nouveau portefeuille nommé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2861"/>
        <source>can&apos;t specify both --restore-deterministic-wallet or --restore-multisig-wallet and --non-deterministic</source>
        <translation>impossible de spécifier à la fois --restore-deterministic-wallet ou --restore-multisig-wallet et --non-deterministic</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2867"/>
        <source>--restore-multisig-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-multisig-wallet utilise --generate-new-wallet, pas --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2883"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;multisig seed here&quot;</source>
        <translation>spécifiez un paramètre de récupération avec --electrum-seed=&quot;phrase mnémonique multisig ici&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2912"/>
        <source>Multisig seed failed verification</source>
        <translation>Échec de la vérification de la phrase mnémonique multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2963"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3038"/>
        <source>This address is a subaddress which cannot be used here.</source>
        <translation>Cette adresse est une sous-adresse qui ne peut pas être utilisée ici.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3115"/>
        <source>Error: expected M/N, but got: </source>
        <translation>Erreur&#xa0;: M/N attendu, mais lu&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3120"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got: </source>
        <translation>Erreur&#xa0;: N &gt; 1 et N &lt;= M attendu, mais lu&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3125"/>
        <source>Error: M/N is currently unsupported. </source>
        <translation>Erreur&#xa0;: M/N n&apos;est actuellement pas supporté. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3128"/>
        <source>Generating master wallet from %u of %u multisig wallet keys</source>
        <translation>Génération du portefeuille principal à partir de %u de %u clés de portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3157"/>
        <source>failed to parse secret view key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3165"/>
        <source>failed to verify secret view key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3185"/>
        <source>Secret spend key (%u of %u):</source>
        <translation>Clé secrète de dépense (%u de %u)&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3208"/>
        <source>Error: M/N is currently unsupported</source>
        <translation>Erreur&#xa0;: M/N n&apos;est actuellement pas supporté</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3359"/>
        <source>Restore height </source>
        <translation>Hauteur de restauration </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3360"/>
        <source>Still apply restore height?  (Y/Yes/N/No): </source>
        <translation>Appliquer la hauteur de restauration quand même ?  (Y/Yes/Oui/N/No/Non)&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3386"/>
        <source>Warning: using an untrusted daemon at %s, privacy will be lessened</source>
        <translation>Attention&#xa0;: en n&apos;utilisant %s qui n&apos;est pas un démon de confiance, la confidentialité sera réduite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3445"/>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or change the daemon address using the &apos;set_daemon&apos; command.</source>
        <translation>Le démon n&apos;est pas lancé ou un mauvais port a été fourni. Veuillez vous assurer que le démon fonctionne ou changez l&apos;adresse de démon avec la commande &apos;set_daemon&apos;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3593"/>
        <source>Your wallet has been generated!
To start synchronizing with the daemon, use the &quot;refresh&quot; command.
Use the &quot;help&quot; command to see the list of available commands.
Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
Always use the &quot;exit&quot; command when closing monero-wallet-cli to save 
your current session&apos;s state. Otherwise, you might need to synchronize 
your wallet again (your wallet keys are NOT at risk in any case).
</source>
        <translation>Votre portefeuille a été généré !
Pour commencer la synchronisation avec le démon, utilisez la commande &quot;refresh&quot;.
Utilisez la commande &quot;help&quot; pour voir la liste des commandes disponibles.
Utilisez &quot;help &lt;commande&gt;&quot; pour voir la documentation d&apos;une commande.
Utilisez toujours la commande &quot;exit&quot; pour fermer monero-wallet-cli pour sauvegarder 
l&apos;état de votre session. Sinon, vous pourriez avoir besoin de synchroniser 
votre portefeuille à nouveau (mais les clés de votre portefeuille ne risquent rien).
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3734"/>
        <source>failed to generate new mutlisig wallet</source>
        <translation>échec de la génération du nouveau portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3737"/>
        <source>Generated new %u/%u multisig wallet: </source>
        <translation>Nouveau portefeuille multisig %u/%u généré&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3784"/>
        <source>Opened %u/%u multisig wallet%s</source>
        <translation>Portefeuille multisig %u/%u ouvert%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3845"/>
        <source>Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
</source>
        <translation>Utilisez &quot;help &lt;commande&gt;&quot; pour voir la documentation d&apos;une commande.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3903"/>
        <source>wallet is multisig and cannot save a watch-only version</source>
        <translation>c&apos;est un portefeuille multisig et il ne peut pas sauvegarder une version d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4017"/>
        <source>missing daemon URL argument</source>
        <translation>URL du démon manquante en argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4028"/>
        <source>Unexpected array length - Exited simple_wallet::set_daemon()</source>
        <translation>Taille de tableau inattendue - Sortie de simple_wallet::set_daemon()</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4069"/>
        <source>This does not seem to be a valid daemon URL.</source>
        <translation>Ceci semble ne pas être une URL de démon valide.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4105"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4142"/>
        <source>txid </source>
        <translation>ID transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4107"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4144"/>
        <source>idx </source>
        <translation>index </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4275"/>
        <source> (Some owned outputs have partial key images - import_multisig_info needed)</source>
        <translation> (Certaines sorties ont des images de clé partielles - import_multisig_info requis)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4278"/>
        <source>Currently selected account: [</source>
        <translation>Compte actuellement sélectionné&#xa0;: [</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4278"/>
        <source>] </source>
        <translation>] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4280"/>
        <source>Tag: </source>
        <translation>Mot clé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4280"/>
        <source>(No tag assigned)</source>
        <translation>(Pas de mot clé assigné)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4287"/>
        <source>Balance per address:</source>
        <translation>Solde par adresse&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4288"/>
        <source>Address</source>
        <translation>Adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4288"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7192"/>
        <source>Balance</source>
        <translation>Solde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4288"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7192"/>
        <source>Unlocked balance</source>
        <translation>Solde débloqué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4288"/>
        <source>Outputs</source>
        <translation>Sorties</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4288"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7192"/>
        <source>Label</source>
        <translation>Étiquette</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4296"/>
        <source>%8u %6s %21s %21s %7u %21s</source>
        <translation>%8u %6s %21s %21s %7u %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4305"/>
        <source>usage: balance [detail]</source>
        <translation>usage&#xa0;: balance [detail]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4317"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4359"/>
        <source>usage: incoming_transfers [available|unavailable] [verbose] [index=&lt;N&gt;]</source>
        <translation>usage: incoming_transfers [available|unavailable] [verbose] [index=&lt;N&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4378"/>
        <source>spent</source>
        <translation>dépensé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4378"/>
        <source>global index</source>
        <translation>index global</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4378"/>
        <source>tx id</source>
        <translation>ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4378"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4429"/>
        <source>addr index</source>
        <translation>index adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4401"/>
        <source>No incoming transfers</source>
        <translation>Aucun transfert entrant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4405"/>
        <source>No incoming available transfers</source>
        <translation>Aucun transfert entrant disponible</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4409"/>
        <source>No incoming unavailable transfers</source>
        <translation>Aucun transfert entrant non disponible</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4420"/>
        <source>expected at least one payment ID</source>
        <translation>au moins un ID de paiement attendu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4429"/>
        <source>payment</source>
        <translation>paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4429"/>
        <source>transaction</source>
        <translation>transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4429"/>
        <source>height</source>
        <translation>hauteur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4429"/>
        <source>unlock time</source>
        <translation>durée de déverrouillage</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4441"/>
        <source>No payments with id </source>
        <translation>Aucun paiement avec l&apos;ID </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4489"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4555"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4892"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5305"/>
        <source>failed to get blockchain height: </source>
        <translation>échec de la récupération de la hauteur de la chaîne de blocs&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4545"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6388"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6426"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6483"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6514"/>
        <source>failed to connect to the daemon</source>
        <translation>échec de la connexion au démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4563"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation>
Transaction %llu/%llu&#xa0;: ID=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4584"/>
        <source>
Input %llu/%llu: amount=%s</source>
        <translation>
Entrée %llu/%llu&#xa0;: montant=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4600"/>
        <source>failed to get output: </source>
        <translation>échec de la récupération de la sortie&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4608"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation>la hauteur du bloc d&apos;origine de la clé de la sortie ne devrait pas être supérieure à celle de la chaîne de blocs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4612"/>
        <source>
Originating block heights: </source>
        <translation>
Hauteurs des blocs d&apos;origine&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4627"/>
        <source>
|</source>
        <translation>
|</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4627"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6915"/>
        <source>|
</source>
        <translation>|
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4644"/>
        <source>
Warning: Some input keys being spent are from </source>
        <translation>
Attention&#xa0;: Certaines clés d&apos;entrées étant dépensées sont issues de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4646"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation>, ce qui peut casser l&apos;anonymat du cercle de signature. Assurez-vous que c&apos;est intentionnel !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4688"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5257"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5536"/>
        <source>Ring size must not be 0</source>
        <translation>La taille de cercle ne doit pas être 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4700"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5269"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5548"/>
        <source>ring size %u is too small, minimum is %u</source>
        <translation>la taille de cercle %u est trop petite, le minimum est %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4712"/>
        <source>wrong number of arguments</source>
        <translation>mauvais nombre d&apos;arguments</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4869"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5400"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5648"/>
        <source>No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): </source>
        <translation>Aucun ID de paiement n&apos;est inclus dans cette transaction. Est-ce correct ? (Y/Yes/Oui/N/No/Non)&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4908"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5420"/>
        <source>No outputs found, or daemon is not ready</source>
        <translation>Aucune sortie trouvée, ou le démon n&apos;est pas prêt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7740"/>
        <source>command only supported by HW wallet</source>
        <translation>commande supportée uniquement par un portefeuille matériel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7749"/>
        <source>Failed to reconnect device</source>
        <translation>Échec de la reconnexion à l&apos;appareil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7754"/>
        <source>Failed to reconnect device: </source>
        <translation>Échec de la reconnexion à l&apos;appareil : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8017"/>
        <source>Transaction successfully saved to </source>
        <translation>Transaction sauvegardée avec succès dans </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8017"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8019"/>
        <source>, txid </source>
        <translation>, ID transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8019"/>
        <source>Failed to save transaction to </source>
        <translation>Échec de la sauvegarde de la transaction dans </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5134"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5448"/>
        <source>Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Balayage de %s dans %llu transactions pour des frais totaux de %s. Est-ce correct ? (Y/Yes/Oui/N/No/Non)&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5140"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5454"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5688"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Balayage de %s pour des frais totaux de %s. Est-ce correct ? (Y/Yes/Oui/N/No/Non)&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5966"/>
        <source>This is a watch only wallet</source>
        <translation>Ceci est un portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7841"/>
        <source>usage: show_transfer &lt;txid&gt;</source>
        <translation>usage&#xa0;: show_transfer &lt;ID_de_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7947"/>
        <source>Double spend seen on the network: this transaction may or may not end up being mined</source>
        <translation>Double dépense détectée sur le réseau&#xa0;: cette transaction sera peut-être invalidée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7982"/>
        <source>Transaction ID not found</source>
        <translation>ID de transaction non trouvé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="243"/>
        <source>true</source>
        <translation>vrai</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="296"/>
        <source>failed to parse refresh type</source>
        <translation>échec de l&apos;analyse du type de rafraîchissement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="628"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="693"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="842"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="875"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="954"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1006"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1058"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1137"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1211"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1279"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5956"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6020"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6057"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6154"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6365"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6455"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7581"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7656"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7698"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7765"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7804"/>
        <source>command not supported by HW wallet</source>
        <translation>commande non supportée par le portefeuille matériel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="633"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="703"/>
        <source>wallet is watch-only and has no seed</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il n&apos;a pas de phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="650"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="713"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation>c&apos;est un portefeuille non déterministe et il n&apos;a pas de phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="657"/>
        <source>Enter optional seed offset passphrase, empty to see raw seed</source>
        <translation>Entrer une phrase de passe facultative pour le décalage de la phrase mnémonique, effacer pour voir la phrase mnémonique brute</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="723"/>
        <source>Incorrect password</source>
        <translation>Mot de passe invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="792"/>
        <source>Current fee is %s %s per %s</source>
        <translation>Les frais sont actuellement de %s %s par %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1356"/>
        <source>usage: print_ring &lt;key_image|txid&gt;</source>
        <translation>usage&#xa0;: print_ring &lt;image clé|ID transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1362"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1519"/>
        <source>Invalid key image</source>
        <translation>Image de clé invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1368"/>
        <source>Invalid txid</source>
        <translation>ID de transaction invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1380"/>
        <source>Key image either not spent, or spent with mixin 0</source>
        <translation>Image de clé soit non dépensée, soit dépensée avec 0 mélange</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1395"/>
        <source>Failed to get key image ring: </source>
        <translation>Échec de la récupération du cercle de l&apos;image de clé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1410"/>
        <source>File doesn&apos;t exist</source>
        <translation>Le fichier d&apos;existe pas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1432"/>
        <source>Invalid ring specification: </source>
        <translation>Spécification de cercle invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1440"/>
        <source>Invalid key image: </source>
        <translation>Image de clé invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1445"/>
        <source>Invalid ring type, expected relative or abosolute: </source>
        <translation>Type de cercle invalide, &quot;relative&quot; ou &quot;absolute&quot; attendu : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1451"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1463"/>
        <source>Error reading line: </source>
        <translation>Erreur lors de la lecture de la ligne : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1474"/>
        <source>Invalid ring: </source>
        <translation>Cercle invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1483"/>
        <source>Invalid relative ring: </source>
        <translation>Cercle relatif invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1495"/>
        <source>Invalid absolute ring: </source>
        <translation>Cercle absolu invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1504"/>
        <source>Failed to set ring for key image: </source>
        <translation>Échec de l&apos;affectation du cercle pour l&apos;image de clé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1504"/>
        <source>Continuing.</source>
        <translation>On continue.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1513"/>
        <source>usage: set_ring &lt;filename&gt; | ( &lt;key_image&gt; absolute|relative &lt;index&gt; [&lt;index&gt;...] )</source>
        <translation>usage&#xa0;: set_ring &lt;nom_fichier&gt; | ( &lt;image_clé&gt; absolute|relative &lt;index&gt; [&lt;index&gt;...] )</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1534"/>
        <source>Missing absolute or relative keyword</source>
        <translation>Mot clé &quot;absolute&quot; ou &quot;relative&quot; manquant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1544"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1551"/>
        <source>invalid index: must be a strictly positive unsigned integer</source>
        <translation>index invalide : doit être un nombre entier strictement positif</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1559"/>
        <source>invalid index: indices wrap</source>
        <translation>index invalide : boucle des indices</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1569"/>
        <source>invalid index: indices should be in strictly ascending order</source>
        <translation>index invalide : les indices doivent être en ordre strictement croissant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1576"/>
        <source>failed to set ring</source>
        <translation>échec de l&apos;affectation du cercle</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1588"/>
        <source>usage: blackball &lt;amount&gt;/&lt;offset&gt; | &lt;filename&gt; [add]</source>
        <translation>usage : blackball &lt;montant&gt;/&lt;offset&gt; | &lt;nom_fichier&gt; [add]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1621"/>
        <source>First line is not an amount</source>
        <translation>La première ligne n&apos;est pas un montant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1635"/>
        <source>Invalid output: </source>
        <translation>Sortie invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1645"/>
        <source>Bad argument: </source>
        <translation>Mauvais argument : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1645"/>
        <source>should be &quot;add&quot;</source>
        <translation>devrait être &quot;add&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1654"/>
        <source>Failed to open file</source>
        <translation>Échec de l&apos;ouverture du fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1660"/>
        <source>Invalid output key, and file doesn&apos;t exist</source>
        <translation>Clé de sortie invalide, et le fichier n&apos;existe pas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1666"/>
        <source>Failed to blackball output: </source>
        <translation>Échec du blackboulage de la sortie : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1677"/>
        <source>usage: unblackball &lt;amount&gt;/&lt;offset&gt;</source>
        <translation>usage : unblackball &lt;montant&gt;/&lt;offset&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1683"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1710"/>
        <source>Invalid output</source>
        <translation>Sortie invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1693"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1723"/>
        <source>Failed to unblackball output: </source>
        <translation>Échec du déblackboulage de la sortie : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1704"/>
        <source>usage: blackballed &lt;amount&gt;/&lt;offset&gt;</source>
        <translation>usage : blackballed &lt;montant&gt;/&lt;offset&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1717"/>
        <source>Blackballed: </source>
        <translation>Blackboulé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1719"/>
        <source>not blackballed: </source>
        <translation>non blackboulé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1738"/>
        <source>Failed to save known rings: </source>
        <translation>Échec de la sauvegarde des cercles connus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1779"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1798"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas transférer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1816"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5031"/>
        <source>WARNING: this is a non default ring size, which may harm your privacy. Default is recommended.</source>
        <translation>ATTENTION : ceci c&apos;est pas la taille de cercle par défaut, ce qui peut nuire à votre confidentialité. La valeur par défaut est recommandée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1818"/>
        <source>WARNING: from v8, ring size will be fixed and this setting will be ignored.</source>
        <translation>ATTENTION : ) partir de v8, la taille de cercle sera fixée et ce paramètre sera ignoré.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1847"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1870"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1886"/>
        <source>priority must be either 0, 1, 2, 3, or 4, or one of: </source>
        <translation>la priorité doit être 0, 1, 2, 3, 4 ou l&apos;une de : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1891"/>
        <source>could not change default priority</source>
        <translation>échec du changement de la priorité par défaut</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1959"/>
        <source>invalid argument: must be either 0/never, 1/action, or 2/encrypt/decrypt</source>
        <translation>argument invalide : doit être soit 0/never, 1/action, ou 2/encrypt/decrypt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2232"/>
        <source>set_daemon &lt;host&gt;[:&lt;port&gt;] [trusted|untrusted]</source>
        <translation>set_daemon &lt;hôte&gt;[:&lt;port&gt;] [trusted|untrusted]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2256"/>
        <source>transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] (&lt;URI&gt; | &lt;address&gt; &lt;amount&gt;) [&lt;payment_id&gt;]</source>
        <translation>transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] (&lt;URI&gt; | &lt;adresse&gt; &lt;montant&gt;) [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2257"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt;. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Transférer &lt;montant&gt; à &lt;adresse&gt; Si le paramètre &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille utilise les sorties reçues par les adresses de ces indices. Si il est omis, le portefeuille choisit les indices d&apos;adresse à utiliser aléatoirement. Dans tous les cas, il essaye de ne pas combiner des sorties de multiples adresses. &lt;priorité&gt; est la priorité de la transaction. Plus la priorité est haute, plus les frais de transaction sont élévés. Les valeurs valides par ordre de priorité (de la plus basse à la plus haute) sont&#xa0;: unimportant, normal, elevated, priority. Si elle est omise, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;taille_cercle&gt; est le nombre d&apos;entrées à inclure pour l&apos;intraçabilité. De multiples paiements peuvent être réalisés d&apos;un coup en ajoutant &lt;URI_2&gt; ou &lt;adresse_2&gt; &lt;montant_2&gt; et cetera (avant l&apos;ID de paiement, si il est inclus)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2260"/>
        <source>locked_transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] (&lt;URI&gt; | &lt;addr&gt; &lt;amount&gt;) &lt;lockblocks&gt; [&lt;payment_id&gt;]</source>
        <translation>locked_transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] (&lt;URI&gt; | &lt;adresse&gt; &lt;montant&gt;) &lt;blocs_verrou&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2261"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Transférer &lt;montant&gt; à &lt;adresse&gt; et le verrouiller pendant &lt;blocs_verrou&gt; (max 1000000). Si le paramètre &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille utilise les sorties reçues par les adresses de ces indices. Si il est omis, le portefeuille choisit les indices d&apos;adresse à utiliser aléatoirement. Dans tous les cas, il essaye de ne pas combiner des sorties de multiples adresses. &lt;priorité&gt; est la priorité de la transaction. Plus la priorité est haute, plus les frais de transaction sont élévés. Les valeurs valides par ordre de priorité (de la plus basse à la plus haute) sont&#xa0;: unimportant, normal, elevated, priority. Si elle est omise, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;taille_cercle&gt; est le nombre d&apos;entrées à inclure pour l&apos;intraçabilité. De multiples paiements peuvent être réalisés d&apos;un coup en ajoutant &lt;URI_2&gt; ou &lt;adresse_2&gt; &lt;montant_2&gt; et cetera (avant l&apos;ID de paiement, si il est inclus)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2264"/>
        <source>locked_sweep_all [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; &lt;lockblocks&gt; [&lt;payment_id&gt;]</source>
        <translation>locked_sweep_all [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] &lt;adresse&gt; &lt;blocs_verrou&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2265"/>
        <source>Send all unlocked balance to an address and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. &lt;priority&gt; is the priority of the sweep. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability.</source>
        <translation>Transférer tout le solde débloqué à une adresse et le verrouiller pendant &lt;blocs_verrou&gt; (max 1000000). Si le paramètre &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille utilise les sorties reçues par ces indices d&apos;adresse. Si il est omis, le portefeuille choisit un index d&apos;adresse à utiliser aléatoirement. &lt;priorité&gt; est la priorité du balayage. Plus la priorité est haute, plus les frais de transaction sont élévés. Les valeurs valides par ordre de priorité (de la plus basse à la plus haute) sont&#xa0;: unimportant, normal, elevated, priority. Si elle est omise, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;taille_cercle&gt; est le nombre d&apos;entrées à inclure pour l&apos;intraçabilité.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2270"/>
        <source>sweep_all [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] [outputs=&lt;N&gt;] &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation>sweep_all [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] [outputs=&lt;N&gt;] &lt;adresse&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2271"/>
        <source>Send all unlocked balance to an address. If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. If the parameter &quot;outputs=&lt;N&gt;&quot; is specified and  N &gt; 0, wallet splits the transaction into N even outputs.</source>
        <translation>Envoyer tout le solde débloqué à une adresse. Si le paramètre &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille balaye les sorties reçues par ces indices d&apos;adresse. Si il est omis, le portefeuille choisit un index d&apos;adresse à utiliser aléatoirement. Si le paramètre &quot;outputs=&lt;N&gt;&quot; est spécifié et N &gt; 0, le portefeuille scinde la transaction en N sorties égales.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2278"/>
        <source>sweep_single [&lt;priority&gt;] [&lt;ring_size&gt;] [outputs=&lt;N&gt;] &lt;key_image&gt; &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation>sweep_single [&lt;priorité&gt;] [&lt;taille_cercle&gt;] [outputs=&lt;N&gt;] &lt;image_clé&gt; &lt;adresse&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2286"/>
        <source>sign_transfer [export_raw]</source>
        <translation>sign_transfer [export_raw]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2287"/>
        <source>Sign a transaction from a file. If the parameter &quot;export_raw&quot; is specified, transaction raw hex data suitable for the daemon RPC /sendrawtransaction is exported.</source>
        <translation>Signer une transaction à partir d&apos;un fichier. Si le paramètre &quot;export_raw&quot; est spécifié, les données brutes hexadécimales adaptées au RPC /sendrawtransaction du démon sont exportées.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2314"/>
        <source>If no arguments are specified or &lt;index&gt; is specified, the wallet shows the default or specified address. If &quot;all&quot; is specified, the wallet shows all the existing addresses in the currently selected account. If &quot;new &quot; is specified, the wallet creates a new address with the provided label text (which can be empty). If &quot;label&quot; is specified, the wallet sets the label of the address specified by &lt;index&gt; to the provided label text.</source>
        <translation>Si aucun argument n&apos;est spécifié ou si &lt;index&gt; est spécifié, le portefeuille affiche l&apos;adresse par défaut ou l&apos;adresse spécifiée. Si &quot;all&quot; est spécifié, le portefeuille affiche toutes les adresses existantes dans le comptes actuellement sélectionné. Si &quot;new&quot; est spécifié, le portefeuille crée une nouvelle adresse avec le texte d&apos;étiquette fourni (qui peut être vide). Si &quot;label&quot; est spécifié, le portefeuille affecte le texte fourni à l&apos;étiquette de l&apos;adresse spécifiée par &lt;index&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2341"/>
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
        <translation>Options disponibles :
 seed langue
   Définir la langue de la phrase mnémonique.
 always-confirm-transfers &lt;1|0&gt;
   Confirmation des transactions non scindées.
 print-ring-members &lt;1|0&gt;
   Affichage d&apos;informations détaillées sur les membres du cercle lors de la confirmation.
 store-tx-info &lt;1|0&gt;
   Sauvegarde des informations des transactions sortantes (adresse de destination, ID de paiement, clé secrète de transaction) pour référence ultérieure.
 default-ring-size &lt;n&gt;
   Définir la taille de cercle par défaut (la valeur par défaut est le minimum 5).
 auto-refresh &lt;1|0&gt;
   Synchronisation automatique des nouveaux blocs du démon.
 refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt;
   Définir le comportement du rafraîchissement du portefeuille.
 priority [0|1|2|3|4]
   Utiliser les frais pour la priorité par défaut/peu importante/normale/élevée/prioritaire.
 confirm-missing-payment-id &lt;1|0&gt;
 ask-password &lt;0|1|2 (ou never|action|decrypt)&gt;
 unit &lt;monero|millinero|micronero|nanonero|piconero&gt;
   Définir la (sous-)unité monero par défaut.
 min-outputs-count [n]
   Essayer de garder au moins ce nombre de sorties d&apos;une valeur d&apos;au moins min-outputs-value.
 min-outputs-value [n]
   Essayer de garder au moins min-outputs-count sorties d&apos;au moins cette valeur.
 merge-destinations &lt;1|0&gt;
   Fusion des paiements multiples vers la même adresse de destination.
 confirm-backlog &lt;1|0&gt;
   Avertir s&apos;il y a un arriéré de transactions.
 confirm-backlog-threshold [n]
   Définir un seuil pour confirm-backlog pour avertir seulement si l&apos;arriéré de transactions est supérieur à n blocs.
 refresh-from-block-height [n]
   Définir la hauteur avant laquelle les blocs sont ignorés.
 auto-low-priority &lt;1|0&gt;
   Utilisation automatique du niveau de frais pour la priorité basse, lorsqu&apos;il est sûr de le faire.
 segregate-pre-fork-outputs &lt;1|0&gt;
   Activez ceci si vous prévoyez de dépenser des sorties à la fois avec Monero ET un fork réutilisant les clés.
 key-reuse-mitigation2 &lt;1|0&gt;
   Activez ceci si vous n&apos;êtes pas sûr de ne jamais utiliser un fork réutilisant les clés.
 subaddress-lookahead &lt;majeur&gt;:&lt;mineur&gt;
   Définir les nombres de sous-adresse à inclure par anticipation dans la table de hachage des sous-adresses.
 segregation-height &lt;n&gt;
   Définir la hauteur d&apos;un fork réutilisant les clés que vous voulez utiliser, 0 par défaut.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2397"/>
        <source>set_tx_key &lt;txid&gt; &lt;tx_key&gt;</source>
        <translation>set_tx_key &lt;ID_transaction&gt; &lt;clé_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2398"/>
        <source>Set the transaction key (r) for a given &lt;txid&gt; in case the tx was made by some other device or 3rd party wallet.</source>
        <translation>Définir la clé de transaction (r) pour un &lt;ID_transaction&gt; donné au cas où cette clé ait été créée par un appareil ou portefeuille tiers.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2431"/>
        <source>show_transfers [in|out|pending|failed|pool|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation>show_transfers [in|out|pending|failed|pool|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;hauteur_min&gt; [&lt;hauteur_max&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2439"/>
        <source>Rescan the blockchain from scratch, losing any information which can not be recovered from the blockchain itself.</source>
        <translation>Rescanner la chaîne de blocs à partir du début, en perdant toute information qui ne peut pas être retrouvée à partir de la chaîne elle même.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2479"/>
        <source>hw_reconnect</source>
        <translation>hw_reconnect</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2480"/>
        <source>Attempts to reconnect HW wallet.</source>
        <translation>Essayer de se reconnecter à un portefeuille matériel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2533"/>
        <source>print_ring &lt;key_image&gt; | &lt;txid&gt;</source>
        <translation>print_ring &lt;image_clé&gt; | &lt;ID_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2534"/>
        <source>Print the ring(s) used to spend a given key image or transaction (if the ring size is &gt; 1)</source>
        <translation>Afficher le(s) cercle(s) utilisé(s) pour dépenser une image de clé ou une transaction (si la taille de cercle est &gt; 1)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2537"/>
        <source>set_ring &lt;filename&gt; | ( &lt;key_image&gt; absolute|relative &lt;index&gt; [&lt;index&gt;...] )</source>
        <translation>set_ring &lt;nom_fichier&gt; | ( &lt;image_clé&gt; absolute|relative &lt;index&gt; [&lt;index&gt;...] )</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2538"/>
        <source>Set the ring used for a given key image, so it can be reused in a fork</source>
        <translation>Définir le cercle utilisé pour une image de clé donnée, afin de pouvoir le réutiliser dans un fork</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2541"/>
        <source>save_known_rings</source>
        <translation>save_known_rings</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2542"/>
        <source>Save known rings to the shared rings database</source>
        <translation>Sauvegarder les cercles connus dans la base de données des cercles partagés</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2545"/>
        <source>blackball &lt;amount&gt;/&lt;offset&gt; | &lt;filename&gt; [add]</source>
        <translation>blackball &lt;montant&gt;/&lt;offset&gt; | &lt;nom_fichier&gt; [add]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2546"/>
        <source>Blackball output(s) so they never get selected as fake outputs in a ring</source>
        <translation>Blackbouler des sorties pour qu&apos;elles ne soient jamais sélectionnées comme leurres dans un cercle</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2549"/>
        <source>unblackball &lt;amount&gt;/&lt;offset&gt;</source>
        <translation>unblackball &lt;montant&gt;/&lt;offset&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2550"/>
        <source>Unblackballs an output so it may get selected as a fake output in a ring</source>
        <translation>Déblackbouler une sortie pour qu&apos;elle puisse être sélectionnée comme leurre dans un cercle</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2553"/>
        <source>blackballed &lt;amount&gt;/&lt;offset&gt;</source>
        <translation>blackballed &lt;montant&gt;/&lt;offset&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2554"/>
        <source>Checks whether an output is blackballed</source>
        <translation>Vérifier si une sortie est blackboulée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2557"/>
        <source>version</source>
        <translation>version</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2558"/>
        <source>Returns version information</source>
        <translation>Retourne les informations de version</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2646"/>
        <source>full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)</source>
        <translation>full (le plus lent, aucune supposition); optimize-coinbase (rapide, suppose que la récompense de bloc est payée à une unique adresse); no-coinbase (le plus rapide, suppose que l&apos;on ne reçoit aucune récompense de bloc), default (comme optimize-coinbase)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2647"/>
        <source>0, 1, 2, 3, or 4, or one of </source>
        <translation>0, 1, 2, 3, 4 ou l&apos;une de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2649"/>
        <source>0|1|2 (or never|action|decrypt)</source>
        <translation>0|1|2 (ou never|action|decrypt)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2650"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation>monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2661"/>
        <source>&lt;major&gt;:&lt;minor&gt;</source>
        <translation>&lt;majeur&gt;:&lt;mineur&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2684"/>
        <source>wrong number range, use: set_log &lt;log_level_number_0-4&gt; | &lt;categories&gt;</source>
        <translation>nombre hors interval, utilisez&#xa0;: set_log &lt;niveau_de_journalisation_0-4&gt; | &lt;catégories&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2723"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation>Nom de portefeuille non valide. Veuillez réessayer ou utilisez Ctrl-C pour quitter.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2740"/>
        <source>Wallet and key files found, loading...</source>
        <translation>Fichier portefeuille et fichier de clés trouvés, chargement...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2746"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation>Fichier de clés trouvé mais pas le fichier portefeuille. Régénération...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2752"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation>Fichier de clés non trouvé. Échec de l&apos;ouverture du portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2771"/>
        <source>Generating new wallet...</source>
        <translation>Génération du nouveau portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2830"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation>Impossible de spécifier plus d&apos;une option parmis --testnet et --stagenet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2842"/>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-device=&quot;wallet_name&quot;</source>
        <translation>impossible de spécifier plus d&apos;une option parmis --generate-new-wallet=&quot;nom_portefeuille&quot;, --wallet-file=&quot;nom_portefeuille&quot;, --generate-from-view-key=&quot;nom_portefeuille&quot;, --generate-from-spend-key=&quot;nom_portefeuille&quot;, --generate-from-keys=&quot;nom_portefeuille&quot;, --generate-from-multisig-keys=&quot;nom_portefeuille&quot;, --generate-from-json=&quot;nom_fichier_json&quot; et --generate-from-device=&quot;nom_portefeuille&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2921"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2926"/>
        <source>Enter seed offset passphrase, empty if none</source>
        <translation>Entrer une phrase de passe pour le décalage de la phrase mnémonique, vide si aucun</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2952"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2972"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3007"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3027"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3047"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3062"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3110"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3135"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3151"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3190"/>
        <source>No data supplied, cancelled</source>
        <translation>Pas de données fournies, annulation</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2958"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3033"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3141"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4823"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5373"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5623"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6173"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6241"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6305"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6521"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7377"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7636"/>
        <source>failed to parse address</source>
        <translation>échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2978"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3068"/>
        <source>failed to parse view key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2987"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3085"/>
        <source>failed to verify view key secret key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2991"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3089"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3170"/>
        <source>view key does not match standard address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2996"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3016"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3093"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3226"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3252"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3283"/>
        <source>account creation failed</source>
        <translation>échec de la création du compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3012"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3053"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3195"/>
        <source>failed to parse spend key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3077"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3215"/>
        <source>failed to verify spend key secret key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3081"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3220"/>
        <source>spend key does not match standard address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3258"/>
        <source>No restore height is specified.</source>
        <translation>Aucune hauteur de restauration n&apos;est spécifiée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3259"/>
        <source>Assumed you are creating a new account, restore will be done from current estimated blockchain height.</source>
        <translation>Nous supposons que vous créez un nouveau compte, la restauration sera faite à partir d&apos;une hauteur de la chaîne de blocs estimée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3260"/>
        <source>Use --restore-height if you want to restore an already setup account from a specific height</source>
        <translation>Utilisez --restore-height si vous voulez restaurer un compte existant à partir d&apos;une hauteur spécifique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3264"/>
        <source>account creation aborted</source>
        <translation>création du compte annulée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3373"/>
        <source>can&apos;t specify --subaddress-lookahead and --wallet-file at the same time</source>
        <translation>Impossible de spécifier --subaddress-lookahead et --wallet-file en même temps</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3377"/>
        <source>failed to open account</source>
        <translation>échec de l&apos;ouverture du compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3381"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3943"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3996"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4081"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6209"/>
        <source>wallet is null</source>
        <translation>portefeuille est nul</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3389"/>
        <source>Failed to initialize ring database: privacy enhancing features will be inactive</source>
        <translation>Impossible d&apos;initialiser la base de données des cercles : les fonctions d&apos;amélioration de la confidentialité seront inactives</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3474"/>
        <source>If your display freezes, exit blind with ^C, then run again with --use-english-language-names</source>
        <translation>Si votre affichage se bloque, quittez en aveugle avec ^C, puis lancer à nouveau en utilisant --use-english-language-names</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3492"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3497"/>
        <source>invalid language choice entered. Please try again.
</source>
        <translation>choix de langue passé invalide. Veuillez réessayer.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3576"/>
        <source>View key: </source>
        <translation>Clé d&apos;audit&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3684"/>
        <source>Generated new wallet on hw device: </source>
        <translation>Nouveau portefeuille généré sur l&apos;appareil : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3763"/>
        <source>Key file not found. Failed to open wallet</source>
        <translation>Fichier des clés non trouvé. Échec d&apos;ouverture du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3838"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation>Vous pourriez vouloir supprimer le fichier &quot;%s&quot; et réessayer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3866"/>
        <source>failed to deinitialize wallet</source>
        <translation>échec de la désinitialisation du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3919"/>
        <source>Watch only wallet saved as: </source>
        <translation>Portefeuille d&apos;audit sauvegardé vers : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3923"/>
        <source>Failed to save watch only wallet: </source>
        <translation>Échec de la sauvegarde du portefeuille d&apos;audit&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3934"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4497"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7703"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation>cette commande requiert un démon de confiance. Activer avec --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3975"/>
        <source>invalid arguments. Please use start_mining [&lt;number_of_threads&gt;] [do_bg_mining] [ignore_battery]</source>
        <translation>arguments invalides. Veuillez utiliser start_mining [&lt;nombre_de_threads&gt;] [do_bg_mining] [ignore_battery]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4050"/>
        <source>Expected trusted or untrusted, got </source>
        <translation>&quot;trusted&quot; ou &quot;untrusted&quot; attendu, mais lu </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4067"/>
        <source>trusted</source>
        <translation>de confiance</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4067"/>
        <source>untrusted</source>
        <translation>non fiable</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4091"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation>la chaîne de blocs ne peut pas être sauvegardée&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4121"/>
        <source>NOTE: this transaction uses an encrypted payment ID: consider using subaddresses instead</source>
        <translation>NOTE: cette transaction utilise un ID de paiement chiffré: veuillez considérer l&apos;utilisation de sous-adresses à la place</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4124"/>
        <source>WARNING: this transaction uses an unencrypted payment ID: consider using subaddresses instead</source>
        <translation>ATTENTION: cette transaction utilise un ID de paiement non chiffré: veuillez considérer l&apos;utilisation de sous-adresses à la place</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4160"/>
        <source>Password needed (%s) - use the refresh command</source>
        <translation>Mot de passe requis (%s) - utilisez la commande refresh</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4168"/>
        <source>Enter password</source>
        <translation>Entrez le mot de passe</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4215"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4511"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4219"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4515"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4229"/>
        <source>refresh error: </source>
        <translation>erreur du rafraîchissement&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4277"/>
        <source> (Some owned outputs have missing key images - import_key_images needed)</source>
        <translation> (Il manque les images de clé de certaines sorties - import_key_images requis)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4281"/>
        <source>Balance: </source>
        <translation>Solde&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4377"/>
        <source>pubkey</source>
        <translation>clé publique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4377"/>
        <source>key image</source>
        <translation>image de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4378"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4388"/>
        <source>unlocked</source>
        <translation>déverrouillé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4378"/>
        <source>ringct</source>
        <translation>ringct</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4387"/>
        <source>T</source>
        <translation>V</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4387"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4388"/>
        <source>locked</source>
        <translation>vérrouillé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4389"/>
        <source>RingCT</source>
        <translation>RingCT</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4389"/>
        <source>-</source>
        <translation>-</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4463"/>
        <source>payment ID has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4519"/>
        <source>failed to get spent status</source>
        <translation>échec de la récupération du statut de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4579"/>
        <source>failed to find construction data for tx input</source>
        <translation>échec de la recherche des données pour contruire l&apos;entrée de la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4645"/>
        <source>the same transaction</source>
        <translation>la même transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4645"/>
        <source>blocks that are temporally very close</source>
        <translation>blocs très proches dans le temps</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4705"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5274"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5553"/>
        <source>ring size %u is too large, maximum is %u</source>
        <translation>la taille de cercle %u est trop grande, le maximum est %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4730"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4847"/>
        <source>Unencrypted payment IDs are bad for privacy: ask the recipient to use subaddresses instead</source>
        <translation>Les ID de paiment non chiffrés sont mauvais pour la confidentialité : demandez au bénéficiaire d&apos;utiliser les sous-adresses à la place</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4747"/>
        <source>payment id failed to encode</source>
        <translation>échec de l&apos;encodage de l&apos;ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4766"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5298"/>
        <source>Locked blocks too high, max 1000000 (˜4 yrs)</source>
        <translation>Nombre de blocs verrou trop élévé, 1000000 max (˜4 ans)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4792"/>
        <source>failed to parse short payment ID from URI</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement court à partir de l&apos;URI</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4815"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4817"/>
        <source>Invalid last argument: </source>
        <translation>Dernier argument invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4834"/>
        <source>a single transaction cannot use more than one payment id</source>
        <translation>une unique transaction ne peut pas utiliser plus d&apos;un ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4851"/>
        <source>failed to parse payment id, though it was detected</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement, bien qu&apos;il ait été détecté</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5186"/>
        <source>Not enough money in unlocked balance</source>
        <translation>Pas assez de fonds dans le solde débloqué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5187"/>
        <source>Discarding %s of unmixable outputs that cannot be spent, which can be undone by &quot;rescan_spent&quot;.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>On se débarrasse de %s de sorties non mélangeables qui ne peuvent pas être dépensées, ce qui peut être défait avec &quot;rescan_spent&quot;. Est-ce d&apos;accord ?  (Y/Yes/N/No) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5215"/>
        <source>usage: %s [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] [outputs=&lt;N&gt;] &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation>usage: %s [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] [outputs=&lt;N&gt;] &lt;adresse&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5283"/>
        <source>missing lockedblocks parameter</source>
        <translation>paramètre blocs_verrou manquant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5293"/>
        <source>bad locked_blocks parameter</source>
        <translation>mauvais paramètre blocs_verrou</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5318"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5562"/>
        <source>Failed to parse number of outputs</source>
        <translation>Échec de l&apos;analyse du nombre de sorties</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5323"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5567"/>
        <source>Amount of outputs should be greater than 0</source>
        <translation>Le nombre de sorties doit être supérieur à 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5609"/>
        <source>usage: sweep_single [&lt;priority&gt;] [&lt;ring_size&gt;] [outputs=&lt;N&gt;] &lt;key_image&gt; &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation>usage&#xa0;: sweep_single [&lt;priorité&gt;] [&lt;taille_cercle&gt;] [outputs=&lt;N&gt;] &lt;image_clé&gt; &lt;adresse&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5770"/>
        <source>donations are not enabled on the testnet or on the stagenet</source>
        <translation>les dons ne sont pas activés sur les réseaux testnet et stagenet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5799"/>
        <source>Donating %s %s to The Monero Project (donate.getmonero.org or %s).</source>
        <translation>Don de %s %s à The Monero Project (donate.getmonero.org ou %s).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5971"/>
        <source>usage: sign_transfer [export_raw]</source>
        <translation>usage&#xa0;: sign_transfer [export_raw]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6097"/>
        <source>usage: set_tx_key &lt;txid&gt; &lt;tx_key&gt;</source>
        <translation>usage : set_tx_key &lt;ID_transaction&gt; &lt;clé_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6114"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6125"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6132"/>
        <source>failed to parse tx_key</source>
        <translation>échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6141"/>
        <source>Tx key successfully stored.</source>
        <translation>Clé de transaction sauvegardée avec succès.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6145"/>
        <source>Failed to store tx key: </source>
        <translation>Échec de la sauvegarde de la clé de transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6324"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6440"/>
        <source>Good signature</source>
        <translation>Bonne signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6351"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6442"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6548"/>
        <source>Bad signature</source>
        <translation>Mauvaise signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6606"/>
        <source>usage: show_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation>usage&#xa0;: show_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;hauteur_min&gt; [&lt;hauteur_max&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6692"/>
        <source>block</source>
        <translation>bloc</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6926"/>
        <source>Warning: this will lose any information which can not be recovered from the blockchain.</source>
        <translation>Attention : ceci pedra toute information qui ne peut pas être retrouvée à partir de la chaîne de blocs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6927"/>
        <source>This includes destination addresses, tx secret keys, tx notes, etc</source>
        <translation>Ceci inclut les adresses de destination, les clé secrètes de transaction, les notes de transaction, etc</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6928"/>
        <source>Rescan anyway ? (Y/Yes/N/No): </source>
        <translation>Rescanner quand même ? (Y/Yes/N/No) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7317"/>
        <source>usage: integrated_address [payment ID]</source>
        <translation>usage&#xa0;: integrated_address [ID paiement]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7353"/>
        <source>Standard address: </source>
        <translation>Adresse standard&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7358"/>
        <source>failed to parse payment ID or address</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement ou de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7369"/>
        <source>usage: address_book [(add (&lt;address&gt; [pid &lt;long or short payment id&gt;])|&lt;integrated address&gt; [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</source>
        <translation>usage&#xa0;: address_book [(add (&lt;adresse&gt; [pid &lt;ID de paiement long ou court&gt;])|&lt;adresse integrée&gt; [&lt;description avec des espaces possible&gt;])|(delete &lt;index&gt;)]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7399"/>
        <source>failed to parse payment ID</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7417"/>
        <source>failed to parse index</source>
        <translation>échec de l&apos;analyse de l&apos;index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7425"/>
        <source>Address book is empty.</source>
        <translation>Le carnet d&apos;adresses est vide.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7431"/>
        <source>Index: </source>
        <translation>Index&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7432"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7562"/>
        <source>Address: </source>
        <translation>Adresse&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7433"/>
        <source>Payment ID: </source>
        <translation>ID de paiement&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7434"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7561"/>
        <source>Description: </source>
        <translation>Description&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7444"/>
        <source>usage: set_tx_note [txid] free text note</source>
        <translation>usage&#xa0;: set_tx_note [ID transaction] note de texte libre</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7472"/>
        <source>usage: get_tx_note [txid]</source>
        <translation>usage&#xa0;: get_tx_note [ID transaction]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7571"/>
        <source>Network type: </source>
        <translation>Type de réseau : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7572"/>
        <source>Testnet</source>
        <translation>Testnet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7573"/>
        <source>Stagenet</source>
        <translation>Stagenet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7573"/>
        <source>Mainnet</source>
        <translation>Mainnet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7586"/>
        <source>usage: sign &lt;filename&gt;</source>
        <translation>usage&#xa0;: sign &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7591"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas signer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1087"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7606"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7629"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7818"/>
        <source>failed to read file </source>
        <translation>échec de la lecture du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6286"/>
        <source>usage: check_tx_proof &lt;txid&gt; &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>usage&#xa0;: check_tx_proof &lt;ID_transaction&gt; &lt;adresse&gt; &lt;fichier_signature&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6313"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6433"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6533"/>
        <source>failed to load signature file</source>
        <translation>échec du chargement du fichier signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6369"/>
        <source>usage: get_spend_proof &lt;txid&gt; [&lt;message&gt;]</source>
        <translation>usage&#xa0;: get_spend_proof &lt;ID_transaction&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6375"/>
        <source>wallet is watch-only and cannot generate the proof</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut générer de preuve</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6413"/>
        <source>usage: check_spend_proof &lt;txid&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>usage&#xa0;: check_spend_proof &lt;ID_transaction&gt; &lt;fichier_signature&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6459"/>
        <source>usage: get_reserve_proof (all|&lt;amount&gt;) [&lt;message&gt;]</source>
        <translation>usage&#xa0;: get_reserve_proof (all|&lt;montant&gt;) [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6465"/>
        <source>The reserve proof can be generated only by a full wallet</source>
        <translation>La preuve de réserve ne peut être généré que par un portefeuille complet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6508"/>
        <source>usage: check_reserve_proof &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>usage&#xa0;: check_reserve_proof &lt;adresse&gt; &lt;fichier_signature&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6526"/>
        <source>Address must not be a subaddress</source>
        <translation>L&apos;adresse ne doit pas être une sous-adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6544"/>
        <source>Good signature -- total: %s, spent: %s, unspent: %s</source>
        <translation>Bonne signature -- total&#xa0;: %s, dépensé&#xa0;: %s, non dépensé&#xa0;: %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6754"/>
        <source>[Double spend seen on the network: this transaction may or may not end up being mined] </source>
        <translation>[Double dépense détectée sur le réseau&#xa0;: cette transaction sera peut-être invalidée] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6790"/>
        <source>usage: unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_amount&gt; [&lt;max_amount&gt;]]</source>
        <translation>usage&#xa0;: unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;montant_min&gt; [&lt;montant_max&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6850"/>
        <source>There is no unspent output in the specified address</source>
        <translation>Il n&apos;y a pas de sortie non dépensée pour l&apos;adresse spécifiée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6970"/>
        <source> (no daemon)</source>
        <translation> (pas de démon)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6972"/>
        <source> (out of sync)</source>
        <translation> (désynchronisé)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7029"/>
        <source>(Untitled account)</source>
        <translation>(compte sans nom)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7042"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7060"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7085"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7108"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7261"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7284"/>
        <source>failed to parse index: </source>
        <translation>échec de l&apos;analyse de l&apos;index&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7047"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7266"/>
        <source>specify an index between 0 and </source>
        <translation>specifiez un index entre 0 et </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7144"/>
        <source>usage:
  account
  account new &lt;label text with white spaces allowed&gt;
  account switch &lt;index&gt;
  account label &lt;index&gt; &lt;label text with white spaces allowed&gt;
  account tag &lt;tag_name&gt; &lt;account_index_1&gt; [&lt;account_index_2&gt; ...]
  account untag &lt;account_index_1&gt; [&lt;account_index_2&gt; ...]
  account tag_description &lt;tag_name&gt; &lt;description&gt;</source>
        <translation>usage&#xa0;:
  account
  account new &lt;texte étiquette avec espaces autorisés&gt;
  account switch &lt;index&gt; 
  account label &lt;index&gt; &lt;texte étiquette avec espaces autorisés&gt;
  account tag &lt;mot_clé&gt; &lt;index_compte_1&gt; [&lt;index_compte_2&gt; ...]
  account untag &lt;index_compte_1&gt; [&lt;index_compte_2&gt; ...]
  account tag_description &lt;mot_clé&gt; &lt;description&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7172"/>
        <source>
Grand total:
  Balance: </source>
        <translation>
Somme finale&#xa0;:
  Solde&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7172"/>
        <source>, unlocked balance: </source>
        <translation>, solde débloqué&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7180"/>
        <source>Untagged accounts:</source>
        <translation>Comptes sans mot clé&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7186"/>
        <source>Tag %s is unregistered.</source>
        <translation>Le mot clé %s n&apos;est pas enregistré.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7189"/>
        <source>Accounts with tag: </source>
        <translation>Comptes avec mot clé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7190"/>
        <source>Tag&apos;s description: </source>
        <translation>Description du mot clé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7192"/>
        <source>Account</source>
        <translation>Compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7198"/>
        <source> %c%8u %6s %21s %21s %21s</source>
        <translation> %c%8u %6s %21s %21s %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7208"/>
        <source>----------------------------------------------------------------------------------</source>
        <translation>----------------------------------------------------------------------------------</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7209"/>
        <source>%15s %21s %21s</source>
        <translation>%15s %21s %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7232"/>
        <source>Primary address</source>
        <translation>Adresse primaire</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7232"/>
        <source>(used)</source>
        <translation>(utilisé)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7253"/>
        <source>(Untitled address)</source>
        <translation>(adresse sans nom)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7293"/>
        <source>&lt;index_min&gt; is already out of bound</source>
        <translation>&lt;index_min&gt; est déjà hors limite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7298"/>
        <source>&lt;index_max&gt; exceeds the bound</source>
        <translation>&lt;index_max&gt; excède la limite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7306"/>
        <source>usage: address [ new &lt;label text with white spaces allowed&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;label text with white spaces allowed&gt; ]</source>
        <translation>usage&#xa0;: address [ new &lt;texte étiquette avec espaces autorisés&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;texte étiquette avec espaces autorisés&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7324"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7336"/>
        <source>Integrated addresses can only be created for account 0</source>
        <translation>Les adresses intégrées ne peuvent être créées que pour le compte 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7348"/>
        <source>Integrated address: %s, payment ID: %s</source>
        <translation>Adresse intégrée&#xa0;: %s, ID de paiement&#xa0;: %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7353"/>
        <source>Subaddress: </source>
        <translation>Sous-adresse&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7513"/>
        <source>usage: get_description</source>
        <translation>usage&#xa0;: get_description</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7519"/>
        <source>no description found</source>
        <translation>pas de description trouvée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7521"/>
        <source>description found: </source>
        <translation>description trouvée&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7560"/>
        <source>Filename: </source>
        <translation>Fichier&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7565"/>
        <source>Watch only</source>
        <translation>Audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7567"/>
        <source>%u/%u multisig%s</source>
        <translation>Multisig %u/%u%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7569"/>
        <source>Normal</source>
        <translation>Normal</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7570"/>
        <source>Type: </source>
        <translation>Type&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7596"/>
        <source>This wallet is multisig and cannot sign</source>
        <translation>C&apos;est un portefeuille multisig et il ne peut pas signer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7618"/>
        <source>usage: verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation>usage&#xa0;: verify &lt;fichier&gt; &lt;adresse&gt; &lt;signature&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7643"/>
        <source>Bad signature from </source>
        <translation>Mauvaise signature de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7647"/>
        <source>Good signature from </source>
        <translation>Bonne signature de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7661"/>
        <source>usage: export_key_images &lt;filename&gt;</source>
        <translation>usage&#xa0;: export_key_images &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7666"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas exporter les images de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1037"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7679"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7785"/>
        <source>failed to save file </source>
        <translation>échec de l&apos;enregistrement du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7690"/>
        <source>Signed key images exported to </source>
        <translation>Images de clé signées exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7709"/>
        <source>usage: import_key_images &lt;filename&gt;</source>
        <translation>usage&#xa0;: import_key_images &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7770"/>
        <source>usage: export_outputs &lt;filename&gt;</source>
        <translation>usage&#xa0;: export_outputs &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7796"/>
        <source>Outputs exported to </source>
        <translation>Sorties exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7809"/>
        <source>usage: import_outputs &lt;filename&gt;</source>
        <translation>usage&#xa0;: import_outputs &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4806"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6476"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6809"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6817"/>
        <source>amount is wrong: </source>
        <translation>montant erroné&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4807"/>
        <source>expected number from 0 to </source>
        <translation>attend un nombre de 0 à </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5132"/>
        <source>Sweeping </source>
        <translation>Balayage de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5728"/>
        <source>Money successfully sent, transaction: </source>
        <translation>Fonds envoyés avec succès, transaction&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5885"/>
        <source>Change goes to more than one address</source>
        <translation>La monnaie rendue va à plus d&apos;une adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5926"/>
        <source>%s change to %s</source>
        <translation>%s de monnaie rendue à %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5929"/>
        <source>no change</source>
        <translation>sans monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1186"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1199"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6001"/>
        <source>Transaction successfully signed to file </source>
        <translation>Transaction signée avec succès dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6061"/>
        <source>usage: get_tx_key &lt;txid&gt;</source>
        <translation>usage&#xa0;: get_tx_key &lt;ID transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6068"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6104"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6166"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6215"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6297"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6382"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6420"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7451"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7479"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7848"/>
        <source>failed to parse txid</source>
        <translation>échec de l&apos;analyse de l&apos;ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6082"/>
        <source>Tx key: </source>
        <translation>Clé de transaction&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6087"/>
        <source>no tx keys found for this txid</source>
        <translation>aucune clé de transaction trouvée pour cet ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6159"/>
        <source>usage: get_tx_proof &lt;txid&gt; &lt;address&gt; [&lt;message&gt;]</source>
        <translation>usage&#xa0;: get_tx_proof &lt;ID_transaction&gt; &lt;adresse&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6184"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6399"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6494"/>
        <source>signature file saved to: </source>
        <translation>fichier signature sauvegardé dans&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6186"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6401"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6496"/>
        <source>failed to save signature file</source>
        <translation>échec de la sauvegarde du fichier signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6200"/>
        <source>usage: check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</source>
        <translation>usage&#xa0;: check_tx_key &lt;ID transaction&gt; &lt;clé transaction&gt; &lt;adresse&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6223"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6232"/>
        <source>failed to parse tx key</source>
        <translation>échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6190"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6278"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6356"/>
        <source>error: </source>
        <translation>erreur&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6254"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6327"/>
        <source>received</source>
        <translation>a reçu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6254"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6327"/>
        <source>in txid</source>
        <translation>dans la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6273"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6346"/>
        <source>received nothing in txid</source>
        <translation>n&apos;a rien reçu dans la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6257"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6330"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation>ATTENTION&#xa0;: cette transaction n&apos;est pas encore inclue dans la chaîne de blocs !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6263"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6336"/>
        <source>This transaction has %u confirmations</source>
        <translation>Cette transaction a %u confirmations</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6267"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6340"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation>ATTENTION&#xa0;: échec de la détermination du nombre de confirmations !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6659"/>
        <source>bad min_height parameter:</source>
        <translation>mauvais paramètre hauteur_minimum&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6671"/>
        <source>bad max_height parameter:</source>
        <translation>mauvais paramètre hauteur_maximum&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6692"/>
        <source>in</source>
        <translation>reçu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6726"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6778"/>
        <source>out</source>
        <translation>payé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6778"/>
        <source>failed</source>
        <translation>échoué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6778"/>
        <source>pending</source>
        <translation>en attente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6824"/>
        <source>&lt;min_amount&gt; should be smaller than &lt;max_amount&gt;</source>
        <translation>&lt;montant_minimum&gt; doit être inférieur à &lt;montant_maximum&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6856"/>
        <source>
Amount: </source>
        <translation>
Montant&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6856"/>
        <source>, number of keys: </source>
        <translation>, nombre de clés&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6861"/>
        <source> </source>
        <translation> </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6866"/>
        <source>
Min block height: </source>
        <translation>
Hauteur de bloc minimum&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6867"/>
        <source>
Max block height: </source>
        <translation>
Hauteur de bloc maximum&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6868"/>
        <source>
Min amount found: </source>
        <translation>
Montant minimum trouvé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6869"/>
        <source>
Max amount found: </source>
        <translation>
Montant maximum trouvé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6870"/>
        <source>
Total count: </source>
        <translation>
Compte total&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6910"/>
        <source>
Bin size: </source>
        <translation>
Taille de classe&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6911"/>
        <source>
Outputs per *: </source>
        <translation>
Sorties par *&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6913"/>
        <source>count
  ^
</source>
        <translation>compte
  ^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6915"/>
        <source>  |</source>
        <translation>  |</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6917"/>
        <source>  +</source>
        <translation>  +</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6917"/>
        <source>+--&gt; block height
</source>
        <translation>+--&gt; hauteur de bloc
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6918"/>
        <source>   ^</source>
        <translation>   ^</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6918"/>
        <source>^
</source>
        <translation>^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6919"/>
        <source>  </source>
        <translation>  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6968"/>
        <source>wallet</source>
        <translation>portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="776"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7328"/>
        <source>Random payment ID: </source>
        <translation>ID de paiement aléatoire&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7329"/>
        <source>Matching integrated address: </source>
        <translation>Adresse intégrée correspondante&#xa0;: </translation>
    </message>
</context>
<context>
    <name>genms</name>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="70"/>
        <source>Base filename (-1, -2, etc suffixes will be appended as needed)</source>
        <translation>Nom de fichier de base (des suffixes -1, -2 etc seront ajoutés si nécessaire)</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="71"/>
        <source>Give threshold and participants at once as M/N</source>
        <translation>Indiquer le seuil et les participants sous la forme M/N</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="72"/>
        <source>How many participants will share parts of the multisig wallet</source>
        <translation>Combien de participants partageront des parts du portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="73"/>
        <source>How many signers are required to sign a valid transaction</source>
        <translation>Combien de signataires sont requis pour signer une transaction valide</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="74"/>
        <source>Create testnet multisig wallets</source>
        <translation>Créer des portefeuilles multisig testnet</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="75"/>
        <source>Create stagenet multisig wallets</source>
        <translation>Créer des portefeuilles multisig stagenet</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="76"/>
        <source>Create an address file for new wallets</source>
        <translation>Créer un fichier d&apos;adresse pour les nouveaux portefeuilles</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="83"/>
        <source>Generating %u %u/%u multisig wallets</source>
        <translation>Génération de %u portefeuilles multisig %u/%u</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="107"/>
        <source>Failed to verify multisig info</source>
        <translation>Échec de la vérification des infos multisig</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="142"/>
        <source>Error verifying multisig extra info</source>
        <translation>Erreur de vérification des infos multisig supplémentaires</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="150"/>
        <source>Error finalizing multisig</source>
        <translation>Erreur de finalisation multisig</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="157"/>
        <source>Generated multisig wallets for address </source>
        <translation>Portefeuilles multisig générés pour l&apos;adresse </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="161"/>
        <source>Error creating multisig wallets: </source>
        <translation>Erreur de création des portefeuilles multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="186"/>
        <source>This program generates a set of multisig wallets - use this simpler scheme only if all the participants trust each other</source>
        <translation>Ce programme génère un ensemble de portefeuilles multisig - n&apos;utilisez cette méthode plus simple que si tous les participants se font confiance</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="205"/>
        <source>Error: Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation>Erreur: Impossible de spécifier plus d&apos;une option parmis --testnet et --stagenet</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="212"/>
        <source>Error: expected N/M, but got: </source>
        <translation>Erreur&#xa0;: N/M attendu, mais lu&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="220"/>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="229"/>
        <source>Error: either --scheme or both of --threshold and --participants may be given</source>
        <translation>Erreur&#xa0;: soit --scheme soit --threshold et --participants doivent être indiqués</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="236"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got N==%u and M==%d</source>
        <translation>Erreur&#xa0;: N &gt; 1 et N &lt;= M attendu, mais lu N==%u et M==%d</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="245"/>
        <source>Error: --filename-base is required</source>
        <translation>Erreur&#xa0;: --filename-base est requis</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="251"/>
        <source>Error: unsupported scheme: only N/N and N-1/N are supported</source>
        <translation>Erreur&#xa0;: schéma non supporté&#xa0;: seuls N/N et N-1/N sont supportés</translation>
    </message>
</context>
<context>
    <name>sw</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="119"/>
        <source>Generate new wallet and save it to &lt;arg&gt;</source>
        <translation>Générer un nouveau portefeuille et le sauvegarder dans &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="120"/>
        <source>Generate new wallet from device and save it to &lt;arg&gt;</source>
        <translation>Générer un nouveau portefeuille à partir de l&apos;appareil et le sauvegarder dans &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="121"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>Générer un portefeuille d&apos;audit à partir d&apos;une clé d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="122"/>
        <source>Generate deterministic wallet from spend key</source>
        <translation>Générer un portefeuille déterministe à partir d&apos;une clé de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="123"/>
        <source>Generate wallet from private keys</source>
        <translation>Générer un portefeuille à partir de clés privées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="124"/>
        <source>Generate a master wallet from multisig wallet keys</source>
        <translation>Générer un portefeuille principal à partir de clés de portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="126"/>
        <source>Language for mnemonic</source>
        <translation>Langue de la phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="127"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>Spécifier la phrase mnémonique Electrum pour la récupération/création d&apos;un portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="128"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Récupérer un portefeuille en utilisant une phrase mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="129"/>
        <source>Recover multisig wallet using Electrum-style mnemonic seed</source>
        <translation>Récupérer un portefeuille multisig en utilisant une phrase mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="130"/>
        <source>Generate non-deterministic view and spend keys</source>
        <translation>Générer des clés d&apos;audit et de dépense non déterministes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="268"/>
        <source>invalid argument: must be either 0/1, true/false, y/n, yes/no</source>
        <translation>argument invalide : doit être soit 0/1, true/false, y/n, yes/no</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="324"/>
        <source>DNSSEC validation passed</source>
        <translation>Validation DNSSEC réussie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="328"/>
        <source>WARNING: DNSSEC validation was unsuccessful, this address may not be correct!</source>
        <translation>ATTENTION: la validation DNSSEC a échoué, cette adresse n&apos;est peut être pas correcte !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="331"/>
        <source>For URL: </source>
        <translation>Pour l&apos;URL : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="333"/>
        <source> Monero Address = </source>
        <translation> Adresse Monero = </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="335"/>
        <source>Is this OK? (Y/n) </source>
        <translation>Est-ce correct ? (Y/n) </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="345"/>
        <source>you have cancelled the transfer request</source>
        <translation>vous avez annulé la demande de transfert</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="366"/>
        <source>failed to parse index: </source>
        <translation>échec de l&apos;analyse de l&apos;index&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="379"/>
        <source>invalid format for subaddress lookahead; must be &lt;major&gt;:&lt;minor&gt;</source>
        <translation>format invalide pour l&apos;anticipation des sous-addresses; doit être &lt;majeur&gt;:&lt;mineur&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="396"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="401"/>
        <source>RPC error: </source>
        <translation>Erreur RPC&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="405"/>
        <source>failed to get random outputs to mix: </source>
        <translation>échec de la récupération de sorties aléatoires à mélanger&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="412"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="420"/>
        <source>Not enough money in unlocked balance</source>
        <translation>Pas assez de fonds dans le solde débloqué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="430"/>
        <source>Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees</source>
        <translation>Impossible de trouver une façon de créer les transactions. Ceci est souvent dû à de la poussière si petite qu&apos;elle ne peut pas payer ses propres frais, ou à une tentative d&apos;envoi d&apos;un montant supérieur au solde débloqué, ou à un montant restant insuffisant pour payer les frais</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="436"/>
        <source>not enough outputs for specified ring size</source>
        <translation>pas assez de sorties pour la taille de cercle spécifiée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="439"/>
        <source>output amount</source>
        <translation>montant de la sortie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="439"/>
        <source>found outputs to use</source>
        <translation>sorties à utiliser trouvées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="441"/>
        <source>Please use sweep_unmixable.</source>
        <translation>Veuillez utiliser sweep_unmixable.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="445"/>
        <source>transaction was not constructed</source>
        <translation>la transaction n&apos;a pas été construite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="450"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transaction %s a été rejetée par le démon avec le statut&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="453"/>
        <source>Reason: </source>
        <translation>Raison : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="462"/>
        <source>one of destinations is zero</source>
        <translation>une des destinations est zéro</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="467"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>échec de la recherche d&apos;une façon adéquate de scinder les transactions</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="473"/>
        <source>unknown transfer error: </source>
        <translation>erreur de transfert inconnue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="478"/>
        <source>Multisig error: </source>
        <translation>Erreur multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="484"/>
        <source>internal error: </source>
        <translation>erreur interne&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="489"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="493"/>
        <source>There was an error, which could mean the node may be trying to get you to retry creating a transaction, and zero in on which outputs you own. Or it could be a bona fide error. It may be prudent to disconnect from this node, and not try to send a transaction immediately. Alternatively, connect to another node so the original node cannot correlate information.</source>
        <translation>Il y a eu une erreur, ce qui pourrait signifier que le noeud essaye de vous faire réessayer de créer une transaction, pour tenter d&apos;identifier quelles sorties sont les votres. Ou il pourrait s&apos;agir d&apos;une erreur de bonne foi. Il pourrait être prudent de se déconnecter de ce noeud, et de ne pas essayer d&apos;envoyer une transaction immédiatement. Ou sinon, se connecter à un autre noeud pour que le noeud original ne puisse pas corréler les informations.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="503"/>
        <source>File %s likely stores wallet private keys! Use a different file name.</source>
        <translation>Le fichier %s contient probablement des clés privées de portefeuille ! Utilisez un nom de fichier différent.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="506"/>
        <source>File %s already exists. Are you sure to overwrite it? (Y/Yes/N/No): </source>
        <translation>Le fichier %s existe déjà. Êtes vous sûr de vouloir l&apos;écraser ? (Y/Yes/N/No) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6580"/>
        <source> seconds</source>
        <translation> secondes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6582"/>
        <source> minutes</source>
        <translation> minutes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6584"/>
        <source> hours</source>
        <translation> heures</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6586"/>
        <source> days</source>
        <translation> jours</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6588"/>
        <source> months</source>
        <translation> mois</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6589"/>
        <source>a long time</source>
        <translation>longtemps</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8074"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.
WARNING: Do not reuse your Monero keys on another fork, UNLESS this fork has key reuse mitigations built in. Doing so will harm your privacy.</source>
        <translation>Ceci est le portefeuille monero en ligne de commande.
Il a besoin de se connecter à un démon monero pour fonctionner correctement.
ATTENTION : Ne réutilisez pas vos clés Monero avec un autre fork, À MOINS QUE ce fork inclue des mitigations contre la réutilisation des clés. Faire ceci nuira à votre confidentialité.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8099"/>
        <source>Unknown command: </source>
        <translation>Commande inconnue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="131"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>Autoriser la communication avec un démon utilisant une version de RPC différente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="132"/>
        <source>Restore from specific blockchain height</source>
        <translation>Restaurer à partir d&apos;une hauteur de bloc spécifique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="133"/>
        <source>The newly created transaction will not be relayed to the monero network</source>
        <translation>La transaction nouvellement créée ne sera pas transmise au réseau monero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="134"/>
        <source>Create an address file for new wallets</source>
        <translation>Créer un fichier d&apos;adresse pour les nouveaux portefeuilles</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="136"/>
        <source>Display English language names</source>
        <translation>Afficher les noms de langue en anglais</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="183"/>
        <source>failed to read wallet password</source>
        <translation>échec de la lecture du mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="190"/>
        <source>Enter a new password for the wallet</source>
        <translation>Entrer un nouveau mot de passe pour le portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="190"/>
        <source>Wallet password</source>
        <translation>Mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="200"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="392"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="209"/>
        <source>possibly lost connection to daemon</source>
        <translation>connexion avec le démon peut-être perdue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="226"/>
        <source>Error: </source>
        <translation>Erreur&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8093"/>
        <source>Failed to initialize wallet</source>
        <translation>Échec de l&apos;initialisation du portefeuille</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="142"/>
        <source>Use daemon instance at &lt;host&gt;:&lt;port&gt;</source>
        <translation>Utiliser l&apos;instance de démon située à &lt;hôte&gt;:&lt;port&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="143"/>
        <source>Use daemon instance at host &lt;arg&gt; instead of localhost</source>
        <translation>Utiliser l&apos;instance de démon située à l&apos;hôte &lt;arg&gt; au lieu de localhost</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="147"/>
        <source>Wallet password file</source>
        <translation>Fichier mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="148"/>
        <source>Use daemon instance at port &lt;arg&gt; instead of 18081</source>
        <translation>Utiliser l&apos;instance de démon située au port &lt;arg&gt; au lieu de 18081</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="150"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>Pour testnet, le démon doit aussi être lancé avec l&apos;option --testnet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="220"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>impossible de spécifier l&apos;hôte ou le port du démon plus d&apos;une fois</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="291"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>impossible de spécifier plus d&apos;une option parmis --password et --password-file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="304"/>
        <source>the password file specified could not be read</source>
        <translation>le fichier mot de passe spécifié n&apos;a pas pu être lu</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="330"/>
        <source>Failed to load file </source>
        <translation>Échec du chargement du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="146"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation>Mot de passe du portefeuille (échapper/citer si nécessaire)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="144"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation>Activer les commandes qui dépendent d&apos;un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="145"/>
        <source>Disable commands which rely on a trusted daemon</source>
        <translation>Désactiver les commandes qui dépendent d&apos;un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="149"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>Spécifier le nom_utilisateur:[mot_de_passe] pour le client RPC du démon</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="151"/>
        <source>For stagenet. Daemon must also be launched with --stagenet flag</source>
        <translation>Pour stagenet, le démon doit aussi être lancé avec l&apos;option --stagenet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="153"/>
        <source>Set shared ring database path</source>
        <translation>Définir le chemin de la base de donnée de cercles partagés</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="164"/>
        <source>Number of rounds for the key derivation function</source>
        <translation>Nombre de rondes de la fonction de dérivation de clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="165"/>
        <source>HW device to use</source>
        <translation>Portefeuille matériel à utiliser</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="251"/>
        <source>--trusted-daemon and --untrusted-daemon are both seen, assuming untrusted</source>
        <translation>--trusted-daemon et --untrusted-daemon présents simultanément, --untrusted-daemon choisi</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="261"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Le démon est local, supposons qu&apos;il est de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="311"/>
        <source>no password specified; use --prompt-for-password to prompt for a password</source>
        <translation>pas de mot de passe spécifié; utilisez --prompt-for-password pour demander un mot de passe</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="313"/>
        <source>Enter a new password for the wallet</source>
        <translation>Entrer un nouveau mot de passe pour le portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="313"/>
        <source>Wallet password</source>
        <translation>Mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="336"/>
        <source>Failed to parse JSON</source>
        <translation>Échec de l&apos;analyse JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="343"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation>Version %u trop récente, on comprend au mieux %u</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="359"/>
        <source>failed to parse view key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="364"/>
        <location filename="../src/wallet/wallet2.cpp" line="432"/>
        <location filename="../src/wallet/wallet2.cpp" line="475"/>
        <source>failed to verify view key secret key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="375"/>
        <source>failed to parse spend key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="380"/>
        <location filename="../src/wallet/wallet2.cpp" line="442"/>
        <location filename="../src/wallet/wallet2.cpp" line="501"/>
        <source>failed to verify spend key secret key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="392"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="412"/>
        <source>At least one of either an Electrum-style word list, private view key, or private spend key must be specified</source>
        <translation>Il faut spécifier au moins une des options parmis la liste de mots de style Electrum, la clé privée d&apos;audit et la clé privée de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="416"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Liste de mots de style Electrum et clé privée spécifiées en même temps</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="426"/>
        <source>invalid address</source>
        <translation>adresse invalide</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="435"/>
        <source>view key does not match standard address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="445"/>
        <source>spend key does not match standard address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="453"/>
        <source>Cannot generate deprecated wallets from JSON</source>
        <translation>Impossible de générer un portefeuille obsolète à partir de JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="487"/>
        <source>failed to parse address: </source>
        <translation>échec de l&apos;analyse de l&apos;adresse&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="493"/>
        <source>Address must be specified in order to create watch-only wallet</source>
        <translation>L&apos;adresse doit être spécifiée afin de créer un portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="510"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="1271"/>
        <source>Password is needed to compute key image for incoming monero</source>
        <translation>Le mot de passe est requis pour calculer l&apos;image de clé pour les moneros entrants</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="1272"/>
        <source>Invalid password: password is needed to compute key image for incoming monero</source>
        <translation>Mot de passe invalide : le mot de passe est requis pour calculer l&apos;image de clé pour les moneros entrants</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="3496"/>
        <location filename="../src/wallet/wallet2.cpp" line="4062"/>
        <location filename="../src/wallet/wallet2.cpp" line="4495"/>
        <source>Primary account</source>
        <translation>Compte primaire</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="9547"/>
        <source>No funds received in this tx.</source>
        <translation>Aucun fonds n&apos;a été reçu dans cette transaction.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="10261"/>
        <source>failed to read file </source>
        <translation>échec de la lecture du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="135"/>
        <source>Set subaddress lookahead sizes to &lt;major&gt;:&lt;minor&gt;</source>
        <translation>Définir les tailles d&apos;anticipation des sous-addresses à &lt;majeur&gt;:&lt;mineur&gt;</translation>
    </message>
</context>
<context>
    <name>tools::wallet_rpc_server</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="180"/>
        <source>Failed to create directory </source>
        <translation>Échec de la création du répertoire </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="182"/>
        <source>Failed to create directory %s: %s</source>
        <translation>Échec de la création du répertoire %s&#xa0;: %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="193"/>
        <source>Cannot specify --</source>
        <translation>Impossible de spécifier --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="193"/>
        <source> and --</source>
        <translation> et --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="212"/>
        <source>Failed to create file </source>
        <translation>Échec de la création du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="212"/>
        <source>. Check permissions or remove file</source>
        <translation>. Vérifiez les permissions ou supprimez le fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="222"/>
        <source>Error writing to file </source>
        <translation>Erreur d&apos;écriture dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="225"/>
        <source>RPC username/password is stored in file </source>
        <translation>nom_utilisateur/mot_de_passe RPC sauvegardé dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="479"/>
        <source>Tag %s is unregistered.</source>
        <translation>Le mot clé %s n&apos;est pas enregistré.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2877"/>
        <source>Transaction not possible. Available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>Transaction impossible. Solde disponible&#xa0;: %s, montant de la transaction %s = %s + %s (frais)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3495"/>
        <source>This is the RPC monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation>Ceci est le portefeuille monero par RPC. Il a besoin de se
connecter à un démon monero pour fonctionner correctement.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3336"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>Impossible de spécifier plus d&apos;une option parmis --wallet-file et --generate-from-json</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3321"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation>Impossible de spécifier plus d&apos;une option parmis --testnet et --stagenet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3348"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>--wallet-file, --generate-from-json ou --wallet-dir doit être spécifié</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3352"/>
        <source>Loading wallet...</source>
        <translation>Chargement du portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3386"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3418"/>
        <source>Saving wallet...</source>
        <translation>Sauvegarde du portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3388"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3420"/>
        <source>Successfully saved</source>
        <translation>Sauvegardé avec succès</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3391"/>
        <source>Successfully loaded</source>
        <translation>Chargé avec succès</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3395"/>
        <source>Wallet initialization failed: </source>
        <translation>Échec de l&apos;initialisation du portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3401"/>
        <source>Failed to initialize wallet RPC server</source>
        <translation>Échec de l&apos;initialisation du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3405"/>
        <source>Starting wallet RPC server</source>
        <translation>Démarrage du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3412"/>
        <source>Failed to run wallet: </source>
        <translation>Échec du lancement du portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3415"/>
        <source>Stopped wallet RPC server</source>
        <translation>Arrêt du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3424"/>
        <source>Failed to save wallet: </source>
        <translation>Échec de la sauvegarde du portefeuille&#xa0;: </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="172"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3476"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8042"/>
        <source>Wallet options</source>
        <translation>Options du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="73"/>
        <source>Generate wallet from JSON format file</source>
        <translation>Générer un portefeuille à partir d&apos;un fichier au format JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="77"/>
        <source>Use wallet &lt;arg&gt;</source>
        <translation>Utiliser le portefeuille &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="105"/>
        <source>Max number of threads to use for a parallel job</source>
        <translation>Nombre maximum de threads à utiliser pour les travaux en parallèle</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="106"/>
        <source>Specify log file</source>
        <translation>Spécifier le fichier journal</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="107"/>
        <source>Config file</source>
        <translation>Fichier de configuration</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="119"/>
        <source>General options</source>
        <translation>Options générales</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="144"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation>Ceci est le portefeuille monero en ligne de commande. Il a besoin de se
connecter à un démon monero pour fonctionner correctement.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="169"/>
        <source>Can&apos;t find config file </source>
        <translation>Impossible de trouver le fichier de configuration </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="210"/>
        <source>Logging to: </source>
        <translation>Journalisation dans&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="212"/>
        <source>Logging to %s</source>
        <translation>Journalisation dans %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="146"/>
        <source>Usage:</source>
        <translation>Usage&#xa0;:</translation>
    </message>
</context>
</TS>
