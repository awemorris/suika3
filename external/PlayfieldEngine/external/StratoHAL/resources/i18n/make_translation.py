with open("message.txt") as f:
    lines = f.readlines()

    print("#include <string.h>")
    print("")
    print("const char *hal_get_system_language(void);")
    print("")
    print("const char *hal_gettext(const char *msg)")
    print("{")
    print("    const char *lang_code = hal_get_system_language();")
    print("")
    print("    if (strcmp(lang_code, \"zh-cn\") == 0) lang_code = \"zh\";")
    print("    if (strcmp(lang_code, \"zh-tw\") == 0) lang_code = \"tw\";")
    print("")

    last = ""
    for line in lines:
        line = line[:-1]
        line = line.replace("\"", "\\\"")
        if line.startswith("ID:"):
            print("    if (strcmp(msg, \"" + line[3:] + "\") == 0) {")
            last = line[3:]
        elif line == "---":
            print("        return \"" + last + "\";")
            print("    }")
        else:
            print("        if (strncmp(lang_code, \"" + line[0:2] + "\", 2) == 0) return \"" + line[3:] + "\";")

    print("    return msg;")
    print("}")
