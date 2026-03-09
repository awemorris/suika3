with open("message.txt") as f:
    lines = f.readlines()

    print("#include <string.h>")
    print("")
    print("const char *noct_get_system_language(void);")
    print("")
    print("const char *noct_gettext(const char *msg)")
    print("{")
    print("    const char *lang_code = noct_get_system_language();")

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
            print("        if (strcmp(lang_code, \"" + line[0:2] + "\") == 0) return \"" + line[3:] + "\";")

    print("    return msg;")
    print("}")
