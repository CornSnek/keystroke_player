#include "shared_string.h"
#include "parser.h"
#include <string.h>
#include <xdo.h>
#include <check.h>
START_TEST(parse_from_string){
    printf("Starting Test parse_from_string\n");
    char a_string_stack[]="(A;Ctrl+Alt+Delete=d;m1=u;.30;)A;";
    char* a_string_heap=(char*)malloc(sizeof(a_string_stack)/sizeof(char));
    strcpy(a_string_heap,a_string_stack);
    command_array_t* cmd_arr=command_array_new();
    repeat_id_manager_t* rim=repeat_id_manager_new();
    macro_buffer_t* mb=macro_buffer_new(a_string_heap,cmd_arr,rim);
    while(macro_buffer_process_next(mb,false)){
        if(mb->token_i>mb->size) break;
    }
    printf("%d\n",mb->parse_error);
    command_array_print(cmd_arr);
    repeat_id_manager_free(rim);
    command_array_free(cmd_arr);
    macro_buffer_free(mb);
}
END_TEST
START_TEST(repeat_id_search_str_test){
    printf("Starting Test repeat_id_search_str_test\n");
    repeat_id_manager_t* rim=repeat_id_manager_new();
    const int num_of_strings=3;
    const int string_size=6;
    const char* str0="AAA",* str1="BB",* str2="DDDD";
    char** string_heaps=(char**)malloc(sizeof(char*)*num_of_strings);
    for(int i=0;i<num_of_strings;i++) string_heaps[i]=(char*)calloc(string_size,sizeof(char));
    strcpy(string_heaps[0],str0);
    strcpy(string_heaps[1],str1);
    strcpy(string_heaps[2],str2);
    repeat_id_manager_add_name(rim,string_heaps[0],10);
    repeat_id_manager_add_name(rim,string_heaps[1],20);
    repeat_id_manager_add_name(rim,string_heaps[2],69);
    ck_assert_int_eq(repeat_id_manager_search_index(rim,string_heaps[2]),69);
    ck_assert_int_eq(repeat_id_manager_search_index(rim,"CCCCC"),-1);
    free(string_heaps);
    repeat_id_manager_free(rim);
}
END_TEST
START_TEST(shared_string_test){
    printf("Starting Test shared_string_test\n");
    shared_string_manager_t* ssm=SSManager_new();
    const char* str0="AAA",* str1="BBB",* str2="AAA";
    char** string_heaps=(char**)malloc(sizeof(char*)*3);
    for(int i=0;i<3;i++) string_heaps[i]=(char*)calloc(4,sizeof(char));
    strcpy(string_heaps[0],str0); strcpy(string_heaps[1],str1); strcpy(string_heaps[2],str2);
    SSManager_add_string(ssm,&string_heaps[0]); SSManager_add_string(ssm,&string_heaps[1]); SSManager_add_string(ssm,&string_heaps[2]);
    for(int i=0;i<3;i++) printf("%lx\n",(uintptr_t)string_heaps[i]);
    ck_assert_ptr_eq(string_heaps[0],string_heaps[2]);
    ck_assert_int_eq(SSManager_count_string(ssm,"AAA"),2);
    ck_assert_int_eq(SSManager_count_string(ssm,"BBB"),1);
    char* is_same_p=SSManager_add_string(ssm,&string_heaps[1]);
    ck_assert_ptr_eq(string_heaps[1],is_same_p);//Same string.
    SSManager_print_strings(ssm);
    ck_assert_int_eq(SSManager_count_string(ssm,"BBB"),2);
    ck_assert_int_eq(SSManager_count_string(ssm,"CCC"),0);
    SSManager_free_string(ssm,"BBB");
    ck_assert_int_eq(SSManager_count_string(ssm,"BBB"),1);
    SSManager_free_string(ssm,"BBB");
    ck_assert_int_eq(SSManager_count_string(ssm,"BBB"),0);
    SSManager_free_string(ssm,"AAA");
    ck_assert_int_eq(SSManager_count_string(ssm,"AAA"),1);
    SSManager_free_string(ssm,"AAA");
    ck_assert_int_eq(SSManager_count_string(ssm,"AAA"),0);
    ck_assert_int_eq(SSManager_count_string(ssm,"CCC"),0);
    free(string_heaps);
    SSManager_free(ssm);
    char a_string_stack[]="Replacing the word, word, with another word";
    char* a_string_heap=(char*)malloc(sizeof(a_string_stack)/sizeof(char));
    strcpy(a_string_heap,a_string_stack);
    printf("%s\n",a_string_heap);
    replace_str(&a_string_heap,"word","farts");
    printf("%s\n",a_string_heap);
    replace_str(&a_string_heap,"an","");
    printf("%s\n",a_string_heap);
    ck_assert_int_eq(strcmp("Replacing the farts, farts, with other farts",a_string_heap),0);
    free(a_string_heap);
}
END_TEST
Suite* test_suite(void){
    Suite* s;
    TCase* tc_core;
    s=suite_create("parser.h");
    tc_core=tcase_create("Core");
    tcase_add_test(tc_core,parse_from_string);
    tcase_add_test(tc_core,repeat_id_search_str_test);
    tcase_add_test(tc_core,shared_string_test);
    suite_add_tcase(s,tc_core);
    return s;
}
int main(void){
    Suite *s;
    SRunner *sr;
    s=test_suite();
    sr=srunner_create(s);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed=srunner_ntests_failed(sr);
    srunner_free(sr);
    return (!number_failed)?EXIT_SUCCESS:EXIT_FAILURE;
}
