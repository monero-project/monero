<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="fr_FR">
<context>
    <name>Monero::AddressBookImpl</name>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="55"/>
        <source>Invalid destination address</source>
        <translation>Adresse de destination invalide</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="65"/>
        <source>Invalid payment ID. Short payment ID should only be used in an integrated address</source>
        <translation>ID de paiement invalide. L&apos;identifiant de paiement court devrait seulement être utilisé dans une adresse intégrée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="72"/>
        <source>Invalid payment ID</source>
        <translation>ID de paiement invalide</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="79"/>
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
        <location filename="../src/wallet/api/pending_transaction.cpp" line="114"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="117"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="121"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transaction %s a été rejetée par le démon avec le statut : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="126"/>
        <source>. Reason: </source>
        <translation>. Raison : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="128"/>
        <source>Unknown exception: </source>
        <translation>Exception inconnue : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="131"/>
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
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="135"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>La monnaie réclamée ne va pas à une adresse payée</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="141"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>La monnaie réclamée est supérieure au paiement à l&apos;adresse de monnaie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="151"/>
        <source>Change goes to more than one address</source>
        <translation>La monnaie rendue va à plus d&apos;une adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="164"/>
        <source>sending %s to %s</source>
        <translation>envoi de %s à %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="170"/>
        <source>with no destinations</source>
        <translation>sans destination</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="176"/>
        <source>%s change to %s</source>
        <translation>%s de monnaie rendue à %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="179"/>
        <source>no change</source>
        <translation>sans monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="181"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min mixin %lu. %s</source>
        <translation>%lu transactions chargées, pour %s, %s de frais, %s, %s, avec mixin minimum de %lu, %s</translation>
    </message>
</context>
<context>
    <name>Monero::WalletImpl</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="942"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="952"/>
        <source>Failed to add short payment id: </source>
        <translation>Échec de l&apos;ajout de l&apos;ID de paiement court : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="978"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1072"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="981"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1075"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="984"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1078"/>
        <source>RPC error: </source>
        <translation>Erreur RPC : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1081"/>
        <source>failed to get random outputs to mix</source>
        <translation>échec de la récupération de sorties aléatoires à mélanger</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="994"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1088"/>
        <source>not enough money to transfer, available only %s, sent amount %s</source>
        <translation>pas assez de fonds pour le transfert, montant disponible %s, montant envoyé %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="403"/>
        <source>failed to parse address</source>
        <translation>échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="415"/>
        <source>failed to parse secret spend key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="425"/>
        <source>No view key supplied, cancelled</source>
        <translation>Pas de clé d&apos;audit fournie, annulation</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="432"/>
        <source>failed to parse secret view key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="442"/>
        <source>failed to verify secret spend key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="447"/>
        <source>spend key does not match address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="453"/>
        <source>failed to verify secret view key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="458"/>
        <source>view key does not match address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="477"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="799"/>
        <source>Failed to load unsigned transactions</source>
        <translation>Échec du chargement des transaction non signées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="820"/>
        <source>Failed to load transaction from file</source>
        <translation>Échec du chargement de la transaction du fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="838"/>
        <source>Wallet is view only</source>
        <translation>Portefeuille d&apos;audit uniquement</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="847"/>
        <source>failed to save file </source>
        <translation>échec de l&apos;enregistrement du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="874"/>
        <source>Failed to import key images: </source>
        <translation>Échec de l&apos;importation des images de clé : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="987"/>
        <source>failed to get random outputs to mix: %s</source>
        <translation>échec de la récupération de sorties aléatoires à mélanger : %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1003"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1097"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>pas assez de fonds pour le transfert, montant disponible %s, montant envoyé %s = %s + %s (frais)</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1012"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1106"/>
        <source>not enough outputs for specified mixin_count</source>
        <translation>pas assez de sorties pour le mixin spécifié</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1014"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1108"/>
        <source>output amount</source>
        <translation>montant de la sortie</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1014"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1108"/>
        <source>found outputs to mix</source>
        <translation>sorties à mélanger trouvées</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1019"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1113"/>
        <source>transaction was not constructed</source>
        <translation>la transaction n&apos;a pas été construite</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1023"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1117"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transaction %s a été rejetée par le démon avec le statut : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1030"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1124"/>
        <source>one of destinations is zero</source>
        <translation>une des destinations est zéro</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1033"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1127"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>échec de la recherche d&apos;une façon adéquate de scinder les transactions</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1036"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1130"/>
        <source>unknown transfer error: </source>
        <translation>erreur de transfert inconnue : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1039"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1133"/>
        <source>internal error: </source>
        <translation>erreur interne : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1042"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1136"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1045"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1139"/>
        <source>unknown error</source>
        <translation>erreur inconnue</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1419"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>Réexaminer les dépenses ne peut se faire qu&apos;avec un démon de confiance</translation>
    </message>
</context>
<context>
    <name>Monero::WalletManagerImpl</name>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="192"/>
        <source>failed to parse txid</source>
        <translation>échec de l&apos;analyse de l&apos;ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="199"/>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="206"/>
        <source>failed to parse tx key</source>
        <translation>échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="217"/>
        <source>failed to parse address</source>
        <translation>échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="227"/>
        <source>failed to get transaction from daemon</source>
        <translation>échec de la récupération de la transaction du démon</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="238"/>
        <source>failed to parse transaction from daemon</source>
        <translation>échec de l&apos;analyse de la transaction du démon</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="245"/>
        <source>failed to validate transaction from daemon</source>
        <translation>échec de la validation de la transaction du démon</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="250"/>
        <source>failed to get the right transaction from daemon</source>
        <translation>échec de la récupération de la bonne transaction du démon</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="257"/>
        <source>failed to generate key derivation from supplied parameters</source>
        <translation>échec de la génération de la dérivation de clé à partir des paramètres fournis</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="313"/>
        <source>error: </source>
        <translation>erreur : </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="319"/>
        <source>received</source>
        <translation>a reçu</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="319"/>
        <source>in txid</source>
        <translation>dans la transaction</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet_manager.cpp" line="323"/>
        <source>received nothing in txid</source>
        <translation>n&apos;a rien reçu dans la transaction</translation>
    </message>
</context>
<context>
    <name>Wallet</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="212"/>
        <source>Failed to parse address</source>
        <translation>Échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="219"/>
        <source>Failed to parse key</source>
        <translation>Échec de l&apos;analyse de la clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="227"/>
        <source>failed to verify key</source>
        <translation>Échec de la vérification de la clé</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="237"/>
        <source>key does not match address</source>
        <translation>la clé ne correspond pas à l&apos;adresse</translation>
    </message>
</context>
<context>
    <name>command_line</name>
    <message>
        <location filename="../src/common/command_line.cpp" line="76"/>
        <source>yes</source>
        <translation>oui</translation>
    </message>
</context>
<context>
    <name>cryptonote::rpc_args</name>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="38"/>
        <source>Specify ip to bind rpc server</source>
        <translation>Spécifier l&apos;IP à laquelle lier le serveur RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="39"/>
        <source>Specify username[:password] required for RPC server</source>
        <translation>Spécifier le nom_utilisateur[:mot_de_passe] requis pour le serveur RPC</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="40"/>
        <source>Confirm rpc-bind-ip value is NOT a loopback (local) IP</source>
        <translation>Confirmer que la valeur de rpc-bind-ip n&apos;est PAS une IP de bouclage (locale)</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="66"/>
        <source>Invalid IP address given for --</source>
        <translation>Adresse IP invalide fournie pour --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="74"/>
        <source> permits inbound unencrypted external connections. Consider SSH tunnel or SSL proxy instead. Override with --</source>
        <translation> autorise les connexions entrantes non cryptées venant de l&apos;extérieur. Considérez plutôt un tunnel SSH ou un proxy SSL. Outrepasser avec --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="89"/>
        <source>Username specified with --</source>
        <translation>Le nom d&apos;utilisateur spécifié avec --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="89"/>
        <source> cannot be empty</source>
        <translation> ne peut pas être vide</translation>
    </message>
