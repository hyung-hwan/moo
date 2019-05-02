#ifndef _MOO_T_T_H_
#define _MOO_T_T_H_

#include <stdio.h>

#define T_ASSERT_FAIL1(msg1) printf("FAILURE in %s[%d] - %s\n", __func__, (int)__LINE__, msg1)
#define T_ASSERT_FAIL2(msg1,msg2) printf("FAILURE in %s[%d] - %s - %s\n", __func__, (int)__LINE__, msg1, msg2)

#define T_ASSERT1(test,msg1) do { if (!(test)) { T_ASSERT_FAIL1(msg1); goto oops; } } while(0)
#define T_ASSERT2(test,msg1,msg2) do { if (!(test)) { T_ASSERT_FAIL2(msg1,msg2); goto oops; } } while(0)


#endif
