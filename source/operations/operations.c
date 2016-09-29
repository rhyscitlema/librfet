/*
    operations.c
*/

#include <_stdio.h>
#include <_string.h>
#include <_texts.h>
#include <component.h>

Value* set_constant (Expression* expression, const Value* argument) { return value_copy(expression->constant); }
/* The order below matters because of no header files */
#include "operations.h"
#include "opr_types.c"
#include "opr_leaves.c"
#include "opr_debugging.c"
#include "opr_indexing.c"
#include "opr_specific.c"
#include "opr_images.c"
#include "opr_other.c"


Expression character_type[80];  // character_type means it can be directly followed by any type
Expression opr_str_type[10];    // opr_str_type means loaded as a string_type but is actually an operator
Expression string_type[60];     // string_type means it must be followed by a space_type or a character_type

Expression number_type;         // is a constant like 1, 23
Expression variable_type;       // is a user defined variable
Expression function_type;       // is a user defined function
Expression parameter_type;      // is a parameter to a user defined function
Expression userstring_type;     // is a user string, like for example a file name
Expression outsider_type;       // is a variable with name defined from outside
Expression current_type;        // is a variable used by the replacement operation

Expression contcall_type;
Expression indexing_type;



static void operation_add (Expression *expr, int ID, int type, int previous, int precedence,
                           Value* (*evaluate) (Expression* expression, const Value* argument))
{
    if(!expr) return;
    memset(expr, 0, sizeof(Expression));
    expr->ID = ID;
    expr->type = type;
    expr->previous = previous;
    expr->precedence = precedence;
    expr->evaluate = evaluate;
}



static void operation_component_type_load ()
{
    operation_add (&number_type     , 0, SkipClimbUp | ACONSTANT  , NOTLEAVE , HIGHEST , set_number       );
    operation_add (&variable_type   , 0, SkipClimbUp | AVARIABLE  , NOTLEAVE , HIGHEST , set_variable     );
    operation_add (&function_type   , 0, SkipClimbUp | AFUNCTION  , NOTLEAVE , HIGHEST , set_function     );
    operation_add (&parameter_type  , 0, SkipClimbUp | APARAMETER , NOTLEAVE , HIGHEST , set_parameter    );
    operation_add (&userstring_type , 0, SkipClimbUp | ACONSTANT  , NOTLEAVE , HIGHEST , set_userstring   );
    operation_add (&outsider_type   , 0, SkipClimbUp | AVARIABLE  , NOTLEAVE , HIGHEST , set_outsider     );
    operation_add (&current_type    , 0, SkipClimbUp | AVARIABLE  , NOTLEAVE , HIGHEST , set_current      );
    operation_add (&contcall_type   , 0, 0, ALEAVE , HIGHEST-1, set_contcall);
}



