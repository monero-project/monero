<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="fr">
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
        <location filename="../src/wallet/api/pending_transaction.cpp" line="138"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="141"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="145"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transaction %s a été rejetée par le démon avec le statut&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="150"/>
        <source>. Reason: </source>
        <translation>. Raison&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="152"/>
        <source>Unknown exception: </source>
        <translation>Exception inconnue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="155"/>
        <source>Unhandled exception</source>
        <translation>Exception non gérée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="228"/>
        <source>Couldn&apos;t multisig sign data: </source>
        <translation>Signature multisig des données impossible : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="250"/>
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
        <location filename="../src/wallet/api/wallet.cpp" line="1509"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1598"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1511"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1600"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1513"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1602"/>
        <source>RPC error: </source>
        <translation>Erreur RPC&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1541"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1633"/>
        <source>not enough outputs for specified ring size</source>
        <translation>pas assez de sorties pour la taille de cercle spécifiée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1543"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1635"/>
        <source>found outputs to use</source>
        <translation>sorties à utiliser trouvées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1545"/>
        <source>Please sweep unmixable outputs.</source>
        <translation>Veuillez balayer les sorties non mélangeables.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1519"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1609"/>
        <source>not enough money to transfer, available only %s, sent amount %s</source>
        <translation>pas assez de fonds pour le transfert, montant disponible %s, montant envoyé %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="589"/>
        <source>failed to parse address</source>
        <translation>échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="600"/>
        <source>failed to parse secret spend key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="623"/>
        <source>failed to parse secret view key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="632"/>
        <source>failed to verify secret spend key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="636"/>
        <source>spend key does not match address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="642"/>
        <source>failed to verify secret view key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="646"/>
        <source>view key does not match address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="669"/>
        <location filename="../src/wallet/api/wallet.cpp" line="686"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="955"/>
        <source>Failed to send import wallet request</source>
        <translation>Échec de l&apos;envoi de la requête d&apos;importation de portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1125"/>
        <source>Failed to load unsigned transactions</source>
        <translation>Échec du chargement des transaction non signées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1144"/>
        <source>Failed to load transaction from file</source>
        <translation>Échec du chargement de la transaction du fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1160"/>
        <source>Wallet is view only</source>
        <translation>Portefeuille d&apos;audit uniquement</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1168"/>
        <source>failed to save file </source>
        <translation>échec de l&apos;enregistrement du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1184"/>
        <source>Key images can only be imported with a trusted daemon</source>
        <translation>Les images de clé ne peuvent être importées qu&apos;avec un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1197"/>
        <source>Failed to import key images: </source>
        <translation>Échec de l&apos;importation des images de clé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1229"/>
        <source>Failed to get subaddress label: </source>
        <translation>Échec de la récupération de l&apos;étiquette de sous-adresse&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1242"/>
        <source>Failed to set subaddress label: </source>
        <translation>Échec de l&apos;affectation de l&apos;étiquette de sous-adresse&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="615"/>
        <source>Neither view key nor spend key supplied, cancelled</source>
        <translation>Ni clé d&apos;audit ni clé de dépense fournie, annulation</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="734"/>
        <source>Electrum seed is empty</source>
        <translation>La phrase Electrum est vide</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="743"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1259"/>
        <source>Failed to get multisig info: </source>
        <translation>Échec de la récupération des infos multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1276"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1290"/>
        <source>Failed to make multisig: </source>
        <translation>Échec de la création multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1305"/>
        <source>Failed to finalize multisig wallet creation</source>
        <translation>Échec de la finalisation de la création du portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1308"/>
        <source>Failed to finalize multisig wallet creation: </source>
        <translation>Échec de la finalisation de la création du portefeuille multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1324"/>
        <source>Failed to export multisig images: </source>
        <translation>Échec de l&apos;exportation des images multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1342"/>
        <source>Failed to parse imported multisig images</source>
        <translation>Échec de l&apos;analyse des images multisig importées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1352"/>
        <source>Failed to import multisig images: </source>
        <translation>Échec de l&apos;importation des images multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1366"/>
        <source>Failed to check for partial multisig key images: </source>
        <translation>Échec de la vérification des images de clé multisig partielles : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1394"/>
        <source>Failed to restore multisig transaction: </source>
        <translation>Échec de la restauration de la transaction multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1435"/>
        <source>Sending all requires one destination address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1439"/>
        <source>Destinations and amounts are unequal</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1447"/>
        <source>payment id has invalid format, expected 64 character hex string: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1455"/>
        <source>Invalid destination address</source>
        <translation>Adresse de destination invalide</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1461"/>
        <source>a single transaction cannot use more than one payment id</source>
        <translation type="unfinished">une unique transaction ne peut pas utiliser plus d&apos;un ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1487"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation type="unfinished">échec de la définition de l&apos;ID de paiement, bien qu&apos;il ait été décodé correctement</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1515"/>
        <source>failed to get outputs to mix: %s</source>
        <translation>échec de la récupération de sorties à mélanger : %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1526"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1617"/>
        <source>not enough money to transfer, overall balance only %s, sent amount %s</source>
        <translation>pas assez de fonds pour le transfer, solde global disponible %s, montant envoyé %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1533"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1625"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>pas assez de fonds pour le transfert, montant disponible %s, montant envoyé %s = %s + %s (frais)</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1543"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1635"/>
        <source>output amount</source>
        <translation>montant de la sortie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1548"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1639"/>
        <source>transaction was not constructed</source>
        <translation>la transaction n&apos;a pas été construite</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1551"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1642"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transaction %s a été rejetée par le démon avec le statut&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1556"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1647"/>
        <source>one of destinations is zero</source>
        <translation>une des destinations est zéro</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1558"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1649"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>échec de la recherche d&apos;une façon adéquate de scinder les transactions</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1560"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1651"/>
        <source>unknown transfer error: </source>
        <translation>erreur de transfert inconnue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1562"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1653"/>
        <source>internal error: </source>
        <translation>erreur interne&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1564"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1655"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1566"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1657"/>
        <source>unknown error</source>
        <translation>erreur inconnue</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1604"/>
        <source>failed to get outputs to mix</source>
        <translation>échec de la récupération de sorties à mélanger</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1745"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1781"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1829"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1857"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1885"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1906"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2382"/>
        <source>Failed to parse txid</source>
        <translation>Échec de l&apos;analyse de l&apos;ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1765"/>
        <source>no tx keys found for this txid</source>
        <translation>aucune clé de transaction trouvée pour cet ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1789"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1798"/>
        <source>Failed to parse tx key</source>
        <translation>Échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1807"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1836"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1864"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1945"/>
        <source>Failed to parse address</source>
        <translation>Échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1950"/>
        <source>Address must not be a subaddress</source>
        <translation>L&apos;adresse ne doit pas être une sous-adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1990"/>
        <source>The wallet must be in multisig ready state</source>
        <translation>Le portefeuille doit être multisig et prêt</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2012"/>
        <source>Given string is not a key</source>
        <translation>La chaîne entrée n&apos;est pas une clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2254"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>Réexaminer les dépenses ne peut se faire qu&apos;avec un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2303"/>
        <source>Invalid output: </source>
        <translation>Sortie invalide : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2310"/>
        <source>Failed to mark outputs as spent</source>
        <translation>Échec du marquage des sorties comme dépensées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2332"/>
        <source>Failed to mark output as spent</source>
        <translation>Échec du marquage de la sortie comme dépensée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2354"/>
        <source>Failed to mark output as unspent</source>
        <translation>Échec du marquage de la sortie comme non dépensée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2321"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2343"/>
        <source>Failed to parse output amount</source>
        <translation>Échec de l&apos;analyse du montant de la sortie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2326"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2348"/>
        <source>Failed to parse output offset</source>
        <translation>Échec de l&apos;analyse de l&apos;offset de la sortie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2365"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2404"/>
        <source>Failed to parse key image</source>
        <translation>Échec de l&apos;analyse de l&apos;image de clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2371"/>
        <source>Failed to get ring</source>
        <translation>Échec de la récupération du cercle</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2389"/>
        <source>Failed to get rings</source>
        <translation>Échec de la récupération des cercles</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2410"/>
        <source>Failed to set ring</source>
        <translation>Échec de l&apos;affectation du cercle</translation>
    </message>
</context>
<context>
    <name>Wallet</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="344"/>
        <source>Failed to parse address</source>
        <translation>Échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="351"/>
        <source>Failed to parse key</source>
        <translation>Échec de l&apos;analyse de la clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="359"/>
        <source>failed to verify key</source>
        <translation>Échec de la vérification de la clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="369"/>
        <source>key does not match address</source>
        <translation>la clé ne correspond pas à l&apos;adresse</translation>
    </message>
</context>
<context>
    <name>command_line</name>
    <message>
        <location filename="../src/common/command_line.cpp" line="54"/>
        <source>yes</source>
        <translation>oui</translation>
    </message>
    <message>
        <location filename="../src/common/command_line.cpp" line="68"/>
        <source>no</source>
        <translation>non</translation>
    </message>
</context>
<context>
    <name>cryptonote::rpc_args</name>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="92"/>
        <source>Specify IP to bind RPC server</source>
        <translation>Spécifier l&apos;IP à laquelle lier le serveur RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="93"/>
        <source>Specify IPv6 address to bind RPC server</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="94"/>
        <source>Allow IPv6 for RPC</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="95"/>
        <source>Require successful IPv4 bind for RPC</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="96"/>
        <source>Specify username[:password] required for RPC server</source>
        <translation>Spécifier le nom_utilisateur[:mot_de_passe] requis pour le serveur RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="97"/>
        <source>Confirm rpc-bind-ip value is NOT a loopback (local) IP</source>
        <translation>Confirmer que la valeur de rpc-bind-ip n&apos;est PAS une IP de bouclage (locale)</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="98"/>
        <source>Specify a comma separated list of origins to allow cross origin resource sharing</source>
        <translation>Spécifier une liste d&apos;origines séparées par des virgules pour autoriser le partage de ressource de différentes origines (CORS)</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="99"/>
        <source>Enable SSL on RPC connections: enabled|disabled|autodetect</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="100"/>
        <source>Path to a PEM format private key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="101"/>
        <source>Path to a PEM format certificate</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="102"/>
        <source>Path to file containing concatenated PEM format certificate(s) to replace system CA(s).</source>
        <translation type="unfinished"></translation>
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
        <translation>Adresse IP invalide fournie pour --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="154"/>
        <location filename="../src/rpc/rpc_args.cpp" line="182"/>
        <source> permits inbound unencrypted external connections. Consider SSH tunnel or SSL proxy instead. Override with --</source>
        <translation> autorise les connexions entrantes non cryptées venant de l&apos;extérieur. Considérez plutôt un tunnel SSH ou un proxy SSL. Outrepasser avec --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="205"/>
        <source>Username specified with --</source>
        <translation>Le nom d&apos;utilisateur spécifié avec --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="205"/>
        <location filename="../src/rpc/rpc_args.cpp" line="215"/>
        <source> cannot be empty</source>
        <translation> ne peut pas être vide</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="215"/>
        <source> requires RPC server password --</source>
        <translation> nécessite le mot de passe du serveur RPC --</translation>
    </message>
</context>
<context>
    <name>cryptonote::simple_wallet</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="665"/>
        <source>Commands: </source>
        <translation>Commandes&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4778"/>
        <source>failed to read wallet password</source>
        <translation>échec de la lecture du mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4365"/>
        <source>invalid password</source>
        <translation>mot de passe invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3397"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation>set seed&#xa0;: requiert un argument. options disponibles&#xa0;: language</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3435"/>
        <source>set: unrecognized argument(s)</source>
        <translation>set&#xa0;: argument(s) non reconnu(s)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4618"/>
        <source>wallet file path not valid: </source>
        <translation>chemin du fichier portefeuille non valide&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3505"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation>Tentative de génération ou de restauration d&apos;un portefeuille, mais le fichier spécifié existe déjà. Sortie pour ne pas risquer de l&apos;écraser.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3383"/>
        <source>needs an argument</source>
        <translation>requiert un argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3406"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3407"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3408"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3410"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3413"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3418"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3419"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3421"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3423"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3424"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3425"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3428"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3429"/>
        <source>0 or 1</source>
        <translation>0 ou 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3416"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3420"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3427"/>
        <source>unsigned integer</source>
        <translation>entier non signé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3664"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-deterministic-wallet utilise --generate-new-wallet, pas --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3693"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation>spécifiez un paramètre de récupération avec --electrum-seed=&quot;liste de mots ici&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4073"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation>spécifiez un chemin de portefeuille avec --generate-new-wallet (pas --wallet-file)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4298"/>
        <source>wallet failed to connect to daemon: </source>
        <translation>échec de la connexion du portefeuille au démon&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4306"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation>Le démon utilise une version majeure de RPC (%u) différente de celle du portefeuille (%u)&#xa0;: %s. Mettez l&apos;un des deux à jour, ou utilisez --allow-mismatched-daemon-version.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4327"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation>Liste des langues disponibles pour la phrase mnémonique de votre portefeuille&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4413"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Veuillez dorénavant utiliser la nouvelle phrase mnémonique que nous fournissons.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4429"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4503"/>
        <source>Generated new wallet: </source>
        <translation>Nouveau portefeuille généré&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4438"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4508"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4552"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4607"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4649"/>
        <source>Opened watch-only wallet</source>
        <translation>Ouverture du portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4653"/>
        <source>Opened wallet</source>
        <translation>Ouverture du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4671"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Veuillez procéder à la mise à jour de votre portefeuille.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4686"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Le format de votre fichier portefeuille est en cours de mise à jour.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4694"/>
        <source>failed to load wallet: </source>
        <translation>échec du chargement du portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4711"/>
        <source>Use the &quot;help&quot; command to see the list of available commands.
