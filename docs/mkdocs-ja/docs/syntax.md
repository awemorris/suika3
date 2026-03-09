Playfield Script の文法
=======================

`Playfield Engine` はスクリプト言語として `Noct` を使用します。

## 代入

Noct の変数は動的型付けであり、明示的な宣言は不要です。
代入演算子（`=`）を使って、変数を作成し値を代入します。

以下のように、Noct では整数、浮動小数点数、文字列など、さまざまなデータ型をサポートしています。

また、変数は実行中に異なる型へ再代入することが可能です。

```
func main() {
    var a = 123;
    print(a);

    var b = 1.0;
    print(b);

    var c = "string";
    print(c);
}
```

## グローバル変数

グローバル変数は関数の中で宣言します。関数の外に書くことはできません。

```
func main() {
    globalVariable = 123;
    print(globalVariable);
}
```

## ローカル変数

`var` キーワードを使うと、変数をローカル変数として宣言できます。
`var` を付けずに代入すると、その変数はグローバル変数として作成されます。

```
func main() {
    var a = 123;
    print(a);
}
```

## 配列

配列はインデックスでアクセスできる、順序付きの値のコレクションです。
配列は `for` ループ構文を使った反復処理をサポートしており、各要素を直接順番に処理できます。

```
func main() {
    var array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

Noct は動的型付けシステムであるため、配列は異なる型の値を同時に保持できます。

```
func main() {
    var array = [123, "string"];
}
```

Noct には配列の末尾に要素を追加するための組み込み関数 `push()` が用意されています。
また、末尾の要素は `pop()` で削除できます。

```
func main() {
    var array = []
    array->push(0);
    array->push(1);
    array->push(2);

    var last = array->pop();
}
```

## 辞書

辞書はキーと値のペアを格納するデータ構造で、他の言語におけるハッシュマップやオブジェクトに似ています。
中括弧 `{}` を使って定義し、キーと値のペアはコロンで区切ります。

辞書は反復処理をサポートしており、キーと値の両方に同時にアクセスできます。

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print("key = " + key);
        print("value = " + value);
    }

}
```

辞書はシングルステップ方式で構築することもできます。
代入の方法としては、`[]` を使う配列スタイルと、`.` を使うオブジェクトスタイルの両方が利用できます。

```
func main() {
    var dict = {};
    dict["key1"] = "value1";
    dict.key2 = "value2";
}
```

ビルトイン関数 `remove()` はキーを削除できます。

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    remove(dict, "key1");
}
```

## for ループ

`for` ループ構文は、範囲・配列・辞書といったシーケンスを反復処理するための簡潔な記法を提供します。

範囲構文（`..` 演算子を使用）は、開始値から終了値のひとつ前までの値を生成するイテレータを作成します。

```
func main() {
    for (i in 0..10) {
        print(i);
    }
}
```

`for` ループは配列やその他のコレクション型を直接反復処理することもできます。

配列は `for-value` 構文 を使って順に処理できます。

```
func main() {
    array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

辞書は `for-key-value` 構文 を使って反復処理できます。

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print(key + "=" + value);
    }
}
```

## while ループ

`while` ループは、指定した条件が真（true）である限り処理を繰り返す、古典的な反復構文です。
コレクションの反復処理を目的とした `for` ループとは異なり、`while` ループはより柔軟で、事前に反復回数がわからないアルゴリズムを実装するときなどに利用できます。

以下の例では、0 から 9 までカウントアップする基本的なカウンターの実装を示しています。

```
func main() {
    var i = 0;
    while (i < 10) {
        print(i);
        i = i + 1;
    }
}
```

## if / else ブロック

条件式を評価してブロックを条件付きで実行することができます。
`if-else` 構文はおなじみの書式で、条件が順に評価されていきます。

```
func main() {
    var a = readint();
    if (a == 0) {
        print("0");
    } else if (a == 1) {
        print("1");
    } else {
        print("other");
    }
}
```

## ラムダ関数

Noct では、関数は第一級オブジェクトとして扱われます。
匿名関数（lambda 式とも呼ばれる）を使うことで、名前を付けずに関数を作成できます。

```
func main() {
    var f = (a, b) => { return a + b; }
    print(f(1, 2));
}
```

ラムダ関数は、コンパイル時に単純に名前付き関数へ変換されます。
そのため、外側の関数で宣言された変数をキャプチャすることはできません。

## インクリメント / デクリメント (+=, -=, ++, --)

```
func main() {
    var a = 123;
    a += 321;
    a++;

    var b = 123;
    b -= 321;
    b--;
}
```

`++` と `--` は、単独の文（`a++;`, `b--;`）としてのみサポートされています。
式の中で使用することは禁止されており、これは複雑な副作用を避けるための仕様です。

## オブジェクト指向

Noct のオブジェクト指向モデルは、プロトタイプベース OOP の軽量なバリエーションです。

- クラスは単なる辞書のテンプレートです
- 継承やインスタンス化は、辞書の複製とマージによって実現されます
- プロトタイプチェーンは存在せず、クラスを変更しても既存のインスタンスには影響しません

辞書は第一級オブジェクトとして扱われ、Dictionary-based OOP（辞書ベースOOP）と呼ばれます。

```
func main() {
    // 基底クラスの定義 (クラスの実体はただの辞書です)
    Animal = class {
        name: "Animal",
        cry: (this) => {
        }
    };

    // サブクラスの定義 (辞書のマージにすぎません)
    Cat = extend Animal {
        name: "Cat",
        voice: "meow",
        cry: (this) => {
            print(this.name + " cries like " + this.voice);
        }
    };

    // インスタンス化 (これも辞書のマージです)
    var myCat = new Cat {
        voice: "neee"
    };

    // this渡し呼び出しには -> () 構文を使います (myCat.cry(myCat) と等価です)
    myCat->cry();
}
```

---

## 組み込み関数

### int()

```
var i = int(1.23);
```

### float()

```
var f = float(123);
```

### newArray()

```
var array = newArray(10);
```

### push()

```
var array = [1, 2, 3];
array->push(4);
```

### pop()

```
var array = [1, 2, 3];
var last = array->pop();
```

### resize()

```
var array = [1, 2, 3];
array->resize(2);
```

### charCount()

```
var s = "ABC文あいう";
var l = s->charCount();
```

### charAt()

```
var s = "ABC文あいう";
for (i in 0 .. s.length) {
   var c = s->charAt(i);
   print(c);
}
```

### substring()

```
var s1 = "ABCDEFG";
var s2 = s1.substring(0, 3); // from the char 0, three characters
var s3 = s1.substring(2, -1); // from the char 1, to the end
}
```

### unset()

```
var dic = {key1: "ABC"};
dic->unset("key1");
```

---

## Math API

## Math

### abs()

```
var a = abs(x);
```

### random()

```
var r = random(); // 0 .. 1.0
```

### Math.sin()

```
var y = sin(x);
```

### Math.cos()

```
var y = cos(x);
```

### Math.tan()

```
var y = tan(x);
```
