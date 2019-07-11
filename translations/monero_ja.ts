<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ja" sourcelanguage="en">
<context>
    <name>Monero::AddressBookImpl</name>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="53"/>
        <source>Invalid destination address</source>
        <translation>不正な宛先アドレス</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="63"/>
        <source>Invalid payment ID. Short payment ID should only be used in an integrated address</source>
        <translation>不正なペイメントIDありっます。インテグレーテットアドレスにだけ短いペイメントIDを使えます</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="70"/>
        <source>Invalid payment ID</source>
        <translation>不正なペイメントID</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/address_book.cpp" line="77"/>
        <source>Integrated address and long payment ID can&apos;t be used at the same time</source>
        <translation>同じ時にインテグレーテットアドレスと長いペイメントIDを使えません</translation>
    </message>
</context>
<context>
    <name>Monero::PendingTransactionImpl</name>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="91"/>
        <source>Attempting to save transaction to file, but specified file(s) exist. Exiting to not risk overwriting. File:</source>
        <translation>ファイルは既に存在するのでファイルに取引を書き出せなかった。上書きしないにエグジットしてます。ファイル：</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="98"/>
        <source>Failed to write transaction(s) to file</source>
        <translation>取引をファイルに書き込めませんでした</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="138"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>デーモンは忙しいです。後でもう一度試してください。</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="141"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>デーモンの接続が確立ありません。デーモンが実行中になっていることを確認してください。</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="145"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>取引 %s がデーモンによって拒否しました。ステータス： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="150"/>
        <source>. Reason: </source>
        <translation>。 理由： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="152"/>
        <source>Unknown exception: </source>
        <translation>未知の例外： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="155"/>
        <source>Unhandled exception</source>
        <translation>未処理の例外</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="228"/>
        <source>Couldn&apos;t multisig sign data: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/pending_transaction.cpp" line="250"/>
        <source>Couldn&apos;t sign multisig transaction: </source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Monero::UnsignedTransactionImpl</name>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="75"/>
        <source>This is a watch only wallet</source>
        <translation>これは閲覧専用ウォレットです</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="85"/>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="92"/>
        <source>Failed to sign transaction</source>
        <translation>取引を署名できませんでした</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="168"/>
        <source>Claimed change does not go to a paid address</source>
        <translation>請求したお釣りはもうお金に送ったアドレス送りません</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="174"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation>請求したお釣りはお釣りのアドレスに送ったペイメントより大きいです</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="184"/>
        <source>Change goes to more than one address</source>
        <translation>お釣りは複数のアドレスに送ります</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="197"/>
        <source>sending %s to %s</source>
        <translation>%s を %s に送ってます</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="203"/>
        <source>with no destinations</source>
        <translation>目的地なし</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="209"/>
        <source>%s change to %s</source>
        <translation>%s のお釣り %s に</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="212"/>
        <source>no change</source>
        <translation>お釣りありません</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/unsigned_transaction.cpp" line="214"/>
        <source>Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu. %s</source>
        <translation>取引は %lu ロードした、 %s に、%s のの手数料、 %s 、 %s 、最小リングサイズ %lu 。%s</translation>
    </message>
</context>
<context>
    <name>Monero::WalletImpl</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1459"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation>ペイメントIDのフォーマットは不正です。16文字または64文字の16進数の文字列が必要で： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1468"/>
        <source>Failed to add short payment id: </source>
        <translation>短いペイメントIDの追加に失敗しました： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1510"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1592"/>
        <source>daemon is busy. Please try again later.</source>
        <translation>デーモンは忙しいです。後でもう一度試してください。</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1512"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1594"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>デーモンの接続が確立ありません。デーモンが実行中になっていることを確認してください。</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1514"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1596"/>
        <source>RPC error: </source>
        <translation>RPCエラー： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1542"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1627"/>
        <source>not enough outputs for specified ring size</source>
        <translation>指定したリングサイズのアウトプットが不十分です</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1544"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1629"/>
        <source>found outputs to use</source>
        <translation>使うためにアウトプットを見つかれました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1546"/>
        <source>Please sweep unmixable outputs.</source>
        <translation>ミックス不能なアウトプットをスイープしてください。</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1520"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1603"/>
        <source>not enough money to transfer, available only %s, sent amount %s</source>
        <translation>振替でMoneroを受け取ることできません。利用可能な金額： %s,  取引の金額： %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="589"/>
        <source>failed to parse address</source>
        <translation>アドレスの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="600"/>
        <source>failed to parse secret spend key</source>
        <translation>秘密なスペンドキーの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="623"/>
        <source>failed to parse secret view key</source>
        <translation>秘密なビューキーの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="632"/>
        <source>failed to verify secret spend key</source>
        <translation>秘密なスペンドキーの検証に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="636"/>
        <source>spend key does not match address</source>
        <translation>スペンドキーがアドレスと一致しませんでした</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="642"/>
        <source>failed to verify secret view key</source>
        <translation>秘密なビューキーの検証に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="646"/>
        <source>view key does not match address</source>
        <translation>ビューキーがアドレスと一致しませんでした</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="669"/>
        <location filename="../src/wallet/api/wallet.cpp" line="686"/>
        <source>failed to generate new wallet: </source>
        <translation>新しいウォレットの生成に失敗しました： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="955"/>
        <source>Failed to send import wallet request</source>
        <translation>インポートウォレットリクエストの送信に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1125"/>
        <source>Failed to load unsigned transactions</source>
        <translation>未署名の取引を読み込めませんでした</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1144"/>
        <source>Failed to load transaction from file</source>
        <translation>ファイルからの取引のロードに失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1160"/>
        <source>Wallet is view only</source>
        <translation>閲覧専用ウォレットです</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1168"/>
        <source>failed to save file </source>
        <translation>ファイルを保存できませんでした </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1184"/>
        <source>Key images can only be imported with a trusted daemon</source>
        <translation>信頼できるデーモンしかでキーイメージをインポートしません</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1197"/>
        <source>Failed to import key images: </source>
        <translation>キーイメージをインポートできませんでした： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1229"/>
        <source>Failed to get subaddress label: </source>
        <translation>サブアドレスラベルを取得できませんでした： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1242"/>
        <source>Failed to set subaddress label: </source>
        <translation>サブアドレスラベルをセットできませんでした： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="615"/>
        <source>Neither view key nor spend key supplied, cancelled</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="734"/>
        <source>Electrum seed is empty</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="743"/>
        <source>Electrum-style word list failed verification</source>
        <translation type="unfinished">Electrumな単語表の検証に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1259"/>
        <source>Failed to get multisig info: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1276"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1290"/>
        <source>Failed to make multisig: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1305"/>
        <source>Failed to finalize multisig wallet creation</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1308"/>
        <source>Failed to finalize multisig wallet creation: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1324"/>
        <source>Failed to export multisig images: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1342"/>
        <source>Failed to parse imported multisig images</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1352"/>
        <source>Failed to import multisig images: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1366"/>
        <source>Failed to check for partial multisig key images: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1394"/>
        <source>Failed to restore multisig transaction: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1434"/>
        <source>Invalid destination address</source>
        <translation type="unfinished">不正な宛先アドレス</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1516"/>
        <source>failed to get outputs to mix: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1527"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1611"/>
        <source>not enough money to transfer, overall balance only %s, sent amount %s</source>
        <translation>振替でMoneroを受け取ることできません。利用可能な金額： %s,  取引の金額： %s</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1534"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1619"/>
        <source>not enough money to transfer, available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>取引は無理です。利用可能な金額 %s、 取引の金額 %s = %s + %s (手数料)</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1544"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1629"/>
        <source>output amount</source>
        <translation>アウトプットの金額</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1549"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1633"/>
        <source>transaction was not constructed</source>
        <translation>取引を作りませんでした</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1552"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1636"/>
        <source>transaction %s was rejected by daemon with status: </source>
        <translation>取引 %s がデーモンによって拒否しました。ステータス： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1557"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1641"/>
        <source>one of destinations is zero</source>
        <translation>宛先の1つはゼロです</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1559"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1643"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation>取引を分割する適切な方法を見つけることができませんでした</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1561"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1645"/>
        <source>unknown transfer error: </source>
        <translation>不明な転送エラー： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1563"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1647"/>
        <source>internal error: </source>
        <translation>内部エラー： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1565"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1649"/>
        <source>unexpected error: </source>
        <translation>予期せぬエラー： </translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1567"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1651"/>
        <source>unknown error</source>
        <translation>不明なエラー</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1598"/>
        <source>failed to get outputs to mix</source>
        <translation type="unfinished"></translation>
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
        <translation>txidの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1743"/>
        <source>no tx keys found for this txid</source>
        <translation>このtxidのためにtxキーを見つかれませんでした</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1761"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1770"/>
        <source>Failed to parse tx key</source>
        <translation>txキーの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1779"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1808"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1836"/>
        <location filename="../src/wallet/api/wallet.cpp" line="1917"/>
        <source>Failed to parse address</source>
        <translation>アドレスの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1922"/>
        <source>Address must not be a subaddress</source>
        <translation>アドレスはサブアドレスであってはならないです</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1962"/>
        <source>The wallet must be in multisig ready state</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="1984"/>
        <source>Given string is not a key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2226"/>
        <source>Rescan spent can only be used with a trusted daemon</source>
        <translation>信頼できるデーモンしかで再スキャンしません</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2275"/>
        <source>Invalid output: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2282"/>
        <source>Failed to mark outputs as spent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2293"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2315"/>
        <source>Failed to parse output amount</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2298"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2320"/>
        <source>Failed to parse output offset</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2304"/>
        <source>Failed to mark output as spent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2326"/>
        <source>Failed to mark output as unspent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2337"/>
        <location filename="../src/wallet/api/wallet.cpp" line="2376"/>
        <source>Failed to parse key image</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2343"/>
        <source>Failed to get ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2361"/>
        <source>Failed to get rings</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="2382"/>
        <source>Failed to set ring</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>Wallet</name>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="344"/>
        <source>Failed to parse address</source>
        <translation>アドレスの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="351"/>
        <source>Failed to parse key</source>
        <translation>キーの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="359"/>
        <source>failed to verify key</source>
        <translation>キーの検証に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/api/wallet.cpp" line="369"/>
        <source>key does not match address</source>
        <translation>キーがアドレスと一致しませんでした</translation>
    </message>
