#include "block.h"
#include "../runtime/classes/string.h"
#include "../runtime/classes/symbol.h"

const char *variable_name(rt_value var)
{
	if(!var)
		return "<nil>";

	rt_value string = rt_string_from_cstr("");

	variable_t *_var = (variable_t *)var;

	switch(_var->type)
	{
		case V_TEMP:
			rt_concat_string(string, rt_string_from_cstr("%"));
			rt_concat_string(string, rt_fixnum_to_s(RT_INT2FIX(_var->index), 0, 0, 0));
			break;

		case V_ARGS:
			rt_concat_string(string, rt_string_from_cstr("<a:"));
			rt_concat_string(string, rt_fixnum_to_s(RT_INT2FIX(_var->index), 0, 0, 0));
			rt_concat_string(string, rt_string_from_cstr(">"));
			break;

		case V_PARAMETER:
			rt_concat_string(string, rt_string_from_cstr("*"));
			rt_concat_string(string, rt_symbol_to_s(_var->name, 0, 0, 0));
			break;

		case V_LOCAL:
			rt_concat_string(string, rt_string_from_cstr("#"));
			rt_concat_string(string, rt_symbol_to_s(_var->name, 0, 0, 0));
			break;

		case V_UPVAL:
			rt_concat_string(string, rt_string_from_cstr("!"));
			rt_concat_string(string, rt_symbol_to_s(_var->name, 0, 0, 0));
			break;

		case V_BLOCK:
			rt_concat_string(string, rt_string_from_cstr("&"));

			if(_var->name)
				rt_concat_string(string, rt_symbol_to_s(_var->name, 0, 0, 0));

			break;
	}

	return rt_string_to_cstr(string);
}

void block_print(block_t *block)
{
	for(size_t i = 0; i < kv_size(block->vector); i++)
	{
		opcode_t *op = kv_A(block->vector, i);

		if(op->type != B_NOP)
		{
			if(op->type == B_LABEL)
				printf("\n");

			opcode_print(kv_A(block->vector, i));
			printf("\n");
		}
	}
}

block_t *block_create(struct compiler *compiler, enum block_type type)
{
	block_t *result = compiler_alloc(compiler, sizeof(struct block));

	result->data = malloc(sizeof(struct block_data));

	result->compiler = compiler;
	result->label_count = 0;
	result->label_usage = kh_init(rt_hash);
	result->self_ref = 0;
	result->require_exceptions = false;
	result->current_exception_block = 0;
	result->current_exception_block_id = (size_t)-1;

	kv_init(result->vector);
	kv_init(result->exception_blocks);
	kv_init(result->upvals);

	result->variables = kh_init(block);

	for(int i = 0; i < VARIABLE_TYPES; i++)
		result->var_count[i] = 0;

	result->type = type;
	result->block_var = 0;

	result->owner = 0;
	result->parent = 0;

	return result;
}