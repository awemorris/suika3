Ray 语法参考
====================

`Ray` 其实就是加上 Suika3 API 的 `Noct` 程式语言。

## 指派

Noct 的变数是动态型别，不需要明确宣告。指派运运算元（`=`）用来建立并设定变数值。

如下例所示，Noct 支援多种资料型别，包括整数、浮点数与字串。变数在执行过程中可以随时重新指派成不同型别。

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

## 全域变数

全域变数可以在函式中定义，不能在函式外定义。

```
func main() {
    globalVariable = 123;
    print(globalVariable);
}
```

## 区域变数

使用 `var` 关键字可以把变数宣告为区域变数。
如果不使用 `var` 宣告，直接对变数赋值就会建立全域变数。

```
func main() {
    var a = 123;
    print(a);
}
```

## 阵列

阵列是按索引存取的有序值集合。阵列支援 `for` 回圈迭代，因此可以直接逐一走访每个值。

```
func main() {
    var array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

阵列可以同时容纳不同型别的值，反映其动态型别系统。

```
func main() {
    var array = [123, "string"];
}
```

语言提供内建函式 `push()`，可以把元素加到阵列尾端。另外，`pop()` 会移除最后一个元素。

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

字典会储存键值对，类似其他语言中的杂凑表或物件。它使用大括号定义，键值对以冒号分隔。字典支援迭代，且可以同时取得键与值。

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

字典也可以用一步完成的方式建立。赋值可以是使用 `[]` 的阵列式写法，或使用 `.` 的物件式写法。

```
func main() {
    var dict = {};
    dict["key1"] = "value1";
    dict.key2 = "value2";
}
```

内建函式 `remove()` 可以根据键删除专案。

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    remove(dict, "key1");
}
```

## for 回圈

for 回圈提供简洁语法，可用来走访范围、阵列与字典等序列。

范围语法（使用 `..` 运运算元）会建立一个迭代器，产生从起始值到结束值前一个数字的值。

```
func main() {
    for (i in 0..10) {
        print(i);
    }
}
```

for 回圈也可以直接迭代阵列与其他集合型别。

阵列可以使用 for-value 语法迭代。

```
func main() {
    array = [0, 1, 2];
    for (value in array) {
        print(value);
    }
}
```

字典可以使用 for-key-value 语法迭代。

```
func main() {
    var dict = {key1: "value1", key2: "value2"};
    for (key, value in dict) {
        print(key + "=" + value);
    }
}
```

## while 回圈

while 回圈提供传统的迭代机制，只要指定条件维持为真就会持续执行。与专为集合迭代设计的 for 回圈不同，while 回圈更有弹性，可用于实作各种无法事先知道迭代次数的演演算法。以下范例示范一个基本计数器，从 0 递增到 9。

```
func main() {
    var i = 0;
    while (i < 10) {
        print(i);
        i = i + 1;
    }
}
```

## if 与 else 区块

控制流程可根据运算后的表示式进行条件式执行。if-else 结构使用熟悉的语法，条件会依序进行判断。

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

函式在这个语言中是第一级物件。匿名函式，也就是 `lambda` 表示式，可以让你在没有名称的情况下建立函式。

```
func main() {
    var f = (a, b) => { return a + b; }
    print(f(1, 2));
}
```

Lambda 函式在编译过程中只会被转成具名函式。因此，它们无法捕捉外层函式中宣告的变数。

## 递增/递减（+=, -=, ++, --）

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

`++` 与 `--` 只支援作为独立叙述（`a++;`、`b--;`）。
为了避免复杂的副作用，不允许把它们放在表示式内使用。

## Noct 的 OOP

Noct 的物件导向模型是一种轻量级、基于原型的 OOP 变体。

- 类别本质上就是字典样板
- 继承与实体化是透过字典合并实现
- 没有原型链，而且修改类别不会影响既有例项

这种设计把字典视为第一级物件，作者称之为基于字典的 OOP（D-OOP）。

```
func main() {
    // 基底类别定义。（类别其实就是字典。）
    Animal = class {
        name: "Animal",
        cry: (this) => {
        }
    };

    // 子类别定义。（其实只是字典合并。）
    Cat = extend Animal {
        name: "Cat",
        voice: "meow",
        cry: (this) => {
            print(this.name + " cries like " + this.voice);
        }
    };

    // 实体化。（其实只是字典合并。）
    var myCat = new Cat {
        voice: "neee"
    };

    // this-call 使用 -> () 语法。（等同于 myCat.cry(myCat)）
    myCat->cry();
}
```

---

## 内建函式

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
var s = "ABC汉语";
var l = s->charCount();
```

### charAt()

```
var s = "ABC汉语";
for (i in 0 .. s.length) {
   var c = s->charAt(i);
   print(c);
}
```

### substring()

```
var s1 = "ABCDEFG";
var s2 = s1.substring(0, 3); // 从字元 0 起算，共三个字元
var s3 = s1.substring(2, -1); // 从字元 1 起，直到结尾
}
```

### unset()

```
var dic = {key1: "ABC"};
dic->unset("key1");
```

---

## 数学 API

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
