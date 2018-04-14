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
        <location filename="../src/wallet/api/pending_transaction.cpp" line="90"/>
        <source>Attempting to save transaction to file, but specified file(s) exist. Exiting to not risk overwriting. File:</source>
        <translation>Tentative d&apos;enregistrement d&apos;une transaction dans un fichier, mais le fichier spécifié existe déjà. Sortie pour ne pas risquer de l&apos;écraser. Fichier :</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="97"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Échec de l&apos;écriture de(s) transaction(s) dans le fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="115"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="118"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="122"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transaction %s a été rejetée par le démon avec le statut : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="127"/>
        <source>. Reason: </source>
        <translation>. Raison : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="129"/>
        <source>Unknown exception: </source>
        <translation>Exception inconnue : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="132"/>
        <source>Unhandled exception</source>
        <translation>Exception non gérée</translation>
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
        <location filename="../src/wallet/api/wallet.cpp" line="1147"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1157"/>
        <source>Failed to add short payment id: </source>
        <translation>Échec de l&apos;ajout de l&apos;ID de paiement court : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1190"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1294"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1193"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1297"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1196"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1300"/>
        <source>RPC error: </source>
        <translation>Erreur RPC : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1233"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1337"/>
        <source>not enough outputs for specified ring size</source>
        <translation>pas assez de sorties pour la taille de cercle spécifiée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1235"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1339"/>
        <source>found outputs to use</source>
        <translation>sorties à utiliser trouvées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1237"/>
        <source>Please sweep unmixable outputs.</source>
        <translation>Veuillez balayer les sorties non mélangeables.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1303"/>
        <source>failed to get random outputs to mix</source>
        <translation>échec de la récupération de sorties aléatoires à mélanger</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1206"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1310"/>
        <source>not enough money to transfer, available only %s, sent amount %s</source>
        <translation>pas assez de fonds pour le transfert, montant disponible %s, montant envoyé %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="506"/>
        <source>failed to parse address</source>
        <translation>échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="518"/>
        <source>failed to parse secret spend key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="528"/>
        <source>No view key supplied, cancelled</source>
        <translation>Pas de clé d&apos;audit fournie, annulation</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="535"/>
        <source>failed to parse secret view key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="545"/>
        <source>failed to verify secret spend key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="550"/>
        <source>spend key does not match address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="556"/>
        <source>failed to verify secret view key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="561"/>
        <source>view key does not match address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="580"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="809"/>
        <source>Failed to send import wallet request</source>
        <translation>Échec de l&apos;envoi de la requête d&apos;importation de portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="955"/>
        <source>Failed to load unsigned transactions</source>
        <translation>Échec du chargement des transaction non signées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="976"/>
        <source>Failed to load transaction from file</source>
        <translation>Échec du chargement de la transaction du fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="994"/>
        <source>Wallet is view only</source>
        <translation>Portefeuille d&apos;audit uniquement</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1003"/>
        <source>failed to save file </source>
        <translation>échec de l&apos;enregistrement du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1022"/>
        <source>Key images can only be imported with a trusted daemon</source>
        <translation>Les images de clé ne peuvent être importées qu&apos;avec un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1035"/>
        <source>Failed to import key images: </source>
        <translation>Échec de l&apos;importation des images de clé : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1068"/>
        <source>Failed to get subaddress label: </source>
        <translation>Échec de la récupération de l&apos;étiquette de sous-adresse : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1082"/>
        <source>Failed to set subaddress label: </source>
        <translation>Échec de l&apos;affectation de l&apos;étiquette de sous-adresse : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1199"/>
        <source>failed to get random outputs to mix: %s</source>
        <translation>échec de la récupération de sorties aléatoires à mélanger : %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1215"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1319"/>
        <source>not enough money to transfer, overall balance only %s, sent amount %s</source>
        <translation>pas assez de fonds pour le transfer, solde global disponible %s, montant envoyé %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1224"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1328"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>pas assez de fonds pour le transfert, montant disponible %s, montant envoyé %s = %s + %s (frais)</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1235"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1339"/>
        <source>output amount</source>
        <translation>montant de la sortie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1241"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1344"/>
        <source>transaction was not constructed</source>
        <translation>la transaction n&apos;a pas été construite</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1245"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1348"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transaction %s a été rejetée par le démon avec le statut : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1252"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1355"/>
        <source>one of destinations is zero</source>
        <translation>une des destinations est zéro</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1255"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1358"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>échec de la recherche d&apos;une façon adéquate de scinder les transactions</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1258"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1361"/>
        <source>unknown transfer error: </source>
        <translation>erreur de transfert inconnue : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1261"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1364"/>
        <source>internal error: </source>
        <translation>erreur interne : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1264"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1367"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1267"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1370"/>
        <source>unknown error</source>
        <translation>erreur inconnue</translation>
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
        <translation>Échec de l&apos;analyse de l&apos;ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1466"/>
        <source>no tx keys found for this txid</source>
        <translation>aucune clé de transaction trouvée pour cet ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1486"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1496"/>
        <source>Failed to parse tx key</source>
        <translation>Échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1506"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1538"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1569"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1657"/>
        <source>Failed to parse address</source>
        <translation>Échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1663"/>
        <source>Address must not be a subaddress</source>
        <translation>L&apos;adresse ne doit pas être une sous-adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1886"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>Réexaminer les dépenses ne peut se faire qu&apos;avec un démon de confiance</translation>
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
        <translation>Échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="268"/>
        <source>Failed to parse key</source>
        <translation>Échec de l&apos;analyse de la clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="276"/>
        <source>failed to verify key</source>
        <translation>Échec de la vérification de la clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="286"/>
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
        <location filename="../src/rpc/rpc_args.cpp" line="95"/>
        <source>Username specified with --</source>
        <translation>Le nom d&apos;utilisateur spécifié avec --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="95"/>
        <location filename="../src/rpc/rpc_args.cpp" line="105"/>
        <source> cannot be empty</source>
        <translation> ne peut pas être vide</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="105"/>
        <source> requires RFC server password --</source>
        <translation> nécessite le mot de passe du serveur RPC --</translation>
    </message>
</context>
<context>
    <name>cryptonote::simple_wallet</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="544"/>
        <source>Commands: </source>
        <translation>Commandes : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3620"/>
        <source>failed to read wallet password</source>
        <translation>échec de la lecture du mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3245"/>
        <source>invalid password</source>
        <translation>mot de passe invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2398"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation>set seed : requiert un argument. options disponibles : language</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2430"/>
        <source>set: unrecognized argument(s)</source>
        <translation>set : argument(s) non reconnu(s)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3478"/>
        <source>wallet file path not valid: </source>
        <translation>chemin du fichier portefeuille non valide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2484"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation>Tentative de génération ou de restauration d&apos;un portefeuille, mais le fichier spécifié existe déjà. Sortie pour ne pas risquer de l&apos;écraser.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="744"/>
        <source>usage: payment_id</source>
        <translation>usage : payment_id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2384"/>
        <source>needs an argument</source>
        <translation>requiert un argument</translation>
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
        <translation>0 ou 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2413"/>
        <source>0, 1, 2, 3, or 4</source>
        <translation>0, 1, 2, 3 ou 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2417"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2421"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2428"/>
        <source>unsigned integer</source>
        <translation>entier non signé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2538"/>
        <source>NOTE: the following 25 words can be used to recover access to your wallet. Write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation>VEUILLEZ NOTER : les 25 mots suivants peuvent être utilisés pour restaurer votre portefeuille. Veuillez les écrire sur papier et les garder dans un endroit sûr. Ne les gardez pas dans un courriel ou dans un service de stockage de fichiers hors de votre contrôle.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2601"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-deterministic-wallet utilise --generate-new-wallet, pas --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2630"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation>spécifiez un paramètre de récupération avec --electrum-seed=&quot;liste de mots ici&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3000"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation>spécifiez un chemin de portefeuille avec --generate-new-wallet (pas --wallet-file)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3180"/>
        <source>wallet failed to connect to daemon: </source>
        <translation>échec de la connexion du portefeuille au démon : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3188"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation>Le démon utilise une version majeure de RPC (%u) différente de celle du portefeuille (%u) : %s. Mettez l&apos;un des deux à jour, ou utilisez --allow-mismatched-daemon-version.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3207"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation>Liste des langues disponibles pour la phrase mnémonique de votre portefeuille :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3217"/>
        <source>Enter the number corresponding to the language of your choice: </source>
        <translation>Entrez le nombre correspondant à la langue de votre choix : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3290"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Veuillez dorénavant utiliser la nouvelle phrase mnémonique que nous fournissons.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3306"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3374"/>
        <source>Generated new wallet: </source>
        <translation>Nouveau portefeuille généré : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3312"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3379"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3415"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3467"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3496"/>
        <source>Opened watch-only wallet</source>
        <translation>Ouverture du portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3500"/>
        <source>Opened wallet</source>
        <translation>Ouverture du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3513"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Veuillez procéder à la mise à jour de votre portefeuille.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3528"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Le format de votre fichier portefeuille est en cours de mise à jour.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3536"/>
        <source>failed to load wallet: </source>
        <translation>échec du chargement du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3553"/>
        <source>Use the &quot;help&quot; command to see the list of available commands.