</context>
<context>
    <name>cryptonote::simple_wallet</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="290"/>
        <source>Commands: </source>
        <translation>Commandes : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1557"/>
        <source>failed to read wallet password</source>
        <translation>échec de la lecture du mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1325"/>
        <source>invalid password</source>
        <translation>mot de passe invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="697"/>
        <source>start_mining [&lt;number_of_threads&gt;] - Start mining in daemon</source>
        <translation>start_mining [&lt;nombre_de_threads&gt;] - Démarrer l&apos;extraction minière dans le démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="698"/>
        <source>Stop mining in daemon</source>
        <translation>Stopper l&apos;extraction minière dans le démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="699"/>
        <source>Save current blockchain data</source>
        <translation>Sauvegarder les données actuelles de la chaîne de blocs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="701"/>
        <source>Show current wallet balance</source>
        <translation>Afficher le solde actuel du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="704"/>
        <source>Show blockchain height</source>
        <translation>Afficher la hauteur de la chaîne de blocs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="715"/>
        <source>Show current wallet public address</source>
        <translation>Afficher l&apos;adresse publique du portefeuille actuel</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="744"/>
        <source>Show this help</source>
        <translation>Afficher cette aide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="788"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation>set seed : requiert un argument. options disponibles : language</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="811"/>
        <source>set: unrecognized argument(s)</source>
        <translation>set : argument(s) non reconnu(s)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1442"/>
        <source>wallet file path not valid: </source>
        <translation>chemin du fichier portefeuille non valide : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="863"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation>Tentative de génération ou de restauration d&apos;un portefeuille, mais le fichier spécifié existe déjà. Sortie pour ne pas risquer de l&apos;écraser.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="416"/>
        <source>usage: payment_id</source>
        <translation>usage : payment_id</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="710"/>
        <source>sweep_below &lt;amount_threshold&gt; [mixin] address [payment_id] - Send all unlocked outputs below the threshold to an address</source>
        <translation>sweep_below &lt;montant_seuil&gt; [mixin] adresse [ID_paiement] - Envoyer toutes les sorties débloquées sous le seuil vers une adresse</translation>
    </message>
    <message>
        <source>Available options: seed language - set wallet seed language; always-confirm-transfers &lt;1|0&gt; - whether to confirm unsplit txes; print-ring-members &lt;1|0&gt; - whether to print detailed information about ring members during confirmation; store-tx-info &lt;1|0&gt; - whether to store outgoing tx info (destination address, payment ID, tx secret key) for future reference; default-mixin &lt;n&gt; - set default mixin (default is 4); auto-refresh &lt;1|0&gt; - whether to automatically sync new blocks from the daemon; refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt; - set wallet refresh behaviour; priority [0|1|2|3|4] - default/unimportant/normal/elevated/priority fee; confirm-missing-payment-id &lt;1|0&gt;; ask-password &lt;1|0&gt;; unit &lt;monero|millinero|micronero|nanonero|piconero&gt; - set default monero (sub-)unit; min-outputs-count [n] - try to keep at least that many outputs of value at least min-outputs-value; min-outputs-value [n] - try to keep at least min-outputs-count outputs of at least that value - merge-destinations &lt;1|0&gt; - whether to merge multiple payments to the same destination address</source>
        <translation type="obsolete">Options disponibles : seed language - définir la langue de la graine du portefeuille; always-confirm-transfers &lt;1|0&gt; - confirmer ou non les transactions non scindées; print-ring-members &lt;1|0&gt; - afficher ou non des informations détaillées sur les membres du cercle pendant la confirmation; store-tx-info &lt;1|0&gt; - sauvegarder ou non les informations des transactions sortantes (adresse de destination, ID de paiement, clé secrète de transaction) pour référence future; default-mixin &lt;n&gt; - définir le mixin par défaut (4 par défaut); auto-refresh &lt;1|0&gt; - synchroniser automatiquement ou non les nouveau blocs du démon; refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt; - définir le comportement du rafraîchissement du portefeuille; priority [0|1|2|3|4] - frais de transaction par défault/peu important/normal/élevé/prioritaire; confirm-missing-payment-id &lt;1|0&gt;; ask-password &lt;1|0&gt;; unit &lt;monero|millinero|micronero|nanonero|piconero&gt; - définir la (sous-)unité monero par défaut; min-outputs-count [n] - essayer de garder au moins ce nombre de sortie d&apos;une valeur d&apos;au moins min-outputs-value; min-outputs-value [n] - essayer de garder au moins min-outputs-count sorties d&apos;une valeur d&apos;au moins ce montant; merge-destinations &lt;1|0&gt; - fusionner ou non de multiples paiements à une même adresse de destination</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="743"/>
        <source>Generate a new random full size payment id - these will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids</source>
        <translation>Générer un nouvel ID de paiement long aléatoire - ceux-ci seront non cryptés dans la chaîne de blocs, voir integrated_address pour les IDs de paiement courts cryptés</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="774"/>
        <source>needs an argument</source>
        <translation>requiert un argument</translation>
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
        <translation>0 ou 1</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="800"/>
        <source>integer &gt;= 2</source>
        <translation>entier &gt;= 2</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="803"/>
        <source>0, 1, 2, 3, or 4</source>
        <translation>0, 1, 2, 3 ou 4</translation>
    </message>
    <message>
        <source>monero, millinero, micronero, nanop, piconero</source>
        <translation type="obsolete">monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="807"/>
        <source>unsigned integer</source>
        <translation>entier non signé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="912"/>
        <source>PLEASE NOTE: the following 25 words can be used to recover access to your wallet. Please write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation>VEUILLEZ NOTER : les 25 mots suivants peuvent être utilisés pour restaurer votre portefeuille. Veuillez les écrire sur papier et les garder dans un endroit sûr. Ne les gardez pas dans un courriel ou dans un service de stockage de fichiers hors de votre contrôle.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="958"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation>--restore-deterministic-wallet utilise --generate-new-wallet, pas --wallet-file</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="973"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation>spécifiez un paramètre de récupération avec --electrum-seed=&quot;liste de mots ici&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1123"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation>spécifiez un chemin de portefeuille avec --generate-new-wallet (pas --wallet-file)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1261"/>
        <source>wallet failed to connect to daemon: </source>
        <translation>échec de la connexion du portefeuille au démon : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1269"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation>Le démon utilise une version majeure de RPC (%u) différente de celle du portefeuille (%u) : %s. Mettez l&apos;un des deux à jour, ou utilisez --allow-mismatched-daemon-version.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1288"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation>Liste des langues disponibles pour la graine de votre portefeuille :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1297"/>
        <source>Enter the number corresponding to the language of your choice: </source>
        <translation>Entrez le nombre correspondant à la langue de votre choix : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1354"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Veuillez dorénavant utiliser la nouvelle graine que nous fournissons.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1368"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1425"/>
        <source>Generated new wallet: </source>
        <translation>Nouveau portefeuille généré : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1374"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1430"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1457"/>
        <source>Opened watch-only wallet</source>
        <translation>Ouverture du portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1457"/>
        <source>Opened wallet</source>
        <translation>Ouverture du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1466"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Veuillez procéder à la mise à jour de votre portefeuille.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1481"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation>Vous avez utilisé une version obsolète du portefeuille. Le format de votre fichier portefeuille est en cours de mise à jour.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1489"/>
        <source>failed to load wallet: </source>
        <translation>échec du chargement du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1497"/>
        <source>Use &quot;help&quot; command to see the list of available commands.
