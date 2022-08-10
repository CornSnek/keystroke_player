#include "shared_string.h"
#include "parser.h"
#include "test_utils.h"
#include <string.h>
#include <check.h>
#include <assert.h>
START_TEST(parse_from_string){
    puts("Starting Test parse_from_string");
    char a_string_stack[]="(A;Ctrl+Alt+Delete=d;m1=u;.30;)A;";
    char* a_string_heap=(char*)malloc(sizeof(a_string_stack)/sizeof(char));
    strcpy(a_string_heap,a_string_stack);
    command_array_t* cmd_arr=command_array_new();
    macro_buffer_t* mb=macro_buffer_new(a_string_heap,cmd_arr);
    while(macro_buffer_process_next(mb,false)){
        if(mb->token_i>mb->str_size) break;
    }
    printf("%d\n",mb->parse_error);
    command_array_print(cmd_arr);
    macro_buffer_free(mb);
    command_array_free(cmd_arr);
}
END_TEST
START_TEST(repeat_id_search_str_test){
    puts("Starting Test repeat_id_search_str_test");
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
    ck_assert_int_eq(repeat_id_manager_search_command_index(rim,string_heaps[2]),69);
    ck_assert_int_eq(repeat_id_manager_search_command_index(rim,"CCCCC"),-1);
    free(string_heaps);
    repeat_id_manager_free(rim);
}
END_TEST
START_TEST(shared_string_test){
    puts("Starting Test shared_string_test");
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
    char* a_string_heap2=(char*)malloc(sizeof(a_string_stack)/sizeof(char));
    strcpy(a_string_heap,a_string_stack);
    strcpy(a_string_heap2,a_string_stack);
    printf("%s\n",a_string_heap);
    replace_str(&a_string_heap,"word","farts");
    printf("%s\n",a_string_heap);
    replace_str(&a_string_heap,"an","");
    printf("%s\n",a_string_heap);
    ck_assert_int_eq(strcmp("Replacing the farts, farts, with other farts",a_string_heap),0);
    const char* begin_w=strchr(a_string_heap2,'w'),* end_w=strchr(a_string_heap2,'d');
    replace_str_at(&a_string_heap2,"word","first mention of 'word' only",begin_w,end_w);
    printf("%s\n",a_string_heap2);
    ck_assert_int_eq(strcmp("Replacing the first mention of 'word' only, word, with another word",a_string_heap2),0);
    free(a_string_heap);
    free(a_string_heap2);
}
END_TEST
START_TEST(innermost_test){
    char str[]="[![!M_NAME:var1:var2:var3]][![![!M_NAME2]]]";
    printf("%s\n",str);
    const char* s_p,* e_p;
    int extra_p=first_innermost_bracket(str,"[!","]",&s_p,&e_p);
    printf("%sErrors\n",!extra_p?"No ":"");
    if(!extra_p){
        printf("%lx %lx %lx\n",(size_t)str,(size_t)s_p,(size_t)e_p);
        printf("%30s\n%30s\n%30s\n",str,s_p,e_p);
    }
    #define DYCK_WORD_LEN 5
    bool dyck_word[DYCK_WORD_LEN*2]={0};
    for(size_t i=DYCK_WORD_LEN;i<DYCK_WORD_LEN*2;i++) dyck_word[i]=true;
    size_t cat=catalan(DYCK_WORD_LEN);
    #define START_B "<[!"
    #define END_B "]>"
    const size_t str_len_s_b=strlen(START_B),str_len_e_b=strlen(END_B);
    const size_t str_len_total=(str_len_s_b+str_len_e_b)*DYCK_WORD_LEN;
    char* bracket_str=malloc(sizeof(char)*(str_len_total)+1);
    bracket_str[str_len_total]='\0';//Null terminate.
    for(size_t c=0;c<cat;c++){
        next_dyck_word(dyck_word,DYCK_WORD_LEN*2);
        size_t str_i=0;
        for(size_t i=0;i<DYCK_WORD_LEN*2;i++){//0 is start bracket, 1 is end bracket.
            if(dyck_word[i]){
                strncpy(bracket_str+str_i,END_B,str_len_e_b);
                str_i+=str_len_e_b;
            }else{
                strncpy(bracket_str+str_i,START_B,str_len_s_b);
                str_i+=str_len_s_b;
            }
        }
        const char* begin_p,* end_p;
        ck_assert_int_eq(first_innermost_bracket(bracket_str,START_B,END_B,&begin_p,&end_p),0);
        ck_assert_int_eq(first_outermost_bracket(bracket_str,START_B,END_B,&begin_p,&end_p),0);
        //printf("%s\n",bracket_str);
    }
    free(bracket_str);
}
END_TEST
#define MACRO_FILE_F "example_scripts/macro_test.kps"
/**nodiscard - Function gives pointer ownership.*/
char* read_macro_file(void){
    FILE* f_obj;
    char* df_str;
    f_obj=fopen(MACRO_FILE_F,"r");
    if(!f_obj) return 0;
    fseek(f_obj,0,SEEK_END);
    size_t str_len=ftell(f_obj);
    rewind(f_obj);
    df_str=malloc(sizeof(char)*(str_len+1));//To include '\0'
    EXIT_IF_NULL(df_str,char*)
    fread(df_str,str_len,1,f_obj);
    df_str[str_len]='\0';
    fclose(f_obj);
    return df_str;
}
START_TEST(macro_paster_test){
    macro_paster_t* mp=macro_paster_new();
    ck_assert_int_eq(macro_paster_add_name(mp,"AAA"),1);
    ck_assert_int_eq(macro_paster_add_name(mp,"AAA"),0);
    ck_assert_int_eq(macro_paster_add_name(mp,"BBB"),1);
    ck_assert_int_eq(macro_paster_add_var(mp,"CCC","def"),0);
    ck_assert_int_eq(macro_paster_add_var(mp,"BBB","def"),1);
    ck_assert_int_eq(macro_paster_add_var(mp,"BBB","def"),0);
    ck_assert_int_eq(macro_paster_add_name(mp,"CCC"),1);
    ck_assert_int_eq(macro_paster_add_var(mp,"BBB","ghi"),1);
    ck_assert_int_eq(macro_paster_add_var(mp,"CCC","ghi"),1);
    ck_assert_int_eq(macro_paster_write_var_by_str(mp,"DDD","aaa","ddd"),0);
    ck_assert_int_eq(macro_paster_write_var_by_str(mp,"CCC","ghj","klmnop"),0);
    ck_assert_int_eq(macro_paster_write_var_by_str(mp,"BBB","def","klmnop"),1);
    ck_assert_int_eq(macro_paster_write_var_by_str(mp,"BBB","ghi","nopqrst"),1);
    ck_assert_int_eq(macro_paster_write_var_by_str(mp,"CCC","ghi","write"),1);
    ck_assert_int_eq(macro_paster_write_var_by_str(mp,"CCC","ghi","rewrite"),1);
    ck_assert_int_eq(macro_paster_add_var(mp,"AAA","var_1"),1);
    ck_assert_int_eq(macro_paster_write_var_by_ind(mp,"DDD",0,"value_1"),0);
    ck_assert_int_eq(macro_paster_write_var_by_ind(mp,"AAA",-1,"value_1"),0);
    ck_assert_int_eq(macro_paster_write_var_by_ind(mp,"AAA",1,"value_1"),0);
    ck_assert_int_eq(macro_paster_write_var_by_ind(mp,"AAA",0,"value_1"),1);
    ck_assert_int_eq(macro_paster_write_var_by_ind(mp,"AAA",0,"value_2"),1);
    ck_assert_int_eq(macro_paster_write_macro_def(mp,"BBC",":def+:ghi;(:def*:ghi)"),0);
    ck_assert_int_eq(macro_paster_write_macro_def(mp,"BBB",":def+:ghi;(:def*:ghi)"),1);
    macro_paster_print(mp);
    char* str=0;
    macro_paster_get_val_string(mp,"BBB",':',&str);
    printf("%s\n",str);
    free(str);
    macro_paster_free(mp);
    macro_paster_t* mp2=macro_paster_new();
    char* file_str=read_macro_file();
    ck_assert_ptr_ne(file_str,0);
    trim_comments(&file_str);//So that the program doesn't process commented macros.
    MacroProcessStatus mps=file_contains_macro_definitions(file_str,MACROS_DEF_START_B,MACROS_DEF_END_B);
    if(mps==MPS_HasDefinitions){
        macro_paster_process_macros(mp2,file_str,MACROS_DEF_START_B,MACROS_DEF_END_B,MACRO_START_B,MACRO_END_B,MACRO_DEF_SEP,MACRO_VAR_SEP);
        char* cmd_output;
        macro_paster_expand_macros(mp2,file_str,MACROS_DEF_END_B,MACRO_START_B,MACRO_END_B,MACRO_VAR_SEP,&cmd_output);
        free(cmd_output);
    }
    macro_paster_free(mp2);
    printf("%s\n",file_str);
    char* highlight_str=print_string_highlight(file_str,file_str,file_str,"\x1B[47;30;1m","\x1B[0m");
    char* srv=string_read_view(highlight_str,highlight_str+11,5);
    printf("string_read_view:\n%s\n",srv);
    free(srv);
    free(highlight_str);
    free(file_str);
}
END_TEST
START_TEST(replace_test){
    replace_node_t WordList[]={
        {"ab","Word1"}
        ,{"acb","Word2"}
        ,{"abc","Word3"}
        ,{"abcd","Word4"}
        ,{"acbde","Word5"}
        ,{"abcdef","Word6"}
        ,{"babcde","Word7"}
    };
    const size_t WordList_len=sizeof(WordList)/sizeof(replace_node_t);
    const char* dummy_str="abcdef abcde ab acb abc abf abcd acbde abcdef babcde";
    char* dummy_str_heap=(char*)malloc(sizeof(char)*(strlen(dummy_str)+1));//This will be changed.
    EXIT_IF_NULL(dummy_str_heap,char*)
    strcpy(dummy_str_heap,dummy_str);
    printf("%s\n",dummy_str_heap);
    replace_str_list(&dummy_str_heap,WordList,WordList_len);
    printf("%s\n",dummy_str_heap);
    free(dummy_str_heap);
    const char* words="abcdefghijkl";
    const char* words_split;
    int words_s_l,words_e_l;
    split_at_sep(words,"efg",&words_split,&words_s_l,&words_e_l);
    printf("%s %s,%d %d\n",words,words_split,words_s_l,words_e_l);
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
    tcase_add_test(tc_core,innermost_test);
    tcase_set_timeout(tc_core,60.);
    tcase_add_test(tc_core,macro_paster_test);
    tcase_add_test(tc_core,replace_test);
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
