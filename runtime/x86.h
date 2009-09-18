#pragma once
#include "../globals.h"
#include "classes.h"
#include "support.h"

rt_value rt_support_closure(rt_compiled_block_t block, size_t argc, rt_upval_t *argv[]);

rt_value __stdcall rt_support_define_class(rt_value name, rt_value super);
rt_value __stdcall rt_support_define_module(rt_value name);

void __stdcall rt_support_define_method(rt_value name, rt_compiled_block_t block);

rt_compiled_block_t __cdecl rt_support_lookup_method(rt_value obj);
rt_upval_t *rt_support_upval_create(void);

rt_value rt_support_get_ivar(void);
void __stdcall rt_support_set_ivar(rt_value value);

rt_value rt_support_get_upval(void);
void __stdcall rt_support_set_upval(rt_value value);
