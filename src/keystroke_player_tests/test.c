#include "shared_string.h"
#include "parser.h"
#include "test_utils.h"
#include <string.h>
#include <check.h>
#include <assert.h>
#include <sys/random.h>
ssize_t getrandom(void *buf, size_t buflen, unsigned int flags);
START_TEST(parse_from_string){
    puts("Starting Test parse_from_string");
    char a_string_stack[]="(A;Ctrl+Alt+Delete=d;m1=u;.30;)A;";
    char* a_string_heap=malloc(sizeof(a_string_stack)/sizeof(char));
    strcpy(a_string_heap,a_string_stack);
    command_array_t* cmd_arr=command_array_new();
    macro_buffer_t* mb=macro_buffer_new(a_string_heap,cmd_arr);
    while(macro_buffer_process_next(mb,false)){
        if(mb->token_i>mb->str_size) break;
    }
    printf("%d\n",mb->parse_error);
    command_array_print(cmd_arr,mb->vl);
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
    char** string_heaps=malloc(sizeof(char*)*num_of_strings);
    for(int i=0;i<num_of_strings;i++) string_heaps[i]=calloc(string_size,sizeof(char));
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
    char** string_heaps=malloc(sizeof(char*)*3);
    for(int i=0;i<3;i++) string_heaps[i]=calloc(4,sizeof(char));
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
    char* a_string_heap=malloc(sizeof(a_string_stack)/sizeof(char));
    char* a_string_heap2=malloc(sizeof(a_string_stack)/sizeof(char));
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
    EXIT_IF_NULL(df_str,char*);
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
        char* cmd_output=0;
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
START_TEST(macro_fail_test){
    macro_paster_t* mp=macro_paster_new();
    const char* bad_macro_def_strings[]={
        "[!!"//Mismatched macro brackets
        ,"[!![!A:=1!]\n[!B:=0!!]"//Mismatched macro definition brackets
        ,"[!![!A!]!!]"//No separator :=
        ,"[!![!A-:=3!]!!]"//Illegal character '-'
        ,"[!![!AB:b:c-:=3!]!!]"//Illegal character '-' in variable
        ,"[!![!AB:=3!]!!]"//Duplicate AB because AB was already defined from above.
    };
    for(size_t i=0;i<sizeof(bad_macro_def_strings)/sizeof(bad_macro_def_strings[0]);i++){
        ck_assert_int_eq(
            macro_paster_process_macros(mp,bad_macro_def_strings[i],MACROS_DEF_START_B,MACROS_DEF_END_B,MACRO_START_B,MACRO_END_B,MACRO_DEF_SEP,MACRO_VAR_SEP)
        ,0);
    }
    macro_paster_process_macros(mp,"[!![!RECURSION:=[!RECURSION!]!][!ABC:=what!]!!]",MACROS_DEF_START_B,MACROS_DEF_END_B,MACRO_START_B,MACRO_END_B,MACRO_DEF_SEP,MACRO_VAR_SEP);
    macro_paster_print(mp);
    const char* bad_macro_exp_strings[]={
        "[!ABC!]"//No macro definition end bracket
        ,"[!!!!][!ABC!]!]"//Mismatched brackets
        ,"[!!!!][!ABC!][!ABC-!][!B!]"//Illegal character
        ,"[!!!!][!ABC!][!ABC:3!][!B!]"//Out of range (ABC has no variables)
        ,"[!!!!][!ABC!][!ABC!][!RECURSION!]"//Recursive macro
    };
    for(size_t i=0;i<sizeof(bad_macro_exp_strings)/sizeof(bad_macro_exp_strings[0]);i++){
        char* cmd_output=0;
        ck_assert_int_eq(
            macro_paster_expand_macros(mp,bad_macro_exp_strings[i],MACROS_DEF_END_B,MACRO_START_B,MACRO_END_B,MACRO_VAR_SEP,&cmd_output)
        ,0);
        free(cmd_output);
    }
    macro_paster_free(mp);
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
    char* dummy_str_heap=malloc(sizeof(char)*(strlen(dummy_str)+1));//This will be changed.
    EXIT_IF_NULL(dummy_str_heap,char*);
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
#include "variable_loader.h"
#include "hash_map_impl2.h"
START_TEST(hash_map_test){
    const unsigned char StrMaxLen=10;
    const size_t RandArrayAmount=10;//Less if using valgrind. More without it.
    char** RandStrsArray=malloc(sizeof(char*)*(RandArrayAmount));
    EXIT_IF_NULL(RandStrsArray,char**);
    for(size_t i=0;i<RandArrayAmount;i++){
        while(true){
            unsigned char rand_b=0;
            char* temp;
            new_string:
            do{}while(getrandom(&rand_b,1,0)==-1);//Random here so that it doesn't repeat the same 1-char strings.
            temp=random_str(rand_b%StrMaxLen+1);
            for(size_t j=0;j<i;j++){
                if(!strcmp(temp,RandStrsArray[j])){//No duplicates or it would fail the tests below.
                    free(temp);
                    goto new_string;
                }
            }
            RandStrsArray[i]=temp;
            break;
        }
    }
    long* RandLongIntsArray=malloc(sizeof(long)*(RandArrayAmount));
    EXIT_IF_NULL(RandLongIntsArray,long*);
    for(size_t i=0;i<RandArrayAmount;i++){
        while(true){
            long temp;
            new_size_t:
            temp=random_size_t();
            for(size_t j=0;j<i;j++){
                if(temp==RandLongIntsArray[j]){
                    goto new_size_t;
                }
            }
            RandLongIntsArray[i]=temp;
            break;
        }
    }
    const size_t RepeatTests=10;
    for(size_t i=0;i<RepeatTests;i++){
        size_t* rand_indices[2]={random_unique_indices(RandArrayAmount),random_unique_indices(RandArrayAmount)};//2 for adding/removing all keys
        StringMap_SizeT_t* sm_st=StringMap_SizeT_new(RandArrayAmount);
        char** str=malloc(sizeof(char*)*(RandArrayAmount));
        EXIT_IF_NULL(str,char**);
        bool* key_exists=malloc(sizeof(bool)*(RandArrayAmount));
        EXIT_IF_NULL(key_exists,bool*);
        for(size_t j=0;j<RandArrayAmount;j++){
            char* const this_key=RandStrsArray[rand_indices[0][j]];
            str[j]=this_key; key_exists[j]=true;
            ck_assert_int_eq(StringMap_SizeT_assign(sm_st,this_key,j),VA_Written);//j's will be used to not read the respective key if deleted.
            if(sm_st->size<sm_st->MaxSize) ck_assert_int_eq(StringMap_SizeT_assign(sm_st,this_key,j),VA_Rewritten);
            else ck_assert_int_eq(StringMap_SizeT_assign(sm_st,this_key,j),VA_Full);
            ck_assert_int_eq(sm_st->size,j+1);
            for(size_t k=0;k<=j;k++){//To check each value that after inserting a string.
                StringMapValue_SizeT_t smv_st=StringMap_SizeT_read(sm_st,str[k]);
                ck_assert_int_eq(smv_st.exists,1);
                ck_assert_int_eq(smv_st.value,k);
            }
        }
        ck_assert_int_eq(StringMap_SizeT_resize(&sm_st,RandArrayAmount*2),1);
        for(size_t j=0;j<RandArrayAmount;j++){
            char* const this_key=RandStrsArray[rand_indices[1][j]];
            key_exists[StringMap_SizeT_read(sm_st,this_key).value]=false;//Don't read (value deleted)
            ck_assert_int_eq(StringMap_SizeT_erase(sm_st,this_key),1);
            ck_assert_int_eq(sm_st->size,RandArrayAmount-j-1);
            for(size_t k=0;k<RandArrayAmount;k++){
                if(key_exists[k]){
                    StringMapValue_SizeT_t smv_st=StringMap_SizeT_read(sm_st,str[k]);
                    ck_assert_int_eq(smv_st.exists,1);
                    ck_assert_int_eq(smv_st.value,k);
                }else ck_assert_int_eq(StringMap_SizeT_read(sm_st,str[k]).exists,0);
            }
        }
        StringMap_SizeT_free(sm_st);
        free(str);
        IntLongMap_SizeT_t* ilm_st=IntLongMap_SizeT_new(RandArrayAmount);
        long* long_arr=malloc(sizeof(long)*(RandArrayAmount));
        EXIT_IF_NULL(long_arr,long*);
        for(size_t j=0;j<RandArrayAmount;j++){
            const long this_key=RandLongIntsArray[rand_indices[0][j]];
            long_arr[j]=this_key; key_exists[j]=true;
            ck_assert_int_eq(IntLongMap_SizeT_assign(ilm_st,this_key,j),VA_Written);
            if(ilm_st->size<ilm_st->MaxSize) ck_assert_int_eq(IntLongMap_SizeT_assign(ilm_st,this_key,j),VA_Rewritten);
            else ck_assert_int_eq(IntLongMap_SizeT_assign(ilm_st,this_key,j),VA_Full);
            ck_assert_int_eq(ilm_st->size,j+1);
            for(size_t k=0;k<=j;k++){//To check each value that after inserting a long int key.
                IntLongMapValue_SizeT_t ilmv_st=IntLongMap_SizeT_read(ilm_st,RandLongIntsArray[rand_indices[0][k]]);
                ck_assert_int_eq(ilmv_st.exists,1);
                ck_assert_int_eq(ilmv_st.value,k);
            }
        }
        ck_assert_int_eq(IntLongMap_SizeT_resize(&ilm_st,RandArrayAmount*2),1);
        for(size_t j=0;j<RandArrayAmount;j++){
            const long this_key=RandLongIntsArray[rand_indices[1][j]];
            key_exists[IntLongMap_SizeT_read(ilm_st,this_key).value]=false;
            ck_assert_int_eq(IntLongMap_SizeT_erase(ilm_st,this_key),1);
            ck_assert_int_eq(ilm_st->size,RandArrayAmount-j-1);
            for(size_t k=0;k<RandArrayAmount;k++){
                if(key_exists[k]){
                    IntLongMapValue_SizeT_t ilmv_st=IntLongMap_SizeT_read(ilm_st,long_arr[k]);
                    ck_assert_int_eq(ilmv_st.exists,1);
                    ck_assert_int_eq(ilmv_st.value,k);
                }else ck_assert_int_eq(IntLongMap_SizeT_read(ilm_st,long_arr[k]).exists,0);
            }
        }
        IntLongMap_SizeT_free(ilm_st);
        free(long_arr);
        free(key_exists);
        free(rand_indices[0]);
        free(rand_indices[1]);
        printf("StringMap/IntLongMap - Finished tests #%lu/%lu\n",i+1,RepeatTests);
    }
    for(size_t i=0;i<RandArrayAmount;i++) free(RandStrsArray[i]);
    free(RandStrsArray);
    free(RandLongIntsArray);
}
END_TEST
START_TEST(variable_loader_test){
    VariableLoader_t* vl=VL_new(20);
    char* a_string="abcde",* string_no_exist="defg";
    vlcallback_info vlc_add[5]={
        VL_new_callback_add_as_l(vl,str_dup(a_string)),
        VL_new_callback_add_as_d(vl,str_dup(a_string)),
        VL_new_callback_rewrite_as_d(vl,str_dup(a_string)),
        VL_new_callback_rewrite_as_l(vl,str_dup(string_no_exist))
    };
    ck_assert_int_eq(ProcessVLCallback(vl,vlc_add[0],&(long){123}),1);
    ck_assert_int_eq(ProcessVLCallback(vl,vlc_add[1],&(double){200}),0);
    ck_assert_int_eq(ProcessVLCallback(vl,vlc_add[2],&(double){69}),1);
    ck_assert_int_eq(ProcessVLCallback(vl,vlc_add[3],&(long){70}),0);
    vlcallback_info vlc[4];
    vlc[0]=VL_new_callback_vdouble(vl,str_dup(a_string));
    vlc[1]=VL_new_callback_long(vl,80085);
    vlc[2]=VL_new_callback_vlong(vl,str_dup(string_no_exist));
    vlc[3]=VL_new_callback_double(vl,420.69);
    long lnum;
    double dnum;
    ck_assert_int_eq(ProcessVLCallback(vl,vlc[0],&dnum),1);
    printf("%.50lf 0x%016lx\n",dnum,(LD_u){.d=dnum}.l);
    ck_assert_int_eq(ProcessVLCallback(vl,vlc[1],&lnum),1);
    printf("%ld\n",lnum);
    ck_assert_int_eq(ProcessVLCallback(vl,vlc[2],&lnum),0);
    ck_assert_int_eq(ProcessVLCallback(vl,vlc[3],&dnum),1);
    printf("%.50lf 0x%016lx\n",dnum,(LD_u){.d=dnum}.l);
    SSManager_print_strings(vl->ssm);
    VL_free(vl);
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
    tcase_add_test(tc_core,macro_paster_test);
    tcase_add_test(tc_core,macro_fail_test);
    tcase_add_test(tc_core,replace_test);
    tcase_set_timeout(tc_core,1000.);
    tcase_add_test(tc_core,hash_map_test);
    tcase_add_test(tc_core,variable_loader_test);
    suite_add_tcase(s,tc_core);
    return s;
}
#include "rpn_evaluator.h"
int main(void){
    RPNEvaluatorInit();
    VariableLoader_t* vl=VL_new(20);
    RPNEvaluatorValidString("(ABC,DEF)",vl,RPN_EVAL_START_B,RPN_EVAL_END_B,RPN_EVAL_SEP);
    VL_free(vl);
    RPNEvaluatorFree();
    return 0;
    Suite *s;
    SRunner *sr;
    s=test_suite();
    sr=srunner_create(s);
    srunner_run_all(sr,CK_NORMAL);
    int number_failed=srunner_ntests_failed(sr);
    srunner_free(sr);
    return (!number_failed)?EXIT_SUCCESS:EXIT_FAILURE;
}
