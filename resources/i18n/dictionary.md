# NoctLang translation dictionary

This glossary defines the preferred translations for recurring NoctLang terms.
English is the source language. Translators should use the listed term unless the
grammar of a complete sentence requires inflection or a different word order.

## Rules

- Do not translate code identifiers or command-line options, including `noct`,
  `__fast`, `Packed`, `rpacked`, `int`, `long`, `float`, and `double`.
- Keep technology and API names unchanged: NoctLang, C, Emacs Lisp, Scheme, JIT,
  VM, GC, GPU, SIMD, HIR, LIR, Vulkan, SPIR-V, OpenGL ES, and Direct3D 12.
- Preserve all printf placeholders exactly, including their order and modifiers
  (`%s`, `%d`, `%ld`, `%zu`, and so on). Preserve escaped characters such as `\n`.
- Translate *argument* and *parameter* distinctly. An argument is supplied at a
  call site; a parameter belongs to a function declaration.
- In accelerator diagnostics, *host* means the CPU side and *device* means the
  GPU side. Do not translate either as a network host or a generic appliance.
- `Packed` and `rpacked` are NoctLang type names. The lowercase word *packed* is
  also kept as `packed` when it refers to those types.
- The `zh` locale uses Simplified Chinese; `tw` uses Traditional Chinese.

## Languages

| Code | Language |
|---|---|
| `en` | English (source) |
| `es` | Spanish |
| `fr` | French |
| `de` | German |
| `it` | Italian |
| `el` | Greek |
| `ru` | Russian |
| `zh` | Simplified Chinese |
| `tw` | Traditional Chinese |
| `ja` | Japanese |

## Preferred terms: Western European languages