</source>
        <translation>Utilisez la commande &quot;help&quot; pour voir la liste des commandes disponibles.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1541"/>
        <source>Wallet data saved</source>
        <translation>Données du portefeuille sauvegardées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1613"/>
        <source>Mining started in daemon</source>
        <translation>L&apos;extraction minière dans le démon a démarré</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1615"/>
        <source>mining has NOT been started: </source>
        <translation>l&apos;extraction minière n&apos;a PAS démarré : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1634"/>
        <source>Mining stopped in daemon</source>
        <translation>L&apos;extraction minière dans le démon a été stoppée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1636"/>
        <source>mining has NOT been stopped: </source>
        <translation>l&apos;extraction minière n&apos;a PAS été stoppée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1655"/>
        <source>Blockchain saved</source>
        <translation>Chaîne de blocs sauvegardée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1670"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1687"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1699"/>
        <source>Height </source>
        <translation>Hauteur </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1671"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1688"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1700"/>
        <source>transaction </source>
        <translation>transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1672"/>
        <source>received </source>
        <translation>reçu </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1689"/>
        <source>spent </source>
        <translation>dépensé </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1701"/>
        <source>unsupported transaction format</source>
        <translation>format de transaction non supporté</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1718"/>
        <source>Starting refresh...</source>
        <translation>Démarrage du rafraîchissement...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1731"/>
        <source>Refresh done, blocks received: </source>
        <translation>Rafraîchissement effectué, blocs reçus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2186"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2701"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2201"/>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="2228"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2726"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation>une unique transaction ne peut pas utiliser plus d&apos;un ID de paiement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2237"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2735"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation>échec de la définition de l&apos;ID de paiement, bien qu&apos;il ait été décodé correctement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2262"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2355"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2533"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2749"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2794"/>
        <source>transaction cancelled.</source>
        <translation>transaction annulée.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2323"/>
        <source>Sending %s.  </source>
        <translation>Envoi de %s. </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2326"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation>Votre transaction doit être scindée en %llu transactions. Il en résulte que des frais de transaction doivent être appliqués à chaque transaction, pour un total de %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2332"/>
        <source>The transaction fee is %s</source>
        <translation>Les frais de transaction sont de %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2335"/>
        <source>, of which %s is dust from change</source>
        <translation>, dont %s est de la poussière de monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2336"/>
        <source>.</source>
        <translation>.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2336"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation>Un total de %s de poussière de monnaie rendue sera envoyé à une adresse de poussière</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2341"/>
        <source>.
