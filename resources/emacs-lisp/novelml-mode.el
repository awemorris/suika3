;; novelml-mode.el --- Major mode for NovelML files -*- lexical-binding: t -*-
;; (C) Copyright 2026 Awe Morris
;; Licensed under the zlib license

(defgroup novelml nil
  "NovelML editing mode."
  :group 'languages)

;; Face color mapping.
(defvar novelml-tag-face 'font-lock-function-name-face)
(defvar novelml-attr-face 'font-lock-variable-name-face)
(defvar novelml-variable-face 'font-lock-warning-face)

;; Tag list.
(defvar novelml-tag-list
  '("anime" "bg" "bgm" "callmacro" "ch" "chapter" "choose"
	"click" "config" "defmacro" "else" "elseif" "endif"
	"endmacro" "goto" "gui" "if" "label" "layer" "load"
	"move" "pencl" "returnmacro" "se" "set" "skip" "text"
	"video" "volume" "wait")
)

;; Make font-lock keywords.
(defvar novelml-font-lock-keywords
  (let ((re-tags (regexp-opt novelml-tag-list)))
    (list
     '("#.*$" . font-lock-comment-face)
     '("\\${[a-zA-Z_][a-zA-Z0-9_]*}" . font-lock-variable-name-face)
     (list (concat "\\[\\(" re-tags "\\)\\b") 1 'font-lock-keyword-face)
     '("\\b\\([a-zA-Z0-9_-]+\\)\\(?==\\)" 1 font-lock-variable-name-face)
     '("\\]" . font-lock-bracket-face)
     '("\"\\([^\"]*\\)\"" 1 font-lock-string-face)
     '("'\\([^']*\\)'" 1 font-lock-string-face)))
)

;; Parameter completion.
(defun novelml--after-tag-completion (str status)
  "Completion properties after tag insertion"
  (when (string= status "finished")
    (cond
     ((string= str "bg") (insert " time=\"0.5\" file=\"\"]") (backward-char 2))
     ((string= str "ch") (insert " time=\"0.5\" center=\"\"]") (backward-char 2))
     ((string= str "if") (insert " lhs=\"\" op="==" rhs=\"\"]"))
     ; Add whatever you like.
     (t (insert " ")))))

;; Tag name completion.
(defun novelml-completion-at-point ()
  "Provide completion after ["
  (let ((bnd (bounds-of-thing-at-point 'symbol)))
    (when (save-excursion
            ;; Check if the beginning of the current word is [
            (goto-char (or (car bnd) (point)))
            (looking-back "\\[" 1))
      ;; Returns a list of (start-pos end-pos suggest-list :props)
      (list (or (car bnd) (point))
            (or (cdr bnd) (point))
            novelml-tag-list
            :exclusive 'no
            :exit-function #'novelml--after-tag-completion)))
)

;; novelml-mode
(define-derived-mode novelml-mode prog-mode "NovelML"
  "Major mode for editing NovelML files."
  :group 'novelml

  ; Syntax highlighting
  (set-syntax-table (let ((st (make-syntax-table)))
                      (modify-syntax-entry ?# "< b" st)
                      (modify-syntax-entry ?\n "> b" st)
                      (modify-syntax-entry ?_ "w" st)
                      st))
  (setq font-lock-defaults '(novelml-font-lock-keywords))
  (setq-local comment-start "# ")

  ; Completion
  (add-hook 'completion-at-point-functions #'novelml-completion-at-point nil t)
  (local-set-key (kbd "TAB") #'completion-at-point)
  (local-set-key (kbd "<tab>") #'completion-at-point)
)

;; Register .novel
(add-to-list 'auto-mode-alist '("\\.novel\\'" . novelml-mode))

;; Provide 'novelml-mode
(provide 'novelml-mode)