static void operation_character_type_load ()
{
    int i=0;

    operation_add (&character_type[i++], Opened_Bracket_1 , SkipClimbUp | OPENBRACKET  , NOTLEAVE | AFUNCTION ,  1 , NULL    );
    operation_add (&character_type[i++], Opened_Bracket_2 , SkipClimbUp | OPENBRACKET  , NOTLEAVE | AFUNCTION ,  1 , NULL    );
    operation_add (&character_type[i++], Opened_Bracket_3 , SkipClimbUp | OPENBRACKET  , NOTLEAVE | AFUNCTION ,  1 , NULL    );

    operation_add (&character_type[i++], Closed_Bracket_1 , RightAssoct | CLOSEBRACKET , ALEAVE       ,  2    , NULL          );
    operation_add (&character_type[i++], Closed_Bracket_2 , RightAssoct | CLOSEBRACKET , ALEAVE       ,  2    , NULL          );
    operation_add (&character_type[i++], Closed_Bracket_3 , RightAssoct | CLOSEBRACKET , ALEAVE       ,  2    , NULL          );

    operation_add (&character_type[i++], EndOfStatement   ,               ACOMMA       , ALEAVE       ,  3    , opr_comma     );
    operation_add (&character_type[i++], CommaSeparator   , RightAssoct | ACOMMA       , ALEAVE       ,  3    , opr_comma     );
    operation_add (&character_type[i++], Concatenate      , RightAssoct | ACOMMA       , ALEAVE       ,  3    , opr_concatenate);

    operation_add (&character_type[i++], Replacement      ,               ABASIC       , ALEAVE       ,  4    , opr_replace   );
    operation_add (&character_type[i++], ConditionAsk     , RightAssoct | ABASIC       , ALEAVE       ,  5    , opr_condition );
    operation_add (&character_type[i++], ConditionChoose  , RightAssoct | ABASIC       , ALEAVE       ,  5    , opr_condition );
                                                                                                     /* see 6,7,8 below */

    operation_add (&character_type[i++], Assignment       ,               ACOMPARE     , ALEAVE       ,  10   , opr_equal_type4          );
    operation_add (&character_type[i++], Equal_1          ,               ACOMPARE     , ALEAVE       ,  10   , opr_equal_type4          );
    operation_add (&character_type[i++], Equal_2          ,               ACOMPARE     , ALEAVE       ,  10   , opr_equal_type2          );
    operation_add (&character_type[i++], NotEqual_1       ,               ACOMPARE     , ALEAVE       ,  11   , opr_notEqual_type4       );
    operation_add (&character_type[i++], NotEqual_2       ,               ACOMPARE     , ALEAVE       ,  11   , opr_notEqual_type2       );

    operation_add (&character_type[i++], LessThan_1       ,               ACOMPARE     , ALEAVE       ,  12   , opr_lessThan_type3       );
    operation_add (&character_type[i++], LessThan_2       ,               ACOMPARE     , ALEAVE       ,  12   , opr_lessThan_type2       );
    operation_add (&character_type[i++], GreaterThan_1    ,               ACOMPARE     , ALEAVE       ,  13   , opr_greaterThan_type3    );
    operation_add (&character_type[i++], GreaterThan_2    ,               ACOMPARE     , ALEAVE       ,  13   , opr_greaterThan_type2    );
    operation_add (&character_type[i++], LessOrEqual_1    ,               ACOMPARE     , ALEAVE       ,  14   , opr_lessOrEqual_type3    );
    operation_add (&character_type[i++], LessOrEqual_2    ,               ACOMPARE     , ALEAVE       ,  14   , opr_lessOrEqual_type2    );
    operation_add (&character_type[i++], GreaterOrEqual_1 ,               ACOMPARE     , ALEAVE       ,  15   , opr_greaterOrEqual_type3 );
    operation_add (&character_type[i++], GreaterOrEqual_2 ,               ACOMPARE     , ALEAVE       ,  15   , opr_greaterOrEqual_type2 );

    operation_add (&character_type[i++], Plus             ,               ABASIC       , ALEAVE       ,  16   , opr_plus_type2           );
    operation_add (&character_type[i++], Minus            ,               ABASIC       , ALEAVE       ,  17   , opr_minus_type2          );
    operation_add (&character_type[i++], Positive         , SkipClimbUp | ABASIC       , NOTLEAVE     ,  18   , opr_positive             );
    operation_add (&character_type[i++], Negative         , SkipClimbUp | ABASIC       , NOTLEAVE     ,  19   , opr_negative             );

    operation_add (&character_type[i++], Times_1          ,               ABASIC       , ALEAVE       ,  20   , opr_times_typeS          );
    operation_add (&character_type[i++], Times_2          ,               ABASIC       , ALEAVE       ,  20   , opr_times_type2          );
    operation_add (&character_type[i++], Divide_1         ,               ABASIC       , ALEAVE       ,  20   , opr_divide_type3         );
    operation_add (&character_type[i++], Divide_2         ,               ABASIC       , ALEAVE       ,  20   , opr_divide_type2         );
    operation_add (&character_type[i++], ToPower_1        , RightAssoct | ABASIC       , ALEAVE       ,  22   , opr_topower_type3        );
    operation_add (&character_type[i++], ToPower_2        , RightAssoct | ABASIC       , ALEAVE       ,  22   , opr_topower_type2        );

    operation_add (&character_type[i++], Ari_Shift_Right  ,               ABASIC       , ALEAVE       ,  23   , opr_shift_right_type2    );
    operation_add (&character_type[i++], Ari_Shift_Left   ,               ABASIC       , ALEAVE       ,  23   , opr_shift_left_type2     );
    operation_add (&character_type[i++], Bitwise_OR       ,               ABASIC       , ALEAVE       ,  24   , opr_bitwise_or_type2     );
    operation_add (&character_type[i++], Bitwise_XOR      ,               ABASIC       , ALEAVE       ,  25   , opr_bitwise_xor_type2    );
    operation_add (&character_type[i++], Bitwise_AND      ,               ABASIC       , ALEAVE       ,  26   , opr_bitwise_and_type2    );
    operation_add (&character_type[i++], Bitwise_NOT      , SkipClimbUp | ABASIC       , NOTLEAVE     ,  27   , opr_bitwise_not          );

                                                                                                       /* see 29 below */
    operation_add (&character_type[i++], DotProduct       ,               ABASIC       , ALEAVE       ,  30   , opr_dot_product          );

    operation_add (&character_type[i++], Factorial        ,               ACONSTANT    , ALEAVE       ,  31   , opr_factorial    );
    operation_add (&character_type[i++], Transpose        ,               ACONSTANT    , ALEAVE       ,  31   , opr_transpose    );
    operation_add (&indexing_type      , 0                ,               ABASIC       , ALEAVE       ,  31   , opr_indexing     );

    operation_add (&character_type[i++], 0,0,0,0,0);
}



static void operation_opr_str_type_load ()
{
    int i=0;

    operation_add (&opr_str_type[i++], Logical_OR         , RightAssoct | ALOGICAL     , ALEAVE       ,  6    , opr_logical_or_type2   );
    operation_add (&opr_str_type[i++], Logical_AND        , RightAssoct | ALOGICAL     , ALEAVE       ,  7    , opr_logical_and_type2  );
    operation_add (&opr_str_type[i++], Logical_NOT        , SkipClimbUp | ALOGICAL     , NOTLEAVE     ,  8    , opr_logical_not        );

    operation_add (&opr_str_type[i++], Modulo_Remainder   , RightAssoct | ABASIC       , ALEAVE       ,  29   , opr_modulo_type2       );

    operation_add (&opr_str_type[i++], 0,0,0,0,0);
}