</context>
<context>
    <name>command_line</name>
    <message>
        <location filename="../src/common/command_line.cpp" line="54"/>
        <source>yes</source>
        <translation>はい</translation>
    </message>
    <message>
        <location filename="../src/common/command_line.cpp" line="68"/>
        <source>no</source>
        <translation>いいえ</translation>
    </message>
</context>
<context>
    <name>cryptonote::rpc_args</name>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="40"/>
        <source>Specify IP to bind RPC server</source>
        <translation>RPCサーバに接続するIPを指定してください</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="41"/>
        <source>Specify username[:password] required for RPC server</source>
        <translation>RPCサーバを使うためにユーザー名[：パスワード]を指定してください</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="42"/>
        <source>Confirm rpc-bind-ip value is NOT a loopback (local) IP</source>
        <translation>rpc-bind-ipの価はループバック(ローカル)IPじゃないことを確認してください</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="43"/>
        <source>Specify a comma separated list of origins to allow cross origin resource sharing</source>
        <translation>クロスオリジンリソース共同をできるためにコンマ区切りリストを指定してください</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="70"/>
        <source>Invalid IP address given for --</source>
        <translation>このRPCサーバーのIPはだめです --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="78"/>
        <source> permits inbound unencrypted external connections. Consider SSH tunnel or SSL proxy instead. Override with --</source>
        <translation> は暗号化されていない外部接続をできますがSSHトンネルやSSLプロキシの方がいいです。これでオーバーライド --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="101"/>
        <source>Username specified with --</source>
        <translation>このRPCサーバのユーザー名につて --</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="101"/>
        <location filename="../src/rpc/rpc_args.cpp" line="111"/>
        <source> cannot be empty</source>
        <translation> 入力する必要があります</translation>
    </message>
    <message>
        <location filename="../src/rpc/rpc_args.cpp" line="111"/>
        <source> requires RPC server password --</source>
        <translation> のRPCサーバのパスワードありません --</translation>
    </message>
</context>
<context>
    <name>cryptonote::simple_wallet</name>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="666"/>
        <source>Commands: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4636"/>
        <source>failed to read wallet password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4231"/>
        <source>invalid password</source>
        <translation>不正なパスワード</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3283"/>
        <source>set seed: needs an argument. available options: language</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3319"/>
        <source>set: unrecognized argument(s)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4476"/>
        <source>wallet file path not valid: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3389"/>
        <source>Attempting to generate or restore wallet, but specified file(s) exist.  Exiting to not risk overwriting.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3269"/>
        <source>needs an argument</source>
        <translation type="unfinished"></translation>
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
        <translation>０や１</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3302"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3306"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3313"/>
        <source>unsigned integer</source>
        <translation>符号無しの整数</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3548"/>
        <source>--restore-deterministic-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3577"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;words list here&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3957"/>
        <source>specify a wallet path with --generate-new-wallet (not --wallet-file)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4164"/>
        <source>wallet failed to connect to daemon: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4172"/>
        <source>Daemon uses a different RPC major version (%u) than the wallet (%u): %s. Either update one of them, or use --allow-mismatched-daemon-version.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4193"/>
        <source>List of available languages for your wallet&apos;s seed:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4277"/>
        <source>You had been using a deprecated version of the wallet. Please use the new seed that we provide.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4293"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4365"/>
        <source>Generated new wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4302"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4370"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4412"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4465"/>
        <source>failed to generate new wallet: </source>
        <translation>新しいウォレットの生成に失敗しました： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4507"/>
        <source>Opened watch-only wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4511"/>
        <source>Opened wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4529"/>
        <source>You had been using a deprecated version of the wallet. Please proceed to upgrade your wallet.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4544"/>
        <source>You had been using a deprecated version of the wallet. Your wallet file format is being upgraded now.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4552"/>
        <source>failed to load wallet: </source>
        <translation>ウォレットをロードできませんでした： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4569"/>
        <source>Use the &quot;help&quot; command to see the list of available commands.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4614"/>
        <source>Wallet data saved</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4819"/>
        <source>Mining started in daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4821"/>
        <source>mining has NOT been started: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4841"/>
        <source>Mining stopped in daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4843"/>
        <source>mining has NOT been stopped: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4925"/>
        <source>Blockchain saved</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4940"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4973"/>
        <source>Height </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4975"/>
        <source>spent </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5089"/>
        <source>Starting refresh...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5115"/>
        <source>Refresh done, blocks received: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6349"/>
        <source>payment id has invalid format, expected 16 or 64 character hex string: </source>
        <translation type="unfinished">ペイメントIDのフォーマットは不正です。16文字または64文字の16進数の文字列が必要で： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5704"/>
        <source>bad locked_blocks parameter:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6369"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6639"/>
        <source>a single transaction cannot use more than one payment id: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5806"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6378"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6607"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6647"/>
        <source>failed to set up payment id, though it was decoded correctly</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5659"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6271"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6564"/>
        <source>ring size %u is too large, maximum is %u</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5690"/>
        <source>payment id failed to encode</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5737"/>
        <source>failed to parse short payment ID from URI</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5762"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5764"/>
        <source>Invalid last argument: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5782"/>
        <source>a single transaction cannot use more than one payment id</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5800"/>
        <source>failed to parse payment id, though it was detected</source>
        <translation type="unfinished"></translation>
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
        <translation>
取引 </translation>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="5942"/>
        <source>Sending %s.  </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5945"/>
        <source>Your transaction needs to be split into %llu transactions.  This will result in a transaction fee being applied to each transaction, for a total fee of %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5951"/>
        <source>The transaction fee is %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5954"/>
        <source>, of which %s is dust from change</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5955"/>
        <source>.</source>
        <translation>。</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5955"/>
        <source>A total of %s from dust change will be sent to dust address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5960"/>
        <source>.