This transaction will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation>.
Cette transaction sera déverrouillée au bloc %llu, dans approximativement %s jours (en supposant 2 minutes par bloc)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2367"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2544"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2805"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>Échec de l&apos;écriture de(s) transaction(s) dans le fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2371"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2548"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2809"/>
        <source>Unsigned transaction(s) successfully written to file: </source>
        <translation>Transaction(s) non signée(s) écrite(s) dans le fichier avec succès : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2406"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2583"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2844"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3157"/>
        <source>Not enough money in unlocked balance</source>
        <translation>Pas assez de fonds dans le solde débloqué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2415"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2592"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2853"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3166"/>
        <source>Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees</source>
        <translation>Échec de la recherche d&apos;une façon de créer les transactions. Ceci est généralement dû à de la poussière si petite qu&apos;elle de peut pas payer ses propre frais, à une tentative d&apos;envoi d&apos;un montant supérieur au solde débloqué, ou parce qu&apos;il n&apos;en reste pas assez pour les frais de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2435"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2612"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2873"/>
        <source>Reason: </source>
        <translation>Raison : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2447"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2624"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2885"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>échec de la recherche d&apos;une façon adéquate de scinder les transactions</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2503"/>
        <source>No unmixable outputs found</source>
        <translation>Aucune sortie non mélangeable trouvée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2709"/>
        <source>No address given</source>
        <translation>Aucune adresse fournie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2921"/>
        <source>missing amount threshold</source>
        <translation>montant seuil manquant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2926"/>
        <source>invalid amount threshold</source>
        <translation>montant seuil invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3008"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>La monnaie réclamée ne va pas à une adresse payée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3013"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>La monnaie réclamée est supérieure au paiement à l&apos;adresse de monnaie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3035"/>
        <source>sending %s to %s</source>
        <translation>envoi de %s à %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3041"/>
        <source>with no destinations</source>
        <translation>sans destination</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3088"/>
        <source>Failed to sign transaction</source>
        <translation>Échec de signature de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3094"/>
        <source>Failed to sign transaction: </source>
        <translation>Échec de signature de transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3120"/>
        <source>Failed to load transaction from file</source>
        <translation>Échec du chargement de la transaction du fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3137"/>
        <source>daemon is busy. Please try later</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1745"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1995"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2395"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2572"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2833"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3146"/>
        <source>RPC error: </source>
        <translation>Erreur RPC : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="312"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il n&apos;a pas de clé de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="390"/>
        <source>Your original password was incorrect.</source>
        <translation>Votre mot de passe original est incorrect.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="404"/>
        <source>Error with wallet rewrite: </source>
        <translation>Erreur avec la réécriture du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="513"/>
        <source>priority must be 0, 1, 2, 3, or 4 </source>
        <translation>la priorité doit être 0, 1, 2, 3 ou 4 </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="525"/>
        <source>priority must be 0, 1, 2, 3,or 4</source>
        <translation>la priorité doit être 0, 1, 2, 3 ou 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="540"/>
        <source>priority must be 0, 1, 2 3,or 4</source>
        <translation>la priorité doit être 0, 1, 2, 3 ou 4</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="623"/>
        <source>invalid unit</source>
        <translation>unité invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="641"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation>nombre invalide : un entier non signé est attendu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="659"/>
        <source>invalid value</source>
        <translation>valeur invalide</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="705"/>
        <source>Same as transfer, but using an older transaction building algorithm</source>
        <translation>Comme transfer, mais utilise an algorithme de construction de transaction plus ancien</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="709"/>
        <source>sweep_all [mixin] address [payment_id] - Send all unlocked balance to an address</source>
        <translation>sweep_all [mixin] adresse [ID_paiement] - Envoyer tout le solde débloqué à une adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="711"/>
        <source>donate [&lt;mixin_count&gt;] &lt;amount&gt; [payment_id] - Donate &lt;amount&gt; to the development team (donate.getmonero.org)</source>
        <translation>donate [&lt;mixin&gt;] &lt;montant&gt; [ID_paiement] - Donner &lt;montant&gt; à l&apos;équipe de développement (donate.getmonero.org)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="714"/>
        <source>set_log &lt;level&gt;|&lt;categories&gt; - Change current log detail (level must be &lt;0-4&gt;)</source>
        <translation>set_log &lt;niveau&gt;|&lt;catégories&gt; - Changer les détails du journal (niveau entre &lt;0-4&gt;)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="717"/>
        <source>address_book [(add (&lt;address&gt; [pid &lt;long or short payment id&gt;])|&lt;integrated address&gt; [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)] - Print all entries in the address book, optionally adding/deleting an entry to/from it</source>
        <translation>address_book [(add (&lt;adresse&gt; [pid &lt;ID de paiement long ou court&gt;])|&lt;adresse integrée&gt; [&lt;description avec des espaces possible&gt;])|(delete &lt;index&gt;)] - Afficher toutes les entrées du carnet d&apos;adresses, éventuellement en y ajoutant/supprimant une entrée</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="729"/>
        <source>show_transfers [in|out|pending|failed|pool] [&lt;min_height&gt; [&lt;max_height&gt;]] - Show incoming/outgoing transfers within an optional height range</source>
        <translation>show_transfers [in|out|pending|failed|pool] [&lt;hauteur_minimum&gt; [&lt;hauteur_maximum&gt;]] - Afficher les transferts entrants/sortants en précisant éventuellement un intervalle de hauteurs</translation>
    </message>
    <message>
        <source>unspent_outputs [&lt;min_amount&gt; &lt;max_amount&gt;] - Show unspent outputs within an optional amount range)</source>
        <translation type="obsolete">unspent_outputs [&lt;montant_minimum&gt; &lt;montant_maximum&gt;] - Afficher les sorties non dépensées en précisant éventuellement un intervalle de montants</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="741"/>
        <source>Show information about a transfer to/from this address</source>
        <translation>Afficher les informations à propos d&apos;un transfert vers/de cette adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="742"/>
        <source>Change wallet password</source>
        <translation>Changer le mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="820"/>
        <source>usage: set_log &lt;log_level_number_0-4&gt; | &lt;categories&gt;</source>
        <translation>usage : set_log &lt;niveau_de_journalisation_0-4&gt; | &lt;catégories&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="886"/>
        <source>(Y/Yes/N/No): </source>
        <translation>(Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1157"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1184"/>
        <source>bad m_restore_height parameter: </source>
        <translation>mauvais paramètre m_restore_height : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1162"/>
        <source>date format must be YYYY-MM-DD</source>
        <translation>le format de date doit être AAAA-MM-JJ</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1175"/>
        <source>Restore height is: </source>
        <translation>La hauteur de restauration est : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1176"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2348"/>
        <source>Is this okay?  (Y/Yes/N/No): </source>
        <translation>Est-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1212"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Le démon est local, supposons qu&apos;il est de confiance</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1553"/>
        <source>Password for new watch-only wallet</source>
        <translation>Mot de passe pour le nouveau portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1604"/>
        <source>invalid arguments. Please use start_mining [&lt;number_of_threads&gt;] [do_bg_mining] [ignore_battery], &lt;number_of_threads&gt; should be from 1 to </source>
        <translation>arguments invalides. Veuillez utiliser start_mining [&lt;nombre_de_threads&gt;] [mine_en_arrière_plan] [ignorer_batterie], &lt;nombre_de_threads&gt; devrait être entre 1 et </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1755"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2457"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2634"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2895"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3205"/>
        <source>internal error: </source>
        <translation>erreur interne : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1760"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2000"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2462"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2639"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2900"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3210"/>
        <source>unexpected error: </source>
        <translation>erreur inattendue : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1765"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2005"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2467"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2644"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2905"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3215"/>
        <source>unknown error</source>
        <translation>erreur inconnue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1770"/>
        <source>refresh failed: </source>
        <translation>échec du rafraîchissement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1770"/>
        <source>Blocks received: </source>
        <translation>Blocs reçus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1795"/>
        <source>unlocked balance: </source>
        <translation>solde débloqué : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="808"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>amount</source>
        <translation>montant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <source>spent</source>
        <translation>dépensé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <source>global index</source>
        <translation>index global</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <source>tx id</source>
        <translation>ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1868"/>
        <source>No incoming transfers</source>
        <translation>Aucun transfert entrant</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1872"/>
        <source>No incoming available transfers</source>
        <translation>Aucun transfert entrant disponible</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1876"/>
        <source>No incoming unavailable transfers</source>
        <translation>Aucun transfert entrant non disponible</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1887"/>
        <source>expected at least one payment_id</source>
        <translation>au moins un ID de paiement attendu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>payment</source>
        <translation>paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>transaction</source>
        <translation>transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>height</source>
        <translation>hauteur</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1896"/>
        <source>unlock time</source>
        <translation>durée de déverrouillage</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1908"/>
        <source>No payments with id </source>
        <translation>Aucun paiement avec l&apos;ID </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1960"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2026"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2280"/>
        <source>failed to get blockchain height: </source>
        <translation>échec de la récupération de la hauteur de la chaîne de blocs : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2016"/>
        <source>failed to connect to the daemon</source>
        <translation>échec de la connexion au démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2034"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation>
Transaction %llu/%llu : ID=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2044"/>
        <source>
Input %llu/%llu: amount=%s</source>
        <translation>
Entrée %llu/%llu : montant=%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2060"/>
        <source>failed to get output: </source>
        <translation>échec de la récupération de la sortie : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2068"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation>la hauteur du bloc d&apos;origine de la clé de la sortie ne devrait pas être supérieure à celle de la chaîne de blocs</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2072"/>
        <source>
Originating block heights: </source>
        <translation>
Hauteurs des blocs d&apos;origine : </translation>
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
        <translation>
Attention : Certaines clés d&apos;entrées étant dépensées sont issues de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2106"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation>, ce qui peut casser l&apos;anonymat du cercle de signature. Assurez-vous que c&apos;est intentionnel !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2152"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2937"/>
        <source>wrong number of arguments</source>
        <translation>mauvais nombre d&apos;arguments</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2257"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2744"/>
        <source>No payment id is included with this transaction. Is this okay?  (Y/Yes/N/No): </source>
        <translation>Aucun ID de paiement n&apos;est inclus dans cette transaction. Est-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2298"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2762"/>
        <source>No outputs found, or daemon is not ready</source>
        <translation>Aucune sortie trouvée, ou le démon n&apos;est pas prêt</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2399"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2576"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2837"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3150"/>
        <source>failed to get random outputs to mix: </source>
        <translation>échec de la récupération de sorties aléatoires à mélanger : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2518"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2779"/>
        <source>Sweeping %s in %llu transactions for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Balayage de %s dans %llu transactions pour des frais totaux de %s. Est-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2524"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No): </source>
        <translation>Balayage de %s pour des frais totaux de %s. Est-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2969"/>
        <source>Donating </source>
        <translation>Don de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3053"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min mixin %lu. %sIs this okay? (Y/Yes/N/No): </source>
        <translation>Chargement de %lu transactions, pour %s, %s de frais, %s. %s, avec mixin minimum de %lu. %sEst-ce correct ? (Y/Yes/Oui/N/No/Non) : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3077"/>
        <source>This is a watch only wallet</source>
        <translation>Ceci est un portefeuille d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4443"/>
        <source>usage: show_transfer &lt;txid&gt;</source>
        <translation>usage : show_transfer &lt;ID_de_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4557"/>
        <source>Transaction ID not found</source>
        <translation>ID de transaction non trouvé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="237"/>
        <source>true</source>
        <translation>vrai</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="266"/>
        <source>failed to parse refresh type</source>
        <translation>échec de l&apos;analyse du type de rafraîchissement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="330"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="362"/>
        <source>wallet is watch-only and has no seed</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il n&apos;a pas de graine</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="353"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="367"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation>c&apos;est un portefeuille non déterministe et il n&apos;a pas de graine</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="450"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="467"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas transférer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="474"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="480"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="496"/>
        <source>mixin must be an integer &gt;= 2</source>
        <translation>mixin doit être un entier &gt;= 2</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="501"/>
        <source>could not change default mixin</source>
        <translation>échec du changement du mixin par défaut</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="545"/>
        <source>could not change default priority</source>
        <translation>échec du changement de la priorité par défaut</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="700"/>
        <source>Synchronize transactions and balance</source>
        <translation>Synchroniser les transactions et le solde</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="702"/>
        <source>incoming_transfers [available|unavailable] - Show incoming transfers, all or filtered by availability</source>
        <translation>incoming_transfers [available|unavailable] - Afficher les transferts entrants, soit tous soit filtrés par disponibilité</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="703"/>
        <source>payments &lt;PID_1&gt; [&lt;PID_2&gt; ... &lt;PID_N&gt;] - Show payments for given payment ID[s]</source>
        <translation>payments &lt;PID_1&gt; [&lt;PID_2&gt; ... &lt;PID_N&gt;] - Affichier les paiements pour certains ID de paiement donnés</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="706"/>
        <source>transfer [&lt;priority&gt;] [&lt;mixin_count&gt;] &lt;address&gt; &lt;amount&gt; [&lt;payment_id&gt;] - Transfer &lt;amount&gt; to &lt;address&gt;. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the fee of the transaction. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;mixin_count&gt; is the number of extra inputs to include for untraceability. Multiple payments can be made at once by adding &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation>transfer [&lt;priorité&gt;] [&lt;mixin&gt;] &lt;adresse&gt; &lt;montant&gt; [&lt;ID_paiement&gt;] - Transférer &lt;montant&gt; à &lt;adresse&gt;. &lt;priorité&gt; est la priorité de la transaction. Plus la priorité est élevée, plues les frais de transaction seront élévés. Les valeurs de priorité valies sont dans l&apos;ordre (de la plus basse à la plus élevée) : unimportant, normal, elevated, priority. Si ce paramètre est omis, la valeur par défaut (voir la commande &quot;set priority&quot;) est utilisée. &lt;mixin&gt; est le nombre d&apos;entrées supplémentaires à inclure pour l&apos;intraçabilité. De multiples paiements peuvent être effectués d&apos;un coup en ajoutant &lt;adresse_2&gt; &lt;montant_2&gt; et cetera (avant l&apos;ID de paiement, s&apos;il est inclus)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="707"/>
        <source>locked_transfer [&lt;mixin_count&gt;] &lt;addr&gt; &lt;amount&gt; &lt;lockblocks&gt;(Number of blocks to lock the transaction for, max 1000000) [&lt;payment_id&gt;]</source>
        <translation>locked_transfer [&lt;mixin&gt;] &lt;adresse&gt; &lt;montant&gt; &lt;blocs_vérrous&gt;(Nombre de blocs pendant lequel vérrouiller la transaction, maximum 1000000) [&lt;ID_paiement&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="708"/>
        <source>Send all unmixable outputs to yourself with mixin 0</source>
        <translation>Envoyer toutes les sorties non méleangeables à vous-même avec un mixin de 0</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="712"/>
        <source>Sign a transaction from a file</source>
        <translation>Signer une transaction d&apos;un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="713"/>
        <source>Submit a signed transaction from a file</source>
        <translation>Soumettre une transaction signée d&apos;un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="716"/>
        <source>integrated_address [PID] - Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation>integrated_address [PID] - Encoder un ID de paiement dans une adresse intégrée pour l&apos;adresse publique du portefeuille actuel (sans argument un ID de paiement aléatoire est utilisé), ou décoder une adresse intégrée en une adresse standard et un ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="718"/>
        <source>Save wallet data</source>
        <translation>Sauvegarder les données du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="719"/>
        <source>Save a watch-only keys file</source>
        <translation>Sauvegarder un fichier de clés d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="720"/>
        <source>Display private view key</source>
        <translation>Afficher la clé privée d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="721"/>
        <source>Display private spend key</source>
        <translation>Afficher la clé privée de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="722"/>
        <source>Display Electrum-style mnemonic seed</source>
        <translation>Afficher la graine mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="723"/>
        <source>Available options: seed language - set wallet seed language; always-confirm-transfers &lt;1|0&gt; - whether to confirm unsplit txes; print-ring-members &lt;1|0&gt; - whether to print detailed information about ring members during confirmation; store-tx-info &lt;1|0&gt; - whether to store outgoing tx info (destination address, payment ID, tx secret key) for future reference; default-mixin &lt;n&gt; - set default mixin (default is 4); auto-refresh &lt;1|0&gt; - whether to automatically sync new blocks from the daemon; refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt; - set wallet refresh behaviour; priority [0|1|2|3|4] - default/unimportant/normal/elevated/priority fee; confirm-missing-payment-id &lt;1|0&gt;; ask-password &lt;1|0&gt;; unit &lt;monero|millinero|micronero|nanonero|piconero&gt; - set default monero (sub-)unit; min-outputs-count [n] - try to keep at least that many outputs of value at least min-outputs-value; min-outputs-value [n] - try to keep at least min-outputs-count outputs of at least that value; merge-destinations &lt;1|0&gt; - whether to merge multiple payments to the same destination address</source>
        <translation>Options disponibles : seed language - définir la langue de la graine du portefeuille; always-confirm-transfers &lt;1|0&gt; - confirmer ou non les transactions non scindées; print-ring-members &lt;1|0&gt; - afficher ou non des informations détaillées sur les membres du cercle pendant la confirmation; store-tx-info &lt;1|0&gt; - sauvegarder ou non les informations des transactions sortantes (adresse de destination, ID de paiement, clé secrète de transaction) pour référence future; default-mixin &lt;n&gt; - définir le mixin par défaut (4 par défaut); auto-refresh &lt;1|0&gt; - synchroniser automatiquement ou non les nouveau blocs du démon; refresh-type &lt;full|optimize-coinbase|no-coinbase|default&gt; - définir le comportement du rafraîchissement du portefeuille; priority [0|1|2|3|4] - frais de transaction par défault/peu important/normal/élevé/prioritaire; confirm-missing-payment-id &lt;1|0&gt;; ask-password &lt;1|0&gt;; unit &lt;monero|millinero|micronero|nanonero|piconero&gt; - définir la (sous-)unité monero par défaut; min-outputs-count [n] - essayer de garder au moins ce nombre de sortie d&apos;une valeur d&apos;au moins min-outputs-value; min-outputs-value [n] - essayer de garder au moins min-outputs-count sorties d&apos;une valeur d&apos;au moins ce montant; merge-destinations &lt;1|0&gt; - fusionner ou non de multiples paiements à une même adresse de destination</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="724"/>
        <source>Rescan blockchain for spent outputs</source>
        <translation>Réexaminer la chaîne de blocs pour trouver les sorties dépensées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="725"/>
        <source>Get transaction key (r) for a given &lt;txid&gt;</source>
        <translation>Obtenir la clé de transaction (r) pour un &lt;ID_de_transaction&gt; donné</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="726"/>
        <source>Check amount going to &lt;address&gt; in &lt;txid&gt;</source>
        <translation>Vérifier le montant allant à &lt;adresse&gt; dans &lt;ID_de_transaction&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="727"/>
        <source>Generate a signature to prove payment to &lt;address&gt; in &lt;txid&gt; using the transaction secret key (r) without revealing it</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="728"/>
        <source>Check tx proof for payment going to &lt;address&gt; in &lt;txid&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="730"/>
        <source>unspent_outputs [&lt;min_amount&gt; &lt;max_amount&gt;] - Show unspent outputs within an optional amount range</source>
        <translation>unspent_outputs [&lt;montant_minimum&gt; &lt;montant_maximum&gt;] - Afficher les sorties non dépensées en précisant éventuellement un intervalle de montants</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="731"/>
        <source>Rescan blockchain from scratch</source>
        <translation>Réexaminer la chaîne de blocs depuis le début</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="732"/>
        <source>Set an arbitrary string note for a txid</source>
        <translation>Définir une note arbitraire pour un ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="733"/>
        <source>Get a string note for a txid</source>
        <translation>Obtenir une note pour un ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="734"/>
        <source>Show wallet status information</source>
        <translation>Afficher les informations sur le statut du portefuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="735"/>
        <source>Sign the contents of a file</source>
        <translation>Signer le contenu d&apos;un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="736"/>
        <source>Verify a signature on the contents of a file</source>
        <translation>Vérifier une signature du contenu d&apos;un fichier</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="737"/>
        <source>Export a signed set of key images</source>
        <translation>Exporter un ensemble signé d&apos;images de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="738"/>
        <source>Import signed key images list and verify their spent status</source>
        <translation>Importer un ensemble signé d&apos;images de clé et vérifier leurs statuts de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="739"/>
        <source>Export a set of outputs owned by this wallet</source>
        <translation>Exporter un ensemble de sorties possédées par ce portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="740"/>
        <source>Import set of outputs owned by this wallet</source>
        <translation>Importer un ensemble de sorties possédées par ce portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="802"/>
        <source>full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)</source>
        <translation>full (le plus lent, aucune supposition); optimize-coinbase (rapide, suppose que la récompense de bloc est payée à une unique adresse); no-coinbase (le plus rapide, suppose que l&apos;on ne reçoit aucune récompense de bloc), default (comme optimize-coinbase)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="806"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation>monero, millinero, micronero, nanonero, piconero</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="851"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation>Nom de portefeuille non valide. Veuillez réessayer ou utilisez Ctrl-C pour quitter.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="868"/>
        <source>Wallet and key files found, loading...</source>
        <translation>Fichier portefeuille et fichier de clés trouvés, chargement...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="874"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation>Fichier de clés trouvé mais pas le fichier portefeuille. Régénération...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="880"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation>Fichier de clés non trouvé. Échec de l&apos;ouverture du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="894"/>
        <source>Generating new wallet...</source>
        <translation>Génération du nouveau portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="937"/>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-keys=&quot;wallet_name&quot;</source>
        <translation>impossible de spécifier plus d&apos;une option parmis --generate-new-wallet=&quot;nom_du_portefeuille&quot;, --wallet-file=&quot;nom_du_portefeuille&quot;, --generate-from-view-key=&quot;nom_du_portefeuille&quot;, --generate-from-json=&quot;nom_du_fichier_json&quot; et --generate-from-keys=&quot;nom_du_portefeuille&quot;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="953"/>
        <source>can&apos;t specify both --restore-deterministic-wallet and --non-deterministic</source>
        <translation>impossible de spécifier à la fois --restore-deterministic-wallet et --non-deterministic</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="982"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="994"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1011"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1046"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1063"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1079"/>
        <source>No data supplied, cancelled</source>
        <translation>Pas de données fournies, annulation</translation>
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
        <translation>échec de l&apos;analyse de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1017"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1085"/>
        <source>failed to parse view key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1027"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1103"/>
        <source>failed to verify view key secret key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1031"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1107"/>
        <source>view key does not match standard address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1036"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1111"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1128"/>
        <source>account creation failed</source>
        <translation>échec de la création du compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1069"/>
        <source>failed to parse spend key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1095"/>
        <source>failed to verify spend key secret key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1099"/>
        <source>spend key does not match standard address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1199"/>
        <source>failed to open account</source>
        <translation>échec de l&apos;ouverture du compte</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1203"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1579"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1626"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1647"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3348"/>
        <source>wallet is null</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1262"/>
        <source>Daemon either is not started or wrong port was passed. Please make sure daemon is running or restart the wallet with the correct daemon address.</source>
        <translation>Le démon n&apos;est pas démarré ou un mauvais port a été passé. Veuillez vous assurer que le démon fonctionne ou redémarrez le portefeuille avec l&apos;adresse de démon correcte.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1306"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1311"/>
        <source>invalid language choice passed. Please try again.
</source>
        <translation>choix de langue passé invalide. Veuillez réessayer.
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1370"/>
        <source>View key: </source>
        <translation>Clé d&apos;audit : </translation>
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
        <translation>Votre portefeuille a été généré !
Pour commencer la synchronisation avec le démon, utilisez la commande &quot;refresh&quot;.
Utilisez la commande &quot;help&quot; pour voir la liste des commandes disponibles.
Utilisez toujours la commande &quot;exit&quot; pour fermer monero-wallet-cli afin de sauvegarder
l&apos;état actuel de votre session. Sinon vous pourriez avoir besoin de synchroniser
votre portefeuille à nouveau (mais les clés de votre portefeuille ne risquent rien).
</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1492"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation>Vous pourriez vouloir supprimer le fichier &quot;%s&quot; et réessayer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1518"/>
        <source>failed to deinitialize wallet</source>
        <translation>échec de la désinitialisation du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1570"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1968"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation>cette commande requiert un démon de confiance. Activer avec --trusted-daemon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1657"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation>la chaîne de blocs ne peut pas être sauvegardée : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1736"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1982"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2386"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2563"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2824"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1740"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1986"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2390"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2567"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2828"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1750"/>
        <source>refresh error: </source>
        <translation>erreur du rafraîchissement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1794"/>
        <source>Balance: </source>
        <translation>Solde : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1845"/>
        <source>pubkey</source>
        <translation>clé publique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1845"/>
        <source>key image</source>
        <translation>image de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1856"/>
        <source>unlocked</source>
        <translation>déverrouillé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1846"/>
        <source>ringct</source>
        <translation>ringct</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1855"/>
        <source>T</source>
        <translation>V</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1855"/>
        <source>F</source>
        <translation>F</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1856"/>
        <source>locked</source>
        <translation>vérrouillé</translation>
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
        <translation>format d&apos;identifiant de paiement invalide, 16 ou 64 caractères hexadécimaux attendus : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1990"/>
        <source>failed to get spent status</source>
        <translation>échec de la récupération du statut de dépense</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2105"/>
        <source>the same transaction</source>
        <translation>la même transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2105"/>
        <source>blocks that are temporally very close</source>
        <translation>blocs très proches dans le temps</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2206"/>
        <source>Locked blocks too high, max 1000000 (˜4 yrs)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3258"/>
        <source>usage: get_tx_proof &lt;txid&gt; &lt;dest_address&gt; [&lt;tx_key&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3289"/>
        <source>failed to parse tx_key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3298"/>
        <source>Tx secret key was found for the given txid, but you&apos;ve also provided another tx secret key which doesn&apos;t match the found one.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3306"/>
        <source>Tx secret key wasn&apos;t found in the wallet file. Provide it as the optional third parameter if you have it elsewhere.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3330"/>
        <source>Signature: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3508"/>
        <source>usage: check_tx_proof &lt;txid&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3539"/>
        <source>Signature header check error</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3550"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3555"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3560"/>
        <source>Signature decoding error</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3602"/>
        <source>Tx pubkey was not found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3609"/>
        <source>Good signature</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3613"/>
        <source>Bad signature</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3621"/>
        <source>failed to generate key derivation</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3994"/>
        <source>usage: integrated_address [payment ID]</source>
        <translation>usage : integrated_address [ID paiement]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4017"/>
        <source>Integrated address: account %s, payment ID %s</source>
        <translation>Adresse intégrée : compte %s, ID de paiement %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4022"/>
        <source>Standard address: </source>
        <translation>Adresse standard : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4027"/>
        <source>failed to parse payment ID or address</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement ou de l&apos;adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4038"/>
        <source>usage: address_book [(add (&lt;address&gt; [pid &lt;long or short payment id&gt;])|&lt;integrated address&gt; [&lt;description possibly with whitespaces&gt;])|(delete &lt;index&gt;)]</source>
        <translation>usage : address_book [(add (&lt;adresse&gt; [pid &lt;ID de paiement long ou court&gt;])|&lt;adresse integrée&gt; [&lt;description avec des espaces possible&gt;])|(delete &lt;index&gt;)]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4070"/>
        <source>failed to parse payment ID</source>
        <translation>échec de l&apos;analyse de l&apos;ID de paiement</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4088"/>
        <source>failed to parse index</source>
        <translation>échec de l&apos;analyse de l&apos;index</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4096"/>
        <source>Address book is empty.</source>
        <translation>Le carnet d&apos;adresses est vide.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4102"/>
        <source>Index: </source>
        <translation>Index : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4103"/>
        <source>Address: </source>
        <translation>Adresse : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4104"/>
        <source>Payment ID: </source>
        <translation>ID de paiement : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4105"/>
        <source>Description: </source>
        <translation>Description : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4115"/>
        <source>usage: set_tx_note [txid] free text note</source>
        <translation>usage : set_tx_note [ID transaction] note de texte libre</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4143"/>
        <source>usage: get_tx_note [txid]</source>
        <translation>usage : get_tx_note [ID transaction]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4193"/>
        <source>usage: sign &lt;filename&gt;</source>
        <translation>usage : sign &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4198"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas signer</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4207"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4230"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4374"/>
        <source>failed to read file </source>
        <translation>échec de la lecture du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4219"/>
        <source>usage: verify &lt;filename&gt; &lt;address&gt; &lt;signature&gt;</source>
        <translation>usage : verify &lt;fichier&gt; &lt;adresse&gt; &lt;signature&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4246"/>
        <source>Bad signature from </source>
        <translation>Mauvaise signature de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4250"/>
        <source>Good signature from </source>
        <translation>Bonne signature de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4259"/>
        <source>usage: export_key_images &lt;filename&gt;</source>
        <translation>usage : export_key_images &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4264"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation>c&apos;est un portefeuille d&apos;audit et il ne peut pas exporter les images de clé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4274"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4346"/>
        <source>failed to save file </source>
        <translation>échec de l&apos;enregistrement du fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4285"/>
        <source>Signed key images exported to </source>
        <translation>Images de clé signées exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4293"/>
        <source>usage: import_key_images &lt;filename&gt;</source>
        <translation>usage : import_key_images &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4323"/>
        <source>usage: export_outputs &lt;filename&gt;</source>
        <translation>usage : export_outputs &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4357"/>
        <source>Outputs exported to </source>
        <translation>Sorties exportées vers </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4365"/>
        <source>usage: import_outputs &lt;filename&gt;</source>
        <translation>usage : import_outputs &lt;fichier&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2246"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3818"/>
        <source>amount is wrong: </source>
        <translation>montant erroné : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2247"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3819"/>
        <source>expected number from 0 to </source>
        <translation>attend un nombre de 0 à </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2378"/>
        <source>Money successfully sent, transaction </source>
        <translation>Fonds envoyés avec succès, transaction </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3141"/>
        <source>no connection to daemon. Please, make sure daemon is running.</source>
        <translation>pas de connexion au démon. Veuillez vous assurer que le démon fonctionne.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2420"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2597"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2858"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3171"/>
        <source>not enough outputs for specified mixin_count</source>
        <translation>pas assez de sorties pour le mixin spécifié</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2423"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2600"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2861"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3174"/>
        <source>output amount</source>
        <translation>montant de la sortie</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2423"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2600"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2861"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3174"/>
        <source>found outputs to mix</source>
        <translation>sorties à mélanger trouvées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2428"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2605"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2866"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3179"/>
        <source>transaction was not constructed</source>
        <translation>la transaction n&apos;a pas été construite</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2432"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2609"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2870"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3183"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>la transaction %s a été rejetée par le démon avec le statut : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2443"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2620"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2881"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3191"/>
        <source>one of destinations is zero</source>
        <translation>une des destinations est zéro</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3195"/>
        <source>Failed to find a suitable way to split transactions</source>
        <translation>Échec de la recherche d&apos;une façon adéquate de scinder les transactions</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2452"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2629"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2890"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3200"/>
        <source>unknown transfer error: </source>
        <translation>erreur de transfert inconnue : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2516"/>
        <source>Sweeping </source>
        <translation>Balayage de </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2785"/>
        <source>Sweeping %s for a total fee of %s.  Is this okay?  (Y/Yes/N/No)</source>
        <translation>Balayage de %s pour des frais totaux de %s. Est-ce correct ? (Y/Yes/Oui/N/No/Non)</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2555"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2816"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3129"/>
        <source>Money successfully sent, transaction: </source>
        <translation>Fonds envoyés avec succès, transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3022"/>
        <source>Change goes to more than one address</source>
        <translation>La monnaie rendue va à plus d&apos;une adresse</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3047"/>
        <source>%s change to %s</source>
        <translation>%s de monnaie rendue à %s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3050"/>
        <source>no change</source>
        <translation>sans monnaie rendue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3105"/>
        <source>Transaction successfully signed to file </source>
        <translation>Transaction signée avec succès dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3226"/>
        <source>usage: get_tx_key &lt;txid&gt;</source>
        <translation>usage : get_tx_key &lt;ID transaction&gt;</translation>
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
        <translation>échec de l&apos;analyse de l&apos;ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3245"/>
        <source>Tx key: </source>
        <translation>Clé de transaction : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3250"/>
        <source>no tx keys found for this txid</source>
        <translation>aucune clé de transaction trouvée pour cet ID de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3339"/>
        <source>usage: check_tx_key &lt;txid&gt; &lt;txkey&gt; &lt;address&gt;</source>
        <translation>usage : check_tx_key &lt;ID transaction&gt; &lt;clé transaction&gt; &lt;adresse&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3361"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3368"/>
        <source>failed to parse tx key</source>
        <translation>échec de l&apos;analyse de la clé de transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3400"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3573"/>
        <source>failed to get transaction from daemon</source>
        <translation>échec de la récupération de la transaction du démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3411"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3584"/>
        <source>failed to parse transaction from daemon</source>
        <translation>échec de l&apos;analyse de la transaction du démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3418"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3591"/>
        <source>failed to validate transaction from daemon</source>
        <translation>échec de la validation de la transaction du démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3423"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3596"/>
        <source>failed to get the right transaction from daemon</source>
        <translation>échec de la récupération de la bonne transaction du démon</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3385"/>
        <source>failed to generate key derivation from supplied parameters</source>
        <translation>échec de la génération de la dérivation de clé à partir des paramètres fournis</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3471"/>
        <source>error: </source>
        <translation>erreur : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3477"/>
        <source>received</source>
        <translation>a reçu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3477"/>
        <source>in txid</source>
        <translation>dans la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3481"/>
        <source>received nothing in txid</source>
        <translation>n&apos;a rien reçu dans la transaction</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3485"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation>ATTENTION : cette transaction n&apos;est pas encore inclue dans la chaîne de blocs !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3494"/>
        <source>This transaction has %u confirmations</source>
        <translation>Cette transaction a %u confirmations</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3498"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation>ATTENTION : échec de la détermination du nombre de confirmations !</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3661"/>
        <source>usage: show_transfers [in|out|all|pending|failed] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation>usage : show_transfers [in|out|all|pending|failed] [&lt;hauteur_minimum&gt; [&lt;hauteur_maximum&gt;]]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3700"/>
        <source>bad min_height parameter:</source>
        <translation>mauvais paramètre hauteur_minimum :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3712"/>
        <source>bad max_height parameter:</source>
        <translation>mauvais paramètre hauteur_maximum :</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3760"/>
        <source>in</source>
        <translation>reçu</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3760"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3798"/>
        <source>out</source>
        <translation>payé</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3798"/>
        <source>failed</source>
        <translation>échoué</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3798"/>
        <source>pending</source>
        <translation>en attente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3809"/>
        <source>usage: unspent_outputs [&lt;min_amount&gt; &lt;max_amount&gt;]</source>
        <translation>usage : unspent_outputs [&lt;montant_minimum&gt; &lt;montant_maximum&gt;]</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3824"/>
        <source>&lt;min_amount&gt; should be smaller than &lt;max_amount&gt;</source>
        <translation>&lt;montant_minimum&gt; doit être inférieur à &lt;montant_maximum&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3856"/>
        <source>
Amount: </source>
        <translation>
Montant : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3856"/>
        <source>, number of keys: </source>
        <translation>, nombre de clés : </translation>
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
        <translation>
Hauteur de bloc minimum : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3867"/>
        <source>
Max block height: </source>
        <translation>
Hauteur de bloc maximum : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3868"/>
        <source>
Min amount found: </source>
        <translation>
Montant minimum trouvé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3869"/>
        <source>
Max amount found: </source>
        <translation>
Montant maximum trouvé : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3870"/>
        <source>
Total count: </source>
        <translation>
Compte total : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3910"/>
        <source>
Bin size: </source>
        <translation>
Taille de classe : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3911"/>
        <source>
Outputs per *: </source>
        <translation>
Sorties par * : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3913"/>
        <source>count
  ^
</source>
        <translation>compte
  ^
</translation>
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
        <translation>+--&gt; hauteur de bloc
</translation>
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
        <translation>portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="420"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4000"/>
        <source>Random payment ID: </source>
        <translation>ID de paiement aléatoire : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4001"/>
        <source>Matching integrated address: </source>
        <translation>Adresse intégrée correspondante : </translation>
    </message>
</context>
<context>
    <name>sw</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="116"/>
        <source>Generate new wallet and save it to &lt;arg&gt;</source>
        <translation>Générer un nouveau portefeuille et le sauvegarder dans &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="117"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>Générer un portefeuille d&apos;audit à partir d&apos;une clé d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="118"/>
        <source>Generate wallet from private keys</source>
        <translation>Générer un portefeuille à partir de clés privées</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="120"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>Spécifier la graine Electrum pour la récupération/création d&apos;un portefeuille</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="121"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Récupérer un portefeuille en utilisant une graine mnémonique de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="122"/>
        <source>Create non-deterministic view and spend keys</source>
        <translation>Créer des clés d&apos;audit et de dépense non déterministes</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="123"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation>Activer les commandes qui dépendent d&apos;un démon de confiance</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="124"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>Autoriser la communication avec un démon utilisant une version de RPC différente</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="125"/>
        <source>Restore from specific blockchain height</source>
        <translation>Restaurer à partir d&apos;une hauteur de bloc spécifique</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="136"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>le démon est occupé. Veuillez réessayer plus tard.</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="145"/>
        <source>possibly lost connection to daemon</source>
        <translation>connexion avec le démon peut-être perdue</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="226"/>
        <source>Error: </source>
        <translation>Erreur : </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4614"/>
        <source>Failed to initialize wallet</source>
        <translation>Échec de l&apos;initialisation du portefeuille</translation>
    </message>
</context>
<context>
    <name>tools::dns_utils</name>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="430"/>
        <source>DNSSEC validation passed</source>
        <translation>validation DNSSEC réussie</translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="434"/>
        <source>WARNING: DNSSEC validation was unsuccessful, this address may not be correct!</source>
        <translation>ATTENTION : validation DNSSEC échouée, cette adresse pourrait ne pas être correcte !</translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="437"/>
        <source>For URL: </source>
        <translation>Pour l&apos;URL : </translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="439"/>
        <source> Monero Address = </source>
        <translation> Adresse Monero = </translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="441"/>
        <source>Is this OK? (Y/n) </source>
        <translation>Est-ce correct ? (Y/n) </translation>
    </message>
    <message>
        <location filename="../src/common/dns_utils.cpp" line="451"/>
        <source>you have cancelled the transfer request</source>
        <translation>vous avez annulé la requête de transfert</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="106"/>
        <source>Use daemon instance at &lt;host&gt;:&lt;port&gt;</source>
        <translation>Utiliser l&apos;instance de démon située à &lt;hôte&gt;:&lt;port&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="107"/>
        <source>Use daemon instance at host &lt;arg&gt; instead of localhost</source>
        <translation>Utiliser l&apos;instance de démon située à l&apos;hôte &lt;arg&gt; au lieu de localhost</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="460"/>
        <source>Wallet password</source>
        <translation>Mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="109"/>
        <source>Wallet password file</source>
        <translation>Fichier mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="110"/>
        <source>Use daemon instance at port &lt;arg&gt; instead of 18081</source>
        <translation>Utiliser l&apos;instance de démon située au port &lt;arg&gt; au lieu de 18081</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="112"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>Pour testnet, le démon doit aussi être lancé avec l&apos;option --testnet</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="113"/>
        <source>Restricts to view-only commands</source>
        <translation>Restreindre aux commandes en lecture-seule</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="152"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>impossible de spécifier l&apos;hôte ou le port du démon plus d&apos;une fois</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="188"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>impossible de spécifier plus d&apos;une option parmis --password et --password-file</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="204"/>
        <source>the password file specified could not be read</source>
        <translation>le fichier mot de passe spécifié n&apos;a pas pu être lu</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="460"/>
        <source>Enter new wallet password</source>
        <translation>Entrez le mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="464"/>
        <source>failed to read wallet password</source>
        <translation>échec de la lecture du mot de passe du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="227"/>
        <source>Failed to load file </source>
        <translation>Échec du chargement du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="108"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation>Mot de passe du portefeuille (échapper/citer si nécessaire)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="111"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>Spécifier le nom_utilisateur:[mot_de_passe] pour le client RPC du démon</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="233"/>
        <source>Failed to parse JSON</source>
        <translation>Échec de l&apos;analyse JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="240"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation>Version %u trop récente, on comprend au mieux %u</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="258"/>
        <source>failed to parse view key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="264"/>
        <location filename="../src/wallet/wallet2.cpp" line="331"/>
        <location filename="../src/wallet/wallet2.cpp" line="373"/>
        <source>failed to verify view key secret key</source>
        <translation>échec de la vérification de la clé secrète d&apos;audit</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="276"/>
        <source>failed to parse spend key secret key</source>
        <translation>échec de l&apos;analyse de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="282"/>
        <location filename="../src/wallet/wallet2.cpp" line="343"/>
        <location filename="../src/wallet/wallet2.cpp" line="394"/>
        <source>failed to verify spend key secret key</source>
        <translation>échec de la vérification de la clé secrète de dépense</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="295"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Échec de la vérification de la liste de mots de style Electrum</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="306"/>
        <source>At least one of Electrum-style word list and private view key must be specified</source>
        <translation>Au moins une des listes de mots de style Electrum ou clé privée d&apos;audit doit être spécifiée</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="311"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Liste de mots de style Electrum et clé privée spécifiées en même temps</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="324"/>
        <source>invalid address</source>
        <translation>adresse invalide</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="335"/>
        <source>view key does not match standard address</source>
        <translation>la clé d&apos;audit ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="347"/>
        <source>spend key does not match standard address</source>
        <translation>la clé de dépense ne correspond pas à l&apos;adresse standard</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="356"/>
        <source>Cannot create deprecated wallets from JSON</source>
        <translation>Impossible de créer un portefeuille obsolète à partir de JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="403"/>
        <source>failed to generate new wallet: </source>
        <translation>échec de la génération du nouveau portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="5205"/>
        <source>failed to read file </source>
        <translation>échec de la lecture du fichier </translation>
    </message>
</context>
<context>
    <name>tools::wallet_rpc_server</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="151"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>Le démon est local, supposons qu&apos;il est de confiance</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="171"/>
        <source>Cannot specify --</source>
        <translation>Impossible de spécifier --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="171"/>
        <source> and --</source>
        <translation> et --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="198"/>
        <source>Failed to create file </source>
        <translation>Échec de la création du fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="198"/>
        <source>. Check permissions or remove file</source>
        <translation>. Vérifiez les permissions ou supprimez le fichier</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="209"/>
        <source>Error writing to file </source>
        <translation>Erreur d&apos;écriture dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="212"/>
        <source>RPC username/password is stored in file </source>
        <translation>nom_utilisateur/mot_de_passe RPC sauvegardé dans le fichier </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1748"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>Impossible de spécifier plus d&apos;une option parmis --wallet-file et --generate-from-json</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1760"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>--wallet-file, --generate-from-json ou --wallet-dir doit être spécifié</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1764"/>
        <source>Loading wallet...</source>
        <translation>Chargement du portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1789"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1814"/>
        <source>Storing wallet...</source>
        <translation>Sauvegarde du portefeuille...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1791"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1816"/>
        <source>Stored ok</source>
        <translation>Sauvegarde OK</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1794"/>
        <source>Loaded ok</source>
        <translation>Chargement OK</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1798"/>
        <source>Wallet initialization failed: </source>
        <translation>Échec de l&apos;initialisation du portefeuille : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1805"/>
        <source>Failed to initialize wallet rpc server</source>
        <translation>Échec de l&apos;initialisation du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1809"/>
        <source>Starting wallet rpc server</source>
        <translation>Démarrage du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1811"/>
        <source>Stopped wallet rpc server</source>
        <translation>Arrêt du serveur RPC du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1820"/>
        <source>Failed to store wallet: </source>
        <translation>Échec de la sauvegarde du portefeuille : </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="1715"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4580"/>
        <source>Wallet options</source>
        <translation>Options du portefeuille</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="59"/>
        <source>Generate wallet from JSON format file</source>
        <translation>Générer un portefeuille à partir d&apos;un fichier au format JSON</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="63"/>
        <source>Use wallet &lt;arg&gt;</source>
        <translation>Utiliser le portefeuille &lt;arg&gt;</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="87"/>
        <source>Max number of threads to use for a parallel job</source>
        <translation>Nombre maximum de threads à utiliser pour les travaux en parallèle</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="88"/>
        <source>Specify log file</source>
        <translation>Spécifier le fichier journal</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="89"/>
        <source>Config file</source>
        <translation>Fichier de configuration</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="98"/>
        <source>General options</source>
        <translation>Options générales</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="128"/>
        <source>Can&apos;t find config file </source>
        <translation>Impossible de trouver le fichier de configuration </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="172"/>
        <source>Logging to: </source>
        <translation>Journalisation dans : </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="173"/>
        <source>Logging to %s</source>
        <translation>Journalisation dans %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="153"/>
        <source>Usage:</source>
        <translation>Usage :</translation>
    </message>
</context>
</TS>
