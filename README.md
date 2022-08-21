OpenVR Input to VRChat OSC Bridge
====


## これは何?
OpenVRで扱えるコントローラー・トラッカーなどの入力をトリガーとしてOSCメッセージを発行するための汎用ツールです。
VRChatと直接的な依存関係はありませんが基本的にはVRChatのOSC機能との連携を念頭に置いて設計されています。


## ビルド
Visual Studio 2022 でソリューションファイルを開いてビルドしてください。
また、vcpkgであらかじめ以下のパッケージをインストールしておく必要があります。

- `boost:x64-windows`
- `openvr:x64-windows`
- `nlohmann-json:x64-windows`


## 設定

### 設定ファイルの構成
リリースパッケージのアーカイブを展開すると以下のようなファイル構成となっています。

```
+ OVR-VRC-OSC-Bridge
    + action_sets
        + VRChat.json
        + VirtualLens2.json
    + OVR-VRC-OSC-Bridge.exe
    + default.json
```

`action_sets` フォルダの中に後述するアクションセット定義ファイルを配置し、アプリケーション全体の設定は `default.json` に記述します。

### アクションセットの定義
以下にアクションセットの定義の例を示します。

```javascript
{
  "id": "example",
  "name": "Example Action Set",
  "actions": [
    {
      "id": "analog",
      "name": "Analog Action",
      "analog": [
        {
          "key": "/avatar/parameters/AnalogInput",
          "input_min": -1.0,
          "input_max":  1.0,
          "output_min": 0.0,
          "output_max": 1.0
        }
      ]
    },
    {
      "id": "binary",
      "name": "Binary Action",
      "binary": {
        "press": [ { "key": "/avatar/parameters/Binary", "value": 1 } ],
        "release": [ { "key": "/avatar/parameters/Binary", "value": 0 } ]
      }
    },
    {
      "id": "rotate",
      "name": "Rotate Action",
      "rotate": [
        {
          "enter": [ { "key": "/avatar/parameters/Rotate0", "value": true } ],
          "exit": [ { "key": "/avatar/parameters/Rotate0", "value": false } ]
        },
        {
          "enter": [ { "key": "/avatar/parameters/Rotate1", "value": true } ],
          "exit": [ { "key": "/avatar/parameters/Rotate1", "value": false } ]
        },
        {
          "enter": [ { "key": "/avatar/parameters/Rotate2", "value": true } ],
          "exit": [ { "key": "/avatar/parameters/Rotate2", "value": false } ]
        }
      ]
    },
  ]
}
```

- `id`: OpenVR内でのアクションセットの識別に使用されるIDを指定します。
- `name`: OpenVRの設定UI上での表示に使用される文字列を指定します。
- `actions`: このアクションセット内に含まれるアクションのリストを指定します。アクションに定義については次の節をご参照ください。

#### アクションの定義

##### アナログアクション
アクション定義が要素 `analog` を持つ場合、そのアクションは1軸アナログ入力 (`vector1`) として扱われます。
入力値の範囲 `[input_min, input_max]` を出力値の範囲 `[output_min, output_max]` に変換して出力します。

##### バイナリアクション
アクション定義が要素 `binary` を持つ場合、そのアクションは2値のデジタル入力として扱われます。
ボタンが押されたタイミングでリスト `press` に指定された0個以上のキーと値の組からなるメッセージを発行し、ボタンが離されたタイミングで `release` に指定されたキーと値の組からなるメッセージが発行されます。

##### ローテートアクション
アクション定義が要素 `rotate` を持つ場合、そのアクションは複数の状態をローテートする入力として扱われます。
対応するボタンが離されたタイミングで内部の状態を1つ次の状態に遷移させ、遷移元の `exit` に対応するメッセージと遷移先の `enter` に対応するメッセージが発行されます。