This transaction will unlock on block %llu, in approximately %s days (assuming 2 minutes per block)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6004"/>
        <source>Unsigned transaction(s) successfully written to MMS</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished">取引をファイルに書き込めませんでした</translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6026"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6477"/>
        <source>Failed to cold sign transaction with HW wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6109"/>
        <source>No unmixable outputs found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6176"/>
        <source>Not enough money in unlocked balance</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6216"/>
        <source>No address given</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6280"/>
        <source>missing lockedblocks parameter</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6290"/>
        <source>bad locked_blocks parameter</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6315"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6573"/>
        <source>Failed to parse number of outputs</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6320"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6578"/>
        <source>Amount of outputs should be greater than 0</source>
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
        <translation>キーイメージの解析に失敗しました</translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6770"/>
        <source>invalid amount threshold</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6919"/>
        <source>Claimed change does not go to a paid address</source>
        <translation type="unfinished">請求したお釣りはもうお金に送ったアドレス送りません</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6924"/>
        <source>Claimed change is larger than payment to the change address</source>
        <translation type="unfinished">請求したお釣りはお釣りのアドレスに送ったペイメントより大きいです</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6955"/>
        <source>sending %s to %s</source>
        <translation>%s を %s に送ってます</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6965"/>
        <source> dummy output(s)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6968"/>
        <source>with no destinations</source>
        <translation>目的地なし</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7009"/>
        <source>This is a multisig wallet, it can only sign with sign_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7032"/>
        <source>Failed to sign transaction</source>
        <translation>取引を署名できませんでした</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7038"/>
        <source>Failed to sign transaction: </source>
        <translation>取引を署名できませんでした： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7059"/>
        <source>Transaction raw hex data exported to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7080"/>
        <source>Failed to load transaction from file</source>
        <translation>ファイルからの取引のロードに失敗しました</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5132"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5459"/>
        <source>RPC error: </source>
        <translation>RPCエラー： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="716"/>
        <source>wallet is watch-only and has no spend key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="860"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1044"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1097"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1164"/>
        <source>Your original password was incorrect.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="875"/>
        <source>Error with wallet rewrite: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2435"/>
        <source>invalid unit</source>
        <translation>不正なユニット</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2453"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2515"/>
        <source>invalid count: must be an unsigned integer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2471"/>
        <source>invalid value</source>
        <translation>不正な金額</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4021"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4041"/>
        <source>bad m_restore_height parameter: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3985"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4032"/>
        <source>Restore height is: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4897"/>
        <source>Daemon is local, assuming trusted</source>
        <translation>デーモンはローカルです。信頼できるデーモン予期してます</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4632"/>
        <source>Password for new watch-only wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5142"/>
        <source>internal error: </source>
        <translation>内部エラー： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1632"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5147"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5464"/>
        <source>unexpected error: </source>
        <translation>予期せぬエラー： </translation>
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
        <translation>不明なエラー</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5157"/>
        <source>refresh failed: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5157"/>
        <source>Blocks received: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5195"/>
        <source>unlocked balance: </source>
        <translation>ロック解除された残高： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3303"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5302"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5364"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>amount</source>
        <translation>金額</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="362"/>
        <source>false</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="680"/>
        <source>Unknown command: </source>
        <translation>未知のコマンド： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="687"/>
        <source>Command usage: </source>
        <translation>コマンドの使用： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="690"/>
        <source>Command description: </source>
        <translation>コマンドの記述： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="756"/>
        <source>wallet is multisig but not yet finalized</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="789"/>
        <source>Failed to retrieve seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="813"/>
        <source>wallet is multisig and has no seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="922"/>
        <source>Error: failed to estimate backlog array size: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="927"/>
        <source>Error: bad estimated backlog array size</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="939"/>
        <source> (current)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="942"/>
        <source>%u block (%u minutes) backlog at priority %u%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="944"/>
        <source>%u to %u block (%u to %u minutes) backlog at priority %u</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="947"/>
        <source>No backlog at priority </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="967"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1012"/>
        <source>This wallet is already multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="972"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1017"/>
        <source>wallet is watch-only and cannot be made multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="978"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1023"/>
        <source>This wallet has been used before, please use a new wallet to create a multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="986"/>
        <source>Send this multisig info to all other participants, then use make_multisig &lt;threshold&gt; &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="987"/>
        <source>This includes the PRIVATE view key, so needs to be disclosed only to that multisig wallet&apos;s participants </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1037"/>
        <source>Invalid threshold</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1057"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1179"/>
        <source>Another step is needed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1069"/>
        <source>Error creating multisig: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1076"/>
        <source>Error creating multisig: new wallet is not multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1079"/>
        <source> multisig address: </source>
        <translation> マルチサインアドレス： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1103"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1152"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1219"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1285"/>
        <source>This wallet is not multisig</source>
        <translation>これはマルチシッグウォレットではありません</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1108"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1157"/>
        <source>This wallet is already finalized</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1124"/>
        <source>Failed to finalize multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1130"/>
        <source>Failed to finalize multisig: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1191"/>
        <source>Multisig address: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1224"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1290"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1384"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1500"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1581"/>
        <source>This multisig wallet is not yet finalized</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1260"/>
        <source>Error exporting multisig info: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1264"/>
        <source>Multisig info exported to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1330"/>
        <source>Multisig info imported</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1334"/>
        <source>Failed to import multisig info: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1345"/>
        <source>Failed to update spent status after importing multisig info: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1351"/>
        <source>Untrusted daemon, spent status may be incorrect. Use a trusted daemon and run &quot;rescan_spent&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1379"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1495"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1576"/>
        <source>This is not a multisig wallet</source>
        <translation>これはマルチシッグウォレットではありません</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1429"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1438"/>
        <source>Failed to sign multisig transaction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1445"/>
        <source>Multisig error: </source>
        <translation>マルチサインエラー： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1450"/>
        <source>Failed to sign multisig transaction: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1473"/>
        <source>It may be relayed to the network with submit_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1532"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1602"/>
        <source>Failed to load multisig transaction from file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1538"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1607"/>
        <source>Multisig transaction signed by only %u signers, needs %u more signatures</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1547"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9330"/>
        <source>Transaction successfully submitted, transaction </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1548"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9331"/>
        <source>You can check its status by using the `show_transfers` command.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1623"/>
        <source>Failed to export multisig transaction to file </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1627"/>
        <source>Saved exported multisig transaction file(s): </source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2274"/>
        <source>could not change default ring size</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2549"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2620"/>
        <source>Invalid height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2667"/>
        <source>invalid argument: must be either 1/yes or 0/no</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2738"/>
        <source>Start mining in the daemon (bg_mining and ignore_battery are optional booleans).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2741"/>
        <source>Stop mining in the daemon.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2745"/>
        <source>Set another daemon to connect to.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2748"/>
        <source>Save the current blockchain data.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2751"/>
        <source>Synchronize the transactions and balance.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2755"/>
        <source>Show the wallet&apos;s balance of the currently selected account.</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2768"/>
        <source>Show the blockchain height.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2782"/>
        <source>Send all unmixable outputs to yourself with ring_size 1</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2789"/>
        <source>Send all unlocked outputs below the threshold to an address.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2793"/>
        <source>Send a single output of the given key image to an address without change.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2797"/>
        <source>Donate &lt;amount&gt; to the development team (donate.getmonero.org).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2804"/>
        <source>Submit a signed transaction from a file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2808"/>
        <source>Change the current log detail (level must be &lt;0-4&gt;).</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2822"/>
        <source>If no arguments are specified or &lt;index&gt; is specified, the wallet shows the default or specified address. If &quot;all&quot; is specified, the wallet shows all the existing addresses in the currently selected account. If &quot;new &quot; is specified, the wallet creates a new address with the provided label text (which can be empty). If &quot;label&quot; is specified, the wallet sets the label of the address specified by &lt;index&gt; to the provided label text.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2826"/>
        <source>Encode a payment ID into an integrated address for the current wallet public address (no argument uses a random payment ID), or decode an integrated address to standard address and payment ID</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2830"/>
        <source>Print all entries in the address book, optionally adding/deleting an entry to/from it.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2833"/>
        <source>Save the wallet data.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2836"/>
        <source>Save a watch-only keys file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2839"/>
        <source>Display the private view key.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2842"/>
        <source>Display the private spend key.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2845"/>
        <source>Display the Electrum-style mnemonic seed</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2900"/>
        <source>Rescan the blockchain for spent outputs.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2904"/>
        <source>Get the transaction key (r) for a given &lt;txid&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2912"/>
        <source>Check the amount going to &lt;address&gt; in &lt;txid&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2916"/>
        <source>Generate a signature proving funds sent to &lt;address&gt; in &lt;txid&gt;, optionally with a challenge string &lt;message&gt;, using either the transaction secret key (when &lt;address&gt; is not your wallet&apos;s address) or the view secret key (otherwise), which does not disclose the secret key.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2920"/>
        <source>Check the proof for funds going to &lt;address&gt; in &lt;txid&gt; with the challenge string &lt;message&gt; if any.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2924"/>
        <source>Generate a signature proving that you generated &lt;txid&gt; using the spend secret key, optionally with a challenge string &lt;message&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2928"/>
        <source>Check a signature proving that the signer generated &lt;txid&gt;, optionally with a challenge string &lt;message&gt;.</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2958"/>
        <source>Show the unspent outputs of a specified address within an optional amount range.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2962"/>
        <source>Rescan the blockchain from scratch. If &quot;hard&quot; is specified, you will lose any information which can not be recovered from the blockchain itself.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2966"/>
        <source>Set an arbitrary string note for a &lt;txid&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2970"/>
        <source>Get a string note for a txid.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2974"/>
        <source>Set an arbitrary description for the wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2978"/>
        <source>Get the description of the wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2981"/>
        <source>Show the wallet&apos;s status.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2984"/>
        <source>Show the wallet&apos;s information.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2988"/>
        <source>Sign the contents of a file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2992"/>
        <source>Verify a signature on the contents of a file.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3000"/>
        <source>Import a signed key images list and verify their spent status.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3012"/>
        <source>Export a set of outputs owned by this wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3016"/>
        <source>Import a set of outputs owned by this wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3020"/>
        <source>Show information about a transfer to/from this address.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3023"/>
        <source>Change the wallet&apos;s password.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3030"/>
        <source>Print the information about the current fee and transaction backlog.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3032"/>
        <source>Export data needed to create a multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3035"/>
        <source>Turn this wallet into a multisig wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3039"/>
        <source>Turn this wallet into a multisig wallet, extra step for N-1/N wallets</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3047"/>
        <source>Export multisig info for other participants</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3051"/>
        <source>Import multisig info from other participants</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3055"/>
        <source>Sign a multisig transaction from a file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3059"/>
        <source>Submit a signed multisig transaction from a file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3063"/>
        <source>Export a signed multisig transaction to a file</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3316"/>
        <source>1/yes or 0/no</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3414"/>
        <source>No wallet found with that name. Confirm creation of new wallet named: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3540"/>
        <source>can&apos;t specify both --restore-deterministic-wallet or --restore-multisig-wallet and --non-deterministic</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3546"/>
        <source>--restore-multisig-wallet uses --generate-new-wallet, not --wallet-file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3562"/>
        <source>specify a recovery parameter with the --electrum-seed=&quot;multisig seed here&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3591"/>
        <source>Multisig seed failed verification</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3641"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3718"/>
        <source>This address is a subaddress which cannot be used here.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3796"/>
        <source>Error: expected M/N, but got: </source>
        <translation>エラー： N/Mを欲しかったでもこれを貰いました： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3801"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got: </source>
        <translation>エラー： N ＞ 1 と N ＜＝ M のこと欲しかったでもこれを貰いました： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3806"/>
        <source>Error: M/N is currently unsupported. </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3809"/>
        <source>Generating master wallet from %u of %u multisig wallet keys</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3838"/>
        <source>failed to parse secret view key</source>
        <translation>秘密なビューキーの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3846"/>
        <source>failed to verify secret view key</source>
        <translation>秘密なビューキーの検証に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3889"/>
        <source>Error: M/N is currently unsupported</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4055"/>
        <source>Restore height </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4083"/>
        <source>Warning: using an untrusted daemon at %s, privacy will be lessened</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4203"/>
        <source>Enter the number corresponding to the language of your choice</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4457"/>
        <source>failed to generate new mutlisig wallet</source>
        <translation>新しいマルチシッグウォレットの生成に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4460"/>
        <source>Generated new %u/%u multisig wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4509"/>
        <source>Opened %u/%u multisig wallet%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4570"/>
        <source>Use &quot;help &lt;command&gt;&quot; to see a command&apos;s documentation.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4628"/>
        <source>wallet is multisig and cannot save a watch-only version</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4905"/>
        <source>This does not seem to be a valid daemon URL.</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4956"/>
        <source>WARNING: this transaction uses an unencrypted payment ID: these are obsolete. Support will be withdrawn in the future. Use subaddresses instead.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5108"/>
        <source>New transfer received since rescan was started. Key images are incomplete.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5183"/>
        <source> (Some owned outputs have partial key images - import_multisig_info needed)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5186"/>
        <source>Currently selected account: [</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5186"/>
        <source>] </source>
        <translation>] </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5188"/>
        <source>Tag: </source>
        <translation>タグ： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5188"/>
        <source>(No tag assigned)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5200"/>
        <source>Balance per address:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5201"/>
        <source>Address</source>
        <translation>アドレス</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5201"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8451"/>
        <source>Balance</source>
        <translation>残高</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5201"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8451"/>
        <source>Unlocked balance</source>
        <translation>ロック解除された残高</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5201"/>
        <source>Outputs</source>
        <translation>アウトプット</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5201"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8451"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9564"/>
        <source>Label</source>
        <translation>ラベル</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5209"/>
        <source>%8u %6s %21s %21s %7u %21s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5302"/>
        <source>spent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5302"/>
        <source>global index</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5336"/>
        <source>No incoming available transfers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5340"/>
        <source>No incoming unavailable transfers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5364"/>
        <source>payment</source>
        <translation>ペイメント</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5364"/>
        <source>transaction</source>
        <translation>取引</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5364"/>
        <source>height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5364"/>
        <source>unlock time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5376"/>
        <source>No payments with id </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5424"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5514"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5843"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6302"/>
        <source>failed to get blockchain height: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5522"/>
        <source>