</source>
        <translation>Utilisez la commande &quot;help&quot; pour voir la liste des commandes disponibles.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4756"/>
        <source>Wallet data saved</source>
        <translation>Données du portefeuille sauvegardées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4961"/>
        <source>Mining started in daemon</source>
        <translation>La mine a démarré dans le démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4963"/>
        <source>mining has NOT been started: </source>
        <translation>la mine n&apos;a PAS démarré&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4983"/>
        <source>Mining stopped in daemon</source>
        <translation>La mine a été stoppée dans le démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4985"/>
        <source>mining has NOT been stopped: </source>
        <translation>la mine n&apos;a PAS été stoppée&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5067"/>
        <source>Blockchain saved</source>
        <translation>Chaîne de blocs sauvegardée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5086"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5125"/>
        <source>Height </source>
        <translation>Hauteur </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5127"/>
        <source>spent </source>
        <translation>dépensé </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5237"/>
        <source>Starting refresh...</source>
        <translation>Démarrage du rafraîchissement...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5263"/>
        <source>Refresh done, blocks received: </source>
        <translation>Rafraîchissement effectué, blocs reçus&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6604"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5950"/>
        <source>bad locked_blocks parameter:</source>
        <translation>mauvais paramètre locked_blocks&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6624"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6900"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation>une unique transaction ne peut pas utiliser plus d&apos;un ID de paiement&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6052"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6633"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6868"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6908"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>échec de la définition de l&apos;ID de paiement, bien qu&apos;il ait été décodé correctement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1064"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1186"/>
        <source>Send this multisig info to all other participants, then use exchange_multisig_keys &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Envoyez ces infos multisig à tous les autres participants, puis utilisez exchange_multisig_keys &lt;info1&gt; [&lt;info2&gt;...] avec les infos multisig des autres</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1195"/>
        <source>Multisig wallet has been successfully created. Current wallet type: </source>
        <translation>Le portefeuille multisig a été créé avec succès. Type du portefeuille actuel : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1201"/>
        <source>Failed to perform multisig keys exchange: </source>
        <translation>Échec de l&apos;échange de clés multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1528"/>
        <source>Failed to load multisig transaction from MMS</source>
        <translation>Échec du chargement de la transaction multisig à partir du MMS</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1996"/>
        <source>Failed to mark output spent: </source>
        <translation>Échec du marquage de la sortie comme dépensée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2023"/>
        <source>Failed to mark output unspent: </source>
        <translation>Échec du marquage de la sortie comme non dépensée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2047"/>
        <source>Spent: </source>
        <translation>Dépensé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2049"/>
        <source>Not spent: </source>
        <translation>Non dépensé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2053"/>
        <source>Failed to check whether output is spent: </source>
        <translation>Impossible de vérifier si la sortie est dépensée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2202"/>
        <source>Please confirm the transaction on the device</source>
        <translation>Veuillez confirmer la transaction sur l&apos;appareil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2740"/>
        <source>Device name not specified</source>
        <translation>Nom de l&apos;appareil non spécifié</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2749"/>
        <source>Device reconnect failed</source>
        <translation>Échec de la reconnexion à l&apos;appareil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2754"/>
        <source>Device reconnect failed: </source>
        <translation>Échec de la reconnexion à l&apos;appareil : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3044"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="3054"/>
        <source>export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;filepath&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3055"/>
        <source>Export to CSV the incoming/outgoing transfers within an optional height range.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3097"/>
        <source>Export a signed set of key images to a &lt;filename&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3105"/>
        <source>Synchronizes key images with the hw wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3128"/>
        <source>Generate a new random full size payment id (obsolete). These will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3144"/>
        <source>Performs extra multisig keys exchange rounds. Needed for arbitrary M/N multisig wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3168"/>
        <source>Interface with the MMS (Multisig Messaging System)
&lt;subcommand&gt; is one of:
  init, info, signer, list, next, sync, transfer, delete, send, receive, export, note, show, set, help
  send_signer_config, start_auto_config, stop_auto_config, auto_config
Get help about a subcommand with: help mms &lt;subcommand&gt;, or mms help &lt;subcommand&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3176"/>
        <source>Initialize and configure the MMS for M/N = number of required signers/number of authorized signers multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3180"/>
        <source>Display current MMS configuration</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3184"/>
        <source>Set or modify authorized signer info (single-word label, transport address, Monero address), or list all signers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3188"/>
        <source>List all messages</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3192"/>
        <source>Evaluate the next possible multisig-related action(s) according to wallet state, and execute or offer for choice
