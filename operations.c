/*
	operations.c
*/

#include "operations.h"
#include "expression.h"
#include "component.h"
#include "outsider.h"
#include <_stdio.h>


Expression char_type[80];   // char_type means it can be directly followed by any type (ex: +)
Expression oper_type[10];   // oper_type means loaded as a word_type although is an operator (ex: mod)
Expression word_type[80];   // word_type means it must be inbetween space_type or char_type (ex: pi)

Expression constant_type;       // is a literal constant like 1, 23, 'c', "word"
Expression compname_type;       // is a string obtained from component_path_name()
Expression outsider_type;       // is a component defined from an outside program
Expression repl_lhs_type;       // is a variable used by the replacement operator
Expression paramter_type;       // is a parameter to a user defined function
Expression var_func_type;       // is a call to user defined variable or function
Expression dot_call_type;       // is a '.' used for container.component call


static void operation_add( Expression *expr, int ID, int type, int previous, int precedence)
{
	if(!expr) return;
	memset(expr, 0, sizeof(Expression));
	expr->ID = ID;
	expr->type = type;
	expr->previous = previous;
	expr->precedence = precedence;
}


static void operation_load_comp_type()
{
	operation_add( &constant_type  , SET_CONSTANT  , SkipClimbUp | ACONSTANT  , NOTLEAVE , HIGHEST );
	operation_add( &compname_type  , SET_COMPNAME  , SkipClimbUp | ACONSTANT  , NOTLEAVE , HIGHEST );
	operation_add( &outsider_type  , SET_OUTSIDER  , SkipClimbUp | AVARIABLE  , NOTLEAVE , HIGHEST );
	operation_add( &repl_lhs_type  , SET_REPL_LHS  , SkipClimbUp | AVARIABLE  , NOTLEAVE , HIGHEST );
	operation_add( &paramter_type  , SET_PARAMTER  , SkipClimbUp | APARAMTER  , NOTLEAVE , HIGHEST );
	operation_add( &var_func_type  , SET_VAR_FUNC  , SkipClimbUp | AFUNCTION  , NOTLEAVE , HIGHEST );
	operation_add( &dot_call_type  , SET_DOT_CALL  , 0 , ALEAVE , HIGHEST-1 ); // -1 because it is an operator
}


