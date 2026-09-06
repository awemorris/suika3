#include <noct/noct.h>
#include <string.h>

const char *noct_get_system_language(void);

NOCT_DLL const char *noct_gettext(const char *msg)
{
    const char *lang_code = noct_get_system_language();
    if (strcmp(msg, "syntax error") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Syntax error.";
        if (strncmp(lang_code, "es", 2) == 0) return "Error de sintaxis.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Syntax error";
        if (strncmp(lang_code, "de", 2) == 0) return "Syntaxfehler.";
        if (strncmp(lang_code, "it", 2) == 0) return "Errore di sintassi.";
        if (strncmp(lang_code, "el", 2) == 0) return "Σφάλμα σύνταξης.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Синтаксическая ошибка.";
        if (strncmp(lang_code, "zh", 2) == 0) return "语法错误。";
        if (strncmp(lang_code, "tw", 2) == 0) return "語法錯誤。";
        if (strncmp(lang_code, "ja", 2) == 0) return "文法エラーです。";
        return "syntax error";
    }
    if (strcmp(msg, "%s: Out of memory while parsing.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "%s: Out of memory while parsing.";
        if (strncmp(lang_code, "es", 2) == 0) return "%s: No hay suficiente memoria.";
        if (strncmp(lang_code, "fr", 2) == 0) return "%s: Mémoire insuffisante lors de l'analyse.";
        if (strncmp(lang_code, "de", 2) == 0) return "%s: Nicht genügend Speicher beim Parsen.";
        if (strncmp(lang_code, "it", 2) == 0) return "%s: Non c'è abbastanza memoria durante il parsing.";
        if (strncmp(lang_code, "el", 2) == 0) return "%s: Δεν υπάρχει αρκετή μνήμη κατά την ανάλυση.";
        if (strncmp(lang_code, "ru", 2) == 0) return "%s: Недостаточно памяти при разборе.";
        if (strncmp(lang_code, "zh", 2) == 0) return "%s：解析时内存不足。";
        if (strncmp(lang_code, "tw", 2) == 0) return "%s: 解析時記憶體不足。";
        if (strncmp(lang_code, "ja", 2) == 0) return "%s: メモリが足りません。";
        return "%s: Out of memory while parsing.";
    }
    if (strcmp(msg, "Too many functions.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Too many functions.";
        if (strncmp(lang_code, "es", 2) == 0) return "Hay demasiadas funciones.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Trop de fonctions.";
        if (strncmp(lang_code, "de", 2) == 0) return "Zu viele Funktionen.";
        if (strncmp(lang_code, "it", 2) == 0) return "Ci sono troppe funzioni.";
        if (strncmp(lang_code, "el", 2) == 0) return "Πάρα πολλές συναρτήσεις.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком много функций.";
        if (strncmp(lang_code, "zh", 2) == 0) return "函数过多。";
        if (strncmp(lang_code, "tw", 2) == 0) return "函式過多。";
        if (strncmp(lang_code, "ja", 2) == 0) return "関数が多すぎます。";
        return "Too many functions.";
    }
    if (strcmp(msg, "continue appeared outside loop.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "continue appeared outside loop.";
        if (strncmp(lang_code, "es", 2) == 0) return "continue es utilizado fuera de un bucle.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'instruction continue est apparue en dehors d'une boucle.";
        if (strncmp(lang_code, "de", 2) == 0) return "continue wurde außerhalb einer Schleife verwendet.";
        if (strncmp(lang_code, "it", 2) == 0) return "continue è stato utilizzato al di fuori di un ciclo.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η εντολή continue εμφανίστηκε εκτός βρόχου.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Оператор continue использован вне цикла.";
        if (strncmp(lang_code, "zh", 2) == 0) return "continue 语句出现在循环外部。";
        if (strncmp(lang_code, "tw", 2) == 0) return "continue 敘述出現在迴圈外部。";
        if (strncmp(lang_code, "ja", 2) == 0) return "continue文がループの外で使用されました。";
        return "continue appeared outside loop.";
    }
    if (strcmp(msg, "LHS is not a term or an array element.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "LHS is not a term or an array element.";
        if (strncmp(lang_code, "es", 2) == 0) return "LHS no es un término ni un elemento de un array.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La partie gauche n'est ni un terme ni un élément de tableau.";
        if (strncmp(lang_code, "de", 2) == 0) return "LHS ist weder ein Term noch ein Array-Element.";
        if (strncmp(lang_code, "it", 2) == 0) return "LHS non è un termine né un elemento di un array.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η αριστερή πλευρά δεν είναι ούτε όρος ούτε στοιχείο πίνακα.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Левая часть не является ни выражением, ни элементом массива.";
        if (strncmp(lang_code, "zh", 2) == 0) return "左侧不是项或数组元素。";
        if (strncmp(lang_code, "tw", 2) == 0) return "左側不是項或陣列元素。";
        if (strncmp(lang_code, "ja", 2) == 0) return "左辺が項か配列要素ではありません。";
        return "LHS is not a term or an array element.";
    }
    if (strcmp(msg, "var is specified without a single symbol.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "var is specified without a single symbol.";
        if (strncmp(lang_code, "es", 2) == 0) return "var es especificado sin un solo símbolo.";
        if (strncmp(lang_code, "fr", 2) == 0) return "var est spécifié sans symbole unique.";
        if (strncmp(lang_code, "de", 2) == 0) return "var wird ohne ein einzelnes Symbol angegeben.";
        if (strncmp(lang_code, "it", 2) == 0) return "var è specificato senza un singolo simbolo.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το var καθορίστηκε χωρίς ένα μοναδικό σύμβολο.";
        if (strncmp(lang_code, "ru", 2) == 0) return "var указан без единственного символа.";
        if (strncmp(lang_code, "zh", 2) == 0) return "var 未指定单个符号。";
        if (strncmp(lang_code, "tw", 2) == 0) return "var 未指定單一符號。";
        if (strncmp(lang_code, "ja", 2) == 0) return "varが変数名以外に指定されました。";
        return "var is specified without a single symbol.";
    }
    if (strcmp(msg, "else-if block appeared without if block.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "else-if block appeared without if block.";
        if (strncmp(lang_code, "es", 2) == 0) return "bloque else-if utilizado sin un bloque if.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Un bloc else-if est apparu sans bloc if.";
        if (strncmp(lang_code, "de", 2) == 0) return "else-if-Block wurde ohne zugehörigen if-Block verwendet.";
        if (strncmp(lang_code, "it", 2) == 0) return "blocco else-if è stato utilizzato senza un blocco if.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το μπλοκ else-if εμφανίστηκε χωρίς μπλοκ if.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Блок else-if появился без блока if.";
        if (strncmp(lang_code, "zh", 2) == 0) return "出现了没有 if 块的 else-if 块。";
        if (strncmp(lang_code, "tw", 2) == 0) return "出現了沒有 if 區塊的 else-if 區塊。";
        if (strncmp(lang_code, "ja", 2) == 0) return "else if文がifブロックの後ろ以外で使用されました。";
        return "else-if block appeared without if block.";
    }
    if (strcmp(msg, "else-if appeared after else.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "else-if appeared after else.";
        if (strncmp(lang_code, "es", 2) == 0) return "else-if utilizado después de else.";
        if (strncmp(lang_code, "fr", 2) == 0) return "else-if est apparu après else.";
        if (strncmp(lang_code, "de", 2) == 0) return "else-if wurde nach else verwendet.";
        if (strncmp(lang_code, "it", 2) == 0) return "else-if è stato utilizzato dopo else.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το else-if εμφανίστηκε μετά το else.";
        if (strncmp(lang_code, "ru", 2) == 0) return "else-if появился после else.";
        if (strncmp(lang_code, "zh", 2) == 0) return "else 之后出现了 else-if。";
        if (strncmp(lang_code, "tw", 2) == 0) return "else 之後出現了 else-if。";
        if (strncmp(lang_code, "ja", 2) == 0) return "else if文がelse文の後ろで使用されました。";
        return "else-if appeared after else.";
    }
    if (strcmp(msg, "else appeared after else.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "else appeared after else.";
        if (strncmp(lang_code, "es", 2) == 0) return "else utilizado después de else.";
        if (strncmp(lang_code, "fr", 2) == 0) return "else apparaît après else.";
        if (strncmp(lang_code, "de", 2) == 0) return "else wurde nach else verwendet.";
        if (strncmp(lang_code, "it", 2) == 0) return "else è stato utilizzato dopo else.";
        if (strncmp(lang_code, "el", 2) == 0) return "το else χρησιμοποιήθηκε μετά από το else.";
        if (strncmp(lang_code, "ru", 2) == 0) return "else использовано после else.";
        if (strncmp(lang_code, "zh", 2) == 0) return "else 出现在 else 之后。";
        if (strncmp(lang_code, "tw", 2) == 0) return "else 出現在 else 之後。";
        if (strncmp(lang_code, "ja", 2) == 0) return "else文がelse文の後ろで使用されました。";
        return "else appeared after else.";
    }
    if (strcmp(msg, "Exceeded the maximum argument count.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Exceeded the maximum argument count.";
        if (strncmp(lang_code, "es", 2) == 0) return "Se ha superado el número máximo de argumentos.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Nombre maximal d'arguments dépassé.";
        if (strncmp(lang_code, "de", 2) == 0) return "Die maximale Anzahl von Argumenten wurde überschritten.";
        if (strncmp(lang_code, "it", 2) == 0) return "È stato superato il numero massimo di argomenti.";
        if (strncmp(lang_code, "el", 2) == 0) return "Υπέρβαση του μέγιστου αριθμού ορισμάτων.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Превышено максимальное количество аргументов.";
        if (strncmp(lang_code, "zh", 2) == 0) return "超过了最大参数数量。";
        if (strncmp(lang_code, "tw", 2) == 0) return "超過了最大參數數量。";
        if (strncmp(lang_code, "ja", 2) == 0) return "引数が多すぎます。";
        return "Exceeded the maximum argument count.";
    }
    if (strcmp(msg, "Too many anonymous functions.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Too many anonymous functions.";
        if (strncmp(lang_code, "es", 2) == 0) return "Hay demasiadas funciones anónimas.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Trop de fonctions anonymes.";
        if (strncmp(lang_code, "de", 2) == 0) return "Zu viele anonyme Funktionen.";
        if (strncmp(lang_code, "it", 2) == 0) return "Ci sono troppe funzioni anonime.";
        if (strncmp(lang_code, "el", 2) == 0) return "Πάρα πολλές ανώνυμες συναρτήσεις.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком много анонимных функций.";
        if (strncmp(lang_code, "zh", 2) == 0) return "匿名函数过多。";
        if (strncmp(lang_code, "tw", 2) == 0) return "匿名函式過多。";
        if (strncmp(lang_code, "ja", 2) == 0) return "無名関数が多すぎます。";
        return "Too many anonymous functions.";
    }
    if (strcmp(msg, "Too many local variables.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Too many local variables.";
        if (strncmp(lang_code, "es", 2) == 0) return "Hay demasiadas variables locales.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Trop de variables locales.";
        if (strncmp(lang_code, "de", 2) == 0) return "Zu viele lokale Variablen.";
        if (strncmp(lang_code, "it", 2) == 0) return "Ci sono troppe variabili locali.";
        if (strncmp(lang_code, "el", 2) == 0) return "Πάρα πολλές τοπικές μεταβλητές.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком много локальных переменных.";
        if (strncmp(lang_code, "zh", 2) == 0) return "局部变量过多。";
        if (strncmp(lang_code, "tw", 2) == 0) return "區域變數過多。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ローカル変数が多すぎます。";
        if (strncmp(lang_code, "ca", 2) == 0) return "Hi ha massa variables locals.";
        return "Too many local variables.";
    }
    if (strcmp(msg, "Too many jumps.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Too many jumps.";
        if (strncmp(lang_code, "es", 2) == 0) return "Hay demasiados saltos.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Trop de sauts.";
        if (strncmp(lang_code, "de", 2) == 0) return "Zu viele Sprünge.";
        if (strncmp(lang_code, "it", 2) == 0) return "Ci sono troppi salti.";
        if (strncmp(lang_code, "el", 2) == 0) return "Πάρα πολλά άλματα.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком много переходов.";
        if (strncmp(lang_code, "zh", 2) == 0) return "跳转过多。";
        if (strncmp(lang_code, "tw", 2) == 0) return "跳轉過多。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ジャンプが多すぎます。";
        if (strncmp(lang_code, "ca", 2) == 0) return "Hi ha massa salts.";
        return "Too many jumps.";
    }
    if (strcmp(msg, "LIR: Out of memory error.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "LIR: Out of memory error.";
        if (strncmp(lang_code, "es", 2) == 0) return "LIR: No hay suficiente memoria.";
        if (strncmp(lang_code, "fr", 2) == 0) return "LIR: Memoria insuficiente.";
        if (strncmp(lang_code, "de", 2) == 0) return "LIR: Nicht genügend Speicher.";
        if (strncmp(lang_code, "it", 2) == 0) return "LIR: Non c'è abbastanza memoria.";
        if (strncmp(lang_code, "el", 2) == 0) return "LIR: Δεν υπάρχει αρκετή μνήμη.";
        if (strncmp(lang_code, "ru", 2) == 0) return "LIR: Недостаточно памяти.";
        if (strncmp(lang_code, "zh", 2) == 0) return "LIR: 内存不足。";
        if (strncmp(lang_code, "tw", 2) == 0) return "LIR: 記憶體不足。";
        if (strncmp(lang_code, "ja", 2) == 0) return "LIR: メモリが足りません。";
        return "LIR: Out of memory error.";
    }
    if (strcmp(msg, "Failed to load bytecode data.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Failed to load bytecode data.";
        if (strncmp(lang_code, "es", 2) == 0) return "No se pudo cargar los datos de bytecode.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Échec du chargement des données de bytecode.";
        if (strncmp(lang_code, "de", 2) == 0) return "Bytecode-Daten konnten nicht geladen werden.";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile caricare i dati del bytecode.";
        if (strncmp(lang_code, "el", 2) == 0) return "Απέτυχε η φόρτωση των δεδομένων bytecode.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не удалось загрузить данные байткода.";
        if (strncmp(lang_code, "zh", 2) == 0) return "加载字节码数据失败。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法載入位元碼資料。";
        if (strncmp(lang_code, "ja", 2) == 0) return "バイトコードの読み込みに失敗しました。";
        return "Failed to load bytecode data.";
    }
    if (strcmp(msg, "Memory mapping failed.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Memory mapping failed.";
        if (strncmp(lang_code, "es", 2) == 0) return "El mapeo de la memoria ha fallado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Échec de l'association mémoire.";
        if (strncmp(lang_code, "de", 2) == 0) return "Speicherabbildung fehlgeschlagen.";
        if (strncmp(lang_code, "it", 2) == 0) return "Mappatura della memoria fallita.";
        if (strncmp(lang_code, "el", 2) == 0) return "Αποτυχία αντιστοίχισης μνήμης.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Ошибка отображения памяти.";
        if (strncmp(lang_code, "zh", 2) == 0) return "内存映射失败。";
        if (strncmp(lang_code, "tw", 2) == 0) return "記憶體映射失敗。";
        if (strncmp(lang_code, "ja", 2) == 0) return "メモリマップに失敗しました。";
        return "Memory mapping failed.";
    }
    if (strcmp(msg, "Code too big.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Code too big.";
        if (strncmp(lang_code, "es", 2) == 0) return "El código es demasiado grande.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Le code est trop volumineux.";
        if (strncmp(lang_code, "de", 2) == 0) return "Der Code ist zu groß.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il codice è troppo grande.";
        if (strncmp(lang_code, "el", 2) == 0) return "Ο κώδικας είναι πολύ μεγάλος.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Код слишком большой.";
        if (strncmp(lang_code, "zh", 2) == 0) return "代码过大。";
        if (strncmp(lang_code, "tw", 2) == 0) return "程式碼過大。";
        if (strncmp(lang_code, "ja", 2) == 0) return "コードが大きすぎます。";
        if (strncmp(lang_code, "ca", 2) == 0) return "El codi és massa gran.";
        return "Code too big.";
    }
    if (strcmp(msg, "Branch target not found.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Branch target not found.";
        if (strncmp(lang_code, "es", 2) == 0) return "El destino de la rama no se ha encontrado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Cible de branchement introuvable.";
        if (strncmp(lang_code, "de", 2) == 0) return "Sprungziel nicht gefunden.";
        if (strncmp(lang_code, "it", 2) == 0) return "Destinazione del ramo non trovata.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν βρέθηκε ο στόχος διακλάδωσης.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Цель перехода не найдена.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未找到分支目标。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未找到分支目標。";
        if (strncmp(lang_code, "ja", 2) == 0) return "分岐先がみつかりません。";
        return "Branch target not found.";
    }
    if (strcmp(msg, "Failed to load bytecode.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Failed to load bytecode.";
        if (strncmp(lang_code, "es", 2) == 0) return "No se ha podido cargar el bytecode.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Échec du chargement du bytecode.";
        if (strncmp(lang_code, "de", 2) == 0) return "Bytecode konnte nicht geladen werden.";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile caricare il bytecode.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не удалось загрузить байт-код.";
        if (strncmp(lang_code, "el", 2) == 0) return "Αποτυχία φόρτωσης του bytecode.";
        if (strncmp(lang_code, "zh", 2) == 0) return "字节码加载失败。";
        if (strncmp(lang_code, "tw", 2) == 0) return "位元組碼載入失敗。";
        if (strncmp(lang_code, "ja", 2) == 0) return "バイトコードの読み込みに失敗しました。";
        return "Failed to load bytecode.";
    }
    if (strcmp(msg, "Cannot find function %s.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Cannot find function %s.";
        if (strncmp(lang_code, "es", 2) == 0) return "No se ha encontrado la función %s.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible de trouver la fonction %s.";
        if (strncmp(lang_code, "de", 2) == 0) return "Funktion %s nicht gefunden.";
        if (strncmp(lang_code, "it", 2) == 0) return "Funzione %s non trovata.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν βρέθηκε η συνάρτηση %s.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Функция %s не найдена.";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法找到函数 %s。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法找到函式 %s。";
        if (strncmp(lang_code, "ja", 2) == 0) return "関数%sがみつかりません。";
        return "Cannot find function %s.";
    }
    if (strcmp(msg, "Not an array.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Not an array.";
        if (strncmp(lang_code, "es", 2) == 0) return "No es un array.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Ce n'est pas un tableau.";
        if (strncmp(lang_code, "de", 2) == 0) return "Kein Array.";
        if (strncmp(lang_code, "it", 2) == 0) return "Non è un array.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι πίνακας.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Это не массив.";
        if (strncmp(lang_code, "zh", 2) == 0) return "不是数组。";
        if (strncmp(lang_code, "tw", 2) == 0) return "不是陣列。";
        if (strncmp(lang_code, "ja", 2) == 0) return "配列ではありません。";
        return "Not an array.";
    }
    if (strcmp(msg, "Array index %d is out-of-range.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Array index %d is out-of-range.";
        if (strncmp(lang_code, "es", 2) == 0) return "El índice %d del array se encuentra fuera de rango.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'index de tableau %d est hors de portée.";
        if (strncmp(lang_code, "de", 2) == 0) return "Array-Index %d ist außerhalb des gültigen Bereichs.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'indice %d dell'array è fuori dall'intervallo.";
        if (strncmp(lang_code, "el", 2) == 0) return "Ο δείκτης πίνακα %d είναι εκτός ορίων.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Индекс массива %d находится вне допустимого диапазона.";
        if (strncmp(lang_code, "zh", 2) == 0) return "数组索引 %d 超出范围。";
        if (strncmp(lang_code, "tw", 2) == 0) return "陣列索引 %d 超出範圍。";
        if (strncmp(lang_code, "ja", 2) == 0) return "配列の添字 %d は範囲外です。";
        return "Array index %d is out-of-range.";
    }
    if (strcmp(msg, "Dictionary index %d is out-of-range.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Dictionary index %d is out-of-range.";
        if (strncmp(lang_code, "es", 2) == 0) return "El índice del diccionario %d está fuera de rango.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'index du dictionnaire %d est hors de portée.";
        if (strncmp(lang_code, "de", 2) == 0) return "Wörterbuch-Index %d ist außerhalb des Bereichs.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'indice del dizionario %d è fuori intervallo.";
        if (strncmp(lang_code, "el", 2) == 0) return "Ο δείκτης λεξικού %d είναι εκτός ορίων.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Индекс словаря %d вне диапазона.";
        if (strncmp(lang_code, "zh", 2) == 0) return "字典索引 %d 超出范围。";
        if (strncmp(lang_code, "tw", 2) == 0) return "字典索引 %d 超出範圍。";
        if (strncmp(lang_code, "ja", 2) == 0) return "辞書のインデックス %d は範囲外です。";
        return "Dictionary index %d is out-of-range.";
    }
    if (strcmp(msg, "Dictionary key \"%s\" not found.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Dictionary key \"%s\" not found.";
        if (strncmp(lang_code, "es", 2) == 0) return "La llave \"%s\" no se encontró en el diccionario.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La clé \"%s\" du dictionnaire est introuvable.";
        if (strncmp(lang_code, "de", 2) == 0) return "Schlüssel \"%s\" im Dictionary nicht gefunden.";
        if (strncmp(lang_code, "it", 2) == 0) return "Chiave \"%s\" non trovata nel dizionario.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το κλειδί \"%s\" δεν βρέθηκε στο λεξικό.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Ключ \"%s\" не найден в словаре.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未找到字典键 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未找到字典鍵 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "辞書のキー \"%s\" がみつかりません。";
        return "Dictionary key \"%s\" not found.";
    }
    if (strcmp(msg, "Local variable \"%s\" not found.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Local variable \"%s\" not found.";
        if (strncmp(lang_code, "es", 2) == 0) return "La variable local \"%s\" no se ha encontrado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La variable locale \"%s\" est introuvable.";
        if (strncmp(lang_code, "de", 2) == 0) return "Lokale Variable \"%s\" nicht gefunden.";
        if (strncmp(lang_code, "it", 2) == 0) return "Variabile locale \"%s\" non trovata.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η τοπική μεταβλητή \"%s\" δεν βρέθηκε.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Локальная переменная \"%s\" не найдена.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未找到局部变量 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未找到區域變數 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ローカル変数 \"%s\" がみつかりません。";
        return "Local variable \"%s\" not found.";
    }
    if (strcmp(msg, "Global variable \"%s\" not found.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Global variable \"%s\" not found.";
        if (strncmp(lang_code, "es", 2) == 0) return "La variable global \"%s\" no se ha encontrado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La variable globale \"%s\" est introuvable.";
        if (strncmp(lang_code, "de", 2) == 0) return "Globale Variable \"%s\" nicht gefunden.";
        if (strncmp(lang_code, "it", 2) == 0) return "Variabile globale \"%s\" non trovata.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η καθολική μεταβλητή \"%s\" δεν βρέθηκε.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Глобальная переменная \"%s\" не найдена.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未找到全局变量 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未找到全域變數 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "グローバル変数 \"%s\" がみつかりません。";
        return "Global variable \"%s\" not found.";
    }
    if (strcmp(msg, "Value is not a number.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Value is not a number.";
        if (strncmp(lang_code, "es", 2) == 0) return "El valor no es un número.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La valeur n'est pas un nombre.";
        if (strncmp(lang_code, "de", 2) == 0) return "Wert ist keine Zahl.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il valore non è un numero.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η τιμή δεν είναι αριθμός.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Значение не является числом.";
        if (strncmp(lang_code, "zh", 2) == 0) return "值不是数字。";
        if (strncmp(lang_code, "tw", 2) == 0) return "值不是數字。";
        if (strncmp(lang_code, "ja", 2) == 0) return "値が数ではありません。";
        return "Value is not a number.";
    }
    if (strcmp(msg, "Value is not an integer.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Value is not an integer.";
        if (strncmp(lang_code, "es", 2) == 0) return "El valor no es un entero.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La valeur n'est pas un entier.";
        if (strncmp(lang_code, "de", 2) == 0) return "Wert ist keine ganze Zahl.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il valore non è un intero.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η τιμή δεν είναι ακέραιος αριθμός.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Значение не является целым числом.";
        if (strncmp(lang_code, "zh", 2) == 0) return "值不是整数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "值不是整數。";
        if (strncmp(lang_code, "ja", 2) == 0) return "値が整数ではありません。";
        return "Value is not an integer.";
    }
    if (strcmp(msg, "Value is not a string.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Value is not a string.";
        if (strncmp(lang_code, "es", 2) == 0) return "El valor no es un string.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La valeur n'est pas une chaîne de caractères.";
        if (strncmp(lang_code, "de", 2) == 0) return "Wert ist kein String.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il valore non è un string.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Значение не является строкой.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η τιμή δεν είναι συμβολοσειρά.";
        if (strncmp(lang_code, "zh", 2) == 0) return "值不是字符串。";
        if (strncmp(lang_code, "tw", 2) == 0) return "值不是字串。";
        if (strncmp(lang_code, "ja", 2) == 0) return "値が文字列ではありません。";
        return "Value is not a string.";
    }
    if (strcmp(msg, "Value is not a number or a string.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Value is not a number or a string.";
        if (strncmp(lang_code, "es", 2) == 0) return "El valor no es un número ni un string.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La valeur n'est ni un nombre ni une chaîne.";
        if (strncmp(lang_code, "de", 2) == 0) return "Wert ist weder eine Zahl noch ein String.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il valore non è né un numero né un string.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η τιμή δεν είναι ούτε αριθμός ούτε συμβολοσειρά.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Значение не является ни числом, ни строкой.";
        if (strncmp(lang_code, "zh", 2) == 0) return "值不是数字或字符串。";
        if (strncmp(lang_code, "tw", 2) == 0) return "值不是數字或字串。";
        if (strncmp(lang_code, "ja", 2) == 0) return "値が数か文字列ではありません。";
        return "Value is not a number or a string.";
    }
    if (strcmp(msg, "Division by zero.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Division by zero.";
        if (strncmp(lang_code, "es", 2) == 0) return "División por cero.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Division par zéro.";
        if (strncmp(lang_code, "de", 2) == 0) return "Division durch Null.";
        if (strncmp(lang_code, "it", 2) == 0) return "Divisione per zero.";
        if (strncmp(lang_code, "el", 2) == 0) return "Διαίρεση με το μηδέν.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Деление на ноль.";
        if (strncmp(lang_code, "zh", 2) == 0) return "被零除。";
        if (strncmp(lang_code, "tw", 2) == 0) return "被零除。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ゼロによる除算です。";
        return "Division by zero.";
    }
    if (strcmp(msg, "Subscript not an integer.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Subscript not an integer.";
        if (strncmp(lang_code, "es", 2) == 0) return "El subíndice no es un entero.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'indice n'est pas un entier.";
        if (strncmp(lang_code, "de", 2) == 0) return "Der Index ist keine ganze Zahl.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'indice non è un intero.";
        if (strncmp(lang_code, "el", 2) == 0) return "Ο δείκτης δεν είναι ακέραιος αριθμός.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Индекс не является целым числом.";
        if (strncmp(lang_code, "zh", 2) == 0) return "下标不是整数";
        if (strncmp(lang_code, "tw", 2) == 0) return "下標不是整數。";
        if (strncmp(lang_code, "ja", 2) == 0) return "添字が整数ではありません。";
        return "Subscript not an integer.";
    }
    if (strcmp(msg, "Not an array or a dictionary.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Not an array or a dictionary.";
        if (strncmp(lang_code, "es", 2) == 0) return "No es un array ni un diccionario.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Ce n'est ni un tableau ni un dictionnaire.";
        if (strncmp(lang_code, "de", 2) == 0) return "Weder ein Array noch ein Dictionary.";
        if (strncmp(lang_code, "it", 2) == 0) return "Non è un array né un dizionario.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι ούτε πίνακας ούτε λεξικό.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Это не массив и не словарь.";
        if (strncmp(lang_code, "zh", 2) == 0) return "不是数组或字典。";
        if (strncmp(lang_code, "tw", 2) == 0) return "不是陣列或字典。";
        if (strncmp(lang_code, "ja", 2) == 0) return "配列か辞書ではありません。";
        return "Not an array or a dictionary.";
    }
    if (strcmp(msg, "Subscript not a string.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Subscript not a string.";
        if (strncmp(lang_code, "es", 2) == 0) return "El subíndice no es un string.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'indice n'est pas une chaîne.";
        if (strncmp(lang_code, "de", 2) == 0) return "Der Index ist kein String.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'indice non è un string.";
        if (strncmp(lang_code, "el", 2) == 0) return "Ο δείκτης δεν είναι συμβολοσειρά.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Индекс не является строкой.";
        if (strncmp(lang_code, "zh", 2) == 0) return "下标不是字符串。";
        if (strncmp(lang_code, "tw", 2) == 0) return "下標不是字串。";
        if (strncmp(lang_code, "ja", 2) == 0) return "添字が文字列ではありません。";
        return "Subscript not a string.";
    }
    if (strcmp(msg, "Value is not a string, an array, or a dictionary.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Value is not a string, an array, or a dictionary.";
        if (strncmp(lang_code, "es", 2) == 0) return "El valor no es un string, un array ni un diccionario.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La valeur n'est ni une chaîne, ni un tableau, ni un dictionnaire.";
        if (strncmp(lang_code, "de", 2) == 0) return "Wert ist weder ein String, noch ein Array noch ein Dictionary.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il valore non è né un string, né un array né un dizionario.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η τιμή δεν είναι ούτε συμβολοσειρά, ούτε πίνακας, ούτε λεξικό.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Значение не является ни строкой, ни массивом, ни словарем.";
        if (strncmp(lang_code, "zh", 2) == 0) return "值不是字符串、数组或字典。";
        if (strncmp(lang_code, "tw", 2) == 0) return "值不是字串、陣列或字典。";
        if (strncmp(lang_code, "ja", 2) == 0) return "値が文字列、配列、辞書ではありません。";
        return "Value is not a string, an array, or a dictionary.";
    }
    if (strcmp(msg, "Not a dictionary.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Not a dictionary.";
        if (strncmp(lang_code, "es", 2) == 0) return "No es un diccionario.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Ce n'est pas un dictionnaire.";
        if (strncmp(lang_code, "de", 2) == 0) return "Kein Dictionary.";
        if (strncmp(lang_code, "it", 2) == 0) return "Non è un dizionario.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι λεξικό.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Это не словарь.";
        if (strncmp(lang_code, "zh", 2) == 0) return "不是字典。";
        if (strncmp(lang_code, "tw", 2) == 0) return "不是字典。";
        if (strncmp(lang_code, "ja", 2) == 0) return "辞書ではありません。";
        if (strncmp(lang_code, "ca", 2) == 0) return "No és un diccionari.";
        return "Not a dictionary.";
    }
    if (strcmp(msg, "Dictionary index out-of-range.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Dictionary index out-of-range.";
        if (strncmp(lang_code, "es", 2) == 0) return "El índice del diccionario está fuera de rango.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Indice du dictionnaire hors de portée.";
        if (strncmp(lang_code, "de", 2) == 0) return "Dictionary-Index außerhalb des gültigen Bereichs.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'indice del dizionario è fuori dall'intervallo.";
        if (strncmp(lang_code, "el", 2) == 0) return "Ο δείκτης του λεξικού είναι εκτός ορίων.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Индекс словаря вне допустимого диапазона.";
        if (strncmp(lang_code, "zh", 2) == 0) return "字典索引超出范围。";
        if (strncmp(lang_code, "tw", 2) == 0) return "字典索引超出範圍。";
        if (strncmp(lang_code, "ja", 2) == 0) return "辞書のインデックスが範囲外です。";
        return "Dictionary index out-of-range.";
    }
    if (strcmp(msg, "Symbol \"%s\" not found.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Symbol \"%s\" not found.";
        if (strncmp(lang_code, "es", 2) == 0) return "El símbolo \"%s\" no se ha encontrado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Le symbole \"%s\" est introuvable.";
        if (strncmp(lang_code, "de", 2) == 0) return "Symbol \"%s\" nicht gefunden.";
        if (strncmp(lang_code, "it", 2) == 0) return "Simbolo \"%s\" non trovato.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το σύμβολο \"%s\" δεν βρέθηκε.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Символ \"%s\" не найден.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未找到符号 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未找到符號 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "シンボル \"%s\" がみつかりません。";
        return "Symbol \"%s\" not found.";
    }
    if (strcmp(msg, "%s(): Function arguments not match.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "%s(): Function arguments not match.";
        if (strncmp(lang_code, "es", 2) == 0) return "%s(): Los argumentos de la función no coinciden.";
        if (strncmp(lang_code, "fr", 2) == 0) return "%s(): Les arguments de la fonction ne correspondent pas.";
        if (strncmp(lang_code, "de", 2) == 0) return "%s(): Die Funktionsargumente stimmen nicht überein.";
        if (strncmp(lang_code, "it", 2) == 0) return "%s(): Gli argomenti della funzione non corrispondono.";
        if (strncmp(lang_code, "el", 2) == 0) return "%s(): Τα ορίσματα της συνάρτησης δεν ταιριάζουν.";
        if (strncmp(lang_code, "ru", 2) == 0) return "%s(): Аргументы функции не совпадают.";
        if (strncmp(lang_code, "zh", 2) == 0) return "%s(): 函数参数不匹配。";
        if (strncmp(lang_code, "tw", 2) == 0) return "%s(): 函式參數不相符。";
        if (strncmp(lang_code, "ja", 2) == 0) return "%s(): 関数の引数が一致しません。";
        return "%s(): Function arguments not match.";
    }
    if (strcmp(msg, "Element %d is not an integer.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Element %d is not an integer.";
        if (strncmp(lang_code, "es", 2) == 0) return "El elemento %d no es un número entero.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'élément %d n'est pas un entier.";
        if (strncmp(lang_code, "de", 2) == 0) return "Element %d ist keine ganze Zahl.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'elemento %d non è un numero intero.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το στοιχείο %d δεν είναι ακέραιος αριθμός.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Элемент %d не является целым числом.";
        if (strncmp(lang_code, "zh", 2) == 0) return "第 %d 个元素不是整数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "第 %d 個元素不是整數。";
        if (strncmp(lang_code, "ja", 2) == 0) return "%d 番目の要素が整数ではありません。";
        return "Element %d is not an integer.";
    }
    if (strcmp(msg, "Element %d is not a float.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Element %d is not a float.";
        if (strncmp(lang_code, "es", 2) == 0) return "El elemento %d no es un número decimal.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'élément %d n'est pas un nombre à virgule flottante.";
        if (strncmp(lang_code, "de", 2) == 0) return "Element %d ist keine Gleitkommazahl.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'elemento %d non è un numero decimale.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το στοιχείο %d δεν είναι δεκαδικός αριθμός.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Элемент %d не является числом с плавающей точкой.";
        if (strncmp(lang_code, "zh", 2) == 0) return "第 %d 个元素不是浮点数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "第 %d 個元素不是浮點數。";
        if (strncmp(lang_code, "ja", 2) == 0) return "%d 番目の要素が浮動小数点数ではありません。";
        return "Element %d is not a float.";
    }
    if (strcmp(msg, "Element %d is not a string.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Element %d is not a string.";
        if (strncmp(lang_code, "es", 2) == 0) return "El elemento %d no es una cadena.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'élément %d n'est pas une chaîne de caractères.";
        if (strncmp(lang_code, "de", 2) == 0) return "Element %d ist keine Zeichenkette.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'elemento %d non è una stringa.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το στοιχείο %d δεν είναι συμβολοσειρά.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Элемент %d не является строкой.";
        if (strncmp(lang_code, "zh", 2) == 0) return "第 %d 个元素不是字符串。";
        if (strncmp(lang_code, "tw", 2) == 0) return "第 %d 個元素不是字串。";
        if (strncmp(lang_code, "ja", 2) == 0) return "%d 番目の要素が文字列ではありません。";
        return "Element %d is not a string.";
    }
    if (strcmp(msg, "Element %d is not an array.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Element %d is not an array.";
        if (strncmp(lang_code, "es", 2) == 0) return "El elemento %d no es una matriz.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'élément %d n'est pas un tableau.";
        if (strncmp(lang_code, "de", 2) == 0) return "Element %d ist kein Array.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'elemento %d non è un array.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το στοιχείο %d δεν είναι πίνακας.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Элемент %d не является массивом.";
        if (strncmp(lang_code, "zh", 2) == 0) return "第 %d 个元素不是数组。";
        if (strncmp(lang_code, "tw", 2) == 0) return "第 %d 個元素不是陣列。";
        if (strncmp(lang_code, "ja", 2) == 0) return "%d 番目の要素が配列ではありません。";
        return "Element %d is not an array.";
    }
    if (strcmp(msg, "Element %d is not a dictionary.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Element %d is not a dictionary.";
        if (strncmp(lang_code, "es", 2) == 0) return "El elemento %d no es un diccionario.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'élément %d n'est pas un dictionnaire.";
        if (strncmp(lang_code, "de", 2) == 0) return "Element %d ist kein Dictionary.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'elemento %d non è un dizionario.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το στοιχείο %d δεν είναι λεξικό.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Элемент %d не является словарем.";
        if (strncmp(lang_code, "zh", 2) == 0) return "第 %d 个元素不是字典。";
        if (strncmp(lang_code, "tw", 2) == 0) return "第 %d 個元素不是字典。";
        if (strncmp(lang_code, "ja", 2) == 0) return "%d 番目の要素が辞書ではありません。";
        return "Element %d is not a dictionary.";
    }
    if (strcmp(msg, "Element %d is not a function.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Element %d is not a function.";
        if (strncmp(lang_code, "es", 2) == 0) return "El elemento %d no es una función.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'élément %d n'est pas une fonction.";
        if (strncmp(lang_code, "de", 2) == 0) return "Element %d ist keine Funktion.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'elemento %d non è una funzione.";
        if (strncmp(lang_code, "el", 2) == 0) return "L'elemento %d non è una funzione.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Элемент %d не является функцией.";
        if (strncmp(lang_code, "zh", 2) == 0) return "第 %d 个元素不是函数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "第 %d 個元素不是函式。";
        if (strncmp(lang_code, "ja", 2) == 0) return "%d 番目の要素が関数ではありません。";
        return "Element %d is not a function.";
    }
    if (strcmp(msg, "Value for key %s is not an integer.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Value for key %s is not an integer.";
        if (strncmp(lang_code, "es", 2) == 0) return "El valor de la clave %s no es un número entero.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La valeur associée à la clé %s n'est pas un entier.";
        if (strncmp(lang_code, "de", 2) == 0) return "Der Wert für den Schlüssel %s ist keine ganze Zahl.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il valore per la chiave %s non è un numero intero.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η τιμή για το κλειδί %s δεν είναι ακέραιος αριθμός.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Значение для ключа %s не является целым числом.";
        if (strncmp(lang_code, "zh", 2) == 0) return "键 %s 对应的值不是整数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "鍵 %s 對應的值不是整數。";
        if (strncmp(lang_code, "ja", 2) == 0) return "キー %s に対応する値が整数ではありません。";
        if (strncmp(lang_code, "ca", 2) == 0) return "El valor per a la clau %s no és un nombre enter.";
        return "Value for key %s is not an integer.";
    }
    if (strcmp(msg, "Value for key %s is not a float.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Value for key %s is not a float.";
        if (strncmp(lang_code, "es", 2) == 0) return "El valor de la clave %s no es un número decimal.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La valeur associée à la clé %s n'est pas un nombre à virgule flottante.";
        if (strncmp(lang_code, "de", 2) == 0) return "Der Wert für den Schlüssel %s ist keine Gleitkommazahl.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il valore per la chiave %s non è un numero decimale.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η τιμή για το κλειδί %s δεν είναι δεκαδικός αριθμός.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Значение для ключа %s не является числом с плавающей точкой.";
        if (strncmp(lang_code, "zh", 2) == 0) return "键 %s 对应的值不是浮点数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "鍵 %s 對應的值不是浮點數。";
        if (strncmp(lang_code, "ja", 2) == 0) return "キー %s に対応する値が浮動小数点数ではありません。";
        if (strncmp(lang_code, "ca", 2) == 0) return "El valor per a la clau %s no és un nombre amb coma flotant.";
        return "Value for key %s is not a float.";
    }
    if (strcmp(msg, "Value for key %s is not a string.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Value for key %s is not a string.";
        if (strncmp(lang_code, "es", 2) == 0) return "El valor de la clave %s no es una cadena.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La valeur associée à la clé %s n'est pas une chaîne de caractères.";
        if (strncmp(lang_code, "de", 2) == 0) return "Der Wert für den Schlüssel %s ist keine Zeichenkette.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il valore per la chiave %s non è una stringa.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η τιμή για το κλειδί %s δεν είναι συμβολοσειρά.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Значение для ключа %s не является строкой.";
        if (strncmp(lang_code, "zh", 2) == 0) return "键 %s 对应的值不是字符串。";
        if (strncmp(lang_code, "tw", 2) == 0) return "鍵 %s 對應的值不是字串。";
        if (strncmp(lang_code, "ja", 2) == 0) return "キー %s に対応する値が文字列ではありません。";
        return "Value for key %s is not a string.";
    }
    if (strcmp(msg, "Value for key %s is not an array.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Value for key %s is not an array.";
        if (strncmp(lang_code, "es", 2) == 0) return "El valor de la clave %s no es una matriz.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La valeur associée à la clé %s n'est pas un tableau.";
        if (strncmp(lang_code, "de", 2) == 0) return "Der Wert für den Schlüssel %s ist kein Array.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il valore per la chiave %s non è un array.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η τιμή για το κλειδί %s δεν είναι πίνακας.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Значение для ключа %s не является массивом.";
        if (strncmp(lang_code, "zh", 2) == 0) return "键 %s 对应的值不是数组。";
        if (strncmp(lang_code, "tw", 2) == 0) return "鍵 %s 對應的值不是陣列。";
        if (strncmp(lang_code, "ja", 2) == 0) return "キー %s に対応する値が配列ではありません。";
        return "Value for key %s is not an array.";
    }
    if (strcmp(msg, "Value for key %s is not a dictionary.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Value for key %s is not a dictionary.";
        if (strncmp(lang_code, "es", 2) == 0) return "El valor de la clave %s no es un diccionario.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La valeur associée à la clé %s n'est pas un dictionnaire.";
        if (strncmp(lang_code, "de", 2) == 0) return "Der Wert für den Schlüssel %s ist kein Dictionary.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il valore per la chiave %s non è un dizionario.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η τιμή για το κλειδί %s δεν είναι λεξικό.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Значение для ключа %s не является словарем.";
        if (strncmp(lang_code, "zh", 2) == 0) return "键 %s 对应的值不是字典。";
        if (strncmp(lang_code, "tw", 2) == 0) return "鍵 %s 對應的值不是字典。";
        if (strncmp(lang_code, "ja", 2) == 0) return "キー %s に対応する値が辞書ではありません。";
        return "Value for key %s is not a dictionary.";
    }
    if (strcmp(msg, "Value for key %s is not a function.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Value for key %s is not a function.";
        if (strncmp(lang_code, "es", 2) == 0) return "El valor de la clave %s no es una función.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La valeur associée à la clé %s n'est pas une fonction.";
        if (strncmp(lang_code, "de", 2) == 0) return "Der Wert für den Schlüssel %s ist keine Funktion.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il valore per la chiave %s non è una funzione.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η τιμή για το κλειδί %s δεν είναι συνάρτηση.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Значение для ключа %s не является функцией.";
        if (strncmp(lang_code, "zh", 2) == 0) return "键 %s 对应的值不是函数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "鍵 %s 對應的值不是函式。";
        if (strncmp(lang_code, "ja", 2) == 0) return "キー %s に対応する値が関数ではありません。";
        return "Value for key %s is not a function.";
    }
    if (strcmp(msg, "Argument (%d: %s) not an integer.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Argument (%d: %s) not an integer.";
        if (strncmp(lang_code, "es", 2) == 0) return "Argumento (%d: %s) no es un número entero.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'argument (%d : %s) n'est pas un entier.";
        if (strncmp(lang_code, "de", 2) == 0) return "Argument (%d: %s) ist keine ganze Zahl.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'argomento (%d: %s) non è un numero intero.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το όρισμα (%d: %s) δεν είναι ακέραιος αριθμός.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Аргумент (%d: %s) не является целым числом.";
        if (strncmp(lang_code, "zh", 2) == 0) return "参数 (%d: %s) 不是整数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "參數 (%d: %s) 不是整數。";
        if (strncmp(lang_code, "ja", 2) == 0) return "引数 (%d: %s) が整数ではありません。";
        return "Argument (%d: %s) not an integer.";
    }
    if (strcmp(msg, "Argument (%d: %s) not a float.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Argument (%d: %s) not a float.";
        if (strncmp(lang_code, "es", 2) == 0) return "Argumento (%d: %s) no es un número decimal.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'argument (%d : %s) n'est pas un nombre à virgule flottante.";
        if (strncmp(lang_code, "de", 2) == 0) return "Argument (%d: %s) ist keine Gleitkommazahl.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'argomento (%d: %s) non è un numero decimale.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το όρισμα (%d: %s) δεν είναι δεκαδικός αριθμός.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Аргумент (%d: %s) не является числом с плавающей точкой.";
        if (strncmp(lang_code, "zh", 2) == 0) return "参数 (%d: %s) 不是浮点数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "參數 (%d: %s) 不是浮點數。";
        if (strncmp(lang_code, "ja", 2) == 0) return "引数 (%d: %s) が浮動小数点数ではありません。";
        return "Argument (%d: %s) not a float.";
    }
    if (strcmp(msg, "Argument (%d: %s) not a string.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Argument (%d: %s) not a string.";
        if (strncmp(lang_code, "es", 2) == 0) return "Argumento (%d: %s) no es una cadena.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'argument (%d : %s) n'est pas une chaîne de caractères.";
        if (strncmp(lang_code, "de", 2) == 0) return "Argument (%d: %s) ist keine Zeichenkette.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'argomento (%d: %s) non è una stringa.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το όρισμα (%d: %s) δεν είναι συμβολοσειρά.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Аргумент (%d: %s) не является строкой.";
        if (strncmp(lang_code, "zh", 2) == 0) return "参数 (%d: %s) 不是字符串。";
        if (strncmp(lang_code, "tw", 2) == 0) return "參數 (%d: %s) 不是字串。";
        if (strncmp(lang_code, "ja", 2) == 0) return "引数 (%d: %s) が文字列ではありません。";
        if (strncmp(lang_code, "ca", 2) == 0) return "L'argument (%d: %s) no és una cadena de caràcters.";
        return "Argument (%d: %s) not a string.";
    }
    if (strcmp(msg, "Argument (%d: %s) is not an array.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Argument (%d: %s) is not an array.";
        if (strncmp(lang_code, "es", 2) == 0) return "Argumento (%d: %s) no es una matriz.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'argument (%d : %s) n'est pas un tableau.";
        if (strncmp(lang_code, "de", 2) == 0) return "Argument (%d: %s) ist kein Array.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'argomento (%d: %s) non è un array.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το όρισμα (%d: %s) δεν είναι πίνακας.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Аргумент (%d: %s) не является массивом.";
        if (strncmp(lang_code, "zh", 2) == 0) return "参数 (%d: %s) 不是数组。";
        if (strncmp(lang_code, "tw", 2) == 0) return "參數 (%d: %s) 不是陣列。";
        if (strncmp(lang_code, "ja", 2) == 0) return "引数 (%d: %s) が配列ではありません。";
        return "Argument (%d: %s) is not an array.";
    }
    if (strcmp(msg, "Argument (%d: %s) is not a dictionary.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Argument (%d: %s) is not a dictionary.";
        if (strncmp(lang_code, "es", 2) == 0) return "Argumento (%d: %s) no es un diccionario.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'argument (%d : %s) n'est pas un dictionnaire.";
        if (strncmp(lang_code, "de", 2) == 0) return "Argument (%d: %s) ist kein Dictionary.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'argomento (%d: %s) non è un dizionario.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το όρισμα (%d: %s) δεν είναι λεξικό.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Аргумент (%d: %s) не является словарем.";
        if (strncmp(lang_code, "zh", 2) == 0) return "参数 (%d: %s) 不是字典。";
        if (strncmp(lang_code, "tw", 2) == 0) return "參數 (%d: %s) 不是字典。";
        if (strncmp(lang_code, "ja", 2) == 0) return "引数 (%d: %s) が辞書ではありません。";
        return "Argument (%d: %s) is not a dictionary.";
    }
    if (strcmp(msg, "Not a function.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Not a function.";
        if (strncmp(lang_code, "es", 2) == 0) return "No es una función.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Ce n'est pas une fonction.";
        if (strncmp(lang_code, "de", 2) == 0) return "Keine Funktion.";
        if (strncmp(lang_code, "it", 2) == 0) return "Non è una funzione.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι συνάρτηση.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Это не функция.";
        if (strncmp(lang_code, "zh", 2) == 0) return "不是函数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "不是函式。";
        if (strncmp(lang_code, "ja", 2) == 0) return "関数ではありません。";
        return "Not a function.";
    }
    if (strcmp(msg, "Empty array.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Empty array.";
        if (strncmp(lang_code, "es", 2) == 0) return "Arreglo vacío.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Tableau vide.";
        if (strncmp(lang_code, "de", 2) == 0) return "Leeres Array.";
        if (strncmp(lang_code, "it", 2) == 0) return "Array vuoto.";
        if (strncmp(lang_code, "el", 2) == 0) return "Κενός πίνακας.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Пустой массив.";
        if (strncmp(lang_code, "zh", 2) == 0) return "空数组。";
        if (strncmp(lang_code, "tw", 2) == 0) return "空陣列。";
        if (strncmp(lang_code, "ja", 2) == 0) return "配列が空です。";
        return "Empty array.";
    }
    if (strcmp(msg, "Out of memory.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Out of memory.";
        if (strncmp(lang_code, "es", 2) == 0) return "Memoria insuficiente.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Mémoire insuffisante.";
        if (strncmp(lang_code, "de", 2) == 0) return "Nicht genügend Speicher.";
        if (strncmp(lang_code, "it", 2) == 0) return "Memoria insufficiente.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν υπάρχει αρκετή μνήμη.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недостаточно памяти.";
        if (strncmp(lang_code, "zh", 2) == 0) return "内存不足。";
        if (strncmp(lang_code, "tw", 2) == 0) return "記憶體不足。";
        if (strncmp(lang_code, "ja", 2) == 0) return "メモリが足りません。";
        return "Out of memory.";
    }
    if (strcmp(msg, "Too many pinned global variables.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Too many pinned global variables.";
        if (strncmp(lang_code, "es", 2) == 0) return "Demasiadas variables globales fijadas.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Trop de variables globales epinglees.";
        if (strncmp(lang_code, "de", 2) == 0) return "Zu viele angepinnte globale Variablen.";
        if (strncmp(lang_code, "it", 2) == 0) return "Troppe variabili globali bloccate.";
        if (strncmp(lang_code, "el", 2) == 0) return "Πάρα πολλές καρφιτσωμένες καθολικές μεταβλητές.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком много закрепленных глобальных переменных.";
        if (strncmp(lang_code, "zh", 2) == 0) return "固定全局变量过多。";
        if (strncmp(lang_code, "tw", 2) == 0) return "固定全域變數過多。";
        if (strncmp(lang_code, "ja", 2) == 0) return "固定されたグローバル変数が多すぎます。";
        return "Too many pinned global variables.";
    }
    if (strcmp(msg, "Too many pinned local variables.") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Too many pinned local variables.";
        if (strncmp(lang_code, "es", 2) == 0) return "Demasiadas variables locales fijadas.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Trop de variables locales epinglees.";
        if (strncmp(lang_code, "de", 2) == 0) return "Zu viele angepinnte lokale Variablen.";
        if (strncmp(lang_code, "it", 2) == 0) return "Troppe variabili locali bloccate.";
        if (strncmp(lang_code, "el", 2) == 0) return "Πάρα πολλές καρφιτσωμένες τοπικές μεταβλητές.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком много закрепленных локальных переменных.";
        if (strncmp(lang_code, "zh", 2) == 0) return "固定局部变量过多。";
        if (strncmp(lang_code, "tw", 2) == 0) return "固定區域變數過多。";
        if (strncmp(lang_code, "ja", 2) == 0) return "固定されたローカル変数が多すぎます。";
        return "Too many pinned local variables.";
    }
    if (strcmp(msg, "Cannot open file %s.\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Cannot open file %s.\n";
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede abrir el archivo %s.\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible d'ouvrir le fichier %s.\n";
        if (strncmp(lang_code, "de", 2) == 0) return "Datei %s kann nicht geöffnet werden.\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile aprire il file %s.\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι δυνατό το άνοιγμα του αρχείου %s.\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не удаётся открыть файл %s.\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法打开文件 %s。\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法開啟檔案 %s。\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "ファイル%sを開けません。\n";
        return "Cannot open file %s.\n";
    }
    if (strcmp(msg, "Cannot read file %s.\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Cannot read file %s.\n";
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede leer el archivo %s.\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible de lire le fichier %s.\n";
        if (strncmp(lang_code, "de", 2) == 0) return "Datei %s kann nicht gelesen werden.\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile leggere il file %s.\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι δυνατή η ανάγνωση του αρχείου %s.\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не удалось прочитать файл %s.\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法读取文件 %s。\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法讀取檔案 %s。\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "ファイル%sを読み込めません。\n";
        return "Cannot read file %s.\n";
    }
    if (strcmp(msg, "Error: %s: %d: %s\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Error: %s: %d: %s\n";
        if (strncmp(lang_code, "es", 2) == 0) return "Error: %s: %d: %s\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Erreur: %s: %d: %s\n";
        if (strncmp(lang_code, "de", 2) == 0) return "Fehler: %s: %d: %s\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Errore: %s: %d: %s\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Σφάλμα: %s: %d: %s\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Ошибка: %s: %d: %s\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "错误: %s: %d: %s\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "錯誤: %s: %d: %s\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "エラー: %s: %d: %s\n";
        return "Error: %s: %d: %s\n";
    }
    if (strcmp(msg, "Adding file %s\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Adding file %s\n";
        if (strncmp(lang_code, "es", 2) == 0) return "Añadiendo el archivo %s\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Ajout du fichier %s\n";
        if (strncmp(lang_code, "de", 2) == 0) return "Datei %s wird hinzugefügt\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Aggiunta del file %s\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Προσθήκη του αρχείου %s\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Добавление файла %s\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "正在添加文件 %s\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "正在新增檔案 %s\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "ファイル%sを追加しています。\n";
        return "Adding file %s\n";
    }
    if (strcmp(msg, "Cannot find %s.\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Cannot find %s.\n";
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede encontrar %s.\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible de trouver %s.\n";
        if (strncmp(lang_code, "de", 2) == 0) return "%s konnte nicht gefunden werden.\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile trovare %s.\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν ήταν δυνατή η εύρεση του %s.\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не удалось найти %s.\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法找到 %s。\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法找到 %s。\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "%sがみつかりません。\n";
        if (strncmp(lang_code, "ca", 2) == 0) return "No es pot trobar %s.\n";
        return "Cannot find %s.\n";
    }
    if (strcmp(msg, "Searching directory %s.\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Searching directory %s.\n";
        if (strncmp(lang_code, "es", 2) == 0) return "Buscando en el directorio %s.\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Recherche dans le répertoire %s.\n";
        if (strncmp(lang_code, "de", 2) == 0) return "Durchsuche Verzeichnis %s.\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Ricerca nella directory %s.\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Αναζήτηση στον κατάλογο %s.\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Поиск в каталоге %s.\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "正在搜索目录 %s。\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "正在搜尋目錄 %s。\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "ディレクトリ%sを検索中です。\n";
        return "Searching directory %s.\n";
    }
    if (strcmp(msg, "Skipping empty directory %s.\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Skipping empty directory %s.\n";
        if (strncmp(lang_code, "es", 2) == 0) return "Omitiendo el directorio vacío %s.\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Répertoire vide ignoré : %s.\n";
        if (strncmp(lang_code, "de", 2) == 0) return "Leeres Verzeichnis %s wird übersprungen.\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Directory vuota %s ignorata.\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Παράλειψη του κενoύ καταλόγου %s.\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Пропуск пустого каталога %s.\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "跳过空目录 %s。\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "跳過空目錄 %s。\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "空のディレクトリ%sをスキップしています。\n";
        return "Skipping empty directory %s.\n";
    }
    if (strcmp(msg, "Unknown option %s.\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Unknown option %s.\n";
        if (strncmp(lang_code, "es", 2) == 0) return "Opción desconocida %s.\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Option inconnue : %s.\n";
        if (strncmp(lang_code, "de", 2) == 0) return "Unbekannte Option %s.\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Opzione sconosciuta: %s.\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Άγνωστη επιλογή %s.\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Неизвестная опция %s.\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "未知选项 %s。\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "未知選項 %s。\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "不明なオプション%s\n";
        return "Unknown option %s.\n";
    }
    if (strcmp(msg, "Specify a file.\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Specify a file.\n";
        if (strncmp(lang_code, "es", 2) == 0) return "Especifique un archivo.\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Veuillez spécifier un fichier.\n";
        if (strncmp(lang_code, "de", 2) == 0) return "Bitte eine Datei angeben.\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Specificare un file.\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Καθορίστε ένα αρχείο.\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Укажите файл.\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "请指定一个文件。\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "請指定一個檔案。\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "ファイルを指定してください。\n";
        return "Specify a file.\n";
    }
    if (strcmp(msg, "%s:%d: Error: %s\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "%s:%d: Error: %s\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "%s:%d: Erreur: %s\n";
        if (strncmp(lang_code, "de", 2) == 0) return "%s:%d: Fehler: %s\n";
        if (strncmp(lang_code, "it", 2) == 0) return "%s:%d: Errore: %s\n";
        if (strncmp(lang_code, "el", 2) == 0) return "%s:%d: Σφάλμα: %s\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "%s:%d: Ошибка: %s\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "%s:%d: 错误: %s\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "%s:%d: 錯誤: %s\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "%s:%d: エラー: %s\n";
        return "%s:%d: Error: %s\n";
    }
    if (strcmp(msg, "Out of memory.\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Out of memory.\n";
        if (strncmp(lang_code, "es", 2) == 0) return "Memoria insuficiente.\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Mémoire insuffisante.\n";
        if (strncmp(lang_code, "de", 2) == 0) return "Nicht genügend Speicher.\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Memoria insufficiente.\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν υπάρχει αρκετή μνήμη.\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недостаточно памяти.\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "内存不足。\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "記憶體不足。\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "メモリが足りません。\n";
        return "Out of memory.\n";
    }
    if (strcmp(msg, "Noct Programming Language\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Noct Programming Language\n";
        if (strncmp(lang_code, "es", 2) == 0) return "Lenguaje de programación Noct\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Langage de programmation Noct\n";
        if (strncmp(lang_code, "de", 2) == 0) return "Programmiersprache Noct\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Linguaggio di programmazione Noct\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Γλώσσα προγραμματισμού Noct\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Язык программирования Noct\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "Noct 编程语言\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "Noct 程式語言\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "Noct プログラミング言語\n";
        return "Noct Programming Language\n";
    }
    if (strcmp(msg, "Version %s\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Version %s\n";
        if (strncmp(lang_code, "es", 2) == 0) return "Versión %s\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Version %s\n";
        if (strncmp(lang_code, "de", 2) == 0) return "Version %s\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Versione %s\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Έκδοση %s\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Версия %s\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "版本 %s\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "版本 %s\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "バージョン %s\n";
        return "Version %s\n";
    }
    if (strcmp(msg, "Entering REPL mode.\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Entering REPL mode.\n";
        if (strncmp(lang_code, "es", 2) == 0) return "Entrando en modo REPL.\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Entrée en mode REPL.\n";
        if (strncmp(lang_code, "de", 2) == 0) return "REPL-Modus wird gestartet.\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Accesso alla modalità REPL.\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Είσοδος σε λειτουργία REPL.\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Переход в режим REPL.\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "进入 REPL 模式。\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "進入 REPL 模式。\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "REPLモードに入ります。\n";
        return "Entering REPL mode.\n";
    }
    if (strcmp(msg, "JIT compilation is enabled. Starting the fast VM...\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "JIT compilation is enabled. Starting the fast VM...\n";
        if (strncmp(lang_code, "es", 2) == 0) return "La compilación JIT está activada. Iniciando la máquina virtual rápida...\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "La compilation JIT est activée. Démarrage de la machine virtuelle rapide...\n";
        if (strncmp(lang_code, "de", 2) == 0) return "JIT-Kompilierung ist aktiviert. Starte die schnelle VM...\n";
        if (strncmp(lang_code, "it", 2) == 0) return "La compilazione JIT è attivata. Avvio della VM veloce...\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Η μεταγλώττιση JIT είναι ενεργοποιημένη. Εκκίνηση της γρήγορης εικονικής μηχανής...\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "JIT-компиляция включена. Запуск быстрой виртуальной машины...\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "触发编译已启用。正在启动快速虚拟机...\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "觸發編譯已啟用。正在啟動快速虛擬機器...\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "JIT コンパイルが有効です。高速仮想マシンを開始します...\n";
        return "JIT compilation is enabled. Starting the fast VM...\n";
    }
    if (strcmp(msg, "Usage\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "Usage\n";
        if (strncmp(lang_code, "es", 2) == 0) return "Uso\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "Utilisation\n";
        if (strncmp(lang_code, "de", 2) == 0) return "Verwendung\n";
        if (strncmp(lang_code, "it", 2) == 0) return "Utilizzo\n";
        if (strncmp(lang_code, "el", 2) == 0) return "Χρήση\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "Использование\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "用法\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "用法\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "使い方\n";
        return "Usage\n";
    }
    if (strcmp(msg, "  noct <vm-options> <files>           ... run a program\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "  noct <vm-options> <files>           ... run a program\n";
        if (strncmp(lang_code, "es", 2) == 0) return "  noct <vm-options> <files>           ... ejecutar un programa\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "  noct <vm-options> <files>           ... exécuter un programme\n";
        if (strncmp(lang_code, "de", 2) == 0) return "  noct <vm-options> <files>           ... ein Programm ausführen\n";
        if (strncmp(lang_code, "it", 2) == 0) return "  noct <vm-options> <files>           ... eseguire un programma\n";
        if (strncmp(lang_code, "el", 2) == 0) return "  noct <vm-options> <files>           ... εκτέλεση προγράμματος\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "  noct <vm-options> <files>           ... запуск программы\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "  noct <vm-options> <files>           ... 运行程序\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "  noct <vm-options> <files>           ... 執行程式\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "  noct <vm-options> <入力...>         ... プログラムを実行します\n";
        return "  noct <vm-options> <files>           ... run a program\n";
    }
        if (strncmp(lang_code, "  ", 2) == 0) return "  noct --compile <in-files>           ... convert to bytecode files\n\"));";
    if (strcmp(msg, "  noct --compile <in-files>           ... convert to bytecode files\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "  noct --compile <in-files>           ... convert to bytecode files\n";
        if (strncmp(lang_code, "es", 2) == 0) return "  noct --compile <in-files>           ... convertir en archivos de bytecode\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "  noct --compile <in-files>           ... convertir en fichiers bytecode\n";
        if (strncmp(lang_code, "de", 2) == 0) return "  noct --compile <in-files>           ... in Bytecode-Dateien umwandeln\n";
        if (strncmp(lang_code, "it", 2) == 0) return "  noct --compile <in-files>           ... convertire in file di bytecode\n";
        if (strncmp(lang_code, "el", 2) == 0) return "  noct --compile <in-files>           ... μετατροπή σε αρχεία bytecode\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "  noct --compile <in-files>           ... преобразовать в байткод-файлы\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "  noct --compile <in-files>           ... 转换为字节码文件\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "  noct --compile <in-files>           ... 轉換為位元組碼檔案\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "  noct --compile <入力...>            ... バイトコードに変換します\n";
        return "  noct --compile <in-files>           ... convert to bytecode files\n";
    }
    if (strcmp(msg, "  noct --ansic <out-file> <in-files>  ... convert to a C source file\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "  noct --ansic <out-file> <in-files>  ... convert to a C source file\n";
        if (strncmp(lang_code, "es", 2) == 0) return "  noct --ansic <out-file> <in-files>  ... convertir en un archivo fuente en C\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "  noct --ansic <out-file> <in-files>  ... convertir en un fichier source C\n";
        if (strncmp(lang_code, "de", 2) == 0) return "  noct --ansic <out-file> <in-files>  ... in eine C-Quelldatei umwandeln\n";
        if (strncmp(lang_code, "it", 2) == 0) return "  noct --ansic <out-file> <in-files>  ... convertire in un file sorgente C\n";
        if (strncmp(lang_code, "el", 2) == 0) return "  noct --ansic <out-file> <in-files>  ... μετατροπή σε αρχείο πηγαίου κώδικα C\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "  noct --ansic <out-file> <in-files>  ... преобразовать в исходный файл C\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "  noct --ansic <out-file> <in-files>  ... 转换为 C 源代码文件\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "  noct --ansic <out-file> <in-files>  ... 轉換為 C 原始碼檔案\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "  noct --ansic <出力> <入力...>       ... C ソースに変換します\n";
        return "  noct --ansic <out-file> <in-files>  ... convert to a C source file\n";
    }
    if (strcmp(msg, "  noct --elisp <out-file> <in-files>  ... convert to an Emacs Lisp source file\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "  noct --elisp <out-file> <in-files>  ... convert to an Emacs Lisp source file\n";
        if (strncmp(lang_code, "es", 2) == 0) return "  noct --elisp <out-file> <in-files>  ... convertir en un archivo fuente Emacs Lisp\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "  noct --elisp <out-file> <in-files>  ... convertir en un fichier source Emacs Lisp\n";
        if (strncmp(lang_code, "de", 2) == 0) return "  noct --elisp <out-file> <in-files>  ... in eine Emacs-Lisp-Quelldatei umwandeln\n";
        if (strncmp(lang_code, "it", 2) == 0) return "  noct --elisp <out-file> <in-files>  ... convertire in un file sorgente Emacs Lisp\n";
        if (strncmp(lang_code, "el", 2) == 0) return "  noct --elisp <out-file> <in-files>  ... μετατροπή σε αρχείο πηγαίου κώδικα Emacs Lisp\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "  noct --elisp <out-file> <in-files>  ... преобразовать в исходный файл Emacs Lisp\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "  noct --elisp <out-file> <in-files>  ... 转换为 Emacs Lisp 源代码文件\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "  noct --elisp <out-file> <in-files>  ... 轉換為 Emacs Lisp 原始碼檔案\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "  noct --elisp <出力> <入力...>       ... Emacs Lisp ソースに変換します\n";
        return "  noct --elisp <out-file> <in-files>  ... convert to an Emacs Lisp source file\n";
    }
    if (strcmp(msg, "  noct --scheme <out-file> <in-files> ... convert to an Scheme source file\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "  noct --scheme <out-file> <in-files> ... convert to an Scheme source file\n";
        if (strncmp(lang_code, "es", 2) == 0) return "  noct --scheme <out-file> <in-files> ... convertir en un archivo fuente Scheme\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "  noct --scheme <out-file> <in-files> ... convertir en un fichier source Scheme\n";
        if (strncmp(lang_code, "de", 2) == 0) return "  noct --scheme <out-file> <in-files> ... in eine Scheme-Quelldatei umwandeln\n";
        if (strncmp(lang_code, "it", 2) == 0) return "  noct --scheme <out-file> <in-files> ... convertire in un file sorgente Scheme\n";
        if (strncmp(lang_code, "el", 2) == 0) return "  noct --scheme <out-file> <in-files> ... μετατροπή σε αρχείο πηγαίου κώδικα Scheme\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "  noct --scheme <out-file> <in-files> ... преобразовать в исходный файл Scheme\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "  noct --scheme <out-file> <in-files> ... 转换为 Scheme 源代码文件\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "  noct --scheme <out-file> <in-files> ... 轉換為 Scheme 原始碼檔案\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "  noct --scheme <出力> <入力...>      ... Scheme ソースに変換します\n";
        return "  noct --scheme <out-file> <in-files> ... convert to an Scheme source file\n";
    }
    if (strcmp(msg, "  --disable-jit        ... disable JIT\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "  --disable-jit        ... disable JIT\n";
        if (strncmp(lang_code, "es", 2) == 0) return "  --disable-jit        ... desactivar JIT\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "  --disable-jit        ... désactiver le JIT\n";
        if (strncmp(lang_code, "de", 2) == 0) return "  --disable-jit        ... JIT deaktivieren\n";
        if (strncmp(lang_code, "it", 2) == 0) return "  --disable-jit        ... disabilita JIT\n";
        if (strncmp(lang_code, "el", 2) == 0) return "  --disable-jit        ... απενεργοποίηση JIT\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "  --disable-jit        ... отключить JIT\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "  --disable-jit        ... 禁用 JIT\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "  --disable-jit        ... 停用 JIT\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "  --disable-jit        ... JIT を無効にする\n";
        return "  --disable-jit        ... disable JIT\n";
    }
    if (strcmp(msg, "  --force-jit          ... equivalent to --jit-threshold=0\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "  --force-jit          ... equivalent to --jit-threshold=0\n";
        if (strncmp(lang_code, "es", 2) == 0) return "  --force-jit          ... equivalente a --jit-threshold=0\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "  --force-jit          ... équivalent à --jit-threshold=0\n";
        if (strncmp(lang_code, "de", 2) == 0) return "  --force-jit          ... entspricht --jit-threshold=0\n";
        if (strncmp(lang_code, "it", 2) == 0) return "  --force-jit          ... equivalente a --jit-threshold=0\n";
        if (strncmp(lang_code, "el", 2) == 0) return "  --force-jit          ... ισοδύναμο με --jit-threshold=0\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "  --force-jit          ... эквивалентно --jit-threshold=0\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "  --force-jit          ... 等同于 --jit-threshold=0\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "  --force-jit          ... 等同於 --jit-threshold=0\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "  --force-jit          ... --jit-threshold=0 と同等\n";
        return "  --force-jit          ... equivalent to --jit-threshold=0\n";
    }
    if (strcmp(msg, "  --jit-threshold=N    ... call-count threshold for compilation\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "  --jit-threshold=N    ... call-count threshold for compilation\n";
        if (strncmp(lang_code, "es", 2) == 0) return "  --jit-threshold=N    ... umbral de número de llamadas para la compilación\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "  --jit-threshold=N    ... seuil du nombre d'appels pour la compilation\n";
        if (strncmp(lang_code, "de", 2) == 0) return "  --jit-threshold=N    ... Aufrufanzahl-Schwellenwert für die Kompilierung\n";
        if (strncmp(lang_code, "it", 2) == 0) return "  --jit-threshold=N    ... soglia del numero di chiamate per la compilazione\n";
        if (strncmp(lang_code, "el", 2) == 0) return "  --jit-threshold=N    ... όριο αριθμού κλήσεων για μεταγλώττιση\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "  --jit-threshold=N    ... порог количества вызовов для компиляции\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "  --jit-threshold=N    ... 触发编译的调用次数阈值\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "  --jit-threshold=N    ... 觸發編譯的呼叫次數閾值\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "  --jit-threshold=N    ... コンパイルを行う呼び出し回数のしきい値\n";
        return "  --jit-threshold=N    ... call-count threshold for compilation\n";
    }
    if (strcmp(msg, "  --optimize-level=N   ... optimize level (0/1)\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "  --optimize-level=N   ... optimize level (0/1)\n";
        if (strncmp(lang_code, "es", 2) == 0) return "  --optimize-level=N   ... nivel de optimización (0/1)\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "  --optimize-level=N   ... niveau d’optimisation (0/1)\n";
        if (strncmp(lang_code, "de", 2) == 0) return "  --optimize-level=N   ... Optimierungsstufe (0/1)\n";
        if (strncmp(lang_code, "it", 2) == 0) return "  --optimize-level=N   ... livello di ottimizzazione (0/1)\n";
        if (strncmp(lang_code, "el", 2) == 0) return "  --optimize-level=N   ... επίπεδο βελτιστοποίησης (0/1)\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "  --optimize-level=N   ... уровень оптимизации (0/1)\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "  --optimize-level=N   ... 优化级别 (0/1)\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "  --optimize-level=N   ... 最佳化等級 (0/1)\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "  --optimize-level=N   ... 最適化レベル (0/1)\n";
        return "  --optimize-level=N   ... optimize level (0/1)\n";
    }
    if (strcmp(msg, "  --gc-nursery-size=N  ... first GC space size in bytes\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "  --gc-nursery-size=N  ... first GC space size in bytes\n";
        if (strncmp(lang_code, "es", 2) == 0) return "  --gc-nursery-size=N  ... tamaño del primer espacio GC en bytes\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "  --gc-nursery-size=N  ... taille du premier espace GC en octets\n";
        if (strncmp(lang_code, "de", 2) == 0) return "  --gc-nursery-size=N  ... Größe des ersten GC-Speicherbereichs in Byte\n";
        if (strncmp(lang_code, "it", 2) == 0) return "  --gc-nursery-size=N  ... dimensione del primo spazio GC in byte\n";
        if (strncmp(lang_code, "el", 2) == 0) return "  --gc-nursery-size=N  ... μέγεθος του πρώτου χώρου GC σε byte\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "  --gc-nursery-size=N  ... размер первой области GC в байтах\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "  --gc-nursery-size=N  ... 第一 GC 空间大小（字节）\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "  --gc-nursery-size=N  ... 第一 GC 空間大小（位元組）\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "  --gc-nursery-size=N  ... 第 1 GC 領域のサイズ（バイト単位）\n";
        return "  --gc-nursery-size=N  ... first GC space size in bytes\n";
    }
    if (strcmp(msg, "  --gc-graduate-size=N ... second GC space size in bytes\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "  --gc-graduate-size=N ... second GC space size in bytes\n";
        if (strncmp(lang_code, "es", 2) == 0) return "  --gc-graduate-size=N ... tamaño del segundo espacio GC en bytes\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "  --gc-graduate-size=N ... taille du second espace GC en octets\n";
        if (strncmp(lang_code, "de", 2) == 0) return "  --gc-graduate-size=N ... Größe des zweiten GC-Speicherbereichs in Byte\n";
        if (strncmp(lang_code, "it", 2) == 0) return "  --gc-graduate-size=N ... dimensione del secondo spazio GC in byte\n";
        if (strncmp(lang_code, "el", 2) == 0) return "  --gc-graduate-size=N ... μέγεθος του δεύτερου χώρου GC σε byte\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "  --gc-graduate-size=N ... размер второй области GC в байтах\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "  --gc-graduate-size=N ... 第二 GC 空间大小（字节）\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "  --gc-graduate-size=N ... 第二 GC 空間大小（位元組）\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "  --gc-graduate-size=N ... 第 2 GC 領域のサイズ（バイト単位）\n";
        return "  --gc-graduate-size=N ... second GC space size in bytes\n";
    }
    if (strcmp(msg, "  --gc-tenure-size=N   ... final GC space size in bytes\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "  --gc-tenure-size=N   ... final GC space size in bytes\n";
        if (strncmp(lang_code, "es", 2) == 0) return "  --gc-tenure-size=N   ... tamaño del espacio GC final en bytes\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "  --gc-tenure-size=N   ... taille de l’espace GC final en octets\n";
        if (strncmp(lang_code, "de", 2) == 0) return "  --gc-tenure-size=N   ... Größe des finalen GC-Speicherbereichs in Byte\n";
        if (strncmp(lang_code, "it", 2) == 0) return "  --gc-tenure-size=N   ... dimensione dello spazio GC finale in byte\n";
        if (strncmp(lang_code, "el", 2) == 0) return "  --gc-tenure-size=N   ... μέγεθος του τελικού χώρου GC σε byte\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "  --gc-tenure-size=N   ... размер финальной области GC в байтах\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "  --gc-tenure-size=N   ... 最终 GC 空间大小（字节）\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "  --gc-tenure-size=N   ... 最終 GC 空間大小（位元組）\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "  --gc-tenure-size=N   ... 最終 GC 領域のサイズ（バイト単位）\n";
        return "  --gc-tenure-size=N   ... final GC space size in bytes\n";
    }
    if (strcmp(msg, "  --gc-lop-threshold=N ... move objects larger than N-bytes to final GC space\n") == 0) {
        if (strncmp(lang_code, "en", 2) == 0) return "  --gc-lop-threshold=N ... move objects larger than N-bytes to final GC space\n";
        if (strncmp(lang_code, "es", 2) == 0) return "  --gc-lop-threshold=N ... mover objetos mayores de N bytes al espacio GC final\n";
        if (strncmp(lang_code, "fr", 2) == 0) return "  --gc-lop-threshold=N ... deplacer les objets de plus de N octets vers lespace GC final\n";
        if (strncmp(lang_code, "de", 2) == 0) return "  --gc-lop-threshold=N ... Objekte groser als N Byte in den finalen GC-Speicherbereich verschieben\n";
        if (strncmp(lang_code, "it", 2) == 0) return "  --gc-lop-threshold=N ... sposta gli oggetti più grandi di N byte nello spazio GC finale\n";
        if (strncmp(lang_code, "el", 2) == 0) return "  --gc-lop-threshold=N ... μεταφορά αντικειμένων μεγαλύτερων από N byte στον τελικό χώρο GC\n";
        if (strncmp(lang_code, "ru", 2) == 0) return "  --gc-lop-threshold=N ... перемещать объекты больше N байт в финальную область GC\n";
        if (strncmp(lang_code, "zh", 2) == 0) return "  --gc-lop-threshold=N ... 将大于 N 字节的对象移动到最终 GC 空间\n";
        if (strncmp(lang_code, "tw", 2) == 0) return "  --gc-lop-threshold=N ... 將大於 N 位元組的物件移動到最終 GC 空間\n";
        if (strncmp(lang_code, "ja", 2) == 0) return "  --gc-lop-threshold=N ... N バイトを超えるオブジェクトを最終 GC 領域へ移動\n";
        return "  --gc-lop-threshold=N ... move objects larger than N-bytes to final GC space\n";
    }
    return msg;
}