Transaction %llu/%llu: txid=%s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5559"/>
        <source>failed to get output: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5567"/>
        <source>output key&apos;s originating block height shouldn&apos;t be higher than the blockchain height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5571"/>
        <source>
Originating block heights: </source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5602"/>
        <source>, which can break the anonymity of ring signature. Make sure this is intentional!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5642"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6254"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6547"/>
        <source>Ring size must not be 0</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5654"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6266"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6559"/>
        <source>ring size %u is too small, minimum is %u</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5666"/>
        <source>wrong number of arguments</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6816"/>
        <source>Failed to parse donation address: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6830"/>
        <source>Donating %s %s to The Monero Project (donate.getmonero.org or %s).</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6832"/>
        <source>Donating %s %s to %s.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7164"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7175"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7182"/>
        <source>failed to parse tx_key</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7191"/>
        <source>Tx key successfully stored.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7195"/>
        <source>Failed to store tx key: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7696"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>block</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7844"/>
        <source>usage: show_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7897"/>
        <source>usage: export_transfers [in|out|all|pending|failed|coinbase] [index=&lt;N1&gt;[,&lt;N2&gt;,...]] [&lt;min_height&gt; [&lt;max_height&gt;]] [output=&lt;path&gt;]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>direction</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>timestamp</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>running balance</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>hash</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>payment ID</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>fee</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>destination</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>index</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>note</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7976"/>
        <source>CSV exported to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8159"/>
        <source>Warning: this will lose any information which can not be recovered from the blockchain.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8160"/>
        <source>This includes destination addresses, tx secret keys, tx notes, etc</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8832"/>
        <source>Network type: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8833"/>
        <source>Testnet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8834"/>
        <source>Stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8834"/>
        <source>Mainnet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8999"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9045"/>
        <source>command only supported by HW wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9004"/>
        <source>hw wallet does not support cold KI sync</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9016"/>
        <source>Please confirm the key image sync on the device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9022"/>
        <source>Key images synchronized to height </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9025"/>
        <source>Running untrusted daemon, cannot determine which transaction output is spent. Use a trusted daemon with --trusted-daemon and run rescan_spent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9028"/>
        <source> spent, </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9028"/>
        <source> unspent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9032"/>
        <source>Failed to import key images</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9037"/>
        <source>Failed to import key images: </source>
        <translation type="unfinished">キーイメージをインポートできませんでした： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9054"/>
        <source>Failed to reconnect device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9059"/>
        <source>Failed to reconnect device: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9323"/>
        <source>Transaction successfully saved to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9323"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9325"/>
        <source>, txid </source>
        <translation>、txid </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9325"/>
        <source>Failed to save transaction to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7014"/>
        <source>This is a watch only wallet</source>
        <translation>これは閲覧専用ウォレットです</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9253"/>
        <source>Double spend seen on the network: this transaction may or may not end up being mined</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9288"/>
        <source>Transaction ID not found</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="357"/>
        <source>true</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="410"/>
        <source>failed to parse refresh type</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="747"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="818"/>
        <source>wallet is watch-only and has no seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="765"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="828"/>
        <source>wallet is non-deterministic and has no seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="772"/>
        <source>Enter optional seed offset passphrase, empty to see raw seed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="838"/>
        <source>Incorrect password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="906"/>
        <source>Current fee is %s %s per %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1059"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1181"/>
        <source>Send this multisig info to all other participants, then use exchange_multisig_keys &lt;info1&gt; [&lt;info2&gt;...] with others&apos; multisig info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1190"/>
        <source>Multisig wallet has been successfully created. Current wallet type: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1196"/>
        <source>Failed to perform multisig keys exchange: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1523"/>
        <source>Failed to load multisig transaction from MMS</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1655"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1812"/>
        <source>Invalid key image</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1661"/>
        <source>Invalid txid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1673"/>
        <source>Key image either not spent, or spent with mixin 0</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1688"/>
        <source>Failed to get key image ring: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1703"/>
        <source>File doesn&apos;t exist</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1725"/>
        <source>Invalid ring specification: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1733"/>
        <source>Invalid key image: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1738"/>
        <source>Invalid ring type, expected relative or abosolute: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1744"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1756"/>
        <source>Error reading line: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1767"/>
        <source>Invalid ring: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1776"/>
        <source>Invalid relative ring: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1788"/>
        <source>Invalid absolute ring: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1797"/>
        <source>Failed to set ring for key image: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1797"/>
        <source>Continuing.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1827"/>
        <source>Missing absolute or relative keyword</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1837"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1844"/>
        <source>invalid index: must be a strictly positive unsigned integer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1852"/>
        <source>invalid index: indices wrap</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1862"/>
        <source>invalid index: indices should be in strictly ascending order</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1869"/>
        <source>failed to set ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1946"/>
        <source>First line is not an amount</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1960"/>
        <source>Invalid output: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1970"/>
        <source>Bad argument: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1970"/>
        <source>should be &quot;add&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1979"/>
        <source>Failed to open file</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1985"/>
        <source>Invalid output key, and file doesn&apos;t exist</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1991"/>
        <source>Failed to mark output spent: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2008"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2035"/>
        <source>Invalid output</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2018"/>
        <source>Failed to mark output unspent: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2042"/>
        <source>Spent: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2044"/>
        <source>Not spent: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2048"/>
        <source>Failed to check whether output is spent: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2063"/>
        <source>Failed to save known rings: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2171"/>
        <source>Please confirm the transaction on the device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2218"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2237"/>
        <source>wallet is watch-only and cannot transfer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2255"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5982"/>
        <source>WARNING: this is a non default ring size, which may harm your privacy. Default is recommended.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2257"/>
        <source>WARNING: from v8, ring size will be fixed and this setting will be ignored.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2286"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2309"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2325"/>
        <source>priority must be either 0, 1, 2, 3, or 4, or one of: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2330"/>
        <source>could not change default priority</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2400"/>
        <source>invalid argument: must be either 0/never, 1/action, or 2/encrypt/decrypt</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2686"/>
        <source>Device name not specified</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2695"/>
        <source>Device reconnect failed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2700"/>
        <source>Device reconnect failed: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2771"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt;. If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2775"/>
        <source>Transfer &lt;amount&gt; to &lt;address&gt; and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index=&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet uses outputs received by addresses of those indices. If omitted, the wallet randomly chooses address indices to be used. In any case, it tries its best not to combine outputs across multiple addresses. &lt;priority&gt; is the priority of the transaction. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability. Multiple payments can be made at once by adding URI_2 or &lt;address_2&gt; &lt;amount_2&gt; etcetera (before the payment ID, if it&apos;s included)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2779"/>
        <source>Send all unlocked balance to an address and lock it for &lt;lockblocks&gt; (max. 1000000). If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. &lt;priority&gt; is the priority of the sweep. The higher the priority, the higher the transaction fee. Valid values in priority order (from lowest to highest) are: unimportant, normal, elevated, priority. If omitted, the default value (see the command &quot;set priority&quot;) is used. &lt;ring_size&gt; is the number of inputs to include for untraceability.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2785"/>
        <source>Send all unlocked balance to an address. If the parameter &quot;index&lt;N1&gt;[,&lt;N2&gt;,...]&quot; is specified, the wallet sweeps outputs received by those address indices. If omitted, the wallet randomly chooses an address index to be used. If the parameter &quot;outputs=&lt;N&gt;&quot; is specified and  N &gt; 0, wallet splits the transaction into N even outputs.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2801"/>
        <source>Sign a transaction from a file. If the parameter &quot;export_raw&quot; is specified, transaction raw hex data suitable for the daemon RPC /sendrawtransaction is exported.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2908"/>
        <source>Set the transaction key (r) for a given &lt;txid&gt; in case the tx was made by some other device or 3rd party wallet.</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2954"/>
        <source>Export to CSV the incoming/outgoing transfers within an optional height range.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="2996"/>
        <source>Export a signed set of key images to a &lt;filename&gt;.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3004"/>
        <source>Synchronizes key images with the hw wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3008"/>
        <source>Attempts to reconnect HW wallet.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3027"/>
        <source>Generate a new random full size payment id (obsolete). These will be unencrypted on the blockchain, see integrated_address for encrypted short payment ids.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3043"/>
        <source>Performs extra multisig keys exchange rounds. Needed for arbitrary M/N multisig wallets</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3079"/>
        <source>Display current MMS configuration</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3083"/>
        <source>Set or modify authorized signer info (single-word label, transport address, Monero address), or list all signers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3087"/>
        <source>List all messages</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3091"/>
        <source>Evaluate the next possible multisig-related action(s) according to wallet state, and execute or offer for choice
