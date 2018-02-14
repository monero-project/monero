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
        <source>Integrated address and long payment id can&apos;t be used at the same time</source>
        <translation>Une adresse intégrée et un identifiant de paiement long ne peuvent pas être utilisés en même temps</translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min mixin %lu. %s</source>
        <translation type="vanished">%lu transactions chargées, pour %s, %s de frais, %s, %s, avec mixin minimum de %lu, %s</translation>
    </message>
</context>
<context>
    <name>Monero::WalletImpl</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1111"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1121"/>
        <source>Failed to add short payment id: </source>
        <translation>Échec de l&apos;ajout de l&apos;ID de paiement court : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1154"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1257"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1157"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1260"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1160"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1263"/>
        <source>RPC error: </source>
        <translation>Erreur RPC : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1197"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1300"/>
        <source>not enough outputs for specified ring size</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1199"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1302"/>
        <source>found outputs to use</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1266"/>
        <source>failed to get random outputs to mix</source>
        <translation>échec de la récupération de sorties aléatoires à mélanger</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1170"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1273"/>
        <source>not enough money to transfer, available only %s, sent amount %s</source>
        <translation>pas assez de fonds pour le transfert, montant disponible %s, montant envoyé %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="474"/>
        <source>failed to parse address</source>
        <translation>échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="486"/>
        <source>failed to parse secret spend key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="496"/>
        <source>No view key supplied, cancelled</source>
        <translation>Pas de clé d&apos;audit fournie, annulation</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="503"/>
        <source>failed to parse secret view key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="513"/>
        <source>failed to verify secret spend key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="518"/>
        <source>spend key does not match address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="524"/>
        <source>failed to verify secret view key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="529"/>
        <source>view key does not match address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="548"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="773"/>
        <source>Failed to send import wallet request</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="919"/>
        <source>Failed to load unsigned transactions</source>
        <translation>Échec du chargement des transaction non signées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="940"/>
        <source>Failed to load transaction from file</source>
        <translation>Échec du chargement de la transaction du fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="958"/>
        <source>Wallet is view only</source>
        <translation>Portefeuille d&apos;audit uniquement</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="967"/>
        <source>failed to save file </source>
        <translation>échec de l&apos;enregistrement du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="986"/>
        <source>Key images can only be imported with a trusted daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="999"/>
        <source>Failed to import key images: </source>
        <translation>Échec de l&apos;importation des images de clé : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1032"/>
        <source>Failed to get subaddress label: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1046"/>
        <source>Failed to set subaddress label: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1163"/>
        <source>failed to get random outputs to mix: %s</source>
        <translation>échec de la récupération de sorties aléatoires à mélanger : %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1179"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1282"/>
        <source>not enough money to transfer, overall balance only %s, sent amount %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1188"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1291"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>pas assez de fonds pour le transfert, montant disponible %s, montant envoyé %s = %s + %s (frais)</translation>
    </message>
    <message>
        <source>not enough outputs for specified mixin_count</source>
        <translation type="vanished">pas assez de sorties pour le mixin spécifié</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1199"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1302"/>
        <source>output amount</source>
        <translation>montant de la sortie</translation>
    </message>
    <message>
        <source>found outputs to mix</source>
        <translation type="vanished">sorties à mélanger trouvées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1204"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1307"/>
        <source>transaction was not constructed</source>
        <translation>la transaction n&apos;a pas été construite</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1208"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1311"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transaction %s a été rejetée par le démon avec le statut : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1215"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1318"/>
        <source>one of destinations is zero</source>
        <translation>une des destinations est zéro</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1218"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1321"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>échec de la recherche d&apos;une façon adéquate de scinder les transactions</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1221"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1324"/>
        <source>unknown transfer error: </source>
        <translation>erreur de transfert inconnue : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1224"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1327"/>
        <source>internal error: </source>
        <translation>erreur interne : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1227"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1330"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1230"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1333"/>
        <source>unknown error</source>
        <translation>erreur inconnue</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1411"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1440"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1493"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1524"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1555"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1578"/>
        <source>Failed to parse txid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1429"/>
        <source>no tx keys found for this txid</source>
        <translation type="unfinished">aucune clé de transaction trouvée pour cet ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1449"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1459"/>
        <source>Failed to parse tx key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1469"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1501"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1532"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1620"/>
        <source>Failed to parse address</source>
        <translation type="unfinished">Échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1626"/>
        <source>Address must not be a subaddress</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1848"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>Réexaminer les dépenses ne peut se faire qu&apos;avec un démon de confiance</translation>
    </message>
</context>
<context>
    <name>Monero::WalletManagerImpl</name>
    <message>
        <source>failed to parse txid</source>
        <translation type="vanished">échec de l&apos;analyse de l&apos;ID de transaction</translation>
    </message>
    <message>
        <source>failed to parse tx key</source>
        <translation type="vanished">échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <source>failed to parse address</source>
        <translation type="vanished">échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <source>failed to get transaction from daemon</source>
        <translation type="vanished">échec de la récupération de la transaction du démon</translation>
    </message>
    <message>
        <source>failed to parse transaction from daemon</source>
        <translation type="vanished">échec de l&apos;analyse de la transaction du démon</translation>
    </message>
    <message>
        <source>failed to validate transaction from daemon</source>
        <translation type="vanished">échec de la validation de la transaction du démon</translation>
    </message>
    <message>
        <source>failed to get the right transaction from daemon</source>
        <translation type="vanished">échec de la récupération de la bonne transaction du démon</translation>
    </message>
    <message>
        <source>failed to generate key derivation from supplied parameters</source>
        <translation type="vanished">échec de la génération de la dérivation de clé à partir des paramètres fournis</translation>
    </message>
    <message>
        <source>error: </source>
        <translation type="vanished">erreur : </translation>
    </message>
    <message>
        <source>received</source>
        <translation type="vanished">a reçu</translation>
    </message>
    <message>
        <source>in txid</source>
        <translation type="vanished">dans la transaction</translation>
    </message>
    <message>
        <source>received nothing in txid</source>
        <translation type="vanished">n&apos;a rien reçu dans la transaction</translation>
    </message>
</context>
<context>
    <name>Wallet</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="246"/>
        <source>Failed to parse address</source>
        <translation>Échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="253"/>
        <source>Failed to parse key</source>
        <translation>Échec de l&apos;analyse de la clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="261"/>
        <source>failed to verify key</source>
        <translation>Échec de la vérification de la clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="271"/>
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
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>cryptonote::rpc_args</name>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="40"/>
        <source>Specify ip to bind rpc server</source>
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
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>cryptonote::simple_wallet</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="478"/>
        <source>Commands: </source>
        <translation>Commandes : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3003"/>
        <source>failed to read wallet password</source>
        <translation>échec de la lecture du mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2694"/>
        <source>invalid password</source>
        <translation>mot de passe invalide</translation>
    </message>
    <message>
        <source>start_mining [&lt;number_of_threads&gt;] - Start mining in daemon</source>
        <translation type="vanished">start_mining [&lt;nombre_de_threads&gt;] - Démarrer l&apos;extraction minière dans le démon</translation>
    </message>
    <message>
        <source>Stop mining in daemon</source>
        <translation type="vanished">Stopper l&apos;extraction minière dans le démon</translation>
    </message>
    <message>
        <source>Save current blockchain data</source>
        <translation type="vanished">Sauvegarder les données actuelles de la chaîne de blocs</translation>
    </message>
    <message>
        <source>Show current wallet balance</source>
        <translation type="vanished">Afficher le solde actuel du portefeuille</translation>
    </message>
    <message>
        <source>Show blockchain height</source>
        <translation type="vanished">Afficher la hauteur de la chaîne de blocs</translation>
    </message>
    <message>
        <source>Show current wallet public address</source>
        <translation type="vanished">Afficher l&apos;adresse publique du portefeuille actuel</translation>
    </message>
    <message>
        <source>Show this help</source>
        <translation type="vanished">Afficher cette aide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1900"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation>set seed : requiert un argument. options disponibles : language</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1928"/>
        <source>set: unrecognized argument(s)</source>
        <translation>set : argument(s) non reconnu(s)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2864"/>
        <source>wallet file path not valid: </source>
        <translation>chemin du fichier portefeuille non valide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1982"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation>Tentative de génération ou de restauration d&apos;un portefeuille, mais le fichier spécifié existe déjà. Sortie pour ne pas risquer de l&apos;écraser.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="661"/>
        <source>usage: payment_id</source>
        <translation>usage : payment_id</translation>
    </message>
    <message>
        <source>sweep_below &lt;amount_threshold&gt; [mixin] address [payment_id] - Send all unlocked outputs below the threshold to an address</source>
        <translation type="vanished">sweep_below &lt;montant_seuil&gt; [mixin] adresse [ID_paiement] - Envoyer toutes les sorties débloquées sous le seuil vers une adresse</translation>
    </message>
    <message>
        <source>Available options: seed language - set wallet seed language; always-confirm-transfers &lt;1|0&gt; - whether to confirm unsplit txes; print-ring-members &lt;1|0&gt; - whether to print detailed information about ring members during confirmation; store-tx-info &lt;1|0&gt; - whether to store outgoing tx info (destination address, payment ID, tx secret key) for future reference; default-mixin &lt;n&gt; - set default mixin (default is 4); auto-refresh &lt;1|0&gt; - whether to automatically sync new blocks from the daemon; refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt; - set wallet refresh behaviour; priority [0|1|2|3|4] - default/unimportant/normal/elevated/priority fee; confirm-missing-payment-id &lt;1|0&gt;; ask-password &lt;1|0&gt;; unit &lt;monero|millinero|micronero|nanonero|piconero&gt; - set default monero (sub-)unit; min-outputs-count [n] - try to keep at least that many outputs of value at least min-outputs-value; min-outputs-value [n] - try to keep at least min-outputs-count outputs of at least that value - merge-destinations &lt;1|0&gt; - whether to merge multiple payments to the same destination address</source>
        <translation type="obsolete">Options disponibles : seed language - définir la langue de la graine du portefeuille; always-confirm-transfers &lt;1|0&gt; - confirmer ou non les transactions non scindées; print-ring-members &lt;1|0&gt; - afficher ou non des informations détaillées sur les membres du cercle pendant la confirmation; store-tx-info &lt;1|0&gt; - sauvegarder ou non les informations des transactions sortantes (adresse de destination, ID de paiement, clé secrète de transaction) pour référence future; default-mixin &lt;n&gt; - définir le mixin par défaut (4 par défaut); auto-refresh &lt;1|0&gt; - synchroniser automatiquement ou non les nouveau blocs du démon; refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt; - définir le comportement du rafraîchissement du portefeuille; priority [0|1|2|3|4] - frais de transaction par défault/peu important/normal/élevé/prioritaire; confirm-missing-payment-id &lt;1|0&gt;; ask-password &lt;1|0&gt;; unit &lt;monero|millinero|micronero|nanonero|piconero&gt; - définir la (sous-)unité monero par défaut; min-outputs-count [n] - essayer de garder au moins ce nombre de sortie d&apos;une valeur d&apos;au moins min-outputs-value; min-outputs-value [n] - essayer de garder au moins min-outputs-count sorties d&apos;une valeur d&apos;au moins ce montant; merge-destinations &lt;1|0&gt; - fusionner ou non de multiples paiements à une même adresse de destination</translation>
    </message>
    <message>
        <source>Generate a new random full size payment id - these will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids</source>
        <translation type="vanished">Générer un nouvel ID de paiement long aléatoire - ceux-ci seront non cryptés dans la chaîne de blocs, voir integrated_address pour les IDs de paiement courts cryptés</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1886"/>
        <source>needs an argument</source>
        <translation>requiert un argument</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1909"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1910"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1911"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1913"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1916"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1917"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1921"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1922"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1924"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1926"/>
        <source>0 or 1</source>
        <translation>0 ou 1</translation>
    </message>
    <message>
        <source>integer &gt;= 2</source>
        <translation type="vanished">entier &gt;= 2</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1915"/>
        <source>0, 1, 2, 3, or 4</source>
        <translation>0, 1, 2, 3 ou 4</translation>
    </message>
    <message>
        <source>monero, millinero, micronero, nanop, piconero</source>
        <translation type="obsolete">monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1919"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1923"/>
        <source>unsigned integer</source>
        <translation>entier non signé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2036"/>
        <source>NOTE: the following 25 words can be used to recover access to your wallet. Write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation>VEUILLEZ NOTER : les 25 mots suivants peuvent être utilisés pour restaurer votre portefeuille. Veuillez les écrire sur papier et les garder dans un endroit sûr. Ne les gardez pas dans un courriel ou dans un service de stockage de fichiers hors de votre contrôle.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2087"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-deterministic-wallet utilise --generate-new-wallet, pas --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2116"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation>spécifiez un paramètre de récupération avec --electrum-seed=&quot;liste de mots ici&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2466"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation>spécifiez un chemin de portefeuille avec --generate-new-wallet (pas --wallet-file)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2630"/>
        <source>wallet failed to connect to daemon: </source>
        <translation>échec de la connexion du portefeuille au démon : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2638"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation>Le démon utilise une version majeure de RPC (%u) différente de celle du portefeuille (%u) : %s. Mettez l&apos;un des deux à jour, ou utilisez --allow-mismatched-daemon-version.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2657"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation>Liste des langues disponibles pour la graine de votre portefeuille :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2666"/>
        <source>Enter the number corresponding to the language of your choice: </source>
        <translation>Entrez le nombre correspondant à la langue de votre choix : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2732"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Veuillez dorénavant utiliser la nouvelle graine que nous fournissons.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2746"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2804"/>
        <source>Generated new wallet: </source>
        <translation>Nouveau portefeuille généré : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2752"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2809"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2853"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2882"/>
        <source>Opened watch-only wallet</source>
        <translation>Ouverture du portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2886"/>
        <source>Opened wallet</source>
        <translation>Ouverture du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2896"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Veuillez procéder à la mise à jour de votre portefeuille.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2911"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Le format de votre fichier portefeuille est en cours de mise à jour.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2919"/>
        <source>failed to load wallet: </source>
        <translation>échec du chargement du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2936"/>
        <source>Use the &quot;help&quot; command to see the list of available commands.
