#include "parser.h"
#include "macros.h"
#include "variable_loader.h"
#include "rpn_evaluator.h"
#include <string.h>
#include <ctype.h>
__ReadStateWithStringDef(__ReadStateEnums)
const char* DebugConfigTypeString[3]={
    "debug_print_type",
    "rpn_decimals",
    "rpn_stack_debug"
};
const int IndexNotFound=-1;
const int JumpFromNotConnected=-2;
macro_buffer_t* macro_buffer_new(char* str_owned, command_array_t* cmd_arr){
    macro_buffer_t* this=(malloc(sizeof(macro_buffer_t)));
    EXIT_IF_NULL(this,macro_buffer_t);
    *this=(macro_buffer_t){.token_i=0,.str_size=strlen(str_owned),.contents=str_owned,.cmd_arr=cmd_arr,.rim=repeat_id_manager_new(),.jim=jump_id_manager_new(),.vl=VL_new(400),.vpa=vp_array_new(),.parse_error=false};
    return this;
}
void print_where_error_is(const char* contents,int begin_error,int end_error){
    char* str_to_print=malloc(sizeof(char)*(end_error+2)); //+2 to count a character and for '\0'.
    EXIT_IF_NULL(str_to_print,char*);
    strncpy(str_to_print,contents+begin_error,end_error+1);
    str_to_print[end_error+1]='\0';
    printf("%s < Command where error occured.\n",str_to_print);
    free(str_to_print);
}
int error_move_offset(const char* begin_error_p){//Moves the pointer to the end to help process multiple errors.
    const char* next_p=begin_error_p;
    do{
        if(*next_p==';'||*next_p=='\0') return next_p-begin_error_p;//strchr for ; makes segmentation faults at end of commands.
        next_p++;
    }while(true);
}
bool macro_buffer_process_next(macro_buffer_t* this,bool print_debug,bool rpn_debug){//Returns bool if processed successfully or not.
    ReadState read_state=RS_Start;
    bool key_processed=false;
    InputState input_state=IS_Down;
    char* str_name=0,* num_str_arr[4]={0},* rpn_str_arr[4]={0},* parse_start_p=this->contents+this->token_i;
    int print_str_len=0;
    char** print_rpn_strs;
    int print_rpn_strs_len=0;
    char esc_seq;
    vlcallback_info vlci[4];
    const char* begin_p,* end_p;
    long delay_mult=0;
    long parsed_num=0;
    int parsed_num_i=0;
    bool added_keystate=false;
    int read_i=0; //Index to read.
    int read_offset_i=0; //Last character to read by offset of read_i.
    int query_jump_e=1,query_jump_ne=2; //Default jump for queries.
    bool first_number_parsed=false, print_cmd=false, store_index=false, invert_b=false, held_down=true, print_newline, is_absolute;
    CompareCoords cmp_flags=CMP_NULL;
    VLCallbackType vct;
    DebugConfigType dct;
    QueryCombType query_comb_type=QCT_NONE;
    #define DO_ERROR()\
    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);\
    this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);\
    this->parse_error=true;\
    key_processed=true; (void)0
    do{
        size_t line_num,char_num;
        const char current_char=this->contents[this->token_i+read_i+read_offset_i];
        const char* current_char_p=this->contents+this->token_i+read_i+read_offset_i;
        get_line_column_positions_p1(this->contents,current_char_p,&line_num,&char_num);
        if(print_debug) printf("'%c' line: %lu char: %lu token_i:%d read_offset_i:%d read_i:%d State:%s\n",current_char,line_num,char_num,this->token_i,read_offset_i,read_i,ReadStateStrings[read_state]);
        switch(read_state){
            case RS_Start:
                if(!strncmp(current_char_p,"PRINT>>",7)){
                    print_cmd=true;
                    read_i+=7;
                    read_offset_i=-1;
                    break;
                }
                if(!strncmp(current_char_p,"init,",5)){
                    read_i+=5;
                    read_offset_i=-1;
                    read_state=RS_InitVarType;
                    break;
                }
                if(!strncmp(current_char_p,"edit,",5)){
                    read_i+=5;
                    read_offset_i=-1;
                    read_state=RS_EditVarName;
                    break;
                }
                if(!strncmp(current_char_p,"exit;",5)){
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_Exit,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=4; //Exclude reading "exit;" The +1 for ';' is from read_offset_i++
                    key_processed=true;
                    break;
                }
                if(!strncmp(current_char_p,"pass;",5)){//Just like exit, but does nothing.
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_Pass,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=4;
                    key_processed=true;
                    break;
                }
                if(!strncmp(current_char_p,"ungrab_keys;",12)){
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_UngrabKeyAll,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=11;
                    key_processed=true;
                    break;
                }
                if(!strncmp(current_char_p,"ungrab_buttons;",15)){
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_UngrabButtonAll,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=15;
                    key_processed=true;
                    break;
                }
                if(!strncmp(current_char_p,"save_mma;",9)){
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_SaveMouseCoords,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=8;
                    key_processed=true;
                    break;
                }
                if(!strncmp(current_char_p,"load_mma;",9)){
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_LoadMouseCoords,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=8;
                    key_processed=true;
                    break;
                }
                if(!strncmp(current_char_p,"rep_reset;",10)){
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_RepeatResetCounters,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=9;
                    key_processed=true;
                    break;
                }
                if(!strncmp(current_char_p,"JTIA>",5)){
                    is_absolute=true;
                    read_i+=5;
                    read_offset_i=-1;
                    read_state=RS_JumpToIndex;
                    break;
                }
                if(!strncmp(current_char_p,"JTIR>",5)){
                    is_absolute=false;
                    read_i+=5;
                    read_offset_i=-1;
                    read_state=RS_JumpToIndex;
                    break;
                }
                if(!strncmp(current_char_p,"JT>",3)){
                    read_i+=3;
                    read_offset_i=-1;
                    read_state=RS_JumpTo;
                    break;
                }
                if(!strncmp(current_char_p,"JTS>",4)){
                    read_i+=4;
                    read_offset_i=-1;
                    store_index=true;
                    read_state=RS_JumpTo;
                    break;
                }
                if(!strncmp(current_char_p,"JF>",3)){
                    read_i+=3;
                    read_offset_i=-1;
                    read_state=RS_JumpFrom;
                    break;
                }
                if(!strncmp(current_char_p,"JB>;",4)){
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_JumpBack,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=3;
                    key_processed=true;
                    break;
                }
                if(!strncmp(current_char_p,"mma=",4)){
                    read_i+=4;
                    read_offset_i=-1;
                    is_absolute=true;
                    read_state=RS_MoveMouse;
                    break;
                }
                if(!strncmp(current_char_p,"mmr=",4)){
                    read_i+=4;
                    read_offset_i=-1;
                    is_absolute=false;
                    read_state=RS_MoveMouse;
                    break;
                }
                if(!strncmp(current_char_p,"wait_key=",9)){
                    read_i+=9;
                    read_offset_i=-1;
                    read_state=RS_WaitUntilKey;
                    break;
                }
                if(!strncmp(current_char_p,"!wait_key=",10)){
                    read_i+=10;
                    read_offset_i=-1;
                    invert_b=true;
                    read_state=RS_WaitUntilKey;
                    break;
                }
                if(!strncmp(current_char_p,"wait_button=",12)){
                    read_i+=12;
                    read_offset_i=-1;
                    held_down=true;
                    read_state=RS_WaitUntilButton;
                    break;
                }
                if(!strncmp(current_char_p,"!wait_button=",13)){
                    read_i+=13;
                    read_offset_i=-1;
                    held_down=true;
                    invert_b=true;
                    read_state=RS_WaitUntilButton;
                    break;
                }
                if(!strncmp(current_char_p,"wait_buttonc=",13)){
                    read_i+=13;
                    read_offset_i=-1;
                    held_down=false;
                    read_state=RS_WaitUntilButton;
                    break;
                }
                if(!strncmp(current_char_p,"grab_key=",9)){
                    read_i+=9;
                    read_offset_i=-1;
                    read_state=RS_GrabKey;
                    break;
                }
                if(!strncmp(current_char_p,"ungrab_key=",11)){
                    read_i+=11;
                    read_offset_i=-1;
                    read_state=RS_UngrabKey;
                    break;
                }
                if(!strncmp(current_char_p,"grab_button=",12)){
                    read_i+=12;
                    read_offset_i=-1;
                    read_state=RS_GrabButton;
                    break;
                }
                if(!strncmp(current_char_p,"ungrab_button=",14)){
                    read_i+=14;
                    read_offset_i=-1;
                    read_state=RS_UngrabButton;
                    break;
                }
                if(!strncmp(current_char_p,"print=",6)){
                    read_i+=6;
                    read_offset_i=-1;
                    read_state=RS_PrintString;
                    print_newline=false;
                    str_name=malloc(0);
                    print_rpn_strs=malloc(0);
                    EXIT_IF_NULL(str_name,char*);
                    EXIT_IF_NULL(print_rpn_strs,char**);
                    break;
                }
                if(!strncmp(current_char_p,"println=",8)){
                    read_i+=8;
                    read_offset_i=-1;
                    read_state=RS_PrintString;
                    print_newline=true;
                    str_name=malloc(0);
                    print_rpn_strs=malloc(0);
                    EXIT_IF_NULL(str_name,char*);
                    EXIT_IF_NULL(print_rpn_strs,char**);
                    break;
                }
                if(!strncmp(current_char_p,"debug,",6)){
                    read_i+=6;
                    read_offset_i=-1;
                    read_state=RS_DebugType;
                    break;
                }
                if(char_is_key(current_char)){
                    if(current_char=='m'&&isdigit(this->contents[this->token_i+read_i+read_offset_i+1])){
                        read_i++;
                        read_offset_i=-1;
                        read_state=RS_MouseClickType;
                        break;
                    }
                    char* escape_p,* semic_p;
                    if((escape_p=strstr(current_char_p,"Escape"))
                        &&(semic_p=strchr(current_char_p,';'))&&escape_p<semic_p){
                        fprintf(stderr,ERR("Key 'Escape' cannot be used, because it is used to quit the macro.\n"));
                        DO_ERROR();
                        break;
                    }
                    read_state=RS_Keys;
                    break;
                }
                switch(current_char){
                    case '.':
                        read_i++;
                        read_offset_i=-1;
                        read_state=RS_Delay;
                        break;
                    case '(':
                        read_i++;
                        read_offset_i=-1;
                        read_state=RS_RepeatStart;
                        break;
                    case ')':
                        read_i++;
                        read_offset_i=-1;
                        read_state=RS_RepeatEnd;
                        break;
                    case ';'://Fallthough
                    case '\0':
                        this->token_i+=read_i+read_offset_i+1;//Don't add string slice to null characters or semi-colon.
                        return !this->parse_error;
                        break;
                    case '\n':
                        parse_start_p++;
                        read_i++;
                        read_offset_i=-1;
                        line_num++;
                        char_num=0;//1 after loop repeats.
                        break;
                    case '#':
                        parse_start_p++;
                        read_state=RS_Comments;
                        break;
                    case ' '://Fallthrough
                    case '\t'://Allow tabs and spaces before making comments.
                        parse_start_p++;
                        read_i++;
                        read_offset_i=-1;
                        break;
                    case '?':
                        read_i++;
                        read_offset_i=-1;
                        read_state=RS_Query;
                        break;
                    case '|':
                        read_i++;
                        read_offset_i=-1;
                        query_comb_type=QCT_OR;
                        read_state=RS_Query;
                        break;
                    case '&':
                        read_i++;
                        read_offset_i=-1;
                        query_comb_type=QCT_AND;
                        read_state=RS_Query;
                        break;
                    default:
                        fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                }
                break;
            case RS_Comments:
                parse_start_p++;
                if(current_char=='\n'){
                    read_i+=read_offset_i+1;
                    read_offset_i=-1;
                    line_num++;
                    char_num=0;//1 after loop repeats.
                    read_state=RS_Start;
                }else if(current_char=='\0'){//Don't add string slice to null characters.
                    this->token_i+=read_i+read_offset_i+1;
                    return !this->parse_error;
                }
                break; //No need for errors here.
            case RS_RepeatStart:
                if(char_is_key(current_char)) break;
                if(current_char==';'){
                    str_name=calloc(read_offset_i+1,sizeof(char));
                    EXIT_IF_NULL(str_name,char);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    repeat_id_manager_add_name(this->rim,str_name,command_array_count(this->cmd_arr));
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_RepeatStart,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.repeat_start=(repeat_start_t){
                                .counter=0,
                                .str_index=repeat_id_manager_search_string_index(this->rim,str_name)
                            }
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_RepeatEnd:
                if(char_is_key(current_char)) break;
                if(current_char=='='){
                    str_name=calloc(read_offset_i+1,sizeof(char));
                    EXIT_IF_NULL(str_name,char);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    if(!SSManager_add_string(this->rim->ssm,&str_name)){//String exists.
                        read_i+=read_offset_i+1;
                        read_offset_i=-1;
                        read_state=RS_RepeatEndValue;
                        break;
                    }
                    fprintf(stderr,ERR("String '%s' was not initially defined from a Loop Start at line %lu char %lu.\n"),str_name,line_num,char_num);
                    DO_ERROR();
                    break;
                }
                if(current_char==';'){
                    str_name=calloc(read_offset_i+1,sizeof(char));
                    EXIT_IF_NULL(str_name,char);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    if(!SSManager_add_string(this->rim->ssm,&str_name)){//String exists.
                        command_array_add(this->cmd_arr,
                            (command_t){.type=CMD_RepeatEnd,.subtype=CMDST_Command,.print_cmd=print_cmd,
                                .cmd_u.repeat_end=(repeat_end_t){
                                    .cmd_index=repeat_id_manager_search_command_index(this->rim,str_name),
                                    .str_index=repeat_id_manager_search_string_index(this->rim,str_name),
                                    .counter=VL_new_callback_int(this->vl,0)
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }
                    fprintf(stderr,ERR("String '%s' was not initially defined from a Loop Start at line %lu char %lu.\n"),str_name,line_num,char_num);
                   DO_ERROR();
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_RepeatEndValue:
                if(isdigit(current_char)){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=';'&&isdigit(*end_p)&&*end_p){}
                    if(*end_p!=';'){
                        fprintf(stderr,ERR("Semicolon not found or non-number found at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    num_str_arr[0]=char_string_slice(begin_p,end_p-1);//-1 to exclude ;
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_RepeatEnd,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.repeat_end=(repeat_end_t){
                                .cmd_index=repeat_id_manager_search_command_index(this->rim,str_name),
                                .str_index=repeat_id_manager_search_string_index(this->rim,str_name),
                                .counter=VL_new_callback_int(this->vl,strtol(num_str_arr[0],NULL,10))
                            }
                        }
                    );
                    free(num_str_arr[0]);
                    read_offset_i+=end_p-begin_p;
                    key_processed=true;
                    break;
                }
                if(current_char=='('){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=')'&&*end_p!=';'&&*end_p){}
                    if(*end_p!=')'||*(end_p+1)!=';'){
                        fprintf(stderr,ERR("RPN string doesn't terminate with ');' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;   
                    }
                    rpn_str_arr[0]=char_string_slice(begin_p,end_p);
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_RepeatEnd,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.repeat_end=(repeat_end_t){
                                .cmd_index=repeat_id_manager_search_command_index(this->rim,str_name),
                                .str_index=repeat_id_manager_search_string_index(this->rim,str_name),
                                .counter=VL_new_callback_number_rpn(this->vl,rpn_str_arr[0],rpn_debug)
                            }
                        }
                    );
                    read_offset_i+=end_p-begin_p+1;//+1 to read past semicolon.
                    key_processed=true;
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_Keys:
                if(char_is_x11_key(current_char)) break;
                else if(current_char=='='){
                    read_state=RS_KeyState;
                    added_keystate=false;
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_KeyState:
                if(char_is_keystate(current_char)){
                    if(added_keystate){
                        fprintf(stderr,ERR("Cannot add more than 1 keystate at line %lu char %lu.\n"),line_num,char_num);
                        DO_ERROR();
                        break;
                    }
                    switch(current_char){
                        case 'D': case 'd': input_state=IS_Down; break;
                        case 'U': case 'u': input_state=IS_Up; break;
                        default: input_state=IS_Click;
                    }
                    added_keystate=true;
                    break;
                }else if(added_keystate&&current_char==';'){
                    str_name=malloc(sizeof(char)*(read_offset_i-1));//-2 to exclude RS_KeyState modifiers, but -1 because null terminator.
                    EXIT_IF_NULL(str_name,char*);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i-2);
                    str_name[read_offset_i-2]='\0';
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_KeyStroke,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.auto_ks=(auto_keystroke_t){
                                .key=str_name,//SSManager/auto_keystroke_t owns char* key via command_array_add.
                                .key_state=input_state
                            }
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_Delay:
                if(char_is_delay(current_char)){
                    switch(current_char){
                        case 's': case 'S': delay_mult=1000000; break;
                        case 'm': case 'M': delay_mult=1000; break;
                        case 'u': case 'U': delay_mult=1;
                    }
                    read_i++;
                    read_offset_i=-1;
                    read_state=RS_DelayValue;
                    break;
                }else if(isdigit(current_char)){
                    delay_mult=1;//Default microseconds.
                    read_state=RS_DelayValue;
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_DelayValue:
                if(isdigit(current_char)){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=';'&&isdigit(*end_p)&&*end_p){}
                    if(*end_p!=';'){
                        fprintf(stderr,ERR("Semicolon not found or non-number found at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    num_str_arr[0]=char_string_slice(begin_p,end_p-1);
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_Delay,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.delay=(delay_t){
                                .callback=VL_new_callback_long(this->vl,strtol(num_str_arr[0],NULL,10)),
                                .delay_mult=delay_mult
                            }
                        }
                    );
                    free(num_str_arr[0]);
                    read_offset_i+=end_p-begin_p;
                    key_processed=true;
                    break;
                }
                if(current_char=='('){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=')'&&*end_p!=';'&&*end_p){}
                    if(*end_p!=')'||*(end_p+1)!=';'){
                        fprintf(stderr,ERR("RPN string doesn't terminate with ');' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;   
                    }
                    rpn_str_arr[0]=char_string_slice(begin_p,end_p);
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_Delay,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.delay=(delay_t){
                                .callback=VL_new_callback_number_rpn(this->vl,rpn_str_arr[0],rpn_debug),
                                .delay_mult=delay_mult
                            }
                        }
                    );
                    read_offset_i+=end_p-begin_p+1;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_MouseClickType:
                if(isdigit(current_char)&&!first_number_parsed){
                    num_str_arr[0]=calloc(sizeof(char),2);
                    EXIT_IF_NULL(num_str_arr[0],char*);
                    num_str_arr[0][0]=current_char;
                    parsed_num=strtol(num_str_arr[0],NULL,10);
                    free(num_str_arr[0]);
                    first_number_parsed=true;
                    break;
                }else if(current_char=='='&&first_number_parsed){
                    read_i+=2;//To read numbers.
                    read_offset_i=-1;
                    read_state=RS_MouseClickState;
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_MouseClickState:
                if(char_is_keystate(current_char)){
                    if(added_keystate){
                        fprintf(stderr,ERR("Cannot add more than 1 keystate at line %lu char %lu.\n"),line_num,char_num);
                        DO_ERROR();
                        break;
                    }
                    switch(current_char){
                        case 'D': case 'd': input_state=IS_Down; break;
                        case 'U': case 'u': input_state=IS_Up; break;
                        default: input_state=IS_Click;
                    }
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_MouseClick,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.mouse_click=(mouse_click_t){.mouse_state=input_state,
                                .mouse_type=parsed_num
                            }
                        }
                    );
                    added_keystate=true;
                    break;
                }else if(added_keystate&&current_char==';'){
                    key_processed=true;
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_MoveMouse:
                if(isdigit(current_char)||current_char=='-'){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=(first_number_parsed?';':',')&&isdigit(*end_p)&&*end_p){}
                    if(*end_p!=(first_number_parsed?';':',')){
                        fprintf(stderr,ERR("%s not found or non-number found at line %lu char %lu state %s.\n"),first_number_parsed?"Semicolon":"Comma",line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    num_str_arr[first_number_parsed]=char_string_slice(begin_p,end_p-1);
                    read_i+=end_p-begin_p+1;
                    read_offset_i=-1;
                }else if(current_char=='('){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=')'&&*end_p!=';'&&*end_p){}
                    if(*end_p!=')'||*(end_p+1)!=(first_number_parsed?';':',')){
                        fprintf(stderr,ERR("RPN string doesn't terminate with ')%c' at line %lu char %lu state %s.\n"),first_number_parsed?';':',',line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;   
                    }
                    rpn_str_arr[first_number_parsed]=char_string_slice(begin_p,end_p);
                    read_i+=end_p-begin_p+2;//+2 for ')' and ','/';'.
                    read_offset_i=-1;
                }else{
                    fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                    DO_ERROR();
                    break;
                }
                if(num_str_arr[first_number_parsed]){
                    vlci[first_number_parsed]=VL_new_callback_int(this->vl,strtol(num_str_arr[first_number_parsed],NULL,10));
                    free(num_str_arr[first_number_parsed]);
                }else vlci[first_number_parsed]=VL_new_callback_number_rpn(this->vl,rpn_str_arr[first_number_parsed],rpn_debug);
                if(!first_number_parsed){
                    first_number_parsed=true;
                    break;
                }//After second number is successfully parsed.
                if(this->contents[this->token_i+read_i+read_offset_i]!=';'){
                    fprintf(stderr,ERR("Expecting only 2 numbers that end with ';' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                    DO_ERROR();
                    break;
                }
                command_array_add(this->cmd_arr,
                (command_t){.type=CMD_MoveMouse,.subtype=CMDST_Command,.print_cmd=print_cmd,
                        .cmd_u.mouse_move=(mouse_move_t){
                            .x_cb=vlci[0],.y_cb=vlci[1],.is_absolute=is_absolute
                        }
                    }
                );
                key_processed=true;
                break;
            case RS_JumpTo:
                if(char_is_key(current_char)) break;
                if(current_char==';'){
                    str_name=malloc(sizeof(char)*(read_offset_i+1));
                    EXIT_IF_NULL(str_name,char*);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    str_name[read_offset_i]='\0';
                    int jid_cmd_i=jump_id_manager_search_command_index(this->jim,str_name);
                    if(jid_cmd_i==-1){
                        jump_id_manager_add_name(this->jim,str_name,JumpFromNotConnected,false);//-2 Because it's not a JumpFrom
                        command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_JumpTo,.subtype=CMDST_Command,.print_cmd=print_cmd,
                                .cmd_u.jump_to=(jump_to_t){
                                    .cmd_index=JumpFromNotConnected,//Will be edited from a CMD_JumpFrom later.
                                    .str_index=jump_id_manager_search_string_index(this->jim,str_name),
                                    .store_index=store_index
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }else{
                        command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_JumpTo,.subtype=CMDST_Command,.print_cmd=print_cmd,
                                .cmd_u.jump_to=(jump_to_t){
                                    .cmd_index=jid_cmd_i,
                                    .str_index=jump_id_manager_search_string_index(this->jim,str_name),
                                    .store_index=store_index
                                }
                            }
                        );
                        key_processed=true;
                        free(str_name);
                        break;
                    }
                    fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                   DO_ERROR();
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_JumpToIndex:
                if(isdigit(current_char)||current_char=='-'){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=';'&&isdigit(*end_p)&&*end_p){}
                    if(*end_p!=';'){
                        fprintf(stderr,ERR("Semicolon not found or non-number found at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    num_str_arr[0]=char_string_slice(begin_p,end_p-1);//-1 to exclude ;
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_JumpToIndex,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.jump_to_index=(jump_to_index_t){
                                .jump_cb=VL_new_callback_int(this->vl,strtol(num_str_arr[0],NULL,10)),
                                .is_absolute=is_absolute
                            }
                            
                        }
                    );
                    free(num_str_arr[0]);
                    read_offset_i+=end_p-begin_p;
                    key_processed=true;
                    break;
                }
                if(current_char=='('){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=')'&&*end_p!=';'&&*end_p){}
                    if(*end_p!=')'||*(end_p+1)!=';'){
                        fprintf(stderr,ERR("RPN string doesn't terminate with ');' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;   
                    }
                    rpn_str_arr[0]=char_string_slice(begin_p,end_p);
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_JumpToIndex,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.jump_to_index=(jump_to_index_t){
                                .jump_cb=VL_new_callback_number_rpn(this->vl,rpn_str_arr[0],rpn_debug),
                                .is_absolute=is_absolute
                            }
                        }
                    );
                    read_offset_i+=end_p-begin_p+1;//+1 to read past semicolon.
                    key_processed=true;
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_JumpFrom:
                if(char_is_key(current_char)) break;
                if(current_char==';'){
                    str_name=malloc(sizeof(char)*(read_offset_i+1));
                    EXIT_IF_NULL(str_name,char*);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    str_name[read_offset_i]='\0';
                    int jid_cmd_i=jump_id_manager_search_command_index(this->jim,str_name);
                    int jid_str_i=jump_id_manager_search_string_index(this->jim,str_name);
                    int cmd_arr_count=command_array_count(this->cmd_arr);
                    if(jid_cmd_i==-1){
                        jump_id_manager_add_name(this->jim,str_name,cmd_arr_count,true);
                        command_array_add(this->cmd_arr,
                            (command_t){.type=CMD_JumpFrom,.subtype=CMDST_Command,.print_cmd=print_cmd,
                                .cmd_u.jump_from=(jump_from_t){
                                    .str_index=jump_id_manager_search_string_index(this->jim,str_name)
                                }
                            }
                        );
                        SSManager_add_string(this->jim->ssm,&str_name);//Consume all strings with the same contents to free (No memory leak).
                        key_processed=true;
                        break;
                    }else{
                        bool unique=jump_id_manager_set_command_index_once(this->jim,jid_str_i,cmd_arr_count);
                        if(unique){//No Second RS_JumpFrom
                            command_array_add(this->cmd_arr,
                                (command_t){.type=CMD_JumpFrom,.subtype=CMDST_Command,.print_cmd=print_cmd,
                                    .cmd_u.jump_from=(jump_from_t){
                                        .str_index=jid_str_i
                                    }
                                }
                            );
                            for(int i=0;i<cmd_arr_count;i++){
                                command_t* this_cmd=this->cmd_arr->cmds+i;
                                if(this_cmd->type==CMD_JumpTo&&this_cmd->cmd_u.jump_to.str_index==jid_str_i){
                                    this_cmd->cmd_u.jump_to.cmd_index=cmd_arr_count;//Set all JumpTo to this JumpFrom index.
                                }
                            }
                            SSManager_add_string(this->jim->ssm,&str_name);
                            key_processed=true;
                            break;
                        }
                        fprintf(stderr,ERR("Cannot add a second JumpFrom with string '%s' at line %lu char %lu state %s.\n"),str_name,line_num,char_num,ReadStateStrings[read_state]);
                        free(str_name);
                        DO_ERROR();
                        break;
                    }
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_Query:
                if(isdigit(current_char)){
                    if(query_comb_type!=QCT_NONE){
                        fprintf(stderr,ERR("? queries can only have custom index numbers allowed once at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    const char* nb=current_char_p,* ne=current_char_p-1;
                    if(*(++ne)!='-') ne--; //Check if '-' as first char only. Recheck in while loop if not.
                    while(isdigit(*(++ne))&&*ne!=':'&&*ne){}
                    if(*ne!=':'){
                        fprintf(stderr,ERR("Invalid character '%c' (Should be numbers ending with ':') at line %lu char %lu state %s.\n"),*ne,line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    char* num=char_string_slice(nb,ne-1);
                    query_jump_e=strtol(num,NULL,10);
                    free(num);
                    nb=ne+1;
                    if(*(++ne)!='-') ne--;
                    while(isdigit(*(++ne))&&*ne!=','&&*ne){}
                    if(*ne!=','){
                        fprintf(stderr,ERR("Invalid character '%c' (Should be numbers ending with ',') at line %lu char %lu state %s.\n"),*ne,line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    num=char_string_slice(nb,ne-1);
                    query_jump_ne=strtol(num,NULL,10);
                    free(num);
                    read_i+=ne-current_char_p+1;
                    read_offset_i=-1;
                    query_comb_type=QCT_CUSTOM;
                    break;
                }
                if(current_char=='!'){
                    read_i+=1;
                    read_offset_i=-1;
                    invert_b=!invert_b;
                    break;
                }
                if(!strncmp(current_char_p,"pxc=",4)){
                    read_i+=4;
                    read_offset_i=-1;
                    read_state=RS_QueryComparePixel;
                    break;
                }
                if(!strncmp(current_char_p,"coords=",7)){
                    if(this->contents[this->token_i+read_i+read_offset_i+7]=='x'){
                        cmp_flags|=CMP_X;
                        read_i+=8;
                        read_offset_i=-1;
                        read_state=RS_QueryCoordsType;
                        break;
                    }else if(this->contents[this->token_i+read_i+read_offset_i+7]=='y'){
                        cmp_flags|=CMP_Y;
                        read_i+=8;
                        read_offset_i=-1;
                        read_state=RS_QueryCoordsType;
                        break;
                    }
                }
                if(!strncmp(current_char_p,"within=",7)){
                    read_i+=7;
                    read_offset_i=-1;
                    read_state=RS_QueryCoordsWithin;
                    break;
                }
                if(!strncmp(current_char_p,"eval=",5)){
                    read_i+=5;
                    read_offset_i=-1;
                    read_state=RS_QueryRPNEval;
                    break;
                }
                if(!strncmp(current_char_p,"key_pressed=",12)){
                    read_i+=12;
                    read_offset_i=-1;
                    read_state=RS_QueryKeyPress;
                    break;
                }
                if(!strncmp(current_char_p,"button_pressed=",15)){
                    read_i+=15;
                    read_offset_i=-1;
                    read_state=RS_QueryButtonPress;
                    break;
                }
                SHOULD_BE_UNREACHABLE();
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_QueryComparePixel:
                if(isdigit(current_char)){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=((parsed_num_i!=3)?',':'?')&&isdigit(*end_p)&&*end_p){}
                    if(*end_p!=((parsed_num_i!=3)?',':'?')){
                        fprintf(stderr,ERR("%s not found or non-number found at line %lu char %lu state %s.\n"),(parsed_num_i==3)?"Question mark":"Comma",line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    num_str_arr[parsed_num_i]=char_string_slice(begin_p,end_p-1);
                    read_i+=end_p-begin_p+1;
                    read_offset_i=-1;
                }else if(current_char=='('){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=')'&&*end_p!=';'&&*end_p){}
                    if(*end_p!=')'||*(end_p+1)!=((parsed_num_i!=3)?',':'?')){
                        fprintf(stderr,ERR("RPN string doesn't terminate with ')%c' at line %lu char %lu state %s.\n"),(parsed_num_i!=3)?',':'?',line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;   
                    }
                    rpn_str_arr[parsed_num_i]=char_string_slice(begin_p,end_p);
                    read_i+=end_p-begin_p+2;
                    read_offset_i=-1;
                }else{
                    fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                    DO_ERROR();
                    break;
                }
                if(num_str_arr[parsed_num_i]){
                    vlci[parsed_num_i]=VL_new_callback_char(this->vl,strtol(num_str_arr[parsed_num_i],NULL,10));
                    free(num_str_arr[parsed_num_i]);
                }else vlci[parsed_num_i]=VL_new_callback_number_rpn(this->vl,rpn_str_arr[parsed_num_i],rpn_debug);
                if(parsed_num_i++!=3) break; //Don't break after 4th number is successfully parsed.
                command_array_add(this->cmd_arr,
                    (command_t){.type=CMD_QueryComparePixel,.subtype=CMDST_Query,.print_cmd=print_cmd,
                        .cmd_u.pixel_compare=(pixel_compare_t){
                            .r_cb=vlci[0],.g_cb=vlci[1],.b_cb=vlci[2],.thr_cb=vlci[3]
                        },
                        .query_details=(query_details_t){
                            .invert=invert_b,
                            .comb_type=query_comb_type,
                            .jump_e=query_jump_e,
                            .jump_ne=query_jump_ne
                        }
                    }
                );
                key_processed=true;
                break;
            case RS_QueryCoordsType:
                if(current_char=='>'){
                    cmp_flags|=CMP_GT;
                    if(this->contents[this->token_i+read_i+read_offset_i+1]=='='){
                        read_i++; cmp_flags|=CMP_W_EQ;
                    }
                    read_i++;
                    read_offset_i=-1;
                    read_state=RS_QueryCoordsVarValue;
                    break;
                }
                if(current_char=='<'){
                    cmp_flags|=CMP_LT;
                    if(this->contents[this->token_i+read_i+read_offset_i+1]=='='){
                        read_i++; cmp_flags|=CMP_W_EQ;
                    }
                    read_i++;
                    read_offset_i=-1;
                    read_state=RS_QueryCoordsVarValue;
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_QueryCoordsVarValue:
                if(isdigit(current_char)){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!='?'&&isdigit(*end_p)&&*end_p){}
                    if(*end_p!='?'){
                        fprintf(stderr,ERR("Question mark not found or non-number found at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    num_str_arr[0]=char_string_slice(begin_p,end_p-1);
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_QueryCompareCoords,.subtype=CMDST_Query,.print_cmd=print_cmd,
                            .cmd_u.compare_coords=(compare_coords_t){
                                .cmp_flags=cmp_flags,.var_callback=VL_new_callback_int(this->vl,strtol(num_str_arr[0],NULL,10))
                            },
                            .query_details=(query_details_t){
                                .invert=invert_b,
                                .comb_type=query_comb_type,
                                .jump_e=query_jump_e,
                                .jump_ne=query_jump_ne
                        }
                        }
                    );
                    free(num_str_arr[0]);
                    read_offset_i+=end_p-begin_p;
                    key_processed=true;
                    break;
                }
                if(current_char=='('){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=')'&&*end_p!='?'&&*end_p){}
                    if(*end_p!=')'||*(end_p+1)!='?'){
                        fprintf(stderr,ERR("RPN string doesn't terminate with ')?' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;   
                    }
                    rpn_str_arr[0]=char_string_slice(begin_p,end_p);
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_QueryCompareCoords,.subtype=CMDST_Query,.print_cmd=print_cmd,
                            .cmd_u.compare_coords=(compare_coords_t){
                                .cmp_flags=cmp_flags,.var_callback=VL_new_callback_number_rpn(this->vl,rpn_str_arr[0],rpn_debug)
                            },
                            .query_details=(query_details_t){
                                .invert=invert_b,
                                .comb_type=query_comb_type,
                                .jump_e=query_jump_e,
                                .jump_ne=query_jump_ne
                            }
                        }
                    );
                    read_offset_i+=end_p-begin_p+1;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_QueryCoordsWithin:
                if(isdigit(current_char)){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=((parsed_num_i!=3)?',':'?')&&isdigit(*end_p)&&*end_p){}
                    if(*end_p!=((parsed_num_i!=3)?',':'?')){
                        fprintf(stderr,ERR("%s not found or non-number found at line %lu char %lu state %s.\n"),(parsed_num_i==3)?"Question mark":"Comma",line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    num_str_arr[parsed_num_i]=char_string_slice(begin_p,end_p-1);
                    read_i+=end_p-begin_p+1;
                    read_offset_i=-1;
                }else if(current_char=='('){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=')'&&*end_p!=';'&&*end_p){}
                    if(*end_p!=')'||*(end_p+1)!=((parsed_num_i!=3)?',':'?')){
                        fprintf(stderr,ERR("RPN string doesn't terminate with ')%c' at line %lu char %lu state %s.\n"),(parsed_num_i!=3)?',':'?',line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;   
                    }
                    rpn_str_arr[parsed_num_i]=char_string_slice(begin_p,end_p);
                    read_i+=end_p-begin_p+2;
                    read_offset_i=-1;
                }else{
                    fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                    DO_ERROR();
                    break;
                }
                if(num_str_arr[parsed_num_i]){
                    vlci[parsed_num_i]=VL_new_callback_int(this->vl,strtol(num_str_arr[parsed_num_i],NULL,10));
                    free(num_str_arr[parsed_num_i]);
                }else vlci[parsed_num_i]=VL_new_callback_number_rpn(this->vl,rpn_str_arr[parsed_num_i],rpn_debug);
                if(parsed_num_i++!=3) break; //Don't break after 4th number is successfully parsed.
                command_array_add(this->cmd_arr,
                    (command_t){.type=CMD_QueryCoordsWithin,.subtype=CMDST_Query,.print_cmd=print_cmd,
                        .cmd_u.coords_within=(coords_within_t){
                            .xl_cb=vlci[0],.yl_cb=vlci[1],.xh_cb=vlci[2],.yh_cb=vlci[3]
                        },
                        .query_details=(query_details_t){
                            .invert=invert_b,
                            .comb_type=query_comb_type,
                            .jump_e=query_jump_e,
                            .jump_ne=query_jump_ne
                        }
                    }
                );
                key_processed=true;
                break;
            case RS_QueryRPNEval:
                if(char_is_key(current_char)) break;
                if(current_char=='('){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=')'&&*end_p!='?'&&*end_p){}
                    if(*end_p!=')'||*(end_p+1)!='?'){
                        fprintf(stderr,ERR("RPN string doesn't terminate with ')?' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;   
                    }
                    rpn_str_arr[0]=char_string_slice(begin_p,end_p);
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_QueryRPNEval,.subtype=CMDST_Query,.print_cmd=print_cmd,
                            .cmd_u.rpn_eval=VL_new_callback_number_rpn(this->vl,rpn_str_arr[0],rpn_debug),
                            .query_details=(query_details_t){
                                .invert=invert_b,
                                .comb_type=query_comb_type,
                                .jump_e=query_jump_e,
                                .jump_ne=query_jump_ne
                            }
                        }
                    );
                    read_offset_i+=end_p-begin_p+1;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_QueryKeyPress:
                if(char_is_key(current_char)) break;
                if(current_char=='?'){
                    str_name=malloc(sizeof(char)*(read_offset_i+1));
                    EXIT_IF_NULL(str_name,char*);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    str_name[read_offset_i]='\0';
                    KeySym keysym;
                    if(!strcmp(str_name,"Escape")){
                        fprintf(stderr,ERR("Key 'Escape' cannot be used, because it is used to quit the macro.\n"));
                        free(str_name);
                        DO_ERROR();
                        break;
                    }
                    if((keysym=XStringToKeysym(str_name))){
                        command_array_add(this->cmd_arr,
                            (command_t){.type=CMD_QueryKeyPress,.subtype=CMDST_Query,.print_cmd=print_cmd,
                                .cmd_u.qkey_pressed=(keystroke_t){
                                    .keysym=keysym,
                                    .key=str_name
                                },
                                .query_details=(query_details_t){
                                    .invert=invert_b,
                                    .comb_type=query_comb_type,
                                    .jump_e=query_jump_e,
                                    .jump_ne=query_jump_ne
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }else{
                        fprintf(stderr,ERR("Key '%s' does not contain a valid KeySym number.\n"),str_name);
                        free(str_name);
                        DO_ERROR();
                        break;
                    }
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_QueryButtonPress:
                if(isdigit(current_char)){
                    if(current_char_p[1]!='?'){
                        fprintf(stderr,ERR("Command should be terminated with '?' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    parsed_num=current_char-'0';
                    if(parsed_num<1||parsed_num>5){
                        fprintf(stderr,ERR("Invalid mouse button number (Should be 1 to 5) at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_QueryButtonPress,.subtype=CMDST_Query,.print_cmd=print_cmd,
                            .cmd_u.qbutton_pressed=(mouse_button_t){
                                .button=parsed_num
                            },
                            .query_details=(query_details_t){
                                .invert=invert_b,
                                .comb_type=query_comb_type,
                                .jump_e=query_jump_e,
                                .jump_ne=query_jump_ne
                            }
                        }
                    );
                    key_processed=true;
                    read_offset_i++;
                    break;
                }
                fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_InitVarType:
                switch(current_char){
                    case 'c': vct=VLCallback_Char; goto valid_var_char;
                    case 'i': vct=VLCallback_Int; goto valid_var_char;
                    case 'l': vct=VLCallback_Long; goto valid_var_char;
                    case 'd': vct=VLCallback_Double; goto valid_var_char;
                    case 'r': vct=VLCallback_NumberRPN; goto valid_var_char;
                    default: goto invalid_var_char;
                }
                valid_var_char:
                if(*(current_char_p+1)==','){
                    read_i+=2;
                    read_offset_i=-1;
                    read_state=RS_InitVarName;
                    break;
                }
                invalid_var_char:
                fprintf(stderr,ERR("Invalid characters (Should be 'c/i/l/d/r' with ',' at end) at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_InitVarName:
                if(char_is_key(current_char)) break;
                if(current_char=='='){
                    str_name=malloc(sizeof(char)*(read_offset_i+1));
                    EXIT_IF_NULL(str_name,char*);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    str_name[read_offset_i]='\0';
                    read_i+=read_offset_i+1;
                    read_offset_i=-1;
                    read_state=RS_InitVarValue;
                    break;
                }
                fprintf(stderr,ERR("Invalid variable character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_InitVarValue:
                if(vct==VLCallback_NumberRPN){
                    switch(*current_char_p){
                        case 'c': vct=VLCallback_Char; goto valid_var_char2;
                        case 'i': vct=VLCallback_Int; goto valid_var_char2;
                        case 'l': vct=VLCallback_Long; goto valid_var_char2;
                        case 'd': vct=VLCallback_Double; goto valid_var_char2;
                        default: goto invalid_var_char2;
                    }
                    invalid_var_char2:
                    fprintf(stderr,ERR("RPN string needs to start with integer type i/l/d/c for InitVar commands at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                    free(str_name);
                    DO_ERROR();
                    break; 
                    valid_var_char2:
                    if(*(++current_char_p)=='('){
                        as_number_t an;
                        begin_p=current_char_p;
                        while(*(end_p=++current_char_p)!=')'&&*end_p!='?'&&*end_p){}
                        if(*end_p!=')'||*(end_p+1)!=';'){
                            free(str_name);
                            fprintf(stderr,ERR("RPN string doesn't terminate with ');' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                            DO_ERROR();
                            break;   
                        }
                        rpn_str_arr[0]=char_string_slice(begin_p,end_p);
                        RPNValidStringE status=RPNEvaluatorEvaluate(rpn_str_arr[0],this->vl,&an,rpn_debug,true,RPN_EVAL_START_B,RPN_EVAL_END_B,RPN_EVAL_SEP);
                        free(rpn_str_arr[0]);
                        if(status!=RPNVS_Ok){
                            free(str_name);
                            DO_ERROR();
                            break;
                        }
                        switch(vct){
                            case VLCallback_Char: an=VLNumberCast(an,VLNT_Char); break;
                            case VLCallback_Int: an=VLNumberCast(an,VLNT_Int); break;
                            case VLCallback_Long: an=VLNumberCast(an,VLNT_Long); break;
                            case VLCallback_Double: an=VLNumberCast(an,VLNT_Double); break;
                            default: SHOULD_BE_UNREACHABLE(); break;
                        }
                        switch(an.type){
                            case VLNT_Char:
                                if(VL_add_as_char(this->vl,&str_name,an.c)==VA_Rewritten){
                                    fprintf(stderr,ERR("Variable name '%s' already assigned at line %lu char %lu state %s.\n"),str_name,line_num,char_num,ReadStateStrings[read_state]);
                                    this->parse_error=true;
                                    key_processed=true;
                                    break;
                                }
                                goto init_var_value_rpn_success;
                            case VLNT_Int:
                                if(VL_add_as_int(this->vl,&str_name,an.i)==VA_Rewritten){
                                    fprintf(stderr,ERR("Variable name '%s' already assigned at line %lu char %lu state %s.\n"),str_name,line_num,char_num,ReadStateStrings[read_state]);
                                    this->parse_error=true;
                                    key_processed=true;
                                    break;
                                }
                                goto init_var_value_rpn_success;
                            case VLNT_Long:
                                if(VL_add_as_long(this->vl,&str_name,an.l)==VA_Rewritten){
                                    fprintf(stderr,ERR("Variable name '%s' already assigned at line %lu char %lu state %s.\n"),str_name,line_num,char_num,ReadStateStrings[read_state]);
                                    this->parse_error=true;
                                    key_processed=true;
                                    break;
                                }
                                goto init_var_value_rpn_success;
                            case VLNT_Double:
                                if(VL_add_as_double(this->vl,&str_name,an.d)==VA_Rewritten){
                                    fprintf(stderr,ERR("Variable name '%s' already assigned at line %lu char %lu state %s.\n"),str_name,line_num,char_num,ReadStateStrings[read_state]);
                                    this->parse_error=true;
                                    key_processed=true;
                                    break;
                                }
                                goto init_var_value_rpn_success;
                            default: SHOULD_BE_UNREACHABLE(); break;
                        }
                        init_var_value_rpn_success:
                        command_array_add(this->cmd_arr,
                            (command_t){.type=CMD_InitVar,.subtype=CMDST_Command,.print_cmd=print_cmd,
                                .cmd_u.init_var=(init_var_t){
                                    .as_number=an,
                                    .variable=str_name
                                }
                            }
                        );
                        read_offset_i+=end_p-begin_p+2;
                        key_processed=true;
                        break;
                    }
                    free(str_name);
                    fprintf(stderr,ERR("Unexpected character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                    DO_ERROR();
                    break;
                }
                if(isdigit(current_char)||current_char=='.'||current_char=='-') break;
                if(current_char==';'){
                    num_str_arr[0]=malloc(sizeof(char)*(read_offset_i+1));
                    EXIT_IF_NULL(num_str_arr[0],char*);
                    strncpy(num_str_arr[0],this->contents+this->token_i+read_i,read_offset_i);
                    num_str_arr[0][read_offset_i]='\0';
                    switch(vct){
                        case VLCallback_Char:
                            if(VL_add_as_char(this->vl,&str_name,strtol(num_str_arr[0],0,10))==VA_Rewritten){
                                fprintf(stderr,ERR("Variable name '%s' already assigned at line %lu char %lu state %s.\n"),str_name,line_num,char_num,ReadStateStrings[read_state]);
                                this->parse_error=true;
                                break;
                            }
                            command_array_add(this->cmd_arr,
                                (command_t){.type=CMD_InitVar,.subtype=CMDST_Command,.print_cmd=print_cmd,
                                    .cmd_u.init_var=(init_var_t){
                                        .as_number=(as_number_t){
                                            .c=strtol(num_str_arr[0],0,10),
                                            .type=VLNT_Char
                                        },
                                        .variable=str_name
                                    }
                                }
                            );
                            break;
                        case VLCallback_Int:
                            if(VL_add_as_int(this->vl,&str_name,strtol(num_str_arr[0],0,10))==VA_Rewritten){
                                fprintf(stderr,ERR("Variable name '%s' already assigned at line %lu char %lu state %s.\n"),str_name,line_num,char_num,ReadStateStrings[read_state]);
                                this->parse_error=true;
                                break;
                            }
                            command_array_add(this->cmd_arr,
                                (command_t){.type=CMD_InitVar,.subtype=CMDST_Command,.print_cmd=print_cmd,
                                    .cmd_u.init_var=(init_var_t){
                                        .as_number=(as_number_t){
                                            .i=strtol(num_str_arr[0],0,10),
                                            .type=VLNT_Int
                                        },
                                        .variable=str_name
                                    }
                                }
                            );
                            break;
                        case VLCallback_Long:
                            if(VL_add_as_long(this->vl,&str_name,strtol(num_str_arr[0],0,10))==VA_Rewritten){
                                fprintf(stderr,ERR("Variable name '%s' already assigned at line %lu char %lu state %s.\n"),str_name,line_num,char_num,ReadStateStrings[read_state]);
                                this->parse_error=true;
                                break;
                            }
                            command_array_add(this->cmd_arr,
                                (command_t){.type=CMD_InitVar,.subtype=CMDST_Command,.print_cmd=print_cmd,
                                    .cmd_u.init_var=(init_var_t){
                                        .as_number=(as_number_t){
                                            .l=strtol(num_str_arr[0],0,10),
                                            .type=VLNT_Long
                                        },
                                        .variable=str_name
                                    }
                                }
                            );
                            break;
                        case VLCallback_Double:
                            if(VL_add_as_double(this->vl,&str_name,strtol(num_str_arr[0],0,10))==VA_Rewritten){
                                fprintf(stderr,ERR("Variable name '%s' already assigned at line %lu char %lu state %s.\n"),str_name,line_num,char_num,ReadStateStrings[read_state]);
                                this->parse_error=true;
                                break;
                            }
                            command_array_add(this->cmd_arr,
                                (command_t){.type=CMD_InitVar,.subtype=CMDST_Command,.print_cmd=print_cmd,
                                    .cmd_u.init_var=(init_var_t){
                                        .as_number=(as_number_t){
                                            .d=strtod(num_str_arr[0],0),
                                            .type=VLNT_Double
                                        },
                                        .variable=str_name
                                    }
                                }
                            );
                            break;
                        default: SHOULD_BE_UNREACHABLE(); break;
                    }
                    free(num_str_arr[0]);
                    key_processed=true;
                    break;
                }
                free(str_name);
                fprintf(stderr,ERR("Invalid variable character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_EditVarName:
                if(char_is_key(current_char)) break;
                if(current_char=='='){
                    str_name=malloc(sizeof(char)*(read_offset_i+1));
                    EXIT_IF_NULL(str_name,char*);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    str_name[read_offset_i]='\0';
                    if(!VL_get_as_number(this->vl,str_name).exists){
                        fprintf(stderr,ERR("Unitialized variable '%s' at line %lu char %lu state %s.\n"),str_name,line_num,char_num,ReadStateStrings[read_state]);
                        free(str_name);
                        DO_ERROR();
                        break;
                    }
                    read_i+=read_offset_i+1;
                    read_offset_i=-1;
                    read_state=RS_EditVarValue;
                    break;
                }
                fprintf(stderr,ERR("Invalid variable character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_EditVarValue:
                if(current_char=='('){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=')'&&*end_p!=';'&&*end_p){}
                    if(*end_p!=')'||*(end_p+1)!=';'){
                        fprintf(stderr,ERR("RPN string doesn't terminate with ');' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        free(str_name);
                        DO_ERROR();
                        break;   
                    }
                    rpn_str_arr[0]=char_string_slice(begin_p,end_p);
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_EditVar,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.edit_var=VL_new_callback_rewrite_variable_rpn(this->vl,rpn_str_arr[0],str_name,rpn_debug)
                        }
                    );
                    read_offset_i+=end_p-begin_p+1;
                    key_processed=true;
                    break;
                }
                free(str_name);
                fprintf(stderr,ERR("Invalid variable character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_WaitUntilKey:
                if(char_is_key(current_char)) break;
                if(current_char==';'){
                    str_name=malloc(sizeof(char)*(read_offset_i+1));
                    EXIT_IF_NULL(str_name,char*);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    str_name[read_offset_i]='\0';
                    KeySym keysym;
                    if(!strcmp(str_name,"Escape")){
                        fprintf(stderr,ERR("Key 'Escape' cannot be used, because it is used to quit the macro.\n"));
                        free(str_name);
                        DO_ERROR();
                        break;
                    }
                    if((keysym=XStringToKeysym(str_name))){
                        command_array_add(this->cmd_arr,
                            (command_t){.type=CMD_WaitUntilKey,.subtype=CMDST_Command,.print_cmd=print_cmd,
                                .cmd_u.wait_until_key=(keystroke_t){
                                    .keysym=keysym,
                                    .key=str_name,
                                    .invert_press=invert_b
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }else{
                        fprintf(stderr,ERR("Key '%s' does not contain a valid KeySym number.\n"),str_name);
                        free(str_name);
                        DO_ERROR();
                        break;
                    }
                }
                fprintf(stderr,ERR("Invalid variable character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_WaitUntilButton:
                if(isdigit(current_char)){
                    if(current_char_p[1]!=';'){
                        fprintf(stderr,ERR("Command should be terminated with ';' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    parsed_num=current_char-'0';
                    if(parsed_num<1||parsed_num>5){
                        fprintf(stderr,ERR("Invalid mouse button number (Should be 1 to 5) at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_WaitUntilButton,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.wait_until_button=(mouse_button_t){
                                .button=parsed_num,
                                .held_down=held_down,
                                .invert_press=invert_b
                            }
                        }
                    );
                    key_processed=true;
                    read_offset_i++;
                    break;
                }
                fprintf(stderr,ERR("Invalid variable character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_GrabKey:
                if(char_is_key(current_char)) break;
                if(current_char==';'){
                    str_name=malloc(sizeof(char)*(read_offset_i+1));
                    EXIT_IF_NULL(str_name,char*);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    str_name[read_offset_i]='\0';
                    KeySym keysym;
                    if(!strcmp(str_name,"Escape")){
                        fprintf(stderr,ERR("Key 'Escape' cannot be used, because it is used to quit the macro.\n"));
                        free(str_name);
                        DO_ERROR();
                        break;
                    }
                    if((keysym=XStringToKeysym(str_name))){
                        command_array_add(this->cmd_arr,
                            (command_t){.type=CMD_GrabKey,.subtype=CMDST_Command,.print_cmd=print_cmd,
                                .cmd_u.grab_key=(keystroke_t){
                                    .keysym=keysym,
                                    .key=str_name
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }else{
                        fprintf(stderr,ERR("Key '%s' does not contain a valid KeySym number.\n"),str_name);
                        free(str_name);
                        DO_ERROR();
                        break;
                    }
                }
                fprintf(stderr,ERR("Invalid variable character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_UngrabKey:
                if(char_is_key(current_char)) break;
                if(current_char==';'){
                    str_name=malloc(sizeof(char)*(read_offset_i+1));
                    EXIT_IF_NULL(str_name,char*);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    str_name[read_offset_i]='\0';
                    KeySym keysym;
                    if(!strcmp(str_name,"Escape")){
                        fprintf(stderr,ERR("Key 'Escape' cannot be used, because it is used to quit the macro.\n"));
                        free(str_name);
                        DO_ERROR();
                        break;
                    }
                    if((keysym=XStringToKeysym(str_name))){
                        command_array_add(this->cmd_arr,
                            (command_t){.type=CMD_UngrabKey,.subtype=CMDST_Command,.print_cmd=print_cmd,
                                .cmd_u.ungrab_key=(keystroke_t){
                                    .keysym=keysym,
                                    .key=str_name
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }else{
                        fprintf(stderr,ERR("Key '%s' does not contain a valid KeySym number.\n"),str_name);
                        free(str_name);
                        DO_ERROR();
                        break;
                    }
                }
                fprintf(stderr,ERR("Invalid variable character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_GrabButton:
                if(isdigit(current_char)){
                    if(current_char_p[1]!=';'){
                        fprintf(stderr,ERR("Command should be terminated with ';' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    parsed_num=current_char-'0';
                    if(parsed_num<1||parsed_num>5){
                        fprintf(stderr,ERR("Invalid mouse button number (Should be 1 to 5) at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_GrabButton,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.grab_button=(mouse_button_t){
                                .button=parsed_num
                            }
                        }
                    );
                    key_processed=true;
                    read_offset_i++;
                    break;
                }
                fprintf(stderr,ERR("Invalid variable character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_UngrabButton:
                if(isdigit(current_char)){
                    if(current_char_p[1]!=';'){
                        fprintf(stderr,ERR("Command should be terminated with ';' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    parsed_num=current_char-'0';
                    if(parsed_num<1||parsed_num>5){
                        fprintf(stderr,ERR("Invalid mouse button number (Should be 1 to 5) at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;
                    }
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_UngrabButton,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.ungrab_button=(mouse_button_t){
                                .button=parsed_num
                            }
                        }
                    );
                    key_processed=true;
                    read_offset_i++;
                    break;
                }
                fprintf(stderr,ERR("Invalid variable character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_PrintString:
                if(current_char=='('){
                    begin_p=current_char_p;
                    while(*(end_p=++current_char_p)!=')'&&*end_p)
                        if(*end_p==';') goto print_string_rpn_semicolon;
                    if(*end_p=='\0'){
                        free(str_name);
                        free(print_rpn_strs);
                        fprintf(stderr,ERR("Print command probably doesn't end with ';;' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                        DO_ERROR();
                        break;   
                    }
                    rpn_str_arr[0]=char_string_slice(begin_p,end_p);
                    read_offset_i+=end_p-begin_p;
                    SSManager_add_string(this->vl->ssm,&rpn_str_arr[0]);//Add rpn string to free later.
                    str_name=realloc(str_name,sizeof(char[(print_str_len=print_str_len+5)]));
                    EXIT_IF_NULL(str_name,char*);
                    print_rpn_strs=realloc(print_rpn_strs,sizeof(char*)*++print_rpn_strs_len);
                    EXIT_IF_NULL(print_rpn_strs,char**);
                    print_rpn_strs[print_rpn_strs_len-1]=rpn_str_arr[0];//Add to array for command.
                    memcpy(str_name+print_str_len-5,"%_rpn",sizeof(char[5]));
                    break;
                    print_string_rpn_semicolon:
                    free(str_name);
                    free(print_rpn_strs);
                    fprintf(stderr,ERR("Print command probably doesn't end with ';;' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                    DO_ERROR();
                    break;
                }
                if(current_char==';'&&current_char_p[1]==';'){
                    if(print_newline){
                        str_name=realloc(str_name,sizeof(char[++print_str_len]));
                        str_name[print_str_len-1]='\n';
                    }
                    str_name=realloc(str_name,sizeof(char[++print_str_len]));
                    str_name[print_str_len-1]='\0';
                    vp_array_add(this->vpa,print_rpn_strs);
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_PrintString,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.print_string=(print_string_t){
                                .str=str_name,
                                .newline=print_newline,
                                .rpn_strs=(const char**)print_rpn_strs,
                                .rpn_strs_len=print_rpn_strs_len
                            }
                        }
                    );
                    read_offset_i+=1; //+1 to count for ;;.
                    key_processed=true;
                    break;
                }
                if(current_char=='\\'){
                    switch(current_char_p[1]){
                        case 'a': esc_seq='\a'; goto valid_esc_seq;
                        case 'b': esc_seq='\b'; goto valid_esc_seq;
                        case 'e': esc_seq='\x1b'; goto valid_esc_seq;
                        case 'f': esc_seq='\f'; goto valid_esc_seq;
                        case 'n': esc_seq='\n'; goto valid_esc_seq;
                        case 'r': esc_seq='\r'; goto valid_esc_seq;
                        case 't': esc_seq='\t'; goto valid_esc_seq;
                        case 'v': esc_seq='\v'; goto valid_esc_seq;
                        case '(': esc_seq='('; goto valid_esc_seq;
                        case ';': esc_seq=';'; goto valid_esc_seq;
                        case '\n': goto skip_esc_seq;
                        default: goto invalid_esc_seq;
                    }
                    valid_esc_seq:
                    str_name=realloc(str_name,sizeof(char[++print_str_len]));
                    str_name[print_str_len-1]=esc_seq;
                    skip_esc_seq://Newline or pressing enter. ('\\n')
                    read_offset_i+=1;
                    break;
                    invalid_esc_seq:
                    free(str_name);
                    free(print_rpn_strs);
                    fprintf(stderr,ERR("Invalid escape sequence \\%c found at line %lu char %lu state %s.\n"),current_char_p[1],line_num,char_num,ReadStateStrings[read_state]);
                    DO_ERROR();
                    break;
                }
                if(current_char=='\0'){
                    free(str_name);
                    free(print_rpn_strs);
                    fprintf(stderr,ERR("Print command probably doesn't end with ';;' at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                    DO_ERROR();
                    break;
                }
                str_name=realloc(str_name,sizeof(char[++print_str_len]));
                str_name[print_str_len-1]=current_char;
                break;
            case RS_DebugType:
                if(!strncmp(current_char_p,"debug_print_type=",17)){
                    read_i+=17;
                    read_offset_i=-1;
                    dct=DCT_DebugPrintType;
                    read_state=RS_DebugValue;
                    break;
                }
                if(!strncmp(current_char_p,"rpn_decimals=",13)){
                    read_i+=13;
                    read_offset_i=-1;
                    dct=DCT_RPNDecimals;
                    read_state=RS_DebugValue;
                    break;
                }
                if(!strncmp(current_char_p,"rpn_stack_debug=",16)){
                    read_i+=16;
                    read_offset_i=-1;
                    dct=DCT_RPNStackDebug;
                    read_state=RS_DebugValue;
                    break;
                }
                fprintf(stderr,ERR("Invalid variable character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_DebugValue:
                if(isdigit(current_char)) break;
                if(current_char==';'){
                    num_str_arr[0]=malloc(sizeof(char)*(read_offset_i+1));
                    EXIT_IF_NULL(num_str_arr[0],char*);
                    strncpy(num_str_arr[0],this->contents+this->token_i+read_i,read_offset_i);
                    num_str_arr[0][read_offset_i]='\0';
                    parsed_num=strtol(num_str_arr[0],NULL,10);
                    free(num_str_arr[0]);
                    if(dct==DCT_DebugPrintType){
                        if(parsed_num<0||parsed_num>2){
                            fprintf(stderr,ERR("debug_print_type should be 0, 1, or 2 at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                            DO_ERROR();
                            break;
                        }
                    }else if(dct==DCT_RPNDecimals){
                        if(parsed_num<0||parsed_num>255){
                            fprintf(stderr,ERR("rpn_decimals should be from 0 to 255 at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                            DO_ERROR();
                            break;
                        }
                    }else if(dct==DCT_RPNStackDebug){
                        if(parsed_num!=0&&parsed_num!=1){
                            fprintf(stderr,ERR("rpn_stack_debug should be from 0 to 1 at line %lu char %lu state %s.\n"),line_num,char_num,ReadStateStrings[read_state]);
                            DO_ERROR();
                            break;
                        }
                    }
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_DebugConfig,.subtype=CMDST_Command,.print_cmd=print_cmd,
                            .cmd_u.debug_config=(debug_config_t){
                                .type=dct,
                                .value=parsed_num
                            }
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,ERR("Invalid variable character '%c' at line %lu char %lu state %s.\n"),current_char,line_num,char_num,ReadStateStrings[read_state]);
                DO_ERROR();
                break;
            case RS_Count://Nothing (Shouldn't be used).
                break;
        }
        read_offset_i++;
    }while(!key_processed);
    this->token_i+=read_i+read_offset_i;
    char* parse_end_p=this->contents+this->token_i-1;
    if(!this->parse_error){
        int cmd_i=this->cmd_arr->size-1;
        command_t* this_cmd=this->cmd_arr->cmds+cmd_i;
        if(this_cmd->type==CMD_InitVar){
            int last_cmd_i=this->cmd_arr->size-2;
            if(last_cmd_i>=0&&(this->cmd_arr->cmds[last_cmd_i].type!=CMD_InitVar)){\
                fprintf(stderr,ERR("Unable to build! InitVar commands should be used only used at the start of the macro.\n"));
                this->parse_error=true;
                goto skip_check;
            }
        }
        this_cmd->start_cmd_p=parse_start_p;
        this_cmd->end_cmd_p=parse_end_p;
        if(this_cmd->subtype==CMDST_Query){
            if(cmd_i-1>=0){
                command_t* last_cmd=this->cmd_arr->cmds+cmd_i-1;
                if(last_cmd->subtype!=CMDST_Query&&this_cmd->subtype==CMDST_Query&&this_cmd->query_details.comb_type&~QCT_NONE&~QCT_CUSTOM)
                    puts(ERR("And and Or query types shouldn't be at the beginning (no effect)."));
            }
        }else goto skip_check;
        const QueryCombType tcqdct=this_cmd->query_details.comb_type;
        if(cmd_i-1>=0){
            command_t* last_cmd=this->cmd_arr->cmds+cmd_i-1;
            if(tcqdct==QCT_OR&&last_cmd->subtype==CMDST_Query)
                last_cmd->query_details.jump_ne=1;
        }
        while(--cmd_i>=0){
            command_t* last_cmd=this->cmd_arr->cmds+cmd_i;
            bool last_same=false;
            if(last_cmd->subtype!=CMDST_Query) break;
            const QueryCombType lcqdct=last_cmd->query_details.comb_type;
            if(tcqdct==QCT_AND&&lcqdct&(QCT_AND|QCT_NONE)){
                last_cmd->query_details.jump_ne++;
                last_same=true;
            }else if(tcqdct==QCT_OR&&lcqdct&(QCT_OR|QCT_NONE)){
                last_cmd->query_details.jump_e++;
                last_same=true;
            }else if(tcqdct==QCT_CUSTOM&&lcqdct==QCT_CUSTOM) last_same=true;
            if(!last_same){//Only chain same query AND/OR types with a normal one at the beginning.
                fprintf(stderr,ERR("Chained &...? and |...? query types cannot be mixed together, or queries without custom indices chained together.\n"));
                this->parse_error=true;
                break;
            } 
        }
    }
    skip_check:
    return !this->parse_error;
}
#undef DO_ERROR
void macro_buffer_str_id_check(macro_buffer_t* this,const VariableLoader_t* vl){
    bool* id_check=calloc(this->rim->size,sizeof(bool));
    EXIT_IF_NULL(id_check,bool*);
    for(int i=0;i<this->rim->size;i++){
        for(int j=0;j<this->cmd_arr->size;j++){
            const command_t cmd=this->cmd_arr->cmds[j];
            if(cmd.type==CMD_RepeatEnd&&cmd.cmd_u.repeat_end.str_index==i){id_check[i]=true;}
        }
    }
    for(int i=0;i<this->rim->size;i++){
        if(!id_check[i]){
            fprintf(stderr,ERR("RepeatEnd command missing for string '%s'\n"),this->rim->names[i]);
            this->parse_error=true;
        }
    }
    free(id_check);
    id_check=calloc(this->jim->size,sizeof(bool));
    EXIT_IF_NULL(id_check,bool*);
    for(int i=0;i<this->jim->size;i++){
        for(int j=0;j<this->cmd_arr->size;j++){
            const command_t cmd=this->cmd_arr->cmds[j];
            if(cmd.type==CMD_JumpFrom&&cmd.cmd_u.jump_from.str_index==i){id_check[i]=true;}
        }
    }
    for(int i=0;i<this->jim->size;i++){
        if(!id_check[i]){
            fprintf(stderr,ERR("JumpFrom command missing for string '%s'\n"),this->jim->names[i]);
            this->parse_error=true;
        }
    }
    free(id_check);
    int check_i=this->cmd_arr->size-1;
    const command_t any_cmd=this->cmd_arr->cmds[check_i];
    if(any_cmd.type==CMD_JumpFrom){
        fprintf(stderr,ERR("JumpFrom found without a command next to it. Error command found at end of file.\n"));
        this->parse_error=true;
    }
    if(any_cmd.type==CMD_JumpTo&&any_cmd.cmd_u.jump_to.store_index){
        fprintf(stderr,ERR("JumpTo with store_index enabled found without a command next to it. Error command found at end of file.\n"));
        this->parse_error=true;
    }
    if(this->cmd_arr->cmds[check_i--].subtype==CMDST_Query){//Check twice.
        fprintf(stderr,ERR("Queries should have at least 2 commands next to it. Error command found at end of file.\n"));
        this->parse_error=true;
    }else if(check_i>0&&this->cmd_arr->cmds[check_i].subtype==CMDST_Query){
        fprintf(stderr,ERR("Queries should have at least 2 commands next to it. Error command found at end of file.\n"));
        this->parse_error=true;
    }
    for(int rpn_i=0;rpn_i<vl->ssm->count;rpn_i++){
        const char* rpn_str;
        if(*(rpn_str=vl->ssm->c_strs[rpn_i])=='('){//Exclude variables without '()'.
            printf("Checking RPN string '%s'. ",rpn_str);
            RPNValidStringE status=RPNEvaluatorEvaluate(rpn_str,vl,&(as_number_t){0},false,false,RPN_EVAL_START_B,RPN_EVAL_END_B,RPN_EVAL_SEP);
            if(status!=RPNVS_Ok&&!(this->parse_error)){this->parse_error=true;}else{ puts("Valid string.");}
        }
    }
}
void macro_buffer_free(macro_buffer_t* this){
    repeat_id_manager_free(this->rim);
    jump_id_manager_free(this->jim);
    VL_free(this->vl);
    vp_array_free(this->vpa);
    free(this->contents);
    free(this);
}
repeat_id_manager_t* repeat_id_manager_new(void){
    repeat_id_manager_t* this=(malloc(sizeof(repeat_id_manager_t)));
    EXIT_IF_NULL(this,repeat_id_manager_t);
    *this=(repeat_id_manager_t){.size=0,.names=NULL,.index=NULL,.ssm=SSManager_new()};
    return this;
}
void repeat_id_manager_add_name(repeat_id_manager_t* this, char* str_owned, int index){
    this->size++;
    if(this->names){
        this->names=realloc(this->names,sizeof(char*)*(this->size));
        this->index=realloc(this->index,sizeof(int)*(this->size));
    }else{
        this->names=(malloc(sizeof(char*)));
        this->index=(malloc(sizeof(int)));
    }
    EXIT_IF_NULL(this->names,char**);
    EXIT_IF_NULL(this->index,int*);
    if(SSManager_add_string(this->ssm,&str_owned)){//Is unique
        this->names[this->size-1]=str_owned;
        this->index[this->size-1]=index;
        return;
    }
    SHOULD_BE_UNREACHABLE();
}
int repeat_id_manager_search_command_index(const repeat_id_manager_t* this,const char* search_str){
    for(int i=0;i<this->size;i++){
        if(!strcmp(search_str,this->names[i])){
            return this->index[i];
        }
    }
    return IndexNotFound;
}
int repeat_id_manager_search_string_index(const repeat_id_manager_t* this,const char* search_str){
    for(int i=0;i<this->size;i++){
        if(!strcmp(search_str,this->names[i])){
            return i;
        }
    }
    return IndexNotFound;
}
void repeat_id_manager_free(repeat_id_manager_t* this){
    SSManager_free(this->ssm);
    free(this->names);
    free(this->index);
    free(this);
}
jump_id_manager_t* jump_id_manager_new(void){
    jump_id_manager_t* this=(malloc(sizeof(jump_id_manager_t)));
    EXIT_IF_NULL(this,jump_id_manager_t);
    *this=(jump_id_manager_t){.size=0,.names=NULL,.index=NULL,.jump_from_added=NULL,.ssm=SSManager_new()};
    return this;
}
void jump_id_manager_add_name(jump_id_manager_t* this, char* str_owned, int index, bool is_jump_from){
    this->size++;
    if(this->names){
        this->names=realloc(this->names,sizeof(char*)*(this->size));
        this->index=realloc(this->index,sizeof(int)*(this->size));
        this->jump_from_added=realloc(this->jump_from_added,sizeof(bool)*(this->size));
    }else{
        this->names=(malloc(sizeof(char*)));
        this->index=(malloc(sizeof(int)));
        this->jump_from_added=(malloc(sizeof(bool)));
    }
    EXIT_IF_NULL(this->names,char**);
    EXIT_IF_NULL(this->index,int*);
    EXIT_IF_NULL(this->jump_from_added,bool*);
    if(SSManager_add_string(this->ssm,&str_owned)){//Is unique.
        this->names[this->size-1]=str_owned;
        this->index[this->size-1]=index;
        this->jump_from_added[this->size-1]=is_jump_from;
        return;
    }
    SHOULD_BE_UNREACHABLE();
}
int jump_id_manager_search_command_index(const jump_id_manager_t* this,const char* search_str){
    for(int i=0;i<this->size;i++){
        if(!strcmp(search_str,this->names[i])){
            return this->index[i];
        }
    }
    return IndexNotFound;
}
int jump_id_manager_search_string_index(const jump_id_manager_t* this,const char* search_str){//Searches index of string instead.
    for(int i=0;i<this->size;i++){
        if(!strcmp(search_str,this->names[i])){
            return i;
        }
    }
    return IndexNotFound;
}
bool jump_id_manager_set_command_index_once(jump_id_manager_t* this, int index, int cmd_index){
    if(this->jump_from_added[index]) return false;//If JumpFrom was already added before JumpTo
    this->jump_from_added[index]=true;
    this->index[index]=cmd_index;
    return true;
}
void jump_id_manager_free(jump_id_manager_t* this){
    SSManager_free(this->ssm);
    free(this->names);
    free(this->index);
    free(this->jump_from_added);
    free(this);
}
command_array_t* command_array_new(void){
    command_array_t* this=(malloc(sizeof(command_array_t)));
    EXIT_IF_NULL(this,command_array_t);
    *this=(command_array_t){.size=0,.cmds=NULL,.SSM=SSManager_new()};
    return this;
}
void command_array_add(command_array_t* this, command_t cmd){
    this->size++;
    if(this->cmds) this->cmds=realloc(this->cmds,sizeof(command_t)*(this->size));
    else this->cmds=malloc(sizeof(command_t));
    EXIT_IF_NULL(this->cmds,command_t*);
    switch(cmd.type){//Edit pointer for any shared strings and take ownership.
        case CMD_KeyStroke: SSManager_add_string(this->SSM,(char**)&cmd.cmd_u.auto_ks.key); break;
        case CMD_WaitUntilKey: SSManager_add_string(this->SSM,(char**)&cmd.cmd_u.wait_until_key.key); break;
        case CMD_QueryKeyPress: SSManager_add_string(this->SSM,(char**)&cmd.cmd_u.qkey_pressed.key); break;
        case CMD_GrabKey: SSManager_add_string(this->SSM,(char**)&cmd.cmd_u.grab_key.key); break;
        case CMD_UngrabKey: SSManager_add_string(this->SSM,(char**)&cmd.cmd_u.ungrab_key.key); break;
        case CMD_PrintString: SSManager_add_string(this->SSM,(char**)&cmd.cmd_u.print_string.str); break;
        default: break;
    }
    this->cmds[this->size-1]=cmd;
}
int command_array_count(const command_array_t* this){
    return this->size;
}
void command_array_print(const command_array_t* this,const VariableLoader_t* vl,unsigned char decimals){
    vlcallback_t* vlct;
    for(int i=0;i<this->size;i++){
        const command_union_t cmd=this->cmds[i].cmd_u;
        printf("Command Index: %d ",i);
        char* cmd_str=char_string_slice(this->cmds[i].start_cmd_p,this->cmds[i].end_cmd_p);
        printf("\tCommand String: '%s'\n\tCommandType: ",cmd_str);
        free(cmd_str);
        switch(this->cmds[i].type){
            case CMD_KeyStroke:
                printf("Key '%s' KeyState: %u\n",cmd.auto_ks.key,cmd.auto_ks.key_state);
                break;
            case CMD_Delay:
                vlct=VL_get_callback(vl,cmd.delay.callback);
                printf("Delay ");
                switch(vlct->callback_type){
                    case VLCallback_Long:
                        printf("using value %lu ",vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("using RPN '%s' ",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                printf("with multiplier of %ld\n",cmd.delay.delay_mult);
                break;
            case CMD_RepeatStart:
                printf("RepeatStart Counter: %d str_i: %d\n",cmd.repeat_start.counter,cmd.repeat_start.str_index);
                break;
            case CMD_RepeatEnd:
                printf("RepeatEnd RepeatAtIndex: %d str_i: %d ",cmd.repeat_end.cmd_index,cmd.repeat_start.str_index);
                vlct=VL_get_callback(vl,cmd.repeat_end.counter);
                switch(vlct->callback_type){
                    case VLCallback_Int:
                        printf("MaxCounter: %d\n",(int)vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("MaxCounter(RPN): '%s'\n",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                break;
            case CMD_RepeatResetCounters:
                puts("RepeatResetCounters");
                break;
            case CMD_MouseClick:
                printf("MouseClick MouseType: %d MouseState: %u\n",cmd.mouse_click.mouse_type,cmd.mouse_click.mouse_state);
                break;
            case CMD_MoveMouse:
                printf("MoveMouse is_absolute: %d ",cmd.mouse_move.is_absolute);
                vlct=VL_get_callback(vl,cmd.mouse_move.x_cb);
                switch(vlct->callback_type){
                    case VLCallback_Int:
                        printf("x: %d ",(int)vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("x(RPN): '%s' ",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                vlct=VL_get_callback(vl,cmd.mouse_move.y_cb);
                switch(vlct->callback_type){
                    case VLCallback_Int:
                        printf("y: %d\n",(int)vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("y(RPN): '%s'\n",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                break;
            case CMD_Exit:
                puts("ExitProgram");
                break;
            case CMD_Pass:
                puts("Pass");
                break;
            case CMD_UngrabKeyAll:
                puts("UngrabKeyAll");
                break;
            case CMD_UngrabButtonAll:
                puts("UngrabButtonAll");
                break;
            case CMD_JumpTo:
                printf("JumpTo cmd_i: %d str_i: %d store_i: %d\n",cmd.jump_to.cmd_index,cmd.jump_to.str_index,cmd.jump_to.store_index);
                break;
            case CMD_JumpToIndex:
                printf("JumpToIndex(%s) value",cmd.jump_to_index.is_absolute?"absolute":"relative");
                vlct=VL_get_callback(vl,cmd.jump_to_index.jump_cb);
                switch(vlct->callback_type){
                    case VLCallback_Int:
                        printf(": %d\n",(int)vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("(RPN): '%s'\n",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                break;
            case CMD_JumpFrom:
                printf("JumpFrom str_i: %d\n",cmd.jump_from.str_index);
                break;
            case CMD_JumpBack:
                puts("JumpBack");
                break;
            case CMD_SaveMouseCoords:
                puts("SaveMouseCoords");
                break;
            case CMD_LoadMouseCoords:
                puts("LoadMouseCoords");
                break;
            case CMD_QueryComparePixel:
                printf("QueryComparePixel ");
                printf("%s",this->cmds[i].query_details.invert?"(inverted) ":"");
                vlct=VL_get_callback(vl,cmd.pixel_compare.r_cb);
                switch(vlct->callback_type){
                    case VLCallback_Char:
                        printf("r: %d ",(unsigned char)vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("r(RPN): '%s' ",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                vlct=VL_get_callback(vl,cmd.pixel_compare.g_cb);
                switch(vlct->callback_type){
                    case VLCallback_Char:
                        printf("g: %d ",(unsigned char)vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("g(RPN): '%s' ",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                vlct=VL_get_callback(vl,cmd.pixel_compare.b_cb);
                switch(vlct->callback_type){
                    case VLCallback_Char:
                        printf("b: %d ",(unsigned char)vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("b(RPN): '%s' ",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                vlct=VL_get_callback(vl,cmd.pixel_compare.thr_cb);
                switch(vlct->callback_type){
                    case VLCallback_Char:
                        printf("threshold: %d\n",(unsigned char)vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("threshold(RPN): '%s'\n",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                break;
            case CMD_QueryCompareCoords:
                printf("QueryCompareCoords %scmp_flags: '%c,%c%s' var"
                    ,this->cmds[i].query_details.invert?"(inverted) ":""
                    ,(cmd.compare_coords.cmp_flags&CMP_Y)==CMP_Y?'y':'x'
                    ,(cmd.compare_coords.cmp_flags&CMP_GT)==CMP_GT?'>':'<'
                    ,(cmd.compare_coords.cmp_flags&CMP_W_EQ)==CMP_W_EQ?",=":""
                );
                vlct=VL_get_callback(vl,cmd.compare_coords.var_callback);
                switch(vlct->callback_type){
                    case VLCallback_Int:
                        printf(": %d\n",(int)vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("(RPN): '%s'\n",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                break;
            case CMD_QueryCoordsWithin:
                printf("QuerryCoordsWithin ");
                printf("%s",this->cmds[i].query_details.invert?"(inverted) ":"");
                vlct=VL_get_callback(vl,cmd.coords_within.xl_cb);
                switch(vlct->callback_type){
                    case VLCallback_Int:
                        printf("xl: %d ",(int)vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("xl(RPN): '%s' ",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                vlct=VL_get_callback(vl,cmd.coords_within.yl_cb);
                switch(vlct->callback_type){
                    case VLCallback_Int:
                        printf("yl: %d ",(int)vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("yl(RPN): '%s' ",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                vlct=VL_get_callback(vl,cmd.coords_within.xh_cb);
                switch(vlct->callback_type){
                    case VLCallback_Int:
                        printf("xh: %d ",(int)vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("xh(RPN): '%s' ",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                vlct=VL_get_callback(vl,cmd.coords_within.yh_cb);
                switch(vlct->callback_type){
                    case VLCallback_Int:
                        printf("yh: %d\n",(int)vlct->args.number);
                        break;
                    case VLCallback_NumberRPN:
                        printf("yh(RPN): '%s'\n",vlct->args.an_rpn.rpn_str);
                        break;
                    default: SHOULD_BE_UNREACHABLE(); break;
                }
                break;
            case CMD_QueryRPNEval:
                printf("QueryRPNEval String: '%s'\n",VL_get_callback(vl,cmd.rpn_eval)->args.an_rpn.rpn_str);
                break;
            case CMD_QueryKeyPress:
                printf("QueryKeyPress String: '%s'\n",cmd.qkey_pressed.key);
                break;
            case CMD_QueryButtonPress:
                printf("QueryButtonPress Button: %d\n",cmd.qbutton_pressed.button);
                break;
            case CMD_InitVar:
                printf("InitVar Name: '%s' Type: '%s' Value: '",cmd.init_var.variable,VLNumberTypeStr(cmd.init_var.as_number.type));
                VLNumberPrintNumber(cmd.init_var.as_number,decimals);
                puts("'");
                break;
            case CMD_EditVar:
                printf("EditVar Name: '%s' RPN String: '%s'\n",VL_get_callback(vl,cmd.edit_var)->args.rpn.variable,VL_get_callback(vl,cmd.edit_var)->args.rpn.rpn_str);
                break;
            case CMD_WaitUntilKey:
                printf("WaitUntilKey: '%s' KeySym: %lu Invert: %d\n",cmd.wait_until_key.key,cmd.wait_until_key.keysym,cmd.wait_until_key.invert_press);
                break;
            case CMD_WaitUntilButton:
                printf("WaitUntilButton: '%d' HeldDown: %d Invert: %d\n",cmd.wait_until_button.button,cmd.wait_until_button.held_down,cmd.wait_until_button.invert_press);
                break;
            case CMD_GrabKey:
                printf("GrabKey: '%s' KeySym: '%lu'\n",cmd.grab_key.key,cmd.grab_key.keysym);
                break;
            case CMD_UngrabKey:
                printf("UngrabKey: '%s' KeySym: '%lu'\n",cmd.ungrab_key.key,cmd.ungrab_key.keysym);
                break;
            case CMD_GrabButton:
                printf("GrabButton: %d \n",cmd.grab_button.button);
                break;
            case CMD_UngrabButton:
                printf("UngrabButton: %d \n",cmd.ungrab_button.button);
                break;
            case CMD_PrintString:
                printf("PrintString '%s' NewLine: %d",cmd.print_string.str,cmd.print_string.newline);
                if(cmd.print_string.rpn_strs_len){
                    printf(" RPN Strings: ");
                    for(int i=0;i<cmd.print_string.rpn_strs_len;i++){
                        printf("'%s'%s",cmd.print_string.rpn_strs[i],i!=cmd.print_string.rpn_strs_len?", ":"");
                    }
                }
                putchar('\n');
                break;
            case CMD_DebugConfig:
                printf("DebugConfig Type: '%s' Value: %lu\n"
                    ,DebugConfigTypeString[cmd.debug_config.type],cmd.debug_config.value);
                break;
        }
    }
}
void command_array_free(command_array_t* this){
    SSManager_free(this->SSM);
    free(this->cmds);
    free(this);
}
vp_array_t* vp_array_new(void){
    vp_array_t* this=malloc(sizeof(vp_array_t));
    EXIT_IF_NULL(this,vp_array_t*);
    *this=(vp_array_t){.vp_arr=malloc(0),.size=0};
    EXIT_IF_NULL(this->vp_arr,void**);
    return this;
}
void vp_array_add(vp_array_t* this,void* p){
    this->vp_arr=realloc(this->vp_arr,sizeof(void*)*(++this->size));
    EXIT_IF_NULL(this->vp_arr,void**);
    this->vp_arr[this->size-1]=p;
}
void vp_array_free(vp_array_t* this){
    for(int i=0;i<this->size;i++) free(this->vp_arr[i]);
    free(this->vp_arr);
    free(this);
}
