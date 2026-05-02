Ray 構文リファレンス
=================

`Ray` は `Noct` プログラミング言語に Suika3 API を追加したものです。

## 代入 (Assignments)

Noct の変数は動的型付けであり、明示的な型宣言はありません。代入演算子 (`=`) を使用して変数を作成し、値を割り当てます。

以下の例に示すように、Noct は整数、浮動小数点数、文字列を含むさまざまなデータ型をサポートしています。変数は実行中の任意の時点で異なる型に再代入できます。

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

## グローバル変数 (Global Variables)

グローバル変数は関数内で定義でき、関数の外では定義できません。

```
func main() {
    globalVariable = 123;
    print(globalVariable);
}
```

## ローカル変数 (Local Variables)

`var` キーワードを使用すると、変数をローカルとして宣言できます。`var` 宣言がない場合、変数への代入によってグローバル変数が作成されます。

```
func main() {
    var a = 123;
    print(a);
}
```

## 配列 (Array)

配列はインデックスによってアクセスされる値の順序付きコレクションです。配列は `for` ループ構造による反復をサポートしており、各値に直接アクセスできます。

```
func main() {
    var array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

配列は、動的型付けシステムを反映して、異なる型の値を同時に保持できます。

```
func main() {
    var array = [123, "string"];
}
```

この言語は、配列の末尾に要素を追加するための組み込み関数 `push()` を提供しています。また、`pop()` は最後の要素を削除します。

```
func main() {
    var array = []
    array->push(0);
    array->push(1);
    array->push(2);

    var last = array->pop();

    print("Length = " + array.length);
}
```

## 辞書 (Dictionary)

辞書は、他の言語のハッシュマップやオブジェクトと同様に、キーと値のペアを格納します。中括弧を使用し、キーと値をコロンで区切って定義します。辞書は、キーと値の両方に同時にアクセスできる反復をサポートしています。

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print("key = " + key);
        print("value = " + value);
    }

    print("Length = " + dict.length);
}
```

辞書は、段階的に構築することもできます。代入は、`[]` を使用する配列スタイル、または `.` を使用するオブジェクトスタイルのいずれかで行えます。

```
func main() {
    var dict = {};
    dict["key1"] = "value1";
    dict.key2 = "value2";
}
```

組み込み関数 `remove()` を使用すると、キーによってエントリを削除できます。

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    remove(dict, "key1");
}
```

## For ループ (For-loop)

for ループ構造は、範囲 (range)、配列、辞書などのシーケンスを反復処理するための簡潔な構文を提供します。

範囲構文 (`..` 演算子を使用) は、開始値から終了値の 1 つ手前までの値を生成するイテレータを作成します。

```
func main() {
    for (i in 0..10) {
        print(i);
    }
}
```

for ループは、配列やその他のコレクション型を直接反復処理することもできます。

配列は for-value 構文で反復処理できます。

```
func main() {
    array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

辞書は for-key-value 構文で反復処理できます。

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print(key + "=" + value);
    }
}
```

## While ループ (While Loops)

while ループは、指定された条件が真である限り実行を継続する伝統的な反復メカニズムを提供します。コレクションの反復処理に特化した for ループとは異なり、while ループはより柔軟で、反復回数が事前に分からないさまざまなアルゴリズムの実装に使用できます。例では、0 から 9 まで増加する基本的なカウンタの実装を示しています。

```
func main() {
    var i = 0;
    while (i < 10) {
        print(i);
        i = i + 1;
    }
}
```

## If および Else ブロック (If and Else Blocks)

制御フローにより、評価された式に基づいて条件付き実行が可能になります。if-else 構造は、条件が順番に評価されるおなじみの構文に従います。

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

## ラムダ関数 (Lambda Functions)

この言語において、関数は第一級オブジェクトです。匿名関数 (ラムダ式) を使用すると、名前のない関数を作成できます。

```
func main() {
    var f = (a, b) => { return a + b; }
    print(f(1, 2));
}
```

ラムダ関数は、コンパイルプロセス中に単純に名前付き関数に変換されます。そのため、外側の関数で宣言された変数をキャプチャすることはできません。

## インクリメント/デクリメント (+=, -=, ++, --)

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

`++` および `--` は、スタンドアロンのステートメント (`a++;`, `b--;`) としてのみサポートされます。複雑な副作用を避けるため、式の内部で使用することは禁止されています。

## Noct における OOP (OOP in Noct)

Noct のオブジェクト指向モデルは、プロトタイプベースの OOP を軽量化したものです。

- クラスは単なる辞書のテンプレートです
- 継承とインスタンス化は、辞書の結合 (merge) によって実現されます
- プロトタイプチェーンはなく、クラスを変更しても既存のインスタンスには影響しません

この設計では辞書を第一級オブジェクトとして扱い、著者はこれを辞書ベースの OOP (D-OOP) と呼んでいます。

```
func main() {
    // 基本クラスの定義 (クラスは単なる辞書です)
    Animal = class {
        name: "Animal",
        cry: (this) => {
        }
    };

    // サブクラスの定義 (単なる辞書の結合)
    Cat = extend Animal {
        name: "Cat",
        voice: "meow",
        cry: (this) => {
            print(this.name + " cries like " + this.voice);
        }
    };

    // インスタンス化 (単なる辞書の結合)
    var myCat = new Cat {
        voice: "neee"
    };

    // This-call は -> () 構文を使用します (myCat.cry(myCat) と等価です)
    myCat->cry();
}
```

---

## 組み込み関数 (Intrinsics)

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
var s2 = s1.substring(0, 3); // 文字 0 から 3 文字
var s3 = s1.substring(2, -1); // 文字 1 から末尾まで
}
```

### unset()

```
var dic = {key1: "ABC"};
dic->unset("key1");
```

---

## 数学 API (Math API)

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
