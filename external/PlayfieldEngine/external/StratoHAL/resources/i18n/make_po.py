import os

def convert_to_gettext(input_file):
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()

    sections = content.strip().split('---')
    
    catalog = {}

    for section in sections:
        lines = section.strip().split('\n')
        if not lines or not lines[0].startswith('ID:'):
            continue
        
        msgid = lines[0][3:].strip()
        
        for line in lines[1:]:
            if ':' not in line:
                continue
            lang, msgstr = line.split(':', 1)
            lang = lang.strip()
            msgstr = msgstr.strip()
            
            if lang not in catalog:
                catalog[lang] = []
            
            catalog[lang].append((msgid, msgstr))

    os.makedirs('po_files', exist_ok=True)
    for lang, entries in catalog.items():
        file_path = f'po_files/{lang}.po'
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write('msgid ""\nmsgstr ""\n')
            f.write('"Content-Type: text/plain; charset=UTF-8\\n"\n')
            f.write('"Content-Transfer-Encoding: 8bit\\n"\n\n')
            
            for msgid, msgstr in entries:
                safe_id = msgid.replace('"', '\\"')
                safe_str = msgstr.replace('"', '\\"')
                
                f.write(f'msgid "{safe_id}"\n')
                f.write(f'msgstr "{safe_str}"\n\n')

    print(f"Made {len(catalog)} languages into 'po_files' directory.")

if __name__ == "__main__":
    convert_to_gettext('message.txt')