static void operation_load_char_type()
{
	int i=0;

	operation_add( &char_type[i++] , OpenedBracket1 , SkipClimbUp | OPENBRACKET  , NOTLEAVE | AFUNCTION , 1 );
	operation_add( &char_type[i++] , OpenedBracket2 , SkipClimbUp | OPENBRACKET  , NOTLEAVE     ,  1  );
	operation_add( &char_type[i++] , OpenedBracket3 , SkipClimbUp | OPENBRACKET  , NOTLEAVE     ,  1  );
	operation_add( &char_type[i++] , ClosedBracket1 , RightAssoct | CLOSEBRACKET , ALEAVE       ,  2  );
	operation_add( &char_type[i++] , ClosedBracket2 , RightAssoct | CLOSEBRACKET , ALEAVE       ,  2  );
	operation_add( &char_type[i++] , ClosedBracket3 , RightAssoct | CLOSEBRACKET , ALEAVE       ,  2  );

	operation_add( &char_type[i++] , Replacement    , RightAssoct | ABASIC       , ALEAVE       ,  3  );
	operation_add( &char_type[i++] , EndOfStatement ,               ACOMMA       , ALEAVE       ,  4  );
	operation_add( &char_type[i++] , CommaSeparator , RightAssoct | ACOMMA       , ALEAVE       ,  4  );
	operation_add( &char_type[i++] , Concatenate    ,               ABASIC       , ALEAVE       ,  5  );
	operation_add( &char_type[i++] , ConditionAsk   , RightAssoct | ABASIC       , ALEAVE       ,  6  );
	operation_add( &char_type[i++] , ConditionChoose, RightAssoct | ABASIC       , ALEAVE       ,  6  );
	                                                                                      /* see 7,8,9 below */

	operation_add( &char_type[i++] , Assignment     ,               ACOMPARE     , ALEAVE       ,  12 );
	operation_add( &char_type[i++] , EqualTo        ,               ACOMPARE     , ALEAVE       ,  12 );
	operation_add( &char_type[i++] , SameAs         ,               ACOMPARE     , ALEAVE       ,  12 );
	operation_add( &char_type[i++] , NotSame        ,               ACOMPARE     , ALEAVE       ,  13 );
	operation_add( &char_type[i++] , NotEqual       ,               ACOMPARE     , ALEAVE       ,  13 );
	operation_add( &char_type[i++] , LessThan       ,               ACOMPARE     , ALEAVE       ,  14 );
	operation_add( &char_type[i++] , GreaterThan    ,               ACOMPARE     , ALEAVE       ,  14 );
	operation_add( &char_type[i++] , LessOrEqual    ,               ACOMPARE     , ALEAVE       ,  15 );
	operation_add( &char_type[i++] , GreaterOrEqual ,               ACOMPARE     , ALEAVE       ,  15 );

	operation_add( &char_type[i++] , Oper_add       ,               ABASIC       , ALEAVE       ,  16 );
	operation_add( &char_type[i++] , Oper_sub       ,               ABASIC       , ALEAVE       ,  17 );
	operation_add( &char_type[i++] , Oper_pos       , SkipClimbUp | ABASIC       , NOTLEAVE     ,  18 );
	operation_add( &char_type[i++] , Oper_neg       , SkipClimbUp | ABASIC       , NOTLEAVE     ,  19 );

	operation_add( &char_type[i++] , Oper_mul1      ,               ABASIC       , ALEAVE       ,  20 );
	operation_add( &char_type[i++] , Oper_mul2      ,               ABASIC       , ALEAVE       ,  20 );
	operation_add( &char_type[i++] , Oper_div1      ,               ABASIC       , ALEAVE       ,  20 );
	operation_add( &char_type[i++] , Oper_divI      ,               ABASIC       , ALEAVE       ,  20 );
	operation_add( &char_type[i++] , Oper_div2      ,               ABASIC       , ALEAVE       ,  20 );
	operation_add( &char_type[i++] , Oper_mul0      ,               ABASIC       , ALEAVE       ,  21 );
	operation_add( &char_type[i++] , Oper_pow1      , RightAssoct | ABASIC       , ALEAVE       ,  22 );
	operation_add( &char_type[i++] , Oper_pow2      , RightAssoct | ABASIC       , ALEAVE       ,  22 );
	                                                                                        /* see 23 below */

	operation_add( &char_type[i++] , Bitwise_OR     ,               ABASIC       , ALEAVE       ,  24 );
	operation_add( &char_type[i++] , Bitwise_XOR    ,               ABASIC       , ALEAVE       ,  25 );
	operation_add( &char_type[i++] , Bitwise_AND    ,               ABASIC       , ALEAVE       ,  26 );
	operation_add( &char_type[i++] , Bitwise_NOT    , SkipClimbUp | ABASIC       , NOTLEAVE     ,  27 );
	operation_add( &char_type[i++] , Shift_Right    ,               ABASIC       , ALEAVE       ,  28 );
	operation_add( &char_type[i++] , Shift_Left     ,               ABASIC       , ALEAVE       ,  28 );

	operation_add( &char_type[i++] , Oper_dotproduct,               ABASIC       , ALEAVE       ,  30 );
	operation_add( &char_type[i++] , Oper_transpose ,               ACONSTANT    , ALEAVE       ,  31 );
	operation_add( &char_type[i++] , Oper_indexing  ,               ABASIC       , ALEAVE       ,  31 );

	operation_add( &char_type[i++] ,0,0,0,0);
}


static void operation_load_oper_type()
{
	int i=0;

	operation_add( &oper_type[i++] , Logical_OR      , RightAssoct | ALOGICAL     , ALEAVE      ,  7  );
	operation_add( &oper_type[i++] , Logical_AND     , RightAssoct | ALOGICAL     , ALEAVE      ,  8  );
	operation_add( &oper_type[i++] , Logical_NOT     , SkipClimbUp | ALOGICAL     , NOTLEAVE    ,  9  );

	operation_add( &oper_type[i++] , Oper_mod        , RightAssoct | ABASIC       , ALEAVE      ,  23 );

	operation_add( &oper_type[i++] ,0,0,0,0);
}


static void load_constant (int i, enum ID_TWSF ID)
{ operation_add( &word_type[i] , ID , SkipClimbUp | ACONSTANT , NOTLEAVE , HIGHEST ); }

static void load_function (int i, enum ID_TWSF ID)
{ operation_add( &word_type[i] , ID , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST ); }


