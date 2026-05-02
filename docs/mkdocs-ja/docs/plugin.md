Rayプラグイン開発リファレンス
================================

## フォルダ

プラグインは `system/plugin/<プラグイン名>/` ディレクトリに保存する必要があります。

## ファイル

プラグインファイルは `system/plugin/<プラグイン名>/<プラグイン名>.ray` ファイルに保存する必要があります。

## 関数

プラグインは `plugin_init_<プラグイン名>()` 関数を定義する必要があります。

## 新しいタグの定義

`system/plugin/<プラグイン名>/<プラグイン名>.ray` ファイルに `Tag_mytag()` という名前の関数を定義することで、`mytag` という名前の新しいタグを作成できます。
`Suika.loadPlugin()` でプラグインをロードした後、NovelML で `mytag` を使用できます。

## サンプル

`system/plugin/testplugin/testplugin.ray` 内:
```
func plugin_init_testplugin() {
    // ロード時に呼び出される
    print("Plugin is loaded.");
}

// 新しいタグ
func Tag_testplugintag(params) {
    print("Plugin tag is called.");
    print("parameter: " + params.text);

    Suika.moveToNextTag();
}
```

`main.ray` 内:
```
// ゲーム開始前に呼び出される
func start() {
    // 以下の行は削除しないでください
    Suika.start();

    Suika.loadPluin("testplugin");
}
```

`start.novel` 内でのタグ呼び出し:

```
[testplugintag text="hello"]
```
