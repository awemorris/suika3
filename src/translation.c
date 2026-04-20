#include <string.h>

const char *pf_get_system_language(void);

const char *s3_gettext(const char *msg)
{
    const char *lang_code = pf_get_system_language();
    if (strcmp(msg, "%s:%d: %s") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "%s:%d: %s";
        if (strncmp(lang_code, "fr", 2) == 0) return "%s:%d : %s";
        if (strncmp(lang_code, "it", 2) == 0) return "%s:%d: %s";
        if (strncmp(lang_code, "de", 2) == 0) return "%s:%d: %s";
        if (strncmp(lang_code, "el", 2) == 0) return "%s:%d: %s";
        if (strncmp(lang_code, "ru", 2) == 0) return "%s:%d: %s";
        if (strncmp(lang_code, "zh", 2) == 0) return "%s:%d: %s";
        if (strncmp(lang_code, "tw", 2) == 0) return "%s:%d: %s";
        if (strncmp(lang_code, "ja", 2) == 0) return "%s:%d: %s";
        return "%s:%d: %s";
    }
    if (strcmp(msg, "Error: %s:%d: %s") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Error: %s:%d: %s";
        if (strncmp(lang_code, "fr", 2) == 0) return "Erreur : %s:%d : %s";
        if (strncmp(lang_code, "it", 2) == 0) return "Errore: %s:%d: %s";
        if (strncmp(lang_code, "de", 2) == 0) return "Fehler: %s:%d: %s";
        if (strncmp(lang_code, "el", 2) == 0) return "Σφάλμα: %s:%d: %s";
        if (strncmp(lang_code, "ru", 2) == 0) return "Ошибка: %s:%d: %s";
        if (strncmp(lang_code, "zh", 2) == 0) return "错误: %s:%d: %s";
        if (strncmp(lang_code, "tw", 2) == 0) return "錯誤: %s:%d: %s";
        if (strncmp(lang_code, "ja", 2) == 0) return "エラー: %s:%d: %s";
        return "Error: %s:%d: %s";
    }
    if (strcmp(msg, "Error: %s:%d: Tag %s: ") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Error: %s:%d: Tag %s: ";
        if (strncmp(lang_code, "fr", 2) == 0) return "Erreur : %s:%d : Tag %s: ";
        if (strncmp(lang_code, "it", 2) == 0) return "Errore: %s:%d: Tag %s: ";
        if (strncmp(lang_code, "de", 2) == 0) return "Fehler: %s:%d: Tag %s: ";
        if (strncmp(lang_code, "el", 2) == 0) return "Σφάλμα: %s:%d: Tag %s: ";
        if (strncmp(lang_code, "ru", 2) == 0) return "Ошибка: %s:%d: Tag %s: ";
        if (strncmp(lang_code, "zh", 2) == 0) return "错误: %s:%d: Tag %s: ";
        if (strncmp(lang_code, "tw", 2) == 0) return "錯誤: %s:%d: Tag %s: ";
        if (strncmp(lang_code, "ja", 2) == 0) return "エラー: %s:%d: Tag %s: ";
        return "Error: %s:%d: Tag %s: ";
    }
    if (strcmp(msg, "Error: %s:%d: Runtime error") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Error: %s:%d: Error de ejecución";
        if (strncmp(lang_code, "fr", 2) == 0) return "Erreur : %s:%d: Erreur d'exécution";
        if (strncmp(lang_code, "it", 2) == 0) return "Errore: %s:%d: Erreur d'esecuzione";
        if (strncmp(lang_code, "de", 2) == 0) return "Fehler: %s:%d: Laufzeitfehler";
        if (strncmp(lang_code, "el", 2) == 0) return "Σφάλμα: %s:%d: Σφάλμα κατά την εκτέλεση";
        if (strncmp(lang_code, "ru", 2) == 0) return "Ошибка: %s:%d: Ошибка времени выполнения";
        if (strncmp(lang_code, "zh", 2) == 0) return "错误: %s:%d: 运行时错误";
        if (strncmp(lang_code, "tw", 2) == 0) return "錯誤: %s:%d: 執行階段錯誤";
        if (strncmp(lang_code, "ja", 2) == 0) return "エラー: %s:%d: 実行時エラー";
        return "Error: %s:%d: Runtime error";
    }
    if (strcmp(msg, "Error: %s:%d: Tag \"%s\" not found.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Error: %s:%d: No se encontró la etiqueta \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Erreur : %s:%d : Balise \"%s\" introuvable.";
        if (strncmp(lang_code, "it", 2) == 0) return "Errore: %s:%d: Tag \"%s\" non trovato.";
        if (strncmp(lang_code, "de", 2) == 0) return "Fehler: %s:%d: Tag \"%s\" wurde nicht gefunden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Σφάλμα: %s:%d: Η ετικέτα \"%s\" δεν βρέθηκε.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Ошибка: %s:%d: Тег \"%s\" не найден.";
        if (strncmp(lang_code, "zh", 2) == 0) return "错误: %s:%d: 未找到标签 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "錯誤: %s:%d: 找不到標籤 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "エラー: %s:%d: タグ \"%s\" がみつかりません。";
        return "Error: %s:%d: Tag \"%s\" not found.";
    }
    if (strcmp(msg, "Error: %s:%d: \"Tag_%s\" is not a function.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Error: %s:%d: \"Tag_%s\" no es una función.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Erreur : %s:%d : \"Tag_%s\" n'est pas une fonction.";
        if (strncmp(lang_code, "it", 2) == 0) return "Errore: %s:%d: \"Tag_%s\" non è una funzione.";
        if (strncmp(lang_code, "de", 2) == 0) return "Fehler: %s:%d: \"Tag_%s\" ist keine Funktion.";
        if (strncmp(lang_code, "el", 2) == 0) return "Σφάλμα: %s:%d: Το \"Tag_%s\" δεν είναι συνάρτηση.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Ошибка: %s:%d: «Tag_%s» не является функцией.";
        if (strncmp(lang_code, "zh", 2) == 0) return "错误: %s:%d: \"Tag_%s\" 不是函数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "錯誤: %s:%d: \"Tag_%s\" 不是函式。";
        if (strncmp(lang_code, "ja", 2) == 0) return "エラー: %s:%d: \"Tag_%s\" は関数ではありません。";
        return "Error: %s:%d: \"Tag_%s\" is not a function.";
    }
    if (strcmp(msg, "Argument \"%s\" not specified.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se ha especificado el argumento \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Argument \"%s\" non spécifié.";
        if (strncmp(lang_code, "it", 2) == 0) return "Argomento \"%s\" non specificato.";
        if (strncmp(lang_code, "de", 2) == 0) return "Argument \"%s\" nicht angegeben.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το όρισμα \"%s\" δεν έχει οριστεί.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Аргумент «%s» не указан.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未指定参数 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未指定引數 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "引数 \"%s\" が指定されていません。";
        return "Argument \"%s\" not specified.";
    }
    if (strcmp(msg, "Cannot play sound file \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede reproducir el archivo de sonido \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible de lire le fichier sonore \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile riprodurre il file audio \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Sounddatei \"%s\" kann nicht abgespielt werden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι δυνατή η αναπαραγωγή του αρχείου ήχου \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Невозможно воспроизвести звуковой файл «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法播放音频文件 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法播放音訊檔案 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "サウンドファイル \"%s\" を再生できません。";
        return "Cannot play sound file \"%s\".";
    }
    if (strcmp(msg, "Conifg \"%s\" not found.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se encontró la configuración \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Configuration \"%s\" introuvable.";
        if (strncmp(lang_code, "it", 2) == 0) return "Configurazione \"%s\" non trovata.";
        if (strncmp(lang_code, "de", 2) == 0) return "Konfiguration \"%s\" nicht gefunden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η ρύθμιση \"%s\" δεν βρέθηκε.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Конфигурация «%s» не найдена.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未找到配置 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "找不到設定 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "設定 \"%s\" が見つかりません。";
        return "Conifg \"%s\" not found.";
    }
    if (strcmp(msg, "Duplicated config key %s.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Clave de configuración duplicada %s.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Clé de configuration en double %s.";
        if (strncmp(lang_code, "it", 2) == 0) return "Chiave di configurazione duplicata %s.";
        if (strncmp(lang_code, "de", 2) == 0) return "Doppelter Konfigurationsschlüssel %s.";
        if (strncmp(lang_code, "el", 2) == 0) return "Διπλότυπο κλειδί ρύθμισης %s.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Дублирующийся ключ конфигурации %s.";
        if (strncmp(lang_code, "zh", 2) == 0) return "配置键 %s 重复。";
        if (strncmp(lang_code, "tw", 2) == 0) return "設定鍵 %s 重複。";
        if (strncmp(lang_code, "ja", 2) == 0) return "設定キー %s が重複しています。";
        return "Duplicated config key %s.";
    }
    if (strcmp(msg, "Error loading GUI file %s.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Error al cargar el archivo GUI %s.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Erreur lors du chargement du fichier GUI %s.";
        if (strncmp(lang_code, "it", 2) == 0) return "Errore durante il caricamento del file GUI %s.";
        if (strncmp(lang_code, "de", 2) == 0) return "Fehler beim Laden der GUI-Datei %s.";
        if (strncmp(lang_code, "el", 2) == 0) return "Σφάλμα κατά τη φόρτωση του αρχείου GUI %s.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Ошибка загрузки GUI-файла %s.";
        if (strncmp(lang_code, "zh", 2) == 0) return "加载 GUI 文件 %s 時出错。";
        if (strncmp(lang_code, "tw", 2) == 0) return "載入 GUI 檔案 %s 時發生錯誤。";
        if (strncmp(lang_code, "ja", 2) == 0) return "GUIファイル %s の読み込み中にエラーが発生しました。";
        return "Error loading GUI file %s.";
    }
    if (strcmp(msg, "Failed to write save data.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Error al escribir los datos de guardado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Échec de l'écriture des données de sauvegarde.";
        if (strncmp(lang_code, "it", 2) == 0) return "Scrittura dei dati di salvataggio fallita.";
        if (strncmp(lang_code, "de", 2) == 0) return "Fehler beim Schreiben der Spieldaten.";
        if (strncmp(lang_code, "el", 2) == 0) return "Αποτυχία εγγραφής δεδομένων αποθήκευσης.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Ошибка записи данных сохранения.";
        if (strncmp(lang_code, "zh", 2) == 0) return "保存数据写入失败。";
        if (strncmp(lang_code, "tw", 2) == 0) return "儲存存檔資料失敗。";
        if (strncmp(lang_code, "ja", 2) == 0) return "セーブデータの書き込みに失敗しました。";
        return "Failed to write save data.";
    }
    if (strcmp(msg, "File \"%s\" not exist.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "El archivo \"%s\" no existe.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Le fichier \"%s\" n'existe pas.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il file \"%s\" non esiste.";
        if (strncmp(lang_code, "de", 2) == 0) return "Datei \"%s\" existiert nicht.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το αρχείο \"%s\" δεν υπάρχει.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Файл «%s» не существует.";
        if (strncmp(lang_code, "zh", 2) == 0) return "文件 \"%s\" 不存在。";
        if (strncmp(lang_code, "tw", 2) == 0) return "檔案 \"%s\" 不存在。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ファイル \"%s\" がありません。";
        return "File \"%s\" not exist.";
    }
    if (strcmp(msg, "Internal error.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Error interno.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Erreur interne.";
        if (strncmp(lang_code, "it", 2) == 0) return "Errore interno.";
        if (strncmp(lang_code, "de", 2) == 0) return "Interner Fehler.";
        if (strncmp(lang_code, "el", 2) == 0) return "Εσωτερικό σφάλμα.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Внутренняя ошибка.";
        if (strncmp(lang_code, "zh", 2) == 0) return "内部错误。";
        if (strncmp(lang_code, "tw", 2) == 0) return "內部錯誤。";
        if (strncmp(lang_code, "ja", 2) == 0) return "内部エラーが発生しました。";
        return "Internal error.";
    }
    if (strcmp(msg, "Invalid EOF on GUI file %s.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "EOF no válido en el archivo GUI %s.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Fin de fichier (EOF) non valide dans le fichier GUI %s.";
        if (strncmp(lang_code, "it", 2) == 0) return "EOF non valido nel file GUI %s.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültiges Dateiende (EOF) in GUI-Datei %s.";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρο τέλος αρχείου (EOF) στο αρχείο GUI %s.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимый EOF в GUI-файле %s.";
        if (strncmp(lang_code, "zh", 2) == 0) return "GUI 文件 %s 中的 EOF 无效。";
        if (strncmp(lang_code, "tw", 2) == 0) return "GUI 檔案 %s 中的 EOF 無效。";
        if (strncmp(lang_code, "ja", 2) == 0) return "GUIファイル %s の終端が不正です。";
        return "Invalid EOF on GUI file %s.";
    }
    if (strcmp(msg, "Invalid EOF while parsing anime file %s") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "EOF no válido al analizar el archivo de animación %s";
        if (strncmp(lang_code, "fr", 2) == 0) return "Fin de fichier (EOF) non valide lors de l'analyse du fichier d'animation %s";
        if (strncmp(lang_code, "it", 2) == 0) return "EOF non valido durante l'analisi del file di animazione %s";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültiges Dateiende (EOF) beim Parsen der Animationsdatei %s";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρο τέλος αρχείου (EOF) κατά την ανάλυση του αρχείου κινουμένων σχεδίων %s";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимый EOF при анализе файла анимации %s";
        if (strncmp(lang_code, "zh", 2) == 0) return "解析动画文件 %s 時遇到无效的 EOF。";
        if (strncmp(lang_code, "tw", 2) == 0) return "解析動畫檔案 %s 時遇到無效的 EOF。";
        if (strncmp(lang_code, "ja", 2) == 0) return "アニメーションファイル %s の解析中に不正な終端に達しました。";
        return "Invalid EOF while parsing anime file %s";
    }
    if (strcmp(msg, "Invalid accel type \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Tipo de aceleración \"%s\" no válido.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Type d'accélération \"%s\" non valide.";
        if (strncmp(lang_code, "it", 2) == 0) return "Tipo di accelerazione \"%s\" non valido.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültiger Beschleunigungstyp \"%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρος τύπος επιτάχυνσης \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимый тип ускорения «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "加速类型 \"%s\" 无效。";
        if (strncmp(lang_code, "tw", 2) == 0) return "加速類型 \"%s\" 無效。";
        if (strncmp(lang_code, "ja", 2) == 0) return "加速タイプ \"%s\" は無効です。";
        return "Invalid accel type \"%s\".";
    }
    if (strcmp(msg, "Invalid character \"%c\" while parsing anime file \"%s\" line %d.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Carácter \"%c\" no válido al analizar el archivo de animación \"%s\" en la línea %d.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Caractère \"%c\" non valide lors de l'analyse du fichier d'animation \"%s\" ligne %d.";
        if (strncmp(lang_code, "it", 2) == 0) return "Carattere \"%c\" non valido durante l'analisi del file di animazione \"%s\" riga %d.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültiges Zeichen \"%c\" beim Parsen der Animationsdatei \"%s\" Zeile %d.";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρος χαρακτήρας \"%c\" κατά την ανάλυση του αρχείου κινουμένων σχεδίων \"%s\" γραμμή %d.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимый символ «%c» при анализе файла анимации «%s» в строке %d.";
        if (strncmp(lang_code, "zh", 2) == 0) return "解析动画文件 \"%s\" 第 %d 行時发现无效字符 \"%c\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "解析動畫檔案 \"%s\" 第 %d 行時發現無效字元 \"%c\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "アニメーションファイル \"%s\" の %d 行目を解析中に無効な文字 \"%c\" が見つかりました。";
        return "Invalid character \"%c\" while parsing anime file \"%s\" line %d.";
    }
    if (strcmp(msg, "Invalid character.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Carácter no válido.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Caractère non valide.";
        if (strncmp(lang_code, "it", 2) == 0) return "Carattere non valido.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültiges Zeichen.";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρος χαρακτήρας.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимый символ.";
        if (strncmp(lang_code, "zh", 2) == 0) return "无效字符。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無效字元。";
        if (strncmp(lang_code, "ja", 2) == 0) return "文字が不正です。";
        return "Invalid character.";
    }
    if (strcmp(msg, "Invalid fade method \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Método de desvanecimiento \"%s\" no válido.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Méthode de fondu \"%s\" non valide.";
        if (strncmp(lang_code, "it", 2) == 0) return "Metodo di dissolvenza \"%s\" non valido.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültige Fade-Methode \"%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρη μέθοδος εξασθένισης \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимый метод затухания «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "渐变方法 \"%s\" 无效。";
        if (strncmp(lang_code, "tw", 2) == 0) return "漸變方法 \"%s\" 無效。";
        if (strncmp(lang_code, "ja", 2) == 0) return "フェードメソッド \"%s\" は無効です。";
        return "Invalid fade method \"%s\".";
    }
    if (strcmp(msg, "Invalid fade type \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Tipo de desvanecimiento \"%s\" no válido.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Type de fondu \"%s\" non valide.";
        if (strncmp(lang_code, "it", 2) == 0) return "Tipo di dissolvenza \"%s\" non valido.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültiger Fade-Typ \"%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρος τύπος εξασθένισης \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимый тип затухания «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "渐变类型 \"%s\" 无效。";
        if (strncmp(lang_code, "tw", 2) == 0) return "漸變類型 \"%s\" 無效。";
        if (strncmp(lang_code, "ja", 2) == 0) return "フェードタイプ \"%s\" は無効です。";
        return "Invalid fade type \"%s\".";
    }
    if (strcmp(msg, "Invalid layer name \"%s\" while parsing anime file \"%s\" line %d.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Nombre de capa \"%s\" no válido al analizar el archivo de animación \"%s\" en la línea %d.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Nom de calque \"%s\" non valide lors de l'analyse du fichier d'animation \"%s\" ligne %d.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nome livello \"%s\" non valido durante l'analisi del file di animazione \"%s\" riga %d.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültiger Ebenenname \"%s\" beim Parsen der Animationsdatei \"%s\" Zeile %d.";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρο όνομα επιπέδου \"%s\" κατά την ανάλυση του αρχείου κινουμένων σχεδίων \"%s\" γραμμή %d.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимое имя слоя «%s» при анализе файла анимации «%s» в строке %d.";
        if (strncmp(lang_code, "zh", 2) == 0) return "解析动画文件 \"%s\" 第 %d 行時发现无效图层名 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "解析動畫檔案 \"%s\" 第 %d 行時發現無效圖層名稱 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "アニメーションファイル \"%s\" の %d 行目を解析中に無効なレイヤー名 \"%s\" が見つかりました。";
        return "Invalid layer name \"%s\" while parsing anime file \"%s\" line %d.";
    }
    if (strcmp(msg, "Invalid layer name \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Nombre de capa \"%s\" no válido.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Nom de calque \"%s\" non valide.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nome livello \"%s\" non valido.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültiger Ebenenname \"%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρο όνομα επιπέδου \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимое имя слоя «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "图层名 \"%s\" 无效。";
        if (strncmp(lang_code, "tw", 2) == 0) return "圖層名稱 \"%s\" 無效。";
        if (strncmp(lang_code, "ja", 2) == 0) return "レイヤー名 \"%s\" は無効です。";
        return "Invalid layer name \"%s\".";
    }
    if (strcmp(msg, "Invalid registered anime \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Animación registrada \"%s\" no válida.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Animation enregistrée \"%s\" non valide.";
        if (strncmp(lang_code, "it", 2) == 0) return "Animazione registrata \"%s\" non valida.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültige registrierte Animation \"%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρη εγγεγραμμένη κίνηση \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимая зарегистрированная анимация «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "注册动画 \"%s\" 无效。";
        if (strncmp(lang_code, "tw", 2) == 0) return "註冊動畫 \"%s\" 無效。";
        if (strncmp(lang_code, "ja", 2) == 0) return "登録されているアニメーション \"%s\" は無効です。";
        return "Invalid registered anime \"%s\".";
    }
    if (strcmp(msg, "Invalid track \"%s\" specified.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Pista \"%s\" especificada no válida.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Piste \"%s\" spécifiée non valide.";
        if (strncmp(lang_code, "it", 2) == 0) return "Traccia \"%s\" specificata non valida.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültiger Track \"%s\" angegeben.";
        if (strncmp(lang_code, "el", 2) == 0) return "Ορίστηκε μη έγκυρο κομμάτι \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Указана недопустимая дорожка «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "指定的轨道 \"%s\" 无效。";
        if (strncmp(lang_code, "tw", 2) == 0) return "指定的軌道 \"%s\" 無效。";
        if (strncmp(lang_code, "ja", 2) == 0) return "トラック \"%s\" の指定が不正です。";
        return "Invalid track \"%s\" specified.";
    }
    if (strcmp(msg, "Label \"%s\" not found.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se encontró la etiqueta \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Étiquette \"%s\" introuvable.";
        if (strncmp(lang_code, "it", 2) == 0) return "Etichetta \"%s\" non trovata.";
        if (strncmp(lang_code, "de", 2) == 0) return "Label \"%s\" wurde nicht gefunden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η ετικέτα \"%s\" δεν βρέθηκε.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Метка «%s» не найдена.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未找到标签 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "找不到標籤 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ラベル \"%s\" が見つかりません。";
        return "Label \"%s\" not found.";
    }
    if (strcmp(msg, "Layer not specifed while parsing anime file \"%s\" line %d.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se especificó la capa al analizar el archivo de animación \"%s\" en la línea %d.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Calque non spécifié lors de l'analyse du fichier d'animation \"%s\" ligne %d.";
        if (strncmp(lang_code, "it", 2) == 0) return "Livello non specificato durante l'analisi del file di animazione \"%s\" riga %d.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ebene nicht angegeben beim Parsen der Animationsdatei \"%s\" Zeile %d.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν ορίστηκε επίπεδο κατά την ανάλυση του αρχείου κινουμένων σχεδίων \"%s\" γραμμή %d.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слой не указан при анализе файла анимации «%s» в строке %d.";
        if (strncmp(lang_code, "zh", 2) == 0) return "解析动画文件 \"%s\" 第 %d 行時未指定图层。";
        if (strncmp(lang_code, "tw", 2) == 0) return "解析動畫檔案 \"%s\" 第 %d 行時未指定圖層。";
        if (strncmp(lang_code, "ja", 2) == 0) return "アニメーションファイル \"%s\" の %d 行目を解析中にレイヤーが指定されていません。";
        return "Layer not specifed while parsing anime file \"%s\" line %d.";
    }
    if (strcmp(msg, "Macro \"%s\" not found.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se encontró la macro \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Macro \"%s\" introuvable.";
        if (strncmp(lang_code, "it", 2) == 0) return "Macro \"%s\" non trovata.";
        if (strncmp(lang_code, "de", 2) == 0) return "Makro \"%s\" wurde nicht gefunden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η μακροεντολή \"%s\" δεν βρέθηκε.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Макрос «%s» не найден.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未找到宏 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "找不到巨集 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "マクロ \"%s\" が見つかりません。";
        return "Macro \"%s\" not found.";
    }
    if (strcmp(msg, "No LHS specified.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se ha especificado LHS (lado izquierdo).";
        if (strncmp(lang_code, "fr", 2) == 0) return "Aucun membre de gauche (LHS) spécifié.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nessun LHS specificato.";
        if (strncmp(lang_code, "de", 2) == 0) return "Kein LHS angegeben.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν ορίστηκε αριστερό μέλος (LHS).";
        if (strncmp(lang_code, "ru", 2) == 0) return "Левая часть (LHS) не указана.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未指定左值。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未指定左值。";
        if (strncmp(lang_code, "ja", 2) == 0) return "左辺が指定されていません。";
        return "No LHS specified.";
    }
    if (strcmp(msg, "No RHS specified.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se ha especificado RHS (lado derecho).";
        if (strncmp(lang_code, "fr", 2) == 0) return "Aucun membre de droite (RHS) spécifié.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nessun RHS specificato.";
        if (strncmp(lang_code, "de", 2) == 0) return "Kein RHS angegeben.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν ορίστηκε δεξί μέλος (RHS).";
        if (strncmp(lang_code, "ru", 2) == 0) return "Правая часть (RHS) не указана.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未指定右值。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未指定右值。";
        if (strncmp(lang_code, "ja", 2) == 0) return "右辺が指定されていません。";
        return "No RHS specified.";
    }
    if (strcmp(msg, "No available button-id for a GUI button in GUI file \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No hay ID de botón disponible para un botón GUI en el archivo GUI \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Aucun ID de bouton disponible pour un bouton GUI dans le fichier GUI \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Nessun ID pulsante disponibile per un pulsante GUI nel file GUI \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Keine verfügbare Button-ID für einen GUI-Button in GUI-Datei \"%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν υπάρχει διαθέσιμο ID κουμπιού για ένα κουμπί GUI στο αρχείο GUI \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Нет доступного ID кнопки для GUI-кнопки в GUI-файле «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "GUI 文件 \"%s\" 中没有可用的按钮 ID。";
        if (strncmp(lang_code, "tw", 2) == 0) return "GUI 檔案 \"%s\" 中沒有可用的按鈕 ID。";
        if (strncmp(lang_code, "ja", 2) == 0) return "GUIファイル \"%s\" に使用可能なボタンIDがありません。";
        return "No available button-id for a GUI button in GUI file \"%s\".";
    }
    if (strcmp(msg, "No correspoinding \"for\" detected.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se detectó el \"for\" correspondiente.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Aucun \"for\" correspondant détecté.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nessun \"for\" corrispondente rilevato.";
        if (strncmp(lang_code, "de", 2) == 0) return "Kein entsprechendes \"for\" gefunden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν εντοπίστηκε αντίστοιχο \"for\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не обнаружен соответствующий цикл «for».";
        if (strncmp(lang_code, "zh", 2) == 0) return "未发现对应的 \"for\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "找不到對應的 \"for\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "対応する \"for\" がありません。";
        return "No correspoinding \"for\" detected.";
    }
    if (strcmp(msg, "No correspoinding \"if\" detected.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se detectó el \"if\" correspondiente.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Aucun \"if\" correspondant détecté.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nessun \"if\" corrispondente rilevato.";
        if (strncmp(lang_code, "de", 2) == 0) return "Kein entsprechendes \"if\" gefunden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν εντοπίστηκε αντίστοιχο \"if\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не обнаружено соответствующее условие «if».";
        if (strncmp(lang_code, "zh", 2) == 0) return "未发现对应的 \"if\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "找不到對應的 \"if\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "対応する \"if\" がありません。";
        return "No correspoinding \"if\" detected.";
    }
    if (strcmp(msg, "No correspoinding \"while\" detected.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se detectó el \"while\" correspondiente.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Aucun \"while\" correspondant détecté.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nessun \"while\" corrispondente rilevato.";
        if (strncmp(lang_code, "de", 2) == 0) return "Kein entsprechendes \"while\" gefunden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν εντοπίστηκε αντίστοιχο \"while\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не обнаружен соответствующий цикл «while».";
        if (strncmp(lang_code, "zh", 2) == 0) return "未发现对应的 \"while\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "找不到對應的 \"while\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "対応する \"while\" がありません。";
        return "No correspoinding \"while\" detected.";
    }
    if (strcmp(msg, "No matching endif found.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se encontró un endif correspondiente.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Aucun endif correspondant trouvé.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nessun endif corrispondente trovato.";
        if (strncmp(lang_code, "de", 2) == 0) return "Kein passendes endif gefunden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν βρέθηκε αντίστοιχο endif.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Соответствующий endif не найден.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未找到匹配的 endif。";
        if (strncmp(lang_code, "tw", 2) == 0) return "找不到對應的 endif。";
        if (strncmp(lang_code, "ja", 2) == 0) return "endif が見つかりません。";
        return "No matching endif found.";
    }
    if (strcmp(msg, "No matching endmacro found.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se encontró un endmacro correspondiente.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Aucun endmacro correspondant trouvé.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nessun endmacro corrispondente trovato.";
        if (strncmp(lang_code, "de", 2) == 0) return "Kein passendes endmacro gefunden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν βρέθηκε αντίστοιχο endmacro.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Соответствующий endmacro не найден.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未找到匹配的 endmacro。";
        if (strncmp(lang_code, "tw", 2) == 0) return "找不到對應的 endmacro。";
        if (strncmp(lang_code, "ja", 2) == 0) return "endmacro が見つかりません。";
        return "No matching endmacro found.";
    }
    if (strcmp(msg, "No operator specified.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se especificó ningún operador.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Aucun opérateur spécifié.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nessun operatore specificato.";
        if (strncmp(lang_code, "de", 2) == 0) return "Kein Operator angegeben.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν ορίστηκε τελεστής.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Оператор не указан.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未指定操作符。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未指定運算子。";
        if (strncmp(lang_code, "ja", 2) == 0) return "演算子が指定されていません。";
        return "No operator specified.";
    }
    if (strcmp(msg, "Property name too long.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Nombre de propiedad demasiado largo.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Nom de propriété trop long.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nome della proprietà troppo lungo.";
        if (strncmp(lang_code, "de", 2) == 0) return "Eigenschaftsname zu lang.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το όνομα της ιδιότητας είναι πολύ μεγάλο.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Имя свойства слишком длинное.";
        if (strncmp(lang_code, "zh", 2) == 0) return "属性名过长。";
        if (strncmp(lang_code, "tw", 2) == 0) return "屬性名稱過長。";
        if (strncmp(lang_code, "ja", 2) == 0) return "プロパティ名が長すぎます。";
        return "Property name too long.";
    }
    if (strcmp(msg, "Property value too long.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Valor de propiedad demasiado largo.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Valeur de propriété trop longue.";
        if (strncmp(lang_code, "it", 2) == 0) return "Valore della proprietà troppo lungo.";
        if (strncmp(lang_code, "de", 2) == 0) return "Eigenschaftswert zu lang.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η τιμή της ιδιότητας είναι πολύ μεγάλη.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Значение свойства слишком длинное.";
        if (strncmp(lang_code, "zh", 2) == 0) return "属性值过长。";
        if (strncmp(lang_code, "tw", 2) == 0) return "屬性值過長。";
        if (strncmp(lang_code, "ja", 2) == 0) return "プロパティ値が長すぎます。";
        return "Property value too long.";
    }
    if (strcmp(msg, "Rule file is missing.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Falta el archivo de reglas.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Le fichier de règles est manquant.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il file delle regole è mancante.";
        if (strncmp(lang_code, "de", 2) == 0) return "Regeldatei fehlt.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το αρχείο κανόνων λείπει.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Файл правил отсутствует.";
        if (strncmp(lang_code, "zh", 2) == 0) return "缺少规则文件。";
        if (strncmp(lang_code, "tw", 2) == 0) return "缺少規則檔案。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ルールファイルがありません。";
        return "Rule file is missing.";
    }
    if (strcmp(msg, "Rule file is not specified.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se ha especificado el archivo de reglas.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Le fichier de règles n'est pas spécifié.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il file delle regole non è specificato.";
        if (strncmp(lang_code, "de", 2) == 0) return "Regeldatei nicht angegeben.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν ορίστηκε αρχείο κανόνων.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Файл правил не указан.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未指定规则文件。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未指定規則檔案。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ルールファイルが指定されていません。";
        return "Rule file is not specified.";
    }
    if (strcmp(msg, "Save data version mismatched.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "La versión de los datos de guardado no coincide.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La version des données de sauvegarde ne correspond pas.";
        if (strncmp(lang_code, "it", 2) == 0) return "Versione dei dati di salvataggio non corrispondente.";
        if (strncmp(lang_code, "de", 2) == 0) return "Version der Spieldaten stimmt nicht überein.";
        if (strncmp(lang_code, "el", 2) == 0) return "Αναντιστοιχία έκδοσης δεδομένων αποθήκευσης.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Версия данных сохранения не совпадает.";
        if (strncmp(lang_code, "zh", 2) == 0) return "存档数据版本不匹配。";
        if (strncmp(lang_code, "tw", 2) == 0) return "存檔版本不符。";
        if (strncmp(lang_code, "ja", 2) == 0) return "セーブデータのバージョンが一致しません。";
        return "Save data version mismatched.";
    }
    if (strcmp(msg, "Sequece too long while parsing anime file \"%s\" line %d.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Secuencia demasiado larga al analizar el archivo de animación \"%s\" en la línea %d.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Séquence trop longue lors de l'analyse du fichier d'animation \"%s\" ligne %d.";
        if (strncmp(lang_code, "it", 2) == 0) return "Sequenza troppo lunga durante l'analisi del file di animazione \"%s\" riga %d.";
        if (strncmp(lang_code, "de", 2) == 0) return "Sequenz zu lang beim Parsen der Animationsdatei \"%s\" Zeile %d.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η σειρά είναι πολύ μεγάλη κατά την ανάλυση του αρχείου κινουμένων σχεδίων \"%s\" γραμμή %d.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком длинная последовательность при анализе файла анимации «%s» в строке %d.";
        if (strncmp(lang_code, "zh", 2) == 0) return "解析动画文件 \"%s\" 第 %d 行時序列过长。";
        if (strncmp(lang_code, "tw", 2) == 0) return "解析動畫檔案 \"%s\" 第 %d 行時序列過長。";
        if (strncmp(lang_code, "ja", 2) == 0) return "アニメーションファイル \"%s\" の %d 行目を解析中にシーケンスが長すぎます。";
        return "Sequece too long while parsing anime file \"%s\" line %d.";
    }
    if (strcmp(msg, "Specified ID is already used in GUI file \"%s\" line %d.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "El ID especificado ya se está usando en el archivo GUI \"%s\" en la línea %d.";
        if (strncmp(lang_code, "fr", 2) == 0) return "L'ID spécifié est déjà utilisé dans le fichier GUI \"%s\" ligne %d.";
        if (strncmp(lang_code, "it", 2) == 0) return "L'ID specificato è già in uso nel file GUI \"%s\" riga %d.";
        if (strncmp(lang_code, "de", 2) == 0) return "Die angegebene ID wird bereits in der GUI-Datei \"%s\" Zeile %d verwendet.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το καθορισμένο ID χρησιμοποιείται ήδη στο αρχείο GUI \"%s\" γραμμή %d.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Указанный ID уже используется в GUI-файле «%s» в строке %d.";
        if (strncmp(lang_code, "zh", 2) == 0) return "指定的 ID 已在 GUI 文件 \"%s\" 第 %d 行中使用。";
        if (strncmp(lang_code, "tw", 2) == 0) return "指定的 ID 已在 GUI 檔案 \"%s\" 第 %d 行中使用。";
        if (strncmp(lang_code, "ja", 2) == 0) return "指定されたIDはGUIファイル \"%s\" の %d 行目ですでに使用されています。";
        return "Specified ID is already used in GUI file \"%s\" line %d.";
    }
    if (strcmp(msg, "Tag name too long.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Nombre de etiqueta demasiado largo.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Nom de balise trop long.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nome del tag troppo lungo.";
        if (strncmp(lang_code, "de", 2) == 0) return "Tag-Name zu lang.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το όνομα της ετικέτας είναι πολύ μεγάλο.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Имя тега слишком длинное.";
        if (strncmp(lang_code, "zh", 2) == 0) return "标签名过长。";
        if (strncmp(lang_code, "tw", 2) == 0) return "標籤名稱過長。";
        if (strncmp(lang_code, "ja", 2) == 0) return "タグ名が長すぎます。";
        return "Tag name too long.";
    }
    if (strcmp(msg, "This API is not implemented yet.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Esta API aún no se ha implementado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Cette API n'est pas encore implémentée.";
        if (strncmp(lang_code, "it", 2) == 0) return "Questa API non è ancora implementata.";
        if (strncmp(lang_code, "de", 2) == 0) return "Diese API ist noch nicht implementiert.";
        if (strncmp(lang_code, "el", 2) == 0) return "Αυτό το API δεν έχει υλοποιηθεί ακόμα.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Этот API еще не реализован.";
        if (strncmp(lang_code, "zh", 2) == 0) return "该 API 尚未实现。";
        if (strncmp(lang_code, "tw", 2) == 0) return "此 API 尚未實作。";
        if (strncmp(lang_code, "ja", 2) == 0) return "このAPIはまだ実装されていません。";
        return "This API is not implemented yet.";
    }
    if (strcmp(msg, "Too long word while parsing anime file \"%s\" line %d.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Palabra demasiado larga al analizar el archivo de animación \"%s\" en la línea %d.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Mot trop long lors de l'analyse du fichier d'animation \"%s\" ligne %d.";
        if (strncmp(lang_code, "it", 2) == 0) return "Parola troppo lunga durante l'analisi del file di animazione \"%s\" riga %d.";
        if (strncmp(lang_code, "de", 2) == 0) return "Wort zu lang beim Parsen der Animationsdatei \"%s\" Zeile %d.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η λέξη είναι πολύ μεγάλη κατά την ανάλυση του αρχείου κινουμένων σχεδίων \"%s\" γραμμή %d.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком длинное слово при анализе файла анимации «%s» в строке %d.";
        if (strncmp(lang_code, "zh", 2) == 0) return "解析动画文件 \"%s\" 第 %d 行時单词过长。";
        if (strncmp(lang_code, "tw", 2) == 0) return "解析動畫檔案 \"%s\" 第 %d 行時單字過長。";
        if (strncmp(lang_code, "ja", 2) == 0) return "アニメーションファイル \"%s\" の %d 行目を解析中に単語が長すぎます。";
        return "Too long word while parsing anime file \"%s\" line %d.";
    }
    if (strcmp(msg, "Too many GUI buttons in GUI file \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Demasiados botones GUI en el archivo GUI \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Trop de boutons GUI dans le fichier GUI \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Troppi pulsanti GUI nel file GUI \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Zu viele GUI-Buttons in GUI-Datei \"%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Πάρα πολλά κουμπιά GUI στο αρχείο GUI \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком много GUI-кнопок в GUI-файле «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "GUI 文件 \"%s\" 中的 GUI 按钮过多。";
        if (strncmp(lang_code, "tw", 2) == 0) return "GUI 檔案 \"%s\" 中的 GUI 按鈕過多。";
        if (strncmp(lang_code, "ja", 2) == 0) return "GUIファイル \"%s\" のGUIボタンが多すぎます。";
        return "Too many GUI buttons in GUI file \"%s\".";
    }
    if (strcmp(msg, "Too many drawmsg contexts.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Demasiados contextos drawmsg.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Trop de contextes drawmsg.";
        if (strncmp(lang_code, "it", 2) == 0) return "Troppi contesti drawmsg.";
        if (strncmp(lang_code, "de", 2) == 0) return "Zu viele drawmsg-Kontexte.";
        if (strncmp(lang_code, "el", 2) == 0) return "Πάρα πολλά περιβάλλοντα drawmsg.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком много контекстов drawmsg.";
        if (strncmp(lang_code, "zh", 2) == 0) return "drawmsg 上下文过多。";
        if (strncmp(lang_code, "tw", 2) == 0) return "drawmsg 上下文過多。";
        if (strncmp(lang_code, "ja", 2) == 0) return "drawmsg コンテキストが多すぎます。";
        return "Too many drawmsg contexts.";
    }
    if (strcmp(msg, "Too many macro calls.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Demasiadas llamadas a macros.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Trop d'appels de macros.";
        if (strncmp(lang_code, "it", 2) == 0) return "Troppe chiamate macro.";
        if (strncmp(lang_code, "de", 2) == 0) return "Zu viele Makroaufrufe.";
        if (strncmp(lang_code, "el", 2) == 0) return "Πάρα πολλές κλήσεις μακροεντολών.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком много вызовов макросов.";
        if (strncmp(lang_code, "zh", 2) == 0) return "宏调用过多。";
        if (strncmp(lang_code, "tw", 2) == 0) return "巨集呼叫過多。";
        if (strncmp(lang_code, "ja", 2) == 0) return "マクロの呼び出し回数が多すぎます。";
        return "Too many macro calls.";
    }
    if (strcmp(msg, "Too many nests.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Demasiados anidamientos.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Trop d'imbrications.";
        if (strncmp(lang_code, "it", 2) == 0) return "Troppi annidamenti.";
        if (strncmp(lang_code, "de", 2) == 0) return "Zu viele Verschachtelungen.";
        if (strncmp(lang_code, "el", 2) == 0) return "Πάρα πολλές εμφωλεύσεις.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком много уровней вложенности.";
        if (strncmp(lang_code, "zh", 2) == 0) return "嵌套过多。";
        if (strncmp(lang_code, "tw", 2) == 0) return "巢狀層級過多。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ネストが深すぎます。";
        return "Too many nests.";
    }
    if (strcmp(msg, "Too many properties.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Demasiadas propiedades.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Trop de propriétés.";
        if (strncmp(lang_code, "it", 2) == 0) return "Troppe proprietà.";
        if (strncmp(lang_code, "de", 2) == 0) return "Zu viele Eigenschaften.";
        if (strncmp(lang_code, "el", 2) == 0) return "Πάρα πολλές ιδιότητες.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком много свойств.";
        if (strncmp(lang_code, "zh", 2) == 0) return "属性过多。";
        if (strncmp(lang_code, "tw", 2) == 0) return "屬性過多。";
        if (strncmp(lang_code, "ja", 2) == 0) return "プロパティが多すぎます。";
        return "Too many properties.";
    }
    if (strcmp(msg, "Too many registered animes.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Demasiadas animaciones registradas.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Trop d'animations enregistrées.";
        if (strncmp(lang_code, "it", 2) == 0) return "Troppe animazioni registrate.";
        if (strncmp(lang_code, "de", 2) == 0) return "Zu viele registrierte Animationen.";
        if (strncmp(lang_code, "el", 2) == 0) return "Πάρα πολλές εγγεγραμμένες κινήσεις.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком много зарегистрированных анимаций.";
        if (strncmp(lang_code, "zh", 2) == 0) return "注册动画过多。";
        if (strncmp(lang_code, "tw", 2) == 0) return "註冊動畫過多。";
        if (strncmp(lang_code, "ja", 2) == 0) return "登録されているアニメーションが多すぎます。";
        return "Too many registered animes.";
    }
    if (strcmp(msg, "Undefined config key \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Clave de configuración \"%s\" no definida.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Clé de configuration \"%s\" non définie.";
        if (strncmp(lang_code, "it", 2) == 0) return "Chiave di configurazione \"%s\" non definita.";
        if (strncmp(lang_code, "de", 2) == 0) return "Undefinierter Konfigurationsschlüssel \"%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη ορισμένο κλειδί ρύθμισης \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Неопределенный ключ конфигурации «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "未定义的配置键 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未定義的設定鍵 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "設定キー \"%s\" は定義されていません。";
        return "Undefined config key \"%s\".";
    }
    if (strcmp(msg, "Unexpected EOF.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Fin de archivo (EOF) inesperado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Fin de fichier (EOF) inattendue.";
        if (strncmp(lang_code, "it", 2) == 0) return "EOF imprevisto.";
        if (strncmp(lang_code, "de", 2) == 0) return "Unerwartetes Dateiende (EOF).";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη αναμενόμενο τέλος αρχείου (EOF).";
        if (strncmp(lang_code, "ru", 2) == 0) return "Непредвиденный конец файла (EOF).";
        if (strncmp(lang_code, "zh", 2) == 0) return "意外的 EOF。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未預期的 EOF。";
        if (strncmp(lang_code, "ja", 2) == 0) return "予期しないファイル終端です。";
        return "Unexpected EOF.";
    }
    if (strcmp(msg, "Unknown button property \"%s\" found in GUI file \"%s\" line %d.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Se encontró una propiedad de botón \"%s\" desconocida en el archivo GUI \"%s\" en la línea %d.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Propriété de bouton \"%s\" inconnue dans le fichier GUI \"%s\" ligne %d.";
        if (strncmp(lang_code, "it", 2) == 0) return "Proprietà pulsante \"%s\" sconosciuta nel file GUI \"%s\" riga %d.";
        if (strncmp(lang_code, "de", 2) == 0) return "Unbekannte Button-Eigenschaft \"%s\" in GUI-Datei \"%s\" Zeile %d gefunden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Βρέθηκε άγνωστη ιδιότητα κουμπιού \"%s\" στο αρχείο GUI \"%s\" γραμμή %d.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Неизвестное свойство кнопки «%s» в GUI-файле «%s» в строке %d.";
        if (strncmp(lang_code, "zh", 2) == 0) return "在 GUI 文件 \"%s\" 第 %d 行发现未知的按钮属性 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "在 GUI 檔案 \"%s\" 第 %d 行發現未知的按鈕屬性 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "GUIファイル \"%s\" の %d 行目に未知のボタンプロパティ \"%s\" があります。";
        return "Unknown button property \"%s\" found in GUI file \"%s\" line %d.";
    }
    if (strcmp(msg, "Unknown button type name \"%s\" in GUI file \"%s\" line %d.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Nombre de tipo de botón \"%s\" desconocido en el archivo GUI \"%s\" en la línea %d.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Nom de type de bouton \"%s\" inconnu dans le fichier GUI \"%s\" ligne %d.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nome tipo pulsante \"%s\" sconosciuto nel file GUI \"%s\" riga %d.";
        if (strncmp(lang_code, "de", 2) == 0) return "Unbekannter Button-Typname \"%s\" in GUI-Datei \"%s\" Zeile %d.";
        if (strncmp(lang_code, "el", 2) == 0) return "Άγνωστο όνομα τύπου κουμπιού \"%s\" στο αρχείο GUI \"%s\" γραμμή %d.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Неизвестное имя типа кнопки «%s» в GUI-файле «%s» в строке %d.";
        if (strncmp(lang_code, "zh", 2) == 0) return "GUI 文件 \"%s\" 第 %d 行发现未知的按钮类型 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "GUI 檔案 \"%s\" 第 %d 行發現未知的按鈕類型 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "GUIファイル \"%s\" の %d 行目に未知のボタンタイプ \"%s\" があります。";
        return "Unknown button type name \"%s\" in GUI file \"%s\" line %d.";
    }
    if (strcmp(msg, "Unknown config key \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Clave de configuración \"%s\" desconocida.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Clé de configuration \"%s\" inconnue.";
        if (strncmp(lang_code, "it", 2) == 0) return "Chiave di configurazione \"%s\" sconosciuta.";
        if (strncmp(lang_code, "de", 2) == 0) return "Unbekannter Konfigurationsschlüssel \"%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Άγνωστο κλειδί ρύθμισης \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Неизвестный ключ конфигурации «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "未知的配置键 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未知的設定鍵 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "未知の設定キー \"%s\" です。";
        return "Unknown config key \"%s\".";
    }
    if (strcmp(msg, "Unknown global key \"%s\" found in GUI file \"%s\" line %d.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Se encontró una clave global \"%s\" desconocida en el archivo GUI \"%s\" en la línea %d.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Clé globale \"%s\" inconnue dans le fichier GUI \"%s\" ligne %d.";
        if (strncmp(lang_code, "it", 2) == 0) return "Chiave globale \"%s\" sconosciuta nel file GUI \"%s\" riga %d.";
        if (strncmp(lang_code, "de", 2) == 0) return "Unbekannter globaler Schlüssel \"%s\" in GUI-Datei \"%s\" Zeile %d gefunden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Βρέθηκε άγνωστο παγκόσμιο κλειδί \"%s\" στο αρχείο GUI \"%s\" γραμμή %d.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Неизвестный глобальный ключ «%s» в GUI-файле «%s» в строке %d.";
        if (strncmp(lang_code, "zh", 2) == 0) return "在 GUI 文件 \"%s\" 第 %d 行发现未知的全局键 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "在 GUI 檔案 \"%s\" 第 %d 行發現未知的全域鍵 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "GUIファイル \"%s\" の %d 行目に未知のグローバルキー \"%s\" があります。";
        return "Unknown global key \"%s\" found in GUI file \"%s\" line %d.";
    }
    if (strcmp(msg, "Unkwon key \"%s\" while parsing anime file \"%s\" line %d.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Clave \"%s\" desconocida al analizar el archivo de animación \"%s\" en la línea %d.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Clé \"%s\" inconnue lors de l'analyse du fichier d'animation \"%s\" ligne %d.";
        if (strncmp(lang_code, "it", 2) == 0) return "Chiave \"%s\" sconosciuta durante l'analisi del file di animazione \"%s\" riga %d.";
        if (strncmp(lang_code, "de", 2) == 0) return "Unbekannter Schlüssel \"%s\" beim Parsen der Animationsdatei \"%s\" Zeile %d.";
        if (strncmp(lang_code, "el", 2) == 0) return "Άγνωστο κλειδί \"%s\" κατά την ανάλυση του αρχείου κινουμένων σχεδίων \"%s\" γραμμή %d.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Неизвестный ключ «%s» при анализе файла анимации «%s» в строке %d.";
        if (strncmp(lang_code, "zh", 2) == 0) return "解析动画文件 \"%s\" 第 %d 行時发现未知键 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "解析動畫檔案 \"%s\" 第 %d 行時發現未知鍵 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "アニメーションファイル \"%s\" の %d 行目を解析中に未知のキー \"%s\" が見つかりました。";
        return "Unkwon key \"%s\" while parsing anime file \"%s\" line %d.";
    }
    if (strcmp(msg, "Invalid config line at file \"%s\" line %d.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Línea de configuración no válida en el archivo \"%s\" en la línea %d.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Ligne de configuration non valide dans le fichier \"%s\" ligne %d.";
        if (strncmp(lang_code, "it", 2) == 0) return "Linea di configurazione non valida nel file \"%s\" riga %d.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültige Konfigurationszeile in Datei \"%s\" Zeile %d.";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρη γραμμή ρύθμισης στο αρχείο \"%s\" γραμμή %d.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимая строка конфигурации в файле «%s» в строке %d.";
        if (strncmp(lang_code, "zh", 2) == 0) return "文件 \"%s\" 第 %d 行配置无效。";
        if (strncmp(lang_code, "tw", 2) == 0) return "檔案 \"%s\" 第 %d 行設定無効。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ファイル \"%s\" の %d 行目の設定が不正です。";
        return "Invalid config line at file \"%s\" line %d.";
    }
    if (strcmp(msg, "No matching localized text.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se encontro un texto localizado coincidente.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Aucun texte localisé correspondant n'a été trouvé.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nessun testo localizzato corrispondente trovato.";
        if (strncmp(lang_code, "de", 2) == 0) return "Kein passender lokalisierter Text gefunden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν βρέθηκε αντίστοιχο τοπικό κείμενο.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Соответствующий локализованный текст не найден.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未找到匹配的本地化文本。";
        if (strncmp(lang_code, "ja", 2) == 0) return "テキストのローカライズ版がマッチしませんでした。";
        return "No matching localized text.";
    }
    return msg;
}
