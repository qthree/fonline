#if defined(CORRODED_NET) && !defined(__NETMSG__)
#define __NETMSG__

#include "Defines.h"
#include "Item.h"
#include "MsgFiles.h"

struct NetmsgLogin {
    uint uidxor, uidor, uidcalc;
    uint uid[ 5 ];
    char name[ MAX_NAME + 1 ];
    char pass_hash[ PASS_HASH_SIZE ];
    uchar default_combat_mode;
};

struct NetmsgRegister {
    char name[ MAX_NAME + 1 ];
    char pass_hash[ PASS_HASH_SIZE ];
};

struct NetmsgMove {
    uint   move_params;
    ushort hx;
    ushort hy;
};

struct NetmsgText {
    uchar how_say;
    ushort len;
    const char* text;
};

struct NetmsgContainerItem {
    uchar transfer_type;
    uint  cont_id;
    uint  item_id;
    uint  item_count;
    uchar take_flags;
};

struct NetmsgUseItem {
    uint   item_id;
    ushort item_pid;
    uchar  rate;
    uchar  target_type;
    uint   target_id;
    ushort target_pid;
    uint   param;
};

struct NetmsgDialog {
    bool is_say;
    uchar is_npc;
    uint  id_npc_talk;
    uchar num_answer;
    const char* text;
};

struct NetmsgBarter {
    Npc* npc;
    uint const* sale_items;
    uint const* buy_items;
    ushort  sale_count;
    ushort  buy_count;
    bool is_free;
};

struct NetmsgPlayersBarter {
    uchar barter;
    uint  param;
    uint  param_ext;
};

struct NetmsgRuleGlobal {
    uchar command;
    uint  param1;
    uint  param2;
};

struct NetmsgSetUserHoloStr {
    Item* holodisk;
    const char* title;
    const char* text;
    ushort title_len;
    ushort text_len;
};

#endif // __NETMSG__