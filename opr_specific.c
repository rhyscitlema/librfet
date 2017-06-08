/*
    opr_specific.c

    This file is #included by operations.c only.
*/



#define VA(A,i) A[1+i]
static bool vector_cross_product (value* R, const value* A, const value* B)
{
    value n, m;
    R[0] = setSepto2(4,3);

    n = multiply (VA(A,1), VA(B,2));
    m = multiply (VA(A,2), VA(B,1));
    VA(R,0) = subtract (n, m);

    n = multiply (VA(A,2), VA(B,0));
    m = multiply (VA(A,0), VA(B,2));
    VA(R,1) = subtract (n, m);

    n = multiply (VA(A,0), VA(B,1));
    m = multiply (VA(A,1), VA(B,0));
    VA(R,2) = subtract (n, m);

    // R[0] = A[1]*B[2] - A[2]*B[1];
    // R[1] = A[2]*B[0] - A[0]*B[2];
    // R[2] = A[0]*B[1] - A[1]*B[0];
    return 1;
}



static bool matrix_multiply (value* R, const value* A, const value* B, int rowA, int colArowB, int colB)
{
    int i, j, k;
    value T, a, b;

    valueSt_matrix_setSize (R, rowA, colB);

    for(i=0; i < rowA; i++)
    for(j=0; j < colB; j++)
    {
        T = setSmaInt(0);
        for(k=0; k < colArowB; k++)
        {
            a = MVA(A,i,k,colArowB);
            b = MVA(B,k,j,colB);
            // do: T = T + a*b;
            T = add(T, multiply(a, b));
        }
        MVA(R,i,j,colB) = T;
    }
    return 1;
}



/*
    Here we evaluate something like A * B where:
    if A or B is a scalar, then 'scalar multiplication'
    if A and B are two vectors, then 'vector cross product'
    if A and B are two matrices, then 'matrix multiplication'
*/
bool opr_times_typeS (ExprCallArg eca)
{
    value n, m, y;
    long headLen, lastLen;
    value *headVst, *lastVst;
    int rowA, colA, rowB, colB;
    value* stack = eca.stack;

    Expression* expression = eca.expression;
    Expression* head = expression->headChild;
    Expression* last = expression->lastChild;

    headVst = eca.stack;
    eca.expression = head;
    if(!expression_evaluate(eca)) return 0;
    headLen = VST_LEN(headVst);

    lastVst = headVst + headLen;
    eca.expression = last;
    eca.stack = lastVst;
    if(!expression_evaluate(eca)) return 0;
    lastLen = VST_LEN(lastVst);

    value* out = lastVst + lastLen;
    wchar* errormessage = eca.garg->message;

    if(lastLen==1)
    {
      m = *lastVst;
      if(headLen==1)
      {
        for(;;)
        {
            y = *headVst;
            { y = multiply (y, m); CHECK_Y }
            *headVst = y;
            break;
        }
      }
      else
      {
        for( ; headLen--; headVst++)
        {
            y = *headVst;
            if(!isSeptor(y))
            { y = multiply (y, m); CHECK_Y }
            *headVst = y;
        }
      }
    }
    else if(headLen==1)
    {
        n = *headVst;
        for( ; lastLen--; headVst++, lastVst++)
        {
            y = *lastVst;
            if(!isSeptor(y))
            { y = multiply (n, y); CHECK_Y }
            *headVst = y;
        }
    }
    else if(!valueSt_matrix_getSize (headVst, &rowA, &colA))
    {
        VstToStr(headVst, ErrStr0, 1|4,-1,-1,-1);
        set_message(errormessage, TWST(Left_IsNot_Matrix), expression->name, ErrStr0, TWSF(Times_2));
        stack=NULL;
    }
    else if(!valueSt_matrix_getSize (lastVst, &rowB, &colB))
    {
        VstToStr(lastVst, ErrStr0, 1|4,-1,-1,-1);
        set_message(errormessage, TWST(Right_IsNot_Matrix), expression->name, ErrStr0, TWSF(Times_2));
        stack=NULL;
    }
    else if(rowA==3 && colA==1 && rowB==3 && colB==1)
    {   vector_cross_product (out, headVst, lastVst);
        vst_shift(stack, out);
    }
    else if(colA == rowB)
    {   matrix_multiply (out, headVst, lastVst, rowA, rowB, colB);
        vst_shift(stack, out);
    }
    else // else invalid matrix multiplication
    {   set_message(errormessage,
                    TWST(MatrixMult_IsInvalid),
                    expression->name,
                    TIS2(0,rowA), TIS2(1,colA),
                    TIS2(2,rowB), TIS2(3,colB));
        stack=NULL;
    }
    return stack!=NULL;
}



/* Here we evaluate something like A^T where A is a vector or matrix
*/
bool opr_transpose (ExprCallArg eca)
{
    int i, j, cols, rows;
    value *out, *stack = eca.stack;

    Expression* expression = eca.expression;
    eca.expression = expression->headChild;
    if(!expression_evaluate(eca)) return 0;

    // check if child node is of matrix value structure
    if(!valueSt_matrix_getSize (stack, &cols, &rows))
    {
        wchar* errormessage = eca.garg->message;
        VstToStr(stack, ErrStr0, 1|4,-1,-1,-1);
        set_message(errormessage, TWST(Operand_IsNot_Matrix), expression->name, ErrStr0);
        stack=NULL;
    }
    else
    {
        out = stack + VST_LEN(stack);
        valueSt_matrix_setSize (out, rows, cols);

        for(j=0; j<rows; j++)
        for(i=0; i<cols; i++)
            MVA(out,j,i,cols) = MVA(stack,i,j,rows);

        vst_shift(stack, out);
    }
    return stack!=NULL;
}

