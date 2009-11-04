#include "structures.h"
#include "../../runtime/classes/symbol.h"

node_t *parse_class(struct compiler *compiler)
{
	lexer_next(compiler);

	node_t *result = alloc_node(compiler, N_CLASS);

	if(lexer_require(compiler, T_IDENT))
	{
		result->left = (void *)rt_symbol_from_lexer(compiler);

		lexer_next(compiler);
	}
	else
		result->left = 0;

	parse_sep(compiler);

	struct block *block;

	result->right = alloc_scope(compiler, &block, S_CLASS);
	result->right->right = parse_statements(compiler);

	compiler->current_block = block->parent;

	lexer_match(compiler, T_END);

	return result;
}

node_t *parse_module(struct compiler *compiler)
{
	lexer_next(compiler);

	node_t *result = alloc_node(compiler, N_MODULE);

	if(lexer_require(compiler, T_IDENT))
	{
		result->left = (void *)rt_symbol_from_lexer(compiler);

		lexer_next(compiler);
	}
	else
		result->left = 0;

	parse_sep(compiler);

	struct block *block;

	result->right = alloc_scope(compiler, &block, S_MODULE);
	result->right->right = parse_statements(compiler);

	compiler->current_block = block->parent;

	lexer_match(compiler, T_END);

	return result;
}

bool is_parameter(struct compiler *compiler)
{
	switch(lexer_current(compiler))
	{
		case T_AMP:
		case T_IDENT:
			return true;

		default:
			return false;
	}
}

void parse_parameter(struct compiler *compiler, struct block *block)
{
	if(is_parameter(compiler))
	{
		bool block_var = lexer_matches(compiler, T_AMP);

		rt_value symbol = rt_symbol_from_lexer(compiler);

		if(block_var)
		{
			if(block->block_var)
				COMPILER_ERROR(compiler, "You can only receive the block in one parameter.");
			else
			{
				block->block_var = scope_define(block, symbol, V_BLOCK, false);
			}
		}
		else
		{
			if(scope_defined(block, symbol, false))
				COMPILER_ERROR(compiler, "Parameter %s already defined.", rt_symbol_to_cstr(symbol));
			else
				scope_define(block, symbol, V_PARAMETER, false);
		}

		lexer_next(compiler);

		if(lexer_matches(compiler, T_COMMA))
			parse_parameter(compiler, block);
	}
	else
		COMPILER_ERROR(compiler, "Expected paramater, but found %s.", token_type_names[lexer_current(compiler)]);
}

node_t *parse_method(struct compiler *compiler)
{
	lexer_state(compiler, TS_NOKEYWORDS);

	lexer_next(compiler);

	lexer_state(compiler, TS_DEFAULT);

	node_t *result = alloc_node(compiler, N_METHOD);

	switch(lexer_current(compiler))
	{
		case T_IDENT:
		case T_EXT_IDENT:
			{
				result->left = (void *)rt_symbol_from_lexer(compiler);

				lexer_next(compiler);
			}
			break;

		default:
			{
				COMPILER_ERROR(compiler, "Expected identifier but found %s", token_type_names[lexer_current(compiler)]);

				result->left = 0;
			}
	}

	struct block *block;

	result->right = alloc_scope(compiler, &block, S_METHOD);

	block->owner = block;

	if(lexer_matches(compiler, T_PARAM_OPEN))
	{
		parse_parameter(compiler, block);

		lexer_match(compiler, T_PARAM_CLOSE);
	}
	else
	{
		if(is_parameter(compiler))
			parse_parameter(compiler, block);

		parse_sep(compiler);
	}

	result->right->right = parse_statements(compiler);

	compiler->current_block = block->parent;

	lexer_match(compiler, T_END);

	return result;
}
