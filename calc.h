#ifndef __CALC_H__
#define __CALC_H__

typedef enum{
    SUCCESS = 0,
    SYNTAX_ERR,
    MATH_ERR,
    MEM_ERR
}status_t;

status_t Calc(char* expression, double *res);

#endif /*__CALC_H__*/