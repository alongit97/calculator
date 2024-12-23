#include <stdio.h>
#include "calc.h"

int main()
{
    double result;
    status_t status;
    char *string = "4+5";

    status = Calc(string, &result);
    if (status == SUCCESS && result == 9.0)
    {
        printf("Test case 1 passed: %s = %.1f\n", string, result);
    }
    else
    {
        printf("Test case 1 failed: %.1f.  status is %d\n", result, status);
    }

    return 0;
}