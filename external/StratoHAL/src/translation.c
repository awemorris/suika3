#include <string.h>

const char *hal_get_system_language(void);

const char *hal_gettext(const char *msg)
{
    const char *lang_code = hal_get_system_language();
    if (strcmp(msg, "Out of memory.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Memoria insuficiente.";
        if (strcmp(lang_code, "fr") == 0) return "Mémoire insuffisante.";
        if (strcmp(lang_code, "it") == 0) return "Memoria esaurita.";
        if (strcmp(lang_code, "de") == 0) return "Nicht genügend Speicher.";
        if (strcmp(lang_code, "el") == 0) return "Εκτός μνήμης.";
        if (strcmp(lang_code, "ru") == 0) return "Недостаточно памяти.";
        if (strcmp(lang_code, "zh") == 0) return "内存不足。";
        if (strcmp(lang_code, "tw") == 0) return "記憶體不足。";
        if (strcmp(lang_code, "ja") == 0) return "メモリが不足しています。";
        return "Out of memory.";
    }
    if (strcmp(msg, "Cannot open file \"%s\".") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No se puede abrir el archivo \"%s\".";
        if (strcmp(lang_code, "fr") == 0) return "Impossible douvrir le fichier \"%s\".";
        if (strcmp(lang_code, "it") == 0) return "Impossibile aprire il file \"%s\".";
        if (strcmp(lang_code, "de") == 0) return "Datei \"%s\" kann nicht geöffnet werden.";
        if (strcmp(lang_code, "el") == 0) return "Δεν είναι δυνατή η ανοίξη του αρχείου \"%s\".";
        if (strcmp(lang_code, "ru") == 0) return "Невозможно открыть файл \"%s\".";
        if (strcmp(lang_code, "zh") == 0) return "无法打开文件 \"%s\"。";
        if (strcmp(lang_code, "tw") == 0) return "無法打開檔案 \"%s\"。";
        if (strcmp(lang_code, "ja") == 0) return "ファイル \"%s\" を開けません。";
        return "Cannot open file \"%s\".";
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
    if (strcmp(msg, "No startup file.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "No hay archivo de inicio.";
        if (strcmp(lang_code, "fr") == 0) return "Aucun fichier de démarrage.";
        if (strcmp(lang_code, "it") == 0) return "Nessun file di avvio.";
        if (strcmp(lang_code, "de") == 0) return "Keine Startdatei vorhanden.";
        if (strcmp(lang_code, "el") == 0) return "Δεν υπάρχει αρχείο εκκίνησης.";
        if (strcmp(lang_code, "ru") == 0) return "Файл запуска отсутствует.";
        if (strcmp(lang_code, "zh") == 0) return "没有启动文件。";
        if (strcmp(lang_code, "tw") == 0) return "沒有啟動檔案。";
        if (strcmp(lang_code, "ja") == 0) return "スタートアップファイルがありません。";
        return "No startup file.";
    }
    if (strcmp(msg, "The app is crashed.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "La aplicación se ha bloqueado.";
        if (strcmp(lang_code, "fr") == 0) return "L’application a planté.";
        if (strcmp(lang_code, "it") == 0) return "L’applicazione è andata in crash.";
        if (strcmp(lang_code, "de") == 0) return "Die App ist abgestürzt.";
        if (strcmp(lang_code, "el") == 0) return "Η εφαρμογή κατέρρευσε.";
        if (strcmp(lang_code, "ru") == 0) return "Приложение завершилось с ошибкой.";
        if (strcmp(lang_code, "zh") == 0) return "应用程序已崩溃。";
        if (strcmp(lang_code, "tw") == 0) return "應用程式已當機。";
        if (strcmp(lang_code, "ja") == 0) return "アプリケーションがクラッシュしました。";
        return "The app is crashed.";
    }
    if (strcmp(msg, "Failed to initialize the sound.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Error al inicializar el sonido.";
        if (strcmp(lang_code, "fr") == 0) return "Echec de linitialisation du son.";
        if (strcmp(lang_code, "it") == 0) return "Impossibile inizializzare l’audio.";
        if (strcmp(lang_code, "de") == 0) return "Initialisierung des Sounds fehlgeschlagen.";
        if (strcmp(lang_code, "el") == 0) return "Αποτυχία αρχικοποίησης του ήχου.";
        if (strcmp(lang_code, "ru") == 0) return "Не удалось инициализировать звук.";
        if (strcmp(lang_code, "zh") == 0) return "声音初始化失败。";
        if (strcmp(lang_code, "tw") == 0) return "聲音初始化失敗。";
        if (strcmp(lang_code, "ja") == 0) return "サウンドの初期化に失敗しました。";
        return "Failed to initialize the sound.";
    }
    if (strcmp(msg, "Failed to initialize the graphics.") == 0) {
        if (strcmp(lang_code, "es") == 0) return "Error al inicializar los gráficos.";
        if (strcmp(lang_code, "fr") == 0) return "Echec de linitialisation des graphiques.";
        if (strcmp(lang_code, "it") == 0) return "Impossibile inizializzare la grafica.";
        if (strcmp(lang_code, "de") == 0) return "Initialisierung der Grafik fehlgeschlagen.";
        if (strcmp(lang_code, "el") == 0) return "Αποτυχία αρχικοποίησης των γραφικών.";
        if (strcmp(lang_code, "ru") == 0) return "Не удалось инициализировать графику.";
        if (strcmp(lang_code, "zh") == 0) return "图形初始化失败。";
        if (strcmp(lang_code, "tw") == 0) return "圖形初始化失敗。";
        if (strcmp(lang_code, "ja") == 0) return "グラフィックスの初期化に失敗しました。";
        return "Failed to initialize the graphics.";
    }
    return msg;
}
