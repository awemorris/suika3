;;; ray-mode.el --- Major mode for Ray (Noct) language -*- lexical-binding: t -*-

(defgroup ray nil
  "Ray programming language editing mode."
  :group 'languages)

(defvar ray-keyword-face 'font-lock-keyword-face)
(defvar ray-builtin-face 'font-lock-builtin-face)
(defvar ray-constant-face 'font-lock-constant-face)

(defvar ray-font-lock-keywords
  (let* (
         (keywords '("func" "var" "return" "if" "else" "while" "for" "in" "class" "extend" "new"))
         (builtins '("print" "readint" "int" "float" "newArray" "push" "pop" "resize" 
                     "charCount" "charAt" "substring" "unset" "remove"
                     "abs" "random" "sin" "cos" "tan" "Suika" "Engine"))
         (constants '("true" "false" "null"))
         (re-keywords (regexp-opt keywords 'words))
         (re-builtins (regexp-opt builtins 'words))
         (re-constants (regexp-opt constants 'words))
         )
    
    (list
     '("//.*$" . font-lock-comment-face)
     (list re-keywords 1 'ray-keyword-face)
     (list re-builtins 1 'ray-builtin-face)
     (list re-constants 1 'ray-constant-face)
     '("\\bfunc\\s-+\\([a-zA-Z_][a-zA-Z0-9_]*\\)" 1 font-lock-function-name-face)
     '("->\\([a-zA-Z_][a-zA-Z0-9_]*\\)" 1 font-lock-function-name-face)
     '("\\bvar\\s-+\\([a-zA-Z_][a-zA-Z0-9_]*\\)" 1 font-lock-variable-name-face)
     '("\\b[0-9]+\\(\\.[0-9]+\\)?\\b" . font-lock-constant-face)
	 '("\"\\([^\"]*\\)\"" 1 font-lock-string-face)
     )))

(defun ray-completion-at-point ()
  "Completion for Ray keywords and intrinsics"
  (let ((bnd (bounds-of-thing-at-point 'symbol)))
    (list (or (car bnd) (point))
          (or (cdr bnd) (point))
          '("func" "var" "return" "if" "else" "while" "for" "in" "class" "extend" "new"
            "print" "readint" "int" "float" "newArray" "push" "pop" "resize" 
            "charCount" "charAt" "substring" "unset" "remove" "abs" "random" "sin" "cos" "tan")
          :exclusive 'no)))

(define-derived-mode ray-mode prog-mode "Ray"
  "Ray major mode"
  :group 'ray
  (set-syntax-table (let ((st (make-syntax-table)))
                      (modify-syntax-entry ?/ ". 12" st)
                      (modify-syntax-entry ?\n ">" st)
                      (modify-syntax-entry ?_ "w" st)
                      st))
  (setq font-lock-defaults '(ray-font-lock-keywords))
  (setq-local comment-start "// ")
  (add-hook 'completion-at-point-functions #'ray-completion-at-point nil t)
  (local-set-key (kbd "TAB") #'completion-at-point)
  (local-set-key (kbd "<tab>") #'completion-at-point))

(add-to-list 'auto-mode-alist '("\\.ray\\'" . ray-mode))

(provide 'ray-mode)