| English | Spanish (`es`) | French (`fr`) | German (`de`) | Italian (`it`) |
| --- | --- | --- | --- | --- |
| accelerator | acelerador | accélérateur | Beschleuniger | acceleratore |
| backend | backend | backend | Backend | backend |
| virtual machine | máquina virtual | machine virtuelle | virtuelle Maschine | macchina virtuale |
| compiler | compilador | compilateur | Compiler | compilatore |
| interpreter | intérprete | interpréteur | Interpreter | interprete |
| JIT compilation | compilación JIT | compilation JIT | JIT-Kompilierung | compilazione JIT |
| source code | código fuente | code source | Quellcode | codice sorgente |
| bytecode | bytecode | bytecode | Bytecode | bytecode |
| module | módulo | module | Modul | modulo |
| program | programa | programme | Programm | programma |
| application | aplicación | application | Anwendung | applicazione |
| runtime | entorno de ejecución | environnement d’exécution | Laufzeitumgebung | ambiente di esecuzione |
| execution | ejecución | exécution | Ausführung | esecuzione |
| input | entrada | entrée | Eingabe | input |
| output | salida | sortie | Ausgabe | output |
| result | resultado | résultat | Ergebnis | risultato |
| memory | memoria | mémoire | Speicher | memoria |
| function | función | fonction | Funktion | funzione |
| intrinsic | función intrínseca | fonction intrinsèque | intrinsische Funktion | funzione intrinseca |
| call | llamada | appel | Aufruf | chiamata |
| caller | llamador | fonction appelante | Aufrufer | chiamante |
| callee | función llamada | fonction appelée | aufgerufene Funktion | funzione chiamata |
| argument | argumento | argument | Argument | argomento |
| parameter | parámetro | paramètre | Parameter | parametro |
| return value | valor de retorno | valeur de retour | Rückgabewert | valore restituito |
| value | valor | valeur | Wert | valore |
| variable | variable | variable | Variable | variabile |
| local variable | variable local | variable locale | lokale Variable | variabile locale |
| global variable | variable global | variable globale | globale Variable | variabile globale |
| symbol | símbolo | symbole | Symbol | simbolo |
| type | tipo | type | Typ | tipo |
| primitive type | tipo primitivo | type primitif | primitiver Typ | tipo primitivo |
| element | elemento | élément | Element | elemento |
| element type | tipo de elemento | type d’élément | Elementtyp | tipo di elemento |
| size | tamaño | taille | Größe | dimensione |
| count | cantidad | nombre | Anzahl | numero |
| limit | límite | limite | Grenze | limite |
| array | array | tableau | Array | array |
| dictionary | diccionario | dictionnaire | Dictionary | dizionario |
| string | cadena | chaîne de caractères | Zeichenkette | stringa |
| integer | entero | entier | ganze Zahl | intero |
| floating-point number | número de coma flotante | nombre à virgule flottante | Gleitkommazahl | numero in virgola mobile |
| index | índice | index | Index | indice |
| subscript | subíndice | indice | Indexausdruck | pedice |
| range | rango | plage | Bereich | intervallo |
| out of range | fuera de rango | hors limites | außerhalb des Bereichs | fuori intervallo |
| buffer | búfer | tampon | Puffer | buffer |
| device buffer | búfer del dispositivo | tampon du périphérique | Gerätepuffer | buffer del dispositivo |
| host buffer | búfer del host | tampon de l’hôte | Host-Puffer | buffer dell’host |
| scalar | escalar | scalaire | Skalar | scalare |
| scalar result | resultado escalar | résultat scalaire | skalares Ergebnis | risultato scalare |
| packed value | valor `packed` | valeur `packed` | `packed`-Wert | valore `packed` |
| shape | forma | forme | Form | forma |
| rank (number of dimensions) | número de dimensiones | nombre de dimensions | Dimensionsanzahl | numero di dimensioni |
| extent (dimension size) | extensión | étendue | Ausdehnung | estensione |
| binding | vinculación | liaison | Bindung | associazione |
| slot | ranura | emplacement | Slot | slot |
| metadata | metadatos | métadonnées | Metadaten | metadati |
| kernel | kernel | kernel | Kernel | kernel |
| dispatch | despacho | lancement | Dispatch | dispatch |
| workgroup | grupo de trabajo | groupe de travail | Arbeitsgruppe | gruppo di lavoro |
| shader | shader | shader | Shader | shader |
| compute shader | shader de cómputo | shader de calcul | Compute-Shader | compute shader |
| shader module | módulo de shader | module de shader | Shader-Modul | modulo shader |
| pipeline | pipeline | pipeline | Pipeline | pipeline |
| command buffer | búfer de comandos | tampon de commandes | Befehlspuffer | buffer dei comandi |
| device | dispositivo | périphérique | Gerät | dispositivo |
| context | contexto | contexte | Kontext | contesto |
| session | sesión | session | Sitzung | sessione |
| snapshot | instantánea | instantané | Momentaufnahme | istantanea |
| producer | productor | producteur | Erzeuger | produttore |
| consumer | consumidor | consommateur | Verbraucher | consumatore |
| reduction | reducción | réduction | Reduktion | riduzione |
| region | región | région | Region | regione |
| storage | almacenamiento | stockage | Speicher | archiviazione |
| file mapping | mapeo de archivos | mappage de fichier | Dateizuordnung | mappatura di file |
| descriptor | descriptor | descripteur | Deskriptor | descrittore |
| queue | cola | file d’attente | Warteschlange | coda |
| fence (GPU synchronization) | valla | barrière | Fence | fence |
| payload | carga útil | charge utile | Nutzlast | carico utile |
| parser | analizador sintáctico | analyseur syntaxique | Parser | analizzatore sintattico |
| syntax error | error de sintaxis | erreur de syntaxe | Syntaxfehler | errore di sintassi |
| branch | salto | branchement | Sprung | salto |
| branch target | destino del salto | cible de branchement | Sprungziel | destinazione del salto |
| loop | bucle | boucle | Schleife | ciclo |
| basic block | bloque básico | bloc de base | Basisblock | blocco di base |
| control flow | flujo de control | flux de contrôle | Kontrollfluss | flusso di controllo |
| optimizer | optimizador | optimiseur | Optimierer | ottimizzatore |
| vectorization | vectorización | vectorisation | Vektorisierung | vettorizzazione |
| alignment | alineación | alignement | Ausrichtung | allineamento |
| overflow | desbordamiento | dépassement de capacité | Überlauf | overflow |
| underflow | subdesbordamiento | sous-dépassement | Unterlauf | underflow |
| out of memory | memoria insuficiente | mémoire insuffisante | nicht genügend Speicher | memoria insufficiente |

## Preferred terms: Greek, Russian, Chinese, and Japanese

