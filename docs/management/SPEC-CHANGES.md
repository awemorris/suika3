Records of Specification Changes
================================

This file manages the records of specification changes.

---
## Change the definition of `last messages` in save screens

| Item          | Details                                                                 |
|---------------|-------------------------------------------------------------------------|
| ID            | SC-20260427-001                                                         |
| Target LTS    | 26.07 LTS                                                               |
| Refereneces   | [GitHub Issue #29](https://github.com/awemorris/suika3/issues/29)       |
| Added         | 27 April 2026                                                           |

### Description

- Before:
    - In the page mode, `last messages` in save screens starts from
      the beginning of the entire text of the page.
- After:
    - In the page mode, `last messages` in save screens starts from
      the beginning of the last-appended text to the page.

### Outline of The Discussion

Background:
- The change was requested to fit to the naturality.

Concerns:
- None

### Notes on Progress

- 2026-04-27: Agreed to implement.
- 2026-04-27: Implemented.

### Related Commits

* 62cc228e5b245a9f658d31028184805d2341adda `[New Feature] Show only the last appended message in NVL save data`

---
Add here
