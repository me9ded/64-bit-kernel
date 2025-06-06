#include "print.h"
#include <stdlib.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

// the test plan is basically following the rule of thumb for testing making sure we have at least 6 test cases for each function 
// as they have only 2 arguments.


void test_print_char(){
    CU_ASSERT(print_char("\n"));
    CU_ASSERT(print_char(4));
    CU_ASSERT(print_char(0));
    CU_ASSERT(print_char(-2));
    CU_ASSERT(print_char("amine"));
    CU_ASSERT(print_char(" "));
    CU_ASSERT(print_char("a"));

}

void test_print_str(){
    CU_ASSERT(print_str(1));
    CU_ASSERT(print_str(4));
    CU_ASSERT(print_str(0));
    CU_ASSERT(print_str(-2));
    CU_ASSERT(print_str("welcome"));
    CU_ASSERT(print_str(" "));
    CU_ASSERT(print_str("\n"));


}

void test_print_set_color(){
    CU_ASSERT(print_set_color(1,0));
    CU_ASSERT(print_set_color(4,5));
    CU_ASSERT(print_set_color(-1,10));
    CU_ASSERT(print_set_color(200,100));
    CU_ASSERT(print_set_color(5,6));
}

int main(){
    CU_initialize_registry();
    CU_pSuite test_suite = CU_add_suite("testing of the printing functions of the kernel",NULL,NULL);
    CU_add_test(test_suite, "testing of the printing of char function",test_print_char);
    CU_add_test(test_suite, "testing of the string printing function",test_print_str);
    CU_add_test(test_suite, "testing of the color setting function",test_print_set_color);
    CU_run_all_tests();
    CU_cleanup_registry();
    return EXIT_SUCCESS;
}