| English | Greek (`el`) | Russian (`ru`) | Simplified Chinese (`zh`) | Traditional Chinese (`tw`) | Japanese (`ja`) |
|---|---|---|---|---|---|
| accelerator | επιταχυντής | ускоритель | 加速器 | 加速器 | アクセラレータ |
| backend | backend | бэкенд | 后端 | 後端 | バックエンド |
| virtual machine | εικονική μηχανή | виртуальная машина | 虚拟机 | 虛擬機器 | 仮想マシン |
| compiler | μεταγλωττιστής | компилятор | 编译器 | 編譯器 | コンパイラ |
| interpreter | διερμηνευτής | интерпретатор | 解释器 | 直譯器 | インタプリタ |
| JIT compilation | μεταγλώττιση JIT | JIT-компиляция | JIT 编译 | JIT 編譯 | JITコンパイル |
| source code | πηγαίος κώδικας | исходный код | 源代码 | 原始碼 | ソースコード |
| bytecode | bytecode | байткод | 字节码 | 位元組碼 | バイトコード |
| module | μονάδα | модуль | 模块 | 模組 | モジュール |
| program | πρόγραμμα | программа | 程序 | 程式 | プログラム |
| application | εφαρμογή | приложение | 应用程序 | 應用程式 | アプリケーション |
| runtime | περιβάλλον εκτέλεσης | среда выполнения | 运行时环境 | 執行階段環境 | 実行環境 |
| execution | εκτέλεση | выполнение | 执行 | 執行 | 実行 |
| input | είσοδος | ввод | 输入 | 輸入 | 入力 |
| output | έξοδος | вывод | 输出 | 輸出 | 出力 |
| result | αποτέλεσμα | результат | 结果 | 結果 | 結果 |
| memory | μνήμη | память | 内存 | 記憶體 | メモリ |
| function | συνάρτηση | функция | 函数 | 函式 | 関数 |
| intrinsic | ενσωματωμένη συνάρτηση | встроенная функция | 内置函数 | 內建函式 | 組み込み関数 |
| call | κλήση | вызов | 调用 | 呼叫 | 呼び出し |
| caller | καλούσα συνάρτηση | вызывающая функция | 调用方 | 呼叫端 | 呼び出し元 |
| callee | καλούμενη συνάρτηση | вызываемая функция | 被调用方 | 被呼叫端 | 呼び出し先 |
| argument | όρισμα | аргумент | 实参 | 引數 | 引数 |
| parameter | παράμετρος | параметр | 形参 | 參數 | パラメータ |
| return value | τιμή επιστροφής | возвращаемое значение | 返回值 | 傳回值 | 戻り値 |
| value | τιμή | значение | 值 | 值 | 値 |
| variable | μεταβλητή | переменная | 变量 | 變數 | 変数 |
| local variable | τοπική μεταβλητή | локальная переменная | 局部变量 | 區域變數 | ローカル変数 |
| global variable | καθολική μεταβλητή | глобальная переменная | 全局变量 | 全域變數 | グローバル変数 |
| symbol | σύμβολο | символ | 符号 | 符號 | シンボル |
| type | τύπος | тип | 类型 | 型別 | 型 |
| primitive type | πρωτογενής τύπος | примитивный тип | 基本类型 | 基本型別 | プリミティブ型 |
| element | στοιχείο | элемент | 元素 | 元素 | 要素 |
| element type | τύπος στοιχείου | тип элемента | 元素类型 | 元素型別 | 要素型 |
| size | μέγεθος | размер | 大小 | 大小 | サイズ |
| count | πλήθος | количество | 数量 | 數量 | 数 |
| limit | όριο | предел | 限制 | 限制 | 上限 |
| array | πίνακας | массив | 数组 | 陣列 | 配列 |
| dictionary | λεξικό | словарь | 字典 | 字典 | 辞書 |
| string | συμβολοσειρά | строка | 字符串 | 字串 | 文字列 |
| integer | ακέραιος αριθμός | целое число | 整数 | 整數 | 整数 |
| floating-point number | αριθμός κινητής υποδιαστολής | число с плавающей точкой | 浮点数 | 浮點數 | 浮動小数点数 |
| index | δείκτης | индекс | 索引 | 索引 | インデックス |
| subscript | δείκτης | индекс | 下标 | 下標 | 添字 |
| range | εύρος | диапазон | 范围 | 範圍 | 範囲 |
| out of range | εκτός ορίων | вне диапазона | 超出范围 | 超出範圍 | 範囲外 |
| buffer | ενδιάμεση μνήμη | буфер | 缓冲区 | 緩衝區 | バッファ |
| device buffer | ενδιάμεση μνήμη συσκευής | буфер устройства | 设备缓冲区 | 裝置緩衝區 | デバイスバッファ |
| host buffer | ενδιάμεση μνήμη κεντρικού συστήματος | буфер хоста | 主机缓冲区 | 主機緩衝區 | ホストバッファ |
| scalar | βαθμωτό | скаляр | 标量 | 純量 | スカラー |
| scalar result | βαθμωτό αποτέλεσμα | скалярный результат | 标量结果 | 純量結果 | スカラー結果 |
| packed value | τιμή `packed` | значение `packed` | `packed` 值 | `packed` 值 | `packed` 値 |
| shape | σχήμα | форма | 形状 | 形狀 | 形状 |
| rank (number of dimensions) | αριθμός διαστάσεων | число измерений | 维数 | 維度數 | 次元数 |
| extent (dimension size) | έκταση | размер измерения | 维度大小 | 維度大小 | 次元サイズ |
| binding | δέσμευση | привязка | 绑定 | 繫結 | バインディング |
| slot | θέση | слот | 槽位 | 槽位 | スロット |
| metadata | μεταδεδομένα | метаданные | 元数据 | 中繼資料 | メタデータ |
| kernel | πυρήνας | ядро | 内核 | 核心 | カーネル |
| dispatch | αποστολή | диспетчеризация | 调度 | 調度 | ディスパッチ |
| workgroup | ομάδα εργασίας | рабочая группа | 工作组 | 工作群組 | ワークグループ |
| shader | shader | шейдер | 着色器 | 著色器 | シェーダー |
| compute shader | compute shader | вычислительный шейдер | 计算着色器 | 計算著色器 | コンピュートシェーダー |
| shader module | μονάδα shader | модуль шейдера | 着色器模块 | 著色器模組 | シェーダーモジュール |
| pipeline | pipeline | конвейер | 管线 | 管線 | パイプライン |
| command buffer | ενδιάμεση μνήμη εντολών | буфер команд | 命令缓冲区 | 命令緩衝區 | コマンドバッファ |
| device | συσκευή | устройство | 设备 | 裝置 | デバイス |
| context | περιβάλλον | контекст | 上下文 | 內容 | コンテキスト |
| session | συνεδρία | сеанс | 会话 | 工作階段 | セッション |
| snapshot | στιγμιότυπο | снимок | 快照 | 快照 | スナップショット |
| producer | παραγωγός | производитель | 生产者 | 產生者 | プロデューサー |
| consumer | καταναλωτής | потребитель | 消费者 | 使用者 | コンシューマー |
| reduction | αναγωγή | редукция | 归约 | 歸約 | リダクション |
| region | περιοχή | область | 区域 | 區域 | リージョン |
| storage | χώρος αποθήκευσης | хранилище | 存储 | 儲存空間 | ストレージ |
| file mapping | αντιστοίχιση αρχείου | отображение файла | 文件映射 | 檔案對應 | ファイルマッピング |
| descriptor | περιγραφέας | дескриптор | 描述符 | 描述元 | ディスクリプタ |
| queue | ουρά | очередь | 队列 | 佇列 | キュー |
| fence (GPU synchronization) | φράκτης | fence | 栅栏 | 柵欄 | フェンス |
| payload | ωφέλιμο φορτίο | полезная нагрузка | 有效载荷 | 有效負載 | ペイロード |
| parser | συντακτικός αναλυτής | синтаксический анализатор | 语法分析器 | 語法分析器 | パーサー |
| syntax error | συντακτικό σφάλμα | синтаксическая ошибка | 语法错误 | 語法錯誤 | 構文エラー |
| branch | διακλάδωση | переход | 分支 | 分支 | 分岐 |
| branch target | στόχος διακλάδωσης | цель перехода | 分支目标 | 分支目標 | 分岐先 |
| loop | βρόχος | цикл | 循环 | 迴圈 | ループ |
| basic block | βασικό μπλοκ | базовый блок | 基本块 | 基本區塊 | 基本ブロック |
| control flow | ροή ελέγχου | поток управления | 控制流 | 控制流程 | 制御フロー |
| optimizer | βελτιστοποιητής | оптимизатор | 优化器 | 最佳化器 | オプティマイザ |
| vectorization | διανυσματοποίηση | векторизация | 向量化 | 向量化 | ベクトル化 |
| alignment | ευθυγράμμιση | выравнивание | 对齐 | 對齊 | アラインメント |
| overflow | υπερχείλιση | переполнение | 溢出 | 溢位 | オーバーフロー |
| underflow | υποχείλιση | опустошение | 下溢 | 下溢 | アンダーフロー |
| out of memory | ανεπαρκής μνήμη | недостаточно памяти | 内存不足 | 記憶體不足 | メモリ不足 |