</source>
        <translation>Utilisez la commande &quot;help&quot; pour voir la liste des commandes disponibles.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3598"/>
        <source>Wallet data saved</source>
        <translation>Données du portefeuille sauvegardées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3694"/>
        <source>Mining started in daemon</source>
        <translation>La mine a démarré dans le démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3696"/>
        <source>mining has NOT been started: </source>
        <translation>la mine n&apos;a PAS démarré : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3716"/>
        <source>Mining stopped in daemon</source>
        <translation>La mine a été stoppée dans le démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3718"/>
        <source>mining has NOT been stopped: </source>
        <translation>la mine n&apos;a PAS été stoppée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3773"/>
        <source>Blockchain saved</source>
        <translation>Chaîne de blocs sauvegardée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3788"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3806"/>
        <source>Height </source>
        <translation>Hauteur </translation>
    </message>
    <message>
        <source>transaction </source>
        <translation type="vanished">transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3808"/>
        <source>spent </source>
        <translation>dépensé </translation>
    </message>
    <message>
        <source>unsupported transaction format</source>
        <translation type="vanished">format de transaction non supporté</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3834"/>
        <source>Starting refresh...</source>
        <translation>Démarrage du rafraîchissement...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3847"/>
        <source>Refresh done, blocks received: </source>
        <translation>Rafraîchissement effectué, blocs reçus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4384"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4873"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4399"/>
        <source>bad locked_blocks parameter:</source>
        <translation>mauvais paramètre locked_blocks :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4427"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4891"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5105"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation>une unique transaction ne peut pas utiliser plus d&apos;un ID de paiement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4436"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4900"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5073"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5113"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>échec de la définition de l&apos;ID de paiement, bien qu&apos;il ait été décodé correctement</translation>
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
        <translation>transaction annulée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4521"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4531"/>
        <source>Is this okay anyway?  (Y/Yes/N/No): </source>
        <translation>Est-ce correct quand même ?  (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4526"/>
        <source>There is currently a %u block backlog at that fee level. Is this okay?  (Y/Yes/N/No): </source>
        <translation>Il y a actuellement un arriéré de %u blocs à ce niveau de frais. Est-ce correct quand même ?  (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4531"/>
        <source>Failed to check for backlog: </source>
        <translation>Échec de la vérification du backlog : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4572"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4945"/>
        <source>
Transaction </source>
        <translation>
Transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4577"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4950"/>
        <source>Spending from address index %d
</source>
        <translation>Dépense depuis l&apos;adresse d&apos;index %d
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4579"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4952"/>
        <source>WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.
</source>
        <translation>ATTENTION : Des sorties de multiples adresses sont utilisées ensemble, ce qui pourrait potentiellement compromettre votre confidentialité.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4581"/>
        <source>Sending %s.  </source>
        <translation>Envoi de %s.  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4584"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation>Votre transaction doit être scindée en %llu transactions. Il en résulte que des frais de transaction doivent être appliqués à chaque transaction, pour un total de %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4590"/>
        <source>The transaction fee is %s</source>
        <translation>Les frais de transaction sont de %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4593"/>
        <source>, of which %s is dust from change</source>
        <translation>, dont %s est de la poussière de monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4594"/>
        <source>.</source>
        <translation>.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4594"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation>Un total de %s de poussière de monnaie rendue sera envoyé à une adresse de poussière</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4599"/>
        <source>.
This transaction will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation>.
Cette transaction sera déverrouillée au bloc %llu, dans approximativement %s jours (en supposant 2 minutes par bloc)</translation>
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
        <translation>Échec de l&apos;écriture de(s) transaction(s) dans le fichier</translation>
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
        <translation>Transaction(s) non signée(s) écrite(s) dans le fichier avec succès : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4709"/>
        <source>No unmixable outputs found</source>
        <translation>Aucune sortie non mélangeable trouvée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4792"/>
        <source>No address given</source>
        <translation>Aucune adresse fournie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5067"/>
        <source>failed to parse Payment ID</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5083"/>
        <source>usage: sweep_single [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;key_image&gt; &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation>usage : sweep_single [&lt;priorité&gt;] [&lt;taille_cercle&gt;] &lt;image_clé&gt; &lt;adresse&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5090"/>
        <source>failed to parse key image</source>
        <translation>échec de l&apos;analyse de l&apos;image de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5142"/>
        <source>No outputs found</source>
        <translation>Pas de sorties trouvées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5147"/>
        <source>Multiple transactions are created, which is not supposed to happen</source>
        <translation>De multiples transactions sont crées, ce qui n&apos;est pas supposé arriver</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5152"/>
        <source>The transaction uses multiple or no inputs, which is not supposed to happen</source>
        <translation>La transaction utilise aucune ou de multiples entrées, ce qui n&apos;est pas supposé arriver</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5229"/>
        <source>missing threshold amount</source>
        <translation>montant seuil manquant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5234"/>
        <source>invalid amount threshold</source>
        <translation>montant seuil invalide</translation>
    </message>
    <message>
        <source>donations are not enabled on the testnet</source>
        <translation type="vanished">les dons ne sont pas activés sur le réseau testnet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5251"/>
        <source>usage: donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation>usage : donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] &lt;montant&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5345"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>La monnaie réclamée ne va pas à une adresse payée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5350"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>La monnaie réclamée est supérieure au paiement à l&apos;adresse de monnaie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5381"/>
        <source>sending %s to %s</source>
        <translation>envoi de %s à %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5391"/>
        <source> dummy output(s)</source>
        <translation> sortie(s) factice(s)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5394"/>
        <source>with no destinations</source>
        <translation>sans destination</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5406"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu, %s. %sIs this okay? (Y/Yes/N/No): </source>
        <translation>%lu transactions chargées, pour %s, frais %s, %s, %s, taille de cercle minimum %lu, %s. %sEst-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5435"/>
        <source>This is a multisig wallet, it can only sign with sign_multisig</source>
        <translation>Ceci est un portefeuille multisig, il ne peut signer qu&apos;avec sign_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5445"/>
        <source>usage: sign_transfer [export]</source>
        <translation>usage : sign_transfer [export]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5457"/>
        <source>Failed to sign transaction</source>
        <translation>Échec de signature de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5463"/>
        <source>Failed to sign transaction: </source>
        <translation>Échec de signature de transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5484"/>
        <source>Transaction raw hex data exported to </source>
        <translation>Données brutes hex de la transaction exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5505"/>
        <source>Failed to load transaction from file</source>
        <translation>Échec du chargement de la transaction du fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3863"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4166"/>
        <source>RPC error: </source>
        <translation>Erreur RPC : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="591"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il n&apos;a pas de clé de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="716"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="872"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="945"/>
        <source>Your original password was incorrect.</source>
        <translation>Votre mot de passe original est incorrect.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="732"/>
        <source>Error with wallet rewrite: </source>
        <translation>Erreur avec la réécriture du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1683"/>
        <source>priority must be 0, 1, 2, 3, or 4 </source>
        <translation>la priorité doit être 0, 1, 2, 3 ou 4 </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1695"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1710"/>
        <source>priority must be 0, 1, 2, 3, or 4</source>
        <translation>la priorité doit être 0, 1, 2, 3 ou 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1798"/>
        <source>invalid unit</source>
        <translation>unité invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1816"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1878"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation>nombre invalide : un entier non signé est attendu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1834"/>
        <source>invalid value</source>
        <translation>valeur invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2439"/>
        <source>usage: set_log &lt;log_level_number_0-4&gt; | &lt;categories&gt;</source>
        <translation>usage : set_log &lt;niveau_de_journalisation_0-4&gt; | &lt;catégories&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2510"/>
        <source>(Y/Yes/N/No): </source>
        <translation>(Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3043"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3070"/>
        <source>bad m_restore_height parameter: </source>
        <translation>mauvais paramètre m_restore_height : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3048"/>
        <source>date format must be YYYY-MM-DD</source>
        <translation>le format de date doit être AAAA-MM-JJ</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3061"/>
        <source>Restore height is: </source>
        <translation>La hauteur de restauration est : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2988"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3062"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4623"/>
        <source>Is this okay?  (Y/Yes/N/No): </source>
        <translation>Est-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3114"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Le démon est local, supposons qu&apos;il est de confiance</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3616"/>
        <source>Password for new watch-only wallet</source>
        <translation>Mot de passe pour le nouveau portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3685"/>
        <source>invalid arguments. Please use start_mining [&lt;number_of_threads&gt;] [do_bg_mining] [ignore_battery], &lt;number_of_threads&gt; should be from 1 to </source>
        <translation>arguments invalides. Veuillez utiliser start_mining [&lt;nombre_de_threads&gt;] [mine_en_arrière_plan] [ignorer_batterie], &lt;nombre_de_threads&gt; devrait être entre 1 et </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3873"/>
        <source>internal error: </source>
        <translation>erreur interne : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1307"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3878"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4171"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue : </translation>
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
        <translation>erreur inconnue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3888"/>
        <source>refresh failed: </source>
        <translation>échec du rafraîchissement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3888"/>
        <source>Blocks received: </source>
        <translation>Blocs reçus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3919"/>
        <source>unlocked balance: </source>
        <translation>solde débloqué : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2418"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4066"/>
        <source>amount</source>
        <translation>montant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="253"/>
        <source>false</source>
        <translation>faux</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="558"/>
        <source>Unknown command: </source>
        <translation>Commande inconnue : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="565"/>
        <source>Command usage: </source>
        <translation>Usage de la commande : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="568"/>
        <source>Command description: </source>
        <translation>Description de la commande : </translation>
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
        <translation>le portefeuille est multisig mais pas encore finalisé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="642"/>
        <source>Enter optional seed encryption passphrase, empty to see raw seed</source>
        <translation>Entrer une phrase de passe facultative pour le chiffrement de la phrase mnémonique, effacer pour voir la phrase mnémonique brute</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="659"/>
        <source>Failed to retrieve seed</source>
        <translation>Échec de la récupération de la phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="683"/>
        <source>wallet is multisig and has no seed</source>
        <translation>le portefeuille est multisig et n&apos;a pas de phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="756"/>
        <source>Cannot connect to daemon</source>
        <translation>Impossible de se connecter au démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="761"/>
        <source>Current fee is %s monero per kB</source>
        <translation>Les frais sont actuellement de %s monero par kO</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="777"/>
        <source>Error: failed to estimate backlog array size: </source>
        <translation>Erreur : échec de l&apos;estimation de la taille du tableau d&apos;arriéré : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="782"/>
        <source>Error: bad estimated backlog array size</source>
        <translation>Erreur : mauvaise estimation de la taille du tableau d&apos;arriéré</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="794"/>
        <source> (current)</source>
        <translation> (actuel)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="797"/>
        <source>%u block (%u minutes) backlog at priority %u%s</source>
        <translation>arriéré de %u bloc(s) (%u minutes) à la priorité %u%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="799"/>
        <source>%u to %u block (%u to %u minutes) backlog at priority %u</source>
        <translation>arriéré de %u à %u bloc(s) (%u à %u minutes) à la priorité %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="802"/>
        <source>No backlog at priority </source>
        <translation>Pas d&apos;arriéré à la priorité </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="816"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="854"/>
        <source>This wallet is already multisig</source>
        <translation>Le portefeuille est déjà multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="821"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="859"/>
        <source>wallet is watch-only and cannot be made multisig</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas être tranformé en multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="827"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="865"/>
        <source>This wallet has been used before, please use a new wallet to create a multisig wallet</source>
        <translation>Ce portefeuille a été utilisé auparavant, veuillez utiliser un nouveau portefeuille pour créer un portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="834"/>
        <source>Your password is incorrect.</source>
        <translation>Votre mot de passe est incorrect.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="840"/>
        <source>Send this multisig info to all other participants, then use make_multisig &lt;threshold&gt; &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Envoyez ces infos multisig à tous les autres participants, ensuite utilisez make_multisig &lt;seuil&gt; &lt;info1&gt; [&lt;info2&gt;...] avec les infos multisig des autres</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="841"/>
        <source>This includes the PRIVATE view key, so needs to be disclosed only to that multisig wallet&apos;s participants </source>
        <translation>Ceci inclut la clé PRIVÉE d&apos;audit, donc ne doit être divulgué qu&apos;aux participants de ce portefeuille multisig </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="878"/>
        <source>usage: make_multisig &lt;threshold&gt; &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</source>
        <translation>usage : make_multisig &lt;seuil&gt; &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="886"/>
        <source>Invalid threshold</source>
        <translation>Seuil invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="899"/>
        <source>Another step is needed</source>
        <translation>Une autre étape est nécessaire</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="901"/>
        <source>Send this multisig info to all other participants, then use finalize_multisig &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation>Envoyez ces infos multisig à tous les autres participants, ensuite utilisez finalize_multisig &lt;info1&gt; [&lt;info2&gt;...] avec les infos multisig des autres</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="907"/>
        <source>Error creating multisig: </source>
        <translation>Erreur de création multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="914"/>
        <source>Error creating multisig: new wallet is not multisig</source>
        <translation>Erreur de création multisig : le nouveau portefeuille n&apos;est pas multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="917"/>
        <source> multisig address: </source>
        <translation> adresse multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="933"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="982"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1034"/>
        <source>This wallet is not multisig</source>
        <translation>Ce portefeuille n&apos;est pas multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="938"/>
        <source>This wallet is already finalized</source>
        <translation>Ce portefeuille est déjà finalisé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="951"/>
        <source>usage: finalize_multisig &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</source>
        <translation>usage : finalize_multisig &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="959"/>
        <source>Failed to finalize multisig</source>
        <translation>Échec de finalisation multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="965"/>
        <source>Failed to finalize multisig: </source>
        <translation>Échec de finalisation multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="987"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1039"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1118"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1191"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1258"/>
        <source>This multisig wallet is not yet finalized</source>
        <translation>Ce portefeuille multisig n&apos;est pas encore finalisé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="992"/>
        <source>usage: export_multisig_info &lt;filename&gt;</source>
        <translation>usage : export_multisig_info &lt;nom_fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1015"/>
        <source>Error exporting multisig info: </source>
        <translation>Erreur d&apos;importation des infos multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1019"/>
        <source>Multisig info exported to </source>
        <translation>Infos multisig exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1044"/>
        <source>usage: import_multisig_info &lt;filename1&gt; [&lt;filename2&gt;...] - one for each other participant</source>
        <translation>usage : import_multisig_info &lt;nom_fichier1&gt; [&lt;nom_fichier2&gt;...] - un pour chaque autre participant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1072"/>
        <source>Multisig info imported</source>
        <translation>Infos multisig importées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1076"/>
        <source>Failed to import multisig info: </source>
        <translation>Échec de l&apos;importation des infos multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1087"/>
        <source>Failed to update spent status after importing multisig info: </source>
        <translation>Échec de la mise à jour de l&apos;état des dépenses après l&apos;importation des infos multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1092"/>
        <source>Untrusted daemon, spent status may be incorrect. Use a trusted daemon and run &quot;rescan_spent&quot;</source>
        <translation>Pas un démon de confiance, l&apos;état des dépenses peut être incorrect. Utilisez un démon de confiance et executez &quot;rescan_spent&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1113"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1186"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1253"/>
        <source>This is not a multisig wallet</source>
        <translation>Ceci n&apos;est pas un portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1123"/>
        <source>usage: sign_multisig &lt;filename&gt;</source>
        <translation>usage : sign_multisig &lt;nom_fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1136"/>
        <source>Failed to sign multisig transaction</source>
        <translation>Échec de la signature de la transaction multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1142"/>
        <source>Multisig error: </source>
        <translation>Erreur multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1147"/>
        <source>Failed to sign multisig transaction: </source>
        <translation>Échec de la signature de la transaction multisig : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1170"/>
        <source>It may be relayed to the network with submit_multisig</source>
        <translation>Elle peut être transmise au réseau avec submit_multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1196"/>
        <source>usage: submit_multisig &lt;filename&gt;</source>
        <translation>usage : submit_multisig &lt;nom_fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1211"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1277"/>
        <source>Failed to load multisig transaction from file</source>
        <translation>Échec du chargement de la transaction multisig du fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1216"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1282"/>
        <source>Multisig transaction signed by only %u signers, needs %u more signatures</source>
        <translation>Transaction multisig signée par %u signataire(s) seulement, nécessite %u signature(s) de plus</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1225"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7453"/>
        <source>Transaction successfully submitted, transaction </source>
        <translation>Transaction transmise avec succès, transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1226"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7454"/>
        <source>You can check its status by using the `show_transfers` command.</source>
        <translation>Vous pouvez vérifier son statut en utilisant la commane &apos;show_transfers&apos;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1263"/>
        <source>usage: export_raw_multisig &lt;filename&gt;</source>
        <translation>usage : export_raw_multisig &lt;nom_fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1298"/>
        <source>Failed to export multisig transaction to file </source>
        <translation>Échec de l&apos;exportation de la transaction multisig vers le fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1302"/>
        <source>Saved exported multisig transaction file(s): </source>
        <translation>Transaction multisig enregistrée dans le(s) fichier(s) : </translation>
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
        <translation>la taille de cercle doit être un nombre entier &gt;= </translation>
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
        <translation>échec du changement de la taille de cercle par défaut</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1912"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1983"/>
        <source>Invalid height</source>
        <translation>Hauteur invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2016"/>
        <source>start_mining [&lt;number_of_threads&gt;] [bg_mining] [ignore_battery]</source>
        <translation>start_mining [&lt;nombre_de_threads&gt;] [mine_arrière_plan] [ignorer_batterie]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2017"/>
        <source>Start mining in the daemon (bg_mining and ignore_battery are optional booleans).</source>
        <translation>Démarrer la mine dans le démon (mine_arrière_plan et ignorer_batterie sont des booléens facultatifs).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2020"/>
        <source>Stop mining in the daemon.</source>
        <translation>Arrêter la mine dans le démon.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2023"/>
        <source>set_daemon &lt;host&gt;[:&lt;port&gt;]</source>
        <translation>set_daemon &lt;hôte&gt;[:&lt;port&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2024"/>
        <source>Set another daemon to connect to.</source>
        <translation>Spécifier un autre démon auquel se connecter.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2027"/>
        <source>Save the current blockchain data.</source>
        <translation>Sauvegarder les données actuelles de la châine de blocs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2030"/>
        <source>Synchronize the transactions and balance.</source>
        <translation>Synchroniser les transactions et le solde.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2033"/>
        <source>balance [detail]</source>
        <translation>solde [détail]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2034"/>
        <source>Show the wallet&apos;s balance of the currently selected account.</source>
        <translation>Afficher le solde du compte actuellement sélectionné.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2037"/>
        <source>incoming_transfers [available|unavailable] [verbose] [index=&lt;N1&gt;[,&lt;N2&gt;[,...]]]</source>
        <translation>incoming_transfers [available|unavailable] [verbose] [index=&lt;N1&gt;[,&lt;N2&gt;[,...]]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2038"/>
        <source>Show the incoming transfers, all or filtered by availability and address index.</source>
        <translation>Afficher les transferts entrants, tous ou filtrés par disponibilité et index d&apos;adresse.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2041"/>
        <source>payments &lt;PID_1&gt; [&lt;PID_2&gt; ... &lt;PID_N&gt;]</source>
        <translation>payments &lt;PID_1&gt; [&lt;PID_2&gt; ... &lt;PID_N&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2042"/>
        <source>Show the payments for the given payment IDs.</source>
        <translation>Afficher les paiements pour les IDs de paiement donnés.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2045"/>
        <source>Show the blockchain height.</source>
        <translation>Afficher la hauteur de la chaîne de blocs.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2048"/>
        <source>transfer_original [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation>transfer_original [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] &lt;adresse&gt; &lt;montant&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2049"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; using an older transaction building algorithm. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Transférer &lt;montant&gt; à &lt;adresse&gt; en utilisant un algorithme de construction de transaction plus ancien. Si le paramètre &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille utilise les sorties reçues par les adresses de ces indices. Si il est omis, le portefeuille choisit les indices d&apos;adresse à utiliser aléatoirement. Dans tous les cas, il essaye de ne pas combiner des sorties de multiples adresses. &lt;priorité&gt; est la priorité de la transaction. Plus la priorité est haute, plus les frais de transaction sont élévés. Les valeurs valides par ordre de priorité (de la plus basse à la plus haute) sont : unimportant, normal, elevated, priority. Si elle est omise, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;taille_cercle&gt; est le nombre d&apos;entrées à inclure pour l&apos;intraçabilité. De multiples paiements peuvent être réalisés d&apos;un coup en ajoutant &lt;adresse_2&gt; &lt;montant_2&gt; et cetera (avant l&apos;ID de paiement, si il est inclus)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2051"/>
        <source>transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation>transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] &lt;adresse&gt; &lt;montant&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2052"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt;. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Transférer &lt;montant&gt; à &lt;adresse&gt; Si le paramètre &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille utilise les sorties reçues par les adresses de ces indices. Si il est omis, le portefeuille choisit les indices d&apos;adresse à utiliser aléatoirement. Dans tous les cas, il essaye de ne pas combiner des sorties de multiples adresses. &lt;priorité&gt; est la priorité de la transaction. Plus la priorité est haute, plus les frais de transaction sont élévés. Les valeurs valides par ordre de priorité (de la plus basse à la plus haute) sont : unimportant, normal, elevated, priority. Si elle est omise, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;taille_cercle&gt; est le nombre d&apos;entrées à inclure pour l&apos;intraçabilité. De multiples paiements peuvent être réalisés d&apos;un coup en ajoutant &lt;adresse_2&gt; &lt;montant_2&gt; et cetera (avant l&apos;ID de paiement, si il est inclus)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2055"/>
        <source>locked_transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;addr&gt; &lt;amount&gt; &lt;lockblocks&gt; [&lt;payment_id&gt;]</source>
        <translation>locked_transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] &lt;adresse&gt; &lt;montant&gt; &lt;blocs_verrou&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2056"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>Transférer &lt;montant&gt; à &lt;adresse&gt; et le verrouiller pendant &lt;blocs_verrou&gt; (max 1000000). Si le paramètre &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille utilise les sorties reçues par les adresses de ces indices. Si il est omis, le portefeuille choisit les indices d&apos;adresse à utiliser aléatoirement. Dans tous les cas, il essaye de ne pas combiner des sorties de multiples adresses. &lt;priorité&gt; est la priorité de la transaction. Plus la priorité est haute, plus les frais de transaction sont élévés. Les valeurs valides par ordre de priorité (de la plus basse à la plus haute) sont : unimportant, normal, elevated, priority. Si elle est omise, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;taille_cercle&gt; est le nombre d&apos;entrées à inclure pour l&apos;intraçabilité. De multiples paiements peuvent être réalisés d&apos;un coup en ajoutant &lt;adresse_2&gt; &lt;montant_2&gt; et cetera (avant l&apos;ID de paiement, si il est inclus)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2059"/>
        <source>Send all unmixable outputs to yourself with ring_size 1</source>
        <translation>Envoyer toutes les sorties non mélangeables à vous-même avec une taille de cercle de 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2061"/>
        <source>sweep_all [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation>sweep_all [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] &lt;adresse&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2062"/>
        <source>Send all unlocked balance to an address. If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used.</source>
        <translation>Envoyer tout le solde débloqué à une adresse. Si le paramètre &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; est spécifié, le portefeuille balaye les sorties reçues par ces indices d&apos;adresse. Si il est omis, le portefeuille choisit un index d&apos;adresse à utiliser aléatoirement.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2065"/>
        <source>sweep_below &lt;amount_threshold&gt; [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation>sweep_below &lt;montant_seuil&gt; [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] &lt;adresse&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2066"/>
        <source>Send all unlocked outputs below the threshold to an address.</source>
        <translation>Envoyer toutes les sorties débloquées d&apos;un montant inférieur au seuil à une adresse.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2069"/>
        <source>sweep_single [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;key_image&gt; &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation>sweep_single [&lt;priorité&gt;] [&lt;taille_cercle&gt;] &lt;image_clé&gt; &lt;adresse&gt; [&lt;ID_payment&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2070"/>
        <source>Send a single output of the given key image to an address without change.</source>
        <translation>Envoyer une unique sortie ayant une image de clé donnée à une adresse sans rendu de monnaie.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2073"/>
        <source>donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation>donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priorité&gt;] [&lt;taille_cercle&gt;] &lt;montant&gt; [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2074"/>
        <source>Donate &lt;amount&gt; to the development team (donate.getmonero.org).</source>
        <translation>Donner &lt;montant&gt; à l&apos;équipe de développement (donate.getmonero.org).</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2077"/>
        <source>sign_transfer &lt;file&gt;</source>
        <translation>sign_transfer &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2078"/>
        <source>Sign a transaction from a &lt;file&gt;.</source>
        <translation>Signer une transaction d&apos;un &lt;fichier&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2081"/>
        <source>Submit a signed transaction from a file.</source>
        <translation>Transmettre une transaction signée d&apos;un fichier.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2084"/>
        <source>set_log &lt;level&gt;|{+,-,}&lt;categories&gt;</source>
        <translation>set_log &lt;niveau&gt;|{+,-,}&lt;catégories&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2085"/>
        <source>Change the current log detail (level must be &lt;0-4&gt;).</source>
        <translation>Changer le niveau de détail du journal (le niveau doit être &lt;0-4&gt;).</translation>
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
  account new &lt;texte étiquette avec espaces autorisés&gt;
  account switch &lt;index&gt; 
  account label &lt;index&gt; &lt;texte étiquette avec espaces autorisés&gt;
  account tag &lt;mot_clé&gt; &lt;index_compte_1&gt; [&lt;index_compte_2&gt; ...]
  account untag &lt;index_compte_1&gt; [&lt;index_compte_2&gt; ...]
  account tag_description &lt;mot_clé&gt; &lt;description&gt;</translation>
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
        <translation>Si aucun argument n&apos;est spécifié, le portefeuille affiche tous les comptes existants ainsi que leurs soldes.
Si l&apos;argument &quot;new&quot; est spécifié, le portefeuille crée un nouveau compte avec son étiquette initialisée par le texte fourni (qui peut être vide).
Si l&apos;argument &quot;switch&quot; est spécifié, le portefeuille passe au compte spécifié par &lt;index&gt;.
Si l&apos;argument &quot;label&quot; est spécifié, le portefeuille affecte le texte fourni à l&apos;étiquette du compte spécifié par &lt;index&gt;.
Si l&apos;argument &quot;tag&quot; est spécifié, un mot clé &lt;mot_clé&gt; est assigné aux comptes spécifiés &lt;account_index_1&gt;, &lt;account_index_2&gt;, ....
Si l&apos;argument &quot;untag&quot; est spécifié, les mots clés assignés aux comptes spécifiés &lt;account_index_1&gt;, &lt;account_index_2&gt; ..., sont supprimés.
Si l&apos;argument &quot;tag_description&quot; est spécifié, le texte arbitraire &lt;description&gt; est assigné au mot clé &lt;mot_clé&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2104"/>
        <source>address [ new &lt;label text with white spaces allowed&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;label text with white spaces allowed&gt;]</source>
        <translation>address [ new &lt;texte étiquette avec espaces autorisés&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;texte étiquette avec espaces autorisés&gt;]</translation>
    </message>
    <message>
        <source>If no arguments are specified or &lt;index&gt; is specified, the wallet shows the default or specified address. If &quot;all&quot; is specified, the walllet shows all the existing addresses in the currently selected account. If &quot;new &quot; is specified, the wallet creates a new address with the provided label text (which can be empty). If &quot;label&quot; is specified, the wallet sets the label of the address specified by &lt;index&gt; to the provided label text.</source>
        <translation type="vanished">Si aucun argument n&apos;est spécifié ou si &lt;index&gt; est spécifié, le portefeuille affiche l&apos;adresse par défaut ou l&apos;adresse spécifiée. Si &quot;all&quot; est spécifié, le portefeuille affiche toutes les adresses existantes dans le comptes actuellement sélectionné. Si &quot;new&quot; est spécifié, le portefeuille crée une nouvelle adresse avec le texte d&apos;étiquette fourni (qui peut être vide). Si &quot;label&quot; est spécifié, le portefeuille affecte le texte fourni à l&apos;étiquette de l&apos;adresse spécifiée par &lt;index&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2108"/>
        <source>integrated_address [&lt;payment_id&gt; | &lt;address&gt;]</source>
        <translation>integrated_address [&lt;ID_paiement&gt; | &lt;adresse&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2109"/>
        <source>Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation>Encoder un ID de paiement dans une adresse intégrée pour l&apos;adresse publique du portefeuille actuel (en l&apos;absence d&apos;argument un ID de paiement aléatoire est utilisé), ou décoder une adresse intégrée en une adresse standard et un ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2112"/>
        <source>address_book [(add ((&lt;address&gt; [pid &lt;id&gt;])|&lt;integrated address&gt;) [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</source>
        <translation>address_book [(add ((&lt;adresse&gt; [pid &lt;id&gt;])|&lt;adresse intégrée&gt;) [&lt;description avec éventuellement des espaces&gt;])|(delete &lt;index&gt;)]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2113"/>
        <source>Print all entries in the address book, optionally adding/deleting an entry to/from it.</source>
        <translation>Afficher toutes les entrées du carnet d&apos;adresses, optionnellement en y ajoutant/supprimant une entrée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2116"/>
        <source>Save the wallet data.</source>
        <translation>Sauvegarder les données du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2119"/>
        <source>Save a watch-only keys file.</source>
        <translation>Sauvegarder un fichier de clés d&apos;audit.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2122"/>
        <source>Display the private view key.</source>
        <translation>Afficher la clé privée d&apos;audit.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2125"/>
        <source>Display the private spend key.</source>
        <translation>Afficher la clé privée de dépense.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2128"/>
        <source>Display the Electrum-style mnemonic seed</source>
        <translation>Afficher la phrase mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2131"/>
        <source>set &lt;option&gt; [&lt;value&gt;]</source>
        <translation>set &lt;option&gt; [&lt;valeur&gt;]</translation>
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
        <translation type="vanished">Options disponibles :
 seed langue
   Définir la langue de la phrase mnémonique.
 always-confirm-transfers &lt;1|0&gt;
   Confirmation des transactions non divisées.
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
 ask-password &lt;1|0&gt;
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
   Utilisation automatique du niveau de frais pour la priorité basse, lorsqu&apos;il est sûr de le faire.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2178"/>
        <source>Display the encrypted Electrum-style mnemonic seed.</source>
        <translation>Afficher la phrase mnémonique de style Electrum chiffrée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2181"/>
        <source>Rescan the blockchain for spent outputs.</source>
        <translation>Rescanner la chaîne de blocs pour trouver les sorties dépensées.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2184"/>
        <source>get_tx_key &lt;txid&gt;</source>
        <translation>get_tx_key &lt;ID_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2185"/>
        <source>Get the transaction key (r) for a given &lt;txid&gt;.</source>
        <translation>Obtenir la clé de transaction (r) pour un &lt;ID_transaction&gt; donné.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2188"/>
        <source>check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</source>
        <translation>check_tx_key &lt;ID_transaction&gt; &lt;clé_transaction&gt; &lt;adresse&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2189"/>
        <source>Check the amount going to &lt;address&gt; in &lt;txid&gt;.</source>
        <translation>Vérifier le montant allant à &lt;adresse&gt; dans &lt;ID_transaction&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2192"/>
        <source>get_tx_proof &lt;txid&gt; &lt;address&gt; [&lt;message&gt;]</source>
        <translation>get_tx_proof &lt;ID_transaction&gt; &lt;adresse&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2193"/>
        <source>Generate a signature proving funds sent to &lt;address&gt; in &lt;txid&gt;, optionally with a challenge string &lt;message&gt;, using either the transaction secret key (when &lt;address&gt; is not your wallet&apos;s address) or the view secret key (otherwise), which does not disclose the secret key.</source>
        <translation>Générer une signature prouvant l&apos;envoi de fonds à &lt;adresse&gt; dans &lt;ID_transaction&gt;, optionnellement avec un &lt;message&gt; comme challenge, en utilisant soit la clé secrète de transaction (quand &lt;adresse&gt; n&apos;est pas l&apos;adresse de votre portefeuille) soit la clé secrète d&apos;audit (dans le cas contraire), tout en ne divulgant pas la clé secrète.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2196"/>
        <source>check_tx_proof &lt;txid&gt; &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>check_tx_proof &lt;ID_transaction&gt; &lt;adresse&gt; &lt;fichier_signature&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2197"/>
        <source>Check the proof for funds going to &lt;address&gt; in &lt;txid&gt; with the challenge string &lt;message&gt; if any.</source>
        <translation>Vérifier la validité de la preuve de fonds allant à &lt;adresse&gt; dans &lt;ID_transaction&gt; avec le &lt;message&gt; de challenge s&apos;il y en a un.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2200"/>
        <source>get_spend_proof &lt;txid&gt; [&lt;message&gt;]</source>
        <translation>get_spend_proof &lt;ID_transaction&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2201"/>
        <source>Generate a signature proving that you generated &lt;txid&gt; using the spend secret key, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Générer une signature prouvant que vous avez créé &lt;ID_transaction&gt; en utilisant la clé secrète de dépense, optionnellement avec un &lt;message&gt; comme challenge.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2204"/>
        <source>check_spend_proof &lt;txid&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>check_spend_proof &lt;ID_transaction&gt; &lt;fichier_signature&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2205"/>
        <source>Check a signature proving that the signer generated &lt;txid&gt;, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Vérifier la validité de la preuve que le signataire a créé &lt;ID_transaction&gt;, optionnellement avec un &lt;message&gt; comme challenge.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2208"/>
        <source>get_reserve_proof (all|&lt;amount&gt;) [&lt;message&gt;]</source>
        <translation>get_reserve_proof (all|&lt;montant&gt;) [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2209"/>
        <source>Generate a signature proving that you own at least this much, optionally with a challenge string &lt;message&gt;.
If &apos;all&apos; is specified, you prove the entire sum of all of your existing accounts&apos; balances.
Otherwise, you prove the reserve of the smallest possible amount above &lt;amount&gt; available in your current account.</source>
        <translation>Générer une signature prouvant que vous possédez au moins ce montant, optionnellement avec un &lt;message&gt; comme challenge.
Si &apos;all&apos; est spécifié, vous prouvez la somme totale des soldes de tous vos comptes existants.
Sinon, vous prouvez le plus petit solde supérieur à &lt;montant&gt; dans votre compte actuel.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2214"/>
        <source>check_reserve_proof &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>check_reserve_proof &lt;adresse&gt; &lt;fichier_signature&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2215"/>
        <source>Check a signature proving that the owner of &lt;address&gt; holds at least this much, optionally with a challenge string &lt;message&gt;.</source>
        <translation>Vérifier la validité d&apos;une signature prouvant que le propriétaire d&apos;une &lt;adresse&gt; détient au moins un montant, optionnellement avec un &lt;message&gt; comme challenge.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2218"/>
        <source>show_transfers [in|out|pending|failed|pool] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation>show_transfers [in|out|pending|failed|pool] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;hauteur_min&gt; [&lt;hauteur_max&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2219"/>
        <source>Show the incoming/outgoing transfers within an optional height range.</source>
        <translation>Afficher les transferts entrants/sortants dans un interval de hauteurs facultatif.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2222"/>
        <source>unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_amount&gt; [&lt;max_amount&gt;]]</source>
        <translation>unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;montant_min&gt; [&lt;montant_max&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2223"/>
        <source>Show the unspent outputs of a specified address within an optional amount range.</source>
        <translation>Afficher les sorties non dépensées d&apos;une adresse spécifique dans un interval de montants facultatif.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2226"/>
        <source>Rescan the blockchain from scratch.</source>
        <translation>Rescanner la chaîne de blocs en partant de zéro.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2229"/>
        <source>set_tx_note &lt;txid&gt; [free text note]</source>
        <translation>set_tx_note &lt;ID_transaction&gt; [texte de la note]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2230"/>
        <source>Set an arbitrary string note for a &lt;txid&gt;.</source>
        <translation>Définir un texte arbitraire comme note pour &lt;ID_transaction&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2233"/>
        <source>get_tx_note &lt;txid&gt;</source>
        <translation>get_tx_note &lt;ID_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2234"/>
        <source>Get a string note for a txid.</source>
        <translation>Obtenir le texte de la note pour &lt;ID_transaction&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2237"/>
        <source>set_description [free text note]</source>
        <translation>set_description [texte]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2238"/>
        <source>Set an arbitrary description for the wallet.</source>
        <translation>Définir un texte arbitraire comme description du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2241"/>
        <source>Get the description of the wallet.</source>
        <translation>Obtenir la description du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2244"/>
        <source>Show the wallet&apos;s status.</source>
        <translation>Afficher l&apos;état du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2247"/>
        <source>Show the wallet&apos;s information.</source>
        <translation>Afficher les informations du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2250"/>
        <source>sign &lt;file&gt;</source>
        <translation>sign &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2251"/>
        <source>Sign the contents of a file.</source>
        <translation>Signer le contenu d&apos;un fichier.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2254"/>
        <source>verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation>verify &lt;fichier&gt; &lt;adresse&gt; &lt;signature&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2255"/>
        <source>Verify a signature on the contents of a file.</source>
        <translation>Vérifier la signature du contenu d&apos;un fichier.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2258"/>
        <source>export_key_images &lt;file&gt;</source>
        <translation>export_key_images &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2259"/>
        <source>Export a signed set of key images to a &lt;file&gt;.</source>
        <translation>Exported un ensemble signé d&apos;images de clé vers un &lt;fichier&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2262"/>
        <source>import_key_images &lt;file&gt;</source>
        <translation>import_key_images &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2263"/>
        <source>Import a signed key images list and verify their spent status.</source>
        <translation>Importer un ensemble signé d&apos;images de clé et vérifier si elles correspondent à des dépenses.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2266"/>
        <source>export_outputs &lt;file&gt;</source>
        <translation>export_outputs &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2267"/>
        <source>Export a set of outputs owned by this wallet.</source>
        <translation>Exporter un ensemble de sorties possédées par ce portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2270"/>
        <source>import_outputs &lt;file&gt;</source>
        <translation>import_outputs &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2271"/>
        <source>Import a set of outputs owned by this wallet.</source>
        <translation>Importer un ensemble de sorties possédées par ce portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2274"/>
        <source>show_transfer &lt;txid&gt;</source>
        <translation>show_transfer &lt;ID_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2275"/>
        <source>Show information about a transfer to/from this address.</source>
        <translation>Afficher les information à propos d&apos;un transfert vers/depuis cette adresse.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2278"/>
        <source>Change the wallet&apos;s password.</source>
        <translation>Changer le mot de passe du portefeuille.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2281"/>
        <source>Generate a new random full size payment id. These will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids.</source>
        <translation>Générer un nouvel ID de paiement long aléatoire. Ceux-ci sont en clair dans la chaîne de blocs, voir integrated_address pour les IDs de paiement courts cryptés.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2284"/>
        <source>Print the information about the current fee and transaction backlog.</source>
        <translation>Afficher les informations à propos des frais et arriéré de transactions actuels.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2286"/>
        <source>Export data needed to create a multisig wallet</source>
        <translation>Exporter les données nécessaires pour créer un portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2288"/>
        <source>make_multisig &lt;threshold&gt; &lt;string1&gt; [&lt;string&gt;...]</source>
        <translation>make_multisig &lt;seuil&gt; &lt;chaîne_caractères1&gt; [&lt;chaîne_caractères&gt;...]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2289"/>
        <source>Turn this wallet into a multisig wallet</source>
        <translation>Transformer ce portefeuille en portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2292"/>
        <source>finalize_multisig &lt;string&gt; [&lt;string&gt;...]</source>
        <translation>finalize_multisig &lt;chaîne_caractères&gt; [&lt;chaîne_caractères&gt;...]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2293"/>
        <source>Turn this wallet into a multisig wallet, extra step for N-1/N wallets</source>
        <translation>Transformer ce portefeuille en portefeuille multisig, étape supplémentaire pour les portefeuilles N-1/N</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2296"/>
        <source>export_multisig_info &lt;filename&gt;</source>
        <translation>export_multisig_info &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2297"/>
        <source>Export multisig info for other participants</source>
        <translation>Exporter les infos multisig pour les autres participants</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2300"/>
        <source>import_multisig_info &lt;filename&gt; [&lt;filename&gt;...]</source>
        <translation>import_multisig_info &lt;fichier&gt; [&lt;fichier&gt;...]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2301"/>
        <source>Import multisig info from other participants</source>
        <translation>Importer les infos multisig des autres participants</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2304"/>
        <source>sign_multisig &lt;filename&gt;</source>
        <translation>sign_multisig &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2305"/>
        <source>Sign a multisig transaction from a file</source>
        <translation>Signer une transaction multisig d&apos;un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2308"/>
        <source>submit_multisig &lt;filename&gt;</source>
        <translation>submit_multisig &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2309"/>
        <source>Submit a signed multisig transaction from a file</source>
        <translation>Transmettre une transaction multisig signée d&apos;un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2312"/>
        <source>export_raw_multisig_tx &lt;filename&gt;</source>
        <translation>export_raw_multisig_tx &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2313"/>
        <source>Export a signed multisig transaction to a file</source>
        <translation>Exporter une transaction multisig signée vers un fichier</translation>
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
        <translation>help [&lt;commande&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2341"/>
        <source>Show the help section or the documentation about a &lt;command&gt;.</source>
        <translation>Afficher la section d&apos;aide ou la documentation d&apos;une &lt;commande&gt;.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2410"/>
        <source>integer &gt;= </source>
        <translation>entier &gt;= </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2423"/>
        <source>block height</source>
        <translation>hauteur de bloc</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2427"/>
        <source>&lt;major&gt;:&lt;minor&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2509"/>
        <source>No wallet found with that name. Confirm creation of new wallet named: </source>
        <translation>Aucun portefeuille avec ce nom trouvé. Confirmer la création d&apos;un nouveau portefeuille nommé : </translation>
    </message>
    <message>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot; and --generate-from-json=&quot;jsonfilename&quot;</source>
        <translation type="vanished">impossible de spécifier plus d&apos;une option parmis --generate-new-wallet=&quot;nom_portefeuille&quot;, --wallet-file=&quot;nom_portefeuille&quot;, --generate-from-view-key=&quot;nom_portefeuille&quot;, --generate-from-spend-key=&quot;nom_portefeuille&quot;, --generate-from-keys=&quot;nom_portefeuille&quot;, --generate-from-multisig-keys=&quot;nom_portefeuille&quot; and --generate-from-json=&quot;nom_fichier_json&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2593"/>
        <source>can&apos;t specify both --restore-deterministic-wallet or --restore-multisig-wallet and --non-deterministic</source>
        <translation>impossible de spécifier à la fois --restore-deterministic-wallet ou --restore-multisig-wallet et --non-deterministic</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2599"/>
        <source>--restore-multisig-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-multisig-wallet utilise --generate-new-wallet, pas --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2615"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;multisig seed here&quot;</source>
        <translation>spécifiez un paramètre de récupération avec --electrum-seed=&quot;phrase mnémonique multisig ici&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2642"/>
        <source>Multisig seed failed verification</source>
        <translation>Échec de la vérification de la phrase mnémonique multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2655"/>
        <source>Enter seed encryption passphrase, empty if none</source>
        <translation>Entrer une phrase de passe pour le chiffrement de la phrase mnémonique, vide si aucune</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2691"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2765"/>
        <source>This address is a subaddress which cannot be used here.</source>
        <translation>Cette adresse est une sous-adresse qui ne peut pas être utilisée ici.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2843"/>
        <source>Error: expected M/N, but got: </source>
        <translation>Erreur : M/N attendu, mais lu : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2848"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got: </source>
        <translation>Erreur : N &gt; 1 et N &lt;= M attendu, mais lu : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2853"/>
        <source>Error: M/N is currently unsupported. </source>
        <translation>Erreur : M/N n&apos;est actuellement pas supporté. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2856"/>
        <source>Generating master wallet from %u of %u multisig wallet keys</source>
        <translation>Génération du portefeuille principal à partir de %u de %u clés de portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2885"/>
        <source>failed to parse secret view key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2894"/>
        <source>failed to verify secret view key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2914"/>
        <source>Secret spend key (%u of %u):</source>
        <translation>Clé secrète de dépense (%u de %u) :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2938"/>
        <source>Error: M/N is currently unsupported</source>
        <translation>Erreur : M/N n&apos;est actuellement pas supporté</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3084"/>
        <source>Restore height </source>
        <translation>Hauteur de restauration </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3085"/>
        <source>Still apply restore height?  (Y/Yes/N/No): </source>
        <translation>Appliquer la hauteur de restauration quand même ?  (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3097"/>
        <source>can&apos;t specify --subaddress-lookahead and --wallet-file at the same time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3121"/>
        <source>Warning: using an untrusted daemon at %s, privacy will be lessened</source>
        <translation>Attention : en n&apos;utilisant %s qui n&apos;est pas un démon de confiance, la confidentialité sera réduite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3181"/>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or change the daemon address using the &apos;set_daemon&apos; command.</source>
        <translation>Le démon n&apos;est pas lancé ou un mauvais port a été fourni. Veuillez vous assurer que le démon fonctionne ou changez l&apos;adresse de démon avec la commande &apos;set_daemon&apos;.</translation>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="3459"/>
        <source>failed to generate new mutlisig wallet</source>
        <translation>échec de la génération du nouveau portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3462"/>
        <source>Generated new %u/%u multisig wallet: </source>
        <translation>Nouveau portefeuille multisig %u/%u généré : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3498"/>
        <source>Opened %u/%u multisig wallet%s</source>
        <translation>Portefeuille multisig %u/%u ouvert%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3554"/>
        <source>Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
</source>
        <translation>Utilisez &quot;help &lt;commande&gt;&quot; pour voir la documentation d&apos;une commande.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3612"/>
        <source>wallet is multisig and cannot save a watch-only version</source>
        <translation>c&apos;est un portefeuille multisig et il ne peut pas sauvegarder une version d&apos;audit</translation>
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
        <translation>URL du démon manquante en argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3739"/>
        <source>Unexpected array length - Exited simple_wallet::set_daemon()</source>
        <translation>Taille de tableau inattendue - Sortie de simple_wallet::set_daemon()</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3753"/>
        <source>This does not seem to be a valid daemon URL.</source>
        <translation>Ceci semble ne pas être une URL de démon valide.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3789"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3807"/>
        <source>txid </source>
        <translation>ID transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3791"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3809"/>
        <source>idx </source>
        <translation>index </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3914"/>
        <source> (Some owned outputs have partial key images - import_multisig_info needed)</source>
        <translation> (Certaines sorties ont des images de clé partielles - import_multisig_info requis)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3915"/>
        <source>Currently selected account: [</source>
        <translation>Compte actuellement sélectionné : [</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3915"/>
        <source>] </source>
        <translation>] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3917"/>
        <source>Tag: </source>
        <translation>Mot clé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3917"/>
        <source>(No tag assigned)</source>
        <translation>(Pas de mot clé assigné)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3924"/>
        <source>Balance per address:</source>
        <translation>Solde par adresse :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3925"/>
        <source>Address</source>
        <translation>Adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3925"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6593"/>
        <source>Balance</source>
        <translation>Solde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3925"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6593"/>
        <source>Unlocked balance</source>
        <translation>Solde débloqué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3925"/>
        <source>Outputs</source>
        <translation>Sorties</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3925"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6593"/>
        <source>Label</source>
        <translation>Étiquette</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3933"/>
        <source>%8u %6s %21s %21s %7u %21s</source>
        <translation>%8u %6s %21s %21s %7u %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3942"/>
        <source>usage: balance [detail]</source>
        <translation>usage : balance [detail]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3954"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3996"/>
        <source>usage: incoming_transfers [available|unavailable] [verbose] [index=&lt;N&gt;]</source>
        <translation>usage: incoming_transfers [available|unavailable] [verbose] [index=&lt;N&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <source>spent</source>
        <translation>dépensé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <source>global index</source>
        <translation>index global</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <source>tx id</source>
        <translation>ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4066"/>
        <source>addr index</source>
        <translation>index adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4038"/>
        <source>No incoming transfers</source>
        <translation>Aucun transfert entrant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4042"/>
        <source>No incoming available transfers</source>
        <translation>Aucun transfert entrant disponible</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4046"/>
        <source>No incoming unavailable transfers</source>
        <translation>Aucun transfert entrant non disponible</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4057"/>
        <source>expected at least one payment ID</source>
        <translation>au moins un ID de paiement attendu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4066"/>
        <source>payment</source>
        <translation>paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4066"/>
        <source>transaction</source>
        <translation>transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4066"/>
        <source>height</source>
        <translation>hauteur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4066"/>
        <source>unlock time</source>
        <translation>durée de déverrouillage</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4078"/>
        <source>No payments with id </source>
        <translation>Aucun paiement avec l&apos;ID </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4131"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4197"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4479"/>
        <source>failed to get blockchain height: </source>
        <translation>échec de la récupération de la hauteur de la chaîne de blocs : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4187"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5804"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5842"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5899"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5932"/>
        <source>failed to connect to the daemon</source>
        <translation>échec de la connexion au démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4205"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation>
Transaction %llu/%llu : ID=%s</translation>
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
Entrée %llu/%llu : montant=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4242"/>
        <source>failed to get output: </source>
        <translation>échec de la récupération de la sortie : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4250"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation>la hauteur du bloc d&apos;origine de la clé de la sortie ne devrait pas être supérieure à celle de la chaîne de blocs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4254"/>
        <source>
Originating block heights: </source>
        <translation>
Hauteurs des blocs d&apos;origine : </translation>
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
Attention : Certaines clés d&apos;entrées étant dépensées sont issues de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4288"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation>, ce qui peut casser l&apos;anonymat du cercle de signature. Assurez-vous que c&apos;est intentionnel !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4331"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4827"/>
        <source>Ring size must not be 0</source>
        <translation>La taille de cercle ne doit pas être 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4343"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4839"/>
        <source>ring size %u is too small, minimum is %u</source>
        <translation>la taille de cercle %u est trop petite, le minimum est %u</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4350"/>
        <source>wrong number of arguments</source>
        <translation>mauvais nombre d&apos;arguments</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4456"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4909"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5122"/>
        <source>No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): </source>
        <translation>Aucun ID de paiement n&apos;est inclus dans cette transaction. Est-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4498"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4929"/>
        <source>No outputs found, or daemon is not ready</source>
        <translation>Aucune sortie trouvée, ou le démon n&apos;est pas prêt</translation>
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
        <translation>Transaction sauvegardée avec succès dans </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7446"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7448"/>
        <source>, txid </source>
        <translation>, ID transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7448"/>
        <source>Failed to save transaction to </source>
        <translation>Échec de la sauvegarde de la transaction dans </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4724"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4957"/>
        <source>Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Balayage de %s dans %llu transactions pour des frais totaux de %s. Est-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4730"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4963"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5162"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Balayage de %s pour des frais totaux de %s. Est-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5273"/>
        <source>Donating </source>
        <translation>Don de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5440"/>
        <source>This is a watch only wallet</source>
        <translation>Ceci est un portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7274"/>
        <source>usage: show_transfer &lt;txid&gt;</source>
        <translation>usage : show_transfer &lt;ID_de_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7376"/>
        <source>Double spend seen on the network: this transaction may or may not end up being mined</source>
        <translation>Double dépense détectée sur le réseau : cette transaction sera peut-être invalidée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7411"/>
        <source>Transaction ID not found</source>
        <translation>ID de transaction non trouvé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="248"/>
        <source>true</source>
        <translation>vrai</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="301"/>
        <source>failed to parse refresh type</source>
        <translation>échec de l&apos;analyse du type de rafraîchissement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="619"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="688"/>
        <source>wallet is watch-only and has no seed</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il n&apos;a pas de phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="635"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="693"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation>c&apos;est un portefeuille non déterministe et il n&apos;a pas de phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1617"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1636"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas transférer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1715"/>
        <source>could not change default priority</source>
        <translation>échec du changement de la priorité par défaut</translation>
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
        <translation>full (le plus lent, aucune supposition); optimize-coinbase (rapide, suppose que la récompense de bloc est payée à une unique adresse); no-coinbase (le plus rapide, suppose que l&apos;on ne reçoit aucune récompense de bloc), default (comme optimize-coinbase)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2416"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation>monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2472"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation>Nom de portefeuille non valide. Veuillez réessayer ou utilisez Ctrl-C pour quitter.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2489"/>
        <source>Wallet and key files found, loading...</source>
        <translation>Fichier portefeuille et fichier de clés trouvés, chargement...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2495"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation>Fichier de clés trouvé mais pas le fichier portefeuille. Régénération...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2501"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation>Fichier de clés non trouvé. Échec de l&apos;ouverture du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2520"/>
        <source>Generating new wallet...</source>
        <translation>Génération du nouveau portefeuille...</translation>
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
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
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
        <translation>Pas de données fournies, annulation</translation>
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
        <translation>échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2706"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2796"/>
        <source>failed to parse view key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2716"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2814"/>
        <source>failed to verify view key secret key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2720"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2818"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2899"/>
        <source>view key does not match standard address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2725"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2744"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2822"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2956"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2979"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3009"/>
        <source>account creation failed</source>
        <translation>échec de la création du compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2740"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2780"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2924"/>
        <source>failed to parse spend key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2806"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2945"/>
        <source>failed to verify spend key secret key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2810"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2950"/>
        <source>spend key does not match standard address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse standard</translation>
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
        <translation>échec de l&apos;ouverture du compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3105"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3652"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3707"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3765"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5625"/>
        <source>wallet is null</source>
        <translation>portefeuille est nul</translation>
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
        <translation>choix de langue passé invalide. Veuillez réessayer.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3308"/>
        <source>View key: </source>
        <translation>Clé d&apos;audit : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3410"/>
        <source>Generated new wallet on hw device: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3547"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation>Vous pourriez vouloir supprimer le fichier &quot;%s&quot; et réessayer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3575"/>
        <source>failed to deinitialize wallet</source>
        <translation>échec de la désinitialisation du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3643"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4139"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7103"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation>cette commande requiert un démon de confiance. Activer avec --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3775"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation>la chaîne de blocs ne peut pas être sauvegardée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3854"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4153"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3858"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4157"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3868"/>
        <source>refresh error: </source>
        <translation>erreur du rafraîchissement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3918"/>
        <source>Balance: </source>
        <translation>Solde : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4014"/>
        <source>pubkey</source>
        <translation>clé publique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4014"/>
        <source>key image</source>
        <translation>image de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4025"/>
        <source>unlocked</source>
        <translation>déverrouillé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4015"/>
        <source>ringct</source>
        <translation>ringct</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4024"/>
        <source>T</source>
        <translation>V</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4024"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4025"/>
        <source>locked</source>
        <translation>vérrouillé</translation>
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
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4161"/>
        <source>failed to get spent status</source>
        <translation>échec de la récupération du statut de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4287"/>
        <source>the same transaction</source>
        <translation>la même transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4287"/>
        <source>blocks that are temporally very close</source>
        <translation>blocs très proches dans le temps</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4404"/>
        <source>Locked blocks too high, max 1000000 (˜4 yrs)</source>
        <translation>Nombre de blocs verrou trop élévé, 1000000 max (˜4 ans)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5740"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5856"/>
        <source>Good signature</source>
        <translation>Bonne signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5767"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5858"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5966"/>
        <source>Bad signature</source>
        <translation>Mauvaise signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6718"/>
        <source>usage: integrated_address [payment ID]</source>
        <translation>usage : integrated_address [ID paiement]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6754"/>
        <source>Standard address: </source>
        <translation>Adresse standard : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6759"/>
        <source>failed to parse payment ID or address</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement ou de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6770"/>
        <source>usage: address_book [(add (&lt;address&gt; [pid &lt;long or short payment id&gt;])|&lt;integrated address&gt; [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</source>
        <translation>usage : address_book [(add (&lt;adresse&gt; [pid &lt;ID de paiement long ou court&gt;])|&lt;adresse integrée&gt; [&lt;description avec des espaces possible&gt;])|(delete &lt;index&gt;)]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6800"/>
        <source>failed to parse payment ID</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6818"/>
        <source>failed to parse index</source>
        <translation>échec de l&apos;analyse de l&apos;index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6826"/>
        <source>Address book is empty.</source>
        <translation>Le carnet d&apos;adresses est vide.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6832"/>
        <source>Index: </source>
        <translation>Index : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6833"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6963"/>
        <source>Address: </source>
        <translation>Adresse : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6834"/>
        <source>Payment ID: </source>
        <translation>ID de paiement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6835"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6962"/>
        <source>Description: </source>
        <translation>Description : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6845"/>
        <source>usage: set_tx_note [txid] free text note</source>
        <translation>usage : set_tx_note [ID transaction] note de texte libre</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6873"/>
        <source>usage: get_tx_note [txid]</source>
        <translation>usage : get_tx_note [ID transaction]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6987"/>
        <source>usage: sign &lt;filename&gt;</source>
        <translation>usage : sign &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6992"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas signer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1058"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7006"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7029"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7204"/>
        <source>failed to read file </source>
        <translation>échec de la lecture du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5702"/>
        <source>usage: check_tx_proof &lt;txid&gt; &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>usage : check_tx_proof &lt;ID_transaction&gt; &lt;adresse&gt; &lt;fichier_signature&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5729"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5849"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5951"/>
        <source>failed to load signature file</source>
        <translation>échec du chargement du fichier signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5785"/>
        <source>usage: get_spend_proof &lt;txid&gt; [&lt;message&gt;]</source>
        <translation>usage : get_spend_proof &lt;ID_transaction&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5791"/>
        <source>wallet is watch-only and cannot generate the proof</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut générer de preuve</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5829"/>
        <source>usage: check_spend_proof &lt;txid&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>usage : check_spend_proof &lt;ID_transaction&gt; &lt;fichier_signature&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5875"/>
        <source>usage: get_reserve_proof (all|&lt;amount&gt;) [&lt;message&gt;]</source>
        <translation>usage : get_reserve_proof (all|&lt;montant&gt;) [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5881"/>
        <source>The reserve proof can be generated only by a full wallet</source>
        <translation>La preuve de réserve ne peut être généré que par un portefeuille complet</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5926"/>
        <source>usage: check_reserve_proof &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation>usage : check_reserve_proof &lt;adresse&gt; &lt;fichier_signature&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5944"/>
        <source>Address must not be a subaddress</source>
        <translation>L&apos;adresse ne doit pas être une sous-adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5962"/>
        <source>Good signature -- total: %s, spent: %s, unspent: %s</source>
        <translation>Bonne signature -- total : %s, dépensé : %s, non dépensé : %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6026"/>
        <source>usage: show_transfers [in|out|all|pending|failed] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation>usage : show_transfers [in|out|all|pending|failed] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;hauteur_min&gt; [&lt;hauteur_max&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6163"/>
        <source>[Double spend seen on the network: this transaction may or may not end up being mined] </source>
        <translation>[Double dépense détectée sur le réseau : cette transaction sera peut-être invalidée] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6199"/>
        <source>usage: unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_amount&gt; [&lt;max_amount&gt;]]</source>
        <translation>usage : unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;montant_min&gt; [&lt;montant_max&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6259"/>
        <source>There is no unspent output in the specified address</source>
        <translation>Il n&apos;y a pas de sortie non dépensée pour l&apos;adresse spécifiée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6371"/>
        <source> (no daemon)</source>
        <translation> (pas de démon)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6373"/>
        <source> (out of sync)</source>
        <translation> (désynchronisé)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6430"/>
        <source>(Untitled account)</source>
        <translation>(compte sans nom)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6443"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6461"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6486"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6509"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6662"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6685"/>
        <source>failed to parse index: </source>
        <translation>échec de l&apos;analyse de l&apos;index : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6448"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6667"/>
        <source>specify an index between 0 and </source>
        <translation>specifiez un index entre 0 et </translation>
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
        <translation>usage :
  account
  account new &lt;texte étiquette avec espaces autorisés&gt;
  account switch &lt;index&gt; 
  account label &lt;index&gt; &lt;texte étiquette avec espaces autorisés&gt;
  account tag &lt;mot_clé&gt; &lt;index_compte_1&gt; [&lt;index_compte_2&gt; ...]
  account untag &lt;index_compte_1&gt; [&lt;index_compte_2&gt; ...]
  account tag_description &lt;mot_clé&gt; &lt;description&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6573"/>
        <source>
Grand total:
  Balance: </source>
        <translation>
Somme finale :
  Solde : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6573"/>
        <source>, unlocked balance: </source>
        <translation>, solde débloqué : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6581"/>
        <source>Untagged accounts:</source>
        <translation>Comptes sans mot clé :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6587"/>
        <source>Tag %s is unregistered.</source>
        <translation>Le mot clé %s n&apos;est pas enregistré.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6590"/>
        <source>Accounts with tag: </source>
        <translation>Comptes avec mot clé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6591"/>
        <source>Tag&apos;s description: </source>
        <translation>Description du mot clé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6593"/>
        <source>Account</source>
        <translation>Compte</translation>
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
        <translation>Adresse primaire</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6633"/>
        <source>(used)</source>
        <translation>(utilisé)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6654"/>
        <source>(Untitled address)</source>
        <translation>(adresse sans nom)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6694"/>
        <source>&lt;index_min&gt; is already out of bound</source>
        <translation>&lt;index_min&gt; est déjà hors limite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6699"/>
        <source>&lt;index_max&gt; exceeds the bound</source>
        <translation>&lt;index_max&gt; excède la limite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6707"/>
        <source>usage: address [ new &lt;label text with white spaces allowed&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;label text with white spaces allowed&gt; ]</source>
        <translation>usage : address [ new &lt;texte étiquette avec espaces autorisés&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;texte étiquette avec espaces autorisés&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6725"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6737"/>
        <source>Integrated addresses can only be created for account 0</source>
        <translation>Les adresses intégrées ne peuvent être créées que pour le compte 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6749"/>
        <source>Integrated address: %s, payment ID: %s</source>
        <translation>Adresse intégrée : %s, ID de paiement : %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6754"/>
        <source>Subaddress: </source>
        <translation>Sous-adresse : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6914"/>
        <source>usage: get_description</source>
        <translation>usage : get_description</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6920"/>
        <source>no description found</source>
        <translation>pas de description trouvée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6922"/>
        <source>description found: </source>
        <translation>description trouvée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6961"/>
        <source>Filename: </source>
        <translation>Fichier : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6966"/>
        <source>Watch only</source>
        <translation>Audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6968"/>
        <source>%u/%u multisig%s</source>
        <translation>Multisig %u/%u%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6970"/>
        <source>Normal</source>
        <translation>Normal</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6971"/>
        <source>Type: </source>
        <translation>Type : </translation>
    </message>
    <message>
        <source>Testnet: </source>
        <translation type="vanished">Testnet : </translation>
    </message>
    <message>
        <source>Yes</source>
        <translation type="vanished">Oui</translation>
    </message>
    <message>
        <source>No</source>
        <translation type="vanished">Non</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6997"/>
        <source>This wallet is multisig and cannot sign</source>
        <translation>C&apos;est un portefeuille multisig et il ne peut pas signer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7018"/>
        <source>usage: verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation>usage : verify &lt;fichier&gt; &lt;adresse&gt; &lt;signature&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7043"/>
        <source>Bad signature from </source>
        <translation>Mauvaise signature de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7047"/>
        <source>Good signature from </source>
        <translation>Bonne signature de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7061"/>
        <source>usage: export_key_images &lt;filename&gt;</source>
        <translation>usage : export_key_images &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7066"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas exporter les images de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1008"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7079"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7171"/>
        <source>failed to save file </source>
        <translation>échec de l&apos;enregistrement du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7090"/>
        <source>Signed key images exported to </source>
        <translation>Images de clé signées exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7109"/>
        <source>usage: import_key_images &lt;filename&gt;</source>
        <translation>usage : import_key_images &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7145"/>
        <source>usage: export_outputs &lt;filename&gt;</source>
        <translation>usage : export_outputs &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7182"/>
        <source>Outputs exported to </source>
        <translation>Sorties exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7195"/>
        <source>usage: import_outputs &lt;filename&gt;</source>
        <translation>usage : import_outputs &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4445"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5892"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6218"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6226"/>
        <source>amount is wrong: </source>
        <translation>montant erroné : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4446"/>
        <source>expected number from 0 to </source>
        <translation>attend un nombre de 0 à </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4722"/>
        <source>Sweeping </source>
        <translation>Balayage de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5202"/>
        <source>Money successfully sent, transaction: </source>
        <translation>Fonds envoyés avec succès, transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5359"/>
        <source>Change goes to more than one address</source>
        <translation>La monnaie rendue va à plus d&apos;une adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5400"/>
        <source>%s change to %s</source>
        <translation>%s de monnaie rendue à %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5403"/>
        <source>no change</source>
        <translation>sans monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1156"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1169"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5474"/>
        <source>Transaction successfully signed to file </source>
        <translation>Transaction signée avec succès dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5534"/>
        <source>usage: get_tx_key &lt;txid&gt;</source>
        <translation>usage : get_tx_key &lt;ID transaction&gt;</translation>
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
        <translation>échec de l&apos;analyse de l&apos;ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5556"/>
        <source>Tx key: </source>
        <translation>Clé de transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5561"/>
        <source>no tx keys found for this txid</source>
        <translation>aucune clé de transaction trouvée pour cet ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5575"/>
        <source>usage: get_tx_proof &lt;txid&gt; &lt;address&gt; [&lt;message&gt;]</source>
        <translation>usage : get_tx_proof &lt;ID_transaction&gt; &lt;adresse&gt; [&lt;message&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5600"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5815"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5912"/>
        <source>signature file saved to: </source>
        <translation>fichier signature sauvegardé dans : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5602"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5817"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5914"/>
        <source>failed to save signature file</source>
        <translation>échec de la sauvegarde du fichier signature</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5616"/>
        <source>usage: check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</source>
        <translation>usage : check_tx_key &lt;ID transaction&gt; &lt;clé transaction&gt; &lt;adresse&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5639"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5648"/>
        <source>failed to parse tx key</source>
        <translation>échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5606"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5694"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5772"/>
        <source>error: </source>
        <translation>erreur : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5670"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5743"/>
        <source>received</source>
        <translation>a reçu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5670"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5743"/>
        <source>in txid</source>
        <translation>dans la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5689"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5762"/>
        <source>received nothing in txid</source>
        <translation>n&apos;a rien reçu dans la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5673"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5746"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation>ATTENTION : cette transaction n&apos;est pas encore inclue dans la chaîne de blocs !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5679"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5752"/>
        <source>This transaction has %u confirmations</source>
        <translation>Cette transaction a %u confirmations</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5683"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5756"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation>ATTENTION : échec de la détermination du nombre de confirmations !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6074"/>
        <source>bad min_height parameter:</source>
        <translation>mauvais paramètre hauteur_minimum :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6086"/>
        <source>bad max_height parameter:</source>
        <translation>mauvais paramètre hauteur_maximum :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6146"/>
        <source>in</source>
        <translation>reçu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6146"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6187"/>
        <source>out</source>
        <translation>payé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6187"/>
        <source>failed</source>
        <translation>échoué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6187"/>
        <source>pending</source>
        <translation>en attente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6233"/>
        <source>&lt;min_amount&gt; should be smaller than &lt;max_amount&gt;</source>
        <translation>&lt;montant_minimum&gt; doit être inférieur à &lt;montant_maximum&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6265"/>
        <source>
Amount: </source>
        <translation>
Montant : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6265"/>
        <source>, number of keys: </source>
        <translation>, nombre de clés : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6270"/>
        <source> </source>
        <translation> </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6275"/>
        <source>
Min block height: </source>
        <translation>
Hauteur de bloc minimum : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6276"/>
        <source>
Max block height: </source>
        <translation>
Hauteur de bloc maximum : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6277"/>
        <source>
Min amount found: </source>
        <translation>
Montant minimum trouvé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6278"/>
        <source>
Max amount found: </source>
        <translation>
Montant maximum trouvé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6279"/>
        <source>
Total count: </source>
        <translation>
Compte total : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6319"/>
        <source>
Bin size: </source>
        <translation>
Taille de classe : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6320"/>
        <source>
Outputs per *: </source>
        <translation>
Sorties par * : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6322"/>
        <source>count
  ^
</source>
        <translation>compte
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
        <translation>+--&gt; hauteur de bloc
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
        <translation>  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6369"/>
        <source>wallet</source>
        <translation>portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="748"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6729"/>
        <source>Random payment ID: </source>
        <translation>ID de paiement aléatoire : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6730"/>
        <source>Matching integrated address: </source>
        <translation>Adresse intégrée correspondante : </translation>
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
        <source>How many participants wil share parts of the multisig wallet</source>
        <translation type="vanished">Combien de participants partageront des parts du portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="72"/>
        <source>How many participants will share parts of the multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="73"/>
        <source>How many signers are required to sign a valid transaction</source>
        <translation>Combien de signataires sont requis pour signer une transaction valide</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="74"/>
        <source>Create testnet multisig wallets</source>
        <translation>Créer un portefeuille multisig testnet</translation>
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
        <translation>Génération de %u portefeuilles multisig %u/%u</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="140"/>
        <source>Error verifying multisig extra info</source>
        <translation>Erreur de vérification des infos multisig supplémentaires</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="148"/>
        <source>Error finalizing multisig</source>
        <translation>Erreur de finalisation multisig</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="155"/>
        <source>Generated multisig wallets for address </source>
        <translation>Portefeuilles multisig générés pour l&apos;adresse </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="159"/>
        <source>Error creating multisig wallets: </source>
        <translation>Erreur de création des portefeuilles multisig : </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="180"/>
        <source>This program generates a set of multisig wallets - use this simpler scheme only if all the participants trust each other</source>
        <translation>Ce programme génère un ensemble de portefeuilles multisig - n&apos;utilisez cette méthode plus simple que si tous les participants se font confiance</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="197"/>
        <source>Error: Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="204"/>
        <source>Error: expected N/M, but got: </source>
        <translation>Erreur : N/M attendu, mais lu : </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="212"/>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="221"/>
        <source>Error: either --scheme or both of --threshold and --participants may be given</source>
        <translation>Erreur : soit --scheme soit --threshold et --participants doivent être indiqués</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="228"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got N==%u and M==%d</source>
        <translation>Erreur : N &gt; 1 et N &lt;= M attendu, mais lu N==%u et M==%d</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="237"/>
        <source>Error: --filename-base is required</source>
        <translation>Erreur : --filename-base est requis</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="243"/>
        <source>Error: unsupported scheme: only N/N and N-1/N are supported</source>
        <translation>Erreur : schéma non supporté : seuls N/N et N-1/N sont supportés</translation>
    </message>
</context>
<context>
    <name>sw</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="120"/>
        <source>Generate new wallet and save it to &lt;arg&gt;</source>
        <translation>Générer un nouveau portefeuille et le sauvegarder dans &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="121"/>
        <source>Generate new wallet from device and save it to &lt;arg&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="122"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>Générer un portefeuille d&apos;audit à partir d&apos;une clé d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="123"/>
        <source>Generate deterministic wallet from spend key</source>
        <translation>Générer un portefeuille déterministe à partir d&apos;une clé de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="124"/>
        <source>Generate wallet from private keys</source>
        <translation>Générer un portefeuille à partir de clés privées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="125"/>
        <source>Generate a master wallet from multisig wallet keys</source>
        <translation>Générer un portefeuille principal à partir de clés de portefeuille multisig</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="127"/>
        <source>Language for mnemonic</source>
        <translation>Langue de la phrase mnémonique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="128"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>Spécifier la phrase mnémonique Electrum pour la récupération/création d&apos;un portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="129"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Récupérer un portefeuille en utilisant une phrase mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="130"/>
        <source>Recover multisig wallet using Electrum-style mnemonic seed</source>
        <translation>Récupérer un portefeuille multisig en utilisant une phrase mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="131"/>
        <source>Generate non-deterministic view and spend keys</source>
        <translation>Générer des clés d&apos;audit et de dépense non déterministes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="132"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation>Activer les commandes qui dépendent d&apos;un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="133"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>Autoriser la communication avec un démon utilisant une version de RPC différente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="134"/>
        <source>Restore from specific blockchain height</source>
        <translation>Restaurer à partir d&apos;une hauteur de bloc spécifique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="135"/>
        <source>The newly created transaction will not be relayed to the monero network</source>
        <translation>La transaction nouvellement créée ne sera pas transmise au réseau monero</translation>
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
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="214"/>
        <source>possibly lost connection to daemon</source>
        <translation>connexion avec le démon peut-être perdue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="231"/>
        <source>Error: </source>
        <translation>Erreur : </translation>
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
        <translation type="vanished">Ceci est le portefeuille monero en ligne de commande. Il a besoin de se
connecter à un démon monero pour fonctionner correctement.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7514"/>
        <source>Failed to initialize wallet</source>
        <translation>Échec de l&apos;initialisation du portefeuille</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="135"/>
        <source>Use daemon instance at &lt;host&gt;:&lt;port&gt;</source>
        <translation>Utiliser l&apos;instance de démon située à &lt;hôte&gt;:&lt;port&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="136"/>
        <source>Use daemon instance at host &lt;arg&gt; instead of localhost</source>
        <translation>Utiliser l&apos;instance de démon située à l&apos;hôte &lt;arg&gt; au lieu de localhost</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="138"/>
        <source>Wallet password file</source>
        <translation>Fichier mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="139"/>
        <source>Use daemon instance at port &lt;arg&gt; instead of 18081</source>
        <translation>Utiliser l&apos;instance de démon située au port &lt;arg&gt; au lieu de 18081</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="141"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>Pour testnet, le démon doit aussi être lancé avec l&apos;option --testnet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="142"/>
        <source>For stagenet. Daemon must also be launched with --stagenet flag</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="143"/>
        <source>Restricts to view-only commands</source>
        <translation>Restreindre aux commandes en lecture-seule</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="145"/>
        <source>Set shared ring database path</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="202"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>impossible de spécifier l&apos;hôte ou le port du démon plus d&apos;une fois</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="240"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>impossible de spécifier plus d&apos;une option parmis --password et --password-file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="253"/>
        <source>the password file specified could not be read</source>
        <translation>le fichier mot de passe spécifié n&apos;a pas pu être lu</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="278"/>
        <source>Failed to load file </source>
        <translation>Échec du chargement du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="360"/>
        <source>At least one of either an Electrum-style word list, private view key, or private spend key must be specified</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="137"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation>Mot de passe du portefeuille (échapper/citer si nécessaire)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="140"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>Spécifier le nom_utilisateur:[mot_de_passe] pour le client RPC du démon</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="260"/>
        <source>no password specified; use --prompt-for-password to prompt for a password</source>
        <translation>pas de mot de passe spécifié; utilisez --prompt-for-password pour demander un mot de passe</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="284"/>
        <source>Failed to parse JSON</source>
        <translation>Échec de l&apos;analyse JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="291"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation>Version %u trop récente, on comprend au mieux %u</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="307"/>
        <source>failed to parse view key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="312"/>
        <location filename="../src/wallet/wallet2.cpp" line="380"/>
        <location filename="../src/wallet/wallet2.cpp" line="421"/>
        <source>failed to verify view key secret key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="323"/>
        <source>failed to parse spend key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="328"/>
        <location filename="../src/wallet/wallet2.cpp" line="390"/>
        <location filename="../src/wallet/wallet2.cpp" line="446"/>
        <source>failed to verify spend key secret key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="340"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
    </message>
    <message>
        <source>At least one of Electrum-style word list and private view key and private spend key must be specified</source>
        <translation type="vanished">Il faut spécifier au moins une des options parmis la liste de mots de style Electrum, la clé privée d&apos;audit et la clé privée de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="364"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Liste de mots de style Electrum et clé privée spécifiées en même temps</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="374"/>
        <source>invalid address</source>
        <translation>adresse invalide</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="383"/>
        <source>view key does not match standard address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="393"/>
        <source>spend key does not match standard address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="401"/>
        <source>Cannot generate deprecated wallets from JSON</source>
        <translation>Impossible de générer un portefeuille obsolète à partir de JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="433"/>
        <source>failed to parse address: </source>
        <translation>échec de l&apos;analyse de l&apos;adresse : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="439"/>
        <source>Address must be specified in order to create watch-only wallet</source>
        <translation>L&apos;adresse doit être spécifiée afin de créer un portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="454"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille : </translation>
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
        <translation>Compte primaire</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="8703"/>
        <source>No funds received in this tx.</source>
        <translation>Aucun fonds n&apos;a été reçu dans cette transaction.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="9398"/>
        <source>failed to read file </source>
        <translation>échec de la lecture du fichier </translation>
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
        <translation>Le démon est local, supposons qu&apos;il est de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="176"/>
        <source>Failed to create directory </source>
        <translation>Échec de la création du répertoire </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="178"/>
        <source>Failed to create directory %s: %s</source>
        <translation>Échec de la création du répertoire %s : %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="189"/>
        <source>Cannot specify --</source>
        <translation>Impossible de spécifier --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="189"/>
        <source> and --</source>
        <translation> et --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="208"/>
        <source>Failed to create file </source>
        <translation>Échec de la création du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="208"/>
        <source>. Check permissions or remove file</source>
        <translation>. Vérifiez les permissions ou supprimez le fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="218"/>
        <source>Error writing to file </source>
        <translation>Erreur d&apos;écriture dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="221"/>
        <source>RPC username/password is stored in file </source>
        <translation>nom_utilisateur/mot_de_passe RPC sauvegardé dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="442"/>
        <source>Tag %s is unregistered.</source>
        <translation>Le mot clé %s n&apos;est pas enregistré.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2466"/>
        <source>Transaction not possible. Available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>Transaction impossible. Solde disponible : %s, montant de la transaction %s = %s + %s (frais)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2901"/>
        <source>This is the RPC monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation>Ceci est le portefeuille monero par RPC. Il a besoin de se
connecter à un démon monero pour fonctionner correctement.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2920"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2932"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>Impossible de spécifier plus d&apos;une option parmis --wallet-file et --generate-from-json</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2944"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>--wallet-file, --generate-from-json ou --wallet-dir doit être spécifié</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2948"/>
        <source>Loading wallet...</source>
        <translation>Chargement du portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2981"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3014"/>
        <source>Saving wallet...</source>
        <translation>Sauvegarde du portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2983"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3016"/>
        <source>Successfully saved</source>
        <translation>Sauvegardé avec succès</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2986"/>
        <source>Successfully loaded</source>
        <translation>Chargé avec succès</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2990"/>
        <source>Wallet initialization failed: </source>
        <translation>Échec de l&apos;initialisation du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2997"/>
        <source>Failed to initialize wallet RPC server</source>
        <translation>Échec de l&apos;initialisation du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3001"/>
        <source>Starting wallet RPC server</source>
        <translation>Démarrage du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3008"/>
        <source>Failed to run wallet: </source>
        <translation>Échec du lancement du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3011"/>
        <source>Stopped wallet RPC server</source>
        <translation>Arrêt du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3020"/>
        <source>Failed to save wallet: </source>
        <translation>Échec de la sauvegarde du portefeuille : </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7469"/>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="168"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2887"/>
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
        <location filename="../src/wallet/wallet_args.cpp" line="104"/>
        <source>Max number of threads to use for a parallel job</source>
        <translation>Nombre maximum de threads à utiliser pour les travaux en parallèle</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="105"/>
        <source>Specify log file</source>
        <translation>Spécifier le fichier journal</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="106"/>
        <source>Config file</source>
        <translation>Fichier de configuration</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="115"/>
        <source>General options</source>
        <translation>Options générales</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="138"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation>Ceci est le portefeuille monero en ligne de commande. Il a besoin de se
connecter à un démon monero pour fonctionner correctement.</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="161"/>
        <source>Can&apos;t find config file </source>
        <translation>Impossible de trouver le fichier de configuration </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="195"/>
        <source>Logging to: </source>
        <translation>Journalisation dans : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="197"/>
        <source>Logging to %s</source>
        <translation>Journalisation dans %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="140"/>
        <source>Usage:</source>
        <translation>Usage :</translation>
    </message>
</context>
</TS>