By using &apos;sync&apos; processing of waiting messages with multisig sync info can be forced regardless of wallet state</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3197"/>
        <source>Force generation of multisig sync info regardless of wallet state, to recover from special situations like &quot;stale data&quot; errors</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3201"/>
        <source>Initiate transfer with MMS support; arguments identical to normal &apos;transfer&apos; command arguments, for info see there</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3205"/>
        <source>Delete a single message by giving its id, or delete all messages by using &apos;all&apos;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3209"/>
        <source>Send a single message by giving its id, or send all waiting messages</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3213"/>
        <source>Check right away for new messages to receive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3217"/>
        <source>Write the content of a message to a file &quot;mms_message_content&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3221"/>
        <source>Send a one-line message to an authorized signer, identified by its label, or show any waiting unread notes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3225"/>
        <source>Show detailed info about a single message</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3229"/>
        <source>Available options:
 auto-send &lt;1|0&gt;
   Whether to automatically send newly generated messages right away.
 </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3235"/>
        <source>Send completed signer config to all other authorized signers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3239"/>
        <source>Start auto-config at the auto-config manager&apos;s wallet by issuing auto-config tokens and optionally set others&apos; labels</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3243"/>
        <source>Delete any auto-config tokens and abort a auto-config process</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3247"/>
        <source>Start auto-config by using the token received from the auto-config manager</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3269"/>
        <source>Mark output(s) as spent so they never get selected as fake outputs in a ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3273"/>
        <source>Marks an output as unspent so it may get selected as a fake output in a ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3277"/>
        <source>Checks whether an output is marked as spent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3432"/>
        <source>&lt;device_name[:device_spec]&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3454"/>
        <source>wrong number range, use: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3559"/>
        <source>NOTE: the following %s can be used to recover access to your wallet. Write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3561"/>
        <source>string</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3561"/>
        <source>25 words</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3982"/>
        <source>Secret spend key (%u of %u)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4059"/>
        <source>Use --restore-height or --restore-date if you want to restore an already setup account from a specific height.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4061"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4149"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6238"/>
        <source>Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4172"/>
        <source>Still apply restore height?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4337"/>
        <source>Enter the number corresponding to the language of your choice</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5102"/>
        <source>WARNING: this transaction uses an unencrypted payment ID: consider using subaddresses instead.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5102"/>
        <source>WARNING: this transaction uses an unencrypted payment ID: these are obsolete. Support will be withdrawn in the future. Use subaddresses instead.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5169"/>
        <source>Device requires attention</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5175"/>
        <source>Enter device PIN</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5177"/>
        <source>Failed to read device PIN</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5184"/>
        <source>Please enter the device passphrase on the device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5189"/>
        <source>Enter device passphrase</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5191"/>
        <source>Failed to read device passphrase</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5207"/>
        <source>The first refresh has finished for the HW-based wallet with received money. hw_key_images_sync is needed. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4895"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5209"/>
        <source>Do you want to do it now? (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5211"/>
        <source>hw_key_images_sync skipped. Run command manually before a transfer.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5411"/>
        <source>Invalid keyword: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6069"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6149"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6158"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6245"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6394"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6647"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6668"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6711"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6922"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6967"/>
        <source>transaction cancelled.</source>
        <translation>transaction annulée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6138"/>
        <source>Failed to check for backlog: </source>
        <translation>Échec de la vérification du backlog&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6187"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6684"/>
        <source>
Transaction </source>
        <translation>
Transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6192"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6689"/>
        <source>Spending from address index %d
</source>
        <translation>Dépense depuis l&apos;adresse d&apos;index %d
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6194"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6691"/>
        <source>WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.
</source>
        <translation>ATTENTION&#xa0;: Des sorties de multiples adresses sont utilisées ensemble, ce qui pourrait potentiellement compromettre votre confidentialité.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6196"/>
        <source>Sending %s.  </source>
        <translation>Envoi de %s.  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6199"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation>Votre transaction doit être scindée en %llu transactions. Il en résulte que des frais de transaction doivent être appliqués à chaque transaction, pour un total de %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6205"/>
        <source>The transaction fee is %s</source>
        <translation>Les frais de transaction sont de %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6208"/>
        <source>, of which %s is dust from change</source>
        <translation>, dont %s est de la poussière de monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6209"/>
        <source>.</source>
        <translation>.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6209"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation>Un total de %s de poussière de monnaie rendue sera envoyé à une adresse de poussière</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6258"/>
        <source>Unsigned transaction(s) successfully written to MMS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6266"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6303"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6405"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6417"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6722"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6759"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6977"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6989"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Échec de l&apos;écriture de(s) transaction(s) dans le fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6271"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6308"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6409"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6421"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6726"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6763"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6981"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6993"/>
        <source>Unsigned transaction(s) successfully written to file: </source>
        <translation>Transaction(s) non signée(s) écrite(s) dans le fichier avec succès&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6280"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6738"/>
        <source>Failed to cold sign transaction with HW wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6364"/>
        <source>No unmixable outputs found</source>
        <translation>Aucune sortie non mélangeable trouvée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6379"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6696"/>
        <source>Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6385"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6702"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6959"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6432"/>
        <source>Discarding %s of unmixable outputs that cannot be spent, which can be undone by &quot;rescan_spent&quot;.  Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6471"/>
        <source>No address given</source>
        <translation>Aucune adresse fournie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6862"/>
        <source>failed to parse Payment ID</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2083"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2130"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6885"/>
        <source>failed to parse key image</source>
        <translation>échec de l&apos;analyse de l&apos;image de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6939"/>
        <source>No outputs found</source>
        <translation>Pas de sorties trouvées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6944"/>
        <source>Multiple transactions are created, which is not supposed to happen</source>
        <translation>De multiples transactions sont crées, ce qui n&apos;est pas supposé arriver</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6949"/>
        <source>The transaction uses multiple or no inputs, which is not supposed to happen</source>
        <translation>La transaction utilise aucune ou de multiples entrées, ce qui n&apos;est pas supposé arriver</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7027"/>
        <source>missing threshold amount</source>
        <translation>montant seuil manquant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7032"/>
        <source>invalid amount threshold</source>
        <translation>montant seuil invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7243"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu, %s. %sIs this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8445"/>
        <source>Rescan anyway?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8553"/>
        <source>locked due to inactivity</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9012"/>
        <source>Short payment IDs are to be used within an integrated address only</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9835"/>
        <source> (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9862"/>
        <source>Choose processing:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9871"/>
        <source>Sign tx</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9879"/>
        <source>Send the tx for submission to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9883"/>
        <source>Send the tx for signing to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9890"/>
        <source>Submit tx</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9893"/>
        <source>unknown</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9899"/>
        <source>Choice: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9911"/>
        <source>Wrong choice</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9918"/>
        <source>Id</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9918"/>
        <source>I/O</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9918"/>
        <source>Authorized Signer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9919"/>
        <source>Message Type</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9919"/>
        <source>Height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9919"/>
        <source>R</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9919"/>
        <source>Message State</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9919"/>
        <source>Since</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9936"/>
        <source> ago</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9942"/>
        <source>#</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9942"/>
        <source>Transport Address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9943"/>
        <source>Auto-Config Token</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9943"/>
        <source>Monero Address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9947"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9955"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9957"/>
        <source>&lt;not set&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9998"/>
        <source>Message </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9999"/>
        <source>In/out: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10001"/>
        <source>State: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10001"/>
        <source>%s since %s, %s ago</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10005"/>
        <source>Sent: Never</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10009"/>
        <source>Sent: %s, %s ago</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10012"/>
        <source>Authorized signer: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10013"/>
        <source>Content size: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10013"/>
        <source> bytes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10014"/>
        <source>Content: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10014"/>
        <source>(binary data)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10044"/>
        <source>Send these messages now?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10054"/>
        <source>Queued for sending.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10074"/>
        <source>Invalid message id</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10083"/>
        <source>usage: mms init &lt;required_signers&gt;/&lt;authorized_signers&gt; &lt;own_label&gt; &lt;own_transport_address&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10089"/>
        <source>The MMS is already initialized. Re-initialize by deleting all signer info and messages?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10104"/>
        <source>Error in the number of required signers and/or authorized signers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10121"/>
        <source>The MMS is not active.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10144"/>
        <source>Invalid signer number </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10149"/>
        <source>mms signer [&lt;number&gt; &lt;label&gt; [&lt;transport_address&gt; [&lt;monero_address&gt;]]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10168"/>
        <source>Invalid Monero address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10175"/>
        <source>Wallet state does not allow changing Monero addresses anymore</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10187"/>
        <source>Usage: mms list</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10200"/>
        <source>Usage: mms next [sync]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10225"/>
        <source>No next step: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10235"/>
        <source>prepare_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10241"/>
        <source>make_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10256"/>
        <source>exchange_multisig_keys</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10271"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10391"/>
        <source>export_multisig_info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10280"/>
        <source>import_multisig_info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10293"/>
        <source>sign_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10303"/>
        <source>submit_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10313"/>
        <source>Send tx</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10324"/>
        <source>Process signer config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10336"/>
        <source>Replace current signer config with the one displayed above?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10350"/>
        <source>Process auto config data</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10364"/>
        <source>Nothing ready to process</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10384"/>
        <source>Usage: mms sync</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10408"/>
        <source>Usage: mms delete (&lt;message_id&gt; | all)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10415"/>
        <source>Delete all messages?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10441"/>
        <source>Usage: mms send [&lt;message_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10458"/>
        <source>Usage: mms receive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10475"/>
        <source>Usage: mms export &lt;message_id&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10487"/>
        <source>Message content saved to: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10491"/>
        <source>Failed to to save message content</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10515"/>
        <source>Usage: mms note [&lt;label&gt; &lt;text&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10522"/>
        <source>No signer found with label </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10544"/>
        <source>Usage: mms show &lt;message_id&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10563"/>
        <source>Usage: mms set &lt;option_name&gt; [&lt;option_value&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10580"/>
        <source>Wrong option value</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10585"/>
        <source>Auto-send is on</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10585"/>
        <source>Auto-send is off</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10590"/>
        <source>Unknown option</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10598"/>
        <source>Usage: mms help [&lt;subcommand&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10614"/>
        <source>Usage: mms send_signer_config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10620"/>
        <source>Signer config not yet complete</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10635"/>
        <source>Usage: mms start_auto_config [&lt;label&gt; &lt;label&gt; ...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10640"/>
        <source>There are signers without a label set. Complete labels before auto-config or specify them as parameters here.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10646"/>
        <source>Auto-config is already running. Cancel and restart?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10670"/>
        <source>Usage: mms stop_auto_config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10673"/>
        <source>Delete any auto-config tokens and stop auto-config?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10686"/>
        <source>Usage: mms auto_config &lt;auto_config_token&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10693"/>
        <source>Invalid auto-config token</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10699"/>
        <source>Auto-config already running. Cancel and restart?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10717"/>
        <source>MMS not available in this wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10741"/>
        <source>The MMS is not active. Activate using the &quot;mms init&quot; command</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10818"/>
        <source>Invalid MMS subcommand</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10823"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10827"/>
        <source>Error in MMS command: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7182"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>La monnaie réclamée ne va pas à une adresse payée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7187"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>La monnaie réclamée est supérieure au paiement à l&apos;adresse de monnaie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7218"/>
        <source>sending %s to %s</source>
        <translation>envoi de %s à %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7228"/>
        <source> dummy output(s)</source>
        <translation> sortie(s) factice(s)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7231"/>
        <source>with no destinations</source>
        <translation>sans destination</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7272"/>
        <source>This is a multisig wallet, it can only sign with sign_multisig</source>
        <translation>Ceci est un portefeuille multisig, il ne peut signer qu&apos;avec sign_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7295"/>
        <source>Failed to sign transaction</source>
        <translation>Échec de signature de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7301"/>
        <source>Failed to sign transaction: </source>
        <translation>Échec de signature de transaction&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7322"/>
        <source>Transaction raw hex data exported to </source>
        <translation>Données brutes hex de la transaction exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7343"/>
        <source>Failed to load transaction from file</source>
        <translation>Échec du chargement de la transaction du fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5280"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5607"/>
        <source>RPC error: </source>
        <translation>Erreur RPC&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="715"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il n&apos;a pas de clé de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="865"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1049"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1102"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1169"/>
        <source>Your original password was incorrect.</source>
        <translation>Votre mot de passe original est incorrect.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="880"/>
        <source>Error with wallet rewrite: </source>
        <translation>Erreur avec la réécriture du portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2466"/>
        <source>invalid unit</source>
        <translation>unité invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2484"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2546"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation>nombre invalide&#xa0;: un entier non signé est attendu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2502"/>
        <source>invalid value</source>
        <translation>valeur invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4137"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4157"/>
        <source>bad m_restore_height parameter: </source>
        <translation>mauvais paramètre m_restore_height&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4101"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4148"/>
        <source>Restore height is: </source>
        <translation>La hauteur de restauration est&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5039"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Le démon est local, supposons qu&apos;il est de confiance</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4774"/>
        <source>Password for new watch-only wallet</source>
        <translation>Mot de passe pour le nouveau portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5290"/>
        <source>internal error: </source>
        <translation>erreur interne&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1637"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5295"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5612"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1563"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1642"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5300"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5617"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6294"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6324"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6450"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6751"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6778"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7010"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7356"/>
        <source>unknown error</source>
        <translation>erreur inconnue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5305"/>
        <source>refresh failed: </source>
        <translation>échec du rafraîchissement&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5305"/>
        <source>Blocks received: </source>
        <translation>Blocs reçus&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5343"/>
        <source>unlocked balance: </source>
        <translation>solde débloqué&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3417"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5450"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5512"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8206"/>
        <source>amount</source>
        <translation>montant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="361"/>
        <source>false</source>
        <translation>faux</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="679"/>
        <source>Unknown command: </source>
        <translation>Commande inconnue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="686"/>
        <source>Command usage: </source>
        <translation>Usage de la commande&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="689"/>
        <source>Command description: </source>
        <translation>Description de la commande&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="755"/>
        <source>wallet is multisig but not yet finalized</source>
        <translation>le portefeuille est multisig mais pas encore finalisé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="788"/>
        <source>Failed to retrieve seed</source>
        <translation>Échec de la récupération de la phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="818"/>
        <source>wallet is multisig and has no seed</source>
        <translation>le portefeuille est multisig et n&apos;a pas de phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="927"/>
        <source>Error: failed to estimate backlog array size: </source>
        <translation>Erreur&#xa0;: échec de l&apos;estimation de la taille du tableau d&apos;arriéré&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="932"/>
        <source>Error: bad estimated backlog array size</source>
        <translation>Erreur&#xa0;: mauvaise estimation de la taille du tableau d&apos;arriéré</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="944"/>
        <source> (current)</source>
        <translation> (actuel)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="947"/>
        <source>%u block (%u minutes) backlog at priority %u%s</source>
        <translation>arriéré de %u bloc(s) (%u minutes) à la priorité %u%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="949"/>
        <source>%u to %u block (%u to %u minutes) backlog at priority %u</source>
        <translation>arriéré de %u à %u bloc(s) (%u à %u minutes) à la priorité %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="952"/>
        <source>No backlog at priority </source>
        <translation>Pas d&apos;arriéré à la priorité </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="972"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1017"/>
        <source>This wallet is already multisig</source>
        <translation>Le portefeuille est déjà multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="977"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1022"/>
        <source>wallet is watch-only and cannot be made multisig</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas être tranformé en multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="983"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1028"/>
        <source>This wallet has been used before, please use a new wallet to create a multisig wallet</source>
        <translation>Ce portefeuille a été utilisé auparavant, veuillez utiliser un nouveau portefeuille pour créer un portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="991"/>
        <source>Send this multisig info to all other participants, then use make_multisig &lt;threshold&gt; &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Envoyez ces infos multisig à tous les autres participants, ensuite utilisez make_multisig &lt;seuil&gt; &lt;info1&gt; [&lt;info2&gt;...] avec les infos multisig des autres</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="992"/>
        <source>This includes the PRIVATE view key, so needs to be disclosed only to that multisig wallet&apos;s participants </source>
        <translation>Ceci inclut la clé PRIVÉE d&apos;audit, donc ne doit être divulgué qu&apos;aux participants de ce portefeuille multisig </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1042"/>
        <source>Invalid threshold</source>
        <translation>Seuil invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1062"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1184"/>
        <source>Another step is needed</source>
        <translation>Une autre étape est nécessaire</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1074"/>
        <source>Error creating multisig: </source>
        <translation>Erreur de création multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1081"/>
        <source>Error creating multisig: new wallet is not multisig</source>
        <translation>Erreur de création multisig&#xa0;: le nouveau portefeuille n&apos;est pas multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1084"/>
        <source> multisig address: </source>
        <translation> adresse multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1108"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1157"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1224"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1290"/>
        <source>This wallet is not multisig</source>
        <translation>Ce portefeuille n&apos;est pas multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1113"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1162"/>
        <source>This wallet is already finalized</source>
        <translation>Ce portefeuille est déjà finalisé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1129"/>
        <source>Failed to finalize multisig</source>
        <translation>Échec de finalisation multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1135"/>
        <source>Failed to finalize multisig: </source>
        <translation>Échec de finalisation multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1196"/>
        <source>Multisig address: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1229"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1295"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1389"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1505"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1586"/>
        <source>This multisig wallet is not yet finalized</source>
        <translation>Ce portefeuille multisig n&apos;est pas encore finalisé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1265"/>
        <source>Error exporting multisig info: </source>
        <translation>Erreur d&apos;importation des infos multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1269"/>
        <source>Multisig info exported to </source>
        <translation>Infos multisig exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1335"/>
        <source>Multisig info imported</source>
        <translation>Infos multisig importées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1339"/>
        <source>Failed to import multisig info: </source>
        <translation>Échec de l&apos;importation des infos multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1350"/>
        <source>Failed to update spent status after importing multisig info: </source>
        <translation>Échec de la mise à jour de l&apos;état des dépenses après l&apos;importation des infos multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1356"/>
        <source>Untrusted daemon, spent status may be incorrect. Use a trusted daemon and run &quot;rescan_spent&quot;</source>
        <translation>Pas un démon de confiance, l&apos;état des dépenses peut être incorrect. Utilisez un démon de confiance et executez &quot;rescan_spent&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1384"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1500"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1581"/>
        <source>This is not a multisig wallet</source>
        <translation>Ceci n&apos;est pas un portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1434"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1443"/>
        <source>Failed to sign multisig transaction</source>
        <translation>Échec de la signature de la transaction multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1450"/>
        <source>Multisig error: </source>
        <translation>Erreur multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1455"/>
        <source>Failed to sign multisig transaction: </source>
        <translation>Échec de la signature de la transaction multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1478"/>
        <source>It may be relayed to the network with submit_multisig</source>
        <translation>Elle peut être transmise au réseau avec submit_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1537"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1607"/>
        <source>Failed to load multisig transaction from file</source>
        <translation>Échec du chargement de la transaction multisig du fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1543"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1612"/>
        <source>Multisig transaction signed by only %u signers, needs %u more signatures</source>
        <translation>Transaction multisig signée par %u signataire(s) seulement, nécessite %u signature(s) de plus</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1552"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9707"/>
        <source>Transaction successfully submitted, transaction </source>
        <translation>Transaction transmise avec succès, transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1553"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9708"/>
        <source>You can check its status by using the `show_transfers` command.</source>
        <translation>Vous pouvez vérifier son statut en utilisant la commane &apos;show_transfers&apos;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1628"/>
        <source>Failed to export multisig transaction to file </source>
        <translation>Échec de l&apos;exportation de la transaction multisig vers le fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1632"/>
        <source>Saved exported multisig transaction file(s): </source>
        <translation>Transaction multisig enregistrée dans le(s) fichier(s)&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1897"/>
        <source>Invalid key image or txid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1906"/>
        <source>failed to unset ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2077"/>
        <source>usage: %s &lt;key_image&gt;|&lt;pubkey&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2122"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2134"/>
        <source>Frozen: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2136"/>
        <source>Not frozen: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2150"/>
        <source> bytes sent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2151"/>
        <source> bytes received</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2157"/>
        <source>Welcome to Monero, the private cryptocurrency.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2159"/>
        <source>Monero, like Bitcoin, is a cryptocurrency. That is, it is digital money.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2160"/>
        <source>Unlike Bitcoin, your Monero transactions and balance stay private, and not visible to the world by default.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2161"/>
        <source>However, you have the option of making those available to select parties, if you choose to.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2163"/>
        <source>Monero protects your privacy on the blockchain, and while Monero strives to improve all the time,</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2164"/>
        <source>no privacy technology can be 100% perfect, Monero included.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2165"/>
        <source>Monero cannot protect you from malware, and it may not be as effective as we hope against powerful adversaries.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2166"/>
        <source>Flaws in Monero may be discovered in the future, and attacks may be developed to peek under some</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2167"/>
        <source>of the layers of privacy Monero provides. Be safe and practice defense in depth.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2169"/>
        <source>Welcome to Monero and financial privacy. For more information, see https://getmonero.org/</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2275"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2281"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2300"/>
        <source>ring size must be an integer &gt;= </source>
        <translation>la taille de cercle doit être un nombre entier &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2305"/>
        <source>could not change default ring size</source>
        <translation>échec du changement de la taille de cercle par défaut</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2580"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2651"/>
        <source>Invalid height</source>
        <translation>Hauteur invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2721"/>
        <source>invalid argument: must be either 1/yes or 0/no</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2824"/>
        <source>Start mining in the daemon (bg_mining and ignore_battery are optional booleans).</source>
        <translation>Démarrer la mine dans le démon (mine_arrière_plan et ignorer_batterie sont des booléens facultatifs).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2827"/>
        <source>Stop mining in the daemon.</source>
        <translation>Arrêter la mine dans le démon.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2831"/>
        <source>Set another daemon to connect to.</source>
        <translation>Spécifier un autre démon auquel se connecter.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2834"/>
        <source>Save the current blockchain data.</source>
        <translation>Sauvegarder les données actuelles de la châine de blocs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2837"/>
        <source>Synchronize the transactions and balance.</source>
        <translation>Synchroniser les transactions et le solde.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2841"/>
        <source>Show the wallet&apos;s balance of the currently selected account.</source>
        <translation>Afficher le solde du compte actuellement sélectionné.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2845"/>
        <source>Show the incoming transfers, all or filtered by availability and address index.

Output format:
Amount, Spent(&quot;T&quot;|&quot;F&quot;), &quot;frozen&quot;|&quot;locked&quot;|&quot;unlocked&quot;, RingCT, Global Index, Transaction Hash, Address Index, [Public Key, Key Image] </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2851"/>
        <source>Show the payments for the given payment IDs.</source>
        <translation>Afficher les paiements pour les IDs de paiement donnés.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2854"/>
        <source>Show the blockchain height.</source>
        <translation>Afficher la hauteur de la chaîne de blocs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2868"/>
        <source>Send all unmixable outputs to yourself with ring_size 1</source>
        <translation>Envoyer toutes les sorties non mélangeables à vous-même avec une taille de cercle de 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2875"/>
        <source>Send all unlocked outputs below the threshold to an address.</source>
        <translation>Envoyer toutes les sorties débloquées d&apos;un montant inférieur au seuil à une adresse.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2879"/>
        <source>Send a single output of the given key image to an address without change.</source>
        <translation>Envoyer une unique sortie ayant une image de clé donnée à une adresse sans rendu de monnaie.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2883"/>
        <source>Donate &lt;amount&gt; to the development team (donate.getmonero.org).</source>
        <translation>Donner &lt;montant&gt; à l&apos;équipe de développement (donate.getmonero.org).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2890"/>
        <source>Submit a signed transaction from a file.</source>
        <translation>Transmettre une transaction signée d&apos;un fichier.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2894"/>
        <source>Change the current log detail (level must be &lt;0-4&gt;).</source>
        <translation>Changer le niveau de détail du journal (le niveau doit être &lt;0-4&gt;).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2898"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="2912"/>
        <source>Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation>Encoder un ID de paiement dans une adresse intégrée pour l&apos;adresse publique du portefeuille actuel (en l&apos;absence d&apos;argument un ID de paiement aléatoire est utilisé), ou décoder une adresse intégrée en une adresse standard et un ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2916"/>
        <source>Print all entries in the address book, optionally adding/deleting an entry to/from it.</source>
        <translation>Afficher toutes les entrées du carnet d&apos;adresses, optionnellement en y ajoutant/supprimant une entrée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2919"/>
        <source>Save the wallet data.</source>
        <translation>Sauvegarder les données du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2922"/>
        <source>Save a watch-only keys file.</source>
        <translation>Sauvegarder un fichier de clés d&apos;audit.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2925"/>
        <source>Display the private view key.</source>
        <translation>Afficher la clé privée d&apos;audit.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2928"/>
        <source>Display the private spend key.</source>
        <translation>Afficher la clé privée de dépense.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2931"/>
        <source>Display the Electrum-style mnemonic seed</source>
        <translation>Afficher la phrase mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2998"/>
        <source>Display the encrypted Electrum-style mnemonic seed.</source>
        <translation>Afficher la phrase mnémonique de style Electrum chiffrée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3001"/>
        <source>Rescan the blockchain for spent outputs.</source>
        <translation>Rescanner la chaîne de blocs pour trouver les sorties dépensées.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3005"/>
        <source>Get the transaction key (r) for a given &lt;txid&gt;.</source>
        <translation>Obtenir la clé de transaction (r) pour un &lt;ID_transaction&gt; donné.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3013"/>
        <source>Check the amount going to &lt;address&gt; in &lt;txid&gt;.</source>
        <translation>Vérifier le montant allant à &lt;adresse&gt; dans &lt;ID_transaction&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3017"/>
        <source>Generate a signature proving funds sent to &lt;address&gt; in &lt;txid&gt;, optionally with a challenge string &lt;message&gt;, using either the transaction secret key (when &lt;address&gt; is not your wallet&apos;s address) or the view secret key (otherwise), which does not disclose the secret key.</source>
        <translation>Générer une signature prouvant l&apos;envoi de fonds à &lt;adresse&gt; dans &lt;ID_transaction&gt;, optionnellement avec un &lt;message&gt; comme challenge, en utilisant soit la clé secrète de transaction (quand &lt;adresse&gt; n&apos;est pas l&apos;adresse de votre portefeuille) soit la clé secrète d&apos;audit (dans le cas contraire), tout en ne divulguant pas la clé secrète.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3021"/>
        <source>Check the proof for funds going to &lt;address&gt; in &lt;txid&gt; with the challenge string &lt;message&gt; if any.</source>
        <translation>Vérifier la validité de la preuve de fonds allant à &lt;adresse&gt; dans &lt;ID_transaction&gt; avec le &lt;message&gt; de challenge s&apos;il y en a un.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3025"/>
        <source>Generate a signature proving that you generated &lt;txid&gt; using the spend secret key, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Générer une signature prouvant que vous avez créé &lt;ID_transaction&gt; en utilisant la clé secrète de dépense, optionnellement avec un &lt;message&gt; comme challenge.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3029"/>
        <source>Check a signature proving that the signer generated &lt;txid&gt;, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Vérifier la validité de la preuve que le signataire a créé &lt;ID_transaction&gt;, optionnellement avec un &lt;message&gt; comme challenge.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3033"/>
        <source>Generate a signature proving that you own at least this much, optionally with a challenge string &lt;message&gt;.
If &apos;all&apos; is specified, you prove the entire sum of all of your existing accounts&apos; balances.
Otherwise, you prove the reserve of the smallest possible amount above &lt;amount&gt; available in your current account.</source>
        <translation>Générer une signature prouvant que vous possédez au moins ce montant, optionnellement avec un &lt;message&gt; comme challenge.
Si &apos;all&apos; est spécifié, vous prouvez la somme totale des soldes de tous vos comptes existants.
Sinon, vous prouvez le plus petit solde supérieur à &lt;montant&gt; dans votre compte actuel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3039"/>
        <source>Check a signature proving that the owner of &lt;address&gt; holds at least this much, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Vérifier la validité d&apos;une signature prouvant que le propriétaire d&apos;une &lt;adresse&gt; détient au moins un montant, optionnellement avec un &lt;message&gt; comme challenge.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3059"/>
        <source>Show the unspent outputs of a specified address within an optional amount range.</source>
        <translation>Afficher les sorties non dépensées d&apos;une adresse spécifique dans un interval de montants facultatif.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3063"/>
        <source>Rescan the blockchain from scratch. If &quot;hard&quot; is specified, you will lose any information which can not be recovered from the blockchain itself.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3067"/>
        <source>Set an arbitrary string note for a &lt;txid&gt;.</source>
        <translation>Définir un texte arbitraire comme note pour &lt;ID_transaction&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3071"/>
        <source>Get a string note for a txid.</source>
        <translation>Obtenir le texte de la note pour un ID de transaction.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3075"/>
        <source>Set an arbitrary description for the wallet.</source>
        <translation>Définir un texte arbitraire comme description du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3079"/>
        <source>Get the description of the wallet.</source>
        <translation>Obtenir la description du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3082"/>
        <source>Show the wallet&apos;s status.</source>
        <translation>Afficher l&apos;état du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3085"/>
        <source>Show the wallet&apos;s information.</source>
        <translation>Afficher les informations du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3089"/>
        <source>Sign the contents of a file.</source>
        <translation>Signer le contenu d&apos;un fichier.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3093"/>
        <source>Verify a signature on the contents of a file.</source>
        <translation>Vérifier la signature du contenu d&apos;un fichier.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3101"/>
        <source>Import a signed key images list and verify their spent status.</source>
        <translation>Importer un ensemble signé d&apos;images de clé et vérifier si elles correspondent à des dépenses.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3113"/>
        <source>Export a set of outputs owned by this wallet.</source>
        <translation>Exporter un ensemble de sorties possédées par ce portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3117"/>
        <source>Import a set of outputs owned by this wallet.</source>
        <translation>Importer un ensemble de sorties possédées par ce portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3121"/>
        <source>Show information about a transfer to/from this address.</source>
        <translation>Afficher les information à propos d&apos;un transfert vers/depuis cette adresse.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3124"/>
        <source>Change the wallet&apos;s password.</source>
        <translation>Changer le mot de passe du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3131"/>
        <source>Print the information about the current fee and transaction backlog.</source>
        <translation>Afficher les informations à propos des frais et arriéré de transactions actuels.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3133"/>
        <source>Export data needed to create a multisig wallet</source>
        <translation>Exporter les données nécessaires pour créer un portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3136"/>
        <source>Turn this wallet into a multisig wallet</source>
        <translation>Transformer ce portefeuille en portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3140"/>
        <source>Turn this wallet into a multisig wallet, extra step for N-1/N wallets</source>
        <translation>Transformer ce portefeuille en portefeuille multisig, étape supplémentaire pour les portefeuilles N-1/N</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3148"/>
        <source>Export multisig info for other participants</source>
        <translation>Exporter les infos multisig pour les autres participants</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3152"/>
        <source>Import multisig info from other participants</source>
        <translation>Importer les infos multisig des autres participants</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3156"/>
        <source>Sign a multisig transaction from a file</source>
        <translation>Signer une transaction multisig d&apos;un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3160"/>
        <source>Submit a signed multisig transaction from a file</source>
        <translation>Transmettre une transaction multisig signée d&apos;un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3164"/>
        <source>Export a signed multisig transaction to a file</source>
        <translation>Exporter une transaction multisig signée vers un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3261"/>
        <source>Unsets the ring used for a given key image or transaction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3281"/>
        <source>Freeze a single output by key image so it will not be used</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3285"/>
        <source>Thaw a single output by key image so it may be used again</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3289"/>
        <source>Checks whether a given output is currently frozen by key image</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3297"/>
        <source>Prints simple network stats</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3301"/>
        <source>Prints basic info about Monero for first time users</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3309"/>
        <source>Show the help section or the documentation about a &lt;command&gt;.</source>
        <translation>Afficher la section d&apos;aide ou la documentation d&apos;une &lt;commande&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3409"/>
        <source>integer &gt;= </source>
        <translation>entier &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3422"/>
        <source>block height</source>
        <translation>hauteur de bloc</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3431"/>
        <source>1/yes or 0/no</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3530"/>
        <source>No wallet found with that name. Confirm creation of new wallet named: </source>
        <translation>Aucun portefeuille avec ce nom trouvé. Confirmer la création d&apos;un nouveau portefeuille nommé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3656"/>
        <source>can&apos;t specify both --restore-deterministic-wallet or --restore-multisig-wallet and --non-deterministic</source>
        <translation>impossible de spécifier à la fois --restore-deterministic-wallet ou --restore-multisig-wallet et --non-deterministic</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3662"/>
        <source>--restore-multisig-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-multisig-wallet utilise --generate-new-wallet, pas --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3678"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;multisig seed here&quot;</source>
        <translation>spécifiez un paramètre de récupération avec --electrum-seed=&quot;phrase mnémonique multisig ici&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3707"/>
        <source>Multisig seed failed verification</source>
        <translation>Échec de la vérification de la phrase mnémonique multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3757"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3834"/>
        <source>This address is a subaddress which cannot be used here.</source>
        <translation>Cette adresse est une sous-adresse qui ne peut pas être utilisée ici.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3912"/>
        <source>Error: expected M/N, but got: </source>
        <translation>Erreur&#xa0;: M/N attendu, mais lu&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3917"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got: </source>
        <translation>Erreur&#xa0;: N &gt; 1 et N &lt;= M attendu, mais lu&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3922"/>
        <source>Error: M/N is currently unsupported. </source>
        <translation>Erreur&#xa0;: M/N n&apos;est actuellement pas supporté. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3925"/>
        <source>Generating master wallet from %u of %u multisig wallet keys</source>
        <translation>Génération du portefeuille principal à partir de %u de %u clés de portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3954"/>
        <source>failed to parse secret view key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3962"/>
        <source>failed to verify secret view key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4005"/>
        <source>Error: M/N is currently unsupported</source>
        <translation>Erreur&#xa0;: M/N n&apos;est actuellement pas supporté</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4171"/>
        <source>Restore height </source>
        <translation>Hauteur de restauration </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4226"/>
        <source>If you are new to Monero, type &quot;welcome&quot; for a brief overview.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4231"/>
        <source>WARNING: obsolete long payment IDs are enabled. Sending transactions with those payment IDs are bad for your privacy.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4233"/>
        <source>It is recommended that you do not use them, and ask recipients who ask for one to not endanger your privacy.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4299"/>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or change the daemon address using the &apos;set_daemon&apos; command.</source>
        <translation>Le démon n&apos;est pas lancé ou un mauvais port a été fourni. Veuillez vous assurer que le démon fonctionne ou changez l&apos;adresse de démon avec la commande &apos;set_daemon&apos;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4376"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4473"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4521"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4564"/>
        <source>Error creating wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4449"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="4599"/>
        <source>failed to generate new mutlisig wallet</source>
        <translation>échec de la génération du nouveau portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4602"/>
        <source>Generated new %u/%u multisig wallet: </source>
        <translation>Nouveau portefeuille multisig %u/%u généré&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4651"/>
        <source>Opened %u/%u multisig wallet%s</source>
        <translation>Portefeuille multisig %u/%u ouvert%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4712"/>
        <source>Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
</source>
        <translation>Utilisez &quot;help &lt;commande&gt;&quot; pour voir la documentation d&apos;une commande.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4770"/>
        <source>wallet is multisig and cannot save a watch-only version</source>
        <translation>c&apos;est un portefeuille multisig et il ne peut pas sauvegarder une version d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4806"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4838"/>
        <source>Failed to query mining status: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4821"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4849"/>
        <source>Failed to setup background mining: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4825"/>
        <source>Background mining enabled. Thank you for supporting the Monero network.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4853"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4861"/>
        <source>Background mining not enabled. Run &quot;set setup-background-mining 1&quot; to change.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4867"/>
        <source>Using an untrusted daemon, skipping background mining check</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4892"/>
        <source>The daemon is not set up to background mine.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4893"/>
        <source>With background mining enabled, the daemon will mine when idle and not on batttery.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4894"/>
        <source>Enabling this supports the network you are using, and makes you eligible for receiving new monero</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4899"/>
        <source>Background mining not enabled. Set setup-background-mining to 1 to change.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5006"/>
        <source>Unexpected array length - Exited simple_wallet::set_daemon()</source>
        <translation>Taille de tableau inattendue - Sortie de simple_wallet::set_daemon()</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5047"/>
        <source>This does not seem to be a valid daemon URL.</source>
        <translation>Ceci semble ne pas être une URL de démon valide.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5087"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5126"/>
        <source>txid </source>
        <translation>ID transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5089"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5128"/>
        <source>idx </source>
        <translation>index </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5106"/>
        <source>NOTE: This transaction is locked, see details with: show_transfer </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5256"/>
        <source>New transfer received since rescan was started. Key images are incomplete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5331"/>
        <source> (Some owned outputs have partial key images - import_multisig_info needed)</source>
        <translation> (Certaines sorties ont des images de clé partielles - import_multisig_info requis)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5334"/>
        <source>Currently selected account: [</source>
        <translation>Compte actuellement sélectionné&#xa0;: [</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5334"/>
        <source>] </source>
        <translation>] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5336"/>
        <source>Tag: </source>
        <translation>Mot clé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5336"/>
        <source>(No tag assigned)</source>
        <translation>(Pas de mot clé assigné)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5348"/>
        <source>Balance per address:</source>
        <translation>Solde par adresse&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5349"/>
        <source>Address</source>
        <translation>Adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5349"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8766"/>
        <source>Balance</source>
        <translation>Solde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5349"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8766"/>
        <source>Unlocked balance</source>
        <translation>Solde débloqué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5349"/>
        <source>Outputs</source>
        <translation>Sorties</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5349"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8766"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9942"/>
        <source>Label</source>
        <translation>Étiquette</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5357"/>
        <source>%8u %6s %21s %21s %7u %21s</source>
        <translation>%8u %6s %21s %21s %7u %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5450"/>
        <source>spent</source>
        <translation>dépensé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5450"/>
        <source>global index</source>
        <translation>index global</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5450"/>
        <source>tx id</source>
        <translation>ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5450"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5512"/>
        <source>addr index</source>
        <translation>index adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5460"/>
        <source>Used at heights: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5466"/>
        <source>[frozen]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5480"/>
        <source>No incoming transfers</source>
        <translation>Aucun transfert entrant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5484"/>
        <source>No incoming available transfers</source>
        <translation>Aucun transfert entrant disponible</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5488"/>
        <source>No incoming unavailable transfers</source>
        <translation>Aucun transfert entrant non disponible</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5512"/>
        <source>payment</source>
        <translation>paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5512"/>
        <source>transaction</source>
        <translation>transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5512"/>
        <source>height</source>
        <translation>hauteur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5512"/>
        <source>unlock time</source>
        <translation>durée de déverrouillage</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5524"/>
        <source>No payments with id </source>
        <translation>Aucun paiement avec l&apos;ID </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5572"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5662"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6089"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6557"/>
        <source>failed to get blockchain height: </source>
        <translation>échec de la récupération de la hauteur de la chaîne de blocs&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5670"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation>
Transaction %llu/%llu&#xa0;: ID=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5707"/>
        <source>failed to get output: </source>
        <translation>échec de la récupération de la sortie&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5715"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation>la hauteur du bloc d&apos;origine de la clé de la sortie ne devrait pas être supérieure à celle de la chaîne de blocs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5719"/>
        <source>
Originating block heights: </source>
        <translation>
Hauteurs des blocs d&apos;origine&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5731"/>
        <source>
|</source>
        <translation>
|</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5731"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8394"/>
        <source>|
</source>
        <translation>|
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5748"/>
        <source>
Warning: Some input keys being spent are from </source>
        <translation>
Attention&#xa0;: Certaines clés d&apos;entrées étant dépensées sont issues de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5750"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation>, ce qui peut casser l&apos;anonymat du cercle de signature. Assurez-vous que c&apos;est intentionnel !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5781"/>
        <source>Transaction spends more than one very old output. Privacy would be better if they were sent separately.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5782"/>
        <source>Spend them now anyway?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5888"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6509"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6808"/>
        <source>Ring size must not be 0</source>
        <translation>La taille de cercle ne doit pas être 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5900"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6521"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6820"/>
        <source>ring size %u is too small, minimum is %u</source>
        <translation>la taille de cercle %u est trop petite, le minimum est %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5912"/>
        <source>wrong number of arguments</source>
        <translation>mauvais nombre d&apos;arguments</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5932"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6042"/>
        <source>Warning: Unencrypted payment IDs will harm your privacy: ask the recipient to use subaddresses instead</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6105"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6662"/>
        <source>No outputs found, or daemon is not ready</source>
        <translation>Aucune sortie trouvée, ou le démon n&apos;est pas prêt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6214"/>
        <source>.
This transaction (including %s change) will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7079"/>
        <source>Failed to parse donation address: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7095"/>
        <source>Donating %s %s to %s.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8181"/>
        <source>usage: export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;path&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8206"/>
        <source>direction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8206"/>
        <source>timestamp</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8206"/>
        <source>running balance</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8206"/>
        <source>hash</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8206"/>
        <source>payment ID</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8206"/>
        <source>fee</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8206"/>
        <source>destination</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8206"/>
        <source>index</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8206"/>
        <source>note</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8260"/>
        <source>CSV exported to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8456"/>
        <source>Warning: your restore height is higher than wallet restore height: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8457"/>
        <source>Rescan anyway ? (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8476"/>
        <source>MMS received new message</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8893"/>
        <source>&lt;index&gt; is out of bounds</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9367"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9413"/>
        <source>command only supported by HW wallet</source>
        <translation>commande supportée uniquement par un portefeuille matériel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9372"/>
        <source>hw wallet does not support cold KI sync</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9384"/>
        <source>Please confirm the key image sync on the device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9390"/>
        <source>Key images synchronized to height </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9393"/>
        <source>Running untrusted daemon, cannot determine which transaction output is spent. Use a trusted daemon with --trusted-daemon and run rescan_spent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9396"/>
        <source> spent, </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9396"/>
        <source> unspent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9400"/>
        <source>Failed to import key images</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9405"/>
        <source>Failed to import key images: </source>
        <translation type="unfinished">Échec de l&apos;importation des images de clé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9422"/>
        <source>Failed to reconnect device</source>
        <translation>Échec de la reconnexion à l&apos;appareil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9427"/>
        <source>Failed to reconnect device: </source>
        <translation>Échec de la reconnexion à l&apos;appareil : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9700"/>
        <source>Transaction successfully saved to </source>
        <translation>Transaction sauvegardée avec succès dans </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9700"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9702"/>
        <source>, txid </source>
        <translation>, ID transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9702"/>
        <source>Failed to save transaction to </source>
        <translation>Échec de la sauvegarde de la transaction dans </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7277"/>
        <source>This is a watch only wallet</source>
        <translation>Ceci est un portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9630"/>
        <source>Double spend seen on the network: this transaction may or may not end up being mined</source>
        <translation>Double dépense détectée sur le réseau&#xa0;: cette transaction sera peut-être invalidée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9665"/>
        <source>Transaction ID not found</source>
        <translation>ID de transaction non trouvé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="356"/>
        <source>true</source>
        <translation>vrai</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="409"/>
        <source>failed to parse refresh type</source>
        <translation>échec de l&apos;analyse du type de rafraîchissement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="741"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="813"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="967"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1012"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1095"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1152"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1219"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1285"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1379"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1495"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1576"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7267"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7331"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7368"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7673"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7757"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9200"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9277"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9330"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9438"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9487"/>
        <source>command not supported by HW wallet</source>
        <translation>commande non supportée par le portefeuille matériel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="746"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="823"/>
        <source>wallet is watch-only and has no seed</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il n&apos;a pas de phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="764"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="833"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation>c&apos;est un portefeuille non déterministe et il n&apos;a pas de phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="771"/>
        <source>Enter optional seed offset passphrase, empty to see raw seed</source>
        <translation>Entrer une phrase de passe facultative pour le décalage de la phrase mnémonique, effacer pour voir la phrase mnémonique brute</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="843"/>
        <source>Incorrect password</source>
        <translation>Mot de passe invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="911"/>
        <source>Current fee is %s %s per %s</source>
        <translation>Les frais sont actuellement de %s %s par %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1660"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1817"/>
        <source>Invalid key image</source>
        <translation>Image de clé invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1666"/>
        <source>Invalid txid</source>
        <translation>ID de transaction invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1678"/>
        <source>Key image either not spent, or spent with mixin 0</source>
        <translation>Image de clé soit non dépensée, soit dépensée avec 0 mélange</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1693"/>
        <source>Failed to get key image ring: </source>
        <translation>Échec de la récupération du cercle de l&apos;image de clé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1708"/>
        <source>File doesn&apos;t exist</source>
        <translation>Le fichier d&apos;existe pas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1730"/>
        <source>Invalid ring specification: </source>
        <translation>Spécification de cercle invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1738"/>
        <source>Invalid key image: </source>
        <translation>Image de clé invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1743"/>
        <source>Invalid ring type, expected relative or abosolute: </source>
        <translation>Type de cercle invalide, &quot;relative&quot; ou &quot;absolute&quot; attendu : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1749"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1761"/>
        <source>Error reading line: </source>
        <translation>Erreur lors de la lecture de la ligne : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1772"/>
        <source>Invalid ring: </source>
        <translation>Cercle invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1781"/>
        <source>Invalid relative ring: </source>
        <translation>Cercle relatif invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1793"/>
        <source>Invalid absolute ring: </source>
        <translation>Cercle absolu invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1802"/>
        <source>Failed to set ring for key image: </source>
        <translation>Échec de l&apos;affectation du cercle pour l&apos;image de clé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1802"/>
        <source>Continuing.</source>
        <translation>On continue.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1832"/>
        <source>Missing absolute or relative keyword</source>
        <translation>Mot clé &quot;absolute&quot; ou &quot;relative&quot; manquant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1842"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1849"/>
        <source>invalid index: must be a strictly positive unsigned integer</source>
        <translation>index invalide : doit être un nombre entier strictement positif</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1857"/>
        <source>invalid index: indices wrap</source>
        <translation>index invalide : boucle des indices</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1867"/>
        <source>invalid index: indices should be in strictly ascending order</source>
        <translation>index invalide : les indices doivent être en ordre strictement croissant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1874"/>
        <source>failed to set ring</source>
        <translation>échec de l&apos;affectation du cercle</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1951"/>
        <source>First line is not an amount</source>
        <translation>La première ligne n&apos;est pas un montant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1965"/>
        <source>Invalid output: </source>
        <translation>Sortie invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1975"/>
        <source>Bad argument: </source>
        <translation>Mauvais argument : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1975"/>
        <source>should be &quot;add&quot;</source>
        <translation>devrait être &quot;add&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1984"/>
        <source>Failed to open file</source>
        <translation>Échec de l&apos;ouverture du fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1990"/>
        <source>Invalid output key, and file doesn&apos;t exist</source>
        <translation>Clé de sortie invalide, et le fichier n&apos;existe pas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2013"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2040"/>
        <source>Invalid output</source>
        <translation>Sortie invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2068"/>
        <source>Failed to save known rings: </source>
        <translation>Échec de la sauvegarde des cercles connus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2183"/>
        <source>Unknown command &apos;%s&apos;, try &apos;help&apos;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2249"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2268"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas transférer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2286"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6236"/>
        <source>WARNING: this is a non default ring size, which may harm your privacy. Default is recommended.</source>
        <translation>ATTENTION : ceci c&apos;est pas la taille de cercle par défaut, ce qui peut nuire à votre confidentialité. La valeur par défaut est recommandée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2288"/>
        <source>WARNING: from v8, ring size will be fixed and this setting will be ignored.</source>
        <translation>ATTENTION : ) partir de v8, la taille de cercle sera fixée et ce paramètre sera ignoré.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2317"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2340"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2356"/>
        <source>priority must be either 0, 1, 2, 3, or 4, or one of: </source>
        <translation>la priorité doit être 0, 1, 2, 3, 4 ou l&apos;une de : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2361"/>
        <source>could not change default priority</source>
        <translation>échec du changement de la priorité par défaut</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2431"/>
        <source>invalid argument: must be either 0/never, 1/action, or 2/encrypt/decrypt</source>
        <translation>argument invalide : doit être soit 0/never, 1/action, ou 2/encrypt/decrypt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2689"/>
        <source>Inactivity lock timeout disabled on Windows</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2703"/>
        <source>Invalid number of seconds</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2765"/>
        <source>Export format not specified</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2779"/>
        <source>Export format not recognized.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2857"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt;. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Transférer &lt;montant&gt; à &lt;adresse&gt; Si le paramètre &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille utilise les sorties reçues par les adresses de ces indices. Si il est omis, le portefeuille choisit les indices d&apos;adresse à utiliser aléatoirement. Dans tous les cas, il essaye de ne pas combiner des sorties de multiples adresses. &lt;priorité&gt; est la priorité de la transaction. Plus la priorité est haute, plus les frais de transaction sont élevés. Les valeurs valides par ordre de priorité (de la plus basse à la plus haute) sont&#xa0;: unimportant, normal, elevated, priority. Si elle est omise, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;taille_cercle&gt; est le nombre d&apos;entrées à inclure pour l&apos;intraçabilité. De multiples paiements peuvent être réalisés d&apos;un coup en ajoutant &lt;URI_2&gt; ou &lt;adresse_2&gt; &lt;montant_2&gt; et cetera (avant l&apos;ID de paiement, si il est inclus)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2861"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Transférer &lt;montant&gt; à &lt;adresse&gt; et le verrouiller pendant &lt;blocs_verrou&gt; (max 1000000). Si le paramètre &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille utilise les sorties reçues par les adresses de ces indices. Si il est omis, le portefeuille choisit les indices d&apos;adresse à utiliser aléatoirement. Dans tous les cas, il essaye de ne pas combiner des sorties de multiples adresses. &lt;priorité&gt; est la priorité de la transaction. Plus la priorité est haute, plus les frais de transaction sont élevés. Les valeurs valides par ordre de priorité (de la plus basse à la plus haute) sont&#xa0;: unimportant, normal, elevated, priority. Si elle est omise, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;taille_cercle&gt; est le nombre d&apos;entrées à inclure pour l&apos;intraçabilité. De multiples paiements peuvent être réalisés d&apos;un coup en ajoutant &lt;URI_2&gt; ou &lt;adresse_2&gt; &lt;montant_2&gt; et cetera (avant l&apos;ID de paiement, si il est inclus)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2865"/>
        <source>Send all unlocked balance to an address and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. &lt;priority&gt; is the priority of the sweep. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability.</source>
        <translation>Transférer tout le solde débloqué à une adresse et le verrouiller pendant &lt;blocs_verrou&gt; (max 1000000). Si le paramètre &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille utilise les sorties reçues par ces indices d&apos;adresse. Si il est omis, le portefeuille choisit un index d&apos;adresse à utiliser aléatoirement. &lt;priorité&gt; est la priorité du balayage. Plus la priorité est haute, plus les frais de transaction sont élevés. Les valeurs valides par ordre de priorité (de la plus basse à la plus haute) sont : unimportant, normal, elevated, priority. Si elle est omise, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;taille_cercle&gt; est le nombre d&apos;entrées à inclure pour l&apos;intraçabilité.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2871"/>
        <source>Send all unlocked balance to an address. If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. If the parameter &quot;outputs=&lt;N&gt;&quot; is specified and  N &gt; 0, wallet splits the transaction into N even outputs.</source>
        <translation>Envoyer tout le solde débloqué à une adresse. Si le paramètre &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille balaye les sorties reçues par ces indices d&apos;adresse. Si il est omis, le portefeuille choisit un index d&apos;adresse à utiliser aléatoirement. Si le paramètre &quot;outputs=&lt;N&gt;&quot; est spécifié et N &gt; 0, le portefeuille scinde la transaction en N sorties égales.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2887"/>
        <source>Sign a transaction from a file. If the parameter &quot;export_raw&quot; is specified, transaction raw hex data suitable for the daemon RPC /sendrawtransaction is exported.</source>
        <translation>Signer une transaction à partir d&apos;un fichier. Si le paramètre &quot;export_raw&quot; est spécifié, les données brutes hexadécimales adaptées au RPC /sendrawtransaction du démon sont exportées.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2908"/>
        <source>If no arguments are specified or &lt;index&gt; is specified, the wallet shows the default or specified address. If &quot;all&quot; is specified, the wallet shows all the existing addresses in the currently selected account. If &quot;new &quot; is specified, the wallet creates a new address with the provided label text (which can be empty). If &quot;label&quot; is specified, the wallet sets the label of the address specified by &lt;index&gt; to the provided label text.</source>
        <translation>Si aucun argument n&apos;est spécifié ou si &lt;index&gt; est spécifié, le portefeuille affiche l&apos;adresse par défaut ou l&apos;adresse spécifiée. Si &quot;all&quot; est spécifié, le portefeuille affiche toutes les adresses existantes dans le comptes actuellement sélectionné. Si &quot;new&quot; est spécifié, le portefeuille crée une nouvelle adresse avec le texte d&apos;étiquette fourni (qui peut être vide). Si &quot;label&quot; est spécifié, le portefeuille affecte le texte fourni à l&apos;étiquette de l&apos;adresse spécifiée par &lt;index&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2934"/>
        <source>Display the restore height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2938"/>
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
   Set this if you are not sure whether you will spend on a key reusing Monero fork later.
 segregation-height &lt;n&gt;
   Set to the height of a key reusing fork you want to use, 0 to use default.
 ignore-fractional-outputs &lt;1|0&gt;
   Whether to ignore fractional outputs that result in net loss when spending due to fee.
 track-uses &lt;1|0&gt;
   Whether to keep track of owned outputs uses.
 setup-background-mining &lt;1|0&gt;
   Whether to enable background mining. Set this to support the network and to get a chance to receive new monero.
 device-name &lt;device_name[:device_spec]&gt;
   Device name for hardware wallet.
 export-format &lt;&quot;binary&quot;|&quot;ascii&quot;&gt;
   Save all exported files as binary (cannot be copied and pasted) or ascii (can be).
 </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3009"/>
        <source>Set the transaction key (r) for a given &lt;txid&gt; in case the tx was made by some other device or 3rd party wallet.</source>
        <translation>Définir la clé de transaction (r) pour un &lt;ID_transaction&gt; donné au cas où cette clé ait été créée par un appareil ou portefeuille tiers.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3109"/>
        <source>Attempts to reconnect HW wallet.</source>
        <translation>Essayer de se reconnecter à un portefeuille matériel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3251"/>
        <source>Print the ring(s) used to spend a given key image or transaction (if the ring size is &gt; 1)

Output format:
Key Image, &quot;absolute&quot;, list of rings</source>
        <translation>Afficher le(s) cercle(s) utilisé(s) pour dépenser une image de clé ou une transaction (si la taille de cercle est &gt; 1)

Format de sortie :
Image de clé, &quot;absolue&quot;, liste de cercles</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3257"/>
        <source>Set the ring used for a given key image, so it can be reused in a fork</source>
        <translation>Définir le cercle utilisé pour une image de clé donnée, afin de pouvoir le réutiliser dans un fork</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3265"/>
        <source>Save known rings to the shared rings database</source>
        <translation>Sauvegarder les cercles connus dans la base de données des cercles partagés</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3293"/>
        <source>Lock the wallet console, requiring the wallet password to continue</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3305"/>
        <source>Returns version information</source>
        <translation>Retourne les informations de version</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3411"/>
        <source>full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)</source>
        <translation>full (le plus lent, aucune supposition); optimize-coinbase (rapide, suppose que la récompense de bloc est payée à une unique adresse); no-coinbase (le plus rapide, suppose que l&apos;on ne reçoit aucune récompense de bloc), default (comme optimize-coinbase)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3412"/>
        <source>0, 1, 2, 3, or 4, or one of </source>
        <translation>0, 1, 2, 3, 4 ou l&apos;une de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3414"/>
        <source>0|1|2 (or never|action|decrypt)</source>
        <translation>0|1|2 (ou never|action|decrypt)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3415"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation>monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3426"/>
        <source>&lt;major&gt;:&lt;minor&gt;</source>
        <translation>&lt;majeur&gt;:&lt;mineur&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3430"/>
        <source>unsigned integer (seconds, 0 to disable)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3433"/>
        <source>&quot;binary&quot; or &quot;ascii&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3493"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation>Nom de portefeuille non valide. Veuillez réessayer ou utilisez Ctrl-C pour quitter.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3510"/>
        <source>Wallet and key files found, loading...</source>
        <translation>Fichier portefeuille et fichier de clés trouvés, chargement...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3516"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation>Fichier de clés trouvé mais pas le fichier portefeuille. Régénération...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3522"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation>Fichier de clés non trouvé. Échec de l&apos;ouverture du portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3541"/>
        <source>Generating new wallet...</source>
        <translation>Génération du nouveau portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3622"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation>Impossible de spécifier plus d&apos;une option parmis --testnet et --stagenet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3637"/>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-device=&quot;wallet_name&quot;</source>
        <translation>impossible de spécifier plus d&apos;une option parmis --generate-new-wallet=&quot;nom_portefeuille&quot;, --wallet-file=&quot;nom_portefeuille&quot;, --generate-from-view-key=&quot;nom_portefeuille&quot;, --generate-from-spend-key=&quot;nom_portefeuille&quot;, --generate-from-keys=&quot;nom_portefeuille&quot;, --generate-from-multisig-keys=&quot;nom_portefeuille&quot;, --generate-from-json=&quot;nom_fichier_json&quot; et --generate-from-device=&quot;nom_portefeuille&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3716"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3721"/>
        <source>Enter seed offset passphrase, empty if none</source>
        <translation>Entrer une phrase de passe pour le décalage de la phrase mnémonique, vide si aucun</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3746"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3766"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3802"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3823"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3843"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3858"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3907"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3932"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3948"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3987"/>
        <source>No data supplied, cancelled</source>
        <translation>Pas de données fournies, annulation</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3752"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3829"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3938"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6016"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6615"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6892"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7481"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7549"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7613"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7817"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8994"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9257"/>
        <source>failed to parse address</source>
        <translation>échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3772"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3864"/>
        <source>failed to parse view key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3781"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3881"/>
        <source>failed to verify view key secret key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3785"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3885"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3967"/>
        <source>view key does not match standard address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3790"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3811"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3889"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4023"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4050"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4082"/>
        <source>account creation failed</source>
        <translation>échec de la création du compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3807"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3849"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3992"/>
        <source>failed to parse spend key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3873"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4012"/>
        <source>failed to verify spend key secret key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3877"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4017"/>
        <source>spend key does not match standard address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4057"/>
        <source>No restore height is specified.</source>
        <translation>Aucune hauteur de restauration n&apos;est spécifiée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4058"/>
        <source>Assumed you are creating a new account, restore will be done from current estimated blockchain height.</source>
        <translation>Nous supposons que vous créez un nouveau compte, la restauration sera faite à partir d&apos;une hauteur de la chaîne de blocs estimée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4063"/>
        <source>account creation aborted</source>
        <translation>création du compte annulée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4185"/>
        <source>can&apos;t specify --subaddress-lookahead and --wallet-file at the same time</source>
        <translation>Impossible de spécifier --subaddress-lookahead et --wallet-file en même temps</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4189"/>
        <source>failed to open account</source>
        <translation>échec de l&apos;ouverture du compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4194"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4921"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4974"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5059"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7517"/>
        <source>wallet is null</source>
        <translation>portefeuille est nul</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4200"/>
        <source>Warning: using an untrusted daemon at %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4201"/>
        <source>Using a third party daemon can be detrimental to your security and privacy</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4204"/>
        <source>Using your own without SSL exposes your RPC traffic to monitoring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4205"/>
        <source>You are strongly encouraged to connect to the Monero network using your own daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4206"/>
        <source>If you or someone you trust are operating this daemon, you can use --trusted-daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4213"/>
        <source>Moreover, a daemon is also less secure when running in bootstrap mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4217"/>
        <source>Failed to initialize ring database: privacy enhancing features will be inactive</source>
        <translation>Impossible d&apos;initialiser la base de données des cercles : les fonctions d&apos;amélioration de la confidentialité seront inactives</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4328"/>
        <source>If your display freezes, exit blind with ^C, then run again with --use-english-language-names</source>
        <translation>Si votre affichage se bloque, quittez en aveugle avec ^C, puis lancer à nouveau en utilisant --use-english-language-names</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4346"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4351"/>
        <source>invalid language choice entered. Please try again.
</source>
        <translation>choix de langue passé invalide. Veuillez réessayer.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4432"/>
        <source>View key: </source>
        <translation>Clé d&apos;audit&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4547"/>
        <source>Generated new wallet on hw device: </source>
        <translation>Nouveau portefeuille généré sur l&apos;appareil : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4628"/>
        <source>Key file not found. Failed to open wallet</source>
        <translation>Fichier des clés non trouvé. Échec d&apos;ouverture du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4705"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation>Vous pourriez vouloir supprimer le fichier &quot;%s&quot; et réessayer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4733"/>
        <source>failed to deinitialize wallet</source>
        <translation>échec de la désinitialisation du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4786"/>
        <source>Watch only wallet saved as: </source>
        <translation>Portefeuille d&apos;audit sauvegardé vers : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4790"/>
        <source>Failed to save watch only wallet: </source>
        <translation>Échec de la sauvegarde du portefeuille d&apos;audit&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4912"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5580"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9335"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation>cette commande requiert un démon de confiance. Activer avec --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5028"/>
        <source>Expected trusted or untrusted, got </source>
        <translation>&quot;trusted&quot; ou &quot;untrusted&quot; attendu, mais lu </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5045"/>
        <source>trusted</source>
        <translation>de confiance</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5045"/>
        <source>untrusted</source>
        <translation>non fiable</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5069"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation>la chaîne de blocs ne peut pas être sauvegardée&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5148"/>
        <source>Password needed (%s) - use the refresh command</source>
        <translation>Mot de passe requis (%s) - utilisez la commande refresh</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5154"/>
        <source>Enter password</source>
        <translation>Entrez le mot de passe</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5271"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5594"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5275"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5598"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5285"/>
        <source>refresh error: </source>
        <translation>erreur du rafraîchissement&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5333"/>
        <source> (Some owned outputs have missing key images - import_key_images needed)</source>
        <translation> (Il manque les images de clé de certaines sorties - import_key_images requis)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5342"/>
        <source>Balance: </source>
        <translation>Solde&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5449"/>
        <source>pubkey</source>
        <translation>clé publique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5449"/>
        <source>key image</source>
        <translation>image de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5450"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5466"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8206"/>
        <source>unlocked</source>
        <translation>déverrouillé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5450"/>
        <source>ringct</source>
        <translation>ringct</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5465"/>
        <source>T</source>
        <translation>V</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5465"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5466"/>
        <source>locked</source>
        <translation>vérrouillé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5467"/>
        <source>RingCT</source>
        <translation>RingCT</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5467"/>
        <source>-</source>
        <translation>-</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5546"/>
        <source>payment ID has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5602"/>
        <source>failed to get spent status</source>
        <translation>échec de la récupération du statut de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5686"/>
        <source>failed to find construction data for tx input</source>
        <translation>échec de la recherche des données pour contruire l&apos;entrée de la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5691"/>
        <source>
Input %llu/%llu (%s): amount=%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5749"/>
        <source>the same transaction</source>
        <translation>la même transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5749"/>
        <source>blocks that are temporally very close</source>
        <translation>blocs très proches dans le temps</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5805"/>
        <source>I locked your Monero wallet to protect you while you were away</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5825"/>
        <source>Locked due to inactivity. The wallet password is required to unlock the console.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5905"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6526"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6825"/>
        <source>ring size %u is too large, maximum is %u</source>
        <translation>la taille de cercle %u est trop grande, le maximum est %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5936"/>
        <source>payment id failed to encode</source>
        <translation>échec de l&apos;encodage de l&apos;ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5983"/>
        <source>failed to parse short payment ID from URI</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement court à partir de l&apos;URI</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6008"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6010"/>
        <source>Invalid last argument: </source>
        <translation>Dernier argument invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6028"/>
        <source>a single transaction cannot use more than one payment id</source>
        <translation>une unique transaction ne peut pas utiliser plus d&apos;un ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6046"/>
        <source>failed to parse payment id, though it was detected</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement, bien qu&apos;il ait été détecté</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6431"/>
        <source>Not enough money in unlocked balance</source>
        <translation>Pas assez de fonds dans le solde débloqué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6535"/>
        <source>missing lockedblocks parameter</source>
        <translation>paramètre blocs_verrou manquant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6545"/>
        <source>bad locked_blocks parameter</source>
        <translation>mauvais paramètre blocs_verrou</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6570"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6834"/>
        <source>Failed to parse number of outputs</source>
        <translation>Échec de l&apos;analyse du nombre de sorties</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6575"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6839"/>
        <source>Amount of outputs should be greater than 0</source>
        <translation>Le nombre de sorties doit être supérieur à 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7093"/>
        <source>Donating %s %s to The Monero Project (donate.getmonero.org or %s).</source>
        <translation>Don de %s %s à The Monero Project (donate.getmonero.org ou %s).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7427"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7438"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7445"/>
        <source>failed to parse tx_key</source>
        <translation>échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7454"/>
        <source>Tx key successfully stored.</source>
        <translation>Clé de transaction sauvegardée avec succès.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7458"/>
        <source>Failed to store tx key: </source>
        <translation>Échec de la sauvegarde de la clé de transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7632"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7742"/>
        <source>Good signature</source>
        <translation>Bonne signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7659"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7744"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7844"/>
        <source>Bad signature</source>
        <translation>Mauvaise signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8128"/>
        <source>usage: show_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation>usage : show_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;hauteur_min&gt; [&lt;hauteur_max&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7961"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8206"/>
        <source>block</source>
        <translation>bloc</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8443"/>
        <source>Warning: this will lose any information which can not be recovered from the blockchain.</source>
        <translation>Attention : ceci pedra toute information qui ne peut pas être retrouvée à partir de la chaîne de blocs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8444"/>
        <source>This includes destination addresses, tx secret keys, tx notes, etc</source>
        <translation>Ceci inclut les adresses de destination, les clé secrètes de transaction, les notes de transaction, etc</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8970"/>
        <source>Standard address: </source>
        <translation>Adresse standard&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8975"/>
        <source>failed to parse payment ID or address</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement ou de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9017"/>
        <source>failed to parse payment ID</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9035"/>
        <source>failed to parse index</source>
        <translation>échec de l&apos;analyse de l&apos;index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9043"/>
        <source>Address book is empty.</source>
        <translation>Le carnet d&apos;adresses est vide.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9049"/>
        <source>Index: </source>
        <translation>Index&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9050"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9181"/>
        <source>Address: </source>
        <translation>Adresse&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9051"/>
        <source>Payment ID: </source>
        <translation>ID de paiement&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9052"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9180"/>
        <source>Description: </source>
        <translation>Description&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9190"/>
        <source>Network type: </source>
        <translation>Type de réseau : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9191"/>
        <source>Testnet</source>
        <translation>Testnet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9192"/>
        <source>Stagenet</source>
        <translation>Stagenet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9192"/>
        <source>Mainnet</source>
        <translation>Mainnet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9210"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas signer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1318"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9224"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9250"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9501"/>
        <source>failed to read file </source>
        <translation>échec de la lecture du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5955"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6550"/>
        <source>Locked blocks too high, max 1000000 (˜4 yrs)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6064"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6642"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6917"/>
        <source>No payment id is included with this transaction. Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6128"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6138"/>
        <source>Is this okay anyway?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6133"/>
        <source>There is currently a %u block backlog at that fee level. Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7621"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7735"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7829"/>
        <source>failed to load signature file</source>
        <translation>échec du chargement du fichier signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7683"/>
        <source>wallet is watch-only and cannot generate the proof</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut générer de preuve</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7767"/>
        <source>The reserve proof can be generated only by a full wallet</source>
        <translation>La preuve de réserve ne peut être généré que par un portefeuille complet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7822"/>
        <source>Address must not be a subaddress</source>
        <translation>L&apos;adresse ne doit pas être une sous-adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7840"/>
        <source>Good signature -- total: %s, spent: %s, unspent: %s</source>
        <translation>Bonne signature -- total&#xa0;: %s, dépensé&#xa0;: %s, non dépensé&#xa0;: %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8051"/>
        <source>[Double spend seen on the network: this transaction may or may not end up being mined] </source>
        <translation>[Double dépense détectée sur le réseau&#xa0;: cette transaction sera peut-être invalidée] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8329"/>
        <source>There is no unspent output in the specified address</source>
        <translation>Il n&apos;y a pas de sortie non dépensée pour l&apos;adresse spécifiée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8557"/>
        <source> (no daemon)</source>
        <translation> (pas de démon)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8559"/>
        <source> (out of sync)</source>
        <translation> (désynchronisé)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8610"/>
        <source>(Untitled account)</source>
        <translation>(compte sans nom)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8623"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8641"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8666"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8689"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8837"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8860"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8888"/>
        <source>failed to parse index: </source>
        <translation>échec de l&apos;analyse de l&apos;index&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8628"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8842"/>
        <source>specify an index between 0 and </source>
        <translation>specifiez un index entre 0 et </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8746"/>
        <source>
Grand total:
  Balance: </source>
        <translation>
Somme finale&#xa0;:
  Solde&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8746"/>
        <source>, unlocked balance: </source>
        <translation>, solde débloqué&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8754"/>
        <source>Untagged accounts:</source>
        <translation>Comptes sans mot clé&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8760"/>
        <source>Tag %s is unregistered.</source>
        <translation>Le mot clé %s n&apos;est pas enregistré.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8763"/>
        <source>Accounts with tag: </source>
        <translation>Comptes avec mot clé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8764"/>
        <source>Tag&apos;s description: </source>
        <translation>Description du mot clé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8766"/>
        <source>Account</source>
        <translation>Compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8772"/>
        <source> %c%8u %6s %21s %21s %21s</source>
        <translation> %c%8u %6s %21s %21s %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8782"/>
        <source>----------------------------------------------------------------------------------</source>
        <translation>----------------------------------------------------------------------------------</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8783"/>
        <source>%15s %21s %21s</source>
        <translation>%15s %21s %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8807"/>
        <source>Primary address</source>
        <translation>Adresse primaire</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8807"/>
        <source>(used)</source>
        <translation>(utilisé)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8828"/>
        <source>(Untitled address)</source>
        <translation>(adresse sans nom)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8869"/>
        <source>&lt;index_min&gt; is already out of bound</source>
        <translation>&lt;index_min&gt; est déjà hors limite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8874"/>
        <source>&lt;index_max&gt; exceeds the bound</source>
        <translation>&lt;index_max&gt; excède la limite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8938"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8951"/>
        <source>Integrated addresses can only be created for account 0</source>
        <translation>Les adresses intégrées ne peuvent être créées que pour le compte 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8964"/>
        <source>Integrated address: %s, payment ID: %s</source>
        <translation>Adresse intégrée&#xa0;: %s, ID de paiement&#xa0;: %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8970"/>
        <source>Subaddress: </source>
        <translation>Sous-adresse&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9137"/>
        <source>no description found</source>
        <translation>pas de description trouvée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9139"/>
        <source>description found: </source>
        <translation>description trouvée&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9179"/>
        <source>Filename: </source>
        <translation>Fichier&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9184"/>
        <source>Watch only</source>
        <translation>Audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9186"/>
        <source>%u/%u multisig%s</source>
        <translation>Multisig %u/%u%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9188"/>
        <source>Normal</source>
        <translation>Normal</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9189"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10000"/>
        <source>Type: </source>
        <translation>Type&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9215"/>
        <source>This wallet is multisig and cannot sign</source>
        <translation>C&apos;est un portefeuille multisig et il ne peut pas signer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9264"/>
        <source>Bad signature from </source>
        <translation>Mauvaise signature de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9268"/>
        <source>Good signature from </source>
        <translation>Bonne signature de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9284"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas exporter les images de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1257"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9311"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9468"/>
        <source>failed to save file </source>
        <translation>échec de l&apos;enregistrement du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9322"/>
        <source>Signed key images exported to </source>
        <translation>Images de clé signées exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9479"/>
        <source>Outputs exported to </source>
        <translation>Sorties exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5998"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7068"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7778"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8288"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8296"/>
        <source>amount is wrong: </source>
        <translation>montant erroné&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5999"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7068"/>
        <source>expected number from 0 to </source>
        <translation>attend un nombre de 0 à </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6377"/>
        <source>Sweeping </source>
        <translation>Balayage de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6999"/>
        <source>Money successfully sent, transaction: </source>
        <translation>Fonds envoyés avec succès, transaction&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7196"/>
        <source>Change goes to more than one address</source>
        <translation>La monnaie rendue va à plus d&apos;une adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7237"/>
        <source>%s change to %s</source>
        <translation>%s de monnaie rendue à %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7240"/>
        <source>no change</source>
        <translation>sans monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1464"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1477"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7312"/>
        <source>Transaction successfully signed to file </source>
        <translation>Transaction signée avec succès dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7379"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7417"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7474"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7523"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7605"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7690"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7725"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9069"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9097"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9531"/>
        <source>failed to parse txid</source>
        <translation>échec de l&apos;analyse de l&apos;ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7395"/>
        <source>Tx key: </source>
        <translation>Clé de transaction&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7400"/>
        <source>no tx keys found for this txid</source>
        <translation>aucune clé de transaction trouvée pour cet ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7492"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7704"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7793"/>
        <source>signature file saved to: </source>
        <translation>fichier signature sauvegardé dans&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7494"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7706"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7795"/>
        <source>failed to save signature file</source>
        <translation>échec de la sauvegarde du fichier signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7531"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7540"/>
        <source>failed to parse tx key</source>
        <translation>échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7498"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7586"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7664"/>
        <source>error: </source>
        <translation>erreur&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7562"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7635"/>
        <source>received</source>
        <translation>a reçu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7562"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7635"/>
        <source>in txid</source>
        <translation>dans la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7581"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7654"/>
        <source>received nothing in txid</source>
        <translation>n&apos;a rien reçu dans la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7565"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7638"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation>ATTENTION&#xa0;: cette transaction n&apos;est pas encore inclue dans la chaîne de blocs !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7571"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7644"/>
        <source>This transaction has %u confirmations</source>
        <translation>Cette transaction a %u confirmations</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7575"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7648"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation>ATTENTION&#xa0;: échec de la détermination du nombre de confirmations !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7929"/>
        <source>bad min_height parameter:</source>
        <translation>mauvais paramètre hauteur_minimum&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7941"/>
        <source>bad max_height parameter:</source>
        <translation>mauvais paramètre hauteur_maximum&#xa0;:</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7961"/>
        <source>in</source>
        <translation>reçu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8303"/>
        <source>&lt;min_amount&gt; should be smaller than &lt;max_amount&gt;</source>
        <translation>&lt;montant_minimum&gt; doit être inférieur à &lt;montant_maximum&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8335"/>
        <source>
Amount: </source>
        <translation>
Montant&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8335"/>
        <source>, number of keys: </source>
        <translation>, nombre de clés&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8340"/>
        <source> </source>
        <translation> </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8345"/>
        <source>
Min block height: </source>
        <translation>
Hauteur de bloc minimum&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8346"/>
        <source>
Max block height: </source>
        <translation>
Hauteur de bloc maximum&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8347"/>
        <source>
Min amount found: </source>
        <translation>
Montant minimum trouvé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8348"/>
        <source>
Max amount found: </source>
        <translation>
Montant maximum trouvé&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8349"/>
        <source>
Total count: </source>
        <translation>
Compte total&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8389"/>
        <source>
Bin size: </source>
        <translation>
Taille de classe&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8390"/>
        <source>
Outputs per *: </source>
        <translation>
Sorties par *&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8392"/>
        <source>count
  ^
</source>
        <translation>compte
  ^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8394"/>
        <source>  |</source>
        <translation>  |</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8396"/>
        <source>  +</source>
        <translation>  +</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8396"/>
        <source>+--&gt; block height
</source>
        <translation>+--&gt; hauteur de bloc
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8397"/>
        <source>   ^</source>
        <translation>   ^</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8397"/>
        <source>^
</source>
        <translation>^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8398"/>
        <source>  </source>
        <translation>  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8555"/>
        <source>wallet</source>
        <translation>portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="898"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8942"/>
        <source>Random payment ID: </source>
        <translation>ID de paiement aléatoire&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8943"/>
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
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="153"/>
        <source>Generated multisig wallets for address </source>
        <translation>Portefeuilles multisig générés pour l&apos;adresse </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="157"/>
        <source>Error creating multisig wallets: </source>
        <translation>Erreur de création des portefeuilles multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="182"/>
        <source>This program generates a set of multisig wallets - use this simpler scheme only if all the participants trust each other</source>
        <translation>Ce programme génère un ensemble de portefeuilles multisig - n&apos;utilisez cette méthode plus simple que si tous les participants se font confiance</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="201"/>
        <source>Error: Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation>Erreur: Impossible de spécifier plus d&apos;une option parmis --testnet et --stagenet</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="208"/>
        <source>Error: expected N/M, but got: </source>
        <translation>Erreur&#xa0;: N/M attendu, mais lu&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="216"/>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="225"/>
        <source>Error: either --scheme or both of --threshold and --participants may be given</source>
        <translation>Erreur&#xa0;: soit --scheme soit --threshold et --participants doivent être indiqués</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="232"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got N==%u and M==%d</source>
        <translation>Erreur&#xa0;: N &gt; 1 et N &lt;= M attendu, mais lu N==%u et M==%d</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="241"/>
        <source>Error: --filename-base is required</source>
        <translation>Erreur&#xa0;: --filename-base est requis</translation>
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
        <translation type="unfinished"></translation>
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
        <translation type="unfinished">reçu</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1378"/>
        <source>out</source>
        <translation type="unfinished">payé</translation>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="139"/>
        <source>Generate new wallet and save it to &lt;arg&gt;</source>
        <translation>Générer un nouveau portefeuille et le sauvegarder dans &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="140"/>
        <source>Generate new wallet from device and save it to &lt;arg&gt;</source>
        <translation>Générer un nouveau portefeuille à partir de l&apos;appareil et le sauvegarder dans &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="141"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>Générer un portefeuille d&apos;audit à partir d&apos;une clé d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="142"/>
        <source>Generate deterministic wallet from spend key</source>
        <translation>Générer un portefeuille déterministe à partir d&apos;une clé de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="143"/>
        <source>Generate wallet from private keys</source>
        <translation>Générer un portefeuille à partir de clés privées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="144"/>
        <source>Generate a master wallet from multisig wallet keys</source>
        <translation>Générer un portefeuille principal à partir de clés de portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="146"/>
        <source>Language for mnemonic</source>
        <translation>Langue de la phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="147"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>Spécifier la phrase mnémonique Electrum pour la récupération/création d&apos;un portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="148"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Récupérer un portefeuille en utilisant une phrase mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="149"/>
        <source>Recover multisig wallet using Electrum-style mnemonic seed</source>
        <translation>Récupérer un portefeuille multisig en utilisant une phrase mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="150"/>
        <source>Generate non-deterministic view and spend keys</source>
        <translation>Générer des clés d&apos;audit et de dépense non déterministes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="153"/>
        <source>Restore from estimated blockchain height on specified date</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="381"/>
        <source>invalid argument: must be either 0/1, true/false, y/n, yes/no</source>
        <translation>argument invalide : doit être soit 0/1, true/false, y/n, yes/no</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="437"/>
        <source>DNSSEC validation passed</source>
        <translation>Validation DNSSEC réussie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="441"/>
        <source>WARNING: DNSSEC validation was unsuccessful, this address may not be correct!</source>
        <translation>ATTENTION: la validation DNSSEC a échoué, cette adresse n&apos;est peut être pas correcte !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="444"/>
        <source>For URL: </source>
        <translation>Pour l&apos;URL : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="446"/>
        <source> Monero Address = </source>
        <translation> Adresse Monero = </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="458"/>
        <source>you have cancelled the transfer request</source>
        <translation>vous avez annulé la demande de transfert</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="479"/>
        <source>failed to parse index: </source>
        <translation>échec de l&apos;analyse de l&apos;index&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="492"/>
        <source>invalid format for subaddress lookahead; must be &lt;major&gt;:&lt;minor&gt;</source>
        <translation>format invalide pour l&apos;anticipation des sous-addresses; doit être &lt;majeur&gt;:&lt;mineur&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="509"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="514"/>
        <source>RPC error: </source>
        <translation>Erreur RPC&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="518"/>
        <source>failed to get random outputs to mix: </source>
        <translation>échec de la récupération de sorties aléatoires à mélanger&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="525"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="533"/>
        <source>Not enough money in unlocked balance</source>
        <translation>Pas assez de fonds dans le solde débloqué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="543"/>
        <source>Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees</source>
        <translation>Impossible de trouver une façon de créer les transactions. Ceci est souvent dû à de la poussière si petite qu&apos;elle ne peut pas payer ses propres frais, ou à une tentative d&apos;envoi d&apos;un montant supérieur au solde débloqué, ou à un montant restant insuffisant pour payer les frais</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="549"/>
        <source>not enough outputs for specified ring size</source>
        <translation>pas assez de sorties pour la taille de cercle spécifiée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="552"/>
        <source>output amount</source>
        <translation>montant de la sortie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="552"/>
        <source>found outputs to use</source>
        <translation>sorties à utiliser trouvées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="554"/>
        <source>Please use sweep_unmixable.</source>
        <translation>Veuillez utiliser sweep_unmixable.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="558"/>
        <source>transaction was not constructed</source>
        <translation>la transaction n&apos;a pas été construite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="566"/>
        <source>Reason: </source>
        <translation>Raison : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="575"/>
        <source>one of destinations is zero</source>
        <translation>une des destinations est zéro</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="580"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>échec de la recherche d&apos;une façon adéquate de scinder les transactions</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="586"/>
        <source>unknown transfer error: </source>
        <translation>erreur de transfert inconnue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="591"/>
        <source>Multisig error: </source>
        <translation>Erreur multisig&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="597"/>
        <source>internal error: </source>
        <translation>erreur interne&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="602"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="606"/>
        <source>There was an error, which could mean the node may be trying to get you to retry creating a transaction, and zero in on which outputs you own. Or it could be a bona fide error. It may be prudent to disconnect from this node, and not try to send a transaction immediately. Alternatively, connect to another node so the original node cannot correlate information.</source>
        <translation>Il y a eu une erreur, ce qui pourrait signifier que le noeud essaye de vous faire réessayer de créer une transaction, pour tenter d&apos;identifier quelles sorties sont les votres. Ou il pourrait s&apos;agir d&apos;une erreur de bonne foi. Il pourrait être prudent de se déconnecter de ce noeud, et de ne pas essayer d&apos;envoyer une transaction immédiatement. Ou sinon, se connecter à un autre noeud pour que le noeud original ne puisse pas corréler les informations.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="616"/>
        <source>File %s likely stores wallet private keys! Use a different file name.</source>
        <translation>Le fichier %s contient probablement des clés privées de portefeuille ! Utilisez un nom de fichier différent.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7858"/>
        <source> seconds</source>
        <translation> secondes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7860"/>
        <source> minutes</source>
        <translation> minutes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7862"/>
        <source> hours</source>
        <translation> heures</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7864"/>
        <source> days</source>
        <translation> jours</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7866"/>
        <source> months</source>
        <translation> mois</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7867"/>
        <source>a long time</source>
        <translation>longtemps</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9760"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.
WARNING: Do not reuse your Monero keys on another fork, UNLESS this fork has key reuse mitigations built in. Doing so will harm your privacy.</source>
        <translation>Ceci est le portefeuille monero en ligne de commande.
Il a besoin de se connecter à un démon monero pour fonctionner correctement.
ATTENTION : Ne réutilisez pas vos clés Monero avec un autre fork, À MOINS QUE ce fork inclue des mitigations contre la réutilisation des clés. Faire ceci nuira à votre confidentialité.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9785"/>
        <source>Unknown command: </source>
        <translation>Commande inconnue&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="151"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>Autoriser la communication avec un démon utilisant une version de RPC différente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="152"/>
        <source>Restore from specific blockchain height</source>
        <translation>Restaurer à partir d&apos;une hauteur de bloc spécifique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="154"/>
        <source>The newly created transaction will not be relayed to the monero network</source>
        <translation>La transaction nouvellement créée ne sera pas transmise au réseau monero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="155"/>
        <source>Create an address file for new wallets</source>
        <translation>Créer un fichier d&apos;adresse pour les nouveaux portefeuilles</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="157"/>
        <source>Display English language names</source>
        <translation>Afficher les noms de langue en anglais</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="158"/>
        <source>Support obsolete long (unencrypted) payment ids (using them harms your privacy)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="296"/>
        <source>failed to read wallet password</source>
        <translation>échec de la lecture du mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="303"/>
        <source>Enter a new password for the wallet</source>
        <translation>Entrer un nouveau mot de passe pour le portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="303"/>
        <source>Wallet password</source>
        <translation>Mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="313"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="505"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="322"/>
        <source>possibly lost connection to daemon</source>
        <translation>connexion avec le démon peut-être perdue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="339"/>
        <source>Error: </source>
        <translation>Erreur&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="448"/>
        <source>Is this OK?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="563"/>
        <source>transaction %s was rejected by daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="619"/>
        <source>File %s already exists. Are you sure to overwrite it?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9779"/>
        <source>Failed to initialize wallet</source>
        <translation>Échec de l&apos;initialisation du portefeuille</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="247"/>
        <source>Use daemon instance at &lt;host&gt;:&lt;port&gt;</source>
        <translation>Utiliser l&apos;instance de démon située à &lt;hôte&gt;:&lt;port&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="248"/>
        <source>Use daemon instance at host &lt;arg&gt; instead of localhost</source>
        <translation>Utiliser l&apos;instance de démon située à l&apos;hôte &lt;arg&gt; au lieu de localhost</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="253"/>
        <source>Wallet password file</source>
        <translation>Fichier mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="254"/>
        <source>Use daemon instance at port &lt;arg&gt; instead of 18081</source>
        <translation>Utiliser l&apos;instance de démon située au port &lt;arg&gt; au lieu de 18081</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="263"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>Pour testnet, le démon doit aussi être lancé avec l&apos;option --testnet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="380"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>impossible de spécifier l&apos;hôte ou le port du démon plus d&apos;une fois</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="511"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>impossible de spécifier plus d&apos;une option parmis --password et --password-file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="524"/>
        <source>the password file specified could not be read</source>
        <translation>le fichier mot de passe spécifié n&apos;a pas pu être lu</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="550"/>
        <source>Failed to load file </source>
        <translation>Échec du chargement du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="252"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation>Mot de passe du portefeuille (échapper/citer si nécessaire)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="249"/>
        <source>[&lt;ip&gt;:]&lt;port&gt; socks proxy to use for daemon connections</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="250"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation>Activer les commandes qui dépendent d&apos;un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="251"/>
        <source>Disable commands which rely on a trusted daemon</source>
        <translation>Désactiver les commandes qui dépendent d&apos;un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="255"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>Spécifier le nom_utilisateur:[mot_de_passe] pour le client RPC du démon</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="256"/>
        <source>Enable SSL on daemon RPC connections: enabled|disabled|autodetect</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="260"/>
        <source>List of valid fingerprints of allowed RPC servers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="261"/>
        <source>Allow any SSL certificate from the daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="262"/>
        <source>Allow user (via --daemon-ssl-ca-certificates) chain certificates</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="264"/>
        <source>For stagenet. Daemon must also be launched with --stagenet flag</source>
        <translation>Pour stagenet, le démon doit aussi être lancé avec l&apos;option --stagenet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="266"/>
        <source>Set shared ring database path</source>
        <translation>Définir le chemin de la base de donnée de cercles partagés</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="277"/>
        <source>Number of rounds for the key derivation function</source>
        <translation>Nombre de rondes de la fonction de dérivation de clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="278"/>
        <source>HW device to use</source>
        <translation>Portefeuille matériel à utiliser</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="279"/>
        <source>HW device wallet derivation path (e.g., SLIP-10)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="281"/>
        <source>Do not use DNS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="282"/>
        <source>Do not connect to a daemon, nor use DNS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="283"/>
        <source>File containing extra entropy to initialize the PRNG (any data, aim for 256 bits of entropy to be useful, wihch typically means more than 256 bits of data)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="372"/>
        <source>Invalid argument for </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="419"/>
        <source>Enabling --</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="419"/>
        <source> requires --</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="420"/>
        <source> or --</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="420"/>
        <source> or use of a .onion/.i2p domain</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="454"/>
        <source>--trusted-daemon and --untrusted-daemon are both seen, assuming untrusted</source>
        <translation>--trusted-daemon et --untrusted-daemon présents simultanément, --untrusted-daemon choisi</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="464"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Le démon est local, supposons qu&apos;il est de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="531"/>
        <source>no password specified; use --prompt-for-password to prompt for a password</source>
        <translation>pas de mot de passe spécifié; utilisez --prompt-for-password pour demander un mot de passe</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="533"/>
        <source>Enter a new password for the wallet</source>
        <translation>Entrer un nouveau mot de passe pour le portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="533"/>
        <source>Wallet password</source>
        <translation>Mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="556"/>
        <source>Failed to parse JSON</source>
        <translation>Échec de l&apos;analyse JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="563"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation>Version %u trop récente, on comprend au mieux %u</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="579"/>
        <source>failed to parse view key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="584"/>
        <location filename="../src/wallet/wallet2.cpp" line="652"/>
        <location filename="../src/wallet/wallet2.cpp" line="697"/>
        <source>failed to verify view key secret key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="595"/>
        <source>failed to parse spend key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="600"/>
        <location filename="../src/wallet/wallet2.cpp" line="662"/>
        <location filename="../src/wallet/wallet2.cpp" line="723"/>
        <source>failed to verify spend key secret key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="612"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="632"/>
        <source>At least one of either an Electrum-style word list, private view key, or private spend key must be specified</source>
        <translation>Il faut spécifier au moins une des options parmis la liste de mots de style Electrum, la clé privée d&apos;audit et la clé privée de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="636"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Liste de mots de style Electrum et clé privée spécifiées en même temps</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="646"/>
        <source>invalid address</source>
        <translation>adresse invalide</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="655"/>
        <source>view key does not match standard address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="665"/>
        <source>spend key does not match standard address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="673"/>
        <source>Cannot generate deprecated wallets from JSON</source>
        <translation>Impossible de générer un portefeuille obsolète à partir de JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="709"/>
        <source>failed to parse address: </source>
        <translation>échec de l&apos;analyse de l&apos;adresse&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="715"/>
        <source>Address must be specified in order to create watch-only wallet</source>
        <translation>L&apos;adresse doit être spécifiée afin de créer un portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="732"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="1721"/>
        <source>Password is needed to compute key image for incoming monero</source>
        <translation>Le mot de passe est requis pour calculer l&apos;image de clé pour les moneros entrants</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="1722"/>
        <source>Invalid password: password is needed to compute key image for incoming monero</source>
        <translation>Mot de passe invalide : le mot de passe est requis pour calculer l&apos;image de clé pour les moneros entrants</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="4230"/>
        <location filename="../src/wallet/wallet2.cpp" line="4820"/>
        <location filename="../src/wallet/wallet2.cpp" line="5420"/>
        <source>Primary account</source>
        <translation>Compte primaire</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="10993"/>
        <source>No funds received in this tx.</source>
        <translation>Aucun fonds n&apos;a été reçu dans cette transaction.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="11757"/>
        <source>failed to read file </source>
        <translation>échec de la lecture du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="156"/>
        <source>Set subaddress lookahead sizes to &lt;major&gt;:&lt;minor&gt;</source>
        <translation>Définir les tailles d&apos;anticipation des sous-addresses à &lt;majeur&gt;:&lt;mineur&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="257"/>
        <source>Path to a PEM format private key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="258"/>
        <source>Path to a PEM format certificate</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="259"/>
        <source>Path to file containing concatenated PEM format certificate(s) to replace system CA(s).</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>tools::wallet_rpc_server</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="187"/>
        <source>Failed to create directory </source>
        <translation>Échec de la création du répertoire </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="189"/>
        <source>Failed to create directory %s: %s</source>
        <translation>Échec de la création du répertoire %s&#xa0;: %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="200"/>
        <source>Cannot specify --</source>
        <translation>Impossible de spécifier --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="200"/>
        <source> and --</source>
        <translation> et --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="219"/>
        <source>Failed to create file </source>
        <translation>Échec de la création du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="219"/>
        <source>. Check permissions or remove file</source>
        <translation>. Vérifiez les permissions ou supprimez le fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="229"/>
        <source>Error writing to file </source>
        <translation>Erreur d&apos;écriture dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="232"/>
        <source>RPC username/password is stored in file </source>
        <translation>nom_utilisateur/mot_de_passe RPC sauvegardé dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="587"/>
        <source>Tag %s is unregistered.</source>
        <translation>Le mot clé %s n&apos;est pas enregistré.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3227"/>
        <source>Transaction not possible. Available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>Transaction impossible. Solde disponible&#xa0;: %s, montant de la transaction %s = %s + %s (frais)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4395"/>
        <source>This is the RPC monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation>Ceci est le portefeuille monero par RPC. Il a besoin de se
connecter à un démon monero pour fonctionner correctement.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4236"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>Impossible de spécifier plus d&apos;une option parmis --wallet-file et --generate-from-json</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4221"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation>Impossible de spécifier plus d&apos;une option parmis --testnet et --stagenet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4248"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>--wallet-file, --generate-from-json ou --wallet-dir doit être spécifié</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4252"/>
        <source>Loading wallet...</source>
        <translation>Chargement du portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4286"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4318"/>
        <source>Saving wallet...</source>
        <translation>Sauvegarde du portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4288"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4320"/>
        <source>Successfully saved</source>
        <translation>Sauvegardé avec succès</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4291"/>
        <source>Successfully loaded</source>
        <translation>Chargé avec succès</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4295"/>
        <source>Wallet initialization failed: </source>
        <translation>Échec de l&apos;initialisation du portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4301"/>
        <source>Failed to initialize wallet RPC server</source>
        <translation>Échec de l&apos;initialisation du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4305"/>
        <source>Starting wallet RPC server</source>
        <translation>Démarrage du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4312"/>
        <source>Failed to run wallet: </source>
        <translation>Échec du lancement du portefeuille&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4315"/>
        <source>Stopped wallet RPC server</source>
        <translation>Arrêt du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4324"/>
        <source>Failed to save wallet: </source>
        <translation>Échec de la sauvegarde du portefeuille&#xa0;: </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="168"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9726"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4376"/>
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
        <location filename="../src/wallet/wallet_args.cpp" line="213"/>
        <source>Logging to: </source>
        <translation>Journalisation dans&#xa0;: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="215"/>
        <source>Logging to %s</source>
        <translation>Journalisation dans %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="219"/>
        <source>WARNING: You may not have a high enough lockable memory limit</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="221"/>
        <source>see ulimit -l</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="146"/>
        <source>Usage:</source>
        <translation>Usage&#xa0;:</translation>
    </message>
</context>
</TS>
