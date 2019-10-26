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
        <translation>Tentative d&apos;enregistrement d&apos;une transaction dans un fichier, mais le fichier spécifié existe déjà. Sortie pour ne pas risquer de l&apos;écraser. Fichier :</translation>
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
        <translation>la transaction %s a été rejetée par le démon avec le statut : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="150"/>
        <source>. Reason: </source>
        <translation>. Raison : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="152"/>
        <source>Unknown exception: </source>
        <translation>Exception inconnue : </translation>
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
        <location filename="../src/wallet/api/wallet.cpp" line="1513"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1602"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1515"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1604"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1517"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1606"/>
        <source>RPC error: </source>
        <translation>Erreur RPC : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1545"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1637"/>
        <source>not enough outputs for specified ring size</source>
        <translation>pas assez de sorties pour la taille de cercle spécifiée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1547"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1639"/>
        <source>found outputs to use</source>
        <translation>sorties à utiliser trouvées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1549"/>
        <source>Please sweep unmixable outputs.</source>
        <translation>Veuillez balayer les sorties non mélangeables.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1523"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1613"/>
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
        <translation>échec de la génération du nouveau portefeuille : </translation>
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
        <location filename="../src/wallet/api/wallet.cpp" line="1148"/>
        <source>Failed to load transaction from file</source>
        <translation>Échec du chargement de la transaction du fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1164"/>
        <source>Wallet is view only</source>
        <translation>Portefeuille d&apos;audit uniquement</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1172"/>
        <source>failed to save file </source>
        <translation>échec de l&apos;enregistrement du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1188"/>
        <source>Key images can only be imported with a trusted daemon</source>
        <translation>Les images de clé ne peuvent être importées qu&apos;avec un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1201"/>
        <source>Failed to import key images: </source>
        <translation>Échec de l&apos;importation des images de clé : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1233"/>
        <source>Failed to get subaddress label: </source>
        <translation>Échec de la récupération de l&apos;étiquette de sous-adresse : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1246"/>
        <source>Failed to set subaddress label: </source>
        <translation>Échec de l&apos;affectation de l&apos;étiquette de sous-adresse : </translation>
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
        <location filename="../src/wallet/api/wallet.cpp" line="1263"/>
        <source>Failed to get multisig info: </source>
        <translation>Échec de la récupération des infos multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1280"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1294"/>
        <source>Failed to make multisig: </source>
        <translation>Échec de la création multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1309"/>
        <source>Failed to finalize multisig wallet creation</source>
        <translation>Échec de la finalisation de la création du portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1312"/>
        <source>Failed to finalize multisig wallet creation: </source>
        <translation>Échec de la finalisation de la création du portefeuille multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1328"/>
        <source>Failed to export multisig images: </source>
        <translation>Échec de l&apos;exportation des images multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1346"/>
        <source>Failed to parse imported multisig images</source>
        <translation>Échec de l&apos;analyse des images multisig importées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1356"/>
        <source>Failed to import multisig images: </source>
        <translation>Échec de l&apos;importation des images multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1370"/>
        <source>Failed to check for partial multisig key images: </source>
        <translation>Échec de la vérification des images de clé multisig partielles : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1398"/>
        <source>Failed to restore multisig transaction: </source>
        <translation>Échec de la restauration de la transaction multisig : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1439"/>
        <source>Sending all requires one destination address</source>
        <translation>Tout envoyer nécessite une adresse de destination</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1443"/>
        <source>Destinations and amounts are unequal</source>
        <translation>Les destinations et montants ne sont pas égaux</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1451"/>
        <source>payment id has invalid format, expected 64 character hex string: </source>
        <translation>le format de l&apos;ID de paiement est invalide, 64 caractères hexadécimaux sont attendus : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1459"/>
        <source>Invalid destination address</source>
        <translation>Adresse de destination invalide</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1465"/>
        <source>a single transaction cannot use more than one payment id</source>
        <translation>une unique transaction ne peut pas utiliser plus d&apos;un ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1491"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>échec de la définition de l&apos;ID de paiement, bien qu&apos;il ait été décodé correctement</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1519"/>
        <source>failed to get outputs to mix: %s</source>
        <translation>échec de la récupération de sorties à mélanger : %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1530"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1621"/>
        <source>not enough money to transfer, overall balance only %s, sent amount %s</source>
        <translation>pas assez de fonds pour le transfer, solde global disponible %s, montant envoyé %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1537"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1629"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>pas assez de fonds pour le transfert, montant disponible %s, montant envoyé %s = %s + %s (frais)</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1547"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1639"/>
        <source>output amount</source>
        <translation>montant de la sortie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1552"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1643"/>
        <source>transaction was not constructed</source>
        <translation>la transaction n&apos;a pas été construite</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1555"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1646"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transaction %s a été rejetée par le démon avec le statut : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1560"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1651"/>
        <source>one of destinations is zero</source>
        <translation>une des destinations est zéro</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1562"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1653"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>échec de la recherche d&apos;une façon adéquate de scinder les transactions</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1564"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1655"/>
        <source>unknown transfer error: </source>
        <translation>erreur de transfert inconnue : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1566"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1657"/>
        <source>internal error: </source>
        <translation>erreur interne : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1568"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1659"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1570"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1661"/>
        <source>unknown error</source>
        <translation>erreur inconnue</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1608"/>
        <source>failed to get outputs to mix</source>
        <translation>échec de la récupération de sorties à mélanger</translation>
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
        <translation>Échec de l&apos;analyse de l&apos;ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1769"/>
        <source>no tx keys found for this txid</source>
        <translation>aucune clé de transaction trouvée pour cet ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1793"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1802"/>
        <source>Failed to parse tx key</source>
        <translation>Échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1811"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1840"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1868"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1949"/>
        <source>Failed to parse address</source>
        <translation>Échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1954"/>
        <source>Address must not be a subaddress</source>
        <translation>L&apos;adresse ne doit pas être une sous-adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1994"/>
        <source>The wallet must be in multisig ready state</source>
        <translation>Le portefeuille doit être multisig et prêt</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2016"/>
        <source>Given string is not a key</source>
        <translation>La chaîne entrée n&apos;est pas une clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2258"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>Réexaminer les dépenses ne peut se faire qu&apos;avec un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2307"/>
        <source>Invalid output: </source>
        <translation>Sortie invalide : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2314"/>
        <source>Failed to mark outputs as spent</source>
        <translation>Échec du marquage des sorties comme dépensées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2336"/>
        <source>Failed to mark output as spent</source>
        <translation>Échec du marquage de la sortie comme dépensée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2358"/>
        <source>Failed to mark output as unspent</source>
        <translation>Échec du marquage de la sortie comme non dépensée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2325"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2347"/>
        <source>Failed to parse output amount</source>
        <translation>Échec de l&apos;analyse du montant de la sortie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2330"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2352"/>
        <source>Failed to parse output offset</source>
        <translation>Échec de l&apos;analyse de l&apos;offset de la sortie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2369"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2408"/>
        <source>Failed to parse key image</source>
        <translation>Échec de l&apos;analyse de l&apos;image de clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2375"/>
        <source>Failed to get ring</source>
        <translation>Échec de la récupération du cercle</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2393"/>
        <source>Failed to get rings</source>
        <translation>Échec de la récupération des cercles</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2414"/>
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
        <translation>Autoriser l&apos;IPv6 pour le RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="95"/>
        <source>Ignore unsuccessful IPv4 bind for RPC</source>
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
        <translation>Activer SSL pour les connexions RPC : enabled|disabled|autodetect</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="100"/>
        <source>Path to a PEM format private key</source>
        <translation>Chemin vers une clé privée au format PEM</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="101"/>
        <source>Path to a PEM format certificate</source>
        <translation>Chemin vers un certificat au format PEM</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="102"/>
        <source>Path to file containing concatenated PEM format certificate(s) to replace system CA(s).</source>
        <translation>Chemin vers un fichier contenant le(s) certificat(s) au format PEM concaténé(s) pour remplacer l(es) AC(s) du système.</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="103"/>
        <source>List of certificate fingerprints to allow</source>
        <translation>Liste des empreintes de certificat à autoriser</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="104"/>
        <source>Allow user (via --rpc-ssl-certificates) chain certificates</source>
        <translation>Autoriser une chaîne de certificats utilisateur (via --rpc-ssl-certificates)</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="105"/>
        <source>Allow any peer certificate</source>
        <translation>Autoriser tous les certificats pairs</translation>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="663"/>
        <source>Commands: </source>
        <translation>Commandes : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4801"/>
        <source>failed to read wallet password</source>
        <translation>échec de la lecture du mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4388"/>
        <source>invalid password</source>
        <translation>mot de passe invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3428"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation>set seed : requiert un argument. options disponibles : language</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3467"/>
        <source>set: unrecognized argument(s)</source>
        <translation>set : argument(s) non reconnu(s)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4641"/>
        <source>wallet file path not valid: </source>
        <translation>chemin du fichier portefeuille non valide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3537"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation>Tentative de génération ou de restauration d&apos;un portefeuille, mais le fichier spécifié existe déjà. Sortie pour ne pas risquer de l&apos;écraser.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3414"/>
        <source>needs an argument</source>
        <translation>requiert un argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3437"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3438"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3439"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3441"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3448"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3449"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3451"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3453"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3454"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3455"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3458"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3461"/>
        <source>0 or 1</source>
        <translation>0 ou 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3446"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3450"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3457"/>
        <source>unsigned integer</source>
        <translation>entier non signé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3696"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-deterministic-wallet utilise --generate-new-wallet, pas --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3725"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation>spécifiez un paramètre de récupération avec --electrum-seed=&quot;liste de mots ici&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4105"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation>spécifiez un chemin de portefeuille avec --generate-new-wallet (pas --wallet-file)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4321"/>
        <source>wallet failed to connect to daemon: </source>
        <translation>échec de la connexion du portefeuille au démon : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4329"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation>Le démon utilise une version majeure de RPC (%u) différente de celle du portefeuille (%u) : %s. Mettez l&apos;un des deux à jour, ou utilisez --allow-mismatched-daemon-version.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4350"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation>Liste des langues disponibles pour la phrase mnémonique de votre portefeuille :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4436"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Veuillez dorénavant utiliser la nouvelle phrase mnémonique que nous fournissons.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4452"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4526"/>
        <source>Generated new wallet: </source>
        <translation>Nouveau portefeuille généré : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4461"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4531"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4575"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4630"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4672"/>
        <source>Opened watch-only wallet</source>
        <translation>Ouverture du portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4676"/>
        <source>Opened wallet</source>
        <translation>Ouverture du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4694"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Veuillez procéder à la mise à jour de votre portefeuille.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4709"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Le format de votre fichier portefeuille est en cours de mise à jour.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4717"/>
        <source>failed to load wallet: </source>
        <translation>échec du chargement du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4734"/>
        <source>Use the &quot;help&quot; command to see the list of available commands.
</source>
        <translation>Utilisez la commande &quot;help&quot; pour voir la liste des commandes disponibles.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4779"/>
        <source>Wallet data saved</source>
        <translation>Données du portefeuille sauvegardées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4984"/>
        <source>Mining started in daemon</source>
        <translation>La mine a démarré dans le démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4986"/>
        <source>mining has NOT been started: </source>
        <translation>la mine n&apos;a PAS démarré : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5006"/>
        <source>Mining stopped in daemon</source>
        <translation>La mine a été stoppée dans le démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5008"/>
        <source>mining has NOT been stopped: </source>
        <translation>la mine n&apos;a PAS été stoppée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5090"/>
        <source>Blockchain saved</source>
        <translation>Chaîne de blocs sauvegardée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5109"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5155"/>
        <source>Height </source>
        <translation>Hauteur </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5157"/>
        <source>spent </source>
        <translation>dépensé </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5267"/>
        <source>Starting refresh...</source>
        <translation>Démarrage du rafraîchissement...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5293"/>
        <source>Refresh done, blocks received: </source>
        <translation>Rafraîchissement effectué, blocs reçus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6617"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5980"/>
        <source>bad locked_blocks parameter:</source>
        <translation>mauvais paramètre locked_blocks :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6637"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6896"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation>une unique transaction ne peut pas utiliser plus d&apos;un ID de paiement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6082"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6646"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6864"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6904"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>échec de la définition de l&apos;ID de paiement, bien qu&apos;il ait été décodé correctement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1062"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1184"/>
        <source>Send this multisig info to all other participants, then use exchange_multisig_keys &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Envoyez ces infos multisig à tous les autres participants, puis utilisez exchange_multisig_keys &lt;info1&gt; [&lt;info2&gt;...] avec les infos multisig des autres</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1193"/>
        <source>Multisig wallet has been successfully created. Current wallet type: </source>
        <translation>Le portefeuille multisig a été créé avec succès. Type du portefeuille actuel : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1199"/>
        <source>Failed to perform multisig keys exchange: </source>
        <translation>Échec de l&apos;échange de clés multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1526"/>
        <source>Failed to load multisig transaction from MMS</source>
        <translation>Échec du chargement de la transaction multisig à partir du MMS</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1994"/>
        <source>Failed to mark output spent: </source>
        <translation>Échec du marquage de la sortie comme dépensée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2021"/>
        <source>Failed to mark output unspent: </source>
        <translation>Échec du marquage de la sortie comme non dépensée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2045"/>
        <source>Spent: </source>
        <translation>Dépensé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2047"/>
        <source>Not spent: </source>
        <translation>Non dépensé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2051"/>
        <source>Failed to check whether output is spent: </source>
        <translation>Impossible de vérifier si la sortie est dépensée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2200"/>
        <source>Please confirm the transaction on the device</source>
        <translation>Veuillez confirmer la transaction sur l&apos;appareil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2767"/>
        <source>Device name not specified</source>
        <translation>Nom de l&apos;appareil non spécifié</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2776"/>
        <source>Device reconnect failed</source>
        <translation>Échec de la reconnexion à l&apos;appareil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2781"/>
        <source>Device reconnect failed: </source>
        <translation>Échec de la reconnexion à l&apos;appareil : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3074"/>
        <source>Show the incoming/outgoing transfers within an optional height range.

Output format:
In or Coinbase:    Block Number, &quot;block&quot;|&quot;in&quot;,              Time, Amount,  Transaction Hash, Payment ID, Subaddress Index,                     &quot;-&quot;, Note
Out:               Block Number, &quot;out&quot;,                     Time, Amount*, Transaction Hash, Payment ID, Fee, Destinations, Input addresses**, &quot;-&quot;, Note
Pool:                            &quot;pool&quot;, &quot;in&quot;,              Time, Amount,  Transaction Hash, Payment Id, Subaddress Index,                     &quot;-&quot;, Note, Double Spend Note
Pending or Failed:               &quot;failed&quot;|&quot;pending&quot;, &quot;out&quot;, Time, Amount*, Transaction Hash, Payment ID, Fee, Input addresses**,               &quot;-&quot;, Note

* Excluding change and fee.
** Set of address indices used as inputs in this transfer.</source>
        <translation>Affiche les transferts entrants/sortants dans une plage de hauteur optionnelle.

Format de sortie :
Entrant ou Base de la pièce: Numéro du bloc, &quot;block&quot;|&quot;in&quot;,              Heure, Montant,  Hachage de transation, ID de paiement, Indice de sous-adresse,                    &quot;-&quot;, Remarque
Sortant:                     Numéro du bloc, &quot;out&quot;,                     Heure, Montant*, Hachage de transation, ID de paiement, Frais, Destinations, Adresses d&apos;entrées**, &quot;-&quot;, Remarque
Pool:                                        &quot;pool&quot;, &quot;in&quot;,              Heure, Montant,  Hachage de transation, ID de paiement, Indice de sous-adresse,                    &quot;-&quot;, Remarque, Remarque de double dépense
En attente ou Échoué:                        &quot;failed&quot;|&quot;pending&quot;, &quot;out&quot;, Heure, Montant*, Hachage de transation, ID de paiement, Frais, Adresses d&apos;entrées**,               &quot;-&quot;, Remarque