static void operation_load_word_type()
{
	int i=0;

	load_constant (i++ , Constant_catch     );
	load_constant (i++ , Constant_true      );
	load_constant (i++ , Constant_false     );
	load_constant (i++ , Constant_e_2_718_  );
	load_constant (i++ , Constant_pi_3_141_ );
	load_constant (i++ , SQRT_of_Neg_One    );

	load_function (i++ , Function_factorial );
	load_function (i++ , Function_fullfloor );
	load_function (i++ , Function_getprimes );
	load_function (i++ , Function_srand     );
	load_function (i++ , Function_rand      );

	load_function (i++ , Function_gcd       );
	load_function (i++ , Function_ilog      );
	load_function (i++ , Function_isqrt     );
	load_function (i++ , Function_floor     );
	load_function (i++ , Function_ceil      );

	load_function (i++ , Function_sqrt      );
	load_function (i++ , Function_cbrt      );
	load_function (i++ , Function_exp       );
	load_function (i++ , Function_log       );

	load_function (i++ , Function_cos       );
	load_function (i++ , Function_sin       );
	load_function (i++ , Function_tan       );
	load_function (i++ , Function_acos      );
	load_function (i++ , Function_asin      );
	load_function (i++ , Function_atan      );

	load_function (i++ , Function_cosh      );
	load_function (i++ , Function_sinh      );
	load_function (i++ , Function_tanh      );
	load_function (i++ , Function_acosh     );
	load_function (i++ , Function_asinh     );
	load_function (i++ , Function_atanh     );

	load_function (i++ , Function_cabs      );
	load_function (i++ , Function_carg      );
	load_function (i++ , Function_real      );
	load_function (i++ , Function_imag      );
	load_function (i++ , Function_conj      );
	load_function (i++ , Function_proj      );

	load_function (i++ , Function_size      );
	load_function (i++ , Function_span      );
	load_function (i++ , Function_sum       );
	load_function (i++ , Function_max       );
	load_function (i++ , Function_min       );

	load_function (i++ , Function_vector    );
	load_function (i++ , Function_range     );
	load_function (i++ , Function_try       );

	load_function (i++ , Function_tostr     );
	load_function (i++ , Function_tonum     );
	load_function (i++ , Function_toint     );
	load_function (i++ , Function_torat     );
	load_function (i++ , Function_toflt     );

	load_function (i++ , Function_eval      );
	load_function (i++ , Function_call      );
	load_function (i++ , Function_print     );
	load_function (i++ , Function_strlen    );

	load_function (i++ , Function_alert     );
	load_function (i++ , Function_confirm   );
	load_function (i++ , Function_prompt    );
	load_function (i++ , Function_read      );
	load_function (i++ , Function_write     );

	load_function (i++ , Function_PcnToChr  );
	load_function (i++ , Function_ChrToPcn  );
	load_function (i++ , Function_ChrToFcn  );
	load_function (i++ , Function_SetIsFcn  );

	operation_add( &word_type[i++] ,0,0,0,0);
}


void operations_init (value stack)
{
	texts_load_twst(stack, RFET_ENGLISH);
	texts_load_twsf(stack, RFET_ENGLISH);
	operation_load_comp_type();
	operation_load_char_type();
	operation_load_oper_type();
	operation_load_word_type();
}


//----------------------------------------------------------------------------------------


static value pointer_resolve (value v, const_value n, const_value beg, const_value end)
{
	if(end==NULL) end--;
	n = vGet(n);
	if(beg<=n && n<=end)
	{
		uint32_t a = *n;
		if(VTYPE(a)==VALUE_VECTOR)
		{
			a = VECTOR_LEN(a);
			value w = v;
			v += 2; // reserve space for vector header
			n += 2; // skip vector header
			int i;
			for(i=0; i<a; i++)
			{
				if(i) n = vNext(n);
				v = unOffset(pointer_resolve(v, n, beg, end));
			}
			setVector(w, a, v-w-2);
			v = setOffset(v, v-w);
		}
		else v = vCopy(v, n);
	}
	return v;
}


static value doCall (value v) // TODO: provide a special ValueType to store the container/component (same for 'this') which however prints itself as a string. But what if the container is inside a LHS and has been deleted? Answer: this will be equivalent to a constant value that has changed during parsing.
{
	value y = vPrev(v);
	const_value n = vGet(y);
	uint32_t a = *n;
	if(VTYPE(a)==VALUE_MESSAGE) return v;

	if(VTYPE(a)==VALUE_VECTOR){
		a = VECTOR_LEN(a);
		n += 2; // skip vector header
	} else a=1;

	int i, count;
	Container* container = NULL;
	{
		if(!a || !isStr2(n))
			return setError(y, L"1st argument must be the path to function to call.");
		bool fullAccess = n[1]&1;

		const wchar* wstr = getStr2(n);
		i = strlen2(wstr);
		lchar lstr[1000]; // [i+1]
		Str3 path = set_lchar_array(lstr, i+1, wstr, L"path to function");

		if(wstr && *wstr!='|')
			return setError(y, L"Given path to function must start with a '|'.");
		else container = container_find(v, NULL, path, fullAccess);

		if(!container){
			assert(fullAccess == false);
			assert(VERROR(vnext(v)));
			return vCopy(y, v);
		}
	}
	n = vNext(n); // skip function name
	const_value t;
	uint64_t mask;

	if(a<=2) mask = count = a-1;
	else{
		count = a-2;
		t = vGet(n);
		if(value_type(t) != aSmaInt)
			return setError(y, L"Second argument must be an integer mask.");
		mask = getSmaInt(t);
		n = vNext(n);
	}

	uint32_t arg[100*4];
	const_value m[100];
	int len[100];
	for(i=0; i<count; i++)
	{
		t = vGet(n);
		a = *t;
		if(VTYPE(a)==VALUE_VECTOR && (mask & (1L<<i)))
		{
			a = VECTOR_LEN(a);
			if(a){
				len[i] = a-1;
				m[i] = t+2;
			}
		}
		else a=0;
		if(!a){
			len[i] = 0;
			m[i] = t;
		}
		n = vNext(n);
	}

	v += 2; // see e-=2 below
	value e = v;
	a = 0;
	while(true)
	{
		a++;
		value w = arg;
		for(i=0; i<count; i++)
			w = setAbsRef(w, m[i]);
		if(count>1) tovector(w, count);
		else if(count==0) setVector(arg,0,0);

		component_evaluate(v, container, container, arg);
		if(VTYPE(*v)) v = vNEXT(v);
		else return vCopy(y, v);

		w = NULL;
		for(i=0; i<count; i++)
			if(len[i]) {
				len[i]--;
				m[i] = vNext(m[i]);
				w++;
			}
		if(!w) break;
	}

	if(a != 1) // if result is a vector
	{
		e -= 2; // reserve space for vector header
		setVector(e, a, v-e-2); // set vector header
	}
	return vPrevCopy(y, pointer_resolve(v,e,y,NULL));
}


