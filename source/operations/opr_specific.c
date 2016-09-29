/*
    opr_specific.c

    This file is #included by operations.c only.
*/



#define VA(A,i) A[1+i]
static Value* vector_cross_product (const Value* A, const Value* B)
{
    Value n, m;
    Value* R = value_alloc(4);
    setSepto2(R[0],4);

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
    return R;
}



static Value* matrix_multiply (const Value* A, const Value* B, int rowA, int colArowB, int colB)
{
    int i, j, k;
    Value T, a, b;

    Value* R = value_alloc(1+rowA*(1+colB));
    valueSt_matrix_setSize (R, rowA, colB);

    for(i=0; i < rowA; i++)
    for(j=0; j < colB; j++)
    {
        setSmaInt(T,0);
        for(k=0; k < colArowB; k++)
        {
            a = MVA(A,i,k,colArowB);
            b = MVA(B,k,j,colB);
            // do: T = T + a*b;
            T = add(T, multiply(a, b));
        }
        MVA(R,i,j,colB) = T;
    }
    return R;
}



/*
    Here we evaluate something like A * B where:
    if A or B is a scalar, then 'scalar multiplication'
    if A and B are two vectors, then 'vector cross product'
    if A and B are two matrices, then 'matrix multiplication'
*/
Value* times_typeS (const Value* headVst, const Value* lastVst, const lchar* name, int info, Value CALL (Value, Value))
{
    if(!headVst || !lastVst) return NULL;
    Value y, n, m;
    Value *out, *output=NULL;
    long headSize = VST_LEN(headVst);
    long lastSize = VST_LEN(lastVst);
    int rowA, colA, rowB, colB;

    if(lastSize==1)
    {
        output = out = value_alloc(headSize);
        m = *lastVst;
        for( ; headSize--; headVst++)
        {
            n = *headVst;
            if(!isSeptor(n))
            { y = multiply (n, m); CHECK_Y } else y=n;
            *out++ = y;
        }
    }
    else if(headSize==1)
    {
        output = out = value_alloc(lastSize);
        n = *headVst;
        for( ; lastSize--; lastVst++)
        {
            m = *lastVst;
            if(!isSeptor(m))
            { y = multiply (n, m); CHECK_Y } else y=m;
            *out++ = y;
        }
    }
    else if(!valueSt_matrix_getSize (headVst, &rowA, &colA))
    {
        VstToStr(headVst, error_message+1000, 1|4,-1,-1,-1);
        set_error_message (TWST(Left_IsNot_Matrix), name, error_message+1000, TWSF(Times_2));
    }
    else if(!valueSt_matrix_getSize (lastVst, &rowB, &colB))
    {
        VstToStr(lastVst, error_message+1000, 1|4,-1,-1,-1);
        set_error_message (TWST(Right_IsNot_Matrix), name, error_message+1000, TWSF(Times_2));
    }
    else if((rowA==1 && colA==3 && rowB==1 && colB==3)
         || (rowA==3 && colA==1 && rowB==3 && colB==1))
        output = vector_cross_product (headVst, lastVst);

    else if(colA == rowB)
        output = matrix_multiply (headVst, lastVst, rowA, rowB, colB);

    else // else invalid matrix multiplication
        set_error_message (TWST(MatrixMult_IsInvalid), name,
                           TIS2(0,rowA), TIS2(1,colA),
                           TIS2(2,rowB), TIS2(3,colB));
    return output;
}
static Value* opr_times_typeS (Expression* expression, const Value* argument) { return OPER2 (expression, argument, times_typeS, 0, 0); }




/* Here we evaluate something like A^T where A is a vector or matrix
*/
Value* transpose (const Value* A, const lchar* name, int info, Value CALL (Value))
{
    int i, j, cols, rows;
    if(A==NULL) return NULL;

    // check if child node is of matrix value structure
    if(!valueSt_matrix_getSize (A, &cols, &rows))
    {
        VstToStr(A, error_message+1000, 1|4,-1,-1,-1);
        set_error_message (TWST(Operand_IsNot_Matrix), name, error_message+1000);
        return NULL;
    }

    i = (cols==1) ? 1 : (1+cols);
    Value* R = value_alloc (1+rows*i);
    valueSt_matrix_setSize (R, rows, cols);

    for(j=0; j<rows; j++)
      for(i=0; i<cols; i++)
        MVA(R,j,i,cols) = MVA(A,i,j,rows);
    return R;
}
Value* opr_transpose (Expression* expression, const Value* argument) { return OPER1 (expression, argument, transpose, 0, 0); }

