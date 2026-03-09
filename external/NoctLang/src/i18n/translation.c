#include <string.h>

const char *noct_get_system_language(void);

const char *noct_gettext(const char *msg)
{
    const char *lang_code = noct_get_system_language();
    if (strcmp(msg, "syntax error") == 0) {
        if (strcmp(lang_code, "en") == 0) return "Syntax error.";
        if (strcmp(lang_code, "es") == 0) return "Error de sintaxis.";
        if (strcmp(lang_code, "fr") == 0) return "syntax error";
        if (strcmp(lang_code, "de") == 0) return "Syntaxfehler.";
        if (strcmp(lang_code, "it") == 0) return "Errore di sintassi.";
        if (strcmp(lang_code, "el") == 0) return "Σφάλμα σύνταξης.";
        if (strcmp(lang_code, "ru") == 0) return "Синтаксическая ошибка.";
        if (strcmp(lang_code, "zh") == 0) return "语法错误。";
        if (strcmp(lang_code, "tw") == 0) return "語法錯誤。";
        if (strcmp(lang_code, "ja") == 0) return "文法エラーです。";
        if (strcmp(lang_code, "ca") == 0) return "Error de sintaxi.";
        return "syntax error";
    }
    if (strcmp(msg, "%s: Out of memory while parsing.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "%s: No hay suficiente memoria.";
        if (strcmp(lang_code, "fr") == 0) return "%s: Mémoire insuffisante lors de l'analyse.";
        if (strcmp(lang_code, "de") == 0) return "%s: Nicht genügend Speicher beim Parsen.";
        if (strcmp(lang_code, "it") == 0) return "%s: Non c'è abbastanza memoria durante il parsing.";
        if (strcmp(lang_code, "el") == 0) return "%s: Δεν υπάρχει αρκετή μνήμη κατά την ανάλυση.";
        if (strcmp(lang_code, "ru") == 0) return "%s: Недостаточно памяти при разборе.";
        if (strcmp(lang_code, "zh") == 0) return "%s：解析时内存不足。";
        if (strcmp(lang_code, "tw") == 0) return "%s: Out of memory while parsing.";
        if (strcmp(lang_code, "ja") == 0) return "%s: メモリが足りません。";
        if (strcmp(lang_code, "ca") == 0) return "%s: No hi ha prou memòria.";
        return "%s: Out of memory while parsing.";
    }
    if (strcmp(msg, "Too many functions.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Hay demasiadas funciones.";
        if (strcmp(lang_code, "fr") == 0) return "Trop de fonctions.";
        if (strcmp(lang_code, "de") == 0) return "Zu viele Funktionen.";
        if (strcmp(lang_code, "it") == 0) return "Ci sono troppe funzioni.";
        if (strcmp(lang_code, "el") == 0) return "Πάρα πολλές συναρτήσεις.";
        if (strcmp(lang_code, "ru") == 0) return "Слишком много функций.";
        if (strcmp(lang_code, "zh") == 0) return "函数过多。";
        if (strcmp(lang_code, "tw") == 0) return "繁体中国語。";
        if (strcmp(lang_code, "ja") == 0) return "関数が多すぎます。";
        if (strcmp(lang_code, "ca") == 0) return "Hi ha massa funcions.";
        return "Too many functions.";
    }
    if (strcmp(msg, "continue appeared outside loop.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "continue es utilizado fuera de un bucle.";
        if (strcmp(lang_code, "fr") == 0) return "L’instruction continue est apparue en dehors d’une boucle.";
        if (strcmp(lang_code, "de") == 0) return "continue wurde außerhalb einer Schleife verwendet.";
        if (strcmp(lang_code, "it") == 0) return "continue è stato utilizzato al di fuori di un ciclo.";
        if (strcmp(lang_code, "el") == 0) return "Η εντολή continue εμφανίστηκε εκτός βρόχου.";
        if (strcmp(lang_code, "ru") == 0) return "Оператор continue использован вне цикла.";
        if (strcmp(lang_code, "zh") == 0) return "continue 语句出现在循环外部。";
        if (strcmp(lang_code, "tw") == 0) return "continue 敘述出現在迴圈外部。";
        if (strcmp(lang_code, "ja") == 0) return "continue文がループの外で使用されました。";
        if (strcmp(lang_code, "ca") == 0) return "continue és utilitzat fora d'un bucle.";
        return "continue appeared outside loop.";
    }
    if (strcmp(msg, "LHS is not a term or an array element.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "LHS no es un término ni un elemento de un array.";
        if (strcmp(lang_code, "fr") == 0) return "La partie gauche n’est ni un terme ni un élément de tableau.";
        if (strcmp(lang_code, "de") == 0) return "LHS ist weder ein Term noch ein Array-Element.";
        if (strcmp(lang_code, "it") == 0) return "LHS non è un termine né un elemento di un array.";
        if (strcmp(lang_code, "el") == 0) return "Η αριστερή πλευρά δεν είναι ούτε όρος ούτε στοιχείο πίνακα.";
        if (strcmp(lang_code, "ru") == 0) return "Левая часть не является ни выражением, ни элементом массива.";
        if (strcmp(lang_code, "zh") == 0) return "左侧不是项或数组元素。";
        if (strcmp(lang_code, "tw") == 0) return "左側不是項或陣列元素。";
        if (strcmp(lang_code, "ja") == 0) return "左辺が項か配列要素ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "LHS no és un terme ni un element d'array.";
        return "LHS is not a term or an array element.";
    }
    if (strcmp(msg, "var is specified without a single symbol.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "var es especificado sin un solo símbolo.";
        if (strcmp(lang_code, "fr") == 0) return "var est spécifié sans symbole unique.";
        if (strcmp(lang_code, "de") == 0) return "var wird ohne ein einzelnes Symbol angegeben.";
        if (strcmp(lang_code, "it") == 0) return "var è specificato senza un singolo simbolo.";
        if (strcmp(lang_code, "el") == 0) return "Το var καθορίστηκε χωρίς ένα μοναδικό σύμβολο.";
        if (strcmp(lang_code, "ru") == 0) return "var указан без единственного символа.";
        if (strcmp(lang_code, "zh") == 0) return "var 未指定单个符号。";
        if (strcmp(lang_code, "tw") == 0) return "var 未指定單一符號。";
        if (strcmp(lang_code, "ja") == 0) return "varが変数名以外に指定されました。";
        if (strcmp(lang_code, "ca") == 0) return "var és especificat sense un sol símbol.";
        return "var is specified without a single symbol.";
    }
    if (strcmp(msg, "else-if block appeared without if block.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "bloque else-if utilizado sin un bloque if.";
        if (strcmp(lang_code, "fr") == 0) return "Un bloc else-if est apparu sans bloc if.";
        if (strcmp(lang_code, "de") == 0) return "else-if-Block wurde ohne zugehörigen if-Block verwendet.";
        if (strcmp(lang_code, "it") == 0) return "blocco else-if è stato utilizzato senza un blocco if.";
        if (strcmp(lang_code, "el") == 0) return "Το μπλοκ else-if εμφανίστηκε χωρίς μπλοκ if.";
        if (strcmp(lang_code, "ru") == 0) return "Блок else-if появился без блока if.";
        if (strcmp(lang_code, "zh") == 0) return "出现了没有 if 块的 else-if 块。";
        if (strcmp(lang_code, "tw") == 0) return "出現了沒有 if 區塊的 else-if 區塊。";
        if (strcmp(lang_code, "ja") == 0) return "else if文がifブロックの後ろ以外で使用されました。";
        if (strcmp(lang_code, "ca") == 0) return "bloc else-if és utilitzat fora el bloc if.";
        return "else-if block appeared without if block.";
    }
    if (strcmp(msg, "else-if appeared after else.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "else-if utilizado después de else.";
        if (strcmp(lang_code, "fr") == 0) return "else-if est apparu après else.";
        if (strcmp(lang_code, "de") == 0) return "else-if wurde nach else verwendet.";
        if (strcmp(lang_code, "it") == 0) return "else-if è stato utilizzato dopo else.";
        if (strcmp(lang_code, "el") == 0) return "Το else-if εμφανίστηκε μετά το else.";
        if (strcmp(lang_code, "ru") == 0) return "else-if появился после else.";
        if (strcmp(lang_code, "zh") == 0) return "else 之后出现了 else-if。";
        if (strcmp(lang_code, "tw") == 0) return "else 之後出現了 else-if。";
        if (strcmp(lang_code, "ja") == 0) return "else if文がelse文の後ろで使用されました。";
        if (strcmp(lang_code, "ca") == 0) return "else-if és utilitzat després d'else.";
        return "else-if appeared after else.";
    }
    if (strcmp(msg, "else appeared after else.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "else文がelse文の後ろで使用されました。";
        if (strcmp(lang_code, "ca") == 0) return "else és utilitzat després d'else.";
        if (strcmp(lang_code, "es") == 0) return "else utilizado después de else.";
        if (strcmp(lang_code, "de") == 0) return "else wurde nach else verwendet.";
        if (strcmp(lang_code, "it") == 0) return "else è stato utilizzato dopo else.";
        return "else appeared after else.";
    }
    if (strcmp(msg, "Exceeded the maximum argument count.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Se ha superado el número máximo de argumentos.";
        if (strcmp(lang_code, "fr") == 0) return "Nombre maximal d’arguments dépassé.";
        if (strcmp(lang_code, "de") == 0) return "Die maximale Anzahl von Argumenten wurde überschritten.";
        if (strcmp(lang_code, "it") == 0) return "È stato superato il numero massimo di argomenti.";
        if (strcmp(lang_code, "el") == 0) return "Υπέρβαση του μέγιστου αριθμού ορισμάτων.";
        if (strcmp(lang_code, "ru") == 0) return "Превышено максимальное количество аргументов.";
        if (strcmp(lang_code, "zh") == 0) return "超过了最大参数数量。";
        if (strcmp(lang_code, "tw") == 0) return "超過了最大參數數量。";
        if (strcmp(lang_code, "ja") == 0) return "引数が多すぎます。";
        if (strcmp(lang_code, "ca") == 0) return "S'ha superat el nombre màxim d'arguments.";
        return "Exceeded the maximum argument count.";
    }
    if (strcmp(msg, "Too many anonymous functions.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Hay demasiadas funciones anónimas.";
        if (strcmp(lang_code, "fr") == 0) return "Trop de fonctions anonymes.";
        if (strcmp(lang_code, "de") == 0) return "Zu viele anonyme Funktionen.";
        if (strcmp(lang_code, "it") == 0) return "Ci sono troppe funzioni anonime.";
        if (strcmp(lang_code, "el") == 0) return "Πάρα πολλές ανώνυμες συναρτήσεις.";
        if (strcmp(lang_code, "ru") == 0) return "Слишком много анонимных функций.";
        if (strcmp(lang_code, "zh") == 0) return "匿名函数过多。";
        if (strcmp(lang_code, "tw") == 0) return "匿名函式過多。";
        if (strcmp(lang_code, "ja") == 0) return "無名関数が多すぎます。";
        if (strcmp(lang_code, "ca") == 0) return "Hi ha massa funcions anònimes.";
        return "Too many anonymous functions.";
    }
    if (strcmp(msg, "Too many local variables.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Hay demasiadas variables locales.";
        if (strcmp(lang_code, "fr") == 0) return "Trop de variables locales.";
        if (strcmp(lang_code, "de") == 0) return "Zu viele lokale Variablen.";
        if (strcmp(lang_code, "it") == 0) return "Ci sono troppe variabili locali.";
        if (strcmp(lang_code, "el") == 0) return "Πάρα πολλές τοπικές μεταβλητές.";
        if (strcmp(lang_code, "ru") == 0) return "Слишком много локальных переменных.";
        if (strcmp(lang_code, "zh") == 0) return "局部变量过多。";
        if (strcmp(lang_code, "tw") == 0) return "區域變數過多。";
        if (strcmp(lang_code, "ja") == 0) return "ローカル変数が多すぎます。";
        if (strcmp(lang_code, "ca") == 0) return "Hi ha massa variables locals.";
        return "Too many local variables.";
    }
    if (strcmp(msg, "Too many jumps.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Hay demasiados saltos.";
        if (strcmp(lang_code, "fr") == 0) return "Trop de sauts.";
        if (strcmp(lang_code, "de") == 0) return "Zu viele Sprünge.";
        if (strcmp(lang_code, "it") == 0) return "Ci sono troppi salti.";
        if (strcmp(lang_code, "el") == 0) return "Πάρα πολλά άλματα.";
        if (strcmp(lang_code, "ru") == 0) return "Слишком много переходов.";
        if (strcmp(lang_code, "zh") == 0) return "跳转过多。";
        if (strcmp(lang_code, "tw") == 0) return "跳轉過多。";
        if (strcmp(lang_code, "ja") == 0) return "ジャンプが多すぎます。";
        if (strcmp(lang_code, "ca") == 0) return "Hi ha massa salts.";
        return "Too many jumps.";
    }
    if (strcmp(msg, "LIR: Out of memory error.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "LIR: No hay suficiente memoria.";
        if (strcmp(lang_code, "fr") == 0) return "LIR: Memoria insuficiente.";
        if (strcmp(lang_code, "de") == 0) return "LIR: Nicht genügend Speicher.";
        if (strcmp(lang_code, "it") == 0) return "LIR: Non c'è abbastanza memoria.";
        if (strcmp(lang_code, "el") == 0) return "LIR: Δεν υπάρχει αρκετή μνήμη.";
        if (strcmp(lang_code, "ru") == 0) return "LIR: Недостаточно памяти.";
        if (strcmp(lang_code, "zh") == 0) return "LIR: 内存不足。";
        if (strcmp(lang_code, "tw") == 0) return "LIR: 記憶體不足。";
        if (strcmp(lang_code, "ja") == 0) return "LIR: メモリが足りません。";
        if (strcmp(lang_code, "ca") == 0) return "LIR: No hi ha prou memòria.";
        return "LIR: Out of memory error.";
    }
    if (strcmp(msg, "Failed to load bytecode data.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se pudo cargar los datos de bytecode.";
        if (strcmp(lang_code, "fr") == 0) return "Échec du chargement des données de bytecode.";
        if (strcmp(lang_code, "de") == 0) return "Bytecode-Daten konnten nicht geladen werden.";
        if (strcmp(lang_code, "it") == 0) return "Impossibile caricare i dati del bytecode.";
        if (strcmp(lang_code, "el") == 0) return "Απέτυχε η φόρτωση των δεδομένων bytecode.";
        if (strcmp(lang_code, "ru") == 0) return "Не удалось загрузить данные байткода.";
        if (strcmp(lang_code, "zh") == 0) return "加载字节码数据失败。";
        if (strcmp(lang_code, "tw") == 0) return "無法載入位元碼資料。";
        if (strcmp(lang_code, "ja") == 0) return "バイトコードの読み込みに失敗しました。";
        if (strcmp(lang_code, "ca") == 0) return "No s'han pogut carregar les dades del bytecode.";
        return "Failed to load bytecode data.";
    }
    if (strcmp(msg, "Memory mapping failed.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El mapeo de la memoria ha fallado.";
        if (strcmp(lang_code, "fr") == 0) return "Échec de l’association mémoire.";
        if (strcmp(lang_code, "de") == 0) return "Speicherabbildung fehlgeschlagen.";
        if (strcmp(lang_code, "it") == 0) return "Mappatura della memoria fallita.";
        if (strcmp(lang_code, "el") == 0) return "Αποτυχία αντιστοίχισης μνήμης.";
        if (strcmp(lang_code, "ru") == 0) return "Ошибка отображения памяти.";
        if (strcmp(lang_code, "zh") == 0) return "内存映射失败。";
        if (strcmp(lang_code, "tw") == 0) return "記憶體映射失敗。";
        if (strcmp(lang_code, "ja") == 0) return "メモリマップに失敗しました。";
        if (strcmp(lang_code, "ca") == 0) return "El mapatge de la memòria ha fallat.";
        return "Memory mapping failed.";
    }
    if (strcmp(msg, "Code too big.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El código es demasiado grande.";
        if (strcmp(lang_code, "fr") == 0) return "Le code est trop volumineux.";
        if (strcmp(lang_code, "de") == 0) return "Der Code ist zu groß.";
        if (strcmp(lang_code, "it") == 0) return "Il codice è troppo grande.";
        if (strcmp(lang_code, "el") == 0) return "Ο κώδικας είναι πολύ μεγάλος.";
        if (strcmp(lang_code, "ru") == 0) return "Код слишком большой.";
        if (strcmp(lang_code, "zh") == 0) return "代码过大。";
        if (strcmp(lang_code, "tw") == 0) return "程式碼過大。";
        if (strcmp(lang_code, "ja") == 0) return "コードが大きすぎます。";
        if (strcmp(lang_code, "ca") == 0) return "El codi és massa gran.";
        return "Code too big.";
    }
    if (strcmp(msg, "Branch target not found.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El destino de la rama no se ha encontrado.";
        if (strcmp(lang_code, "fr") == 0) return "Cible de branchement introuvable.";
        if (strcmp(lang_code, "de") == 0) return "Sprungziel nicht gefunden.";
        if (strcmp(lang_code, "it") == 0) return "Destinazione del ramo non trovata.";
        if (strcmp(lang_code, "el") == 0) return "Δεν βρέθηκε ο στόχος διακλάδωσης.";
        if (strcmp(lang_code, "ru") == 0) return "Цель перехода не найдена.";
        if (strcmp(lang_code, "zh") == 0) return "未找到分支目标。";
        if (strcmp(lang_code, "tw") == 0) return "未找到分支目標。";
        if (strcmp(lang_code, "ja") == 0) return "分岐先がみつかりません。";
        if (strcmp(lang_code, "ca") == 0) return "El destí de la branca no s'ha trobat.";
        return "Branch target not found.";
    }
    if (strcmp(msg, "Failed to load bytecode.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "バイトコードの読み込みに失敗しました。";
        if (strcmp(lang_code, "ca") == 0) return "No s'ha pogut carregar el bytecode.";
        if (strcmp(lang_code, "es") == 0) return "No se ha podido cargar el bytecode.";
        if (strcmp(lang_code, "de") == 0) return "Bytecode konnte nicht geladen werden.";
        if (strcmp(lang_code, "it") == 0) return "Impossibile caricare il bytecode.";
        return "Failed to load bytecode.";
    }
    if (strcmp(msg, "Cannot find function %s.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se ha encontrado la función %s.";
        if (strcmp(lang_code, "fr") == 0) return "Impossible de trouver la fonction %s.";
        if (strcmp(lang_code, "de") == 0) return "Funktion %s nicht gefunden.";
        if (strcmp(lang_code, "it") == 0) return "Funzione %s non trovata.";
        if (strcmp(lang_code, "el") == 0) return "Δεν βρέθηκε η συνάρτηση %s.";
        if (strcmp(lang_code, "ru") == 0) return "Функция %s не найдена.";
        if (strcmp(lang_code, "zh") == 0) return "无法找到函数 %s。";
        if (strcmp(lang_code, "tw") == 0) return "無法找到函式 %s。";
        if (strcmp(lang_code, "ja") == 0) return "関数%sがみつかりません。";
        if (strcmp(lang_code, "ca") == 0) return "No s'ha trobat la funció %s.";
        return "Cannot find function %s.";
    }
    if (strcmp(msg, "Not an array.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No es un array.";
        if (strcmp(lang_code, "fr") == 0) return "Ce n’est pas un tableau.";
        if (strcmp(lang_code, "de") == 0) return "Kein Array.";
        if (strcmp(lang_code, "it") == 0) return "Non è un array.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι πίνακας.";
        if (strcmp(lang_code, "ru") == 0) return "Это не массив.";
        if (strcmp(lang_code, "zh") == 0) return "不是数组。";
        if (strcmp(lang_code, "tw") == 0) return "不是陣列。";
        if (strcmp(lang_code, "ja") == 0) return "配列ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "No és un array.";
        return "Not an array.";
    }
    if (strcmp(msg, "Array index %d is out-of-range.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El índice %d del array se encuentra fuera de rango.";
        if (strcmp(lang_code, "fr") == 0) return "L’index de tableau %d est hors de portée.";
        if (strcmp(lang_code, "de") == 0) return "Array-Index %d ist außerhalb des gültigen Bereichs.";
        if (strcmp(lang_code, "it") == 0) return "L'indice %d dell'array è fuori dall'intervallo.";
        if (strcmp(lang_code, "el") == 0) return "Ο δείκτης πίνακα %d είναι εκτός ορίων.";
        if (strcmp(lang_code, "ru") == 0) return "Индекс массива %d находится вне допустимого диапазона.";
        if (strcmp(lang_code, "zh") == 0) return "数组索引 %d 超出范围。";
        if (strcmp(lang_code, "tw") == 0) return "陣列索引 %d 超出範圍。";
        if (strcmp(lang_code, "ja") == 0) return "配列の添字 %d は範囲外です。";
        if (strcmp(lang_code, "ca") == 0) return "L'índex %d de l'array es troba fora del rang.";
        return "Array index %d is out-of-range.";
    }
    if (strcmp(msg, "Dictionary index %d is out-of-range.") == 0) {
        return "Dictionary index %d is out-of-range.";
    }
    if (strcmp(msg, "Dictionary key \"%s\" not found.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "La llave \"%s\" no se encontró en el diccionario.";
        if (strcmp(lang_code, "fr") == 0) return "La clé \"%s\" du dictionnaire est introuvable.";
        if (strcmp(lang_code, "de") == 0) return "Schlüssel \"%s\" im Dictionary nicht gefunden.";
        if (strcmp(lang_code, "it") == 0) return "Chiave \"%s\" non trovata nel dizionario.";
        if (strcmp(lang_code, "el") == 0) return "Το κλειδί \"%s\" δεν βρέθηκε στο λεξικό.";
        if (strcmp(lang_code, "ru") == 0) return "Ключ \"%s\" не найден в словаре.";
        if (strcmp(lang_code, "zh") == 0) return "未找到字典键 \"%s\"。";
        if (strcmp(lang_code, "tw") == 0) return "未找到字典鍵 \"%s\"。";
        if (strcmp(lang_code, "ja") == 0) return "辞書のキー \"%s\" がみつかりません。";
        if (strcmp(lang_code, "ca") == 0) return "La clau \"%s\" del diccionari no s'ha trobat.";
        return "Dictionary key \"%s\" not found.";
    }
    if (strcmp(msg, "Local variable \"%s\" not found.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "La variable local \"%s\" no se ha encontrado.";
        if (strcmp(lang_code, "fr") == 0) return "La variable locale \"%s\" est introuvable.";
        if (strcmp(lang_code, "de") == 0) return "Lokale Variable \"%s\" nicht gefunden.";
        if (strcmp(lang_code, "it") == 0) return "Variabile locale \"%s\" non trovata.";
        if (strcmp(lang_code, "el") == 0) return "Η τοπική μεταβλητή \"%s\" δεν βρέθηκε.";
        if (strcmp(lang_code, "ru") == 0) return "Локальная переменная \"%s\" не найдена.";
        if (strcmp(lang_code, "zh") == 0) return "未找到局部变量 \"%s\"。";
        if (strcmp(lang_code, "tw") == 0) return "未找到區域變數 \"%s\"。";
        if (strcmp(lang_code, "ja") == 0) return "ローカル変数 \"%s\" がみつかりません。";
        if (strcmp(lang_code, "ca") == 0) return "La variable local \"%s\" no s'ha trobat.";
        return "Local variable \"%s\" not found.";
    }
    if (strcmp(msg, "Global variable \"%s\" not found.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "La variable global \"%s\" no se ha encontrado.";
        if (strcmp(lang_code, "fr") == 0) return "La variable globale \"%s\" est introuvable.";
        if (strcmp(lang_code, "de") == 0) return "Globale Variable \"%s\" nicht gefunden.";
        if (strcmp(lang_code, "it") == 0) return "Variabile globale \"%s\" non trovata.";
        if (strcmp(lang_code, "el") == 0) return "Η καθολική μεταβλητή \"%s\" δεν βρέθηκε.";
        if (strcmp(lang_code, "ru") == 0) return "Глобальная переменная \"%s\" не найдена.";
        if (strcmp(lang_code, "zh") == 0) return "未找到全局变量 \"%s\"。";
        if (strcmp(lang_code, "tw") == 0) return "未找到全域變數 \"%s\"。";
        if (strcmp(lang_code, "ja") == 0) return "グローバル変数 \"%s\" がみつかりません。";
        if (strcmp(lang_code, "ca") == 0) return "La variable global \"%s\" no s'ha trobat.";
        return "Global variable \"%s\" not found.";
    }
    if (strcmp(msg, "Value is not a number.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El valor no es un número.";
        if (strcmp(lang_code, "fr") == 0) return "La valeur n’est pas un nombre.";
        if (strcmp(lang_code, "de") == 0) return "Wert ist keine Zahl.";
        if (strcmp(lang_code, "it") == 0) return "Il valore non è un numero.";
        if (strcmp(lang_code, "el") == 0) return "Η τιμή δεν είναι αριθμός.";
        if (strcmp(lang_code, "ru") == 0) return "Значение не является числом.";
        if (strcmp(lang_code, "zh") == 0) return "值不是数字。";
        if (strcmp(lang_code, "tw") == 0) return "值不是數字。";
        if (strcmp(lang_code, "ja") == 0) return "値が数ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "El valor no és un número.";
        return "Value is not a number.";
    }
    if (strcmp(msg, "Value is not an integer.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El valor no es un entero.";
        if (strcmp(lang_code, "fr") == 0) return "La valeur n’est pas un entier.";
        if (strcmp(lang_code, "de") == 0) return "Wert ist keine ganze Zahl.";
        if (strcmp(lang_code, "it") == 0) return "Il valore non è un intero.";
        if (strcmp(lang_code, "el") == 0) return "Η τιμή δεν είναι ακέραιος αριθμός.";
        if (strcmp(lang_code, "ru") == 0) return "Значение не является целым числом.";
        if (strcmp(lang_code, "zh") == 0) return "值不是整数。";
        if (strcmp(lang_code, "tw") == 0) return "值不是整數。";
        if (strcmp(lang_code, "ja") == 0) return "値が整数ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "El valor no és un enter.";
        return "Value is not an integer.";
    }
    if (strcmp(msg, "Value is not a string.") == 0) {
        if (strcmp(lang_code, "ja") == 0) return "値が文字列ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "El valor no és un string.";
        if (strcmp(lang_code, "es") == 0) return "El valor no es un string.";
        if (strcmp(lang_code, "de") == 0) return "Wert ist kein String.";
        if (strcmp(lang_code, "it") == 0) return "Il valore non è un string.";
        return "Value is not a string.";
    }
    if (strcmp(msg, "Value is not a number or a string.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El valor no es un número ni un string.";
        if (strcmp(lang_code, "fr") == 0) return "La valeur n’est ni un nombre ni une chaîne.";
        if (strcmp(lang_code, "de") == 0) return "Wert ist weder eine Zahl noch ein String.";
        if (strcmp(lang_code, "it") == 0) return "Il valore non è né un numero né un string.";
        if (strcmp(lang_code, "el") == 0) return "Η τιμή δεν είναι ούτε αριθμός ούτε συμβολοσειρά.";
        if (strcmp(lang_code, "ru") == 0) return "Значение не является ни числом, ни строкой.";
        if (strcmp(lang_code, "zh") == 0) return "值不是数字或字符串。";
        if (strcmp(lang_code, "tw") == 0) return "值不是數字或字串。";
        if (strcmp(lang_code, "ja") == 0) return "値が数か文字列ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "El valor no és un número ni un string.";
        return "Value is not a number or a string.";
    }
    if (strcmp(msg, "Division by zero.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "División por cero.";
        if (strcmp(lang_code, "fr") == 0) return "Division par zéro.";
        if (strcmp(lang_code, "de") == 0) return "Division durch Null.";
        if (strcmp(lang_code, "it") == 0) return "Divisione per zero.";
        if (strcmp(lang_code, "el") == 0) return "Διαίρεση με το μηδέν.";
        if (strcmp(lang_code, "ru") == 0) return "Деление на ноль.";
        if (strcmp(lang_code, "zh") == 0) return "被零除。";
        if (strcmp(lang_code, "tw") == 0) return "被零除。";
        if (strcmp(lang_code, "ja") == 0) return "ゼロによる除算です。";
        if (strcmp(lang_code, "ca") == 0) return "Divisió per zero.";
        return "Division by zero.";
    }
    if (strcmp(msg, "Subscript not an integer.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El subíndice no es un entero.";
        if (strcmp(lang_code, "fr") == 0) return "L’indice n’est pas un entier.";
        if (strcmp(lang_code, "de") == 0) return "Der Index ist keine ganze Zahl.";
        if (strcmp(lang_code, "it") == 0) return "L'indice non è un intero.";
        if (strcmp(lang_code, "el") == 0) return "Ο δείκτης δεν είναι ακέραιος αριθμός.";
        if (strcmp(lang_code, "ru") == 0) return "Индекс не является целым числом.";
        if (strcmp(lang_code, "zh") == 0) return "下标不是整数";
        if (strcmp(lang_code, "tw") == 0) return "下標不是整數。";
        if (strcmp(lang_code, "ja") == 0) return "添字が整数ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "El subíndex no és un enter.";
        return "Subscript not an integer.";
    }
    if (strcmp(msg, "Not an array or a dictionary.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No es un array ni un diccionario.";
        if (strcmp(lang_code, "fr") == 0) return "Ce n’est ni un tableau ni un dictionnaire.";
        if (strcmp(lang_code, "de") == 0) return "Weder ein Array noch ein Dictionary.";
        if (strcmp(lang_code, "it") == 0) return "Non è un array né un dizionario.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι ούτε πίνακας ούτε λεξικό.";
        if (strcmp(lang_code, "ru") == 0) return "Это не массив и не словарь.";
        if (strcmp(lang_code, "zh") == 0) return "不是数组或字典。";
        if (strcmp(lang_code, "tw") == 0) return "不是陣列或字典。";
        if (strcmp(lang_code, "ja") == 0) return "配列か辞書ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "No és un array ni un diccionari.";
        return "Not an array or a dictionary.";
    }
    if (strcmp(msg, "Subscript not a string.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El subíndice no es un string.";
        if (strcmp(lang_code, "fr") == 0) return "L’indice n’est pas une chaîne.";
        if (strcmp(lang_code, "de") == 0) return "Der Index ist kein String.";
        if (strcmp(lang_code, "it") == 0) return "L'indice non è un string.";
        if (strcmp(lang_code, "el") == 0) return "Ο δείκτης δεν είναι συμβολοσειρά.";
        if (strcmp(lang_code, "ru") == 0) return "Индекс не является строкой.";
        if (strcmp(lang_code, "zh") == 0) return "下标不是字符串。";
        if (strcmp(lang_code, "tw") == 0) return "下標不是字串。";
        if (strcmp(lang_code, "ja") == 0) return "添字が文字列ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "El subíndex no és un string.";
        return "Subscript not a string.";
    }
    if (strcmp(msg, "Value is not a string, an array, or a dictionary.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El valor no es un string, un array ni un diccionario.";
        if (strcmp(lang_code, "fr") == 0) return "La valeur n’est ni une chaîne, ni un tableau, ni un dictionnaire.";
        if (strcmp(lang_code, "de") == 0) return "Wert ist weder ein String, noch ein Array noch ein Dictionary.";
        if (strcmp(lang_code, "it") == 0) return "Il valore non è né un string, né un array né un dizionario.";
        if (strcmp(lang_code, "el") == 0) return "Η τιμή δεν είναι ούτε συμβολοσειρά, ούτε πίνακας, ούτε λεξικό.";
        if (strcmp(lang_code, "ru") == 0) return "Значение не является ни строкой, ни массивом, ни словарем.";
        if (strcmp(lang_code, "zh") == 0) return "值不是字符串、数组或字典。";
        if (strcmp(lang_code, "tw") == 0) return "值不是字串、陣列或字典。";
        if (strcmp(lang_code, "ja") == 0) return "値が文字列、配列、辞書ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "El valor no és un string, un array ni un diccionari.";
        return "Value is not a string, an array, or a dictionary.";
    }
    if (strcmp(msg, "Not a dictionary.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No es un diccionario.";
        if (strcmp(lang_code, "fr") == 0) return "Ce n’est pas un dictionnaire.";
        if (strcmp(lang_code, "de") == 0) return "Kein Dictionary.";
        if (strcmp(lang_code, "it") == 0) return "Non è un dizionario.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι λεξικό.";
        if (strcmp(lang_code, "ru") == 0) return "Это не словарь.";
        if (strcmp(lang_code, "zh") == 0) return "不是字典。";
        if (strcmp(lang_code, "tw") == 0) return "不是字典。";
        if (strcmp(lang_code, "ja") == 0) return "辞書ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "No és un diccionari.";
        return "Not a dictionary.";
    }
    if (strcmp(msg, "Dictionary index out-of-range.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El índice del diccionario está fuera de rango.";
        if (strcmp(lang_code, "fr") == 0) return "Indice du dictionnaire hors de portée.";
        if (strcmp(lang_code, "de") == 0) return "Dictionary-Index außerhalb des gültigen Bereichs.";
        if (strcmp(lang_code, "it") == 0) return "L'indice del dizionario è fuori dall'intervallo.";
        if (strcmp(lang_code, "el") == 0) return "Ο δείκτης του λεξικού είναι εκτός ορίων.";
        if (strcmp(lang_code, "ru") == 0) return "Индекс словаря вне допустимого диапазона.";
        if (strcmp(lang_code, "zh") == 0) return "字典索引超出范围。";
        if (strcmp(lang_code, "tw") == 0) return "字典索引超出範圍。";
        if (strcmp(lang_code, "ja") == 0) return "辞書のインデックスが範囲外です。";
        if (strcmp(lang_code, "ca") == 0) return "L'índex del diccionari està fora de rang.";
        return "Dictionary index out-of-range.";
    }
    if (strcmp(msg, "Symbol \"%s\" not found.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El símbolo \"%s\" no se ha encontrado.";
        if (strcmp(lang_code, "fr") == 0) return "Le symbole \"%s\" est introuvable.";
        if (strcmp(lang_code, "de") == 0) return "Symbol \"%s\" nicht gefunden.";
        if (strcmp(lang_code, "it") == 0) return "Simbolo \"%s\" non trovato.";
        if (strcmp(lang_code, "el") == 0) return "Το σύμβολο \"%s\" δεν βρέθηκε.";
        if (strcmp(lang_code, "ru") == 0) return "Символ \"%s\" не найден.";
        if (strcmp(lang_code, "zh") == 0) return "未找到符号 \"%s\"。";
        if (strcmp(lang_code, "tw") == 0) return "未找到符號 \"%s\"。";
        if (strcmp(lang_code, "ja") == 0) return "シンボル \"%s\" がみつかりません。";
        if (strcmp(lang_code, "ca") == 0) return "El símbol \"%s\" no s'ha trobat.";
        return "Symbol \"%s\" not found.";
    }
    if (strcmp(msg, "%s(): Function arguments not match.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "%s(): Los argumentos de la función no coinciden.";
        if (strcmp(lang_code, "fr") == 0) return "%s(): Les arguments de la fonction ne correspondent pas.";
        if (strcmp(lang_code, "de") == 0) return "%s(): Die Funktionsargumente stimmen nicht überein.";
        if (strcmp(lang_code, "it") == 0) return "%s(): Gli argomenti della funzione non corrispondono.";
        if (strcmp(lang_code, "el") == 0) return "%s(): Τα ορίσματα της συνάρτησης δεν ταιριάζουν.";
        if (strcmp(lang_code, "ru") == 0) return "%s(): Аргументы функции не совпадают.";
        if (strcmp(lang_code, "zh") == 0) return "%s(): 函数参数不匹配。";
        if (strcmp(lang_code, "tw") == 0) return "%s(): 函式參數不相符。";
        if (strcmp(lang_code, "ja") == 0) return "%s(): 関数の引数が一致しません。";
        if (strcmp(lang_code, "ca") == 0) return "%s(): Els arguments de la funció no coincideixen.";
        return "%s(): Function arguments not match.";
    }
    if (strcmp(msg, "Element %d is not an integer.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El elemento %d no es un número entero.";
        if (strcmp(lang_code, "fr") == 0) return "L’élément %d n’est pas un entier.";
        if (strcmp(lang_code, "de") == 0) return "Element %d ist keine ganze Zahl.";
        if (strcmp(lang_code, "it") == 0) return "L’elemento %d non è un numero intero.";
        if (strcmp(lang_code, "el") == 0) return "Το στοιχείο %d δεν είναι ακέραιος αριθμός.";
        if (strcmp(lang_code, "ru") == 0) return "Элемент %d не является целым числом.";
        if (strcmp(lang_code, "zh") == 0) return "第 %d 个元素不是整数。";
        if (strcmp(lang_code, "tw") == 0) return "第 %d 個元素不是整數。";
        if (strcmp(lang_code, "ja") == 0) return "%d 番目の要素が整数ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "L’element %d no és un nombre enter.";
        return "Element %d is not an integer.";
    }
    if (strcmp(msg, "Element %d is not a float.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El elemento %d no es un número decimal.";
        if (strcmp(lang_code, "fr") == 0) return "L’élément %d n’est pas un nombre à virgule flottante.";
        if (strcmp(lang_code, "de") == 0) return "Element %d ist keine Gleitkommazahl.";
        if (strcmp(lang_code, "it") == 0) return "L’elemento %d non è un numero decimale.";
        if (strcmp(lang_code, "el") == 0) return "Το στοιχείο %d δεν είναι δεκαδικός αριθμός.";
        if (strcmp(lang_code, "ru") == 0) return "Элемент %d не является числом с плавающей точкой.";
        if (strcmp(lang_code, "zh") == 0) return "第 %d 个元素不是浮点数。";
        if (strcmp(lang_code, "tw") == 0) return "第 %d 個元素不是浮點數。";
        if (strcmp(lang_code, "ja") == 0) return "%d 番目の要素が浮動小数点数ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "L’element %d no és un nombre amb coma flotant.";
        return "Element %d is not a float.";
    }
    if (strcmp(msg, "Element %d is not a string.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El elemento %d no es una cadena.";
        if (strcmp(lang_code, "fr") == 0) return "L’élément %d n’est pas une chaîne de caractères.";
        if (strcmp(lang_code, "de") == 0) return "Element %d ist keine Zeichenkette.";
        if (strcmp(lang_code, "it") == 0) return "L’elemento %d non è una stringa.";
        if (strcmp(lang_code, "el") == 0) return "Το στοιχείο %d δεν είναι συμβολοσειρά.";
        if (strcmp(lang_code, "ru") == 0) return "Элемент %d не является строкой.";
        if (strcmp(lang_code, "zh") == 0) return "第 %d 个元素不是字符串。";
        if (strcmp(lang_code, "tw") == 0) return "第 %d 個元素不是字串。";
        if (strcmp(lang_code, "ja") == 0) return "%d 番目の要素が文字列ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "L’element %d no és una cadena de caràcters.";
        return "Element %d is not a string.";
    }
    if (strcmp(msg, "Element %d is not an array.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El elemento %d no es una matriz.";
        if (strcmp(lang_code, "fr") == 0) return "L’élément %d n’est pas un tableau.";
        if (strcmp(lang_code, "de") == 0) return "Element %d ist kein Array.";
        if (strcmp(lang_code, "it") == 0) return "L’elemento %d non è un array.";
        if (strcmp(lang_code, "el") == 0) return "Το στοιχείο %d δεν είναι πίνακας.";
        if (strcmp(lang_code, "ru") == 0) return "Элемент %d не является массивом.";
        if (strcmp(lang_code, "zh") == 0) return "第 %d 个元素不是数组。";
        if (strcmp(lang_code, "tw") == 0) return "第 %d 個元素不是陣列。";
        if (strcmp(lang_code, "ja") == 0) return "%d 番目の要素が配列ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "L’element %d no és una matriu.";
        return "Element %d is not an array.";
    }
    if (strcmp(msg, "Element %d is not a dictionary.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El elemento %d no es un diccionario.";
        if (strcmp(lang_code, "fr") == 0) return "L’élément %d n’est pas un dictionnaire.";
        if (strcmp(lang_code, "de") == 0) return "Element %d ist kein Dictionary.";
        if (strcmp(lang_code, "it") == 0) return "L’elemento %d non è un dizionario.";
        if (strcmp(lang_code, "el") == 0) return "Το στοιχείο %d δεν είναι λεξικό.";
        if (strcmp(lang_code, "ru") == 0) return "Элемент %d не является словарем.";
        if (strcmp(lang_code, "zh") == 0) return "第 %d 个元素不是字典。";
        if (strcmp(lang_code, "tw") == 0) return "第 %d 個元素不是字典。";
        if (strcmp(lang_code, "ja") == 0) return "%d 番目の要素が辞書ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "L’element %d no és un diccionari.";
        return "Element %d is not a dictionary.";
    }
    if (strcmp(msg, "Element %d is not a function.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El elemento %d no es una función.";
        if (strcmp(lang_code, "fr") == 0) return "L’élément %d n’est pas une fonction.";
        if (strcmp(lang_code, "de") == 0) return "Element %d ist keine Funktion.";
        if (strcmp(lang_code, "it") == 0) return "L’elemento %d non è una funzione.";
        if (strcmp(lang_code, "el") == 0) return "L’elemento %d non è una funzione.";
        if (strcmp(lang_code, "ru") == 0) return "Элемент %d не является функцией.";
        if (strcmp(lang_code, "zh") == 0) return "第 %d 个元素不是函数。";
        if (strcmp(lang_code, "tw") == 0) return "第 %d 個元素不是函式。";
        if (strcmp(lang_code, "ja") == 0) return "%d 番目の要素が関数ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "L’element %d no és una funció.";
        return "Element %d is not a function.";
    }
    if (strcmp(msg, "Value for key %s is not an integer.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El valor de la clave %s no es un número entero.";
        if (strcmp(lang_code, "fr") == 0) return "La valeur associée à la clé %s n’est pas un entier.";
        if (strcmp(lang_code, "de") == 0) return "Der Wert für den Schlüssel %s ist keine ganze Zahl.";
        if (strcmp(lang_code, "it") == 0) return "Il valore per la chiave %s non è un numero intero.";
        if (strcmp(lang_code, "el") == 0) return "Η τιμή για το κλειδί %s δεν είναι ακέραιος αριθμός.";
        if (strcmp(lang_code, "ru") == 0) return "Значение для ключа %s не является целым числом.";
        if (strcmp(lang_code, "zh") == 0) return "键 %s 对应的值不是整数。";
        if (strcmp(lang_code, "tw") == 0) return "鍵 %s 對應的值不是整數。";
        if (strcmp(lang_code, "ja") == 0) return "キー %s に対応する値が整数ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "El valor per a la clau %s no és un nombre enter.";
        return "Value for key %s is not an integer.";
    }
    if (strcmp(msg, "Value for key %s is not a float.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El valor de la clave %s no es un número decimal.";
        if (strcmp(lang_code, "fr") == 0) return "La valeur associée à la clé %s n’est pas un nombre à virgule flottante.";
        if (strcmp(lang_code, "de") == 0) return "Der Wert für den Schlüssel %s ist keine Gleitkommazahl.";
        if (strcmp(lang_code, "it") == 0) return "Il valore per la chiave %s non è un numero decimale.";
        if (strcmp(lang_code, "el") == 0) return "Η τιμή για το κλειδί %s δεν είναι δεκαδικός αριθμός.";
        if (strcmp(lang_code, "ru") == 0) return "Значение для ключа %s не является числом с плавающей точкой.";
        if (strcmp(lang_code, "zh") == 0) return "键 %s 对应的值不是浮点数。";
        if (strcmp(lang_code, "tw") == 0) return "鍵 %s 對應的值不是浮點數。";
        if (strcmp(lang_code, "ja") == 0) return "キー %s に対応する値が浮動小数点数ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "El valor per a la clau %s no és un nombre amb coma flotant.";
        return "Value for key %s is not a float.";
    }
    if (strcmp(msg, "Value for key %s is not a string.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El valor de la clave %s no es una cadena.";
        if (strcmp(lang_code, "fr") == 0) return "La valeur associée à la clé %s n’est pas une chaîne de caractères.";
        if (strcmp(lang_code, "de") == 0) return "Der Wert für den Schlüssel %s ist keine Zeichenkette.";
        if (strcmp(lang_code, "it") == 0) return "Il valore per la chiave %s non è una stringa.";
        if (strcmp(lang_code, "el") == 0) return "Η τιμή για το κλειδί %s δεν είναι συμβολοσειρά.";
        if (strcmp(lang_code, "ru") == 0) return "Значение для ключа %s не является строкой.";
        if (strcmp(lang_code, "zh") == 0) return "键 %s 对应的值不是字符串。";
        if (strcmp(lang_code, "tw") == 0) return "鍵 %s 對應的值不是字串。";
        if (strcmp(lang_code, "ja") == 0) return "キー %s に対応する値が文字列ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "El valor per a la clau %s no és una cadena de caràcters.";
        return "Value for key %s is not a string.";
    }
    if (strcmp(msg, "Value for key %s is not an array.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El valor de la clave %s no es una matriz.";
        if (strcmp(lang_code, "fr") == 0) return "La valeur associée à la clé %s n’est pas un tableau.";
        if (strcmp(lang_code, "de") == 0) return "Der Wert für den Schlüssel %s ist kein Array.";
        if (strcmp(lang_code, "it") == 0) return "Il valore per la chiave %s non è un array.";
        if (strcmp(lang_code, "el") == 0) return "Η τιμή για το κλειδί %s δεν είναι πίνακας.";
        if (strcmp(lang_code, "ru") == 0) return "Значение для ключа %s не является массивом.";
        if (strcmp(lang_code, "zh") == 0) return "键 %s 对应的值不是数组。";
        if (strcmp(lang_code, "tw") == 0) return "鍵 %s 對應的值不是陣列。";
        if (strcmp(lang_code, "ja") == 0) return "キー %s に対応する値が配列ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "El valor per a la clau %s no és una matriu.";
        return "Value for key %s is not an array.";
    }
    if (strcmp(msg, "Value for key %s is not a dictionary.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El valor de la clave %s no es un diccionario.";
        if (strcmp(lang_code, "fr") == 0) return "La valeur associée à la clé %s n’est pas un dictionnaire.";
        if (strcmp(lang_code, "de") == 0) return "Der Wert für den Schlüssel %s ist kein Dictionary.";
        if (strcmp(lang_code, "it") == 0) return "Il valore per la chiave %s non è un dizionario.";
        if (strcmp(lang_code, "el") == 0) return "Η τιμή για το κλειδί %s δεν είναι λεξικό.";
        if (strcmp(lang_code, "ru") == 0) return "Значение для ключа %s не является словарем.";
        if (strcmp(lang_code, "zh") == 0) return "键 %s 对应的值不是字典。";
        if (strcmp(lang_code, "tw") == 0) return "鍵 %s 對應的值不是字典。";
        if (strcmp(lang_code, "ja") == 0) return "キー %s に対応する値が辞書ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "El valor per a la clau %s no és un diccionari.";
        return "Value for key %s is not a dictionary.";
    }
    if (strcmp(msg, "Value for key %s is not a function.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El valor de la clave %s no es una función.";
        if (strcmp(lang_code, "fr") == 0) return "La valeur associée à la clé %s n’est pas une fonction.";
        if (strcmp(lang_code, "de") == 0) return "Der Wert für den Schlüssel %s ist keine Funktion.";
        if (strcmp(lang_code, "it") == 0) return "Il valore per la chiave %s non è una funzione.";
        if (strcmp(lang_code, "el") == 0) return "Η τιμή για το κλειδί %s δεν είναι συνάρτηση.";
        if (strcmp(lang_code, "ru") == 0) return "Значение для ключа %s не является функцией.";
        if (strcmp(lang_code, "zh") == 0) return "键 %s 对应的值不是函数。";
        if (strcmp(lang_code, "tw") == 0) return "鍵 %s 對應的值不是函式。";
        if (strcmp(lang_code, "ja") == 0) return "キー %s に対応する値が関数ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "El valor per a la clau %s no és una funció.";
        return "Value for key %s is not a function.";
    }
    if (strcmp(msg, "Argument (%d: %s) not an integer.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Argumento (%d: %s) no es un número entero.";
        if (strcmp(lang_code, "fr") == 0) return "L’argument (%d : %s) n’est pas un entier.";
        if (strcmp(lang_code, "de") == 0) return "Argument (%d: %s) ist keine ganze Zahl.";
        if (strcmp(lang_code, "it") == 0) return "L’argomento (%d: %s) non è un numero intero.";
        if (strcmp(lang_code, "el") == 0) return "Το όρισμα (%d: %s) δεν είναι ακέραιος αριθμός.";
        if (strcmp(lang_code, "ru") == 0) return "Аргумент (%d: %s) не является целым числом.";
        if (strcmp(lang_code, "zh") == 0) return "参数 (%d: %s) 不是整数。";
        if (strcmp(lang_code, "tw") == 0) return "參數 (%d: %s) 不是整數。";
        if (strcmp(lang_code, "ja") == 0) return "引数 (%d: %s) が整数ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "L’argument (%d: %s) no és un nombre enter.";
        return "Argument (%d: %s) not an integer.";
    }
    if (strcmp(msg, "Argument (%d: %s) not a float.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Argumento (%d: %s) no es un número decimal.";
        if (strcmp(lang_code, "fr") == 0) return "L’argument (%d : %s) n’est pas un nombre à virgule flottante.";
        if (strcmp(lang_code, "de") == 0) return "Argument (%d: %s) ist keine Gleitkommazahl.";
        if (strcmp(lang_code, "it") == 0) return "L’argomento (%d: %s) non è un numero decimale.";
        if (strcmp(lang_code, "el") == 0) return "Το όρισμα (%d: %s) δεν είναι δεκαδικός αριθμός.";
        if (strcmp(lang_code, "ru") == 0) return "Аргумент (%d: %s) не является числом с плавающей точкой.";
        if (strcmp(lang_code, "zh") == 0) return "参数 (%d: %s) 不是浮点数。";
        if (strcmp(lang_code, "tw") == 0) return "參數 (%d: %s) 不是浮點數。";
        if (strcmp(lang_code, "ja") == 0) return "引数 (%d: %s) が浮動小数点数ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "L’argument (%d: %s) no és un nombre amb coma flotant.";
        return "Argument (%d: %s) not a float.";
    }
    if (strcmp(msg, "Argument (%d: %s) not a string.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Argumento (%d: %s) no es una cadena.";
        if (strcmp(lang_code, "fr") == 0) return "L’argument (%d : %s) n’est pas une chaîne de caractères.";
        if (strcmp(lang_code, "de") == 0) return "Argument (%d: %s) ist keine Zeichenkette.";
        if (strcmp(lang_code, "it") == 0) return "L’argomento (%d: %s) non è una stringa.";
        if (strcmp(lang_code, "el") == 0) return "Το όρισμα (%d: %s) δεν είναι συμβολοσειρά.";
        if (strcmp(lang_code, "ru") == 0) return "Аргумент (%d: %s) не является строкой.";
        if (strcmp(lang_code, "zh") == 0) return "参数 (%d: %s) 不是字符串。";
        if (strcmp(lang_code, "tw") == 0) return "參數 (%d: %s) 不是字串。";
        if (strcmp(lang_code, "ja") == 0) return "引数 (%d: %s) が文字列ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "L’argument (%d: %s) no és una cadena de caràcters.";
        return "Argument (%d: %s) not a string.";
    }
    if (strcmp(msg, "Argument (%d: %s) is not an array.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Argumento (%d: %s) no es una matriz.";
        if (strcmp(lang_code, "fr") == 0) return "L’argument (%d : %s) n’est pas un tableau.";
        if (strcmp(lang_code, "de") == 0) return "Argument (%d: %s) ist kein Array.";
        if (strcmp(lang_code, "it") == 0) return "L’argomento (%d: %s) non è un array.";
        if (strcmp(lang_code, "el") == 0) return "Το όρισμα (%d: %s) δεν είναι πίνακας.";
        if (strcmp(lang_code, "ru") == 0) return "Аргумент (%d: %s) не является массивом.";
        if (strcmp(lang_code, "zh") == 0) return "参数 (%d: %s) 不是数组。";
        if (strcmp(lang_code, "tw") == 0) return "參數 (%d: %s) 不是陣列。";
        if (strcmp(lang_code, "ja") == 0) return "引数 (%d: %s) が配列ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "	L’argument (%d: %s) no és una matriu.";
        return "Argument (%d: %s) is not an array.";
    }
    if (strcmp(msg, "Argument (%d: %s) is not a dictionary.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Argumento (%d: %s) no es un diccionario.";
        if (strcmp(lang_code, "fr") == 0) return "L’argument (%d : %s) n’est pas un dictionnaire.";
        if (strcmp(lang_code, "de") == 0) return "Argument (%d: %s) ist kein Dictionary.";
        if (strcmp(lang_code, "it") == 0) return "L’argomento (%d: %s) non è un dizionario.";
        if (strcmp(lang_code, "el") == 0) return "Το όρισμα (%d: %s) δεν είναι λεξικό.";
        if (strcmp(lang_code, "ru") == 0) return "Аргумент (%d: %s) не является словарем.";
        if (strcmp(lang_code, "zh") == 0) return "参数 (%d: %s) 不是字典。";
        if (strcmp(lang_code, "tw") == 0) return "參數 (%d: %s) 不是字典。";
        if (strcmp(lang_code, "ja") == 0) return "引数 (%d: %s) が辞書ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "L’argument (%d: %s) no és un diccionari.";
        return "Argument (%d: %s) is not a dictionary.";
    }
    if (strcmp(msg, "Not a function.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No es una función.";
        if (strcmp(lang_code, "fr") == 0) return "Ce n’est pas une fonction.";
        if (strcmp(lang_code, "de") == 0) return "Keine Funktion.";
        if (strcmp(lang_code, "it") == 0) return "Non è una funzione.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι συνάρτηση.";
        if (strcmp(lang_code, "ru") == 0) return "Это не функция.";
        if (strcmp(lang_code, "zh") == 0) return "不是函数。";
        if (strcmp(lang_code, "tw") == 0) return "不是函式。";
        if (strcmp(lang_code, "ja") == 0) return "関数ではありません。";
        if (strcmp(lang_code, "ca") == 0) return "No és una funció.";
        return "Not a function.";
    }
    if (strcmp(msg, "Empty array.") == 0) {
        return "Empty array.";
    }
    if (strcmp(msg, "Out of memory.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Memoria insuficiente.";
        if (strcmp(lang_code, "fr") == 0) return "Mémoire insuffisante.";
        if (strcmp(lang_code, "de") == 0) return "Nicht genügend Speicher.";
        if (strcmp(lang_code, "it") == 0) return "Memoria insufficiente.";
        if (strcmp(lang_code, "el") == 0) return "Δεν υπάρχει αρκετή μνήμη.";
        if (strcmp(lang_code, "ru") == 0) return "Недостаточно памяти.";
        if (strcmp(lang_code, "zh") == 0) return "内存不足。";
        if (strcmp(lang_code, "tw") == 0) return "記憶體不足。";
        if (strcmp(lang_code, "ja") == 0) return "メモリが足りません。";
        if (strcmp(lang_code, "ca") == 0) return "Memòria insuficient.";
        return "Out of memory.";
    }
    if (strcmp(msg, "Too many pinned global variables.") == 0) {
        return "Too many pinned global variables.";
    }
    if (strcmp(msg, "Too many pinned local variables.") == 0) {
        return "Too many pinned local variables.";
    }
    if (strcmp(msg, "Cannot open file %s.\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede abrir el archivo %s.\n";
        if (strcmp(lang_code, "fr") == 0) return "Impossible d’ouvrir le fichier %s.\n";
        if (strcmp(lang_code, "de") == 0) return "Datei %s kann nicht geöffnet werden.\n";
        if (strcmp(lang_code, "it") == 0) return "Impossibile aprire il file %s.\n";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι δυνατή η άνοιγμα του αρχείου %s.\n";
        if (strcmp(lang_code, "ru") == 0) return "Не удаётся открыть файл %s.\n";
        if (strcmp(lang_code, "zh") == 0) return "无法打开文件 %s。\n";
        if (strcmp(lang_code, "tw") == 0) return "無法開啟檔案 %s。\n";
        if (strcmp(lang_code, "ja") == 0) return "ファイル%sを開けません。\n";
        if (strcmp(lang_code, "ca") == 0) return "No es pot obrir el fitxer %s.\n";
        return "Cannot open file %s.\n";
    }
    if (strcmp(msg, "Cannot read file %s.\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede leer el archivo %s.\n";
        if (strcmp(lang_code, "fr") == 0) return "Impossible de lire le fichier %s.\n";
        if (strcmp(lang_code, "de") == 0) return "Datei %s kann nicht gelesen werden.\n";
        if (strcmp(lang_code, "it") == 0) return "Impossibile leggere il file %s.\n";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι δυνατή η ανάγνωση του αρχείου %s.\n";
        if (strcmp(lang_code, "ru") == 0) return "Не удалось прочитать файл %s.\n";
        if (strcmp(lang_code, "zh") == 0) return "无法读取文件 %s。\n";
        if (strcmp(lang_code, "tw") == 0) return "無法讀取檔案 %s。\n";
        if (strcmp(lang_code, "ja") == 0) return "ファイル%sを読み込めません。\n";
        if (strcmp(lang_code, "ca") == 0) return "No es pot llegir el fitxer %s.\n";
        return "Cannot read file %s.\n";
    }
    if (strcmp(msg, "Error: %s: %d: %s\n") == 0) {
        if (strcmp(lang_code, "fr") == 0) return "Erreur: %s: %d: %s\n";
        if (strcmp(lang_code, "de") == 0) return "Fehler: %s: %d: %s\n";
        if (strcmp(lang_code, "it") == 0) return "Errore: %s: %d: %s\n";
        if (strcmp(lang_code, "el") == 0) return "Σφάλμα: %s: %d: %s\n";
        if (strcmp(lang_code, "ru") == 0) return "Ошибка: %s: %d: %s\n";
        if (strcmp(lang_code, "zh") == 0) return "错误: %s: %d: %s\n";
        if (strcmp(lang_code, "tw") == 0) return "錯誤: %s: %d: %s\n";
        if (strcmp(lang_code, "ja") == 0) return "エラー: %s: %d: %s\n";
        return "Error: %s: %d: %s\n";
    }
    if (strcmp(msg, "Adding file %s\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Añadiendo el archivo %s\n";
        if (strcmp(lang_code, "fr") == 0) return "Ajout du fichier %s\n";
        if (strcmp(lang_code, "de") == 0) return "Datei %s wird hinzugefügt\n";
        if (strcmp(lang_code, "it") == 0) return "Aggiunta del file %s\n";
        if (strcmp(lang_code, "el") == 0) return "Προσθήκη του αρχείου %s\n";
        if (strcmp(lang_code, "ru") == 0) return "Добавление файла %s\n";
        if (strcmp(lang_code, "zh") == 0) return "正在添加文件 %s\n";
        if (strcmp(lang_code, "tw") == 0) return "正在新增檔案 %s\n";
        if (strcmp(lang_code, "ja") == 0) return "ファイル%sを追加しています。\n";
        if (strcmp(lang_code, "ca") == 0) return "S’està afegint el fitxer %s\n";
        return "Adding file %s\n";
    }
    if (strcmp(msg, "Cannot find %s.\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede encontrar %s.\n";
        if (strcmp(lang_code, "fr") == 0) return "Impossible de trouver %s.\n";
        if (strcmp(lang_code, "de") == 0) return "%s konnte nicht gefunden werden.\n";
        if (strcmp(lang_code, "it") == 0) return "Impossibile trovare %s.\n";
        if (strcmp(lang_code, "el") == 0) return "Δεν ήταν δυνατή η εύρεση του %s.\n";
        if (strcmp(lang_code, "ru") == 0) return "Не удалось найти %s.\n";
        if (strcmp(lang_code, "zh") == 0) return "无法找到 %s。\n";
        if (strcmp(lang_code, "tw") == 0) return "無法找到 %s。\n";
        if (strcmp(lang_code, "ja") == 0) return "%sがみつかりません。\n";
        if (strcmp(lang_code, "ca") == 0) return "No es pot trobar %s.\n";
        return "Cannot find %s.\n";
    }
    if (strcmp(msg, "Searching directory %s.\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Buscando en el directorio %s.\n";
        if (strcmp(lang_code, "fr") == 0) return "Recherche dans le répertoire %s.\n";
        if (strcmp(lang_code, "de") == 0) return "Durchsuche Verzeichnis %s.\n";
        if (strcmp(lang_code, "it") == 0) return "Ricerca nella directory %s.\n";
        if (strcmp(lang_code, "el") == 0) return "Αναζήτηση στον κατάλογο %s.\n";
        if (strcmp(lang_code, "ru") == 0) return "Поиск в каталоге %s.\n";
        if (strcmp(lang_code, "zh") == 0) return "正在搜索目录 %s。\n";
        if (strcmp(lang_code, "tw") == 0) return "正在搜尋目錄 %s。\n";
        if (strcmp(lang_code, "ja") == 0) return "ディレクトリ%sを検索中です。\n";
        if (strcmp(lang_code, "ca") == 0) return "S’està cercant el directori %s.\n";
        return "Searching directory %s.\n";
    }
    if (strcmp(msg, "Skipping empty directory %s.\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Omitiendo el directorio vacío %s.\n";
        if (strcmp(lang_code, "fr") == 0) return "Répertoire vide ignoré : %s.\n";
        if (strcmp(lang_code, "de") == 0) return "Leeres Verzeichnis %s wird übersprungen.\n";
        if (strcmp(lang_code, "it") == 0) return "Directory vuota %s ignorata.\n";
        if (strcmp(lang_code, "el") == 0) return "Παράλειψη του κενoύ καταλόγου %s.\n";
        if (strcmp(lang_code, "ru") == 0) return "Пропуск пустого каталога %s.\n";
        if (strcmp(lang_code, "zh") == 0) return "跳过空目录 %s。\n";
        if (strcmp(lang_code, "tw") == 0) return "跳過空目錄 %s。\n";
        if (strcmp(lang_code, "ja") == 0) return "空のディレクトリ%sをスキップしています。\n";
        if (strcmp(lang_code, "ca") == 0) return "S’està ometent el directori buit %s.\n";
        return "Skipping empty directory %s.\n";
    }
    if (strcmp(msg, "Unknown option %s.\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Opción desconocida %s.\n";
        if (strcmp(lang_code, "fr") == 0) return "Option inconnue : %s.\n";
        if (strcmp(lang_code, "de") == 0) return "Unbekannte Option %s.\n";
        if (strcmp(lang_code, "it") == 0) return "Opzione sconosciuta: %s.\n";
        if (strcmp(lang_code, "el") == 0) return "Άγνωστη επιλογή %s.\n";
        if (strcmp(lang_code, "ru") == 0) return "Неизвестная опция %s.\n";
        if (strcmp(lang_code, "zh") == 0) return "未知选项 %s。\n";
        if (strcmp(lang_code, "tw") == 0) return "未知選項 %s。\n";
        if (strcmp(lang_code, "ja") == 0) return "不明なオプション%s\n";
        if (strcmp(lang_code, "ca") == 0) return "Opció desconeguda: %s.\n";
        return "Unknown option %s.\n";
    }
    if (strcmp(msg, "Specify a file.\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Especifique un archivo.\n";
        if (strcmp(lang_code, "fr") == 0) return "Veuillez spécifier un fichier.\n";
        if (strcmp(lang_code, "de") == 0) return "Bitte eine Datei angeben.\n";
        if (strcmp(lang_code, "it") == 0) return "Specificare un file.\n";
        if (strcmp(lang_code, "el") == 0) return "Καθορίστε ένα αρχείο.\n";
        if (strcmp(lang_code, "ru") == 0) return "Укажите файл.\n";
        if (strcmp(lang_code, "zh") == 0) return "请指定一个文件。\n";
        if (strcmp(lang_code, "tw") == 0) return "請指定一個檔案。\n";
        if (strcmp(lang_code, "ja") == 0) return "ファイルを指定してください。\n";
        if (strcmp(lang_code, "ca") == 0) return "Especifiqueu un fitxer.\n";
        return "Specify a file.\n";
    }
    if (strcmp(msg, "%s:%d: Error: %s\n") == 0) {
        if (strcmp(lang_code, "fr") == 0) return "%s:%d: Erreur: %s\n";
        if (strcmp(lang_code, "de") == 0) return "%s:%d: Fehler: %s\n";
        if (strcmp(lang_code, "it") == 0) return "%s:%d: Errore: %s\n";
        if (strcmp(lang_code, "el") == 0) return "%s:%d: Σφάλμα: %s\n";
        if (strcmp(lang_code, "ru") == 0) return "%s:%d: Ошибка: %s\n";
        if (strcmp(lang_code, "zh") == 0) return "%s:%d: 错误: %s\n";
        if (strcmp(lang_code, "tw") == 0) return "%s:%d: 錯誤: %s\n";
        if (strcmp(lang_code, "ja") == 0) return "%s:%d: エラー: %s\n";
        return "%s:%d: Error: %s\n";
    }
    if (strcmp(msg, "Out of memory.\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Memoria insuficiente.\n";
        if (strcmp(lang_code, "fr") == 0) return "Mémoire insuffisante.\n";
        if (strcmp(lang_code, "de") == 0) return "Nicht genügend Speicher.\n";
        if (strcmp(lang_code, "it") == 0) return "Memoria insufficiente.\n";
        if (strcmp(lang_code, "el") == 0) return "Δεν υπάρχει αρκετή μνήμη.\n";
        if (strcmp(lang_code, "ru") == 0) return "Недостаточно памяти.\n";
        if (strcmp(lang_code, "zh") == 0) return "内存不足。\n";
        if (strcmp(lang_code, "tw") == 0) return "記憶體不足。\n";
        if (strcmp(lang_code, "ja") == 0) return "メモリが足りません。\n";
        if (strcmp(lang_code, "ca") == 0) return "Memòria insuficient.\n";
        return "Out of memory.\n";
    }
    if (strcmp(msg, "Noct Programming Language ") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Lenguaje de programación Noct ";
        if (strcmp(lang_code, "fr") == 0) return "Langage de programmation Noct ";
        if (strcmp(lang_code, "de") == 0) return "Programmiersprache Noct ";
        if (strcmp(lang_code, "it") == 0) return "Linguaggio di programmazione Noct ";
        if (strcmp(lang_code, "el") == 0) return "Γλώσσα προγραμματισμού Noct ";
        if (strcmp(lang_code, "ru") == 0) return "Язык программирования Noct ";
        if (strcmp(lang_code, "zh") == 0) return "Noct 编程语言 ";
        if (strcmp(lang_code, "tw") == 0) return "Noct 程式語言 ";
        if (strcmp(lang_code, "ja") == 0) return "Noct プログラミング言語 ";
        if (strcmp(lang_code, "ca") == 0) return "Llenguatge de programació Noct ";
        return "Noct Programming Language ";
    }
    if (strcmp(msg, "Version %s\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Versión %s\n";
        if (strcmp(lang_code, "it") == 0) return "Versione %s\n";
        if (strcmp(lang_code, "el") == 0) return "Έκδοση %s\n";
        if (strcmp(lang_code, "ru") == 0) return "Версия %s\n";
        if (strcmp(lang_code, "zh") == 0) return "版本 %s\n";
        if (strcmp(lang_code, "tw") == 0) return "版本 %s\n";
        if (strcmp(lang_code, "ja") == 0) return "バージョン %s\n";
        if (strcmp(lang_code, "ca") == 0) return "Versió %s\n";
        return "Version %s\n";
    }
    if (strcmp(msg, "JIT compilation is enabled. Starting the fast VM...\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "La compilación JIT está activada. Iniciando la máquina virtual rápida...\n";
        if (strcmp(lang_code, "fr") == 0) return "La compilation JIT est activée. Démarrage de la machine virtuelle rapide...\n";
        if (strcmp(lang_code, "de") == 0) return "JIT-Kompilierung ist aktiviert. Starte die schnelle VM...\n";
        if (strcmp(lang_code, "it") == 0) return "La compilazione JIT è attivata. Avvio della VM veloce...\n";
        if (strcmp(lang_code, "el") == 0) return "Η μεταγλώττιση JIT είναι ενεργοποιημένη. Εκκίνηση της γρήγορης εικονικής μηχανής...\n";
        if (strcmp(lang_code, "ru") == 0) return "JIT-компиляция включена. Запуск быстрой виртуальной машины...\n";
        if (strcmp(lang_code, "zh") == 0) return "JIT 编译已启用。正在启动快速虚拟机…\n";
        if (strcmp(lang_code, "tw") == 0) return "JIT 編譯已啟用。正在啟動快速虛擬機器…\n";
        if (strcmp(lang_code, "ja") == 0) return "JITコンパイルが有効 ... 高速仮想マシン実行を開始します。\n";
        if (strcmp(lang_code, "ca") == 0) return "La compilació JIT està activada. S’està iniciant la màquina virtual ràpida...\n";
        return "JIT compilation is enabled. Starting the fast VM...\n";
    }
    if (strcmp(msg, "Entering REPL mode.\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Entrando en modo REPL.\n";
        if (strcmp(lang_code, "fr") == 0) return "Entrée en mode REPL.\n";
        if (strcmp(lang_code, "de") == 0) return "REPL-Modus wird gestartet.\n";
        if (strcmp(lang_code, "it") == 0) return "Accesso alla modalità REPL.\n";
        if (strcmp(lang_code, "el") == 0) return "Είσοδος σε λειτουργία REPL.\n";
        if (strcmp(lang_code, "ru") == 0) return "Переход в режим REPL.\n";
        if (strcmp(lang_code, "zh") == 0) return "进入 REPL 模式。\n";
        if (strcmp(lang_code, "tw") == 0) return "進入 REPL 模式。\n";
        if (strcmp(lang_code, "ja") == 0) return "REPLモードに入ります。\n";
        if (strcmp(lang_code, "ca") == 0) return "S’està entrant al mode REPL.\n";
        return "Entering REPL mode.\n";
    }
    if (strcmp(msg, "Usage\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Uso\n";
        if (strcmp(lang_code, "fr") == 0) return "Utilisation\n";
        if (strcmp(lang_code, "de") == 0) return "Verwendung\n";
        if (strcmp(lang_code, "it") == 0) return "Utilizzo\n";
        if (strcmp(lang_code, "el") == 0) return "Χρήση\n";
        if (strcmp(lang_code, "ru") == 0) return "Использование\n";
        if (strcmp(lang_code, "zh") == 0) return "用法\n";
        if (strcmp(lang_code, "tw") == 0) return "用法\n";
        if (strcmp(lang_code, "ja") == 0) return "使い方\n";
        if (strcmp(lang_code, "ca") == 0) return "Ús\n";
        return "Usage\n";
    }
    if (strcmp(msg, "  noct <file>                        ... run a program\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "  noct <file>                        ... ejecutar un programa\n";
        if (strcmp(lang_code, "fr") == 0) return "  noct <file>                        ... exécuter un programme\n";
        if (strcmp(lang_code, "de") == 0) return "  noct <file>                        ... ein Programm ausführen\n";
        if (strcmp(lang_code, "it") == 0) return "  noct <file>                        ... eseguire un programma\n";
        if (strcmp(lang_code, "el") == 0) return "  noct <file>                        ... εκτέλεση προγράμματος\n";
        if (strcmp(lang_code, "ru") == 0) return "  noct <file>                        ... запуск программы\n";
        if (strcmp(lang_code, "zh") == 0) return "  noct <file>                        ... 运行程序\n";
        if (strcmp(lang_code, "tw") == 0) return "  noct <file>                        ... 執行程式\n";
        if (strcmp(lang_code, "ja") == 0) return "  noct <ファイル>                    ... プログラムを実行します\n";
        if (strcmp(lang_code, "ca") == 0) return "  noct <file>                        ... executar un programa\n";
        return "  noct <file>                        ... run a program\n";
    }
    if (strcmp(msg, "  noct --compile <files>             ... convert to bytecode files\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "  noct --compile <files>             ... convertir en archivos de bytecode\n";
        if (strcmp(lang_code, "fr") == 0) return "  noct --compile <files>             ... convertir en fichiers bytecode\n";
        if (strcmp(lang_code, "de") == 0) return "  noct --compile <files>             ... in Bytecode-Dateien umwandeln\n";
        if (strcmp(lang_code, "it") == 0) return "  noct --compile <files>             ... convertire in file di bytecode\n";
        if (strcmp(lang_code, "el") == 0) return "  noct --compile <files>             ... μετατροπή σε αρχεία bytecode\n";
        if (strcmp(lang_code, "ru") == 0) return "  noct --compile <files>             ... преобразовать в байткод-файлы\n";
        if (strcmp(lang_code, "zh") == 0) return "  noct --compile <files>             ... 转换为字节码文件\n";
        if (strcmp(lang_code, "tw") == 0) return "  noct --compile <files>             ... 轉換為位元組碼檔案\n";
        if (strcmp(lang_code, "ja") == 0) return "  noct --compile <ファイル...>       ... バイトコードに変換します\n";
        if (strcmp(lang_code, "ca") == 0) return "  noct --compile <files>             ... convertir a fitxers de bytecode\n";
        return "  noct --compile <files>             ... convert to bytecode files\n";
    }
    if (strcmp(msg, "  noct --ansic <out file> <in files> ... convert to a C source file\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "  noct --ansic <out file> <in files> ... convertir en un archivo fuente en C\n";
        if (strcmp(lang_code, "fr") == 0) return "  noct --ansic <out file> <in files> ... convertir en un fichier source C\n";
        if (strcmp(lang_code, "de") == 0) return "  noct --ansic <out file> <in files> ... in eine C-Quelldatei umwandeln\n";
        if (strcmp(lang_code, "it") == 0) return "  noct --ansic <out file> <in files> ... convertire in un file sorgente C\n";
        if (strcmp(lang_code, "el") == 0) return "  noct --ansic <out file> <in files> ... μετατροπή σε αρχείο πηγαίου κώδικα C\n";
        if (strcmp(lang_code, "ru") == 0) return "  noct --ansic <out file> <in files> ... преобразовать в исходный файл C\n";
        if (strcmp(lang_code, "zh") == 0) return "  noct --ansic <out file> <in files> ... 转换为 C 源代码文件\n";
        if (strcmp(lang_code, "tw") == 0) return "  noct --ansic <out file> <in files> ... 轉換為 C 原始碼檔案\n";
        if (strcmp(lang_code, "ja") == 0) return "  noct --ansic <出力> <入力...>      ... Cソースに変換します\n";
        if (strcmp(lang_code, "ca") == 0) return "  noct --ansic <out file> <in files> ... convertir a un fitxer font en C\n";
        return "  noct --ansic <out file> <in files> ... convert to a C source file\n";
    }
    if (strcmp(msg, "  noct --elisp <out file> <in files> ... convert to an Emacs Lisp source file\n") == 0) {
        if (strcmp(lang_code, "es") == 0) return "  noct --elisp <out file> <in files> ... convertir en un archivo fuente Emacs Lisp\n";
        if (strcmp(lang_code, "fr") == 0) return "  noct --elisp <out file> <in files> ... convertir en un fichier source Emacs Lisp\n";
        if (strcmp(lang_code, "de") == 0) return "  noct --elisp <out file> <in files> ... in eine Emacs-Lisp-Quelldatei umwandeln\n";
        if (strcmp(lang_code, "it") == 0) return "  noct --elisp <out file> <in files> ... convertire in un file sorgente Emacs Lisp\n";
        if (strcmp(lang_code, "el") == 0) return "  noct --elisp <out file> <in files> ... μετατροπή σε αρχείο πηγαίου κώδικα Emacs Lisp\n";
        if (strcmp(lang_code, "ru") == 0) return "  noct --elisp <out file> <in files> ... преобразовать в исходный файл Emacs Lisp\n";
        if (strcmp(lang_code, "zh") == 0) return "  noct --elisp <out file> <in files> ... 转换为 Emacs Lisp 源代码文件\n";
        if (strcmp(lang_code, "tw") == 0) return "  noct --elisp <out file> <in files> ... 轉換為 Emacs Lisp 原始碼檔案\n";
        if (strcmp(lang_code, "ja") == 0) return "  noct --elisp <出力> <入力...>      ... Emacs Lispソースに変換します\n";
        if (strcmp(lang_code, "ca") == 0) return "  noct --elisp <out file> <in files> ... convertir a un fitxer font Emacs Lisp\n";
        return "  noct --elisp <out file> <in files> ... convert to an Emacs Lisp source file\n";
    }
    return msg;
}
