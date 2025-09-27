#include "Script.h"

#define __API_IMPL__
#include "API_AngelScript.h"

const char* Script_String_c_str(const ScriptString *string) {
	return string->c_str();
}