static value doEval (value v) // TODO: avoid nested eval(eval())
{
	value y = vPrev(v);
	const_value n = vGet(y);
	if(VTYPE(*n)==VALUE_MESSAGE) return v;

	const_value argument = NULL;
	uint32_t c = *n;
	if(VTYPE(c)==VALUE_VECTOR)
	{
		n += 2; // skip vector header
		c = VECTOR_LEN(c); // get vector length
		if(c==2) argument = vNext(n);
		else if(c!=1) n=NULL; // mark error
	}
	if(!n || !isStr2(n)) // if error
		v = setError(y, L"Valid usage is eval(\"rfet\") or\r\neval(\"\\\\(parameter)= rfet\", argument).");
	else
		v = vPrevCopy(y, rfet_parse_and_evaluate(v, getStr2(n), L"eval()", argument));
	return v;
}


static value valueSt_compare (value v, const_value obtained, const_value expected, value p)
{
	assert(obtained && expected);
	assert(vGet(expected)==expected);
	obtained = vGet(obtained);
	uint32_t a = *obtained;
	uint32_t b = *expected;

	if(VTYPE(b)==VALUE_VECTOR)  // if a vector is expected
	{
		if(VTYPE(a)==VALUE_VECTOR)  // if a vector is obtained
		{
			a = VECTOR_LEN(a);
			b = VECTOR_LEN(b);
			if(a != b) return NULL; // error, must have same length

			obtained += 2; // skip vector header
			expected += 2;
			while(a--)
			{
				p = valueSt_compare(v, obtained, expected, p);
				if(p==NULL) return NULL;
				obtained = vNext(obtained);
				expected = vNext(expected);
			}
			return p ? p : (value)1;
		}
		else return NULL;
	}
	else if(VTYPE(a)==VALUE_VECTOR
	     && (!p || !VECTOR_LEN(a)))
		return NULL;

	if(p > (value)1) {
		p -= 2;
		*(const_value*)p = obtained;
	}
	return p ? p : (value)1;
}


static value setcatch (value out, const wchar* message)
{
	value v = setStr22(out, message);
	uint32_t a = out[-1];
	if(VTYPE(a)==VALUE_OFFSET)
		a &= 0x0FFFFFFF;
	else a = 0;
	return setOffset(v, v-out+1+a);
}


static value compare_failed (value v, const_value obtained, const_value expected, int ID)
{
	const_Str2 argv[3];
	value w;
	v = VstToStr(setRef(v, obtained), TOSTR_CATEGORY);
	w = VstToStr(setRef(v, expected), TOSTR_CATEGORY);
	argv[0] = TWST(ID);
	argv[1] = getStr2(vGetPrev(v));
	argv[2] = getStr2(vGetPrev(w));
	return setMessage(w, 0, 3, argv);
}

#define OPEV(v) ((OperEval*)v)
#define GET_COMP(v) (*(Component**)(size_t)(v))
#define GET_OPER_ID(a) (((a)>>16) & 0x0FFF) // get the 12-bits [23:16]
#define GET_OPER_SIZE(a) (1+((a) & 0xFFFF)) // get 1 + 16-bits [15:0]


