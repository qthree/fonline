#ifndef __API_ANGELSCRIPT__
#define __API_ANGELSCRIPT__

#include "API_Common.h"

#ifndef __API_IMPL__
struct ScriptString;
#endif //__API_IMPL__

EXPORT const char* Script_String_c_str(const ScriptString *string);

#endif // __API_ANGELSCRIPT__