</source>
        <translation>Utilisez la commande &quot;help&quot; pour voir la liste des commandes disponibles.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2981"/>
        <source>Wallet data saved</source>
        <translation>Données du portefeuille sauvegardées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3067"/>
        <source>Mining started in daemon</source>
        <translation>L&apos;extraction minière dans le démon a démarré</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3069"/>
        <source>mining has NOT been started: </source>
        <translation>l&apos;extraction minière n&apos;a PAS démarré : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3088"/>
        <source>Mining stopped in daemon</source>
        <translation>L&apos;extraction minière dans le démon a été stoppée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3090"/>
        <source>mining has NOT been stopped: </source>
        <translation>l&apos;extraction minière n&apos;a PAS été stoppée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3145"/>
        <source>Blockchain saved</source>
        <translation>Chaîne de blocs sauvegardée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3160"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3178"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3191"/>
        <source>Height </source>
        <translation>Hauteur </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3192"/>
        <source>transaction </source>
        <translation>transaction </translation>
    </message>
    <message>
        <source>received </source>
        <translation type="vanished">reçu </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3180"/>
        <source>spent </source>
        <translation>dépensé </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3193"/>
        <source>unsupported transaction format</source>
        <translation>format de transaction non supporté</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3214"/>
        <source>Starting refresh...</source>
        <translation>Démarrage du rafraîchissement...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3227"/>
        <source>Refresh done, blocks received: </source>
        <translation>Rafraîchissement effectué, blocs reçus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3753"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4225"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3768"/>
        <source>bad locked_blocks parameter:</source>
        <translation>mauvais paramètre locked_blocks :</translation>
    </message>
    <message>
        <source>Locked blocks too high, max 1000000 (Ë4 yrs)</source>
        <translation type="vanished" variants="yes">
            <lengthvariant>Blocs vérrouillés trop élevé, 1000000 maximum (Ë</lengthvariant>
            <lengthvariant>4 ans)</lengthvariant>
        </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3796"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4243"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4457"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation>une unique transaction ne peut pas utiliser plus d&apos;un ID de paiement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3805"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4252"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4425"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4465"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>échec de la définition de l&apos;ID de paiement, bien qu&apos;il ait été décodé correctement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3830"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3911"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3982"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4091"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4266"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4324"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4479"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4522"/>
        <source>transaction cancelled.</source>
        <translation>transaction annulée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3890"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3900"/>
        <source>Is this okay anyway?  (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3895"/>
        <source>There is currently a %u block backlog at that fee level. Is this okay?  (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3900"/>
        <source>Failed to check for backlog: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3941"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4297"/>
        <source>
Transaction </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3946"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4302"/>
        <source>Spending from address index %d
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3948"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4304"/>
        <source>WARNING: Outputs of multiple addresses are being used together, which might potentially compromise your privacy.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3950"/>
        <source>Sending %s.  </source>
        <translation>Envoi de %s. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3953"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation>Votre transaction doit être scindée en %llu transactions. Il en résulte que des frais de transaction doivent être appliqués à chaque transaction, pour un total de %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3959"/>
        <source>The transaction fee is %s</source>
        <translation>Les frais de transaction sont de %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3962"/>
        <source>, of which %s is dust from change</source>
        <translation>, dont %s est de la poussière de monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3963"/>
        <source>.</source>
        <translation>.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3963"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation>Un total de %s de poussière de monnaie rendue sera envoyé à une adresse de poussière</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3968"/>
        <source>.
This transaction will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation>.
Cette transaction sera déverrouillée au bloc %llu, dans approximativement %s jours (en supposant 2 minutes par bloc)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3994"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4006"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4102"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4114"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4335"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4347"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4532"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4544"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Échec de l&apos;écriture de(s) transaction(s) dans le fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3998"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4010"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4106"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4118"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4339"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4351"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4536"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4548"/>
        <source>Unsigned transaction(s) successfully written to file: </source>
        <translation>Transaction(s) non signée(s) écrite(s) dans le fichier avec succès : </translation>
    </message>
    <message>
        <source>Not enough money in unlocked balance</source>
        <translation type="vanished">Pas assez de fonds dans le solde débloqué</translation>
    </message>
    <message>
        <source>Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees</source>
        <translation type="vanished">Échec de la recherche d&apos;une façon de créer les transactions. Ceci est généralement dû à de la poussière si petite qu&apos;elle de peut pas payer ses propre frais, à une tentative d&apos;envoi d&apos;un montant supérieur au solde débloqué, ou parce qu&apos;il n&apos;en reste pas assez pour les frais de transaction</translation>
    </message>
    <message>
        <source>Reason: </source>
        <translation type="vanished">Raison : </translation>
    </message>
    <message>
        <source>failed to find a suitable way to split transactions</source>
        <translation type="vanished">échec de la recherche d&apos;une façon adéquate de scinder les transactions</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4061"/>
        <source>No unmixable outputs found</source>
        <translation>Aucune sortie non mélangeable trouvée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4144"/>
        <source>No address given</source>
        <translation>Aucune adresse fournie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4419"/>
        <source>failed to parse Payment ID</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4435"/>
        <source>usage: sweep_single [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;key_image&gt; &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4442"/>
        <source>failed to parse key image</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4494"/>
        <source>No outputs found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4499"/>
        <source>Multiple transactions are created, which is not supposed to happen</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4504"/>
        <source>The transaction uses multiple or no inputs, which is not supposed to happen</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4581"/>
        <source>missing amount threshold</source>
        <translation>montant seuil manquant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4586"/>
        <source>invalid amount threshold</source>
        <translation>montant seuil invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4596"/>
        <source>donations are not enabled on the testnet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4603"/>
        <source>usage: donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4697"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>La monnaie réclamée ne va pas à une adresse payée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4702"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>La monnaie réclamée est supérieure au paiement à l&apos;adresse de monnaie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4733"/>
        <source>sending %s to %s</source>
        <translation>envoi de %s à %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4743"/>
        <source> dummy output(s)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4746"/>
        <source>with no destinations</source>
        <translation>sans destination</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4758"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu, %s. %sIs this okay? (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4782"/>
        <source>This is a multisig wallet, it can only sign with sign_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4792"/>
        <source>usage: sign_transfer [export]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4804"/>
        <source>Failed to sign transaction</source>
        <translation>Échec de signature de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4810"/>
        <source>Failed to sign transaction: </source>
        <translation>Échec de signature de transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4831"/>
        <source>Transaction raw hex data exported to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4847"/>
        <source>Failed to load transaction from file</source>
        <translation>Échec du chargement de la transaction du fichier</translation>
    </message>
    <message>
        <source>daemon is busy. Please try later</source>
        <translation type="vanished">le démon est occupé. Veuillez réessayer plus tard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3243"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3546"/>
        <source>RPC error: </source>
        <translation>Erreur RPC : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="521"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il n&apos;a pas de clé de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="635"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="779"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="843"/>
        <source>Your original password was incorrect.</source>
        <translation>Votre mot de passe original est incorrect.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="649"/>
        <source>Error with wallet rewrite: </source>
        <translation>Erreur avec la réécriture du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1284"/>
        <source>priority must be 0, 1, 2, 3, or 4 </source>
        <translation>la priorité doit être 0, 1, 2, 3 ou 4 </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1296"/>
        <source>priority must be 0, 1, 2, 3,or 4</source>
        <translation>la priorité doit être 0, 1, 2, 3 ou 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1311"/>
        <source>priority must be 0, 1, 2 3,or 4</source>
        <translation>la priorité doit être 0, 1, 2, 3 ou 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1399"/>
        <source>invalid unit</source>
        <translation>unité invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1417"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1479"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation>nombre invalide : un entier non signé est attendu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1435"/>
        <source>invalid value</source>
        <translation>valeur invalide</translation>
    </message>
    <message>
        <source>Same as transfer, but using an older transaction building algorithm</source>
        <translation type="vanished">Comme transfer, mais utilise an algorithme de construction de transaction plus ancien</translation>
    </message>
    <message>
        <source>sweep_all [mixin] address [payment_id] - Send all unlocked balance to an address</source>
        <translation type="vanished">sweep_all [mixin] adresse [ID_paiement] - Envoyer tout le solde débloqué à une adresse</translation>
    </message>
    <message>
        <source>donate [&lt;mixin_count&gt;] &lt;amount&gt; [payment_id] - Donate &lt;amount&gt; to the development team (donate.getmonero.org)</source>
        <translation type="vanished">donate [&lt;mixin&gt;] &lt;montant&gt; [ID_paiement] - Donner &lt;montant&gt; à l&apos;équipe de développement (donate.getmonero.org)</translation>
    </message>
    <message>
        <source>set_log &lt;level&gt;|&lt;categories&gt; - Change current log detail (level must be &lt;0-4&gt;)</source>
        <translation type="vanished">set_log &lt;niveau&gt;|&lt;catégories&gt; - Changer les détails du journal (niveau entre &lt;0-4&gt;)</translation>
    </message>
    <message>
        <source>address_book [(add (&lt;address&gt; [pid &lt;long or short payment id&gt;])|&lt;integrated address&gt; [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)] - Print all entries in the address book, optionally adding/deleting an entry to/from it</source>
        <translation type="vanished">address_book [(add (&lt;adresse&gt; [pid &lt;ID de paiement long ou court&gt;])|&lt;adresse integrée&gt; [&lt;description avec des espaces possible&gt;])|(delete &lt;index&gt;)] - Afficher toutes les entrées du carnet d&apos;adresses, éventuellement en y ajoutant/supprimant une entrée</translation>
    </message>
    <message>
        <source>show_transfers [in|out|pending|failed|pool] [&lt;min_height&gt; [&lt;max_height&gt;]] - Show incoming/outgoing transfers within an optional height range</source>
        <translation type="vanished">show_transfers [in|out|pending|failed|pool] [&lt;hauteur_minimum&gt; [&lt;hauteur_maximum&gt;]] - Afficher les transferts entrants/sortants en précisant éventuellement un intervalle de hauteurs</translation>
    </message>
    <message>
        <source>unspent_outputs [&lt;min_amount&gt; &lt;max_amount&gt;] - Show unspent outputs within an optional amount range)</source>
        <translation type="obsolete">unspent_outputs [&lt;montant_minimum&gt; &lt;montant_maximum&gt;] - Afficher les sorties non dépensées en précisant éventuellement un intervalle de montants</translation>
    </message>
    <message>
        <source>Show information about a transfer to/from this address</source>
        <translation type="vanished">Afficher les informations à propos d&apos;un transfert vers/de cette adresse</translation>
    </message>
    <message>
        <source>Change wallet password</source>
        <translation type="vanished">Changer le mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1937"/>
        <source>usage: set_log &lt;log_level_number_0-4&gt; | &lt;categories&gt;</source>
        <translation>usage : set_log &lt;niveau_de_journalisation_0-4&gt; | &lt;catégories&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2008"/>
        <source>(Y/Yes/N/No): </source>
        <translation>(Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2504"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2531"/>
        <source>bad m_restore_height parameter: </source>
        <translation>mauvais paramètre m_restore_height : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2509"/>
        <source>date format must be YYYY-MM-DD</source>
        <translation>le format de date doit être AAAA-MM-JJ</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2522"/>
        <source>Restore height is: </source>
        <translation>La hauteur de restauration est : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2523"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3975"/>
        <source>Is this okay?  (Y/Yes/N/No): </source>
        <translation>Est-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2570"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Le démon est local, supposons qu&apos;il est de confiance</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2999"/>
        <source>Password for new watch-only wallet</source>
        <translation>Mot de passe pour le nouveau portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3058"/>
        <source>invalid arguments. Please use start_mining [&lt;number_of_threads&gt;] [do_bg_mining] [ignore_battery], &lt;number_of_threads&gt; should be from 1 to </source>
        <translation>arguments invalides. Veuillez utiliser start_mining [&lt;nombre_de_threads&gt;] [mine_en_arrière_plan] [ignorer_batterie], &lt;nombre_de_threads&gt; devrait être entre 1 et </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3253"/>
        <source>internal error: </source>
        <translation>erreur interne : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1180"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3258"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3551"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1114"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1185"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3263"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3556"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4025"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4133"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4366"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4565"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4860"/>
        <source>unknown error</source>
        <translation>erreur inconnue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3268"/>
        <source>refresh failed: </source>
        <translation>échec du rafraîchissement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3268"/>
        <source>Blocks received: </source>
        <translation>Blocs reçus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3299"/>
        <source>unlocked balance: </source>
        <translation>solde débloqué : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1920"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3395"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3446"/>
        <source>amount</source>
        <translation>montant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="219"/>
        <source>false</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="492"/>
        <source>Unknown command: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="499"/>
        <source>Command usage: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="502"/>
        <source>Command description: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="550"/>
        <source>wallet is multisig but not yet finalized</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="566"/>
        <source>Enter optional seed encryption passphrase, empty to see raw seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="583"/>
        <source>Failed to retrieve seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="602"/>
        <source>wallet is multisig and has no seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="673"/>
        <source>Cannot connect to daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="678"/>
        <source>Current fee is %s monero per kB</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="694"/>
        <source>Error: failed to estimate backlog array size: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="699"/>
        <source>Error: bad estimated backlog array size</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="711"/>
        <source> (current)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="714"/>
        <source>%u block (%u minutes) backlog at priority %u%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="716"/>
        <source>%u to %u block (%u to %u minutes) backlog at priority %u</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="719"/>
        <source>No backlog at priority </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="728"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="761"/>
        <source>This wallet is already multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="733"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="766"/>
        <source>wallet is watch-only and cannot be made multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="739"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="772"/>
        <source>This wallet has been used before, please use a new wallet to create a multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="746"/>
        <source>Your password is incorrect.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="752"/>
        <source>Send this multisig info to all other participants, then use make_multisig &lt;threshold&gt; &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="753"/>
        <source>This includes the PRIVATE view key, so needs to be disclosed only to that multisig wallet&apos;s participants </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="785"/>
        <source>usage: make_multisig &lt;threshold&gt; &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="793"/>
        <source>Invalid threshold</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="806"/>
        <source>Another step is needed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="808"/>
        <source>Send this multisig info to all other participants, then use finalize_multisig &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="814"/>
        <source>Error creating multisig: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="820"/>
        <source> multisig address: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="831"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="875"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="922"/>
        <source>This wallet is not multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="836"/>
        <source>This wallet is already finalized</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="849"/>
        <source>usage: finalize_multisig &lt;multisiginfo1&gt; [&lt;multisiginfo2&gt;...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="857"/>
        <source>Failed to finalize multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="863"/>
        <source>Failed to finalize multisig: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="880"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="927"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1001"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1069"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1131"/>
        <source>This multisig wallet is not yet finalized</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="885"/>
        <source>usage: export_multisig_info &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="908"/>
        <source>Error exporting multisig info: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="912"/>
        <source>Multisig info exported to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="932"/>
        <source>usage: import_multisig_info &lt;filename1&gt; [&lt;filename2&gt;...] - one for each other participant</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="960"/>
        <source>Multisig info imported</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="964"/>
        <source>Failed to import multisig info: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="975"/>
        <source>Failed to update spent status after importing multisig info: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="980"/>
        <source>Untrusted daemon, spent status may be incorrect. Use a trusted daemon and run &quot;rescan_spent&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="996"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1064"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1126"/>
        <source>This is not a multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1006"/>
        <source>usage: sign_multisig &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1019"/>
        <source>Failed to sign multisig transaction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1025"/>
        <source>Multisig error: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1030"/>
        <source>Failed to sign multisig transaction: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1053"/>
        <source>It may be relayed to the network with submit_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1074"/>
        <source>usage: submit_multisig &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1089"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1150"/>
        <source>Failed to load multisig transaction from file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1094"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1155"/>
        <source>Multisig transaction signed by only %u signers, needs %u more signatures</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1103"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6745"/>
        <source>Transaction successfully submitted, transaction </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1104"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6746"/>
        <source>You can check its status by using the `show_transfers` command.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1136"/>
        <source>usage: export_raw_multisig &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1171"/>
        <source>Failed to export multisig transaction to file </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1175"/>
        <source>Saved exported multisig transaction file(s): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1247"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1253"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1267"/>
        <source>ring size must be an integer &gt;= </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1272"/>
        <source>could not change default ring size</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1513"/>
        <source>Invalid height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1559"/>
        <source>start_mining [&lt;number_of_threads&gt;] [bg_mining] [ignore_battery]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1560"/>
        <source>Start mining in the daemon (bg_mining and ignore_battery are optional booleans).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1563"/>
        <source>Stop mining in the daemon.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1566"/>
        <source>set_daemon &lt;host&gt;[:&lt;port&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1567"/>
        <source>Set another daemon to connect to.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1570"/>
        <source>Save the current blockchain data.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1573"/>
        <source>Synchronize the transactions and balance.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1576"/>
        <source>balance [detail]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1577"/>
        <source>Show the wallet&apos;s balance of the currently selected account.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1580"/>
        <source>incoming_transfers [available|unavailable] [verbose] [index=&lt;N1&gt;[,&lt;N2&gt;[,...]]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1581"/>
        <source>Show the incoming transfers, all or filtered by availability and address index.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1584"/>
        <source>payments &lt;PID_1&gt; [&lt;PID_2&gt; ... &lt;PID_N&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1585"/>
        <source>Show the payments for the given payment IDs.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1588"/>
        <source>Show the blockchain height.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1591"/>
        <source>transfer_original [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1592"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; using an older transaction building algorithm. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the fee of the transaction. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1594"/>
        <source>transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1595"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt;. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the fee of the transaction. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1598"/>
        <source>locked_transfer [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;addr&gt; &lt;amount&gt; &lt;lockblocks&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1599"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the fee of the transaction. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1602"/>
        <source>Send all unmixable outputs to yourself with ring_size 1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1604"/>
        <source>sweep_all [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1605"/>
        <source>Send all unlocked balance to an address. If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1608"/>
        <source>sweep_below &lt;amount_threshold&gt; [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1609"/>
        <source>Send all unlocked outputs below the threshold to an address.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1612"/>
        <source>sweep_single [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;key_image&gt; &lt;address&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1613"/>
        <source>Send a single output of the given key image to an address without change.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1616"/>
        <source>donate [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;priority&gt;] [&lt;ring_size&gt;] &lt;amount&gt; [&lt;payment_id&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1617"/>
        <source>Donate &lt;amount&gt; to the development team (donate.getmonero.org).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1620"/>
        <source>sign_transfer &lt;file&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1621"/>
        <source>Sign a transaction from a &lt;file&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1624"/>
        <source>Submit a signed transaction from a file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1627"/>
        <source>set_log &lt;level&gt;|{+,-,}&lt;categories&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1628"/>
        <source>Change the current log detail (level must be &lt;0-4&gt;).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1631"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="1638"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="1647"/>
        <source>address [ new &lt;label text with white spaces allowed&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;label text with white spaces allowed&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1648"/>
        <source>If no arguments are specified or &lt;index&gt; is specified, the wallet shows the default or specified address. If &quot;all&quot; is specified, the walllet shows all the existing addresses in the currently selected account. If &quot;new &quot; is specified, the wallet creates a new address with the provided label text (which can be empty). If &quot;label&quot; is specified, the wallet sets the label of the address specified by &lt;index&gt; to the provided label text.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1651"/>
        <source>integrated_address [&lt;payment_id&gt; | &lt;address&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1652"/>
        <source>Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1655"/>
        <source>address_book [(add ((&lt;address&gt; [pid &lt;id&gt;])|&lt;integrated address&gt;) [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1656"/>
        <source>Print all entries in the address book, optionally adding/deleting an entry to/from it.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1659"/>
        <source>Save the wallet data.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1662"/>
        <source>Save a watch-only keys file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1665"/>
        <source>Display the private view key.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1668"/>
        <source>Display the private spend key.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1671"/>
        <source>Display the Electrum-style mnemonic seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1674"/>
        <source>set &lt;option&gt; [&lt;value&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1675"/>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1712"/>
        <source>Display the encrypted Electrum-style mnemonic seed.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1715"/>
        <source>Rescan the blockchain for spent outputs.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1718"/>
        <source>get_tx_key &lt;txid&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1719"/>
        <source>Get the transaction key (r) for a given &lt;txid&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1722"/>
        <source>check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1723"/>
        <source>Check the amount going to &lt;address&gt; in &lt;txid&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1726"/>
        <source>get_tx_proof &lt;txid&gt; &lt;address&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1727"/>
        <source>Generate a signature proving funds sent to &lt;address&gt; in &lt;txid&gt;, optionally with a challenge string &lt;message&gt;, using either the transaction secret key (when &lt;address&gt; is not your wallet&apos;s address) or the view secret key (otherwise), which does not disclose the secret key.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1730"/>
        <source>check_tx_proof &lt;txid&gt; &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1731"/>
        <source>Check the proof for funds going to &lt;address&gt; in &lt;txid&gt; with the challenge string &lt;message&gt; if any.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1734"/>
        <source>get_spend_proof &lt;txid&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1735"/>
        <source>Generate a signature proving that you generated &lt;txid&gt; using the spend secret key, optionally with a challenge string &lt;message&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1738"/>
        <source>check_spend_proof &lt;txid&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1739"/>
        <source>Check a signature proving that the signer generated &lt;txid&gt;, optionally with a challenge string &lt;message&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1742"/>
        <source>get_reserve_proof (all|&lt;amount&gt;) [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1743"/>
        <source>Generate a signature proving that you own at least this much, optionally with a challenge string &lt;message&gt;.
If &apos;all&apos; is specified, you prove the entire sum of all of your existing accounts&apos; balances.
Otherwise, you prove the reserve of the smallest possible amount above &lt;amount&gt; available in your current account.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1748"/>
        <source>check_reserve_proof &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1749"/>
        <source>Check a signature proving that the owner of &lt;address&gt; holds at least this much, optionally with a challenge string &lt;message&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1752"/>
        <source>show_transfers [in|out|pending|failed|pool] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1753"/>
        <source>Show the incoming/outgoing transfers within an optional height range.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1756"/>
        <source>unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_amount&gt; [&lt;max_amount&gt;]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1757"/>
        <source>Show the unspent outputs of a specified address within an optional amount range.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1760"/>
        <source>Rescan the blockchain from scratch.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1763"/>
        <source>set_tx_note &lt;txid&gt; [free text note]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1764"/>
        <source>Set an arbitrary string note for a &lt;txid&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1767"/>
        <source>get_tx_note &lt;txid&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1768"/>
        <source>Get a string note for a txid.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1771"/>
        <source>set_description [free text note]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1772"/>
        <source>Set an arbitrary description for the wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1775"/>
        <source>Get the description of the wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1778"/>
        <source>Show the wallet&apos;s status.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1781"/>
        <source>Show the wallet&apos;s information.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1784"/>
        <source>sign &lt;file&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1785"/>
        <source>Sign the contents of a file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1788"/>
        <source>verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1789"/>
        <source>Verify a signature on the contents of a file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1792"/>
        <source>export_key_images &lt;file&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1793"/>
        <source>Export a signed set of key images to a &lt;file&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1796"/>
        <source>import_key_images &lt;file&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1797"/>
        <source>Import a signed key images list and verify their spent status.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1800"/>
        <source>export_outputs &lt;file&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1801"/>
        <source>Export a set of outputs owned by this wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1804"/>
        <source>import_outputs &lt;file&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1805"/>
        <source>Import a set of outputs owned by this wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1808"/>
        <source>show_transfer &lt;txid&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1809"/>
        <source>Show information about a transfer to/from this address.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1812"/>
        <source>Change the wallet&apos;s password.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1815"/>
        <source>Generate a new random full size payment id. These will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1818"/>
        <source>Print the information about the current fee and transaction backlog.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1820"/>
        <source>Export data needed to create a multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1822"/>
        <source>make_multisig &lt;threshold&gt; &lt;string1&gt; [&lt;string&gt;...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1823"/>
        <source>Turn this wallet into a multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1826"/>
        <source>finalize_multisig &lt;string&gt; [&lt;string&gt;...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1827"/>
        <source>Turn this wallet into a multisig wallet, extra step for N-1/N wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1830"/>
        <source>export_multisig_info &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1831"/>
        <source>Export multisig info for other participants</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1834"/>
        <source>import_multisig_info &lt;filename&gt; [&lt;filename&gt;...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1835"/>
        <source>Import multisig info from other participants</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1838"/>
        <source>sign_multisig &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1839"/>
        <source>Sign a multisig transaction from a file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1842"/>
        <source>submit_multisig &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1843"/>
        <source>Submit a signed multisig transaction from a file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <source>export_raw_multisig_tx &lt;filename&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1847"/>
        <source>Export a signed multisig transaction to a file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1850"/>
        <source>help [&lt;command&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1851"/>
        <source>Show the help section or the documentation about a &lt;command&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1912"/>
        <source>integer &gt;= </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1925"/>
        <source>block height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2007"/>
        <source>No wallet found with that name. Confirm creation of new wallet named: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2063"/>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot; and --generate-from-json=&quot;jsonfilename&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2079"/>
        <source>can&apos;t specify both --restore-deterministic-wallet or --restore-multisig-wallet and --non-deterministic</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2085"/>
        <source>--restore-multisig-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2101"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;multisig seed here&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2128"/>
        <source>Multisig seed failed verification</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2144"/>
        <source>Enter seed encryption passphrase, empty if none</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2180"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2254"/>
        <source>This address is a subaddress which cannot be used here.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2332"/>
        <source>Error: expected M/N, but got: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2337"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2342"/>
        <source>Error: M/N is currently unsupported. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2345"/>
        <source>Generating master wallet from %u of %u multisig wallet keys</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2374"/>
        <source>failed to parse secret view key</source>
        <translation type="unfinished">échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2383"/>
        <source>failed to verify secret view key</source>
        <translation type="unfinished">échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2403"/>
        <source>Secret spend key (%u of %u):</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2427"/>
        <source>Error: M/N is currently unsupported</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2545"/>
        <source>Restore height </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2546"/>
        <source>Still apply restore height?  (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2577"/>
        <source>Warning: using an untrusted daemon at %s, privacy will be lessened</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2631"/>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or change the daemon address using the &apos;set_daemon&apos; command.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2763"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="2845"/>
        <source>failed to generate new mutlisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2848"/>
        <source>Generated new %u/%u multisig wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2884"/>
        <source>Opened %u/%u multisig wallet%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2937"/>
        <source>Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2995"/>
        <source>wallet is multisig and cannot save a watch-only version</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3100"/>
        <source>missing daemon URL argument</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3111"/>
        <source>Unexpected array length - Exited simple_wallet::set_daemon()</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3125"/>
        <source>This does not seem to be a valid daemon URL.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3161"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3179"/>
        <source>txid </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3163"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3181"/>
        <source>idx </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3294"/>
        <source> (Some owned outputs have partial key images - import_multisig_info needed)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3295"/>
        <source>Currently selected account: [</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3295"/>
        <source>] </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3297"/>
        <source>Tag: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3297"/>
        <source>(No tag assigned)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3304"/>
        <source>Balance per address:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3305"/>
        <source>Address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3305"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5916"/>
        <source>Balance</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3305"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5916"/>
        <source>Unlocked balance</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3305"/>
        <source>Outputs</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3305"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5916"/>
        <source>Label</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3313"/>
        <source>%8u %6s %21s %21s %7u %21s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3322"/>
        <source>usage: balance [detail]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3334"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3376"/>
        <source>usage: incoming_transfers [available|unavailable] [verbose] [index=&lt;N&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3395"/>
        <source>spent</source>
        <translation>dépensé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3395"/>
        <source>global index</source>
        <translation>index global</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3395"/>
        <source>tx id</source>
        <translation>ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3395"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3446"/>
        <source>addr index</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3418"/>
        <source>No incoming transfers</source>
        <translation>Aucun transfert entrant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3422"/>
        <source>No incoming available transfers</source>
        <translation>Aucun transfert entrant disponible</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3426"/>
        <source>No incoming unavailable transfers</source>
        <translation>Aucun transfert entrant non disponible</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3437"/>
        <source>expected at least one payment_id</source>
        <translation>au moins un ID de paiement attendu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3446"/>
        <source>payment</source>
        <translation>paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3446"/>
        <source>transaction</source>
        <translation>transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3446"/>
        <source>height</source>
        <translation>hauteur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3446"/>
        <source>unlock time</source>
        <translation>durée de déverrouillage</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3458"/>
        <source>No payments with id </source>
        <translation>Aucun paiement avec l&apos;ID </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3511"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3577"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3848"/>
        <source>failed to get blockchain height: </source>
        <translation>échec de la récupération de la hauteur de la chaîne de blocs : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3567"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5131"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5169"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5221"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5254"/>
        <source>failed to connect to the daemon</source>
        <translation>échec de la connexion au démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3585"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation>
Transaction %llu/%llu : ID=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3595"/>
        <source>
Input %llu/%llu: amount=%s</source>
        <translation>
Entrée %llu/%llu : montant=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3611"/>
        <source>failed to get output: </source>
        <translation>échec de la récupération de la sortie : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3619"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation>la hauteur du bloc d&apos;origine de la clé de la sortie ne devrait pas être supérieure à celle de la chaîne de blocs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3623"/>
        <source>
Originating block heights: </source>
        <translation>
Hauteurs des blocs d&apos;origine : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3638"/>
        <source>
|</source>
        <translation>
|</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3638"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5646"/>
        <source>|
</source>
        <translation>|
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3655"/>
        <source>
Warning: Some input keys being spent are from </source>
        <translation>
Attention : Certaines clés d&apos;entrées étant dépensées sont issues de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3657"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation>, ce qui peut casser l&apos;anonymat du cercle de signature. Assurez-vous que c&apos;est intentionnel !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3700"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4179"/>
        <source>Ring size must not be 0</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3712"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4191"/>
        <source>ring size %u is too small, minimum is %u</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3719"/>
        <source>wrong number of arguments</source>
        <translation>mauvais nombre d&apos;arguments</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3825"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4261"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4474"/>
        <source>No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): </source>
        <translation>Aucun ID de paiement n&apos;est inclus dans cette transaction. Est-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3867"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4281"/>
        <source>No outputs found, or daemon is not ready</source>
        <translation>Aucune sortie trouvée, ou le démon n&apos;est pas prêt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6738"/>
        <source>Transaction successfully saved to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6738"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6740"/>
        <source>, txid </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6740"/>
        <source>Failed to save transaction to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>failed to get random outputs to mix: </source>
        <translation type="vanished">échec de la récupération de sorties aléatoires à mélanger : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4076"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4309"/>
        <source>Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Balayage de %s dans %llu transactions pour des frais totaux de %s. Est-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4082"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4315"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4514"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Balayage de %s pour des frais totaux de %s. Est-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4625"/>
        <source>Donating </source>
        <translation>Don de </translation>
    </message>
    <message>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min mixin %lu. %sIs this okay? (Y/Yes/N/No): </source>
        <translation type="vanished">Chargement de %lu transactions, pour %s, %s de frais, %s. %s, avec mixin minimum de %lu. %sEst-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4787"/>
        <source>This is a watch only wallet</source>
        <translation>Ceci est un portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6566"/>
        <source>usage: show_transfer &lt;txid&gt;</source>
        <translation>usage : show_transfer &lt;ID_de_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6668"/>
        <source>Double spend seen on the network: this transaction may or may not end up being mined</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6703"/>
        <source>Transaction ID not found</source>
        <translation>ID de transaction non trouvé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="214"/>
        <source>true</source>
        <translation>vrai</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="267"/>
        <source>failed to parse refresh type</source>
        <translation>échec de l&apos;analyse du type de rafraîchissement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="540"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="607"/>
        <source>wallet is watch-only and has no seed</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il n&apos;a pas de graine</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="556"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="612"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation>c&apos;est un portefeuille non déterministe et il n&apos;a pas de graine</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1221"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1240"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas transférer</translation>
    </message>
    <message>
        <source>mixin must be an integer &gt;= 2</source>
        <translation type="vanished">mixin doit être un entier &gt;= 2</translation>
    </message>
    <message>
        <source>could not change default mixin</source>
        <translation type="vanished">échec du changement du mixin par défaut</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1316"/>
        <source>could not change default priority</source>
        <translation>échec du changement de la priorité par défaut</translation>
    </message>
    <message>
        <source>Synchronize transactions and balance</source>
        <translation type="vanished">Synchroniser les transactions et le solde</translation>
    </message>
    <message>
        <source>incoming_transfers [available|unavailable] - Show incoming transfers, all or filtered by availability</source>
        <translation type="vanished">incoming_transfers [available|unavailable] - Afficher les transferts entrants, soit tous soit filtrés par disponibilité</translation>
    </message>
    <message>
        <source>payments &lt;PID_1&gt; [&lt;PID_2&gt; ... &lt;PID_N&gt;] - Show payments for given payment ID[s]</source>
        <translation type="vanished">payments &lt;PID_1&gt; [&lt;PID_2&gt; ... &lt;PID_N&gt;] - Affichier les paiements pour certains ID de paiement donnés</translation>
    </message>
    <message>
        <source>transfer [&lt;priority&gt;] [&lt;mixin_count&gt;] &lt;address&gt; &lt;amount&gt; [&lt;payment_id&gt;] - Transfer &lt;amount&gt; to &lt;address&gt;. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the fee of the transaction. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;mixin_count&gt; is the number of extra inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation type="vanished">transfer [&lt;priorité&gt;] [&lt;mixin&gt;] &lt;adresse&gt; &lt;montant&gt; [&lt;ID_paiement&gt;] - Transférer &lt;montant&gt; à &lt;adresse&gt;. &lt;priorité&gt; est la priorité de la transaction. Plus la priorité est élevée, plues les frais de transaction seront élévés. Les valeurs de priorité valies sont dans l&apos;ordre (de la plus basse à la plus élevée) : unimportant, normal, elevated, priority. Si ce paramètre est omis, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;mixin&gt; est le nombre d&apos;entrées supplémentaires à inclure pour l&apos;intraçabilité. De multiples paiements peuvent être effectués d&apos;un coup en ajoutant &lt;adresse_2&gt; &lt;montant_2&gt; et cetera (avant l&apos;ID de paiement, s&apos;il est inclus)</translation>
    </message>
    <message>
        <source>locked_transfer [&lt;mixin_count&gt;] &lt;addr&gt; &lt;amount&gt; &lt;lockblocks&gt;(Number of blocks to lock the transaction for, max 1000000) [&lt;payment_id&gt;]</source>
        <translation type="vanished">locked_transfer [&lt;mixin&gt;] &lt;adresse&gt; &lt;montant&gt; &lt;blocs_vérrous&gt;(Nombre de blocs pendant lequel vérrouiller la transaction, maximum 1000000) [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <source>Send all unmixable outputs to yourself with mixin 0</source>
        <translation type="vanished">Envoyer toutes les sorties non méleangeables à vous-même avec un mixin de 0</translation>
    </message>
    <message>
        <source>Sign a transaction from a file</source>
        <translation type="vanished">Signer une transaction d&apos;un fichier</translation>
    </message>
    <message>
        <source>Submit a signed transaction from a file</source>
        <translation type="vanished">Soumettre une transaction signée d&apos;un fichier</translation>
    </message>
    <message>
        <source>integrated_address [PID] - Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation type="vanished">integrated_address [PID] - Encoder un ID de paiement dans une adresse intégrée pour l&apos;adresse publique du portefeuille actuel (sans argument un ID de paiement aléatoire est utilisé), ou décoder une adresse intégrée en une adresse standard et un ID de paiement</translation>
    </message>
    <message>
        <source>Save wallet data</source>
        <translation type="vanished">Sauvegarder les données du portefeuille</translation>
    </message>
    <message>
        <source>Save a watch-only keys file</source>
        <translation type="vanished">Sauvegarder un fichier de clés d&apos;audit</translation>
    </message>
    <message>
        <source>Display private view key</source>
        <translation type="vanished">Afficher la clé privée d&apos;audit</translation>
    </message>
    <message>
        <source>Display private spend key</source>
        <translation type="vanished">Afficher la clé privée de dépense</translation>
    </message>
    <message>
        <source>Display Electrum-style mnemonic seed</source>
        <translation type="vanished">Afficher la graine mnémonique de style Electrum</translation>
    </message>
    <message>
        <source>Available options: seed language - set wallet seed language; always-confirm-transfers &lt;1|0&gt; - whether to confirm unsplit txes; print-ring-members &lt;1|0&gt; - whether to print detailed information about ring members during confirmation; store-tx-info &lt;1|0&gt; - whether to store outgoing tx info (destination address, payment ID, tx secret key) for future reference; default-mixin &lt;n&gt; - set default mixin (default is 4); auto-refresh &lt;1|0&gt; - whether to automatically sync new blocks from the daemon; refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt; - set wallet refresh behaviour; priority [0|1|2|3|4] - default/unimportant/normal/elevated/priority fee; confirm-missing-payment-id &lt;1|0&gt;; ask-password &lt;1|0&gt;; unit &lt;monero|millinero|micronero|nanonero|piconero&gt; - set default monero (sub-)unit; min-outputs-count [n] - try to keep at least that many outputs of value at least min-outputs-value; min-outputs-value [n] - try to keep at least min-outputs-count outputs of at least that value; merge-destinations &lt;1|0&gt; - whether to merge multiple payments to the same destination address</source>
        <translation type="vanished">Options disponibles : seed language - définir la langue de la graine du portefeuille; always-confirm-transfers &lt;1|0&gt; - confirmer ou non les transactions non scindées; print-ring-members &lt;1|0&gt; - afficher ou non des informations détaillées sur les membres du cercle pendant la confirmation; store-tx-info &lt;1|0&gt; - sauvegarder ou non les informations des transactions sortantes (adresse de destination, ID de paiement, clé secrète de transaction) pour référence future; default-mixin &lt;n&gt; - définir le mixin par défaut (4 par défaut); auto-refresh &lt;1|0&gt; - synchroniser automatiquement ou non les nouveau blocs du démon; refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt; - définir le comportement du rafraîchissement du portefeuille; priority [0|1|2|3|4] - frais de transaction par défault/peu important/normal/élevé/prioritaire; confirm-missing-payment-id &lt;1|0&gt;; ask-password &lt;1|0&gt;; unit &lt;monero|millinero|micronero|nanonero|piconero&gt; - définir la (sous-)unité monero par défaut; min-outputs-count [n] - essayer de garder au moins ce nombre de sortie d&apos;une valeur d&apos;au moins min-outputs-value; min-outputs-value [n] - essayer de garder au moins min-outputs-count sorties d&apos;une valeur d&apos;au moins ce montant; merge-destinations &lt;1|0&gt; - fusionner ou non de multiples paiements à une même adresse de destination</translation>
    </message>
    <message>
        <source>Rescan blockchain for spent outputs</source>
        <translation type="vanished">Réexaminer la chaîne de blocs pour trouver les sorties dépensées</translation>
    </message>
    <message>
        <source>Get transaction key (r) for a given &lt;txid&gt;</source>
        <translation type="vanished">Obtenir la clé de transaction (r) pour un &lt;ID_de_transaction&gt; donné</translation>
    </message>
    <message>
        <source>Check amount going to &lt;address&gt; in &lt;txid&gt;</source>
        <translation type="vanished">Vérifier le montant allant à &lt;adresse&gt; dans &lt;ID_de_transaction&gt;</translation>
    </message>
    <message>
        <source>unspent_outputs [&lt;min_amount&gt; &lt;max_amount&gt;] - Show unspent outputs within an optional amount range</source>
        <translation type="vanished">unspent_outputs [&lt;montant_minimum&gt; &lt;montant_maximum&gt;] - Afficher les sorties non dépensées en précisant éventuellement un intervalle de montants</translation>
    </message>
    <message>
        <source>Rescan blockchain from scratch</source>
        <translation type="vanished">Réexaminer la chaîne de blocs depuis le début</translation>
    </message>
    <message>
        <source>Set an arbitrary string note for a txid</source>
        <translation type="vanished">Définir une note arbitraire pour un ID de transaction</translation>
    </message>
    <message>
        <source>Get a string note for a txid</source>
        <translation type="vanished">Obtenir une note pour un ID de transaction</translation>
    </message>
    <message>
        <source>Show wallet status information</source>
        <translation type="vanished">Afficher les informations sur le statut du portefuille</translation>
    </message>
    <message>
        <source>Sign the contents of a file</source>
        <translation type="vanished">Signer le contenu d&apos;un fichier</translation>
    </message>
    <message>
        <source>Verify a signature on the contents of a file</source>
        <translation type="vanished">Vérifier une signature du contenu d&apos;un fichier</translation>
    </message>
    <message>
        <source>Export a signed set of key images</source>
        <translation type="vanished">Exporter un ensemble signé d&apos;images de clé</translation>
    </message>
    <message>
        <source>Import signed key images list and verify their spent status</source>
        <translation type="vanished">Importer un ensemble signé d&apos;images de clé et vérifier leurs statuts de dépense</translation>
    </message>
    <message>
        <source>Export a set of outputs owned by this wallet</source>
        <translation type="vanished">Exporter un ensemble de sorties possédées par ce portefeuille</translation>
    </message>
    <message>
        <source>Import set of outputs owned by this wallet</source>
        <translation type="vanished">Importer un ensemble de sorties possédées par ce portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1914"/>
        <source>full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)</source>
        <translation>full (le plus lent, aucune supposition); optimize-coinbase (rapide, suppose que la récompense de bloc est payée à une unique adresse); no-coinbase (le plus rapide, suppose que l&apos;on ne reçoit aucune récompense de bloc), default (comme optimize-coinbase)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1918"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation>monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1970"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation>Nom de portefeuille non valide. Veuillez réessayer ou utilisez Ctrl-C pour quitter.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1987"/>
        <source>Wallet and key files found, loading...</source>
        <translation>Fichier portefeuille et fichier de clés trouvés, chargement...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1993"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation>Fichier de clés trouvé mais pas le fichier portefeuille. Régénération...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1999"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation>Fichier de clés non trouvé. Échec de l&apos;ouverture du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2018"/>
        <source>Generating new wallet...</source>
        <translation>Génération du nouveau portefeuille...</translation>
    </message>
    <message>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-keys=&quot;wallet_name&quot;</source>
        <translation type="vanished">impossible de spécifier plus d&apos;une option parmis --generate-new-wallet=&quot;nom_du_portefeuille&quot;, --wallet-file=&quot;nom_du_portefeuille&quot;, --generate-from-view-key=&quot;nom_du_portefeuille&quot;, --generate-from-json=&quot;nom_du_fichier_json&quot; et --generate-from-keys=&quot;nom_du_portefeuille&quot;</translation>
    </message>
    <message>
        <source>can&apos;t specify both --restore-deterministic-wallet and --non-deterministic</source>
        <translation type="vanished">impossible de spécifier à la fois --restore-deterministic-wallet et --non-deterministic</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2136"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2169"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2189"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2224"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2243"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2263"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2279"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2327"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2352"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2368"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2408"/>
        <source>No data supplied, cancelled</source>
        <translation>Pas de données fournies, annulation</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2175"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2249"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2358"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3786"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4235"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4449"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4921"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4989"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5053"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5261"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6101"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6348"/>
        <source>failed to parse address</source>
        <translation>échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2195"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2285"/>
        <source>failed to parse view key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2205"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2303"/>
        <source>failed to verify view key secret key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2209"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2307"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2388"/>
        <source>view key does not match standard address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2214"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2233"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2311"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2445"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2475"/>
        <source>account creation failed</source>
        <translation>échec de la création du compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2229"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2269"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2413"/>
        <source>failed to parse spend key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2295"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2434"/>
        <source>failed to verify spend key secret key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2299"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2439"/>
        <source>spend key does not match standard address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2557"/>
        <source>failed to open account</source>
        <translation>échec de l&apos;ouverture du compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2561"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3025"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3080"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3137"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4957"/>
        <source>wallet is null</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or restart the wallet with the correct daemon address.</source>
        <translation type="vanished">Le démon n&apos;est pas démarré ou un mauvais port a été passé. Veuillez vous assurer que le démon fonctionne ou redémarrez le portefeuille avec l&apos;adresse de démon correcte.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2675"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2680"/>
        <source>invalid language choice passed. Please try again.
</source>
        <translation>choix de langue passé invalide. Veuillez réessayer.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2748"/>
        <source>View key: </source>
        <translation>Clé d&apos;audit : </translation>
    </message>
    <message>
        <source>Your wallet has been generated!
To start synchronizing with the daemon, use the &quot;refresh&quot; command.
Use the &quot;help&quot; command to see the list of available commands.
Always use the &quot;exit&quot; command when closing monero-wallet-cli to save 
your current session&apos;s state. Otherwise, you might need to synchronize 
your wallet again (your wallet keys are NOT at risk in any case).
</source>
        <translation type="vanished">Votre portefeuille a été généré !
Pour commencer la synchronisation avec le démon, utilisez la commande &quot;refresh&quot;.
Utilisez la commande &quot;help&quot; pour voir la liste des commandes disponibles.
Utilisez toujours la commande &quot;exit&quot; pour fermer monero-wallet-cli afin de sauvegarder
l&apos;état actuel de votre session. Sinon vous pourriez avoir besoin de synchroniser
votre portefeuille à nouveau (mais les clés de votre portefeuille ne risquent rien).
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2930"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation>Vous pourriez vouloir supprimer le fichier &quot;%s&quot; et réessayer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2958"/>
        <source>failed to deinitialize wallet</source>
        <translation>échec de la désinitialisation du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3016"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3519"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6405"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation>cette commande requiert un démon de confiance. Activer avec --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3147"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation>la chaîne de blocs ne peut pas être sauvegardée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3234"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3533"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3238"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3537"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3248"/>
        <source>refresh error: </source>
        <translation>erreur du rafraîchissement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3298"/>
        <source>Balance: </source>
        <translation>Solde : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3394"/>
        <source>pubkey</source>
        <translation>clé publique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3394"/>
        <source>key image</source>
        <translation>image de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3395"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3405"/>
        <source>unlocked</source>
        <translation>déverrouillé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3395"/>
        <source>ringct</source>
        <translation>ringct</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3404"/>
        <source>T</source>
        <translation>V</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3404"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3405"/>
        <source>locked</source>
        <translation>vérrouillé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3406"/>
        <source>RingCT</source>
        <translation>RingCT</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3406"/>
        <source>-</source>
        <translation>-</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3480"/>
        <source>payment ID has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3541"/>
        <source>failed to get spent status</source>
        <translation>échec de la récupération du statut de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3656"/>
        <source>the same transaction</source>
        <translation>la même transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3656"/>
        <source>blocks that are temporally very close</source>
        <translation>blocs très proches dans le temps</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3773"/>
        <source>Locked blocks too high, max 1000000 (˜4 yrs)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5072"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5183"/>
        <source>Good signature</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5099"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5185"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5288"/>
        <source>Bad signature</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6041"/>
        <source>usage: integrated_address [payment ID]</source>
        <translation>usage : integrated_address [ID paiement]</translation>
    </message>
    <message>
        <source>Integrated address: account %s, payment ID %s</source>
        <translation type="vanished">Adresse intégrée : compte %s, ID de paiement %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6077"/>
        <source>Standard address: </source>
        <translation>Adresse standard : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6082"/>
        <source>failed to parse payment ID or address</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement ou de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6093"/>
        <source>usage: address_book [(add (&lt;address&gt; [pid &lt;long or short payment id&gt;])|&lt;integrated address&gt; [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</source>
        <translation>usage : address_book [(add (&lt;adresse&gt; [pid &lt;ID de paiement long ou court&gt;])|&lt;adresse integrée&gt; [&lt;description avec des espaces possible&gt;])|(delete &lt;index&gt;)]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6123"/>
        <source>failed to parse payment ID</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6141"/>
        <source>failed to parse index</source>
        <translation>échec de l&apos;analyse de l&apos;index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6149"/>
        <source>Address book is empty.</source>
        <translation>Le carnet d&apos;adresses est vide.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6155"/>
        <source>Index: </source>
        <translation>Index : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6156"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6282"/>
        <source>Address: </source>
        <translation>Adresse : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6157"/>
        <source>Payment ID: </source>
        <translation>ID de paiement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6158"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6281"/>
        <source>Description: </source>
        <translation>Description : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6168"/>
        <source>usage: set_tx_note [txid] free text note</source>
        <translation>usage : set_tx_note [ID transaction] note de texte libre</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6196"/>
        <source>usage: get_tx_note [txid]</source>
        <translation>usage : get_tx_note [ID transaction]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6299"/>
        <source>usage: sign &lt;filename&gt;</source>
        <translation>usage : sign &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6304"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas signer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="946"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6318"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6341"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6496"/>
        <source>failed to read file </source>
        <translation>échec de la lecture du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5034"/>
        <source>usage: check_tx_proof &lt;txid&gt; &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5061"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5176"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5273"/>
        <source>failed to load signature file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5112"/>
        <source>usage: get_spend_proof &lt;txid&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5118"/>
        <source>wallet is watch-only and cannot generate the proof</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5156"/>
        <source>usage: check_spend_proof &lt;txid&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5197"/>
        <source>usage: get_reserve_proof (all|&lt;amount&gt;) [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5203"/>
        <source>The reserve proof can be generated only by a full wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5248"/>
        <source>usage: check_reserve_proof &lt;address&gt; &lt;signature_file&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5266"/>
        <source>Address must not be a subaddress</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5284"/>
        <source>Good signature -- total: %s, spent: %s, unspent: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5348"/>
        <source>usage: show_transfers [in|out|all|pending|failed] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5485"/>
        <source>[Double spend seen on the network: this transaction may or may not end up being mined] </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5521"/>
        <source>usage: unspent_outputs [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_amount&gt; [&lt;max_amount&gt;]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5581"/>
        <source>There is no unspent output in the specified address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5694"/>
        <source> (no daemon)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5696"/>
        <source> (out of sync)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5753"/>
        <source>(Untitled account)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5766"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5784"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5809"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5832"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5985"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6008"/>
        <source>failed to parse index: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5771"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5990"/>
        <source>specify an index between 0 and </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5868"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="5896"/>
        <source>
Grand total:
  Balance: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5896"/>
        <source>, unlocked balance: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5904"/>
        <source>Untagged accounts:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5910"/>
        <source>Tag %s is unregistered.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5913"/>
        <source>Accounts with tag: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5914"/>
        <source>Tag&apos;s description: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5916"/>
        <source>Account</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5922"/>
        <source> %c%8u %6s %21s %21s %21s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5932"/>
        <source>----------------------------------------------------------------------------------</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5933"/>
        <source>%15s %21s %21s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5956"/>
        <source>Primary address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5956"/>
        <source>(used)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5977"/>
        <source>(Untitled address)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6017"/>
        <source>&lt;index_min&gt; is already out of bound</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6022"/>
        <source>&lt;index_max&gt; exceeds the bound</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6030"/>
        <source>usage: address [ new &lt;label text with white spaces allowed&gt; | all | &lt;index_min&gt; [&lt;index_max&gt;] | label &lt;index&gt; &lt;label text with white spaces allowed&gt; ]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6048"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6060"/>
        <source>Integrated addresses can only be created for account 0</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6072"/>
        <source>Integrated address: %s, payment ID: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6077"/>
        <source>Subaddress: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6237"/>
        <source>usage: get_description</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6243"/>
        <source>no description found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6245"/>
        <source>description found: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6280"/>
        <source>Filename: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6285"/>
        <source>Watch only</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6287"/>
        <source>%u/%u multisig%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6289"/>
        <source>Normal</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6290"/>
        <source>Type: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6291"/>
        <source>Testnet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6291"/>
        <source>Yes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6291"/>
        <source>No</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6309"/>
        <source>This wallet is multisig and cannot sign</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6330"/>
        <source>usage: verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation>usage : verify &lt;fichier&gt; &lt;adresse&gt; &lt;signature&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6355"/>
        <source>Bad signature from </source>
        <translation>Mauvaise signature de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6359"/>
        <source>Good signature from </source>
        <translation>Bonne signature de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6368"/>
        <source>usage: export_key_images &lt;filename&gt;</source>
        <translation>usage : export_key_images &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6373"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas exporter les images de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="901"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6386"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6468"/>
        <source>failed to save file </source>
        <translation>échec de l&apos;enregistrement du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6397"/>
        <source>Signed key images exported to </source>
        <translation>Images de clé signées exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6411"/>
        <source>usage: import_key_images &lt;filename&gt;</source>
        <translation>usage : import_key_images &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6442"/>
        <source>usage: export_outputs &lt;filename&gt;</source>
        <translation>usage : export_outputs &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6479"/>
        <source>Outputs exported to </source>
        <translation>Sorties exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6487"/>
        <source>usage: import_outputs &lt;filename&gt;</source>
        <translation>usage : import_outputs &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3814"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5214"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5540"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5548"/>
        <source>amount is wrong: </source>
        <translation>montant erroné : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3815"/>
        <source>expected number from 0 to </source>
        <translation>attend un nombre de 0 à </translation>
    </message>
    <message>
        <source>Money successfully sent, transaction </source>
        <translation type="vanished">Fonds envoyés avec succès, transaction </translation>
    </message>
    <message>
        <source>no connection to daemon. Please, make sure daemon is running.</source>
        <translation type="vanished">pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <source>not enough outputs for specified mixin_count</source>
        <translation type="vanished">pas assez de sorties pour le mixin spécifié</translation>
    </message>
    <message>
        <source>output amount</source>
        <translation type="vanished">montant de la sortie</translation>
    </message>
    <message>
        <source>found outputs to mix</source>
        <translation type="vanished">sorties à mélanger trouvées</translation>
    </message>
    <message>
        <source>transaction was not constructed</source>
        <translation type="vanished">la transaction n&apos;a pas été construite</translation>
    </message>
    <message>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation type="vanished">la transaction %s a été rejetée par le démon avec le statut : </translation>
    </message>
    <message>
        <source>one of destinations is zero</source>
        <translation type="vanished">une des destinations est zéro</translation>
    </message>
    <message>
        <source>Failed to find a suitable way to split transactions</source>
        <translation type="vanished">Échec de la recherche d&apos;une façon adéquate de scinder les transactions</translation>
    </message>
    <message>
        <source>unknown transfer error: </source>
        <translation type="vanished">erreur de transfert inconnue : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4074"/>
        <source>Sweeping </source>
        <translation>Balayage de </translation>
    </message>
    <message>
        <source>Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No)</source>
        <translation type="vanished">Balayage de %s pour des frais totaux de %s. Est-ce correct ? (Y/Yes/Oui/N/No/Non)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4554"/>
        <source>Money successfully sent, transaction: </source>
        <translation>Fonds envoyés avec succès, transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4711"/>
        <source>Change goes to more than one address</source>
        <translation>La monnaie rendue va à plus d&apos;une adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4752"/>
        <source>%s change to %s</source>
        <translation>%s de monnaie rendue à %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4755"/>
        <source>no change</source>
        <translation>sans monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1039"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1052"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4821"/>
        <source>Transaction successfully signed to file </source>
        <translation>Transaction signée avec succès dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4871"/>
        <source>usage: get_tx_key &lt;txid&gt;</source>
        <translation>usage : get_tx_key &lt;ID transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4879"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4914"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4963"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5045"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5125"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5163"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6175"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6203"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6573"/>
        <source>failed to parse txid</source>
        <translation>échec de l&apos;analyse de l&apos;ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4893"/>
        <source>Tx key: </source>
        <translation>Clé de transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4898"/>
        <source>no tx keys found for this txid</source>
        <translation>aucune clé de transaction trouvée pour cet ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4907"/>
        <source>usage: get_tx_proof &lt;txid&gt; &lt;address&gt; [&lt;message&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4932"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5142"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5234"/>
        <source>signature file saved to: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4934"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5144"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5236"/>
        <source>failed to save signature file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4948"/>
        <source>usage: check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</source>
        <translation>usage : check_tx_key &lt;ID transaction&gt; &lt;clé transaction&gt; &lt;adresse&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4971"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4980"/>
        <source>failed to parse tx key</source>
        <translation>échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <source>failed to get transaction from daemon</source>
        <translation type="vanished">échec de la récupération de la transaction du démon</translation>
    </message>
    <message>
        <source>failed to parse transaction from daemon</source>
        <translation type="vanished">échec de l&apos;analyse de la transaction du démon</translation>
    </message>
    <message>
        <source>failed to validate transaction from daemon</source>
        <translation type="vanished">échec de la validation de la transaction du démon</translation>
    </message>
    <message>
        <source>failed to get the right transaction from daemon</source>
        <translation type="vanished">échec de la récupération de la bonne transaction du démon</translation>
    </message>
    <message>
        <source>failed to generate key derivation from supplied parameters</source>
        <translation type="vanished">échec de la génération de la dérivation de clé à partir des paramètres fournis</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4938"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5026"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5104"/>
        <source>error: </source>
        <translation>erreur : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5002"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5075"/>
        <source>received</source>
        <translation>a reçu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5002"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5075"/>
        <source>in txid</source>
        <translation>dans la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5021"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5094"/>
        <source>received nothing in txid</source>
        <translation>n&apos;a rien reçu dans la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5005"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5078"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation>ATTENTION : cette transaction n&apos;est pas encore inclue dans la chaîne de blocs !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5011"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5084"/>
        <source>This transaction has %u confirmations</source>
        <translation>Cette transaction a %u confirmations</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5015"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5088"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation>ATTENTION : échec de la détermination du nombre de confirmations !</translation>
    </message>
    <message>
        <source>usage: show_transfers [in|out|all|pending|failed] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation type="vanished">usage : show_transfers [in|out|all|pending|failed] [&lt;hauteur_minimum&gt; [&lt;hauteur_maximum&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5396"/>
        <source>bad min_height parameter:</source>
        <translation>mauvais paramètre hauteur_minimum :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5408"/>
        <source>bad max_height parameter:</source>
        <translation>mauvais paramètre hauteur_maximum :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5468"/>
        <source>in</source>
        <translation>reçu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5468"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5509"/>
        <source>out</source>
        <translation>payé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5509"/>
        <source>failed</source>
        <translation>échoué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5509"/>
        <source>pending</source>
        <translation>en attente</translation>
    </message>
    <message>
        <source>usage: unspent_outputs [&lt;min_amount&gt; &lt;max_amount&gt;]</source>
        <translation type="vanished">usage : unspent_outputs [&lt;montant_minimum&gt; &lt;montant_maximum&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5555"/>
        <source>&lt;min_amount&gt; should be smaller than &lt;max_amount&gt;</source>
        <translation>&lt;montant_minimum&gt; doit être inférieur à &lt;montant_maximum&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5587"/>
        <source>
Amount: </source>
        <translation>
Montant : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5587"/>
        <source>, number of keys: </source>
        <translation>, nombre de clés : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5592"/>
        <source> </source>
        <translation> </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5597"/>
        <source>
Min block height: </source>
        <translation>
Hauteur de bloc minimum : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5598"/>
        <source>
Max block height: </source>
        <translation>
Hauteur de bloc maximum : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5599"/>
        <source>
Min amount found: </source>
        <translation>
Montant minimum trouvé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5600"/>
        <source>
Max amount found: </source>
        <translation>
Montant maximum trouvé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5601"/>
        <source>
Total count: </source>
        <translation>
Compte total : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5641"/>
        <source>
Bin size: </source>
        <translation>
Taille de classe : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5642"/>
        <source>
Outputs per *: </source>
        <translation>
Sorties par * : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5644"/>
        <source>count
  ^
</source>
        <translation>compte
  ^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5646"/>
        <source>  |</source>
        <translation>  |</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5648"/>
        <source>  +</source>
        <translation>  +</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5648"/>
        <source>+--&gt; block height
</source>
        <translation>+--&gt; hauteur de bloc
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5649"/>
        <source>   ^</source>
        <translation>   ^</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5649"/>
        <source>^
</source>
        <translation>^
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5650"/>
        <source>  </source>
        <translation>  </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5691"/>
        <source>wallet</source>
        <translation>portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="665"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6052"/>
        <source>Random payment ID: </source>
        <translation>ID de paiement aléatoire : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6053"/>
        <source>Matching integrated address: </source>
        <translation>Adresse intégrée correspondante : </translation>
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
        <source>How many participants wil share parts of the multisig wallet</source>
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
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="81"/>
        <source>Generating %u %u/%u multisig wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="138"/>
        <source>Error verifying multisig extra info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="146"/>
        <source>Error finalizing multisig</source>
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
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="176"/>
        <source>This program generates a set of multisig wallets - use this simpler scheme only if all the participants trust each other</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="194"/>
        <source>Error: expected N/M, but got: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="202"/>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="211"/>
        <source>Error: either --scheme or both of --threshold and --participants may be given</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="218"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got N==%u and M==%d</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="227"/>
        <source>Error: --filename-base is required</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="233"/>
        <source>Error: unsupported scheme: only N/N and N-1/N are supported</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>sw</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="115"/>
        <source>Generate new wallet and save it to &lt;arg&gt;</source>
        <translation>Générer un nouveau portefeuille et le sauvegarder dans &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="116"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>Générer un portefeuille d&apos;audit à partir d&apos;une clé d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="117"/>
        <source>Generate deterministic wallet from spend key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="118"/>
        <source>Generate wallet from private keys</source>
        <translation>Générer un portefeuille à partir de clés privées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="119"/>
        <source>Generate a master wallet from multisig wallet keys</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="121"/>
        <source>Language for mnemonic</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="122"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>Spécifier la graine Electrum pour la récupération/création d&apos;un portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="123"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Récupérer un portefeuille en utilisant une graine mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="124"/>
        <source>Recover multisig wallet using Electrum-style mnemonic seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="125"/>
        <source>Generate non-deterministic view and spend keys</source>
        <translation>Générer des clés d&apos;audit et de dépense non déterministes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="126"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation>Activer les commandes qui dépendent d&apos;un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="127"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>Autoriser la communication avec un démon utilisant une version de RPC différente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="128"/>
        <source>Restore from specific blockchain height</source>
        <translation>Restaurer à partir d&apos;une hauteur de bloc spécifique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="129"/>
        <source>The newly created transaction will not be relayed to the monero network</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="171"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="180"/>
        <source>possibly lost connection to daemon</source>
        <translation>connexion avec le démon peut-être perdue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="197"/>
        <source>Error: </source>
        <translation>Erreur : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6782"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6796"/>
        <source>Failed to initialize wallet</source>
        <translation>Échec de l&apos;initialisation du portefeuille</translation>
    </message>
</context>
<context>
    <name>tools::dns_utils</name>
    <message>
        <source>DNSSEC validation passed</source>
        <translation type="vanished">validation DNSSEC réussie</translation>
    </message>
    <message>
        <source>WARNING: DNSSEC validation was unsuccessful, this address may not be correct!</source>
        <translation type="vanished">ATTENTION : validation DNSSEC échouée, cette adresse pourrait ne pas être correcte !</translation>
    </message>
    <message>
        <source>For URL: </source>
        <translation type="vanished">Pour l&apos;URL : </translation>
    </message>
    <message>
        <source> Monero Address = </source>
        <translation type="vanished"> Adresse Monero = </translation>
    </message>
    <message>
        <source>Is this OK? (Y/n) </source>
        <translation type="vanished">Est-ce correct ? (Y/n) </translation>
    </message>
    <message>
        <source>you have cancelled the transfer request</source>
        <translation type="vanished">vous avez annulé la requête de transfert</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="113"/>
        <source>Use daemon instance at &lt;host&gt;:&lt;port&gt;</source>
        <translation>Utiliser l&apos;instance de démon située à &lt;hôte&gt;:&lt;port&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="114"/>
        <source>Use daemon instance at host &lt;arg&gt; instead of localhost</source>
        <translation>Utiliser l&apos;instance de démon située à l&apos;hôte &lt;arg&gt; au lieu de localhost</translation>
    </message>
    <message>
        <source>Wallet password</source>
        <translation type="vanished">Mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="116"/>
        <source>Wallet password file</source>
        <translation>Fichier mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="117"/>
        <source>Use daemon instance at port &lt;arg&gt; instead of 18081</source>
        <translation>Utiliser l&apos;instance de démon située au port &lt;arg&gt; au lieu de 18081</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="119"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>Pour testnet, le démon doit aussi être lancé avec l&apos;option --testnet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="120"/>
        <source>Restricts to view-only commands</source>
        <translation>Restreindre aux commandes en lecture-seule</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="168"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>impossible de spécifier l&apos;hôte ou le port du démon plus d&apos;une fois</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="204"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>impossible de spécifier plus d&apos;une option parmis --password et --password-file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="217"/>
        <source>the password file specified could not be read</source>
        <translation>le fichier mot de passe spécifié n&apos;a pas pu être lu</translation>
    </message>
    <message>
        <source>Enter new wallet password</source>
        <translation type="vanished">Entrez le mot de passe du portefeuille</translation>
    </message>
    <message>
        <source>failed to read wallet password</source>
        <translation type="vanished">échec de la lecture du mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="240"/>
        <source>Failed to load file </source>
        <translation>Échec du chargement du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="115"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation>Mot de passe du portefeuille (échapper/citer si nécessaire)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="118"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>Spécifier le nom_utilisateur:[mot_de_passe] pour le client RPC du démon</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="224"/>
        <source>no password specified; use --prompt-for-password to prompt for a password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="246"/>
        <source>Failed to parse JSON</source>
        <translation>Échec de l&apos;analyse JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="253"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation>Version %u trop récente, on comprend au mieux %u</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="269"/>
        <source>failed to parse view key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="274"/>
        <location filename="../src/wallet/wallet2.cpp" line="339"/>
        <location filename="../src/wallet/wallet2.cpp" line="379"/>
        <source>failed to verify view key secret key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="285"/>
        <source>failed to parse spend key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="290"/>
        <location filename="../src/wallet/wallet2.cpp" line="349"/>
        <location filename="../src/wallet/wallet2.cpp" line="404"/>
        <source>failed to verify spend key secret key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="302"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="319"/>
        <source>At least one of Electrum-style word list and private view key and private spend key must be specified</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>At least one of Electrum-style word list and private view key must be specified</source>
        <translation type="vanished">Au moins une des listes de mots de style Electrum ou clé privée d&apos;audit doit être spécifiée</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="323"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Liste de mots de style Electrum et clé privée spécifiées en même temps</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="333"/>
        <source>invalid address</source>
        <translation>adresse invalide</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="342"/>
        <source>view key does not match standard address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="352"/>
        <source>spend key does not match standard address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="360"/>
        <source>Cannot generate deprecated wallets from JSON</source>
        <translation>Impossible de générer un portefeuille obsolète à partir de JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="391"/>
        <source>failed to parse address: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="397"/>
        <source>Address must be specified in order to create watch-only wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="412"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="2811"/>
        <location filename="../src/wallet/wallet2.cpp" line="2871"/>
        <location filename="../src/wallet/wallet2.cpp" line="2950"/>
        <location filename="../src/wallet/wallet2.cpp" line="2996"/>
        <location filename="../src/wallet/wallet2.cpp" line="3087"/>
        <location filename="../src/wallet/wallet2.cpp" line="3187"/>
        <location filename="../src/wallet/wallet2.cpp" line="3597"/>
        <location filename="../src/wallet/wallet2.cpp" line="3953"/>
        <source>Primary account</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="7912"/>
        <source>No funds received in this tx.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="8605"/>
        <source>failed to read file </source>
        <translation>échec de la lecture du fichier </translation>
    </message>
</context>
<context>
    <name>tools::wallet_rpc_server</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="160"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Le démon est local, supposons qu&apos;il est de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="175"/>
        <source>Failed to create directory </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="177"/>
        <source>Failed to create directory %s: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="188"/>
        <source>Cannot specify --</source>
        <translation>Impossible de spécifier --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="188"/>
        <source> and --</source>
        <translation> et --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="207"/>
        <source>Failed to create file </source>
        <translation>Échec de la création du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="207"/>
        <source>. Check permissions or remove file</source>
        <translation>. Vérifiez les permissions ou supprimez le fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="217"/>
        <source>Error writing to file </source>
        <translation>Erreur d&apos;écriture dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="220"/>
        <source>RPC username/password is stored in file </source>
        <translation>nom_utilisateur/mot_de_passe RPC sauvegardé dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="443"/>
        <source>Tag %s is unregistered.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2418"/>
        <source>Transaction not possible. Available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2853"/>
        <source>This is the RPC monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2876"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>Impossible de spécifier plus d&apos;une option parmis --wallet-file et --generate-from-json</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2888"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>--wallet-file, --generate-from-json ou --wallet-dir doit être spécifié</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2892"/>
        <source>Loading wallet...</source>
        <translation>Chargement du portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2925"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2958"/>
        <source>Storing wallet...</source>
        <translation>Sauvegarde du portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2927"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2960"/>
        <source>Stored ok</source>
        <translation>Sauvegarde OK</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2930"/>
        <source>Loaded ok</source>
        <translation>Chargement OK</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2934"/>
        <source>Wallet initialization failed: </source>
        <translation>Échec de l&apos;initialisation du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2941"/>
        <source>Failed to initialize wallet rpc server</source>
        <translation>Échec de l&apos;initialisation du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2945"/>
        <source>Starting wallet rpc server</source>
        <translation>Démarrage du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2952"/>
        <source>Failed to run wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2955"/>
        <source>Stopped wallet rpc server</source>
        <translation>Arrêt du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2964"/>
        <source>Failed to store wallet: </source>
        <translation>Échec de la sauvegarde du portefeuille : </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="166"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6755"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="2839"/>
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
        <translation type="unfinished"></translation>
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