value operations_evaluate (value stack, const_value oper)
{
	assert(oper && stack);
	if(!stack || !oper || !VTYPE(*oper))
		return vCopy(stack, oper); // TODO: review this line in operations_evaluate()

	Component *comp=NULL;
	const_Str3 name={0};
	const_Str2 argv[2];

	if(VTYPE(*oper)==VALUE_VECTOR) oper+=2;
	assert(VTYPE(*oper)==VALUE_OPERAT);
	value P = stack;
	value try = P;
	value v = P + OperEvalSize; // see expression.h
	v += OPEV(P)->start;
	OPEV(P)->start = 0; // this is now used for counting recursions

	while(true) // main loop
	{
		uint32_t a = *oper;
		enum ID_TWSF ID = GET_OPER_ID(a);
		if(ID==0) // if end of evaluation then do a return-call
		{
			bool error = VERROR(v);
			if(a & 0xFFFF // if evaluating a component
			&& !error)
			{
				// get the component that contains this expression
				comp = GET_COMP(oper+1); assert(comp!=NULL);

				// get to result of evaluation
				const_value r = vGetPrev(v);

				// if component has an expected result structure
				// then check result structure vs expected
				const_value e = c_expc(comp);
				if(*e && !valueSt_compare(v, r, e, NULL))
				{
					// if comparison failed then get error message
					v = compare_failed(v, r, e, ResultSt_vs_Expected);
					error = true;
				}

				else
				if(comp->state==ISPARSE // if after component_finalise()
				&& comp->para1[0]==0)   // and if component is a variable
				{
					long size = vSize(r);
					if(1+size < SIZEOF(comp->para1)*2)
					{
						// the if below is also found in component_destroy()
						if(comp->constant != 1+comp->para1)
							value_free(comp->constant);
						comp->constant = 1+comp->para1;
					}
					else{ // code above is just malloc-phobia really!
						comp->constant = value_alloc(comp->constant, size);
						assert(comp->constant != NULL);
					}
					memcpy(comp->constant, r, size*sizeof(*r));
					comp->instance = evaluation_instance(0);
					comp->caller = OPEV(P)->caller;
				}
			}

			value p = P;
			oper = OPEV(p)->opers;
			try = p - OPEV(p)->p_try;
			P   = p - OPEV(p)->stack;
			p   = ( ~ OPEV(p)->result ) ?
			     (p - OPEV(p)->result ) : NULL; // get final location of result

			if(!oper)
			{
				if(error)
				{
					if(sChar(name)=='"' || sChar(name)=='\'')
					     argv[0] = L"Error on %s at (%s,%s) in %s:\r\n%s";
					else argv[0] = L"Error on '%s' at (%s,%s) in %s:\r\n%s";
					argv[1] = getMessage(vGetPrev(v));
					v = setMessageE(v, 0, 2, argv, name);
				}
				if(p) v = vPrevCopy(p, v);
				break; // quit main loop
			}
			if(error) goto on_error;
			if(p) v = vPrevCopy(p, v);
			continue;
		}
		name = *(const_Str3*)(oper+1); // record where the expression's name

		if(ID==OperJustJump) { oper += GET_OPER_SIZE(a); continue; }

		if(ID==SET_DOT_CALL     // if a container.variable call
		|| ID==SET_DOT_FUNC)    // if a container.function call
		{
			v = vPrev(v);
			const_value n = vGet(v);
			if(!isStr2(n))
			{
				argv[0] = L"Left to operation must evaluate to a string.";
				v = setMessage(v, 0, 1, argv);
				goto on_error;
			}
			else{
				bool fullAccess = n[1]&1; // see container_path_name()

				// construct pathname
				const_Str2 path = getStr2(n);
				long pLen = strlen2(path);
				lchar lstr[1000+1]; // [pLen+1]
				Str3 pathname = set_lchar_array(lstr, pLen+1, path, L"path");

				const_Str3 Name = name;
				lstr[pLen].chr.c = '|';
				lstr[pLen].next = Name.ptr->next;
				pathname.end = Name.end;

				// get the component that contains this expression
				comp = GET_COMP(oper+1+4); assert(comp!=NULL);

				// get the containing container of that component
				Container* current = c_container(comp);

				Name.ptr = lstr; // get path as a Str3.
				Name.end = lstr+pLen;
				if(path && *path=='|' && !isSpecial3(sNext(Name))) // if path = "|filename"
				{
					Container* container = container_find(v, current, Name, 0);
					if(container==NULL) // if file was not loaded before
					{
						// Note: a 'pointing' string (i.e. for which Name.end != NULL) must
						// be passed to container_parse() or else it will get memory-freed.
						container = container_parse(v, NULL, Name, C37(NULL));
						if(container && !dependence_parse(v)) container=NULL;
						dependence_finalise(container!=NULL);
						if(container==NULL) comp=NULL; // mark error
					}
				}
				if(comp) comp = container_find(v, current, pathname, fullAccess);
				if(!comp) // if error
				{
					assert(v == vPrev(vnext(v)));
					v = vnext(v); // go to after VALUE_MESSAGE
					goto on_error;
				}
				int p = *c_para(comp); // get 1 + number of parameters
				if(p==0 && ID != SET_DOT_CALL) // if error
				{
					argv[0] = L"Expected a variable not a function.";
					v = setMessage(v, 0, 1, argv);
					goto on_error;
				}
				if(p>=1 && ID != SET_DOT_FUNC) // if error
				{
					argv[0] = L"Expected a function not a variable.";
					v = setMessage(v, 0, 1, argv);
					goto on_error;
				}
				ID = SET_DOT_CALL; // prepare for next if() below
			}
		}
		if(ID==SET_DOT_CALL
		|| ID==SET_VAR_FUNC) // see struct OperEval inside expression.h
		{
			if(ID==SET_VAR_FUNC)
			{
				// get the component that contains this expression
				comp = GET_COMP(oper+1+4); assert(comp!=NULL);

				if(comp->replace         // if is a 'replaced' component
				|| comp->state!=ISPARSE) // if before component_finalise()
				{
					Container* caller = OPEV(P)->caller;
					Component* c = component_find(v, caller, name, 0);
					if(c && c_access(c)==ACCESS_REPLACE) comp = c;
				}

				// if variable already evaluated before
				if(comp->instance == evaluation_instance(0)
				&& comp->caller == OPEV(P)->caller)
				{
					v = vCopy(v, comp->constant); // then just copy it
					oper += GET_OPER_SIZE(a);
					continue;
				}
			}

			uint16_t recurs = OPEV(P)->start;
			if(++recurs == 10000) // if too many recursive calls
			{
				setErrorE(v, L"Warning on '%s' at (%s,%s) in %s:\r\nFound too many recursive calls. Continue?", name);
				if(!user_confirm(L"Warning", getMessage(v)))
				{
					v = setError(v, L"Found too many recursive calls.");
					goto on_error;
				}
			}
			const_value w = c_para(comp);
			unsigned int C = *w;
			if(C)   // if a function then get number of parameters
			   C--; // see component_insert() inside component.c
			assert(C<=0xFF);

			value p = v;
			if(C)
			{
				v = vPrev(v);   // get to function arguments
				p += 2*C;       // reserve memory for argument pointers
				w++;            // go to parameter value structure
				if(!valueSt_compare(p, v, w, p)) // set argument pointers
				{
					v = compare_failed(p, v, w, Argument_vs_Parameter);
					//p = NULL; // notify error for if(p) below
					goto on_error;
				}
			}
			//if(p) // false if notified above
			{
				// push values to be recovered on return
				// see struct OperEval in expression.h
				{
					p[0] = C | (P[0] & 0xFF00) | ((uint32_t)recurs<<16);
					//OPEV(p)->paras = C;
					//OPEV(p)->input = OPEV(P)->input;
					//OPEV(p)->start = recurs;

					OPEV(p)->result = p-v;
					OPEV(p)->stack = p-P;
					OPEV(p)->p_try = p-try;

					oper += GET_OPER_SIZE(a);   // tell return-from-call to
					OPEV(p)->opers = oper;      // point to the next operation
				}
				if(ID==SET_DOT_CALL)                      // if a container.component call
				     OPEV(p)->caller = c_container(comp); // then get new caller container
				else OPEV(p)->caller = OPEV(P)->caller;   // else copy old caller

				// finally do the component call
				assert(c_oper(comp)!=NULL);
				oper = c_oper(comp);
				if(VTYPE(*oper)==VALUE_VECTOR) oper+=2;
				assert(VTYPE(*oper)==VALUE_OPERAT);
				P = p;
				try = P;
				v = P + OperEvalSize; // see expression.h
				continue;
			}
		}
		else if(ID==SET_PARAMTER)
		{
			a &= 0xFFFF; // get parameter position
			assert(a < OPEV(P)->paras);
			a = (a+1)*2; // get pointer offset
			v = vCopy(v, *(const_value*)(P-a));
			oper += 1;
			continue;
		}
		else if(ID==ConditionAsk)
		{
			v = vPrev(_not(v));
			const_value n = vGet(v);
			if(!isBool(n)) // if result is not boolean
			{
				assert(VTYPE(*n)==VALUE_VECTOR);
				argv[0] = TWST(Condition_IsNot_Single);
				v = setMessage(v, 0, 1, argv);
			}
			else if(*n & 1) // if condition was false: _not(false) == true
			     { oper += *(oper+1+4); continue; }
			else { oper += GET_OPER_SIZE(a); continue; }
		}
		else if(ID==Function_try)
		{
			oper += GET_OPER_SIZE(a);

			if(try==P) {
				try = v;
				v = setcatch(v, NULL);
			}
			else
			{
				argv[0] = L"A try() inside another is not allowed.";
				v = setMessage(v, 0, 1, argv);
				while(true) // jump to the end of opers
				{
					a = *oper;
					if(GET_OPER_ID(a)==0) break;
					oper += GET_OPER_SIZE(a);
				}
			}
			continue;
		}
		else if(ID==Function_try_that)
		{
			while(true) // jump to the corresponding catch
			{
				a = *oper;
				oper += GET_OPER_SIZE(a);
				if(GET_OPER_ID(a)==Function_try_catch) { try=P; break; }
			}
			continue;
		}
		else if(ID==Constant_catch)
		{
			if(try==P) v = setStr22(v, NULL);
			else v = vCopy(v, try);
			oper += GET_OPER_SIZE(a);
			continue;
		}
		else if(ID==Replacement) // Also see expression_to_operation() in expression.c
		{
			long size = oper[1+4+2+2];  // get offset to the LHS constant
			value w = (value)(size_t)(oper - size);   // get LHS constant

			if((*(w-1) & 0xFFFF)==0)    // if LHS code was just executed just now
			{
				v = vPrev(v);
				const_value n = vGet(v);
				*(w-1) = (VALUE_OPERAT<<28) | (OperJustJump<<16) | (uint16_t)size;
				memcpy(w, n, vSize(n)*sizeof(*n)); // TODO: check size limit, also use vCopy instead
			}
			long* t = (long*)(size_t)(oper+1+4+2);
			size = evaluation_instance(0);
			if(*t != size)
			{
				*t = size;

				// get the component that contains this expression
				comp = GET_COMP(oper+1+4); assert(comp!=NULL);

				if(c_container(comp) == OPEV(P)->caller) // check if on caller container
				{
					// go execute the Right Hand Size of ':='
					oper += GET_OPER_SIZE(a);
					continue;
				}
			}
			v = vCopy(v, w); // copy LHS constant of ':='
			oper += oper[1+4+2+2+1]; // skip RHS code of ':='
			continue;
		}
		else if(ID==ReplaceRecord)
		{
			const_value repl = oper - oper[1];
			value w = (value)(size_t)(repl - repl[1+4+2+2]); // get LHS constant
			const_value u = vGetPrev(v);

			sameAs(setRef(setRef(v, w), u));
			if(!(*v & 1)) // if w and u are not equal
			{
				memcpy(w, u, (v-u-1)*sizeof(*u)); // copy new LHS constant TODO: check for overflow
				replacement_record(repl);
			}
			oper += GET_OPER_SIZE(a);
			continue;
		}
		else if(ID==Function_print)
		{
			value p = stack;
			p  = ( ~ OPEV(p)->result ) ?
			     (p - OPEV(p)->result) : NULL; // get final location of result

			v = VstToStr(v, TOSTR_NEWLINE);
			argv[0] = L"[On \"%s\" at (%s,%s) in %s]\r\n%s";
			argv[1] = getStr2(vGetPrev(v));
			v = vPrevCopy(p, setMessageE(v, 0, 2, argv, name));
			break; // quit main loop
		}
		else{
			switch(ID) // TODO: maybe fetch from an array of "value (*)(value v)" functions
			{
			case Function_try_catch: try=P; break;
			case SET_CONSTANT   : v = vCopy(v, oper+1); break;
			case SET_REPL_LHS   : v = vCopy(v, oper - oper[1]); break;
			case SET_OUTSIDER   : v = set_outsider(v, oper[1+4]); break;

			case Logical_OR     : v = logical_or(v); break;
			case Logical_AND    : v = logical_and(v); break;
			case Logical_NOT    : v = logical_not(v); break;

			case Assignment     : v = equalTo(v); break;
			case EqualTo        : v = equalTo(v); break;
			case SameAs         : v = sameAs(v); break;
			case NotSame        : v = notSame(v); break;
			case NotEqual       : v = notEqual(v); break;
			case LessThan       : v = lessThan(v); break;
			case GreaterThan    : v = greaterThan(v); break;
			case LessOrEqual    : v = lessOrEqual(v); break;
			case GreaterOrEqual : v = greaterOrEqual(v); break;

			case Oper_pos       : v = _pos(v); break;
			case Oper_neg       : v = _neg(v); break;
			case Oper_add       : v = _add(v); break;
			case Oper_sub       : v = _sub(v); break;

			case Oper_mul0      :
			case Oper_mul1      : v = __mul(v); break;
			case Oper_mul2      : v = _mul(v); break;
			case Oper_div1      : v = __div(v); break;
			case Oper_divI      : v = _idiv(v); break;
			case Oper_div2      : v = _div(v); break;
			case Oper_pow1      : v = power(v); break;
			case Oper_pow2      : v = _pow(v); break;
			case Oper_mod       : v = _mod(v); break;

			case Bitwise_OR     : v = bitwise_or(v); break;
			case Bitwise_XOR    : v = bitwise_xor(v); break;
			case Bitwise_AND    : v = bitwise_and(v); break;
			case Bitwise_NOT    : v = bitwise_not(v); break;
			case Shift_Right    : v = shift_right(v); break;
			case Shift_Left     : v = shift_left(v); break;

			case CommaSeparator : v = tovector(v, oper[1]); break;
			case Concatenate    : v = combine(v); break;
			case Oper_indexing  : v = indexing(v); break;
			case Oper_transpose : v = transpose(v); break;
			case Oper_dotproduct: v = dotproduct(v); break;

			case Constant_true      : v = setBool(v, true); break;
			case Constant_false     : v = setBool(v, false); break;
			case Constant_e_2_718_  : v = _exp (setSmaInt(v, 1)); break;
			case Constant_pi_3_141_ : v = _acos(setSmaInt(v,-1)); break;
			case SQRT_of_Neg_One    : v = _sqrt(setSmaInt(v,-1)); break;

			case Function_factorial : v = factorial(v); break;
			case Function_fullfloor : v = fullfloor(v); break;
			case Function_getprimes : v = getprimes(v); break;
			case Function_srand     : v = _srand(v); break;
			case Function_rand      : v = _rand(v); break;

			case Function_gcd   : v = _gcd(v); break;
			case Function_ilog  : v = _ilog(v); break;
			case Function_isqrt : v = _isqrt(v); break;
			case Function_floor : v = _floor(v); break;
			case Function_ceil  : v = _ceil(v); break;

			case Function_sqrt  : v = _sqrt(v); break;
			case Function_cbrt  : v = _cbrt(v); break;
			case Function_exp   : v = _exp(v); break;
			case Function_log   : v = _log(v); break;

			case Function_cos   : v = _cos(v); break;
			case Function_sin   : v = _sin(v); break;
			case Function_tan   : v = _tan(v); break;
			case Function_acos  : v = _acos(v); break;
			case Function_asin  : v = _asin(v); break;
			case Function_atan  : v = _atan(v); break;

			case Function_cosh  : v = _cosh(v); break;
			case Function_sinh  : v = _sinh(v); break;
			case Function_tanh  : v = _tanh(v); break;
			case Function_acosh : v = _acosh(v); break;
			case Function_asinh : v = _asinh(v); break;
			case Function_atanh : v = _atanh(v); break;

			case Function_cabs  : v = _CAbs(v); break;
			case Function_carg  : v = _CArg(v); break;
			case Function_real  : v = _Real(v); break;
			case Function_imag  : v = _Imag(v); break;
			case Function_conj  : v = _Conj(v); break;
			case Function_proj  : v = _Proj(v); break;

			case Function_size  : v = _size(v); break;
			case Function_span  : v = _span(v); break;
			case Function_sum   : v = _sum(v); break;
			case Function_max   : v = _max(v); break;
			case Function_min   : v = _min(v); break;

			case Function_vector: v = _vector(v); break;
			case Function_range : v = _range(v); break;

			case Function_tostr : v = toStr(v); break;
			case Function_tonum : v = toNum(v); break;
			case Function_toint : v = toInt(v); break;
			case Function_torat : v = toRat(v); break;
			case Function_toflt : v = toFlt(v); break;

			case Function_eval   : v = doEval (v); break;
			case Function_call   : v = doCall (v); break;
			case Function_strlen : v = vStrLen(v); break;

			case Function_alert  : v = oper_alert  (v); break;
			case Function_confirm: v = oper_confirm(v); break;
			case Function_prompt : v = oper_prompt (v); break;
			case Function_read   : v = oper_read   (v); break;
			case Function_write  : v = oper_write  (v); break;

			case Function_PcnToChr: v = PcnToChr(v); break;
			case Function_ChrToPcn: v = ChrToPcn(v); break;
			case Function_ChrToFcn: v = ChrToFcn(v); break;
			case Function_SetIsFcn: v = SetIsFcn(v); break;

			default: assert(false); break;
			}
			oper += GET_OPER_SIZE(a);
		}
		if(VERROR(v))
		{
			on_error: ;
			bool skip = false;
			while(true)
			{
				a = *oper;
				ID = GET_OPER_ID(a);
				if(ID==0) break;

				oper += GET_OPER_SIZE(a);
				if(skip) continue;

				if(ID==Function_try_catch) skip = true;
				if(ID==Function_try_that )
				{
					v = setcatch(try, getMessage(vGetPrev(v)));
					break;
				}
			}
		}
	}
	return v;
}

