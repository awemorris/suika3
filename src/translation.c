#include <string.h>

const char *pf_get_system_language(void);

const char *pf_gettext(const char *msg)
{
    const char *lang_code = pf_get_system_language();
    if (strcmp(msg, "Error: %s: %d: %s") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Error: %s: %d: %s";
        if (strcmp(lang_code, "fr") == 0) return "Erreur : %s : %d : %s";
        if (strcmp(lang_code, "it") == 0) return "Errore: %s: %d: %s";
        if (strcmp(lang_code, "de") == 0) return "Fehler: %s: %d: %s";
        if (strcmp(lang_code, "el") == 0) return "Σφάλμα: %s: %d: %s";
        if (strcmp(lang_code, "ru") == 0) return "Ошибка: %s: %d: %s";
        if (strcmp(lang_code, "zh") == 0) return "错误: %s: %d: %s";
        if (strcmp(lang_code, "tw") == 0) return "錯誤: %s: %d: %s";
        if (strcmp(lang_code, "ja") == 0) return "エラー: %s: %d: %s";
        return "Error: %s: %d: %s";
    }
    if (strcmp(msg, "Cannot open file \"%s\".") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede abrir el archivo \"%s\".";
        if (strcmp(lang_code, "fr") == 0) return "Impossible d’ouvrir le fichier \"%s\".";
        if (strcmp(lang_code, "it") == 0) return "Impossibile aprire il file \"%s\".";
        if (strcmp(lang_code, "de") == 0) return "Datei \"%s\" kann nicht geöffnet werden.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι δυνατή η ανοίξη του αρχείου \"%s\".";
        if (strcmp(lang_code, "ru") == 0) return "Невозможно открыть файл \"%s\".";
        if (strcmp(lang_code, "zh") == 0) return "无法打开文件 \"%s\"。";
        if (strcmp(lang_code, "tw") == 0) return "無法打開檔案 \"%s\"。";
        if (strcmp(lang_code, "ja") == 0) return "ファイル \"%s\" を開けません。";
        return "Cannot open file \"%s\".";
    }
    if (strcmp(msg, "Cannot get the size of file \"%s\".") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede obtener el tamaño del archivo \"%s\".";
        if (strcmp(lang_code, "fr") == 0) return "Impossible d’obtenir la taille du fichier \"%s\".";
        if (strcmp(lang_code, "it") == 0) return "Impossibile ottenere la dimensione del file \"%s\".";
        if (strcmp(lang_code, "de") == 0) return "Die Größe der Datei \"%s\" kann nicht ermittelt werden.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι δυνατή η λήψη του μεγέθους του αρχείου \"%s\".";
        if (strcmp(lang_code, "ru") == 0) return "Невозможно получить размер файла \"%s\".";
        if (strcmp(lang_code, "zh") == 0) return "无法获取文件 \"%s\" 的大小。";
        if (strcmp(lang_code, "tw") == 0) return "無法取得檔案 \"%s\" 的大小。";
        if (strcmp(lang_code, "ja") == 0) return "ファイル \"%s\" のサイズを取得できません。";
        return "Cannot get the size of file \"%s\".";
    }
    if (strcmp(msg, "Cannot read file \"%s\".") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede leer el archivo \"%s\".";
        if (strcmp(lang_code, "fr") == 0) return "Impossible de lire le fichier \"%s\".";
        if (strcmp(lang_code, "it") == 0) return "Impossibile leggere il file \"%s\".";
        if (strcmp(lang_code, "de") == 0) return "Datei \"%s\" kann nicht gelesen werden.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι δυνατή η ανάγνωση του αρχείου \"%s\".";
        if (strcmp(lang_code, "ru") == 0) return "Невозможно прочитать файл \"%s\".";
        if (strcmp(lang_code, "zh") == 0) return "无法读取文件 \"%s\"。";
        if (strcmp(lang_code, "tw") == 0) return "無法讀取檔案 \"%s\"。";
        if (strcmp(lang_code, "ja") == 0) return "ファイル \"%s\" を読み込めません。";
        return "Cannot read file \"%s\".";
    }
    if (strcmp(msg, "In tag %s:%d: runtime error.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "En la etiqueta %s:%d: error en tiempo de ejecución.";
        if (strcmp(lang_code, "fr") == 0) return "Dans la balise %s:%d : erreur d’exécution.";
        if (strcmp(lang_code, "it") == 0) return "Nel tag %s:%d: errore di runtime.";
        if (strcmp(lang_code, "de") == 0) return "In Tag %s:%d: Laufzeitfehler.";
        if (strcmp(lang_code, "el") == 0) return "Στην ετικέτα %s:%d: σφάλμα χρόνου εκτέλεσης.";
        if (strcmp(lang_code, "ru") == 0) return "В теге %s:%d: ошибка выполнения.";
        if (strcmp(lang_code, "zh") == 0) return "在标签 %s:%d: 运行时错误。";
        if (strcmp(lang_code, "tw") == 0) return "在標籤 %s:%d: 執行時錯誤。";
        if (strcmp(lang_code, "ja") == 0) return "タグ %s:%d: 実行エラー";
        return "In tag %s:%d: runtime error.";
    }
    if (strcmp(msg, "%s:%d: Tag \"%s\" not found.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "%s:%d: No se encontró la etiqueta \"%s\".";
        if (strcmp(lang_code, "fr") == 0) return "%s:%d : Balise \"%s\" introuvable.";
        if (strcmp(lang_code, "it") == 0) return "%s:%d: Tag \"%s\" non trovato.";
        if (strcmp(lang_code, "de") == 0) return "%s:%d: Tag \"%s\" wurde nicht gefunden.";
        if (strcmp(lang_code, "el") == 0) return "%s:%d: Η ετικέτα \"%s\" δεν βρέθηκε.";
        if (strcmp(lang_code, "ru") == 0) return "%s:%d: Тег \"%s\" не найден.";
        if (strcmp(lang_code, "zh") == 0) return "%s:%d: 未找到标签 \"%s\"。";
        if (strcmp(lang_code, "tw") == 0) return "%s:%d: 找不到標籤 \"%s\"。";
        if (strcmp(lang_code, "ja") == 0) return "%s:%d: タグ \"%s\" がみつかりません。";
        return "%s:%d: Tag \"%s\" not found.";
    }
    if (strcmp(msg, "%s:%d: \"tag_%s\" is not a function.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "%s:%d: \"tag_%s\" no es una función.";
        if (strcmp(lang_code, "fr") == 0) return "%s:%d : \"tag_%s\" n’est pas une fonction.";
        if (strcmp(lang_code, "it") == 0) return "%s:%d: \"tag_%s\" non è una funzione.";
        if (strcmp(lang_code, "de") == 0) return "%s:%d: \"tag_%s\" ist keine Funktion.";
        if (strcmp(lang_code, "el") == 0) return "%s:%d: Το \"tag_%s\" δεν είναι συνάρτηση.";
        if (strcmp(lang_code, "ru") == 0) return "%s:%d: \"tag_%s\" не является функцией.";
        if (strcmp(lang_code, "zh") == 0) return "%s:%d: \"tag_%s\" 不是函数。";
        if (strcmp(lang_code, "tw") == 0) return "%s:%d: \"tag_%s\" 不是函式。";
        if (strcmp(lang_code, "ja") == 0) return "%s:%d: \"tag_%s\" が関数ではありません。";
        return "%s:%d: \"tag_%s\" is not a function.";
    }
    if (strcmp(msg, "In tag %s:%d: Tag \"%s\" execution error.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "En la etiqueta %s:%d: Error de ejecución en la etiqueta \"%s\".";
        if (strcmp(lang_code, "fr") == 0) return "Dans la balise %s:%d : Erreur d’exécution de la balise \"%s\".";
        if (strcmp(lang_code, "it") == 0) return "Nel tag %s:%d: Errore di esecuzione del tag \"%s\".";
        if (strcmp(lang_code, "de") == 0) return "In Tag %s:%d: Ausführungsfehler im Tag \"%s\".";
        if (strcmp(lang_code, "el") == 0) return "Στην ετικέτα %s:%d: Σφάλμα εκτέλεσης στην ετικέτα \"%s\".";
        if (strcmp(lang_code, "ru") == 0) return "В теге %s:%d: Ошибка выполнения тега \"%s\".";
        if (strcmp(lang_code, "zh") == 0) return "在标签 %s:%d: 执行标签 \"%s\" 时出错。";
        if (strcmp(lang_code, "tw") == 0) return "在標籤 %s:%d: 執行標籤 \"%s\" 時發生錯誤。";
        if (strcmp(lang_code, "ja") == 0) return "タグ %s:%d: タグ \"%s\" の実行エラーです。";
        return "In tag %s:%d: Tag \"%s\" execution error.";
    }
    if (strcmp(msg, "Cannot determine the file type for \"%s\".") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede determinar el tipo de archivo de \"%s\".";
        if (strcmp(lang_code, "fr") == 0) return "Impossible de déterminer le type du fichier  \"%s\".";
        if (strcmp(lang_code, "it") == 0) return "Impossibile determinare il tipo di file per \"%s\".";
        if (strcmp(lang_code, "de") == 0) return "Der Dateityp von \"%s\" kann nicht bestimmt werden.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι δυνατός ο προσδιορισμός του τύπου αρχείου για το \"%s\".";
        if (strcmp(lang_code, "ru") == 0) return "Невозможно определить тип файла \"%s\".";
        if (strcmp(lang_code, "zh") == 0) return "无法确定文件 \"%s\" 的类型。";
        if (strcmp(lang_code, "tw") == 0) return "無法判斷檔案 \"%s\" 的類型。";
        if (strcmp(lang_code, "ja") == 0) return "\"%s\" のファイルタイプを決定できません。";
        return "Cannot determine the file type for \"%s\".";
    }
    if (strcmp(msg, "Cannot load an image \"%s\".") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede cargar la imagen \"%s\".";
        if (strcmp(lang_code, "fr") == 0) return "Impossible de charger l’image \"%s\".";
        if (strcmp(lang_code, "it") == 0) return "Impossibile caricare l’immagine \"%s\".";
        if (strcmp(lang_code, "de") == 0) return "Das Bild \"%s\" kann nicht geladen werden.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι δυνατή η φόρτωση της εικόνας \"%s\".";
        if (strcmp(lang_code, "ru") == 0) return "Не удаётся загрузить изображение \"%s\".";
        if (strcmp(lang_code, "zh") == 0) return "无法加载图像 \"%s\"。";
        if (strcmp(lang_code, "tw") == 0) return "無法載入圖片 \"%s\"。";
        if (strcmp(lang_code, "ja") == 0) return "画像 \"%s\" をロードできません。";
        return "Cannot load an image \"%s\".";
    }
    if (strcmp(msg, "Too many textures.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Demasiadas texturas.";
        if (strcmp(lang_code, "fr") == 0) return "Trop de textures.";
        if (strcmp(lang_code, "it") == 0) return "Troppe texture.";
        if (strcmp(lang_code, "de") == 0) return "Zu viele Texturen.";
        if (strcmp(lang_code, "el") == 0) return "Πάρα πολλές υφές.";
        if (strcmp(lang_code, "ru") == 0) return "Слишком много текстур.";
        if (strcmp(lang_code, "zh") == 0) return "纹理过多。";
        if (strcmp(lang_code, "tw") == 0) return "材質過多。";
        if (strcmp(lang_code, "ja") == 0) return "テキスチャが多すぎます。";
        return "Too many textures.";
    }
    if (strcmp(msg, "Invalid font slot index.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Índice de ranura de fuente no válido.";
        if (strcmp(lang_code, "fr") == 0) return "Indice d’emplacement de police non valide.";
        if (strcmp(lang_code, "it") == 0) return "Indice di slot del font non valido.";
        if (strcmp(lang_code, "de") == 0) return "Ungültiger Schriftart-Slot-Index.";
        if (strcmp(lang_code, "el") == 0) return "Μη έγκυρος δείκτης υποδοχής γραμματοσειράς.";
        if (strcmp(lang_code, "ru") == 0) return "Недопустимый индекс слота шрифта.";
        if (strcmp(lang_code, "zh") == 0) return "无效的字体槽索引。";
        if (strcmp(lang_code, "tw") == 0) return "無效的字體槽索引。";
        if (strcmp(lang_code, "ja") == 0) return "フォントスロットの番号が不正です。";
        return "Invalid font slot index.";
    }
    if (strcmp(msg, "Invalid sound stream index.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Índice de flujo de sonido no válido.";
        if (strcmp(lang_code, "fr") == 0) return "Indice de flux sonore non valide.";
        if (strcmp(lang_code, "it") == 0) return "Indice di stream audio non valido.";
        if (strcmp(lang_code, "de") == 0) return "Ungültiger Soundstream-Index.";
        if (strcmp(lang_code, "el") == 0) return "Μη έγκυρος δείκτης ροής ήχου.";
        if (strcmp(lang_code, "ru") == 0) return "Недопустимый индекс звукового потока.";
        if (strcmp(lang_code, "zh") == 0) return "无效的音频流索引。";
        if (strcmp(lang_code, "tw") == 0) return "無效的音訊串流索引。";
        if (strcmp(lang_code, "ja") == 0) return "サウンドストリームの番号が不正です。";
        return "Invalid sound stream index.";
    }
    if (strcmp(msg, "Cannot make the save directory.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede crear el directorio de guardado.";
        if (strcmp(lang_code, "fr") == 0) return "Impossible de créer le répertoire de sauvegarde.";
        if (strcmp(lang_code, "it") == 0) return "Impossibile creare la directory di salvataggio.";
        if (strcmp(lang_code, "de") == 0) return "Das Speicherverzeichnis kann nicht erstellt werden.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι δυνατή η δημιουργία του καταλόγου αποθήκευσης.";
        if (strcmp(lang_code, "ru") == 0) return "Не удалось создать каталог сохранения.";
        if (strcmp(lang_code, "zh") == 0) return "无法创建存档目录。";
        if (strcmp(lang_code, "tw") == 0) return "無法建立存檔目錄。";
        if (strcmp(lang_code, "ja") == 0) return "セーブディレクトリを作成できません。";
        return "Cannot make the save directory.";
    }
    if (strcmp(msg, "Save data key too long.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "La clave de los datos guardados es demasiado larga.";
        if (strcmp(lang_code, "fr") == 0) return "La clé des données de sauvegarde est trop longue.";
        if (strcmp(lang_code, "it") == 0) return "La chiave dei dati di salvataggio è troppo lunga.";
        if (strcmp(lang_code, "de") == 0) return "Der Schlüssel der Spieldaten ist zu lang.";
        if (strcmp(lang_code, "el") == 0) return "Το κλειδί των δεδομένων αποθήκευσης είναι πολύ μεγάλο.";
        if (strcmp(lang_code, "ru") == 0) return "Ключ сохранённых данных слишком длинный.";
        if (strcmp(lang_code, "zh") == 0) return "存档数据键太长。";
        if (strcmp(lang_code, "tw") == 0) return "存檔資料鍵太長。";
        if (strcmp(lang_code, "ja") == 0) return "セーブデータキーが長すぎます。";
        return "Save data key too long.";
    }
    if (strcmp(msg, "Cannot open a save file.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede abrir un archivo de guardado.";
        if (strcmp(lang_code, "fr") == 0) return "Impossible d’ouvrir un fichier de sauvegarde.";
        if (strcmp(lang_code, "it") == 0) return "Impossibile aprire un file di salvataggio.";
        if (strcmp(lang_code, "de") == 0) return "Eine Speicherdatei kann nicht geöffnet werden.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι δυνατή η ανοίξη ενός αρχείου αποθήκευσης.";
        if (strcmp(lang_code, "ru") == 0) return "Не удалось открыть файл сохранения.";
        if (strcmp(lang_code, "zh") == 0) return "无法打开存档文件。";
        if (strcmp(lang_code, "tw") == 0) return "無法開啟存檔檔案。";
        if (strcmp(lang_code, "ja") == 0) return "セーブファイルを開けません。";
        return "Cannot open a save file.";
    }
    if (strcmp(msg, "Cannot write to a save file.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede escribir en un archivo de guardado.";
        if (strcmp(lang_code, "fr") == 0) return "Impossible d’écrire dans un fichier de sauvegarde.";
        if (strcmp(lang_code, "it") == 0) return "Impossibile scrivere in un file di salvataggio.";
        if (strcmp(lang_code, "de") == 0) return "In eine Speicherdatei kann nicht geschrieben werden.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι δυνατή η εγγραφή σε ένα αρχείο αποθήκευσης.";
        if (strcmp(lang_code, "ru") == 0) return "Не удалось записать в файл сохранения.";
        if (strcmp(lang_code, "zh") == 0) return "无法写入存档文件。";
        if (strcmp(lang_code, "tw") == 0) return "無法寫入存檔檔案。";
        if (strcmp(lang_code, "ja") == 0) return "セーブファイルに書き込めません。";
        return "Cannot write to a save file.";
    }
    if (strcmp(msg, "Cannot get the size of a save file.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede obtener el tamaño de un archivo de guardado.";
        if (strcmp(lang_code, "fr") == 0) return "Impossible d’obtenir la taille d’un fichier de sauvegarde.";
        if (strcmp(lang_code, "it") == 0) return "Impossibile ottenere la dimensione di un file di salvataggio.";
        if (strcmp(lang_code, "de") == 0) return "Die Größe einer Speicherdatei kann nicht ermittelt werden.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι δυνατή η λήψη του μεγέθους ενός αρχείου αποθήκευσης.";
        if (strcmp(lang_code, "ru") == 0) return "Не удалось получить размер файла сохранения.";
        if (strcmp(lang_code, "zh") == 0) return "无法获取存档文件的大小。";
        if (strcmp(lang_code, "tw") == 0) return "無法取得存檔檔案的大小。";
        if (strcmp(lang_code, "ja") == 0) return "セーブファイルのサイズを取得できません。";
        return "Cannot get the size of a save file.";
    }
    if (strcmp(msg, "Save file too large.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "El archivo de guardado es demasiado grande.";
        if (strcmp(lang_code, "fr") == 0) return "Le fichier de sauvegarde est trop volumineux.";
        if (strcmp(lang_code, "it") == 0) return "Il file di salvataggio è troppo grande.";
        if (strcmp(lang_code, "de") == 0) return "Die Speicherdatei ist zu groß.";
        if (strcmp(lang_code, "el") == 0) return "Το αρχείο αποθήκευσης είναι πολύ μεγάλο.";
        if (strcmp(lang_code, "ru") == 0) return "Файл сохранения слишком большой.";
        if (strcmp(lang_code, "zh") == 0) return "存档文件过大。";
        if (strcmp(lang_code, "tw") == 0) return "存檔檔案過大。";
        if (strcmp(lang_code, "ja") == 0) return "セーブファイルが大きすぎます。";
        return "Save file too large.";
    }
    if (strcmp(msg, "Cannot read a save file.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede leer un archivo de guardado.";
        if (strcmp(lang_code, "fr") == 0) return "Impossible de lire un fichier de sauvegarde.";
        if (strcmp(lang_code, "it") == 0) return "Impossibile leggere un file di salvataggio.";
        if (strcmp(lang_code, "de") == 0) return "Eine Speicherdatei kann nicht gelesen werden.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι δυνατή η ανάγνωση ενός αρχείου αποθήκευσης.";
        if (strcmp(lang_code, "ru") == 0) return "Не удалось прочитать файл сохранения.";
        if (strcmp(lang_code, "zh") == 0) return "无法读取存档文件。";
        if (strcmp(lang_code, "tw") == 0) return "無法讀取存檔檔案。";
        if (strcmp(lang_code, "ja") == 0) return "セーブファイルを読み込めません。";
        return "Cannot read a save file.";
    }
    if (strcmp(msg, "String too long.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Cadena demasiado larga.";
        if (strcmp(lang_code, "fr") == 0) return "Chaîne trop longue.";
        if (strcmp(lang_code, "it") == 0) return "Stringa troppo lunga.";
        if (strcmp(lang_code, "de") == 0) return "Zeichenkette zu lang.";
        if (strcmp(lang_code, "el") == 0) return "Η συμβολοσειρά είναι πολύ μεγάλη.";
        if (strcmp(lang_code, "ru") == 0) return "Строка слишком длинная.";
        if (strcmp(lang_code, "zh") == 0) return "字符串太长。";
        if (strcmp(lang_code, "tw") == 0) return "字串太長。";
        if (strcmp(lang_code, "ja") == 0) return "文字列が長すぎます。";
        return "String too long.";
    }
    if (strcmp(msg, "Invalid save data.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Datos de guardado no validos.";
        if (strcmp(lang_code, "fr") == 0) return "Données de sauvegarde non valides.";
        if (strcmp(lang_code, "it") == 0) return "Dati di salvataggio non validi.";
        if (strcmp(lang_code, "de") == 0) return "Ungültige Spieldaten.";
        if (strcmp(lang_code, "el") == 0) return "Μη έγκυρα δεδομένα αποθήκευσης.";
        if (strcmp(lang_code, "ru") == 0) return "Недопустимые данные сохранения.";
        if (strcmp(lang_code, "zh") == 0) return "无效的存档数据。";
        if (strcmp(lang_code, "tw") == 0) return "無效的存檔資料。";
        if (strcmp(lang_code, "ja") == 0) return "セーブデータが無効です。";
        return "Invalid save data.";
    }
    return msg;
}
