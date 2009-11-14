#pragma once
#include "ast.h"
#include "bytecode.h"
#include "compiler.h"
#include "../runtime/runtime.h"

/*
 * Exception runtime data
 */

enum exception_handler_type {
	E_RUNTIME_EXCEPTION,
	E_CLASS_EXCEPTION,
	E_FILTER_EXCEPTION,
};

struct exception_handler {
	enum exception_handler_type type;
	struct exception_handler *next;
};

struct runtime_exception_handler {
	struct exception_handler common;
	void *rescue_label;
};

struct class_exception_handler {
	struct runtime_exception_handler common;
};

struct exception_block {
	size_t parent_index;
	struct exception_block *parent;
	kvec_t(struct exception_handler *) handlers;
	void *block_label;
	void *ensure_label;
};

/*
 * Block runtime data
 */

struct block_data {
	kvec_t(struct exception_block *) exception_blocks;
	void **break_targets;
	size_t local_storage;
	void *epilog;
};

/*
 * Variables
 */

enum variable_type {
	V_HEAP,
	V_LOCAL,
	V_TEMP,
	V_ARGS
};

#define VARIABLE_TYPES 4

struct temp_variable {
	enum variable_type type;
	size_t index;
};

struct variable {
	enum variable_type type;
	size_t index;
	struct block *owner;
	struct variable *real;
	rt_value name;
};

KHASH_MAP_INIT_INT(block, struct variable *);

/*
 * Block definition
 */

enum block_type {
	S_MAIN,
	S_METHOD,
	S_CLASS,
	S_MODULE,
	S_CLOSURE
};

struct block {
	enum block_type type;
	struct compiler *compiler; // The compiler which owns this block

	struct block *parent; // The block enclosing this one
	struct block *owner; // The first parent that isn't a closure. This field can point to itself.

	/*
	 * Break related fields
	 */
	bool can_break; // If this block can raise a exception because of a break.
	size_t break_id; // Which of the parent break targets this block belongs to.
	size_t break_targets; // Number of child blocks that can raise a break exception.

	/*
	 * Exception related fields
	 */
	kvec_t(struct exception_block *) exception_blocks;
	struct exception_block *current_exception_block;
	size_t current_exception_block_id;
	bool require_exceptions;

	struct block_data *data; // Runtime data structure

	/*
	 * Variable related fields
	 */
	rt_value var_count[VARIABLE_TYPES]; // An array of counters of the different variable types.
	khash_t(block) *variables; // A hash with all the variables declared or used
	kvec_t(struct variable *) parameters; // A list of all parameters except the block parameter.
	struct variable *block_parameter; // Pointer to a named or unnamed block variable.
	kvec_t(struct block *) scopes; // A list of all the heap variable scopes this block requires.
	bool heap_vars; // If any of the variables must be stored on a heap scope.
	struct variable *scope_var; // A variable with the pointer to the heap scope
	struct variable *closure_var; // A variable with the pointer to the closure information

	#ifdef DEBUG
		rt_value label_count; // Nicer label labeling...
	#endif

	size_t self_ref;

	void *prolog; // The point after the prolog of the block.
	struct opcode *epilog; // The end of the block

	kvec_t(struct opcode *) vector; // The bytecode output
};

/*
 * Block functions
 */

struct block *block_create(struct compiler *compiler, enum block_type type);

static inline void block_require_scope(struct block *block, struct block *scope)
{
	for(int i = 0; i < kv_size(block->scopes); i++)
		if(kv_A(block->scopes, i) == scope)
			return;

	kv_push(struct block *, block->scopes, scope);
}

static inline struct opcode *block_get_label(struct block *block)
{
	struct opcode *op = compiler_alloc(block->compiler, sizeof(struct opcode));
	op->type = B_LABEL;
	op->left = 0;

	#ifdef DEBUG
		op->right = block->label_count++;
	#endif

	return op;
}

static inline struct opcode *block_get_flush_label(struct block *block)
{
	struct opcode *op = block_get_label(block);
	op->left = L_FLUSH;

	return op;
}

static inline struct variable *block_get_var(struct block *block)
{
	struct variable *temp = compiler_alloc(block->compiler, sizeof(struct temp_variable));

	temp->type = V_TEMP;
	temp->index = block->var_count[V_TEMP];

	block->var_count[V_TEMP] += 1;

	return temp;
}

static inline size_t block_push(struct block *block, enum opcode_type type, rt_value result, rt_value left, rt_value right)
{
	struct opcode *op = compiler_alloc(block->compiler, sizeof(struct opcode));
	op->type = type;
	op->result = result;
	op->left = left;
	op->right = right;
	kv_push(struct opcode *, block->vector, op);

	return kv_size(block->vector) - 1;
}

static inline struct variable *block_gen_args(struct block *block)
{
	struct variable *var = compiler_alloc(block->compiler, sizeof(struct temp_variable));

	var->type = V_ARGS;
	var->index = block->var_count[V_ARGS];

	block->var_count[V_ARGS] += 1;

	block_push(block, B_ARGS, (rt_value)var, (rt_value)false, 0);

	return var;
}

static inline void block_end_args(struct block *block, struct variable *var, size_t argc)
{
	block_push(block, B_ARGS, (rt_value)var, (rt_value)true, argc);
}

static inline struct opcode *block_emmit_label(struct block *block, struct opcode *label)
{
	kv_push(struct opcode *, block->vector, label);

	return label;
}

static inline void block_print_label(rt_value label)
{
	#ifdef DEBUG
		printf("#%d", ((struct opcode *)label)->right);
	#else
		printf("#%x", label);
	#endif

}

const char *variable_name(rt_value var);

void block_print(struct block *block);

