#include <string.h>

const char *hal_get_system_language(void);

const char *pf_gettext(const char *msg)
{
    const char *lang_code = hal_get_system_language();
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
    if (strcmp(msg, "Cannot open file \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede abrir el archivo \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible d'ouvrir le fichier \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile aprire il file \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Datei \"%s\" kann nicht geöffnet werden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι δυνατό το άνοιγμα του αρχείου \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Невозможно открыть файл \"%s\".";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法打开文件 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法打開檔案 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ファイル \"%s\" を開けません。";
        return "Cannot open file \"%s\".";
    }
    if (strcmp(msg, "Cannot get the size of file \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede obtener el tamaño del archivo \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible d'obtenir la taille du fichier \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile ottenere la dimensione del file \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Die Größe der Datei \"%s\" kann nicht ermittelt werden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι δυνατή η λήψη του μεγέθους του αρχείου \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Невозможно получить размер файла \"%s\".";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法获取文件 \"%s\" 的大小。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法取得檔案 \"%s\" 的大小。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ファイル \"%s\" のサイズを取得できません。";
        return "Cannot get the size of file \"%s\".";
    }
    if (strcmp(msg, "Cannot read file \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede leer el archivo \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible de lire le fichier \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile leggere il file \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Datei \"%s\" kann nicht gelesen werden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι δυνατή η ανάγνωση του αρχείου \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Невозможно прочитать файл \"%s\".";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法读取文件 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法讀取檔案 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ファイル \"%s\" を読み込めません。";
        return "Cannot read file \"%s\".";
    }
    if (strcmp(msg, "In tag %s:%d: runtime error.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "En la etiqueta %s:%d: error en tiempo de ejecución.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Dans la balise %s:%d: erreur d'exécution.";
        if (strncmp(lang_code, "it", 2) == 0) return "Nel tag %s:%d: errore di runtime.";
        if (strncmp(lang_code, "de", 2) == 0) return "In Tag %s:%d: Laufzeitfehler.";
        if (strncmp(lang_code, "el", 2) == 0) return "Στην ετικέτα %s:%d: σφάλμα χρόνου εκτέλεσης.";
        if (strncmp(lang_code, "ru", 2) == 0) return "В теге %s:%d: ошибка выполнения.";
        if (strncmp(lang_code, "zh", 2) == 0) return "在标签 %s:%d: 运行时错误。";
        if (strncmp(lang_code, "tw", 2) == 0) return "在標籤 %s:%d: 執行時錯誤。";
        if (strncmp(lang_code, "ja", 2) == 0) return "タグ %s:%d: 実行エラー";
        return "In tag %s:%d: runtime error.";
    }
    if (strcmp(msg, "%s:%d: Tag \"%s\" not found.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "%s:%d: No se encontró la etiqueta \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "%s:%d: Balise \"%s\" introuvable.";
        if (strncmp(lang_code, "it", 2) == 0) return "%s:%d: Tag \"%s\" non trovato.";
        if (strncmp(lang_code, "de", 2) == 0) return "%s:%d: Tag \"%s\" wurde nicht gefunden.";
        if (strncmp(lang_code, "el", 2) == 0) return "%s:%d: Η ετικέτα \"%s\" δεν βρέθηκε.";
        if (strncmp(lang_code, "ru", 2) == 0) return "%s:%d: Тег \"%s\" не найден.";
        if (strncmp(lang_code, "zh", 2) == 0) return "%s:%d: 未找到标签 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "%s:%d: 找不到標籤 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "%s:%d: タグ \"%s\" がみつかりません。";
        return "%s:%d: Tag \"%s\" not found.";
    }
    if (strcmp(msg, "%s:%d: \"tag_%s\" is not a function.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "%s:%d: \"tag_%s\" no es una función.";
        if (strncmp(lang_code, "fr", 2) == 0) return "%s:%d: \"tag_%s\" n'est pas une fonction.";
        if (strncmp(lang_code, "it", 2) == 0) return "%s:%d: \"tag_%s\" non è una funzione.";
        if (strncmp(lang_code, "de", 2) == 0) return "%s:%d: \"tag_%s\" ist keine Funktion.";
        if (strncmp(lang_code, "el", 2) == 0) return "%s:%d: Το \"tag_%s\" δεν είναι συνάρτηση.";
        if (strncmp(lang_code, "ru", 2) == 0) return "%s:%d: \"tag_%s\" не является функцией.";
        if (strncmp(lang_code, "zh", 2) == 0) return "%s:%d: \"tag_%s\" 不是函数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "%s:%d: \"tag_%s\" 不是函式。";
        if (strncmp(lang_code, "ja", 2) == 0) return "%s:%d: \"tag_%s\" が関数ではありません。";
        return "%s:%d: \"tag_%s\" is not a function.";
    }
    if (strcmp(msg, "In tag %s:%d: Tag \"%s\" execution error.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "En la etiqueta %s:%d: Error de ejecución en la etiqueta \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Dans la balise %s:%d: Erreur d'exécution de la balise \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Nel tag %s:%d: Errore di esecuzione del tag \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "In Tag %s:%d: Ausführungsfehler im Tag \"%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Στην ετικέτα %s:%d: Σφάλμα εκτέλεσης στην ετικέτα \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "В теге %s:%d: Ошибка выполнения тега \"%s\".";
        if (strncmp(lang_code, "zh", 2) == 0) return "在标签 %s:%d: 执行标签 \"%s\" 时出错。";
        if (strncmp(lang_code, "tw", 2) == 0) return "在標籤 %s:%d: 執行標籤 \"%s\" 時發生錯誤。";
        if (strncmp(lang_code, "ja", 2) == 0) return "タグ %s:%d: タグ \"%s\" の実行エラーです。";
        return "In tag %s:%d: Tag \"%s\" execution error.";
    }
    if (strcmp(msg, "Cannot determine the file type for \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede determinar el tipo de archivo de \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible de déterminer le type du fichier  \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile determinare il tipo di file per \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Der Dateityp von \"%s\" kann nicht bestimmt werden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι δυνατός ο προσδιορισμός του τύπου αρχείου για το \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Невозможно определить тип файла \"%s\".";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法确定文件 \"%s\" 的类型。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法判斷檔案 \"%s\" 的類型。";
        if (strncmp(lang_code, "ja", 2) == 0) return "\"%s\" のファイルタイプを決定できません。";
        return "Cannot determine the file type for \"%s\".";
    }
    if (strcmp(msg, "Cannot load an image \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede cargar la imagen \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible de charger l'image \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile caricare l'immagine \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Das Bild \"%s\" kann nicht geladen werden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι δυνατή η φόρτωση της εικόνας \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не удаётся загрузить изображение \"%s\".";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法加载图像 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法載入圖片 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "画像 \"%s\" をロードできません。";
        return "Cannot load an image \"%s\".";
    }
    if (strcmp(msg, "Too many textures.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Demasiadas texturas.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Trop de textures.";
        if (strncmp(lang_code, "it", 2) == 0) return "Troppe texture.";
        if (strncmp(lang_code, "de", 2) == 0) return "Zu viele Texturen.";
        if (strncmp(lang_code, "el", 2) == 0) return "Πάρα πολλές υφές.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Слишком много текстур.";
        if (strncmp(lang_code, "zh", 2) == 0) return "纹理过多。";
        if (strncmp(lang_code, "tw", 2) == 0) return "材質過多。";
        if (strncmp(lang_code, "ja", 2) == 0) return "テキスチャが多すぎます。";
        return "Too many textures.";
    }
    if (strcmp(msg, "Invalid font slot index.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Índice de ranura de fuente no válido.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Indice d'emplacement de police non valide.";
        if (strncmp(lang_code, "it", 2) == 0) return "Indice di slot del font non valido.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültiger Schriftart-Slot-Index.";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρος δείκτης υποδοχής γραμματοσειράς.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимый индекс слота шрифта.";
        if (strncmp(lang_code, "zh", 2) == 0) return "无效的字体槽索引。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無效的字體槽索引。";
        if (strncmp(lang_code, "ja", 2) == 0) return "フォントスロットの番号が不正です。";
        return "Invalid font slot index.";
    }
    if (strcmp(msg, "Invalid sound stream index.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Índice de flujo de sonido no válido.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Indice de flux sonore non valide.";
        if (strncmp(lang_code, "it", 2) == 0) return "Indice di stream audio non valido.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültiger Soundstream-Index.";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρος δείκτης ροής ήχου.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимый индекс звукового потока.";
        if (strncmp(lang_code, "zh", 2) == 0) return "无效的音频流索引。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無效的音訊串流索引。";
        if (strncmp(lang_code, "ja", 2) == 0) return "サウンドストリームの番号が不正です。";
        return "Invalid sound stream index.";
    }
    if (strcmp(msg, "Cannot make the save directory.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede crear el directorio de guardado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible de créer le répertoire de sauvegarde.";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile creare la directory di salvataggio.";
        if (strncmp(lang_code, "de", 2) == 0) return "Das Speicherverzeichnis kann nicht erstellt werden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι δυνατή η δημιουργία του καταλόγου αποθήκευσης.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не удалось создать каталог сохранения.";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法创建存档目录。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法建立存檔目錄。";
        if (strncmp(lang_code, "ja", 2) == 0) return "セーブディレクトリを作成できません。";
        return "Cannot make the save directory.";
    }
    if (strcmp(msg, "Save data key too long.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "La clave de los datos guardados es demasiado larga.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La clé des données de sauvegarde est trop longue.";
        if (strncmp(lang_code, "it", 2) == 0) return "La chiave dei dati di salvataggio è troppo lunga.";
        if (strncmp(lang_code, "de", 2) == 0) return "Der Schlüssel der Spieldaten ist zu lang.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το κλειδί των δεδομένων αποθήκευσης είναι πολύ μεγάλο.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Ключ сохранённых данных слишком длинный.";
        if (strncmp(lang_code, "zh", 2) == 0) return "存档数据键太长。";
        if (strncmp(lang_code, "tw", 2) == 0) return "存檔資料鍵太長。";
        if (strncmp(lang_code, "ja", 2) == 0) return "セーブデータキーが長すぎます。";
        return "Save data key too long.";
    }
    if (strcmp(msg, "Cannot open a save file.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede abrir un archivo de guardado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible d'ouvrir un fichier de sauvegarde.";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile aprire un file di salvataggio.";
        if (strncmp(lang_code, "de", 2) == 0) return "Eine Speicherdatei kann nicht geöffnet werden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι δυνατό το άνοιγμα ενός αρχείου αποθήκευσης.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не удалось открыть файл сохранения.";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法打开存档文件。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法開啟存檔檔案。";
        if (strncmp(lang_code, "ja", 2) == 0) return "セーブファイルを開けません。";
        return "Cannot open a save file.";
    }
    if (strcmp(msg, "Cannot write to a save file.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede escribir en un archivo de guardado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible d'écrire dans un fichier de sauvegarde.";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile scrivere in un file di salvataggio.";
        if (strncmp(lang_code, "de", 2) == 0) return "In eine Speicherdatei kann nicht geschrieben werden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι δυνατή η εγγραφή σε ένα αρχείο αποθήκευσης.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не удалось записать в файл сохранения.";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法写入存档文件。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法寫入存檔檔案。";
        if (strncmp(lang_code, "ja", 2) == 0) return "セーブファイルに書き込めません。";
        return "Cannot write to a save file.";
    }
    if (strcmp(msg, "Cannot get the size of a save file.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede obtener el tamaño de un archivo de guardado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible d'obtenir la taille d'un fichier de sauvegarde.";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile ottenere la dimensione di un file di salvataggio.";
        if (strncmp(lang_code, "de", 2) == 0) return "Die Größe einer Speicherdatei kann nicht ermittelt werden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι δυνατή η λήψη του μεγέθους ενός αρχείου αποθήκευσης.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не удалось получить размер файла сохранения.";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法获取存档文件的大小。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法取得存檔檔案的大小。";
        if (strncmp(lang_code, "ja", 2) == 0) return "セーブファイルのサイズを取得できません。";
        return "Cannot get the size of a save file.";
    }
    if (strcmp(msg, "Save file too large.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "El archivo de guardado es demasiado grande.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Le fichier de sauvegarde est trop volumineux.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il file di salvataggio è troppo grande.";
        if (strncmp(lang_code, "de", 2) == 0) return "Die Speicherdatei ist zu groß.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το αρχείο αποθήκευσης είναι πολύ μεγάλο.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Файл сохранения слишком большой.";
        if (strncmp(lang_code, "zh", 2) == 0) return "存档文件过大。";
        if (strncmp(lang_code, "tw", 2) == 0) return "存檔檔案過大。";
        if (strncmp(lang_code, "ja", 2) == 0) return "セーブファイルが大きすぎます。";
        return "Save file too large.";
    }
    if (strcmp(msg, "Cannot read a save file.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede leer un archivo de guardado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible de lire un fichier de sauvegarde.";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile leggere un file di salvataggio.";
        if (strncmp(lang_code, "de", 2) == 0) return "Eine Speicherdatei kann nicht gelesen werden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Δεν είναι δυνατή η ανάγνωση ενός αρχείου αποθήκευσης.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не удалось прочитать файл сохранения.";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法读取存档文件。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法讀取存檔檔案。";
        if (strncmp(lang_code, "ja", 2) == 0) return "セーブファイルを読み込めません。";
        return "Cannot read a save file.";
    }
    if (strcmp(msg, "String too long.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Cadena demasiado larga.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Chaîne trop longue.";
        if (strncmp(lang_code, "it", 2) == 0) return "Stringa troppo lunga.";
        if (strncmp(lang_code, "de", 2) == 0) return "Zeichenkette zu lang.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η συμβολοσειρά είναι πολύ μεγάλη.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Строка слишком длинная.";
        if (strncmp(lang_code, "zh", 2) == 0) return "字符串太长。";
        if (strncmp(lang_code, "tw", 2) == 0) return "字串太長。";
        if (strncmp(lang_code, "ja", 2) == 0) return "文字列が長すぎます。";
        return "String too long.";
    }
    if (strcmp(msg, "Invalid save data.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Datos de guardado no validos.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Données de sauvegarde non valides.";
        if (strncmp(lang_code, "it", 2) == 0) return "Dati di salvataggio non validi.";
        if (strncmp(lang_code, "de", 2) == 0) return "Ungültige Spieldaten.";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη έγκυρα δεδομένα αποθήκευσης.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Недопустимые данные сохранения.";
        if (strncmp(lang_code, "zh", 2) == 0) return "无效的存档数据。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無效的存檔資料。";
        if (strncmp(lang_code, "ja", 2) == 0) return "セーブデータが無効です。";
        return "Invalid save data.";
    }
    if (strcmp(msg, "Searching directory \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Buscando en el directorio \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Recherche dans le répertoire \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Ricerca nella directory \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Suche im Verzeichnis \"%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Αναζήτηση στον κατάλογο \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Поиск в каталоге «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "正在搜索目录 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "正在搜尋目錄 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ディレクトリ \"%s\" を検索中。";
        return "Searching directory \"%s\".";
    }
    if (strcmp(msg, "Skipping empty directory \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Omitiendo el directorio vacío \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Ignorer le répertoire vide \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Salto della directory vuota \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Leeres Verzeichnis \"%s\" wird übersprungen.";
        if (strncmp(lang_code, "el", 2) == 0) return "Παράλειψη κενού καταλόγου \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Пропуск пустого каталога «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "跳过空目录 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "跳過空目錄 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "空のディレクトリ \"%s\" をスキップします。";
        return "Skipping empty directory \"%s\".";
    }
    if (strcmp(msg, "Adding file \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Añadiendo archivo \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Ajout du fichier \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Aggiunta del file \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Datei \"%s\" wird hinzugefügt.";
        if (strncmp(lang_code, "el", 2) == 0) return "Προσθήκη αρχείου \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Добавление файла «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "正在添加文件 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "正在新增檔案 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "ファイル \"%s\" を追加しています。";
        return "Adding file \"%s\".";
    }
    if (strcmp(msg, "Parameter is not set.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "El parámetro no está configurado.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Le paramètre n'est pas défini.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il parametro non è impostato.";
        if (strncmp(lang_code, "de", 2) == 0) return "Parameter ist nicht gesetzt.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η παράμετρος δεν έχει οριστεί.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Параметр не задан.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未设置参数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "未設定參數。";
        if (strncmp(lang_code, "ja", 2) == 0) return "パラメータが設定されていません。";
        return "Parameter is not set.";
    }
    if (strcmp(msg, "Unexpected parameter value for \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Valor de parámetro inesperado para \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Valeur de paramètre inattendue pour \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Valore del parametro imprevisto per \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Unerwarteter Parameterwert für \"%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη αναμενόμενη τιμή παραμέτρου για \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Непредвиденное значение параметра для «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "参数 \"%s\" 的值异常。";
        if (strncmp(lang_code, "tw", 2) == 0) return "參數 \"%s\" 的值異常。";
        if (strncmp(lang_code, "ja", 2) == 0) return "パラメータ \"%s\" の値が不正です。";
        return "Unexpected parameter value for \"%s\".";
    }
    if (strcmp(msg, "Parameter \"%s\" doesn't have the key \"%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "El parámetro \"%s\" no tiene la clave \"%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Le paramètre \"%s\" n'a pas la clé \"%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Il parametro \"%s\" non ha la chiave \"%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Parameter \"%s\" hat keinen Schlüssel \"%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Η παράμετρος \"%s\" δεν έχει το κλειδί \"%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Параметр «%s» не содержит ключ «%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "参数 \"%s\" 不包含键 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "參數 \"%s\" 不包含鍵 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "パラメータ \"%s\" にキー \"%s\" がありません。";
        return "Parameter \"%s\" doesn't have the key \"%s\".";
    }
    if (strcmp(msg, "Unexpected parameter value for \"%s.%s\".") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "Valor de parámetro  inesperado para \"%s.%s\".";
        if (strncmp(lang_code, "fr", 2) == 0) return "Valeur de paramètre inattendue pour \"%s.%s\".";
        if (strncmp(lang_code, "it", 2) == 0) return "Valore del parametro imprevisto per \"%s.%s\".";
        if (strncmp(lang_code, "de", 2) == 0) return "Unerwarteter Parameterwert für \"%s.%s\".";
        if (strncmp(lang_code, "el", 2) == 0) return "Μη αναμενόμενη τιμή παραμέτρου για \"%s.%s\".";
        if (strncmp(lang_code, "ru", 2) == 0) return "Непредвиденное значение параметра для «%s.%s».";
        if (strncmp(lang_code, "zh", 2) == 0) return "参数 \"%s.%s\" 的值異常。";
        if (strncmp(lang_code, "tw", 2) == 0) return "參數 \"%s.%s\" 的值異常。";
        if (strncmp(lang_code, "ja", 2) == 0) return "パラメータ \"%s.%s\" の値が不正です。";
        return "Unexpected parameter value for \"%s.%s\".";
    }
    if (strcmp(msg, "Cannot deserialize function.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "No se puede deserializar la función.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Impossible de désérialiser la fonction.";
        if (strncmp(lang_code, "it", 2) == 0) return "Impossibile deserializzare la funzione.";
        if (strncmp(lang_code, "de", 2) == 0) return "Funktion kann nicht deserialisiert werden.";
        if (strncmp(lang_code, "el", 2) == 0) return "Αδυναμία αποσειριοποίησης της συνάρτησης.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Не удалось десериализовать функцию.";
        if (strncmp(lang_code, "zh", 2) == 0) return "无法反序列化函数。";
        if (strncmp(lang_code, "tw", 2) == 0) return "無法反序列化函式。";
        if (strncmp(lang_code, "ja", 2) == 0) return "関数をデシリアライズできません。";
        return "Cannot deserialize function.";
    }
    if (strcmp(msg, "Parameter is required to be a dictionary.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "El parámetro debe ser un diccionario.";
        if (strncmp(lang_code, "fr", 2) == 0) return "Le paramètre doit être un dictionnaire.";
        if (strncmp(lang_code, "de", 2) == 0) return "Der Parameter muss ein Dictionary sein.";
        if (strncmp(lang_code, "it", 2) == 0) return "Il parametro deve essere un dizionario.";
        if (strncmp(lang_code, "el", 2) == 0) return "Η παράμετρος πρέπει να είναι λεξικό.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Параметр должен быть словарем.";
        if (strncmp(lang_code, "zh", 2) == 0) return "参数必须是字典。";
        if (strncmp(lang_code, "tw", 2) == 0) return "參數必須是字典。";
        if (strncmp(lang_code, "ja", 2) == 0) return "引数は辞書形式である必要があります。";
        return "Parameter is required to be a dictionary.";
    }
    if (strcmp(msg, "Parameter key \"%s\" not found.") == 0) {
        if (strncmp(lang_code, "es", 2) == 0) return "La clave \"%s\" no se encontró en el parámetro.";
        if (strncmp(lang_code, "fr", 2) == 0) return "La clé \"%s\" est introuvable dans le paramètre.";
        if (strncmp(lang_code, "de", 2) == 0) return "Schlüssel \"%s\" im Parameter nicht gefunden.";
        if (strncmp(lang_code, "it", 2) == 0) return "Chiave \"%s\" non trovata nel parametro.";
        if (strncmp(lang_code, "el", 2) == 0) return "Το κλειδί \"%s\" δεν βρέθηκε στην παράμετρο.";
        if (strncmp(lang_code, "ru", 2) == 0) return "Ключ \"%s\" не найден в параметре.";
        if (strncmp(lang_code, "zh", 2) == 0) return "未找到参数键 \"%s\"。";
        if (strncmp(lang_code, "tw", 2) == 0) return "找不到參數鍵 \"%s\"。";
        if (strncmp(lang_code, "ja", 2) == 0) return "パラメーターのキー \"%s\" が見つかりません。";
        return "Parameter key \"%s\" not found.";
    }
    return msg;
}