static void operation_string_type_load ()
{
    int i=0;

    operation_add (&string_type[i++], Constant_E_2_718_            , SkipClimbUp | ACONSTANT , NOTLEAVE , HIGHEST , set_number         );
    operation_add (&string_type[i++], Constant_PI_3_141_           , SkipClimbUp | ACONSTANT , NOTLEAVE , HIGHEST , set_number         );
    operation_add (&string_type[i++], SQRT_of_Negative_One         , SkipClimbUp | ACONSTANT , NOTLEAVE , HIGHEST , set_number         );

    operation_add (&string_type[i++], Function_Power_of_e          , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_exp_e          );
    operation_add (&string_type[i++], Function_Logarithm_Base_10   , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_log_10         );
    operation_add (&string_type[i++], Function_Logarithm_Base_e    , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_log_e          );
    operation_add (&string_type[i++], Function_Square_Root         , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_sqrt           );
    operation_add (&string_type[i++], Function_Math_Ceil           , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_ceil           );
    operation_add (&string_type[i++], Function_Math_Floor          , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_floor          );
    operation_add (&string_type[i++], Function_FullFloor           , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_fullfloor      );
    operation_add (&string_type[i++], Function_GetPrimes           , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_getprimes      );

    operation_add (&string_type[i++], Function_Absolute_Value      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_abs            );
    operation_add (&string_type[i++], Function_Argument_Angle      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_arg            );
    operation_add (&string_type[i++], Function_Real_Part           , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_real           );
    operation_add (&string_type[i++], Function_Imag_Part           , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_imag           );
    operation_add (&string_type[i++], Function_Complex_Conjugate   , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_conj           );
    operation_add (&string_type[i++], Function_Complex_Projection  , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_proj           );

    operation_add (&string_type[i++], Function_aRandom_Number      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_rand           );
    operation_add (&string_type[i++], Function_sRandom_Number      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_srand          );
    operation_add (&string_type[i++], Function_Summation           , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_sum            );
    operation_add (&string_type[i++], Function_Maximum             , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_max            );
    operation_add (&string_type[i++], Function_Minimum             , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_min            );

    operation_add (&string_type[i++], Trigonometric_Sine           , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_sin            );
    operation_add (&string_type[i++], Trigonometric_Cosine         , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_cos            );
    operation_add (&string_type[i++], Trigonometric_Tangent        , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_tan            );
    operation_add (&string_type[i++], Trigonometric_Sine_Inverse   , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_asin           );
    operation_add (&string_type[i++], Trigonometric_Cosine_Inverse , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_acos           );
    operation_add (&string_type[i++], Trigonometric_Tangent_Inverse, SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_atan           );

    operation_add (&string_type[i++], Hyperbolic_Sine              , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_sinh           );
    operation_add (&string_type[i++], Hyperbolic_Cosine            , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_cosh           );
    operation_add (&string_type[i++], Hyperbolic_Tangent           , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_tanh           );
    operation_add (&string_type[i++], Hyperbolic_Sine_Inverse      , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_asinh          );
    operation_add (&string_type[i++], Hyperbolic_Cosine_Inverse    , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_acosh          );
    operation_add (&string_type[i++], Hyperbolic_Tangent_Inverse   , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_atanh          );

    operation_add (&string_type[i++], Generate_Range               , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , generate_range     );
    operation_add (&string_type[i++], Generate_Vector              , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , generate_vector    );
    operation_add (&string_type[i++], Length_of_Value              , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , fct_len            );
    operation_add (&string_type[i++], Size_of_Value                , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , size_of_value      );
    operation_add (&string_type[i++], Print_Value                  , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , print_value        );
    operation_add (&string_type[i++], Try_And_Catch                , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , try_and_catch      );

    operation_add (&string_type[i++], Convert_To_Str               , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , convert_to_str     );
    operation_add (&string_type[i++], Convert_To_Num               , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , convert_to_num     );
    operation_add (&string_type[i++], Convert_To_Sma               , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , convert_to_sma     );
    operation_add (&string_type[i++], Convert_To_Big               , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , convert_to_big     );

    operation_add (&string_type[i++], GetImageWidthHeight          , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , get_image_width_height);
    operation_add (&string_type[i++], GetImagePixelColour          , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , get_image_pixel_colour);
    operation_add (&string_type[i++], GetImagePixelMatrix          , SkipClimbUp | AFUNCTION , NOTLEAVE , HIGHEST , get_image_pixel_matrix);

    operation_add (&string_type[i++], 0,0,0,0,0);
}



void mfet_init()
{
    static bool done=0;
    if(done) return;
    done=1;
    operation_component_type_load();
    operation_character_type_load();
    operation_opr_str_type_load();
    operation_string_type_load();
    SET_VSTXX();
}


