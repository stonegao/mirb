#include "classes.h"
#include "runtime.h"
#include "constant.h"
#include "x86.h"
#include "classes/symbol.h"
#include "classes/string.h"
#include "classes/proc.h"

rt_value __cdecl rt_support_closure(rt_compiled_block_t block, size_t argc, rt_upval_t *argv[])
{
	rt_value self;

	__asm__("" : "=D" (self));

	rt_value closure = (rt_value)malloc(sizeof(struct rt_proc));

	RT_COMMON(closure)->flags = C_PROC;
	RT_COMMON(closure)->class_of = rt_Proc;
	RT_PROC(closure)->self = self;
	RT_PROC(closure)->closure = block;
	RT_PROC(closure)->upval_count = argc;
	RT_PROC(closure)->upvals = malloc(sizeof(rt_upval_t *) * argc);

	RT_ARG_EACH_RAW(i)
	{
		RT_PROC(closure)->upvals[i] = RT_ARG(i);
	}

	return closure;
}

rt_compiled_block_t __cdecl rt_support_lookup_method(rt_value obj)
{
	rt_value method;

	__asm__("" : "=a" (method));
/*
	rt_compiled_block_t result = rt_lookup(obj, method);

	__asm__("jmp %0" : : "r" (result));
*/
	return rt_lookup(obj, method);
}

rt_value __stdcall rt_support_define_class(rt_value name, rt_value super)
{
	rt_value obj;

	__asm__("" : "=D" (obj));

	if(obj == rt_main)
		obj = rt_Object;

	return rt_define_class(obj, name, super);
}

rt_value __stdcall rt_support_define_module(rt_value name)
{
	rt_value obj;

	__asm__("" : "=D" (obj));

	if(obj == rt_main)
		obj = rt_Object;

	return rt_define_module(obj, name);
}

void __stdcall rt_support_define_method(rt_value name, rt_compiled_block_t block)
{
	rt_value obj;

	__asm__("" : "=D" (obj));

	if(obj == rt_main)
		obj = rt_Object;

	rt_define_method(obj, name, block);
}

rt_upval_t *rt_support_upval_create(void)
{
	rt_value *real;

	__asm__("" : "=a" (real));

	rt_upval_t *result = malloc(sizeof(rt_upval_t));

	result->val.upval = real;
	result->sealed = false;

	return result;
}

rt_value rt_support_get_ivar(void)
{
	rt_value obj;
	rt_value name;

	__asm__("" : "=D" (obj), "=a" (name));

	return rt_object_get_var(obj, name);
}

void __stdcall rt_support_set_ivar(rt_value value)
{
	rt_value obj;
	rt_value name;

	__asm__("" : "=D" (obj), "=a" (name));

	rt_object_set_var(obj, name, value);
}

rt_value rt_support_get_upval(void)
{
	size_t index;
	rt_upval_t **upvals;

	__asm__("" : "=a" (index), "=S" (upvals));

	rt_upval_t *upval = upvals[index];

	if(!upval->sealed)
		return *(upval->val.upval);
	else
		return upval->val.local;
}

void __stdcall rt_support_set_upval(rt_value value)
{
	size_t index;
	rt_upval_t **upvals;

	__asm__("" : "=a" (index), "=S" (upvals));

	rt_upval_t *upval = upvals[index];

	if(!upval->sealed)
		*(upval->val.upval) = value;
	else
		upval->val.local = value;
}

#ifdef WINDOWS
	extern void __stdcall RtlUnwind(
		PVOID TargetFrame,
		PVOID TargetIp,
		PEXCEPTION_RECORD ExceptionRecord,
		PVOID ReturnValue
	);

	static void rt_seh_global_unwind(rt_seh_frame_t *target)
	{
		int dummy;

		__asm__ __volatile__("pushl %%ebp\n"
			"pushl %%ebx\n"
			"pushl $0\n"
			"pushl $0\n"
			"pushl rt_seh_global_unwind_continue\n"
			"pushl %0\n"
			"call _RtlUnwind@16\n"
			"rt_seh_global_unwind_continue:\n"
			"popl %%ebx\n"
			"popl %%ebp\n"
		: "=a" (dummy)
		: "0" (target)
		: "esi", "edi", "edx", "ecx", "memory");
	}

	static void rt_seh_local_unwind(rt_seh_frame_t *frame_data, exception_block_t *block, exception_block_t *target)
	{
		frame_data->handling = 1;

		size_t ebp = (size_t)&frame_data->old_ebp;

		while(block != target)
		{
			if(block->ensure_label)
			{
				#ifdef DEBUG
					printf("Ensure block found\n");
					printf("Ebp: %x\n", ebp);
					printf("Eip: %x\n", block->ensure_label);
				#endif

				int dummy1, dummy2;

				__asm__ __volatile__("pushl %%ebp\n"
					"pushl %%ebx\n"
					"mov %0, %%ebp\n"
					"call *%1\n"
					"popl %%ebx\n"
					"popl %%ebp\n"
				: "=a" (dummy1), "=c" (dummy2)
				: "0" (ebp), "1" (block->ensure_label)
				: "esi", "edi", "edx", "memory");
			}

			block = block->parent;
		}

		frame_data->handling = 0;
	}

	static void rt_seh_rescue(rt_seh_frame_t *frame_data, exception_block_t *block, exception_block_t *current_block, void *rescue_label)
	{
		rt_seh_global_unwind(frame_data);

		/*
		 * Execute frame local ensure handlers
		 */

		rt_seh_local_unwind(frame_data, block, current_block);

		/*
		 * Set to the current handler index to the parent
		 */

		frame_data->block_index = current_block->parent_index;

		/*
		 * Go to the handler and never return
		 */

		size_t ebp = (size_t)&frame_data->old_ebp;

		#ifdef DEBUG
			printf("Rescue block found\n");
			printf("Ebp: %x\n", ebp);
			printf("Esp: %x\n", ebp - 20 - 12 - frame_data->block->local_storage);
			printf("Eip: %x\n", rescue_label);
		#endif

		__asm__ __volatile__("mov %0, %%eax\n"
			"mov %1, %%esp\n"
			"mov %2, %%ebp\n"
			"jmp *%%eax\n"
		:
		: "r" (rescue_label),
			"r" (ebp - 20 - 12 - frame_data->block->local_storage),
			"r" (ebp)
		: "%eax");

		//__builtin_unreachable(); // This seems undeclared in gcc...
	}

	EXCEPTION_DISPOSITION __cdecl rt_seh_handler(EXCEPTION_RECORD *exception, rt_seh_frame_t *frame_data, CONTEXT *context, void *dispatcher_context)
	{
		exception_block_t *block = kv_A(frame_data->block->exception_blocks, frame_data->block_index);

		if(exception->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
		{
			rt_seh_local_unwind(frame_data, block, 0);
		}
		else if(exception->ExceptionCode == RT_SEH_RUBY_EXCEPTION)
		{
			exception_block_t *current_block = block;

            while(current_block)
            {
				for(size_t i = 0; i < kv_size(current_block->handlers); i++)
				{
					exception_handler_t *handler = kv_A(current_block->handlers, i);

					switch(handler->type)
					{
						case E_RUNTIME_EXCEPTION:
							rt_seh_rescue(frame_data, block, current_block, ((runtime_exception_handler_t *)handler)->rescue_label);

						default:
							break;
					}
				}

                current_block = current_block->parent;
            }
		}

		return ExceptionContinueSearch;
	}
#endif