By using &apos;sync&apos; processing of waiting messages with multisig sync info can be forced regardless of wallet state</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3096"/>
        <source>Force generation of multisig sync info regardless of wallet state, to recover from special situations like &quot;stale data&quot; errors</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3100"/>
        <source>Initiate transfer with MMS support; arguments identical to normal &apos;transfer&apos; command arguments, for info see there</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3104"/>
        <source>Delete a single message by giving its id, or delete all messages by using &apos;all&apos;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3108"/>
        <source>Send a single message by giving its id, or send all waiting messages</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3112"/>
        <source>Check right away for new messages to receive</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3116"/>
        <source>Write the content of a message to a file &quot;mms_message_content&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3120"/>
        <source>Send a one-line message to an authorized signer, identified by its label, or show any waiting unread notes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3124"/>
        <source>Show detailed info about a single message</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3138"/>
        <source>Start auto-config at the auto-config manager&apos;s wallet by issuing auto-config tokens and optionally set others&apos; labels</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3142"/>
        <source>Delete any auto-config tokens and abort a auto-config process</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3146"/>
        <source>Start auto-config by using the token received from the auto-config manager</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3164"/>
        <source>Save known rings to the shared rings database</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3168"/>
        <source>Mark output(s) as spent so they never get selected as fake outputs in a ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3172"/>
        <source>Marks an output as unspent so it may get selected as a fake output in a ring</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3176"/>
        <source>Checks whether an output is marked as spent</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3200"/>
        <source>Returns version information</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3297"/>
        <source>full (slowest, no assumptions); optimize-coinbase (fast, assumes the whole coinbase is paid to a single address); no-coinbase (fastest, assumes we receive no coinbase transaction), default (same as optimize-coinbase)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3298"/>
        <source>0, 1, 2, 3, or 4, or one of </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3300"/>
        <source>0|1|2 (or never|action|decrypt)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3301"/>
        <source>monero, millinero, micronero, nanonero, piconero</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3312"/>
        <source>&lt;major&gt;:&lt;minor&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3317"/>
        <source>&lt;device_name[:device_spec]&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3338"/>
        <source>wrong number range, use: %s</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3377"/>
        <source>Wallet name not valid. Please try again or use Ctrl-C to quit.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3394"/>
        <source>Wallet and key files found, loading...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3400"/>
        <source>Key file found but not wallet file. Regenerating...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3406"/>
        <source>Key file not found. Failed to open wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3425"/>
        <source>Generating new wallet...</source>
        <translation>新しいウォレットを生じてます...</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3443"/>
        <source>NOTE: the following %s can be used to recover access to your wallet. Write them down and store them somewhere safe and secure. Please do not store them in your email or on file storage services outside of your immediate control.