* A l&apos;exception du change et des frais.
** Jeux d&apos;indices d&apos;adresse utilisé en entrées dans ce transfert.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3084"/>
        <source>export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;filepath&gt;]</source>
        <translation>export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;hauteur_min&gt; [&lt;hauteur_max&gt;]] [output=&lt;chemin_de_fichier&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3085"/>
        <source>Export to CSV the incoming/outgoing transfers within an optional height range.</source>
        <translation>Exporter en CSV les transfers entrants/sortants dans une plage de hauteur optionnelle.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3127"/>
        <source>Export a signed set of key images to a &lt;filename&gt;.</source>
        <translation>Exporter un jeux d&apos;images de clé signé dans un &lt;fichier&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3135"/>
        <source>Synchronizes key images with the hw wallet.</source>
        <translation>Synchroniser les images de clé avec le portefeuille matériel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3158"/>
        <source>Generate a new random full size payment id (obsolete). These will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids.</source>
        <translation>Génère un nouvel ID de paiement long (obsolète).  Il se sera pas chiffré sur la chaîne de blocs, voir integrated_address pour des IDs de paiement court et chiffrés.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3174"/>
        <source>Performs extra multisig keys exchange rounds. Needed for arbitrary M/N multisig wallets</source>
        <translation>Réalise des échanges de clés multisignatures supplémentaires. Nécessaire pour des portefeuilles multisignatures M/N arbitraires</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3198"/>
        <source>Interface with the MMS (Multisig Messaging System)
&lt;subcommand&gt; is one of:
  init, info, signer, list, next, sync, transfer, delete, send, receive, export, note, show, set, help
  send_signer_config, start_auto_config, stop_auto_config, auto_config
Get help about a subcommand with: help mms &lt;subcommand&gt;, or mms help &lt;subcommand&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3206"/>
        <source>Initialize and configure the MMS for M/N = number of required signers/number of authorized signers multisig</source>
        <translation>Initialise et configure le MMS pour M/N = nombre de signataires requis / nombre de signataires multisignature autorisés</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3210"/>
        <source>Display current MMS configuration</source>
        <translation>Afficher la configuration MMS actuelle</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3214"/>
        <source>Set or modify authorized signer info (single-word label, transport address, Monero address), or list all signers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3218"/>
        <source>List all messages</source>
        <translation>Lister tous les messages</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3222"/>
        <source>Evaluate the next possible multisig-related action(s) according to wallet state, and execute or offer for choice
By using &apos;sync&apos; processing of waiting messages with multisig sync info can be forced regardless of wallet state</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3227"/>
        <source>Force generation of multisig sync info regardless of wallet state, to recover from special situations like &quot;stale data&quot; errors</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3231"/>
        <source>Initiate transfer with MMS support; arguments identical to normal &apos;transfer&apos; command arguments, for info see there</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3235"/>
        <source>Delete a single message by giving its id, or delete all messages by using &apos;all&apos;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3239"/>
        <source>Send a single message by giving its id, or send all waiting messages</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3243"/>
        <source>Check right away for new messages to receive</source>
        <translation>Vérifier tout de suite les nouveaux messages à recevoir</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3247"/>
        <source>Write the content of a message to a file &quot;mms_message_content&quot;</source>
        <translation>Écrire le contenu d&apos;un message dans un fichier &quot;mms_message_content&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3251"/>
        <source>Send a one-line message to an authorized signer, identified by its label, or show any waiting unread notes</source>
        <translation>Envoyer un message d&apos;une ligne à un signataire autorisé, identifié par son étiquette, ou afficher une note non lue en attente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3255"/>
        <source>Show detailed info about a single message</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3259"/>
        <source>Available options:
 auto-send &lt;1|0&gt;
   Whether to automatically send newly generated messages right away.
 </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3265"/>
        <source>Send completed signer config to all other authorized signers</source>
        <translation>Envoyer la configuration de signataire complétée à tous les autres signataires autorisés</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3269"/>
        <source>Start auto-config at the auto-config manager&apos;s wallet by issuing auto-config tokens and optionally set others&apos; labels</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3273"/>
        <source>Delete any auto-config tokens and abort a auto-config process</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3277"/>
        <source>Start auto-config by using the token received from the auto-config manager</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3299"/>
        <source>Mark output(s) as spent so they never get selected as fake outputs in a ring</source>
        <translation>Marquer les sorties comme dépensées pour qu&apos;elles ne soient jamais sélectionnées comme leurre dans un cercle</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3303"/>
        <source>Marks an output as unspent so it may get selected as a fake output in a ring</source>
        <translation>Marquer une sortie comme non dépensée pour qu&apos;elle puisse être sélectionnée comme leurre dans un cercle</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3307"/>
        <source>Checks whether an output is marked as spent</source>
        <translation>Vérifie si une sortie est marquée comme dépensée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3464"/>
        <source>&lt;device_name[:device_spec]&gt;</source>
        <translation>&lt;nom_périphérique[:spec_périphérique]&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3486"/>
        <source>wrong number range, use: %s</source>
        <translation>mauvaise plage de nombres, utilisez: %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3591"/>
        <source>NOTE: the following %s can be used to recover access to your wallet. Write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3593"/>
        <source>string</source>
        <translation>chaîne</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3593"/>
        <source>25 words</source>
        <translation>phrase mnémonique de 25 mots</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4014"/>
        <source>Secret spend key (%u of %u)</source>
        <translation>Clef secrète de dépense (%u de %u)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4091"/>
        <source>Use --restore-height or --restore-date if you want to restore an already setup account from a specific height.</source>
        <translation>Utilisez --restore-height ou --restore-date si vous voulez restaurer un compte déjà configuré à partir d&apos;une hauteur spécifique.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4093"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4181"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6254"/>
        <source>Is this okay?</source>
        <translation>Est-ce correct ?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4204"/>
        <source>Still apply restore height?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4360"/>
        <source>Enter the number corresponding to the language of your choice</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5199"/>
        <source>Device requires attention</source>
        <translation>Le périphérique demande votre attention</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5205"/>
        <source>Enter device PIN</source>
        <translation>Entrer le code PIN du périphérique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5207"/>
        <source>Failed to read device PIN</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5214"/>
        <source>Please enter the device passphrase on the device</source>
        <translation>Veuillez entrer le mot de passe du périphérique sur le périphérique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5219"/>
        <source>Enter device passphrase</source>
        <translation>Entrer le mot de passe du périphérique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5221"/>
        <source>Failed to read device passphrase</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5237"/>
        <source>The first refresh has finished for the HW-based wallet with received money. hw_key_images_sync is needed. </source>
        <translation>Le premier rafraîchissement du portefeuille matériel s&apos;est terminé avec de l&apos;argent reçu. hw_key_images_sync est requis. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4918"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5239"/>
        <source>Do you want to do it now? (Y/Yes/N/No): </source>
        <translation>Voulez vous le faire maintenant . (Y/Yes/N/No) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5241"/>
        <source>hw_key_images_sync skipped. Run command manually before a transfer.</source>
        <translation>hw_key_images_sync ignoré. Lancez la commande manuellement avant un transfert.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5444"/>
        <source>Invalid keyword: </source>
        <translation>Mot clef invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6165"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6174"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6261"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6410"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6667"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6710"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6947"/>
        <source>transaction cancelled.</source>
        <translation>transaction annulée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6154"/>
        <source>Failed to check for backlog: </source>
        <translation>Échec de la vérification du backlog : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6203"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6683"/>
        <source>
Transaction </source>
        <translation>
Transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6208"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6688"/>
        <source>Spending from address index %d
</source>
        <translation>Dépense depuis l&apos;adresse d&apos;index %d
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6210"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6690"/>
        <source>WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.
</source>
        <translation>ATTENTION : Des sorties de multiples adresses sont utilisées ensemble, ce qui pourrait potentiellement compromettre votre confidentialité.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6212"/>
        <source>Sending %s.  </source>
        <translation>Envoi de %s.  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6215"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation>Votre transaction doit être scindée en %llu transactions. Il en résulte que des frais de transaction doivent être appliqués à chaque transaction, pour un total de %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6221"/>
        <source>The transaction fee is %s</source>
        <translation>Les frais de transaction sont de %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6224"/>
        <source>, of which %s is dust from change</source>
        <translation>, dont %s est de la poussière de monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6225"/>
        <source>.</source>
        <translation>.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6225"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation>Un total de %s de poussière de monnaie rendue sera envoyé à une adresse de poussière</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6274"/>
        <source>Unsigned transaction(s) successfully written to MMS</source>
        <translation>Transaction(s) non signée(s) écrite(s) avec succès vers MMS</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6282"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6319"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6421"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6433"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6721"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6758"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6957"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6969"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Échec de l&apos;écriture de(s) transaction(s) dans le fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6287"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6324"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6425"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6437"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6725"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6762"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6961"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6973"/>
        <source>Unsigned transaction(s) successfully written to file: </source>
        <translation>Transaction(s) non signée(s) écrite(s) dans le fichier avec succès : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6296"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6737"/>
        <source>Failed to cold sign transaction with HW wallet</source>
        <translation>Échec de la signature à froid de la transaction avec le portefeuille matériel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6380"/>
        <source>No unmixable outputs found</source>
        <translation>Aucune sortie non mélangeable trouvée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6395"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6695"/>
        <source>Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6401"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6701"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6939"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6448"/>
        <source>Discarding %s of unmixable outputs that cannot be spent, which can be undone by &quot;rescan_spent&quot;.  Is this okay?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6487"/>
        <source>No address given</source>
        <translation>Aucune adresse fournie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6858"/>
        <source>failed to parse Payment ID</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2081"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2128"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6881"/>
        <source>failed to parse key image</source>
        <translation>échec de l&apos;analyse de l&apos;image de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2158"/>
        <source>Unlike Bitcoin, your Monero transactions and balance stay private and are not visible to the world by default.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2159"/>
        <source>However, you have the option of making those available to select parties if you choose to.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2167"/>
        <source>Welcome to Monero and financial privacy. For more information see https://GetMonero.org</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2672"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2691"/>
        <source>Invalid amount</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2965"/>
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
 </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5128"/>
        <source>NOTE: this transaction uses an encrypted payment ID: consider using subaddresses instead</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5132"/>
        <source>WARNING: this transaction uses an unencrypted payment ID: these are obsolete and ignored. Use subaddresses instead.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6919"/>
        <source>No outputs found</source>
        <translation>Pas de sorties trouvées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6924"/>
        <source>Multiple transactions are created, which is not supposed to happen</source>
        <translation>De multiples transactions sont crées, ce qui n&apos;est pas supposé arriver</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6929"/>
        <source>The transaction uses multiple or no inputs, which is not supposed to happen</source>
        <translation>La transaction utilise aucune ou de multiples entrées, ce qui n&apos;est pas supposé arriver</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7007"/>
        <source>missing threshold amount</source>
        <translation>montant seuil manquant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7012"/>
        <source>invalid amount threshold</source>
        <translation>montant seuil invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7223"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu, %s. %sIs this okay?</source>
        <translation>%lu transactions chargées, pour %s, %s de frais, %s, %s, avec taille de cercle minimum de %lu, %s. %sEst-ce correct ?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8425"/>
        <source>Rescan anyway?</source>
        <translation>Rescanner quand même ?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8533"/>
        <source>locked due to inactivity</source>
        <translation>verrouillé pour cause d&apos;inactivité</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8992"/>
        <source>Short payment IDs are to be used within an integrated address only</source>
        <translation>Les IDs de paiement cours ne peuvent être utilisés que dans les adresses intégrées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9815"/>
        <source> (Y/Yes/N/No): </source>
        <translation> (Y/Yes/N/No): </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9842"/>
        <source>Choose processing:</source>
        <translation>Choisissez comment procéder :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9851"/>
        <source>Sign tx</source>
        <translation>Signer la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9859"/>
        <source>Send the tx for submission to </source>
        <translation>Envoyer la transaction à soumettre à </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9863"/>
        <source>Send the tx for signing to </source>
        <translation>Envoyer la transaction à signer à </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9870"/>
        <source>Submit tx</source>
        <translation>Soumettre la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9873"/>
        <source>unknown</source>
        <translation>inconnu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9879"/>
        <source>Choice: </source>
        <translation>Choix : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9891"/>
        <source>Wrong choice</source>
        <translation>Mauvais choix</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9898"/>
        <source>Id</source>
        <translation>ID</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9898"/>
        <source>I/O</source>
        <translation>E/S</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9898"/>
        <source>Authorized Signer</source>
        <translation>Signataire autorisé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9899"/>
        <source>Message Type</source>
        <translation>Type de message</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9899"/>
        <source>Height</source>
        <translation>Hauteur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9899"/>
        <source>R</source>
        <translation>R</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9899"/>
        <source>Message State</source>
        <translation>État du message</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9899"/>
        <source>Since</source>
        <translation>Depuis</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9916"/>
        <source> ago</source>
        <translation> </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9922"/>
        <source>#</source>
        <translation>#</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9922"/>
        <source>Transport Address</source>
        <translation>Adresse de transport</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9923"/>
        <source>Auto-Config Token</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9923"/>
        <source>Monero Address</source>
        <translation>Adresse Monero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9927"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9935"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9937"/>
        <source>&lt;not set&gt;</source>
        <translation>&lt;non défini&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9978"/>
        <source>Message </source>
        <translation>Message </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9979"/>
        <source>In/out: </source>
        <translation>Entrant/sortant : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9981"/>
        <source>State: </source>
        <translation>État : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9981"/>
        <source>%s since %s, %s ago</source>
        <translation>%s depuis %s, il y a %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9985"/>
        <source>Sent: Never</source>
        <translation>Envoyé : Jamais</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9989"/>
        <source>Sent: %s, %s ago</source>
        <translation>Envoyé : %s, il y a %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9992"/>
        <source>Authorized signer: </source>
        <translation>Signataire autorisé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9993"/>
        <source>Content size: </source>
        <translation>Taille du contenu : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9993"/>
        <source> bytes</source>
        <translation> octets</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9994"/>
        <source>Content: </source>
        <translation>Contenu : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9994"/>
        <source>(binary data)</source>
        <translation>(données binaires)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10024"/>
        <source>Send these messages now?</source>
        <translation>Envoyer ces messages maintenant ?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10034"/>
        <source>Queued for sending.</source>
        <translation>Mis en file d&apos;attente pour l&apos;envoi.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10054"/>
        <source>Invalid message id</source>
        <translation>ID de message invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10063"/>
        <source>usage: mms init &lt;required_signers&gt;/&lt;authorized_signers&gt; &lt;own_label&gt; &lt;own_transport_address&gt;</source>
        <translation>usage: mms init &lt;signataires_requis&gt;/&lt;signataires_autorisés&gt; &lt;mon_étiquette&gt; &lt;mon_adresse_de_transport&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10069"/>
        <source>The MMS is already initialized. Re-initialize by deleting all signer info and messages?</source>
        <translation>MMS est déjà initialisé. Réinitialiser en supprimer toutes les informations des signataires et les messages ?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10084"/>
        <source>Error in the number of required signers and/or authorized signers</source>
        <translation>Erreur dans le nombre de signataires requis et/ou de signataires autorisés</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10101"/>
        <source>The MMS is not active.</source>
        <translation>MMS n&apos;est pas activé.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10124"/>
        <source>Invalid signer number </source>
        <translation>Nombre de signataires invalide </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10129"/>
        <source>mms signer [&lt;number&gt; &lt;label&gt; [&lt;transport_address&gt; [&lt;monero_address&gt;]]]</source>
        <translation>mms signer [&lt;nombre&gt; &lt;étiquette&gt; [&lt;adresse_de_transport&gt; [&lt;adresse_monero&gt;]]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10148"/>
        <source>Invalid Monero address</source>
        <translation>Adresse Monero invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10155"/>
        <source>Wallet state does not allow changing Monero addresses anymore</source>
        <translation>L&apos;état du portefeuille ne permet plus de changer les adresses Monero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10167"/>
        <source>Usage: mms list</source>
        <translation>Usage : mms list</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10180"/>
        <source>Usage: mms next [sync]</source>
        <translation>Usage : mms next [sync]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10205"/>
        <source>No next step: </source>
        <translation>Pas d&apos;étape suivante : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10215"/>
        <source>prepare_multisig</source>
        <translation>prepare_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10221"/>
        <source>make_multisig</source>
        <translation>make_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10236"/>
        <source>exchange_multisig_keys</source>
        <translation>exchange_multisig_keys</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10251"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10371"/>
        <source>export_multisig_info</source>
        <translation>export_multisig_info</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10260"/>
        <source>import_multisig_info</source>
        <translation>import_multisig_info</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10273"/>
        <source>sign_multisig</source>
        <translation>sign_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10283"/>
        <source>submit_multisig</source>
        <translation>submit_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10293"/>
        <source>Send tx</source>
        <translation>Envoyer la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10304"/>
        <source>Process signer config</source>
        <translation>Traiter la configuration de signataire</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10316"/>
        <source>Replace current signer config with the one displayed above?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10330"/>
        <source>Process auto config data</source>
        <translation>Traiter les données d&apos;auto configuration</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10344"/>
        <source>Nothing ready to process</source>
        <translation>Rien n&apos;est prêt à être traité</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10364"/>
        <source>Usage: mms sync</source>
        <translation>Usage : mms sync</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10388"/>
        <source>Usage: mms delete (&lt;message_id&gt; | all)</source>
        <translation>Usage : mms delete (&lt;ID_message&gt; | all)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10395"/>
        <source>Delete all messages?</source>
        <translation>Supprimer tous les messages ?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10421"/>
        <source>Usage: mms send [&lt;message_id&gt;]</source>
        <translation>Usage : mms send [&lt;ID_message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10438"/>
        <source>Usage: mms receive</source>
        <translation>Usage : mms receive</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10455"/>
        <source>Usage: mms export &lt;message_id&gt;</source>
        <translation>Usage : mms export &lt;ID_message&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10467"/>
        <source>Message content saved to: </source>
        <translation>Contenu du message sauvegardé dans : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10471"/>
        <source>Failed to to save message content</source>
        <translation>Échec de la sauvegarde du contenu du message</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10495"/>
        <source>Usage: mms note [&lt;label&gt; &lt;text&gt;]</source>
        <translation>Usage : mms note [&lt;étiquette&gt; &lt;texte&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10502"/>
        <source>No signer found with label </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10524"/>
        <source>Usage: mms show &lt;message_id&gt;</source>
        <translation>Usage : mms show &lt;ID_message&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10543"/>
        <source>Usage: mms set &lt;option_name&gt; [&lt;option_value&gt;]</source>
        <translation>Usage : mms set &lt;nom_option&gt; [&lt;valeur_option&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10560"/>
        <source>Wrong option value</source>
        <translation>Mauvaise valeur d&apos;option</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10565"/>
        <source>Auto-send is on</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10565"/>
        <source>Auto-send is off</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10570"/>
        <source>Unknown option</source>
        <translation>Option inconnue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10578"/>
        <source>Usage: mms help [&lt;subcommand&gt;]</source>
        <translation>Usage : mms help [&lt;sous_commande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10594"/>
        <source>Usage: mms send_signer_config</source>
        <translation>Usage : mms send_signer_config</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10600"/>
        <source>Signer config not yet complete</source>
        <translation>Configuration de signataire pas encore complète</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10615"/>
        <source>Usage: mms start_auto_config [&lt;label&gt; &lt;label&gt; ...]</source>
        <translation>Usage : mms start_auto_config [&lt;étiquette&gt; &lt;étiquette&gt; ...]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10620"/>
        <source>There are signers without a label set. Complete labels before auto-config or specify them as parameters here.</source>
        <translation>Il y a des signataire sans étiquette définie. Définissez les étiquettes avant l&apos;auto configuration ou spécifiez les en paramêtre ici.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10626"/>
        <source>Auto-config is already running. Cancel and restart?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10650"/>
        <source>Usage: mms stop_auto_config</source>
        <translation>Usage : mms stop_auto_config</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10653"/>
        <source>Delete any auto-config tokens and stop auto-config?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10666"/>
        <source>Usage: mms auto_config &lt;auto_config_token&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10673"/>
        <source>Invalid auto-config token</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10679"/>
        <source>Auto-config already running. Cancel and restart?</source>
        <translation>Auto configuration déjà en cours. Annuler et recommencer ?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10697"/>
        <source>MMS not available in this wallet</source>
        <translation>MMS non disponible pour ce portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10721"/>
        <source>The MMS is not active. Activate using the &quot;mms init&quot; command</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10798"/>
        <source>Invalid MMS subcommand</source>
        <translation>Sous-commande MMS invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10803"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10807"/>
        <source>Error in MMS command: </source>
        <translation>Erreur dans la commande MMS : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7162"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>La monnaie réclamée ne va pas à une adresse payée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7167"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>La monnaie réclamée est supérieure au paiement à l&apos;adresse de monnaie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7198"/>
        <source>sending %s to %s</source>
        <translation>envoi de %s à %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7208"/>
        <source> dummy output(s)</source>
        <translation> sortie(s) factice(s)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7211"/>
        <source>with no destinations</source>
        <translation>sans destination</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7252"/>
        <source>This is a multisig wallet, it can only sign with sign_multisig</source>
        <translation>Ceci est un portefeuille multisig, il ne peut signer qu&apos;avec sign_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7275"/>
        <source>Failed to sign transaction</source>
        <translation>Échec de signature de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7281"/>
        <source>Failed to sign transaction: </source>
        <translation>Échec de signature de transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7302"/>
        <source>Transaction raw hex data exported to </source>
        <translation>Données brutes hex de la transaction exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7323"/>
        <source>Failed to load transaction from file</source>
        <translation>Échec du chargement de la transaction du fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5310"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5640"/>
        <source>RPC error: </source>
        <translation>Erreur RPC : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="713"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il n&apos;a pas de clé de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="863"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1047"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1100"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1167"/>
        <source>Your original password was incorrect.</source>
        <translation>Votre mot de passe original est incorrect.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="878"/>
        <source>Error with wallet rewrite: </source>
        <translation>Erreur avec la réécriture du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2456"/>
        <source>invalid unit</source>
        <translation>unité invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2474"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2536"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation>nombre invalide : un entier non signé est attendu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2492"/>
        <source>invalid value</source>
        <translation>valeur invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4169"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4189"/>
        <source>bad m_restore_height parameter: </source>
        <translation>mauvais paramètre m_restore_height : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4133"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4180"/>
        <source>Restore height is: </source>
        <translation>La hauteur de restauration est : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5062"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Le démon est local, supposons qu&apos;il est de confiance</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4797"/>
        <source>Password for new watch-only wallet</source>
        <translation>Mot de passe pour le nouveau portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5320"/>
        <source>internal error: </source>
        <translation>erreur interne : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1635"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5325"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5645"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1561"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1640"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5330"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5650"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6310"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6340"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6466"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6750"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6777"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6990"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7336"/>
        <source>unknown error</source>
        <translation>erreur inconnue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5335"/>
        <source>refresh failed: </source>
        <translation>échec du rafraîchissement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5335"/>
        <source>Blocks received: </source>
        <translation>Blocs reçus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5376"/>
        <source>unlocked balance: </source>
        <translation>solde débloqué : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3447"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3459"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3460"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5483"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5545"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8186"/>
        <source>amount</source>
        <translation>montant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="359"/>
        <source>false</source>
        <translation>faux</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="677"/>
        <source>Unknown command: </source>
        <translation>Commande inconnue : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="684"/>
        <source>Command usage: </source>
        <translation>Usage de la commande : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="687"/>
        <source>Command description: </source>
        <translation>Description de la commande : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="753"/>
        <source>wallet is multisig but not yet finalized</source>
        <translation>le portefeuille est multisig mais pas encore finalisé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="786"/>
        <source>Failed to retrieve seed</source>
        <translation>Échec de la récupération de la phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="816"/>
        <source>wallet is multisig and has no seed</source>
        <translation>le portefeuille est multisig et n&apos;a pas de phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="925"/>
        <source>Error: failed to estimate backlog array size: </source>
        <translation>Erreur : échec de l&apos;estimation de la taille du tableau d&apos;arriéré : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="930"/>
        <source>Error: bad estimated backlog array size</source>
        <translation>Erreur : mauvaise estimation de la taille du tableau d&apos;arriéré</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="942"/>
        <source> (current)</source>
        <translation> (actuel)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="945"/>
        <source>%u block (%u minutes) backlog at priority %u%s</source>
        <translation>arriéré de %u bloc(s) (%u minutes) à la priorité %u%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="947"/>
        <source>%u to %u block (%u to %u minutes) backlog at priority %u</source>
        <translation>arriéré de %u à %u bloc(s) (%u à %u minutes) à la priorité %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="950"/>
        <source>No backlog at priority </source>
        <translation>Pas d&apos;arriéré à la priorité </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="970"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1015"/>
        <source>This wallet is already multisig</source>
        <translation>Le portefeuille est déjà multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="975"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1020"/>
        <source>wallet is watch-only and cannot be made multisig</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas être tranformé en multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="981"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1026"/>
        <source>This wallet has been used before, please use a new wallet to create a multisig wallet</source>
        <translation>Ce portefeuille a été utilisé auparavant, veuillez utiliser un nouveau portefeuille pour créer un portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="989"/>
        <source>Send this multisig info to all other participants, then use make_multisig &lt;threshold&gt; &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Envoyez ces infos multisig à tous les autres participants, ensuite utilisez make_multisig &lt;seuil&gt; &lt;info1&gt; [&lt;info2&gt;...] avec les infos multisig des autres</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="990"/>
        <source>This includes the PRIVATE view key, so needs to be disclosed only to that multisig wallet&apos;s participants </source>
        <translation>Ceci inclut la clé PRIVÉE d&apos;audit, donc ne doit être divulgué qu&apos;aux participants de ce portefeuille multisig </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1040"/>
        <source>Invalid threshold</source>
        <translation>Seuil invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1060"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1182"/>
        <source>Another step is needed</source>
        <translation>Une autre étape est nécessaire</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1072"/>
        <source>Error creating multisig: </source>
        <translation>Erreur de création multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1079"/>
        <source>Error creating multisig: new wallet is not multisig</source>
        <translation>Erreur de création multisig : le nouveau portefeuille n&apos;est pas multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1082"/>
        <source> multisig address: </source>
        <translation> adresse multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1106"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1155"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1222"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1288"/>
        <source>This wallet is not multisig</source>
        <translation>Ce portefeuille n&apos;est pas multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1111"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1160"/>
        <source>This wallet is already finalized</source>
        <translation>Ce portefeuille est déjà finalisé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1127"/>
        <source>Failed to finalize multisig</source>
        <translation>Échec de finalisation multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1133"/>
        <source>Failed to finalize multisig: </source>
        <translation>Échec de finalisation multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1194"/>
        <source>Multisig address: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1227"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1293"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1387"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1503"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1584"/>
        <source>This multisig wallet is not yet finalized</source>
        <translation>Ce portefeuille multisig n&apos;est pas encore finalisé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1263"/>
        <source>Error exporting multisig info: </source>
        <translation>Erreur d&apos;importation des infos multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1267"/>
        <source>Multisig info exported to </source>
        <translation>Infos multisig exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1333"/>
        <source>Multisig info imported</source>
        <translation>Infos multisig importées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1337"/>
        <source>Failed to import multisig info: </source>
        <translation>Échec de l&apos;importation des infos multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1348"/>
        <source>Failed to update spent status after importing multisig info: </source>
        <translation>Échec de la mise à jour de l&apos;état des dépenses après l&apos;importation des infos multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1354"/>
        <source>Untrusted daemon, spent status may be incorrect. Use a trusted daemon and run &quot;rescan_spent&quot;</source>
        <translation>Pas un démon de confiance, l&apos;état des dépenses peut être incorrect. Utilisez un démon de confiance et executez &quot;rescan_spent&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1382"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1498"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1579"/>
        <source>This is not a multisig wallet</source>
        <translation>Ceci n&apos;est pas un portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1432"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1441"/>
        <source>Failed to sign multisig transaction</source>
        <translation>Échec de la signature de la transaction multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1448"/>
        <source>Multisig error: </source>
        <translation>Erreur multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1453"/>
        <source>Failed to sign multisig transaction: </source>
        <translation>Échec de la signature de la transaction multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1476"/>
        <source>It may be relayed to the network with submit_multisig</source>
        <translation>Elle peut être transmise au réseau avec submit_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1535"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1605"/>
        <source>Failed to load multisig transaction from file</source>
        <translation>Échec du chargement de la transaction multisig du fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1541"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1610"/>
        <source>Multisig transaction signed by only %u signers, needs %u more signatures</source>
        <translation>Transaction multisig signée par %u signataire(s) seulement, nécessite %u signature(s) de plus</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1550"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9687"/>
        <source>Transaction successfully submitted, transaction </source>
        <translation>Transaction transmise avec succès, transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1551"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9688"/>
        <source>You can check its status by using the `show_transfers` command.</source>
        <translation>Vous pouvez vérifier son statut en utilisant la commane &apos;show_transfers&apos;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1626"/>
        <source>Failed to export multisig transaction to file </source>
        <translation>Échec de l&apos;exportation de la transaction multisig vers le fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1630"/>
        <source>Saved exported multisig transaction file(s): </source>
        <translation>Transaction multisig enregistrée dans le(s) fichier(s) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1895"/>
        <source>Invalid key image or txid</source>
        <translation>Image de clef ou ID de transaction invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1904"/>
        <source>failed to unset ring</source>
        <translation>échec de la déconstruction du cercle</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2075"/>
        <source>usage: %s &lt;key_image&gt;|&lt;pubkey&gt;</source>
        <translation>usage : %s &lt;image_clef&gt;|&lt;clef_publique&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2120"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2132"/>
        <source>Frozen: </source>
        <translation>Gelé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2134"/>
        <source>Not frozen: </source>
        <translation>Non gelé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2148"/>
        <source> bytes sent</source>
        <translation> octets envoyés</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2149"/>
        <source> bytes received</source>
        <translation> octets reçus</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2155"/>
        <source>Welcome to Monero, the private cryptocurrency.</source>
        <translation>Bienvenue dans Monero, la crypto-monnaie confidentielle.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2157"/>
        <source>Monero, like Bitcoin, is a cryptocurrency. That is, it is digital money.</source>
        <translation>Monero, comme Bitcoin, est une crypto-monnaie. C&apos;est à dire que c&apos;est une monnaie numérique.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2161"/>
        <source>Monero protects your privacy on the blockchain, and while Monero strives to improve all the time,</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2162"/>
        <source>no privacy technology can be 100% perfect, Monero included.</source>
        <translation>aucune technologie de confidentialité n&apos;est parfaite à 100%, Monero inclus.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2163"/>
        <source>Monero cannot protect you from malware, and it may not be as effective as we hope against powerful adversaries.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2164"/>
        <source>Flaws in Monero may be discovered in the future, and attacks may be developed to peek under some</source>
        <translation>Des failles dans Monero pourraient être découvertes dans le futur, et des attaques pourraient être développées pour jeter un coup d&apos;oeil sous certaines</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2165"/>
        <source>of the layers of privacy Monero provides. Be safe and practice defense in depth.</source>
        <translation>des couches de confidentialité que Monero fournit. Soyez prudent et pratiquez la défense en profondeur.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2273"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2279"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2305"/>
        <source>ring size must be an integer &gt;= </source>
        <translation>la taille de cercle doit être un nombre entier &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2310"/>
        <source>could not change default ring size</source>
        <translation>échec du changement de la taille de cercle par défaut</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2570"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2641"/>
        <source>Invalid height</source>
        <translation>Hauteur invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2748"/>
        <source>invalid argument: must be either 1/yes or 0/no</source>
        <translation>argument invalide : doit être 1/yes ou 0/no</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2851"/>
        <source>Start mining in the daemon (bg_mining and ignore_battery are optional booleans).</source>
        <translation>Démarrer la mine dans le démon (mine_arrière_plan et ignorer_batterie sont des booléens facultatifs).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2854"/>
        <source>Stop mining in the daemon.</source>
        <translation>Arrêter la mine dans le démon.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2858"/>
        <source>Set another daemon to connect to.</source>
        <translation>Spécifier un autre démon auquel se connecter.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2861"/>
        <source>Save the current blockchain data.</source>
        <translation>Sauvegarder les données actuelles de la châine de blocs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2864"/>
        <source>Synchronize the transactions and balance.</source>
        <translation>Synchroniser les transactions et le solde.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2868"/>
        <source>Show the wallet&apos;s balance of the currently selected account.</source>
        <translation>Afficher le solde du compte actuellement sélectionné.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2872"/>
        <source>Show the incoming transfers, all or filtered by availability and address index.

Output format:
Amount, Spent(&quot;T&quot;|&quot;F&quot;), &quot;frozen&quot;|&quot;locked&quot;|&quot;unlocked&quot;, RingCT, Global Index, Transaction Hash, Address Index, [Public Key, Key Image] </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2878"/>
        <source>Show the payments for the given payment IDs.</source>
        <translation>Afficher les paiements pour les IDs de paiement donnés.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2881"/>
        <source>Show the blockchain height.</source>
        <translation>Afficher la hauteur de la chaîne de blocs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2895"/>
        <source>Send all unmixable outputs to yourself with ring_size 1</source>
        <translation>Envoyer toutes les sorties non mélangeables à vous-même avec une taille de cercle de 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2902"/>
        <source>Send all unlocked outputs below the threshold to an address.</source>
        <translation>Envoyer toutes les sorties débloquées d&apos;un montant inférieur au seuil à une adresse.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2906"/>
        <source>Send a single output of the given key image to an address without change.</source>
        <translation>Envoyer une unique sortie ayant une image de clé donnée à une adresse sans rendu de monnaie.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2910"/>
        <source>Donate &lt;amount&gt; to the development team (donate.getmonero.org).</source>
        <translation>Donner &lt;montant&gt; à l&apos;équipe de développement (donate.getmonero.org).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2917"/>
        <source>Submit a signed transaction from a file.</source>
        <translation>Transmettre une transaction signée d&apos;un fichier.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2921"/>
        <source>Change the current log detail (level must be &lt;0-4&gt;).</source>
        <translation>Changer le niveau de détail du journal (le niveau doit être &lt;0-4&gt;).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2925"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="2939"/>
        <source>Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation>Encoder un ID de paiement dans une adresse intégrée pour l&apos;adresse publique du portefeuille actuel (en l&apos;absence d&apos;argument un ID de paiement aléatoire est utilisé), ou décoder une adresse intégrée en une adresse standard et un ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2943"/>
        <source>Print all entries in the address book, optionally adding/deleting an entry to/from it.</source>
        <translation>Afficher toutes les entrées du carnet d&apos;adresses, optionnellement en y ajoutant/supprimant une entrée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2946"/>
        <source>Save the wallet data.</source>
        <translation>Sauvegarder les données du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2949"/>
        <source>Save a watch-only keys file.</source>
        <translation>Sauvegarder un fichier de clés d&apos;audit.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2952"/>
        <source>Display the private view key.</source>
        <translation>Afficher la clé privée d&apos;audit.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2955"/>
        <source>Display the private spend key.</source>
        <translation>Afficher la clé privée de dépense.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2958"/>
        <source>Display the Electrum-style mnemonic seed</source>
        <translation>Afficher la phrase mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3028"/>
        <source>Display the encrypted Electrum-style mnemonic seed.</source>
        <translation>Afficher la phrase mnémonique de style Electrum chiffrée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3031"/>
        <source>Rescan the blockchain for spent outputs.</source>
        <translation>Rescanner la chaîne de blocs pour trouver les sorties dépensées.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3035"/>
        <source>Get the transaction key (r) for a given &lt;txid&gt;.</source>
        <translation>Obtenir la clé de transaction (r) pour un &lt;ID_transaction&gt; donné.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3043"/>
        <source>Check the amount going to &lt;address&gt; in &lt;txid&gt;.</source>
        <translation>Vérifier le montant allant à &lt;adresse&gt; dans &lt;ID_transaction&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3047"/>
        <source>Generate a signature proving funds sent to &lt;address&gt; in &lt;txid&gt;, optionally with a challenge string &lt;message&gt;, using either the transaction secret key (when &lt;address&gt; is not your wallet&apos;s address) or the view secret key (otherwise), which does not disclose the secret key.</source>
        <translation>Générer une signature prouvant l&apos;envoi de fonds à &lt;adresse&gt; dans &lt;ID_transaction&gt;, optionnellement avec un &lt;message&gt; comme challenge, en utilisant soit la clé secrète de transaction (quand &lt;adresse&gt; n&apos;est pas l&apos;adresse de votre portefeuille) soit la clé secrète d&apos;audit (dans le cas contraire), tout en ne divulguant pas la clé secrète.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3051"/>
        <source>Check the proof for funds going to &lt;address&gt; in &lt;txid&gt; with the challenge string &lt;message&gt; if any.</source>
        <translation>Vérifier la validité de la preuve de fonds allant à &lt;adresse&gt; dans &lt;ID_transaction&gt; avec le &lt;message&gt; de challenge s&apos;il y en a un.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3055"/>
        <source>Generate a signature proving that you generated &lt;txid&gt; using the spend secret key, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Générer une signature prouvant que vous avez créé &lt;ID_transaction&gt; en utilisant la clé secrète de dépense, optionnellement avec un &lt;message&gt; comme challenge.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3059"/>
        <source>Check a signature proving that the signer generated &lt;txid&gt;, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Vérifier la validité de la preuve que le signataire a créé &lt;ID_transaction&gt;, optionnellement avec un &lt;message&gt; comme challenge.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3063"/>
        <source>Generate a signature proving that you own at least this much, optionally with a challenge string &lt;message&gt;.
If &apos;all&apos; is specified, you prove the entire sum of all of your existing accounts&apos; balances.
Otherwise, you prove the reserve of the smallest possible amount above &lt;amount&gt; available in your current account.</source>
        <translation>Générer une signature prouvant que vous possédez au moins ce montant, optionnellement avec un &lt;message&gt; comme challenge.
Si &apos;all&apos; est spécifié, vous prouvez la somme totale des soldes de tous vos comptes existants.
Sinon, vous prouvez le plus petit solde supérieur à &lt;montant&gt; dans votre compte actuel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3069"/>
        <source>Check a signature proving that the owner of &lt;address&gt; holds at least this much, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Vérifier la validité d&apos;une signature prouvant que le propriétaire d&apos;une &lt;adresse&gt; détient au moins un montant, optionnellement avec un &lt;message&gt; comme challenge.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3089"/>
        <source>Show the unspent outputs of a specified address within an optional amount range.</source>
        <translation>Afficher les sorties non dépensées d&apos;une adresse spécifique dans un interval de montants facultatif.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3093"/>
        <source>Rescan the blockchain from scratch. If &quot;hard&quot; is specified, you will lose any information which can not be recovered from the blockchain itself.</source>
        <translation>Rescanner la chaîne de blocs depuis le début. Si &quot;hard&quot; est spécifié, vous perdrez toute information qui ne peut être récupérée à partir de la chaîne de blocs elle-même.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3097"/>
        <source>Set an arbitrary string note for a &lt;txid&gt;.</source>
        <translation>Définir un texte arbitraire comme note pour &lt;ID_transaction&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3101"/>
        <source>Get a string note for a txid.</source>
        <translation>Obtenir le texte de la note pour un ID de transaction.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3105"/>
        <source>Set an arbitrary description for the wallet.</source>
        <translation>Définir un texte arbitraire comme description du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3109"/>
        <source>Get the description of the wallet.</source>
        <translation>Obtenir la description du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3112"/>
        <source>Show the wallet&apos;s status.</source>
        <translation>Afficher l&apos;état du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3115"/>
        <source>Show the wallet&apos;s information.</source>
        <translation>Afficher les informations du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3119"/>
        <source>Sign the contents of a file.</source>
        <translation>Signer le contenu d&apos;un fichier.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3123"/>
        <source>Verify a signature on the contents of a file.</source>
        <translation>Vérifier la signature du contenu d&apos;un fichier.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3131"/>
        <source>Import a signed key images list and verify their spent status.</source>
        <translation>Importer un ensemble signé d&apos;images de clé et vérifier si elles correspondent à des dépenses.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3143"/>
        <source>Export a set of outputs owned by this wallet.</source>
        <translation>Exporter un ensemble de sorties possédées par ce portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3147"/>
        <source>Import a set of outputs owned by this wallet.</source>
        <translation>Importer un ensemble de sorties possédées par ce portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3151"/>
        <source>Show information about a transfer to/from this address.</source>
        <translation>Afficher les information à propos d&apos;un transfert vers/depuis cette adresse.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3154"/>
        <source>Change the wallet&apos;s password.</source>
        <translation>Changer le mot de passe du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3161"/>
        <source>Print the information about the current fee and transaction backlog.</source>
        <translation>Afficher les informations à propos des frais et arriéré de transactions actuels.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3163"/>
        <source>Export data needed to create a multisig wallet</source>
        <translation>Exporter les données nécessaires pour créer un portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3166"/>
        <source>Turn this wallet into a multisig wallet</source>
        <translation>Transformer ce portefeuille en portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3170"/>
        <source>Turn this wallet into a multisig wallet, extra step for N-1/N wallets</source>
        <translation>Transformer ce portefeuille en portefeuille multisig, étape supplémentaire pour les portefeuilles N-1/N</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3178"/>
        <source>Export multisig info for other participants</source>
        <translation>Exporter les infos multisig pour les autres participants</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3182"/>
        <source>Import multisig info from other participants</source>
        <translation>Importer les infos multisig des autres participants</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3186"/>
        <source>Sign a multisig transaction from a file</source>
        <translation>Signer une transaction multisig d&apos;un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3190"/>
        <source>Submit a signed multisig transaction from a file</source>
        <translation>Transmettre une transaction multisig signée d&apos;un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3194"/>
        <source>Export a signed multisig transaction to a file</source>
        <translation>Exporter une transaction multisig signée vers un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3291"/>
        <source>Unsets the ring used for a given key image or transaction</source>
        <translation>Déconstruire le cercle utilisé pour une image de clé ou une transaction donnée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3311"/>
        <source>Freeze a single output by key image so it will not be used</source>
        <translation>Geler une sortie par image de clé pour qu&apos;elle ne soit pas utilisée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3315"/>
        <source>Thaw a single output by key image so it may be used again</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3319"/>
        <source>Checks whether a given output is currently frozen by key image</source>
        <translation>Vérifie si une sortie est actuellement gelée par image de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3327"/>
        <source>Prints simple network stats</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3331"/>
        <source>Prints basic info about Monero for first time users</source>
        <translation>Affiche des informations basiques à propos de Monero pour les nouveaux utilisateurs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3339"/>
        <source>Show the help section or the documentation about a &lt;command&gt;.</source>
        <translation>Afficher la section d&apos;aide ou la documentation d&apos;une &lt;commande&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3440"/>
        <source>integer &gt;= </source>
        <translation>entier &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3452"/>
        <source>block height</source>
        <translation>hauteur de bloc</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3463"/>
        <source>1/yes or 0/no</source>
        <translation>1/yes ou 0/no</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3562"/>
        <source>No wallet found with that name. Confirm creation of new wallet named: </source>
        <translation>Aucun portefeuille avec ce nom trouvé. Confirmer la création d&apos;un nouveau portefeuille nommé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3688"/>
        <source>can&apos;t specify both --restore-deterministic-wallet or --restore-multisig-wallet and --non-deterministic</source>
        <translation>impossible de spécifier à la fois --restore-deterministic-wallet ou --restore-multisig-wallet et --non-deterministic</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3694"/>
        <source>--restore-multisig-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-multisig-wallet utilise --generate-new-wallet, pas --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3710"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;multisig seed here&quot;</source>
        <translation>spécifiez un paramètre de récupération avec --electrum-seed=&quot;phrase mnémonique multisig ici&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3739"/>
        <source>Multisig seed failed verification</source>
        <translation>Échec de la vérification de la phrase mnémonique multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3789"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3866"/>
        <source>This address is a subaddress which cannot be used here.</source>
        <translation>Cette adresse est une sous-adresse qui ne peut pas être utilisée ici.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3944"/>
        <source>Error: expected M/N, but got: </source>
        <translation>Erreur : M/N attendu, mais lu : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3949"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got: </source>
        <translation>Erreur : N &gt; 1 et N &lt;= M attendu, mais lu : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3954"/>
        <source>Error: M/N is currently unsupported. </source>
        <translation>Erreur : M/N n&apos;est actuellement pas supporté. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3957"/>
        <source>Generating master wallet from %u of %u multisig wallet keys</source>
        <translation>Génération du portefeuille principal à partir de %u de %u clés de portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3986"/>
        <source>failed to parse secret view key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3994"/>
        <source>failed to verify secret view key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4037"/>
        <source>Error: M/N is currently unsupported</source>
        <translation>Erreur : M/N n&apos;est actuellement pas supporté</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4203"/>
        <source>Restore height </source>
        <translation>Hauteur de restauration </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4258"/>
        <source>If you are new to Monero, type &quot;welcome&quot; for a brief overview.</source>
        <translation>Si vous êtes nouveau dans Monero, tapez &quot;welcome&quot; pour un bref aperçu.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4322"/>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or change the daemon address using the &apos;set_daemon&apos; command.</source>
        <translation>Le démon n&apos;est pas lancé ou un mauvais port a été fourni. Veuillez vous assurer que le démon fonctionne ou changez l&apos;adresse de démon avec la commande &apos;set_daemon&apos;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4399"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4496"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4544"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4587"/>
        <source>Error creating wallet: </source>
        <translation>Erreur lors de la création du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4472"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="4622"/>
        <source>failed to generate new mutlisig wallet</source>
        <translation>échec de la génération du nouveau portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4625"/>
        <source>Generated new %u/%u multisig wallet: </source>
        <translation>Nouveau portefeuille multisig %u/%u généré : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4674"/>
        <source>Opened %u/%u multisig wallet%s</source>
        <translation>Portefeuille multisig %u/%u ouvert%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4735"/>
        <source>Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
</source>
        <translation>Utilisez &quot;help &lt;commande&gt;&quot; pour voir la documentation d&apos;une commande.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4793"/>
        <source>wallet is multisig and cannot save a watch-only version</source>
        <translation>c&apos;est un portefeuille multisig et il ne peut pas sauvegarder une version d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4829"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4861"/>
        <source>Failed to query mining status: </source>
        <translation>Échec de la requête du statut de l&apos;extraction minière : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4844"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4872"/>
        <source>Failed to setup background mining: </source>
        <translation>Échec de la configuration de l&apos;extraction minière en arrière-plan : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4848"/>
        <source>Background mining enabled. Thank you for supporting the Monero network.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4876"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4884"/>
        <source>Background mining not enabled. Run &quot;set setup-background-mining 1&quot; to change.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4890"/>
        <source>Using an untrusted daemon, skipping background mining check</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4915"/>
        <source>The daemon is not set up to background mine.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4916"/>
        <source>With background mining enabled, the daemon will mine when idle and not on batttery.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4917"/>
        <source>Enabling this supports the network you are using, and makes you eligible for receiving new monero</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4922"/>
        <source>Background mining not enabled. Set setup-background-mining to 1 to change.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5029"/>
        <source>Unexpected array length - Exited simple_wallet::set_daemon()</source>
        <translation>Taille de tableau inattendue - Sortie de simple_wallet::set_daemon()</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5070"/>
        <source>This does not seem to be a valid daemon URL.</source>
        <translation>Ceci semble ne pas être une URL de démon valide.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5110"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5156"/>
        <source>txid </source>
        <translation>ID transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5112"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5158"/>
        <source>idx </source>
        <translation>index </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5136"/>
        <source>NOTE: This transaction is locked, see details with: show_transfer </source>
        <translation>NOTE : Cette transaction est verrouillée, voir les détails avec : show_transfer </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5286"/>
        <source>New transfer received since rescan was started. Key images are incomplete.</source>
        <translation>Nouveau transfert reçu depuis que l&apos;analyse a été lancée. Les images de clé sont incomplètes.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5364"/>
        <source> (Some owned outputs have partial key images - import_multisig_info needed)</source>
        <translation> (Certaines sorties ont des images de clé partielles - import_multisig_info requis)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5367"/>
        <source>Currently selected account: [</source>
        <translation>Compte actuellement sélectionné : [</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5367"/>
        <source>] </source>
        <translation>] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5369"/>
        <source>Tag: </source>
        <translation>Mot clé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5369"/>
        <source>(No tag assigned)</source>
        <translation>(Pas de mot clé assigné)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5381"/>
        <source>Balance per address:</source>
        <translation>Solde par adresse :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5382"/>
        <source>Address</source>
        <translation>Adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5382"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8746"/>
        <source>Balance</source>
        <translation>Solde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5382"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8746"/>
        <source>Unlocked balance</source>
        <translation>Solde débloqué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5382"/>
        <source>Outputs</source>
        <translation>Sorties</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5382"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8746"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9922"/>
        <source>Label</source>
        <translation>Étiquette</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5390"/>
        <source>%8u %6s %21s %21s %7u %21s</source>
        <translation>%8u %6s %21s %21s %7u %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5483"/>
        <source>spent</source>
        <translation>dépensé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5483"/>
        <source>global index</source>
        <translation>index global</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5483"/>
        <source>tx id</source>
        <translation>ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5483"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5545"/>
        <source>addr index</source>
        <translation>index adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5493"/>
        <source>Used at heights: </source>
        <translation>Utilisé aux hauteurs : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5499"/>
        <source>[frozen]</source>
        <translation>[gelé]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5513"/>
        <source>No incoming transfers</source>
        <translation>Aucun transfert entrant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5517"/>
        <source>No incoming available transfers</source>
        <translation>Aucun transfert entrant disponible</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5521"/>
        <source>No incoming unavailable transfers</source>
        <translation>Aucun transfert entrant non disponible</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5545"/>
        <source>payment</source>
        <translation>paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5545"/>
        <source>transaction</source>
        <translation>transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5545"/>
        <source>height</source>
        <translation>hauteur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5545"/>
        <source>unlock time</source>
        <translation>durée de déverrouillage</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5557"/>
        <source>No payments with id </source>
        <translation>Aucun paiement avec l&apos;ID </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5605"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5695"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6105"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6570"/>
        <source>failed to get blockchain height: </source>
        <translation>échec de la récupération de la hauteur de la chaîne de blocs : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5703"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation>
Transaction %llu/%llu : ID=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5740"/>
        <source>failed to get output: </source>
        <translation>échec de la récupération de la sortie : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5748"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation>la hauteur du bloc d&apos;origine de la clé de la sortie ne devrait pas être supérieure à celle de la chaîne de blocs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5752"/>
        <source>
Originating block heights: </source>
        <translation>
Hauteurs des blocs d&apos;origine : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5764"/>
        <source>
|</source>
        <translation>
|</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5764"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8374"/>
        <source>|
</source>
        <translation>|
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5781"/>
        <source>
Warning: Some input keys being spent are from </source>
        <translation>
Attention : Certaines clés d&apos;entrées étant dépensées sont issues de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5783"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation>, ce qui peut casser l&apos;anonymat du cercle de signature. Assurez-vous que c&apos;est intentionnel !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5814"/>
        <source>Transaction spends more than one very old output. Privacy would be better if they were sent separately.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5815"/>
        <source>Spend them now anyway?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5918"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6522"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6804"/>
        <source>Ring size must not be 0</source>
        <translation>La taille de cercle ne doit pas être 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5930"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6534"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6816"/>
        <source>ring size %u is too small, minimum is %u</source>
        <translation>la taille de cercle %u est trop petite, le minimum est %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5942"/>
        <source>wrong number of arguments</source>
        <translation>mauvais nombre d&apos;arguments</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5962"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6072"/>
        <source>Warning: Unencrypted payment IDs will harm your privacy: ask the recipient to use subaddresses instead</source>
        <translation>Attention : les IDs de paiement non chiffrés vont nuire à votre confidentialité : demandez au destinataire d&apos;utiliser des sous-adresses à la place</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6121"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6661"/>
        <source>No outputs found, or daemon is not ready</source>
        <translation>Aucune sortie trouvée, ou le démon n&apos;est pas prêt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6230"/>
        <source>.
This transaction (including %s change) will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation> 
Cette transaction (%s de monnaie rendue inclue) sera déverrouillée au bloc %llu, dans approximativement %s jours (en supposant 2 minutes par bloc)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7059"/>
        <source>Failed to parse donation address: </source>
        <translation>Échec de l&apos;analyse de l&apos;adresse de don : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7075"/>
        <source>Donating %s %s to %s.</source>
        <translation>Don de %s %s à %s.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8161"/>
        <source>usage: export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;path&gt;]</source>
        <translation>usage: export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;hauteur_min&gt; [&lt;hauteur_max&gt;]] [output=&lt;chemin&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8186"/>
        <source>direction</source>
        <translation>direction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8186"/>
        <source>timestamp</source>
        <translation>horodatage</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8186"/>
        <source>running balance</source>
        <translation>solde courant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8186"/>
        <source>hash</source>
        <translation>hachage</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8186"/>
        <source>payment ID</source>
        <translation>ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8186"/>
        <source>fee</source>
        <translation>frais</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8186"/>
        <source>destination</source>
        <translation>destination</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8186"/>
        <source>index</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8186"/>
        <source>note</source>
        <translation>note</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8240"/>
        <source>CSV exported to </source>
        <translation>CSV exporté vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8436"/>
        <source>Warning: your restore height is higher than wallet restore height: </source>
        <translation>Attention : votre hauteur de restauration est supérieure à celle du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8437"/>
        <source>Rescan anyway ? (Y/Yes/N/No): </source>
        <translation>Rescanner quand même ? (Y/Yes/N/No) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8456"/>
        <source>MMS received new message</source>
        <translation>MMS a reçu un nouveau message</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8873"/>
        <source>&lt;index&gt; is out of bounds</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9347"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9393"/>
        <source>command only supported by HW wallet</source>
        <translation>commande supportée uniquement par un portefeuille matériel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9352"/>
        <source>hw wallet does not support cold KI sync</source>
        <translation>le portefeuille matériel ne supporte pas la synchronisation des images de clé à froid</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9364"/>
        <source>Please confirm the key image sync on the device</source>
        <translation>Veuillez confirmer la synchronisation des images de clé sur le périphérique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9370"/>
        <source>Key images synchronized to height </source>
        <translation>Images de clé synchronisées à la hauteur </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9373"/>
        <source>Running untrusted daemon, cannot determine which transaction output is spent. Use a trusted daemon with --trusted-daemon and run rescan_spent</source>
        <translation>Utilisation d&apos;un démon dans lequel on n&apos;a pas entièrement confiance, impossible de déterminer quelle sortie de transaction est dépensée. Utilisez un démon de confiance avec --trusted-daemon et exécutez rescan_spent</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9376"/>
        <source> spent, </source>
        <translation> dépensé, </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9376"/>
        <source> unspent</source>
        <translation> non dépensé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9380"/>
        <source>Failed to import key images</source>
        <translation>Échec de l&apos;importation des images de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9385"/>
        <source>Failed to import key images: </source>
        <translation>Échec de l&apos;import des images de clé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9402"/>
        <source>Failed to reconnect device</source>
        <translation>Échec de la reconnexion à l&apos;appareil</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9407"/>
        <source>Failed to reconnect device: </source>
        <translation>Échec de la reconnexion à l&apos;appareil : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9680"/>
        <source>Transaction successfully saved to </source>
        <translation>Transaction sauvegardée avec succès dans </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9680"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9682"/>
        <source>, txid </source>
        <translation>, ID transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9682"/>
        <source>Failed to save transaction to </source>
        <translation>Échec de la sauvegarde de la transaction dans </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7257"/>
        <source>This is a watch only wallet</source>
        <translation>Ceci est un portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9610"/>
        <source>Double spend seen on the network: this transaction may or may not end up being mined</source>
        <translation>Double dépense détectée sur le réseau : cette transaction sera peut-être invalidée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9645"/>
        <source>Transaction ID not found</source>
        <translation>ID de transaction non trouvé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="354"/>
        <source>true</source>
        <translation>vrai</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="407"/>
        <source>failed to parse refresh type</source>
        <translation>échec de l&apos;analyse du type de rafraîchissement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="739"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="811"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="965"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1010"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1093"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1150"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1217"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1283"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1377"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1493"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1574"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7247"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7311"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7348"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7653"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7737"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9180"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9257"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9310"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9418"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9467"/>
        <source>command not supported by HW wallet</source>
        <translation>commande non supportée par le portefeuille matériel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="744"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="821"/>
        <source>wallet is watch-only and has no seed</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il n&apos;a pas de phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="762"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="831"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation>c&apos;est un portefeuille non déterministe et il n&apos;a pas de phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="769"/>
        <source>Enter optional seed offset passphrase, empty to see raw seed</source>
        <translation>Entrer une phrase de passe facultative pour le décalage de la phrase mnémonique, effacer pour voir la phrase mnémonique brute</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="841"/>
        <source>Incorrect password</source>
        <translation>Mot de passe invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="909"/>
        <source>Current fee is %s %s per %s</source>
        <translation>Les frais sont actuellement de %s %s par %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1658"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1815"/>
        <source>Invalid key image</source>
        <translation>Image de clé invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1664"/>
        <source>Invalid txid</source>
        <translation>ID de transaction invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1676"/>
        <source>Key image either not spent, or spent with mixin 0</source>
        <translation>Image de clé soit non dépensée, soit dépensée avec 0 mélange</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1691"/>
        <source>Failed to get key image ring: </source>
        <translation>Échec de la récupération du cercle de l&apos;image de clé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1706"/>
        <source>File doesn&apos;t exist</source>
        <translation>Le fichier d&apos;existe pas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1728"/>
        <source>Invalid ring specification: </source>
        <translation>Spécification de cercle invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1736"/>
        <source>Invalid key image: </source>
        <translation>Image de clé invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1741"/>
        <source>Invalid ring type, expected relative or abosolute: </source>
        <translation>Type de cercle invalide, &quot;relative&quot; ou &quot;absolute&quot; attendu : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1747"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1759"/>
        <source>Error reading line: </source>
        <translation>Erreur lors de la lecture de la ligne : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1770"/>
        <source>Invalid ring: </source>
        <translation>Cercle invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1779"/>
        <source>Invalid relative ring: </source>
        <translation>Cercle relatif invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1791"/>
        <source>Invalid absolute ring: </source>
        <translation>Cercle absolu invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1800"/>
        <source>Failed to set ring for key image: </source>
        <translation>Échec de l&apos;affectation du cercle pour l&apos;image de clé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1800"/>
        <source>Continuing.</source>
        <translation>On continue.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1830"/>
        <source>Missing absolute or relative keyword</source>
        <translation>Mot clé &quot;absolute&quot; ou &quot;relative&quot; manquant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1840"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1847"/>
        <source>invalid index: must be a strictly positive unsigned integer</source>
        <translation>index invalide : doit être un nombre entier strictement positif</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1855"/>
        <source>invalid index: indices wrap</source>
        <translation>index invalide : boucle des indices</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1865"/>
        <source>invalid index: indices should be in strictly ascending order</source>
        <translation>index invalide : les indices doivent être en ordre strictement croissant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1872"/>
        <source>failed to set ring</source>
        <translation>échec de l&apos;affectation du cercle</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1949"/>
        <source>First line is not an amount</source>
        <translation>La première ligne n&apos;est pas un montant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1963"/>
        <source>Invalid output: </source>
        <translation>Sortie invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1973"/>
        <source>Bad argument: </source>
        <translation>Mauvais argument : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1973"/>
        <source>should be &quot;add&quot;</source>
        <translation>devrait être &quot;add&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1982"/>
        <source>Failed to open file</source>
        <translation>Échec de l&apos;ouverture du fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1988"/>
        <source>Invalid output key, and file doesn&apos;t exist</source>
        <translation>Clé de sortie invalide, et le fichier n&apos;existe pas</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2011"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2038"/>
        <source>Invalid output</source>
        <translation>Sortie invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2066"/>
        <source>Failed to save known rings: </source>
        <translation>Échec de la sauvegarde des cercles connus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2181"/>
        <source>Unknown command &apos;%s&apos;, try &apos;help&apos;</source>
        <translation>Commande &apos;%s&apos; inconnue, essayez &apos;help&apos;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2247"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2266"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas transférer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2291"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6252"/>
        <source>WARNING: this is a non default ring size, which may harm your privacy. Default is recommended.</source>
        <translation>ATTENTION : ceci c&apos;est pas la taille de cercle par défaut, ce qui peut nuire à votre confidentialité. La valeur par défaut est recommandée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2287"/>
        <source>WARNING: from v8, ring size will be fixed and this setting will be ignored.</source>
        <translation>ATTENTION : ) partir de v8, la taille de cercle sera fixée et ce paramètre sera ignoré.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2322"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2345"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2361"/>
        <source>priority must be either 0, 1, 2, 3, or 4, or one of: </source>
        <translation>la priorité doit être 0, 1, 2, 3, 4 ou l&apos;une de : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2366"/>
        <source>could not change default priority</source>
        <translation>échec du changement de la priorité par défaut</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2421"/>
        <source>invalid argument: must be either 0/never, 1/action, or 2/encrypt/decrypt</source>
        <translation>argument invalide : doit être soit 0/never, 1/action, ou 2/encrypt/decrypt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2716"/>
        <source>Inactivity lock timeout disabled on Windows</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2730"/>
        <source>Invalid number of seconds</source>
        <translation>Nombre de secondes invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2792"/>
        <source>Export format not specified</source>
        <translation>Format d&apos;export non spécifié</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2806"/>
        <source>Export format not recognized.</source>
        <translation>Format d&apos;export non reconnu.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2884"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt;. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Transférer &lt;montant&gt; à &lt;adresse&gt; Si le paramètre &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille utilise les sorties reçues par les adresses de ces indices. Si il est omis, le portefeuille choisit les indices d&apos;adresse à utiliser aléatoirement. Dans tous les cas, il essaye de ne pas combiner des sorties de multiples adresses. &lt;priorité&gt; est la priorité de la transaction. Plus la priorité est haute, plus les frais de transaction sont élevés. Les valeurs valides par ordre de priorité (de la plus basse à la plus haute) sont : unimportant, normal, elevated, priority. Si elle est omise, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;taille_cercle&gt; est le nombre d&apos;entrées à inclure pour l&apos;intraçabilité. De multiples paiements peuvent être réalisés d&apos;un coup en ajoutant &lt;URI_2&gt; ou &lt;adresse_2&gt; &lt;montant_2&gt; et cetera (avant l&apos;ID de paiement, si il est inclus)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2888"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Transférer &lt;montant&gt; à &lt;adresse&gt; et le verrouiller pendant &lt;blocs_verrou&gt; (max 1000000). Si le paramètre &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille utilise les sorties reçues par les adresses de ces indices. Si il est omis, le portefeuille choisit les indices d&apos;adresse à utiliser aléatoirement. Dans tous les cas, il essaye de ne pas combiner des sorties de multiples adresses. &lt;priorité&gt; est la priorité de la transaction. Plus la priorité est haute, plus les frais de transaction sont élevés. Les valeurs valides par ordre de priorité (de la plus basse à la plus haute) sont : unimportant, normal, elevated, priority. Si elle est omise, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;taille_cercle&gt; est le nombre d&apos;entrées à inclure pour l&apos;intraçabilité. De multiples paiements peuvent être réalisés d&apos;un coup en ajoutant &lt;URI_2&gt; ou &lt;adresse_2&gt; &lt;montant_2&gt; et cetera (avant l&apos;ID de paiement, si il est inclus)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2892"/>
        <source>Send all unlocked balance to an address and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. &lt;priority&gt; is the priority of the sweep. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability.</source>
        <translation>Transférer tout le solde débloqué à une adresse et le verrouiller pendant &lt;blocs_verrou&gt; (max 1000000). Si le paramètre &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille utilise les sorties reçues par ces indices d&apos;adresse. Si il est omis, le portefeuille choisit un index d&apos;adresse à utiliser aléatoirement. &lt;priorité&gt; est la priorité du balayage. Plus la priorité est haute, plus les frais de transaction sont élevés. Les valeurs valides par ordre de priorité (de la plus basse à la plus haute) sont : unimportant, normal, elevated, priority. Si elle est omise, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;taille_cercle&gt; est le nombre d&apos;entrées à inclure pour l&apos;intraçabilité.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2898"/>
        <source>Send all unlocked balance to an address. If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. If the parameter &quot;outputs=&lt;N&gt;&quot; is specified and  N &gt; 0, wallet splits the transaction into N even outputs.</source>
        <translation>Envoyer tout le solde débloqué à une adresse. Si le paramètre &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille balaye les sorties reçues par ces indices d&apos;adresse. Si il est omis, le portefeuille choisit un index d&apos;adresse à utiliser aléatoirement. Si le paramètre &quot;outputs=&lt;N&gt;&quot; est spécifié et N &gt; 0, le portefeuille scinde la transaction en N sorties égales.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2914"/>
        <source>Sign a transaction from a file. If the parameter &quot;export_raw&quot; is specified, transaction raw hex data suitable for the daemon RPC /sendrawtransaction is exported.</source>
        <translation>Signer une transaction à partir d&apos;un fichier. Si le paramètre &quot;export_raw&quot; est spécifié, les données brutes hexadécimales adaptées au RPC /sendrawtransaction du démon sont exportées.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2935"/>
        <source>If no arguments are specified or &lt;index&gt; is specified, the wallet shows the default or specified address. If &quot;all&quot; is specified, the wallet shows all the existing addresses in the currently selected account. If &quot;new &quot; is specified, the wallet creates a new address with the provided label text (which can be empty). If &quot;label&quot; is specified, the wallet sets the label of the address specified by &lt;index&gt; to the provided label text.</source>
        <translation>Si aucun argument n&apos;est spécifié ou si &lt;index&gt; est spécifié, le portefeuille affiche l&apos;adresse par défaut ou l&apos;adresse spécifiée. Si &quot;all&quot; est spécifié, le portefeuille affiche toutes les adresses existantes dans le comptes actuellement sélectionné. Si &quot;new&quot; est spécifié, le portefeuille crée une nouvelle adresse avec le texte d&apos;étiquette fourni (qui peut être vide). Si &quot;label&quot; est spécifié, le portefeuille affecte le texte fourni à l&apos;étiquette de l&apos;adresse spécifiée par &lt;index&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2961"/>
        <source>Display the restore height</source>
        <translation>Afficher la hauteur de restauration</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3039"/>
        <source>Set the transaction key (r) for a given &lt;txid&gt; in case the tx was made by some other device or 3rd party wallet.</source>
        <translation>Définir la clé de transaction (r) pour un &lt;ID_transaction&gt; donné au cas où cette clé ait été créée par un appareil ou portefeuille tiers.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3139"/>
        <source>Attempts to reconnect HW wallet.</source>
        <translation>Essayer de se reconnecter à un portefeuille matériel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3281"/>
        <source>Print the ring(s) used to spend a given key image or transaction (if the ring size is &gt; 1)

Output format:
Key Image, &quot;absolute&quot;, list of rings</source>
        <translation>Afficher le(s) cercle(s) utilisé(s) pour dépenser une image de clé ou une transaction (si la taille de cercle est &gt; 1)

Format de sortie :
Image de clé, &quot;absolue&quot;, liste de cercles</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3287"/>
        <source>Set the ring used for a given key image, so it can be reused in a fork</source>
        <translation>Définir le cercle utilisé pour une image de clé donnée, afin de pouvoir le réutiliser dans un fork</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3295"/>
        <source>Save known rings to the shared rings database</source>
        <translation>Sauvegarder les cercles connus dans la base de données des cercles partagés</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3323"/>
        <source>Lock the wallet console, requiring the wallet password to continue</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3335"/>
        <source>Returns version information</source>
        <translation>Retourne les informations de version</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3442"/>
        <source>full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)</source>
        <translation>full (le plus lent, aucune supposition); optimize-coinbase (rapide, suppose que la récompense de bloc est payée à une unique adresse); no-coinbase (le plus rapide, suppose que l&apos;on ne reçoit aucune récompense de bloc), default (comme optimize-coinbase)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3443"/>
        <source>0, 1, 2, 3, or 4, or one of </source>
        <translation>0, 1, 2, 3, 4 ou l&apos;une de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3444"/>
        <source>0|1|2 (or never|action|decrypt)</source>
        <translation>0|1|2 (ou never|action|decrypt)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3445"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation>monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3456"/>
        <source>&lt;major&gt;:&lt;minor&gt;</source>
        <translation>&lt;majeur&gt;:&lt;mineur&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3462"/>
        <source>unsigned integer (seconds, 0 to disable)</source>
        <translation>entier non signé (secondes, 0 pour désactiver)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3465"/>
        <source>&quot;binary&quot; or &quot;ascii&quot;</source>
        <translation>&quot;binary&quot; ou &quot;ascii&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3525"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation>Nom de portefeuille non valide. Veuillez réessayer ou utilisez Ctrl-C pour quitter.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3542"/>
        <source>Wallet and key files found, loading...</source>
        <translation>Fichier portefeuille et fichier de clés trouvés, chargement...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3548"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation>Fichier de clés trouvé mais pas le fichier portefeuille. Régénération...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3554"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation>Fichier de clés non trouvé. Échec de l&apos;ouverture du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3573"/>
        <source>Generating new wallet...</source>
        <translation>Génération du nouveau portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3654"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation>Impossible de spécifier plus d&apos;une option parmis --testnet et --stagenet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3669"/>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-device=&quot;wallet_name&quot;</source>
        <translation>impossible de spécifier plus d&apos;une option parmis --generate-new-wallet=&quot;nom_portefeuille&quot;, --wallet-file=&quot;nom_portefeuille&quot;, --generate-from-view-key=&quot;nom_portefeuille&quot;, --generate-from-spend-key=&quot;nom_portefeuille&quot;, --generate-from-keys=&quot;nom_portefeuille&quot;, --generate-from-multisig-keys=&quot;nom_portefeuille&quot;, --generate-from-json=&quot;nom_fichier_json&quot; et --generate-from-device=&quot;nom_portefeuille&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3748"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3753"/>
        <source>Enter seed offset passphrase, empty if none</source>
        <translation>Entrer une phrase de passe pour le décalage de la phrase mnémonique, vide si aucun</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3778"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3798"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3834"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3855"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3875"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3890"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3939"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3964"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3980"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4019"/>
        <source>No data supplied, cancelled</source>
        <translation>Pas de données fournies, annulation</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3784"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3861"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3970"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6046"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6628"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6888"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7461"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7529"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7593"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7797"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8974"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9237"/>
        <source>failed to parse address</source>
        <translation>échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3804"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3896"/>
        <source>failed to parse view key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3813"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3913"/>
        <source>failed to verify view key secret key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3817"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3917"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3999"/>
        <source>view key does not match standard address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3822"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3843"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3921"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4055"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4082"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4114"/>
        <source>account creation failed</source>
        <translation>échec de la création du compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3839"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3881"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4024"/>
        <source>failed to parse spend key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3905"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4044"/>
        <source>failed to verify spend key secret key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3909"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4049"/>
        <source>spend key does not match standard address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4089"/>
        <source>No restore height is specified.</source>
        <translation>Aucune hauteur de restauration n&apos;est spécifiée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4090"/>
        <source>Assumed you are creating a new account, restore will be done from current estimated blockchain height.</source>
        <translation>Nous supposons que vous créez un nouveau compte, la restauration sera faite à partir d&apos;une hauteur de la chaîne de blocs estimée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4095"/>
        <source>account creation aborted</source>
        <translation>création du compte annulée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4217"/>
        <source>can&apos;t specify --subaddress-lookahead and --wallet-file at the same time</source>
        <translation>Impossible de spécifier --subaddress-lookahead et --wallet-file en même temps</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4221"/>
        <source>failed to open account</source>
        <translation>échec de l&apos;ouverture du compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4226"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4944"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4997"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5082"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7497"/>
        <source>wallet is null</source>
        <translation>portefeuille est nul</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4232"/>
        <source>Warning: using an untrusted daemon at %s</source>
        <translation>Attention : utilisation d&apos;un démon dans lequel on n&apos;a pas entièrement confiance à %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4233"/>
        <source>Using a third party daemon can be detrimental to your security and privacy</source>
        <translation>Utiliser le démon d&apos;un tiers peut nuire à votre sécurité et à votre confidentialité</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4236"/>
        <source>Using your own without SSL exposes your RPC traffic to monitoring</source>
        <translation>Utiliser le votre sans SSL expose votre trafic RPC à la surveillance</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4237"/>
        <source>You are strongly encouraged to connect to the Monero network using your own daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4238"/>
        <source>If you or someone you trust are operating this daemon, you can use --trusted-daemon</source>
        <translation>Si vous ou quelqu&apos;un en qui vous avez confiance gère ce démon, vous pouvez utiliser --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4245"/>
        <source>Moreover, a daemon is also less secure when running in bootstrap mode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4249"/>
        <source>Failed to initialize ring database: privacy enhancing features will be inactive</source>
        <translation>Impossible d&apos;initialiser la base de données des cercles : les fonctions d&apos;amélioration de la confidentialité seront inactives</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4351"/>
        <source>If your display freezes, exit blind with ^C, then run again with --use-english-language-names</source>
        <translation>Si votre affichage se bloque, quittez en aveugle avec ^C, puis lancer à nouveau en utilisant --use-english-language-names</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4369"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4374"/>
        <source>invalid language choice entered. Please try again.
</source>
        <translation>choix de langue passé invalide. Veuillez réessayer.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4455"/>
        <source>View key: </source>
        <translation>Clé d&apos;audit : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4570"/>
        <source>Generated new wallet on hw device: </source>
        <translation>Nouveau portefeuille généré sur l&apos;appareil : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4651"/>
        <source>Key file not found. Failed to open wallet</source>
        <translation>Fichier des clés non trouvé. Échec d&apos;ouverture du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4728"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation>Vous pourriez vouloir supprimer le fichier &quot;%s&quot; et réessayer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4756"/>
        <source>failed to deinitialize wallet</source>
        <translation>échec de la désinitialisation du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4809"/>
        <source>Watch only wallet saved as: </source>
        <translation>Portefeuille d&apos;audit sauvegardé vers : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4813"/>
        <source>Failed to save watch only wallet: </source>
        <translation>Échec de la sauvegarde du portefeuille d&apos;audit : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4935"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5613"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9315"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation>cette commande requiert un démon de confiance. Activer avec --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5051"/>
        <source>Expected trusted or untrusted, got </source>
        <translation>&quot;trusted&quot; ou &quot;untrusted&quot; attendu, mais lu </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5068"/>
        <source>trusted</source>
        <translation>de confiance</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5068"/>
        <source>untrusted</source>
        <translation>non fiable</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5092"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation>la chaîne de blocs ne peut pas être sauvegardée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5178"/>
        <source>Password needed (%s) - use the refresh command</source>
        <translation>Mot de passe requis (%s) - utilisez la commande refresh</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5184"/>
        <source>Enter password</source>
        <translation>Entrez le mot de passe</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5301"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5627"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5305"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5631"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5315"/>
        <source>refresh error: </source>
        <translation>erreur du rafraîchissement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5366"/>
        <source> (Some owned outputs have missing key images - import_key_images needed)</source>
        <translation> (Il manque les images de clé de certaines sorties - import_key_images requis)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5375"/>
        <source>Balance: </source>
        <translation>Solde : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5482"/>
        <source>pubkey</source>
        <translation>clé publique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5482"/>
        <source>key image</source>
        <translation>image de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5483"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5499"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8186"/>
        <source>unlocked</source>
        <translation>déverrouillé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5483"/>
        <source>ringct</source>
        <translation>ringct</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5498"/>
        <source>T</source>
        <translation>V</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5498"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5499"/>
        <source>locked</source>
        <translation>vérrouillé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5500"/>
        <source>RingCT</source>
        <translation>RingCT</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5500"/>
        <source>-</source>
        <translation>-</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5579"/>
        <source>payment ID has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5635"/>
        <source>failed to get spent status</source>
        <translation>échec de la récupération du statut de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5719"/>
        <source>failed to find construction data for tx input</source>
        <translation>échec de la recherche des données pour contruire l&apos;entrée de la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5724"/>
        <source>
Input %llu/%llu (%s): amount=%s</source>
        <translation>
Entrée %llu/%llu (%s): montant=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5782"/>
        <source>the same transaction</source>
        <translation>la même transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5782"/>
        <source>blocks that are temporally very close</source>
        <translation>blocs très proches dans le temps</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5838"/>
        <source>I locked your Monero wallet to protect you while you were away</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5858"/>
        <source>Locked due to inactivity. The wallet password is required to unlock the console.</source>
        <translation>Verrouillé pour cause d&apos;inactivité. Le mot de passe du portefeuille est requis pour déverrouiller la console.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5935"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6539"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6821"/>
        <source>ring size %u is too large, maximum is %u</source>
        <translation>la taille de cercle %u est trop grande, le maximum est %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5966"/>
        <source>payment id failed to encode</source>
        <translation>échec de l&apos;encodage de l&apos;ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6013"/>
        <source>failed to parse short payment ID from URI</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement court à partir de l&apos;URI</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6038"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6040"/>
        <source>Invalid last argument: </source>
        <translation>Dernier argument invalide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6058"/>
        <source>a single transaction cannot use more than one payment id</source>
        <translation>une unique transaction ne peut pas utiliser plus d&apos;un ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6076"/>
        <source>failed to parse payment id, though it was detected</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement, bien qu&apos;il ait été détecté</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6447"/>
        <source>Not enough money in unlocked balance</source>
        <translation>Pas assez de fonds dans le solde débloqué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6548"/>
        <source>missing lockedblocks parameter</source>
        <translation>paramètre blocs_verrou manquant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6558"/>
        <source>bad locked_blocks parameter</source>
        <translation>mauvais paramètre blocs_verrou</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6583"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6830"/>
        <source>Failed to parse number of outputs</source>
        <translation>Échec de l&apos;analyse du nombre de sorties</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6588"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6835"/>
        <source>Amount of outputs should be greater than 0</source>
        <translation>Le nombre de sorties doit être supérieur à 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7073"/>
        <source>Donating %s %s to The Monero Project (donate.getmonero.org or %s).</source>
        <translation>Don de %s %s à The Monero Project (donate.getmonero.org ou %s).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7407"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7418"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7425"/>
        <source>failed to parse tx_key</source>
        <translation>échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7434"/>
        <source>Tx key successfully stored.</source>
        <translation>Clé de transaction sauvegardée avec succès.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7438"/>
        <source>Failed to store tx key: </source>
        <translation>Échec de la sauvegarde de la clé de transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7612"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7722"/>
        <source>Good signature</source>
        <translation>Bonne signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7639"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7724"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7824"/>
        <source>Bad signature</source>
        <translation>Mauvaise signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8108"/>
        <source>usage: show_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation>usage : show_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;hauteur_min&gt; [&lt;hauteur_max&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7941"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8186"/>
        <source>block</source>
        <translation>bloc</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8423"/>
        <source>Warning: this will lose any information which can not be recovered from the blockchain.</source>
        <translation>Attention : ceci pedra toute information qui ne peut pas être retrouvée à partir de la chaîne de blocs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8424"/>
        <source>This includes destination addresses, tx secret keys, tx notes, etc</source>
        <translation>Ceci inclut les adresses de destination, les clé secrètes de transaction, les notes de transaction, etc</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8950"/>
        <source>Standard address: </source>
        <translation>Adresse standard : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8955"/>
        <source>failed to parse payment ID or address</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement ou de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8997"/>
        <source>failed to parse payment ID</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9015"/>
        <source>failed to parse index</source>
        <translation>échec de l&apos;analyse de l&apos;index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9023"/>
        <source>Address book is empty.</source>
        <translation>Le carnet d&apos;adresses est vide.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9029"/>
        <source>Index: </source>
        <translation>Index : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9030"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9161"/>
        <source>Address: </source>
        <translation>Adresse : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9031"/>
        <source>Payment ID: </source>
        <translation>ID de paiement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9032"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9160"/>
        <source>Description: </source>
        <translation>Description : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9170"/>
        <source>Network type: </source>
        <translation>Type de réseau : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9171"/>
        <source>Testnet</source>
        <translation>Testnet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9172"/>
        <source>Stagenet</source>
        <translation>Stagenet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9172"/>
        <source>Mainnet</source>
        <translation>Mainnet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9190"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas signer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1316"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9204"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9230"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9481"/>
        <source>failed to read file </source>
        <translation>échec de la lecture du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5985"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6563"/>
        <source>Locked blocks too high, max 1000000 (˜4 yrs)</source>
        <translation>Nombre de blocs de verrouillage trop élevé, 1000000 maximum (~ 4 ans)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6144"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6154"/>
        <source>Is this okay anyway?</source>
        <translation>Est-ce correct quand même ?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6149"/>
        <source>There is currently a %u block backlog at that fee level. Is this okay?</source>
        <translation>Il y a actuellement un arriéré de %u blocs à ce niveau de frais. Est-ce acceptable ?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7601"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7715"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7809"/>
        <source>failed to load signature file</source>
        <translation>échec du chargement du fichier signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7663"/>
        <source>wallet is watch-only and cannot generate the proof</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut générer de preuve</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7747"/>
        <source>The reserve proof can be generated only by a full wallet</source>
        <translation>La preuve de réserve ne peut être généré que par un portefeuille complet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7802"/>
        <source>Address must not be a subaddress</source>
        <translation>L&apos;adresse ne doit pas être une sous-adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7820"/>
        <source>Good signature -- total: %s, spent: %s, unspent: %s</source>
        <translation>Bonne signature -- total : %s, dépensé : %s, non dépensé : %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8031"/>
        <source>[Double spend seen on the network: this transaction may or may not end up being mined] </source>
        <translation>[Double dépense détectée sur le réseau : cette transaction sera peut-être invalidée] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8309"/>
        <source>There is no unspent output in the specified address</source>
        <translation>Il n&apos;y a pas de sortie non dépensée pour l&apos;adresse spécifiée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8537"/>
        <source> (no daemon)</source>
        <translation> (pas de démon)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8539"/>
        <source> (out of sync)</source>
        <translation> (désynchronisé)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8590"/>
        <source>(Untitled account)</source>
        <translation>(compte sans nom)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8603"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8621"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8646"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8669"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8817"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8840"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8868"/>
        <source>failed to parse index: </source>
        <translation>échec de l&apos;analyse de l&apos;index : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8608"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8822"/>
        <source>specify an index between 0 and </source>
        <translation>specifiez un index entre 0 et </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8726"/>
        <source>
Grand total:
  Balance: </source>
        <translation>
Somme finale :
  Solde : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8726"/>
        <source>, unlocked balance: </source>
        <translation>, solde débloqué : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8734"/>
        <source>Untagged accounts:</source>
        <translation>Comptes sans mot clé :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8740"/>
        <source>Tag %s is unregistered.</source>
        <translation>Le mot clé %s n&apos;est pas enregistré.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8743"/>
        <source>Accounts with tag: </source>
        <translation>Comptes avec mot clé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8744"/>
        <source>Tag&apos;s description: </source>
        <translation>Description du mot clé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8746"/>
        <source>Account</source>
        <translation>Compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8752"/>
        <source> %c%8u %6s %21s %21s %21s</source>
        <translation> %c%8u %6s %21s %21s %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8762"/>
        <source>----------------------------------------------------------------------------------</source>
        <translation>----------------------------------------------------------------------------------</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8763"/>
        <source>%15s %21s %21s</source>
        <translation>%15s %21s %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8787"/>
        <source>Primary address</source>
        <translation>Adresse primaire</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8787"/>
        <source>(used)</source>
        <translation>(utilisé)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8808"/>
        <source>(Untitled address)</source>
        <translation>(adresse sans nom)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8849"/>
        <source>&lt;index_min&gt; is already out of bound</source>
        <translation>&lt;index_min&gt; est déjà hors limite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8854"/>
        <source>&lt;index_max&gt; exceeds the bound</source>
        <translation>&lt;index_max&gt; excède la limite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8918"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8931"/>
        <source>Integrated addresses can only be created for account 0</source>
        <translation>Les adresses intégrées ne peuvent être créées que pour le compte 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8944"/>
        <source>Integrated address: %s, payment ID: %s</source>
        <translation>Adresse intégrée : %s, ID de paiement : %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8950"/>
        <source>Subaddress: </source>
        <translation>Sous-adresse : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9117"/>
        <source>no description found</source>
        <translation>pas de description trouvée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9119"/>
        <source>description found: </source>
        <translation>description trouvée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9159"/>
        <source>Filename: </source>
        <translation>Fichier : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9164"/>
        <source>Watch only</source>
        <translation>Audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9166"/>
        <source>%u/%u multisig%s</source>
        <translation>Multisig %u/%u%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9168"/>
        <source>Normal</source>
        <translation>Normal</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9169"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9980"/>
        <source>Type: </source>
        <translation>Type : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9195"/>
        <source>This wallet is multisig and cannot sign</source>
        <translation>C&apos;est un portefeuille multisig et il ne peut pas signer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9244"/>
        <source>Bad signature from </source>
        <translation>Mauvaise signature de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9248"/>
        <source>Good signature from </source>
        <translation>Bonne signature de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9264"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas exporter les images de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1255"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9291"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9448"/>
        <source>failed to save file </source>
        <translation>échec de l&apos;enregistrement du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9302"/>
        <source>Signed key images exported to </source>
        <translation>Images de clé signées exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9459"/>
        <source>Outputs exported to </source>
        <translation>Sorties exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6028"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7048"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7758"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8268"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8276"/>
        <source>amount is wrong: </source>
        <translation>montant erroné : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6029"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7048"/>
        <source>expected number from 0 to </source>
        <translation>attend un nombre de 0 à </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6393"/>
        <source>Sweeping </source>
        <translation>Balayage de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6979"/>
        <source>Money successfully sent, transaction: </source>
        <translation>Fonds envoyés avec succès, transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7176"/>
        <source>Change goes to more than one address</source>
        <translation>La monnaie rendue va à plus d&apos;une adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7217"/>
        <source>%s change to %s</source>
        <translation>%s de monnaie rendue à %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7220"/>
        <source>no change</source>
        <translation>sans monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1462"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1475"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7292"/>
        <source>Transaction successfully signed to file </source>
        <translation>Transaction signée avec succès dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7359"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7397"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7454"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7503"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7585"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7670"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7705"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9049"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9077"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9511"/>
        <source>failed to parse txid</source>
        <translation>échec de l&apos;analyse de l&apos;ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7375"/>
        <source>Tx key: </source>
        <translation>Clé de transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7380"/>
        <source>no tx keys found for this txid</source>
        <translation>aucune clé de transaction trouvée pour cet ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7472"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7684"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7773"/>
        <source>signature file saved to: </source>
        <translation>fichier signature sauvegardé dans : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7474"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7686"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7775"/>
        <source>failed to save signature file</source>
        <translation>échec de la sauvegarde du fichier signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7511"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7520"/>
        <source>failed to parse tx key</source>
        <translation>échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7478"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7566"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7644"/>
        <source>error: </source>
        <translation>erreur : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7542"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7615"/>
        <source>received</source>
        <translation>a reçu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7542"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7615"/>
        <source>in txid</source>
        <translation>dans la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7561"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7634"/>
        <source>received nothing in txid</source>
        <translation>n&apos;a rien reçu dans la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7545"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7618"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation>ATTENTION : cette transaction n&apos;est pas encore inclue dans la chaîne de blocs !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7551"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7624"/>
        <source>This transaction has %u confirmations</source>
        <translation>Cette transaction a %u confirmations</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7555"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7628"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation>ATTENTION : échec de la détermination du nombre de confirmations !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7909"/>
        <source>bad min_height parameter:</source>
        <translation>mauvais paramètre hauteur_minimum :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7921"/>
        <source>bad max_height parameter:</source>
        <translation>mauvais paramètre hauteur_maximum :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7941"/>
        <source>in</source>
        <translation>reçu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8283"/>
        <source>&lt;min_amount&gt; should be smaller than &lt;max_amount&gt;</source>
        <translation>&lt;montant_minimum&gt; doit être inférieur à &lt;montant_maximum&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8315"/>
        <source>
Amount: </source>
        <translation>
Montant : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8315"/>
        <source>, number of keys: </source>
        <translation>, nombre de clés : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8320"/>
        <source> </source>
        <translation> </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8325"/>
        <source>
Min block height: </source>
        <translation>
Hauteur de bloc minimum : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8326"/>
        <source>
Max block height: </source>
        <translation>
Hauteur de bloc maximum : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8327"/>
        <source>
Min amount found: </source>
        <translation>
Montant minimum trouvé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8328"/>
        <source>
Max amount found: </source>
        <translation>
Montant maximum trouvé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8329"/>
        <source>
Total count: </source>
        <translation>
Compte total : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8369"/>
        <source>
Bin size: </source>
        <translation>
Taille de classe : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8370"/>
        <source>
Outputs per *: </source>
        <translation>
Sorties par * : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8372"/>
        <source>count
  ^
</source>
        <translation>compte
  ^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8374"/>
        <source>  |</source>
        <translation>  |</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8376"/>
        <source>  +</source>
        <translation>  +</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8376"/>
        <source>+--&gt; block height
</source>
        <translation>+--&gt; hauteur de bloc
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8377"/>
        <source>   ^</source>
        <translation>   ^</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8377"/>
        <source>^
</source>
        <translation>^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8378"/>
        <source>  </source>
        <translation>  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8535"/>
        <source>wallet</source>
        <translation>portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="896"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8922"/>
        <source>Random payment ID: </source>
        <translation>ID de paiement aléatoire : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8923"/>
        <source>Matching integrated address: </source>
        <translation>Adresse intégrée correspondante : </translation>
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
        <translation>Erreur de création des portefeuilles multisig : </translation>
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
        <translation>Erreur : N/M attendu, mais lu : </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="216"/>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="225"/>
        <source>Error: either --scheme or both of --threshold and --participants may be given</source>
        <translation>Erreur : soit --scheme soit --threshold et --participants doivent être indiqués</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="232"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got N==%u and M==%d</source>
        <translation>Erreur : N &gt; 1 et N &lt;= M attendu, mais lu N==%u et M==%d</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="241"/>
        <source>Error: --filename-base is required</source>
        <translation>Erreur : --filename-base est requis</translation>
    </message>
</context>
<context>
    <name>mms::message_store</name>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="69"/>
        <source>Use PyBitmessage instance at URL &lt;arg&gt;</source>
        <translation>Utiliser l&apos;instance de PyBitmessage à l&apos;URL &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="70"/>
        <source>Specify &lt;arg&gt; as username:password for PyBitmessage API</source>
        <translation>Spécifier &lt;arg&gt; comme utilisateur:mot_de_passe pour l&apos;API PyBitmessage</translation>
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
        <translation>Le portefeuille ne peut pas commencer une autre ronde d&apos;échange de clés car l&apos;ensemble des clés des autres signataires est manquant ou incomplet.</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1007"/>
        <source>Syncing not done because multisig sync data from other signers are missing or not complete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1121"/>
        <source>There are waiting messages, but nothing is ready to process under normal circumstances</source>
        <translation>Il y a des messages en attente, mais rien n&apos;est prêt à être traité dans des circonstances normales</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1124"/>
        <source>
Use &quot;mms next sync&quot; if you want to force processing of the waiting sync data</source>
        <translation>
Utilisez &quot;mms next sync&quot; si vous voulez forcer le traitement des données de synchronisation en attente</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1128"/>
        <source>
Use &quot;mms note&quot; to display the waiting notes</source>
        <translation>
Utilisez &quot;mms note&quot; pour afficher les notes en attente</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1133"/>
        <source>There are no messages waiting to be processed.</source>
        <translation>Il n&apos;y a pas de message en attente de traitement.</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1351"/>
        <source>key set</source>
        <translation>ensemble de clés</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1353"/>
        <source>additional key set</source>
        <translation>ensemble de clés additionnel</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1355"/>
        <source>multisig sync data</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1357"/>
        <source>partially signed tx</source>
        <translation>transaction partiellement signée</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1359"/>
        <source>fully signed tx</source>
        <translation>transaction complètement signée</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1361"/>
        <source>note</source>
        <translation>note</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1363"/>
        <source>signer config</source>
        <translation>configuration de signataire</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1365"/>
        <source>auto-config data</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1367"/>
        <source>unknown message type</source>
        <translation>type de message inconnu</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1376"/>
        <source>in</source>
        <translation>entrant</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1378"/>
        <source>out</source>
        <translation>sortant</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1380"/>
        <source>unknown message direction</source>
        <translation>direction de message inconnue</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1389"/>
        <source>ready to send</source>
        <translation>prêt à envoyer</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1391"/>
        <source>sent</source>
        <translation>envoyé</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1393"/>
        <source>waiting</source>
        <translation>en attente</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1395"/>
        <source>processed</source>
        <translation>traité</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1397"/>
        <source>cancelled</source>
        <translation>annulé</translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1399"/>
        <source>unknown message state</source>
        <translation>état de message inconnu</translation>
    </message>
</context>
<context>
    <name>sw</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="137"/>
        <source>Generate new wallet and save it to &lt;arg&gt;</source>
        <translation>Générer un nouveau portefeuille et le sauvegarder dans &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="138"/>
        <source>Generate new wallet from device and save it to &lt;arg&gt;</source>
        <translation>Générer un nouveau portefeuille à partir de l&apos;appareil et le sauvegarder dans &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="139"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>Générer un portefeuille d&apos;audit à partir d&apos;une clé d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="140"/>
        <source>Generate deterministic wallet from spend key</source>
        <translation>Générer un portefeuille déterministe à partir d&apos;une clé de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="141"/>
        <source>Generate wallet from private keys</source>
        <translation>Générer un portefeuille à partir de clés privées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="142"/>
        <source>Generate a master wallet from multisig wallet keys</source>
        <translation>Générer un portefeuille principal à partir de clés de portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="144"/>
        <source>Language for mnemonic</source>
        <translation>Langue de la phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="145"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>Spécifier la phrase mnémonique Electrum pour la récupération/création d&apos;un portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="146"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Récupérer un portefeuille en utilisant une phrase mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="147"/>
        <source>alias for --restore-deterministic-wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="148"/>
        <source>Recover multisig wallet using Electrum-style mnemonic seed</source>
        <translation>Récupérer un portefeuille multisig en utilisant une phrase mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="149"/>
        <source>Generate non-deterministic view and spend keys</source>
        <translation>Générer des clés d&apos;audit et de dépense non déterministes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="152"/>
        <source>Restore from estimated blockchain height on specified date</source>
        <translation>Restaurer depuis la hauteur estimée de la chaîne de blocs à la date spécifiée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="379"/>
        <source>invalid argument: must be either 0/1, true/false, y/n, yes/no</source>
        <translation>argument invalide : doit être soit 0/1, true/false, y/n, yes/no</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="435"/>
        <source>DNSSEC validation passed</source>
        <translation>Validation DNSSEC réussie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="439"/>
        <source>WARNING: DNSSEC validation was unsuccessful, this address may not be correct!</source>
        <translation>ATTENTION: la validation DNSSEC a échoué, cette adresse n&apos;est peut être pas correcte !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="442"/>
        <source>For URL: </source>
        <translation>Pour l&apos;URL : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="444"/>
        <source> Monero Address = </source>
        <translation> Adresse Monero = </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="456"/>
        <source>you have cancelled the transfer request</source>
        <translation>vous avez annulé la demande de transfert</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="477"/>
        <source>failed to parse index: </source>
        <translation>échec de l&apos;analyse de l&apos;index : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="490"/>
        <source>invalid format for subaddress lookahead; must be &lt;major&gt;:&lt;minor&gt;</source>
        <translation>format invalide pour l&apos;anticipation des sous-addresses; doit être &lt;majeur&gt;:&lt;mineur&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="507"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="512"/>
        <source>RPC error: </source>
        <translation>Erreur RPC : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="516"/>
        <source>failed to get random outputs to mix: </source>
        <translation>échec de la récupération de sorties aléatoires à mélanger : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="523"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="531"/>
        <source>Not enough money in unlocked balance</source>
        <translation>Pas assez de fonds dans le solde débloqué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="541"/>
        <source>Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees</source>
        <translation>Impossible de trouver une façon de créer les transactions. Ceci est souvent dû à de la poussière si petite qu&apos;elle ne peut pas payer ses propres frais, ou à une tentative d&apos;envoi d&apos;un montant supérieur au solde débloqué, ou à un montant restant insuffisant pour payer les frais</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="547"/>
        <source>not enough outputs for specified ring size</source>
        <translation>pas assez de sorties pour la taille de cercle spécifiée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="550"/>
        <source>output amount</source>
        <translation>montant de la sortie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="550"/>
        <source>found outputs to use</source>
        <translation>sorties à utiliser trouvées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="552"/>
        <source>Please use sweep_unmixable.</source>
        <translation>Veuillez utiliser sweep_unmixable.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="556"/>
        <source>transaction was not constructed</source>
        <translation>la transaction n&apos;a pas été construite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="564"/>
        <source>Reason: </source>
        <translation>Raison : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="573"/>
        <source>one of destinations is zero</source>
        <translation>une des destinations est zéro</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="578"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>échec de la recherche d&apos;une façon adéquate de scinder les transactions</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="584"/>
        <source>unknown transfer error: </source>
        <translation>erreur de transfert inconnue : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="589"/>
        <source>Multisig error: </source>
        <translation>Erreur multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="595"/>
        <source>internal error: </source>
        <translation>erreur interne : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="600"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="604"/>
        <source>There was an error, which could mean the node may be trying to get you to retry creating a transaction, and zero in on which outputs you own. Or it could be a bona fide error. It may be prudent to disconnect from this node, and not try to send a transaction immediately. Alternatively, connect to another node so the original node cannot correlate information.</source>
        <translation>Il y a eu une erreur, ce qui pourrait signifier que le noeud essaye de vous faire réessayer de créer une transaction, pour tenter d&apos;identifier quelles sorties sont les votres. Ou il pourrait s&apos;agir d&apos;une erreur de bonne foi. Il pourrait être prudent de se déconnecter de ce noeud, et de ne pas essayer d&apos;envoyer une transaction immédiatement. Ou sinon, se connecter à un autre noeud pour que le noeud original ne puisse pas corréler les informations.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="614"/>
        <source>File %s likely stores wallet private keys! Use a different file name.</source>
        <translation>Le fichier %s contient probablement des clés privées de portefeuille ! Utilisez un nom de fichier différent.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7838"/>
        <source> seconds</source>
        <translation> secondes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7840"/>
        <source> minutes</source>
        <translation> minutes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7842"/>
        <source> hours</source>
        <translation> heures</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7844"/>
        <source> days</source>
        <translation> jours</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7846"/>
        <source> months</source>
        <translation> mois</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7847"/>
        <source>a long time</source>
        <translation>longtemps</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9740"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.
WARNING: Do not reuse your Monero keys on another fork, UNLESS this fork has key reuse mitigations built in. Doing so will harm your privacy.</source>
        <translation>Ceci est le portefeuille monero en ligne de commande.
Il a besoin de se connecter à un démon monero pour fonctionner correctement.
ATTENTION : Ne réutilisez pas vos clés Monero avec un autre fork, À MOINS QUE ce fork inclue des mitigations contre la réutilisation des clés. Faire ceci nuira à votre confidentialité.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9765"/>
        <source>Unknown command: </source>
        <translation>Commande inconnue : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="150"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>Autoriser la communication avec un démon utilisant une version de RPC différente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="151"/>
        <source>Restore from specific blockchain height</source>
        <translation>Restaurer à partir d&apos;une hauteur de bloc spécifique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="153"/>
        <source>The newly created transaction will not be relayed to the monero network</source>
        <translation>La transaction nouvellement créée ne sera pas transmise au réseau monero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="154"/>
        <source>Create an address file for new wallets</source>
        <translation>Créer un fichier d&apos;adresse pour les nouveaux portefeuilles</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="156"/>
        <source>Display English language names</source>
        <translation>Afficher les noms de langue en anglais</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="294"/>
        <source>failed to read wallet password</source>
        <translation>échec de la lecture du mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="301"/>
        <source>Enter a new password for the wallet</source>
        <translation>Entrer un nouveau mot de passe pour le portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="301"/>
        <source>Wallet password</source>
        <translation>Mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="311"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="503"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="320"/>
        <source>possibly lost connection to daemon</source>
        <translation>connexion avec le démon peut-être perdue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="337"/>
        <source>Error: </source>
        <translation>Erreur : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="446"/>
        <source>Is this OK?</source>
        <translation>Est-ce correct ?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="561"/>
        <source>transaction %s was rejected by daemon</source>
        <translation>la transaction %s a été rejetée par le démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="617"/>
        <source>File %s already exists. Are you sure to overwrite it?</source>
        <translation>Le fichier %s existe déjà. Êtes-vous sûr de vouloir l&apos;écraser ?</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9759"/>
        <source>Failed to initialize wallet</source>
        <translation>Échec de l&apos;initialisation du portefeuille</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="248"/>
        <source>Use daemon instance at &lt;host&gt;:&lt;port&gt;</source>
        <translation>Utiliser l&apos;instance de démon située à &lt;hôte&gt;:&lt;port&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="249"/>
        <source>Use daemon instance at host &lt;arg&gt; instead of localhost</source>
        <translation>Utiliser l&apos;instance de démon située à l&apos;hôte &lt;arg&gt; au lieu de localhost</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="254"/>
        <source>Wallet password file</source>
        <translation>Fichier mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="255"/>
        <source>Use daemon instance at port &lt;arg&gt; instead of 18081</source>
        <translation>Utiliser l&apos;instance de démon située au port &lt;arg&gt; au lieu de 18081</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="264"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>Pour testnet, le démon doit aussi être lancé avec l&apos;option --testnet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="381"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>impossible de spécifier l&apos;hôte ou le port du démon plus d&apos;une fois</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="512"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>impossible de spécifier plus d&apos;une option parmis --password et --password-file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="525"/>
        <source>the password file specified could not be read</source>
        <translation>le fichier mot de passe spécifié n&apos;a pas pu être lu</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="551"/>
        <source>Failed to load file </source>
        <translation>Échec du chargement du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="253"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation>Mot de passe du portefeuille (échapper/citer si nécessaire)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="250"/>
        <source>[&lt;ip&gt;:]&lt;port&gt; socks proxy to use for daemon connections</source>
        <translation>[&lt;ip&gt;:]&lt;port&gt; proxy socks à utiliser pour les connexions du démon</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="251"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation>Activer les commandes qui dépendent d&apos;un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="252"/>
        <source>Disable commands which rely on a trusted daemon</source>
        <translation>Désactiver les commandes qui dépendent d&apos;un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="256"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>Spécifier le nom_utilisateur:[mot_de_passe] pour le client RPC du démon</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="257"/>
        <source>Enable SSL on daemon RPC connections: enabled|disabled|autodetect</source>
        <translation>Activer SSL pour les connexions RPC au démon : enabled|disabled|autodetect</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="261"/>
        <source>List of valid fingerprints of allowed RPC servers</source>
        <translation>Liste des empreintes des serveurs RPC autorisés</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="262"/>
        <source>Allow any SSL certificate from the daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="263"/>
        <source>Allow user (via --daemon-ssl-ca-certificates) chain certificates</source>
        <translation>Autoriser une chaîne de certificats utilisateurs (via --daemon-ssl-ca-certificates)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="265"/>
        <source>For stagenet. Daemon must also be launched with --stagenet flag</source>
        <translation>Pour stagenet, le démon doit aussi être lancé avec l&apos;option --stagenet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="267"/>
        <source>Set shared ring database path</source>
        <translation>Définir le chemin de la base de donnée de cercles partagés</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="278"/>
        <source>Number of rounds for the key derivation function</source>
        <translation>Nombre de rondes de la fonction de dérivation de clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="279"/>
        <source>HW device to use</source>
        <translation>Portefeuille matériel à utiliser</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="280"/>
        <source>HW device wallet derivation path (e.g., SLIP-10)</source>
        <translation>Chemin de dérivation du périphérique de portefeuille matériel (e.g. SLIP-10)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="282"/>
        <source>Do not use DNS</source>
        <translation>Ne pas utiliser DNS</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="283"/>
        <source>Do not connect to a daemon, nor use DNS</source>
        <translation>Ne pas se connecter à un démon, ni utiliser DNS</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="284"/>
        <source>File containing extra entropy to initialize the PRNG (any data, aim for 256 bits of entropy to be useful, wihch typically means more than 256 bits of data)</source>
        <translation>Fichier contenant de l&apos;entropie supplémentaire pour initialiser le PRNG (n&apos;importe quelles données, visez 256 bits d&apos;entropie pour que cela soit utile, ce qui signifie typiquement plus de 256 bits de données)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="373"/>
        <source>Invalid argument for </source>
        <translation>Argument invalide pour </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="420"/>
        <source>Enabling --</source>
        <translation>Activer --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="420"/>
        <source> requires --</source>
        <translation> requiert --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="421"/>
        <source> or --</source>
        <translation> ou --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="421"/>
        <source> or use of a .onion/.i2p domain</source>
        <translation> ou utilisez un domaine .onion/.i2p</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="455"/>
        <source>--trusted-daemon and --untrusted-daemon are both seen, assuming untrusted</source>
        <translation>--trusted-daemon et --untrusted-daemon présents simultanément, --untrusted-daemon choisi</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="465"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Le démon est local, supposons qu&apos;il est de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="532"/>
        <source>no password specified; use --prompt-for-password to prompt for a password</source>
        <translation>pas de mot de passe spécifié; utilisez --prompt-for-password pour demander un mot de passe</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="534"/>
        <source>Enter a new password for the wallet</source>
        <translation>Entrer un nouveau mot de passe pour le portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="534"/>
        <source>Wallet password</source>
        <translation>Mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="557"/>
        <source>Failed to parse JSON</source>
        <translation>Échec de l&apos;analyse JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="564"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation>Version %u trop récente, on comprend au mieux %u</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="580"/>
        <source>failed to parse view key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="585"/>
        <location filename="../src/wallet/wallet2.cpp" line="653"/>
        <location filename="../src/wallet/wallet2.cpp" line="698"/>
        <source>failed to verify view key secret key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="596"/>
        <source>failed to parse spend key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="601"/>
        <location filename="../src/wallet/wallet2.cpp" line="663"/>
        <location filename="../src/wallet/wallet2.cpp" line="724"/>
        <source>failed to verify spend key secret key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="613"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="633"/>
        <source>At least one of either an Electrum-style word list, private view key, or private spend key must be specified</source>
        <translation>Il faut spécifier au moins une des options parmis la liste de mots de style Electrum, la clé privée d&apos;audit et la clé privée de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="637"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Liste de mots de style Electrum et clé privée spécifiées en même temps</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="647"/>
        <source>invalid address</source>
        <translation>adresse invalide</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="656"/>
        <source>view key does not match standard address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="666"/>
        <source>spend key does not match standard address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="674"/>
        <source>Cannot generate deprecated wallets from JSON</source>
        <translation>Impossible de générer un portefeuille obsolète à partir de JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="710"/>
        <source>failed to parse address: </source>
        <translation>échec de l&apos;analyse de l&apos;adresse : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="716"/>
        <source>Address must be specified in order to create watch-only wallet</source>
        <translation>L&apos;adresse doit être spécifiée afin de créer un portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="733"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="1723"/>
        <source>Password is needed to compute key image for incoming monero</source>
        <translation>Le mot de passe est requis pour calculer l&apos;image de clé pour les moneros entrants</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="1724"/>
        <source>Invalid password: password is needed to compute key image for incoming monero</source>
        <translation>Mot de passe invalide : le mot de passe est requis pour calculer l&apos;image de clé pour les moneros entrants</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="4263"/>
        <location filename="../src/wallet/wallet2.cpp" line="4853"/>
        <location filename="../src/wallet/wallet2.cpp" line="5453"/>
        <source>Primary account</source>
        <translation>Compte primaire</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="11062"/>
        <source>No funds received in this tx.</source>
        <translation>Aucun fonds n&apos;a été reçu dans cette transaction.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="11826"/>
        <source>failed to read file </source>
        <translation>échec de la lecture du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="155"/>
        <source>Set subaddress lookahead sizes to &lt;major&gt;:&lt;minor&gt;</source>
        <translation>Définir les tailles d&apos;anticipation des sous-addresses à &lt;majeur&gt;:&lt;mineur&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="258"/>
        <source>Path to a PEM format private key</source>
        <translation>Chemin vers une clé privée au format PEM</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="259"/>
        <source>Path to a PEM format certificate</source>
        <translation>Chemin vers un certificat au format PEM</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="260"/>
        <source>Path to file containing concatenated PEM format certificate(s) to replace system CA(s).</source>
        <translation>Chemin vers un fichier contenant le(s) certificat(s) concaténé(s) au format PEM pour remplacer le(s) CA(s) du système.</translation>
    </message>
</context>
<context>
    <name>tools::wallet_rpc_server</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="188"/>
        <source>Failed to create directory </source>
        <translation>Échec de la création du répertoire </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="190"/>
        <source>Failed to create directory %s: %s</source>
        <translation>Échec de la création du répertoire %s : %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="201"/>
        <source>Cannot specify --</source>
        <translation>Impossible de spécifier --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="201"/>
        <source> and --</source>
        <translation> et --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="220"/>
        <source>Failed to create file </source>
        <translation>Échec de la création du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="220"/>
        <source>. Check permissions or remove file</source>
        <translation>. Vérifiez les permissions ou supprimez le fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="230"/>
        <source>Error writing to file </source>
        <translation>Erreur d&apos;écriture dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="233"/>
        <source>RPC username/password is stored in file </source>
        <translation>nom_utilisateur/mot_de_passe RPC sauvegardé dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="592"/>
        <source>Tag %s is unregistered.</source>
        <translation>Le mot clé %s n&apos;est pas enregistré.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3326"/>
        <source>Transaction not possible. Available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>Transaction impossible. Solde disponible : %s, montant de la transaction %s = %s + %s (frais)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4494"/>
        <source>This is the RPC monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation>Ceci est le portefeuille monero par RPC. Il a besoin de se
connecter à un démon monero pour fonctionner correctement.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4335"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>Impossible de spécifier plus d&apos;une option parmis --wallet-file et --generate-from-json</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4320"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation>Impossible de spécifier plus d&apos;une option parmis --testnet et --stagenet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4347"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>--wallet-file, --generate-from-json ou --wallet-dir doit être spécifié</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4351"/>
        <source>Loading wallet...</source>
        <translation>Chargement du portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4385"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4417"/>
        <source>Saving wallet...</source>
        <translation>Sauvegarde du portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4387"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4419"/>
        <source>Successfully saved</source>
        <translation>Sauvegardé avec succès</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4390"/>
        <source>Successfully loaded</source>
        <translation>Chargé avec succès</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4394"/>
        <source>Wallet initialization failed: </source>
        <translation>Échec de l&apos;initialisation du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4400"/>
        <source>Failed to initialize wallet RPC server</source>
        <translation>Échec de l&apos;initialisation du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4404"/>
        <source>Starting wallet RPC server</source>
        <translation>Démarrage du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4411"/>
        <source>Failed to run wallet: </source>
        <translation>Échec du lancement du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4414"/>
        <source>Stopped wallet RPC server</source>
        <translation>Arrêt du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4423"/>
        <source>Failed to save wallet: </source>
        <translation>Échec de la sauvegarde du portefeuille : </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="168"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4475"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9706"/>
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
        <translation>Journalisation dans : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="215"/>
        <source>Logging to %s</source>
        <translation>Journalisation dans %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="219"/>
        <source>WARNING: You may not have a high enough lockable memory limit</source>
        <translation>ATTENTION : Vous pourriez ne pas avoir une limite de mémoire verrouillable assez élevée</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="221"/>
        <source>see ulimit -l</source>
        <translation>voir ulimit -l</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="146"/>
        <source>Usage:</source>
        <translation>Usage :</translation>
    </message>
</context>
</TS>
