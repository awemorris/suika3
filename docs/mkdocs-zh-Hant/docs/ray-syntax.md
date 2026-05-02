Ray 語法參考
====================

`Ray` 其實就是加上 Suika3 API 的 `Noct` 程式語言。

## 指派

Noct 的變數是動態型別，不需要明確宣告。指派運運算元（`=`）用來建立並設定變數值。

如下例所示，Noct 支援多種資料型別，包括整數、浮點數與字串。變數在執行過程中可以隨時重新指派成不同型別。

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

## 全域變數

全域變數可以在函式中定義，不能在函式外定義。

```
func main() {
    globalVariable = 123;
    print(globalVariable);
}
```

## 區域變數

使用 `var` 關鍵字可以把變數宣告為區域變數。
如果不使用 `var` 宣告，直接對變數賦值就會建立全域變數。

```
func main() {
    var a = 123;
    print(a);
}
```

## 陣列

陣列是按索引存取的有序值集合。陣列支援 `for` 迴圈迭代，因此可以直接逐一走訪每個值。

```
func main() {
    var array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

陣列可以同時容納不同型別的值，反映其動態型別系統。

```
func main() {
    var array = [123, "string"];
}
```

語言提供內建函式 `push()`，可以把元素加到陣列尾端。另外，`pop()` 會移除最後一個元素。

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

## 字典

字典會儲存鍵值對，類似其他語言中的雜湊表或物件。它使用大括號定義，鍵值對以冒號分隔。字典支援迭代，且可以同時取得鍵與值。

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

字典也可以用一步完成的方式建立。賦值可以是使用 `[]` 的陣列式寫法，或使用 `.` 的物件式寫法。

```
func main() {
    var dict = {};
    dict["key1"] = "value1";
    dict.key2 = "value2";
}
```

內建函式 `remove()` 可以根據鍵刪除專案。

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    remove(dict, "key1");
}
```

## for 迴圈

for 迴圈提供簡潔語法，可用來走訪範圍、陣列與字典等序列。

範圍語法（使用 `..` 運運算元）會建立一個迭代器，產生從起始值到結束值前一個數字的值。

```
func main() {
    for (i in 0..10) {
        print(i);
    }
}
```

for 迴圈也可以直接迭代陣列與其他集合型別。

陣列可以使用 for-value 語法迭代。

```
func main() {
    array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

字典可以使用 for-key-value 語法迭代。

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print(key + "=" + value);
    }
}
```

## while 迴圈

while 迴圈提供傳統的迭代機制，只要指定條件維持為真就會持續執行。與專為集合迭代設計的 for 迴圈不同，while 迴圈更有彈性，可用於實作各種無法事先知道迭代次數的演演算法。以下範例示範一個基本計數器，從 0 遞增到 9。

```
func main() {
    var i = 0;
    while (i < 10) {
        print(i);
        i = i + 1;
    }
}
```

## if 與 else 區塊

控制流程可根據運算後的表示式進行條件式執行。if-else 結構使用熟悉的語法，條件會依序進行判斷。

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

## Lambda 函式

函式在這個語言中是第一級物件。匿名函式，也就是 `lambda` 表示式，可以讓你在沒有名稱的情況下建立函式。

```
func main() {
    var f = (a, b) => { return a + b; }
    print(f(1, 2));
}
```

Lambda 函式在編譯過程中只會被轉成具名函式。因此，它們無法捕捉外層函式中宣告的變數。

## 遞增/遞減（+=, -=, ++, --）

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

`++` 與 `--` 只支援作為獨立敘述（`a++;`、`b--;`）。
為了避免複雜的副作用，不允許把它們放在表示式內使用。

## Noct 的 OOP

Noct 的物件導向模型是一種輕量級、基於原型的 OOP 變體。

- 類別本質上就是字典樣板
- 繼承與實體化是透過字典合併實現
- 沒有原型鏈，而且修改類別不會影響既有例項

這種設計把字典視為第一級物件，作者稱之為基於字典的 OOP（D-OOP）。

```
func main() {
    // 基底類別定義。（類別其實就是字典。）
    Animal = class {
        name: "Animal",
        cry: (this) => {
        }
    };

    // 子類別定義。（其實只是字典合併。）
    Cat = extend Animal {
        name: "Cat",
        voice: "meow",
        cry: (this) => {
            print(this.name + " cries like " + this.voice);
        }
    };

    // 實體化。（其實只是字典合併。）
    var myCat = new Cat {
        voice: "neee"
    };

    // this-call 使用 -> () 語法。（等同於 myCat.cry(myCat)）
    myCat->cry();
}
```

---

## 內建函式

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
var s = "ABC漢語";
var l = s->charCount();
```

### charAt()

```
var s = "ABC漢語";
for (i in 0 .. s.length) {
   var c = s->charAt(i);
   print(c);
}
```

### substring()

```
var s1 = "ABCDEFG";
var s2 = s1.substring(0, 3); // 從字元 0 起算，共三個字元
var s3 = s1.substring(2, -1); // 從字元 1 起，直到結尾
}
```

### unset()

```
var dic = {key1: "ABC"};
dic->unset("key1");
```

---

## 數學 API

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