</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3445"/>
        <source>string</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3445"/>
        <source>25 words</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3506"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3521"/>
        <source>can&apos;t specify more than one of --generate-new-wallet=&quot;wallet_name&quot;, --wallet-file=&quot;wallet_name&quot;, --generate-from-view-key=&quot;wallet_name&quot;, --generate-from-spend-key=&quot;wallet_name&quot;, --generate-from-keys=&quot;wallet_name&quot;, --generate-from-multisig-keys=&quot;wallet_name&quot;, --generate-from-json=&quot;jsonfilename&quot; and --generate-from-device=&quot;wallet_name&quot;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3600"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Electrumな単語表の検証に失敗しました</translation>
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
        <translation>データをもらいませんでしたのでキャンセルしました</translation>
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
        <translation>アドレスの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3656"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3748"/>
        <source>failed to parse view key secret key</source>
        <translation>秘密なビューキーの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3665"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3765"/>
        <source>failed to verify view key secret key</source>
        <translation>秘密なビューキーの検証に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3669"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3769"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3851"/>
        <source>view key does not match standard address</source>
        <translation>ビューキーが一般的なアドレスと一致しませんでした</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3674"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3695"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3773"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3907"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3934"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3966"/>
        <source>account creation failed</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3691"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3733"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3876"/>
        <source>failed to parse spend key secret key</source>
        <translation>秘密なスペンドキーの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3757"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3896"/>
        <source>failed to verify spend key secret key</source>
        <translation>秘密なスペンドキーの検証に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3761"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3901"/>
        <source>spend key does not match standard address</source>
        <translation>スペンドキーが一般的なアドレスと一致しませんでした</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3866"/>
        <source>Secret spend key (%u of %u)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3941"/>
        <source>No restore height is specified.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3942"/>
        <source>Assumed you are creating a new account, restore will be done from current estimated blockchain height.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="3947"/>
        <source>account creation aborted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4069"/>
        <source>can&apos;t specify --subaddress-lookahead and --wallet-file at the same time</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4073"/>
        <source>failed to open account</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4078"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4779"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4832"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4917"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7254"/>
        <source>wallet is null</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4086"/>
        <source>Failed to initialize ring database: privacy enhancing features will be inactive</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4296"/>
        <source>View key: </source>
        <translation>ビューキー： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4407"/>
        <source>Generated new wallet on hw device: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4486"/>
        <source>Key file not found. Failed to open wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4563"/>
        <source>You may want to remove the file &quot;%s&quot; and try again</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4591"/>
        <source>failed to deinitialize wallet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4644"/>
        <source>Watch only wallet saved as: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4648"/>
        <source>Failed to save watch only wallet: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4770"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5432"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8967"/>
        <source>this command requires a trusted daemon. Enable with --trusted-daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4886"/>
        <source>Expected trusted or untrusted, got </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4903"/>
        <source>trusted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4903"/>
        <source>untrusted</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4927"/>
        <source>blockchain can&apos;t be saved: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="4992"/>
        <source>Password needed (%s) - use the refresh command</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5000"/>
        <source>Enter password</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5015"/>
        <source>Device requires attention</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5023"/>
        <source>Enter device PIN</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5025"/>
        <source>Failed to read device PIN</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5032"/>
        <source>Please enter the device passphrase on the device</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5039"/>
        <source>Enter device passphrase</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5041"/>
        <source>Failed to read device passphrase</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
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
        <translation>デーモンは忙しいです。後でもう一度試してください。</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5127"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5450"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation>デーモンの接続が確立ありません。デーモンが実行中になっていることを確認してください。</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5137"/>
        <source>refresh error: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5185"/>
        <source> (Some owned outputs have missing key images - import_key_images needed)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5194"/>
        <source>Balance: </source>
        <translation>残高： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5263"/>
        <source>Invalid keyword: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5301"/>
        <source>pubkey</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5301"/>
        <source>key image</source>
        <translation>キーイメージ</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5302"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5318"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7922"/>
        <source>unlocked</source>
        <translation></translation>
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
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5454"/>
        <source>failed to get spent status</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5538"/>
        <source>failed to find construction data for tx input</source>
        <translation type="unfinished"></translation>
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
        <translation>同じ取引</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5601"/>
        <source>blocks that are temporally very close</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5882"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5892"/>
        <source>Is this okay anyway?</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8654"/>
        <source>Short payment IDs are to be used within an integrated address only</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9457"/>
        <source> (Y/Yes/N/No): </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9484"/>
        <source>Choose processing:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9493"/>
        <source>Sign tx</source>
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9515"/>
        <source>unknown</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9521"/>
        <source>Choice: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9533"/>
        <source>Wrong choice</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9540"/>
        <source>Id</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9540"/>
        <source>I/O</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9540"/>
        <source>Authorized Signer</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9541"/>
        <source>Message Type</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9541"/>
        <source>Height</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9541"/>
        <source>R</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9541"/>
        <source>Message State</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9541"/>
        <source>Since</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9558"/>
        <source> ago</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9564"/>
        <source>#</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9564"/>
        <source>Transport Address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9565"/>
        <source>Auto-Config Token</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9565"/>
        <source>Monero Address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9569"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9577"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9579"/>
        <source>&lt;not set&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9620"/>
        <source>Message </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9621"/>
        <source>In/out: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9623"/>
        <source>State: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9623"/>
        <source>%s since %s, %s ago</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9627"/>
        <source>Sent: Never</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9631"/>
        <source>Sent: %s, %s ago</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9634"/>
        <source>Authorized signer: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9635"/>
        <source>Content size: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9635"/>
        <source> bytes</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9636"/>
        <source>Content: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9636"/>
        <source>(binary data)</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9666"/>
        <source>Send these messages now?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9676"/>
        <source>Queued for sending.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9696"/>
        <source>Invalid message id</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9705"/>
        <source>usage: mms init &lt;required_signers&gt;/&lt;authorized_signers&gt; &lt;own_label&gt; &lt;own_transport_address&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9711"/>
        <source>The MMS is already initialized. Re-initialize by deleting all signer info and messages?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9726"/>
        <source>Error in the number of required signers and/or authorized signers</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9743"/>
        <source>The MMS is not active.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9766"/>
        <source>Invalid signer number </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9771"/>
        <source>mms signer [&lt;number&gt; &lt;label&gt; [&lt;transport_address&gt; [&lt;monero_address&gt;]]]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9790"/>
        <source>Invalid Monero address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9797"/>
        <source>Wallet state does not allow changing Monero addresses anymore</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9809"/>
        <source>Usage: mms list</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9822"/>
        <source>Usage: mms next [sync]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9847"/>
        <source>No next step: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9857"/>
        <source>prepare_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9863"/>
        <source>make_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9878"/>
        <source>exchange_multisig_keys</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9893"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="10013"/>
        <source>export_multisig_info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9902"/>
        <source>import_multisig_info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9915"/>
        <source>sign_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9925"/>
        <source>submit_multisig</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9935"/>
        <source>Send tx</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9946"/>
        <source>Process signer config</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9958"/>
        <source>Replace current signer config with the one displayed above?</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9972"/>
        <source>Process auto config data</source>
        <translation type="unfinished"></translation>
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
        <location filename="../src/simplewallet/simplewallet.cpp" line="7369"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7479"/>
        <source>Good signature</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7396"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7481"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7581"/>
        <source>Bad signature</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8612"/>
        <source>Standard address: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8617"/>
        <source>failed to parse payment ID or address</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8659"/>
        <source>failed to parse payment ID</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8677"/>
        <source>failed to parse index</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8685"/>
        <source>Address book is empty.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8691"/>
        <source>Index: </source>
        <translation>インデックス： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8692"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8823"/>
        <source>Address: </source>
        <translation>アドレス： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8693"/>
        <source>Payment ID: </source>
        <translation>ペイメントID： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8694"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8822"/>
        <source>Description: </source>
        <translation>記述： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8852"/>
        <source>wallet is watch-only and cannot sign</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1313"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8866"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8892"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9124"/>
        <source>failed to read file </source>
        <translation>ファイルの読み込みに失敗しました </translation>
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
        <translation>アドレスはサブアドレスであってはならないです</translation>
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
        <translation> (デーモンありません）</translation>
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
        <translation>インデックスの解析に失敗しました： </translation>
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
        <translation>、ロック解除された残高： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8439"/>
        <source>Untagged accounts:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8445"/>
        <source>Tag %s is unregistered.</source>
        <translation type="unfinished">タグ %s を登録してません。</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8448"/>
        <source>Accounts with tag: </source>
        <translation>タグを持ってるアカウント： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8449"/>
        <source>Tag&apos;s description: </source>
        <translation>タグの記述： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8451"/>
        <source>Account</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8457"/>
        <source> %c%8u %6s %21s %21s %21s</source>
        <translation> %c%8u %6s %21s %21s %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8467"/>
        <source>----------------------------------------------------------------------------------</source>
        <translation>----------------------------------------------------------------------------------</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8468"/>
        <source>%15s %21s %21s</source>
        <translation>%15s %21s %21s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8491"/>
        <source>Primary address</source>
        <translation>プライマリアドレス</translation>
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
        <translation>サブアドレス： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8779"/>
        <source>no description found</source>
        <translation>記述を見つかれませんでした</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8781"/>
        <source>description found: </source>
        <translation>記述を見つかれました： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8821"/>
        <source>Filename: </source>
        <translation>ファイル名： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8826"/>
        <source>Watch only</source>
        <translation>閲覧専用</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8828"/>
        <source>%u/%u multisig%s</source>
        <translation>%u/%u マルチサイン%s</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8830"/>
        <source>Normal</source>
        <translation>ノーマル</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8831"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9622"/>
        <source>Type: </source>
        <translation>タイプ： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8857"/>
        <source>This wallet is multisig and cannot sign</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8906"/>
        <source>Bad signature from </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8910"/>
        <source>Good signature from </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8929"/>
        <source>wallet is watch-only and cannot export key images</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1252"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8943"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9091"/>
        <source>failed to save file </source>
        <translation>ファイルを保存できませんでした </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8954"/>
        <source>Signed key images exported to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9102"/>
        <source>Outputs exported to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5752"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6805"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7515"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8004"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8012"/>
        <source>amount is wrong: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="5753"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6805"/>
        <source>expected number from 0 to </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6122"/>
        <source>Sweeping </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6738"/>
        <source>Money successfully sent, transaction: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6933"/>
        <source>Change goes to more than one address</source>
        <translation type="unfinished">お釣りは複数のアドレスに送ります</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6974"/>
        <source>%s change to %s</source>
        <translation>%s のお釣り %s に</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="6977"/>
        <source>no change</source>
        <translation>お釣りありません</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1459"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="1472"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7049"/>
        <source>Transaction successfully signed to file </source>
        <translation type="unfinished"></translation>
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
        <translation>txidの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7132"/>
        <source>Tx key: </source>
        <translation>txキー： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7137"/>
        <source>no tx keys found for this txid</source>
        <translation type="unfinished">このtxidのためにtxキーを見つかれませんでした</translation>
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
        <translation>txキーの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7235"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7323"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7401"/>
        <source>error: </source>
        <translation>エラー： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7299"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7372"/>
        <source>received</source>
        <translation>貰いました</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7299"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7372"/>
        <source>in txid</source>
        <translation>txidに</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7318"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7391"/>
        <source>received nothing in txid</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7302"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7375"/>
        <source>WARNING: this transaction is not yet included in the blockchain!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7308"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7381"/>
        <source>This transaction has %u confirmations</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7312"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7385"/>
        <source>WARNING: failed to determine number of confirmations!</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7666"/>
        <source>bad min_height parameter:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7678"/>
        <source>bad max_height parameter:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="7696"/>
        <source>in</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8019"/>
        <source>&lt;min_amount&gt; should be smaller than &lt;max_amount&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8051"/>
        <source>
Amount: </source>
        <translation>
金額： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8051"/>
        <source>, number of keys: </source>
        <translation>、キーの数： </translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8062"/>
        <source>
Max block height: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8063"/>
        <source>
Min amount found: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8064"/>
        <source>
Max amount found: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8065"/>
        <source>
Total count: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8105"/>
        <source>
Bin size: </source>
        <translation type="unfinished"></translation>
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
        <translation>+--&gt; ブロック高
</translation>
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
        <translation>ウォレット</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="893"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8587"/>
        <source>Random payment ID: </source>
        <translation>ランダムなペイメントID： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="8588"/>
        <source>Matching integrated address: </source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>genms</name>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="70"/>
        <source>Base filename (-1, -2, etc suffixes will be appended as needed)</source>
        <translation>ベースファイル名（必要があれば－１、－２、とかのサフィックスを追加します）</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="71"/>
        <source>Give threshold and participants at once as M/N</source>
        <translation>M/Nってのフォーマットで同じ時に閾値と参加者をください</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="72"/>
        <source>How many participants will share parts of the multisig wallet</source>
        <translation>マルチサインウォレットを分ける人はいくついますか</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="73"/>
        <source>How many signers are required to sign a valid transaction</source>
        <translation>有効な取引を署名するために必要な人いくついますか</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="74"/>
        <source>Create testnet multisig wallets</source>
        <translation>テストネットのマルチサインウォレットを作る</translation>
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
        <translation>%u %u/%u マルチサインウォレットを生じてます</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="107"/>
        <source>Failed to verify multisig info</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="142"/>
        <source>Error verifying multisig extra info</source>
        <translation>マルチサインの追加情報を検証中にエラーありました</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="153"/>
        <source>Generated multisig wallets for address </source>
        <translation>アドレスのためにマルチサインウォレットを生じなかった </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="157"/>
        <source>Error creating multisig wallets: </source>
        <translation>マルチサインウォレットを樹立中にエラーありました： </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="182"/>
        <source>This program generates a set of multisig wallets - use this simpler scheme only if all the participants trust each other</source>
        <translation>このプログラムはマルチサインウォレットのセットを生じます　－　みんながみんなを信用する場合にだけこの単純なスキームを使ってください</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="201"/>
        <source>Error: Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="208"/>
        <source>Error: expected N/M, but got: </source>
        <translation>エラー： N/Mを欲しかったでもこれを貰いました： </translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="216"/>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="225"/>
        <source>Error: either --scheme or both of --threshold and --participants may be given</source>
        <translation>エラー： --scheme あるいは--threshold と --participants を上げられる</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="232"/>
        <source>Error: expected N &gt; 1 and N &lt;= M, but got N==%u and M==%d</source>
        <translation>エラー： N ＞ 1 と N ＜＝ M のこと欲しかったでも N＝＝%u と M＝＝%d を貰いました</translation>
    </message>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="241"/>
        <source>Error: --filename-base is required</source>
        <translation>エラー： --filename-baseを使う必要だがあります</translation>
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
        <translation type="unfinished"></translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/message_store.cpp" line="1386"/>
        <source>out</source>
        <translation type="unfinished"></translation>
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
        <translation>新しいウォレットを生じろ &lt;arg&gt; をこっちにセーブしてください</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="136"/>
        <source>Generate new wallet from device and save it to &lt;arg&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="137"/>
        <source>Generate incoming-only wallet from view key</source>
        <translation>ビューキーで閲覧専用ウォレットを生じろください</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="138"/>
        <source>Generate deterministic wallet from spend key</source>
        <translation>スペンドキーで決定論的なウォレットを生じろください</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="139"/>
        <source>Generate wallet from private keys</source>
        <translation>秘密なキーでウォレットを生じろください</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="140"/>
        <source>Generate a master wallet from multisig wallet keys</source>
        <translation>マルチシガーウォレットキーでマスターウォレットを生じろください</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="142"/>
        <source>Language for mnemonic</source>
        <translation>ニーモニックのための言語</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="143"/>
        <source>Specify Electrum seed for wallet recovery/creation</source>
        <translation>ウォレットの回収や作成のためにElectrumなニーモニックシードを指定してください</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="144"/>
        <source>Recover wallet using Electrum-style mnemonic seed</source>
        <translation>Electrumなニーモニックシードでウォレットを復元してください</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="145"/>
        <source>Recover multisig wallet using Electrum-style mnemonic seed</source>
        <translation>Electrumなニーモニックシードでマルチシガーウォレットを復元してください</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="146"/>
        <source>Generate non-deterministic view and spend keys</source>
        <translation>非決定論的のビューとスペンドキーを生じろください</translation>
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
        <translation type="unfinished">インデックスの解析に失敗しました： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="493"/>
        <source>invalid format for subaddress lookahead; must be &lt;major&gt;:&lt;minor&gt;</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="510"/>
        <source>no connection to daemon. Please make sure daemon is running.</source>
        <translation type="unfinished">デーモンの接続が確立ありません。デーモンが実行中になっていることを確認してください。</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="515"/>
        <source>RPC error: </source>
        <translation type="unfinished">RPCエラー： </translation>
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
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="544"/>
        <source>Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="550"/>
        <source>not enough outputs for specified ring size</source>
        <translation type="unfinished">指定したリングサイズのアウトプットが不十分です</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="553"/>
        <source>output amount</source>
        <translation type="unfinished">アウトプットの金額</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="553"/>
        <source>found outputs to use</source>
        <translation type="unfinished">使うためにアウトプットを見つかれました</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="555"/>
        <source>Please use sweep_unmixable.</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="559"/>
        <source>transaction was not constructed</source>
        <translation type="unfinished">取引を作りませんでした</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="567"/>
        <source>Reason: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="576"/>
        <source>one of destinations is zero</source>
        <translation type="unfinished">宛先の1つはゼロです</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="581"/>
        <source>failed to find a suitable way to split transactions</source>
        <translation type="unfinished">取引を分割する適切な方法を見つけることができませんでした</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="587"/>
        <source>unknown transfer error: </source>
        <translation type="unfinished">不明な転送エラー： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="592"/>
        <source>Multisig error: </source>
        <translation type="unfinished">マルチサインエラー： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="598"/>
        <source>internal error: </source>
        <translation type="unfinished">内部エラー： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="603"/>
        <source>unexpected error: </source>
        <translation type="unfinished">予期せぬエラー： </translation>
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
        <translation type="unfinished">未知のコマンド： </translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="147"/>
        <source>Allow communicating with a daemon that uses a different RPC version</source>
        <translation>別のRPCバージョンを使用してるデーモンとの通信を許可してください</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="148"/>
        <source>Restore from specific blockchain height</source>
        <translation>特定ブロックチェイン高で復元してください</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="150"/>
        <source>The newly created transaction will not be relayed to the monero network</source>
        <translation>新しい取引をネットワークに中継しません</translation>
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
        <translation type="unfinished"></translation>
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
        <translation>デーモンは忙しいです。後でもう一度試してください。</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="323"/>
        <source>possibly lost connection to daemon</source>
        <translation>デモンの接続が切れましたかもしりません</translation>
    </message>
    <message>
        <location filename="../src/simplewallet/simplewallet.cpp" line="340"/>
        <source>Error: </source>
        <translation>エラー： </translation>
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
        <translation>ウォレットを初期化できませんでした</translation>
    </message>
</context>
<context>
    <name>tools::wallet2</name>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="234"/>
        <source>Use daemon instance at &lt;host&gt;:&lt;port&gt;</source>
        <translation>&lt;host&gt;:&lt;port&gt;でデーモンインスタンスを使ってください</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="235"/>
        <source>Use daemon instance at host &lt;arg&gt; instead of localhost</source>
        <translation>localhostの代わりにホスト &lt;arg&gt;でデーモンインスタンスを使ってください</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="240"/>
        <source>Wallet password file</source>
        <translation>ウォレットのパスワードファイル</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="241"/>
        <source>Use daemon instance at port &lt;arg&gt; instead of 18081</source>
        <translation>１８０８１の代わりにポート &lt;arg&gt;でデーモンインスタンスを使ってください</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="250"/>
        <source>For testnet. Daemon must also be launched with --testnet flag</source>
        <translation>テストネットのためにデーモンは --testnet のフラグで開始する必要があります</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="361"/>
        <source>can&apos;t specify daemon host or port more than once</source>
        <translation>デーモンのホストやポートを複数回指定することはできません</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="480"/>
        <source>can&apos;t specify more than one of --password and --password-file</source>
        <translation>--password と --passwordfile を1つしか指定しません</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="493"/>
        <source>the password file specified could not be read</source>
        <translation>指定されたパスワードファイルを読み取れません</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="519"/>
        <source>Failed to load file </source>
        <translation>ファイルのロードに失敗しました </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="239"/>
        <source>Wallet password (escape/quote as needed)</source>
        <translation>ウォレットのパスワード(随時にエスケープ文字を使ってください)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="236"/>
        <source>[&lt;ip&gt;:]&lt;port&gt; socks proxy to use for daemon connections</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="237"/>
        <source>Enable commands which rely on a trusted daemon</source>
        <translation type="unfinished">必要な信用できるデーモンのコマンドをエネーブルしてください</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="238"/>
        <source>Disable commands which rely on a trusted daemon</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="242"/>
        <source>Specify username[:password] for daemon RPC client</source>
        <translation>デーモンのRPCクライアントを使うためにユーザー名[：パスワード]を指定してください</translation>
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
        <translation type="unfinished">デーモンはローカルです。信頼できるデーモン予期してます</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="500"/>
        <source>no password specified; use --prompt-for-password to prompt for a password</source>
        <translation>パスワードを指定しませんでした。パスワードプロンプトを見るために--prompt-for-password を使ってください</translation>
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
        <translation>JSONを解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="532"/>
        <source>Version %u too new, we can only grok up to %u</source>
        <translation>バージョン %u 新しすぎるです。%u までグロークできます</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="548"/>
        <source>failed to parse view key secret key</source>
        <translation>秘密なビューキーの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="553"/>
        <location filename="../src/wallet/wallet2.cpp" line="621"/>
        <location filename="../src/wallet/wallet2.cpp" line="666"/>
        <source>failed to verify view key secret key</source>
        <translation>秘密なビューキーの検証に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="564"/>
        <source>failed to parse spend key secret key</source>
        <translation>秘密なスペンドキーの解析に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="569"/>
        <location filename="../src/wallet/wallet2.cpp" line="631"/>
        <location filename="../src/wallet/wallet2.cpp" line="692"/>
        <source>failed to verify spend key secret key</source>
        <translation>秘密なスペンドキーの検証に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="581"/>
        <source>Electrum-style word list failed verification</source>
        <translation>Electrumな単語表の検証に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="601"/>
        <source>At least one of either an Electrum-style word list, private view key, or private spend key must be specified</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="605"/>
        <source>Both Electrum-style word list and private key(s) specified</source>
        <translation>Electrumな単語表と秘密なキーを指定しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="615"/>
        <source>invalid address</source>
        <translation>不正なアドレス</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="624"/>
        <source>view key does not match standard address</source>
        <translation>ビューキーが一般的なアドレスと一致しませんでした</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="634"/>
        <source>spend key does not match standard address</source>
        <translation>スペンドキーが一般的なアドレスと一致しませんでした</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="642"/>
        <source>Cannot generate deprecated wallets from JSON</source>
        <translation>JSONで非推奨のウォレットを生成することはできません</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="678"/>
        <source>failed to parse address: </source>
        <translation>アドレスの解析に失敗しました： </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="684"/>
        <source>Address must be specified in order to create watch-only wallet</source>
        <translation>閲覧専用ウォレットを作るためにアドレスを指定する必要があります</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="701"/>
        <source>failed to generate new wallet: </source>
        <translation>新しいウォレットの生成に失敗しました： </translation>
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
        <translation>プライマリア カウント</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="10885"/>
        <source>No funds received in this tx.</source>
        <translation>この取引から資金貰いませんでした。</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet2.cpp" line="11645"/>
        <source>failed to read file </source>
        <translation>ファイルの読み込みに失敗しました </translation>
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
        <translation>ディレクトリの作成に失敗しました </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="194"/>
        <source>Failed to create directory %s: %s</source>
        <translation>%s %s ディレクトリの作成に失敗しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="205"/>
        <source>Cannot specify --</source>
        <translation>これを指定しません： --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="205"/>
        <source> and --</source>
        <translation> と --</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="224"/>
        <source>Failed to create file </source>
        <translation>ファイルの作成に失敗しました </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="224"/>
        <source>. Check permissions or remove file</source>
        <translation>。 パーミッションを確認するか、ファイルを削除してください</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="234"/>
        <source>Error writing to file </source>
        <translation>ファイルへの書き込みエラー </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="237"/>
        <source>RPC username/password is stored in file </source>
        <translation>RPCユーザー名/パスワードはファイルに保存しました </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="613"/>
        <source>Tag %s is unregistered.</source>
        <translation>タグ %s を登録してません。</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="3242"/>
        <source>Transaction not possible. Available only %s, transaction amount %s = %s + %s (fee)</source>
        <translation>取引は無理です。利用可能な金額 %s、 取引の金額 %s = %s + %s (手数料)</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4409"/>
        <source>This is the RPC monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation>これはMoneroのRPCウォレットです。正しく動作させるには
Moneroデーモンに接続する必要があります。</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4245"/>
        <source>Can&apos;t specify more than one of --wallet-file and --generate-from-json</source>
        <translation>--wallet-file と --generate-from-json を1つしか指定しません</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4230"/>
        <source>Can&apos;t specify more than one of --testnet and --stagenet</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4257"/>
        <source>Must specify --wallet-file or --generate-from-json or --wallet-dir</source>
        <translation>--wallet-file や --generate-from-json や --wallet-dir を指定する必要があります</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4261"/>
        <source>Loading wallet...</source>
        <translation>ウォレットをロードしてます...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4295"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4327"/>
        <source>Saving wallet...</source>
        <translation>ウォレットを保存してます...</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4297"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4329"/>
        <source>Successfully saved</source>
        <translation>保存しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4300"/>
        <source>Successfully loaded</source>
        <translation>ロードしました</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4304"/>
        <source>Wallet initialization failed: </source>
        <translation>ウォレットを初期化できませんでした: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4310"/>
        <source>Failed to initialize wallet RPC server</source>
        <translation>ウォレットのRPCサーバを初期化できませんでした</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4314"/>
        <source>Starting wallet RPC server</source>
        <translation>ウォレットのRPCサーバを開始してます</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4321"/>
        <source>Failed to run wallet: </source>
        <translation>ウォレットを起動することできませんでした： </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4324"/>
        <source>Stopped wallet RPC server</source>
        <translation>ウォレットのRPCサーバを停止しました</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4333"/>
        <source>Failed to save wallet: </source>
        <translation>ウォレットを保存することできませんでした： </translation>
    </message>
</context>
<context>
    <name>wallet_args</name>
    <message>
        <location filename="../src/gen_multisig/gen_multisig.cpp" line="168"/>
        <location filename="../src/wallet/wallet_rpc_server.cpp" line="4385"/>
        <location filename="../src/simplewallet/simplewallet.cpp" line="9348"/>
        <source>Wallet options</source>
        <translation>ウォレットのオプション</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="73"/>
        <source>Generate wallet from JSON format file</source>
        <translation>JSONフォーマットファイルでウォレットを生じてください</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="77"/>
        <source>Use wallet &lt;arg&gt;</source>
        <translation>ウォレットの &lt;arg&gt; を使てください</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="105"/>
        <source>Max number of threads to use for a parallel job</source>
        <translation>並列ジョブのために最大スレッドの数</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="106"/>
        <source>Specify log file</source>
        <translation>ログファイルを指定してください</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="107"/>
        <source>Config file</source>
        <translation>設定ファイル</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="119"/>
        <source>General options</source>
        <translation>ジェネラルオプション</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="144"/>
        <source>This is the command line monero wallet. It needs to connect to a monero
daemon to work correctly.</source>
        <translation>これはMoneroのコマンドラインウォレットです。正しく動作させるには
Moneroデーモンに接続する必要があります。</translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="169"/>
        <source>Can&apos;t find config file </source>
        <translation>設定ファイルを見つかりませんでした </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="210"/>
        <source>Logging to: </source>
        <translation>こっちにログをしてます: </translation>
    </message>
    <message>
        <location filename="../src/wallet/wallet_args.cpp" line="212"/>
        <source>Logging to %s</source>
        <translation>%s にログをしてます</translation>
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
        <translation>使用：</translation>
    </message>
</context>
</TS>
