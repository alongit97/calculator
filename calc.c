
#include <stdlib.h>/*malloc*/
#include <stddef.h>/*size_t*/
#include <math.h>/*pow*/
#include <ctype.h>/*isdigit*/

#include "stack.h"/*stack_t*/
#include "calc.h"/*calc_t*/

#define ALL_ASCII_VALUES (128)
#define AVAILABLE_SATETS (2)
#define AVAILABLE_HANDLERS (8)

typedef enum state {
    WAIT_FOR_NUM = 0,
    WAIT_FOR_OP,
    FINAL
} state_t;

typedef struct calc {
    stack_t *ops;
    stack_t *nums;
    const char *input;
    state_t current_state;
    double *res;
} calc_t;

typedef double (*action_t)(double arg1, double arg2, double *new_data);
typedef int (*handler_func_t)(calc_t *calc);

typedef enum priority {
    priority0 = 0,
    priority1 = 1,
    priority2 = 2,
    priority3 = 3
} priority_t;

typedef struct operation {
    action_t action;
    priority_t priority;
} operation_t;

static calc_t *CalcInit(char* expression, double *res);
static int Calculate(calc_t *calc);

static int GetNumHandler(calc_t *calc);
static int OpenBracketHandler(calc_t *calc);
static int WhiteSpaceHandler(calc_t *calc);
static int ExecuteOperatorHandler(calc_t *calc);
static int CloseBracketHandler(calc_t *calc);
static int EndOfStringHandler(calc_t *calc);
static int ErrorHandler(calc_t *calc);

static double PlusAction(double arg1, double arg2, double *new_data);
static double MinusAction(double arg1, double arg2, double *new_data);
static double MultyplayAction(double arg1, double arg2, double *new_data);
static double DivideAction(double arg1, double arg2, double *new_data);
static double PowerAction(double arg1, double arg2, double *new_data);
static double OpenAction(double arg1, double arg2, double *new_data);

static operation_t operation_functions_lut[ALL_ASCII_VALUES] = {0};
static size_t operators_lut[ALL_ASCII_VALUES] = {0}; 
static handler_func_t handler_matrix[AVAILABLE_SATETS][AVAILABLE_HANDLERS] = {
    {GetNumHandler, GetNumHandler, ErrorHandler, OpenBracketHandler, WhiteSpaceHandler, ErrorHandler, ErrorHandler, ErrorHandler},
    {ErrorHandler, ExecuteOperatorHandler, ExecuteOperatorHandler, ErrorHandler, WhiteSpaceHandler, CloseBracketHandler,EndOfStringHandler, ErrorHandler}
};

operation_t plus_op = {PlusAction, priority1};
operation_t minus_op = {MinusAction, priority1};
operation_t multyplication_op = {MultyplayAction, priority2};
operation_t divide_op = {DivideAction, priority2};
operation_t power_op = {PowerAction, priority3};
operation_t open_op = {OpenAction, priority0};
operation_t close_op = {NULL, priority0};


status_t Calc(char* expression, double *res)
{
    int status = SUCCESS;

    calc_t *calc = CalcInit((char*)expression, res);
    if (calc == NULL)
    {
        return MEM_ERR;
    }

    while (status == SUCCESS && calc->current_state != FINAL)
    {
        status = (handler_matrix[calc->current_state]
                                    [operators_lut[(int)*calc->input]])(calc);
    }

    StackDestroy(calc->nums);
    StackDestroy(calc->ops);
    free(calc);

    return status;
}

/************************Handler Functions************************************/

static int GetNumHandler(calc_t *calc)
{
    double data = strtod(calc->input, (char**)&calc->input);

    StackPush(calc->nums, &data);
    calc->current_state = WAIT_FOR_OP;

    return SUCCESS;
}

static int OpenBracketHandler(calc_t *calc)
{
    StackPush(calc->ops, (void*)calc->input);
    calc->input++;
    calc->current_state = WAIT_FOR_NUM;
    return SUCCESS;
}

static int WhiteSpaceHandler(calc_t *calc)
{
    calc->input++;
    return SUCCESS;
}

static int ExecuteOperatorHandler(calc_t *calc)
{
    int status = SUCCESS;

    if(StackIsEmpty(calc->ops) || 
    operation_functions_lut[(int)*(calc->input)].priority 
    > operation_functions_lut[(int)*(char*)StackPeek(calc->ops)].priority 
    || (*(calc->input) == '^' && *(char*)StackPeek(calc->ops) == '^'))
    {
        StackPush(calc->ops, (void*)calc->input);
        calc->input++;
        calc->current_state = WAIT_FOR_NUM;
        return SUCCESS;
    }

    status = Calculate(calc);
    return status;
}

