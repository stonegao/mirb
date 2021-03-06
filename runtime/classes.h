#pragma once
#include "../globals.h"
#include "runtime.h"
#include "classes/bool.h"
#include "classes/fixnum.h"
#include "classes/class.h"
#include "classes/object.h"

extern rt_value rt_main;

extern khash_t(rt_hash) *object_var_hashes;

static inline rt_value rt_class_of(rt_value obj)
{
	if(obj & RT_FLAG_FIXNUM)
		return rt_Fixnum;
	else if(obj <= RT_MAX)
	{
		switch(obj)
		{
			case RT_TRUE:
				return rt_TrueClass;

			case RT_FALSE:
				return rt_FalseClass;

			case RT_NIL:
				return rt_NilClass;

			default:
				RT_ASSERT(0);
		}
	}
	else
		return RT_COMMON(obj)->class_of;
}

static inline rt_value rt_real_class(rt_value obj)
{
	while(obj && ((RT_COMMON(obj)->flags & RT_CLASS_SINGLETON) || (RT_COMMON(obj)->flags & RT_TYPE_MASK) == C_ICLASS))
		obj = RT_CLASS(obj)->super;

	return obj;
}

static inline rt_value rt_real_class_of(rt_value obj)
{
	return rt_real_class(rt_class_of(obj));
}

static inline void rt_class_set_method(rt_value obj, rt_value name, rt_compiled_block_t block)
{
	int ret;

	khiter_t k = kh_put(rt_block, rt_get_methods(obj), name, &ret);

	if(!ret)
	{
		khiter_t k = kh_get(rt_block, RT_CLASS(obj)->methods, name);

		if (k == kh_end(RT_CLASS(obj)->methods))
			RT_ASSERT(0);
	}

	kh_value(RT_CLASS(obj)->methods, k) = block;
}

static inline rt_compiled_block_t rt_class_get_method(rt_value obj, rt_value name)
{
	if(!RT_CLASS(obj)->methods)
		return 0;

	khiter_t k = kh_get(rt_block, RT_CLASS(obj)->methods, name);

	if (k == kh_end(RT_CLASS(obj)->methods))
		return 0;

	return kh_value(RT_CLASS(obj)->methods, k);
}

khash_t(rt_hash) *rt_object_get_vars(rt_value obj);

static inline void rt_object_set_var(rt_value obj, rt_value name, rt_value value)
{
	khash_t(rt_hash) *vars = rt_object_get_vars(obj);

	int ret;

	khiter_t k = kh_put(rt_hash, vars, name, &ret);

	RT_ASSERT(ret);

	kh_value(vars, k) = value;
}

static inline rt_value rt_object_get_var(rt_value obj, rt_value name)
{
	khash_t(rt_hash) *vars = rt_object_get_vars(obj);

	khiter_t k = kh_get(rt_hash, vars, name);

	if (k == kh_end(vars))
		return RT_NIL;

	return kh_value(vars, k);
}

void rt_setup_classes(void);

rt_value rt_define_class(rt_value obj, rt_value name, rt_value super);
rt_value rt_define_module(rt_value obj, rt_value name);

void rt_include_module(rt_value obj, rt_value module);

void rt_define_method(rt_value obj, rt_value name, rt_compiled_block_t block);
void rt_define_singleton_method(rt_value obj, rt_value name, rt_compiled_block_t block);

void rt_class_name(rt_value obj, rt_value under, rt_value name);
rt_value rt_class_create_unnamed(rt_value super);
rt_value rt_class_create_bare(rt_value super);
rt_value rt_class_create_singleton(rt_value object, rt_value super);