static int CloseBracketHandler(calc_t *calc)
{
    while (!StackIsEmpty(calc->ops) && *(char*)StackPeek(calc->ops) != '(')
    {
        Calculate(calc);
    }
    if (!StackIsEmpty(calc->ops))
    {
        StackPop(calc->ops);
        calc->current_state = WAIT_FOR_OP;
        calc->input++;
        return SUCCESS;
    }
    return SYNTAX_ERR;
}

static int EndOfStringHandler(calc_t *calc)
{
    int status;
    calc->current_state = FINAL;
    *calc->res = 0.0;
    
    while (StackSize(calc->nums) != 1)
    {
       status = Calculate(calc);
       if(status != SUCCESS)
       {
           return status; 
       }
    }

    if(StackIsEmpty(calc->ops))
    {
        *calc->res = *(double*)StackPeek(calc->nums);
        return SUCCESS;
    }

    return SYNTAX_ERR;
}


static int ErrorHandler(calc_t *calc)
{
    (void)calc;
    return SYNTAX_ERR;
}

/*********************ACtion Functions Of all operators***********************/
static double PlusAction(double arg1, double arg2, double *new_data)
{
    *new_data = (arg2 + arg1);
    return SUCCESS;
}

static double MinusAction(double arg1, double arg2, double *new_data)
{

    *new_data = (arg2 - arg1);
    return SUCCESS;
}

static double MultyplayAction(double arg1, double arg2, double *new_data)
{

    *new_data = (arg2 * arg1);
    return SUCCESS;
}

static double DivideAction(double arg1, double arg2, double *new_data)
{
    if (arg1 == 0)
    {
        return MATH_ERR;
    }
    *new_data = (arg2 / arg1);
    return SUCCESS;
}

static double PowerAction(double arg1, double arg2, double *new_data)
{
    *new_data = pow(arg2, arg1);
    return SUCCESS;
}

static double OpenAction (double arg1, double arg2, double *new_data)
{
    (void)arg1;
    (void)arg2;
    (void)new_data;

    return SYNTAX_ERR;
}


/*****************************************************************************/

static calc_t *CalcInit(char* expression, double *res)
{
    size_t i = 0;
    size_t capacity = 100;
    calc_t *calc = (calc_t*)malloc(sizeof(calc_t));
    if (NULL == calc)
    {
        return NULL;
    }

    calc->ops = StackCreate(capacity, sizeof(char));
    calc->nums = StackCreate(capacity, sizeof(double));
    calc->input = expression;
    calc->current_state = WAIT_FOR_NUM;
    calc->res = res;

    if (calc->ops == NULL || calc->nums == NULL)
    {
        if (calc->ops != NULL)
        {
            free(calc->ops);
        }

        if (calc->nums != NULL)
        {
            free(calc->nums);
        }
        free(calc);
        return NULL;
    }

    operation_functions_lut['+'] = plus_op;
    operation_functions_lut['-'] = minus_op;
    operation_functions_lut['*'] = multyplication_op;
    operation_functions_lut['/'] = divide_op;
    operation_functions_lut['^'] = power_op;
    operation_functions_lut['('] = open_op;
    operation_functions_lut[')'] = close_op;

    while (i++ < ALL_ASCII_VALUES)
    {
        operators_lut[i] = 7;
    }

    operators_lut['0'] = 0;
    operators_lut['1'] = 0;
    operators_lut['2'] = 0;
    operators_lut['3'] = 0;
    operators_lut['4'] = 0;
    operators_lut['5'] = 0;
    operators_lut['6'] = 0;
    operators_lut['7'] = 0;
    operators_lut['8'] = 0;
    operators_lut['9'] = 0;

    operators_lut['+'] = 1;
    operators_lut['-'] = 1;
    operators_lut['*'] = 2;
    operators_lut['/'] = 2;
    operators_lut['^'] = 2;
    operators_lut['('] = 3;
    operators_lut[' '] = 4;
    operators_lut[')'] = 5;
    operators_lut['\0'] = 6;

    return calc;
}

static int Calculate(calc_t *calc)
{
    double new_data;
    double arg1;
    double arg2;
    action_t func;
    int status = SUCCESS;

    arg1 = *(double*)StackPeek(calc->nums);
    StackPop(calc->nums);

    arg2 = *(double*)StackPeek(calc->nums);
    StackPop(calc->nums);

    func = operation_functions_lut[(int)*(char*)StackPeek(calc->ops)].action;
    status = func(arg1, arg2, &new_data);

    StackPush(calc->nums, &new_data);
    StackPop(calc->ops);

    return status;
}