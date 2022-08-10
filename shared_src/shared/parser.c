#include "parser.h"
#include "macros.h"
#include <string.h>
#include <ctype.h>
__ReadStateWithStringDef(__ReadStateEnums)
const int IndexNotFound=-1;
const int JumpFromNotConnected=-2;
macro_buffer_t* macro_buffer_new(char* str_owned, command_array_t* cmd_arr){
    macro_buffer_t* this=(macro_buffer_t*)(malloc(sizeof(macro_buffer_t)));
    EXIT_IF_NULL(this,macro_buffer_t);
    *this=(macro_buffer_t){.token_i=0,.str_size=strlen(str_owned),.contents=str_owned,.cmd_arr=cmd_arr,.rim=repeat_id_manager_new(),.jim=jump_id_manager_new(),.parse_error=false};
    return this;
}
void print_where_error_is(const char* contents,int begin_error,int end_error){
    char* str_to_print=(char*)malloc(sizeof(char)*(end_error+2)); //+2 to count a character and for '\0'.
    EXIT_IF_NULL(str_to_print,char*)
    strncpy(str_to_print,contents+begin_error,end_error+1);
    str_to_print[end_error+1]='\0';
    printf("%s < Command where error occured.\n",str_to_print);
    free(str_to_print);
}
int error_move_offset(const char* begin_error_p){//Moves the pointer to the end to help process multiple errors.
    const char* end_p1=strchr(begin_error_p,';'),* end_p2=strchr(begin_error_p,'\0');
    return ((end_p1<end_p2)?end_p1:end_p2)-begin_error_p;
}
bool macro_buffer_process_next(macro_buffer_t* this,bool print_debug){//Returns bool if processed successfully or not.
    ReadState read_state=RS_Start;
    bool key_processed=false;
    InputState input_state=IS_Down;
    char* str_name=0;
    __uint64_t delay_mult=0; 
    char* num_str=0;
    __uint64_t parsed_num[4]={0};
    int parsed_num_i=0;
    bool added_keystate=false;
    int read_i=0; //Index to read.
    int read_offset_i=0; //Last character to read by offset of read_i.
    bool first_number=false;
    bool is_query=false;
    bool print_cmd=false;
    bool store_index=false;
    CompareCoords cmp_flags=CMP_NULL;
    bool mouse_absolute;
    char* start_p=this->contents+this->token_i;
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
                if(!strncmp(current_char_p,"exit;",5)){
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_Exit,.is_query=is_query,.print_cmd=print_cmd,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=4; //Exclude reading "exit;" The +1 for ';' is from read_offset_i++
                    key_processed=true;
                    break;
                }
                if(!strncmp(current_char_p,"pass;",5)){//Just like exit, but does nothing.
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_Pass,.is_query=is_query,.print_cmd=print_cmd,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=4;
                    key_processed=true;
                    break;
                }
                if(!strncmp(current_char_p,"save_mma;",9)){
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_SaveMouseCoords,.is_query=is_query,.print_cmd=print_cmd,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=8;
                    key_processed=true;
                    break;
                }
                if(!strncmp(current_char_p,"load_mma;",9)){
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_LoadMouseCoords,.is_query=is_query,.print_cmd=print_cmd,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=8;
                    key_processed=true;
                    break;
                }
                if(!strncmp(current_char_p,"rep_reset;",10)){
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_RepeatResetCounters,.is_query=is_query,.print_cmd=print_cmd,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=9;
                    key_processed=true;
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
                        (command_t){.type=CMD_JumpBack,.is_query=is_query,.print_cmd=print_cmd,
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
                    mouse_absolute=true;
                    read_state=RS_MoveMouse;
                    break;
                }
                if(!strncmp(current_char_p,"mmr=",4)){
                    read_i+=4;
                    read_offset_i=-1;
                    mouse_absolute=false;
                    read_state=RS_MoveMouse;
                    break;
                }
                if(char_is_key(current_char)){
                    if(current_char=='m'&&isdigit(this->contents[this->token_i+read_i+read_offset_i+1])){
                        read_i++;
                        read_offset_i=-1;
                        read_state=RS_MouseClickType;
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
                        start_p++;
                        read_i++;
                        read_offset_i=-1;
                        line_num++;
                        char_num=0;//1 after loop repeats.
                        break;
                    case '#':
                        start_p++;
                        read_state=RS_Comments;
                        break;
                    case ' '://Fallthrough
                    case '\t'://Allow tabs and spaces before making comments.
                        start_p++;
                        read_i++;
                        read_offset_i=-1;
                        break;
                    case '?':
                        read_i++;
                        read_offset_i=-1;
                        read_state=RS_Query;
                        break;
                    default:
                        fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                        print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                        this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                        this->parse_error=true;
                        key_processed=true;
                        break;
                }
                break;
            case RS_Comments:
                start_p++;
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
                    str_name=(char*)calloc(read_offset_i+1,sizeof(char));
                    EXIT_IF_NULL(str_name,char);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    repeat_id_manager_add_name(this->rim,str_name,command_array_count(this->cmd_arr));
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_RepeatStart,.is_query=is_query,.print_cmd=print_cmd,
                            .cmd_u.repeat_start=(repeat_start_t){
                                .counter=0,
                                .str_index=repeat_id_manager_search_string_index(this->rim,str_name)
                            }
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_RepeatEnd:
                if(char_is_key(current_char)) break;
                if(current_char=='='){
                    str_name=(char*)calloc(read_offset_i+1,sizeof(char));
                    EXIT_IF_NULL(str_name,char);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    const bool str_exists=(str_name!=SSManager_add_string(this->rim->ssm,&str_name));
                    if(str_exists){
                        read_i+=read_offset_i+1;
                        read_offset_i=-1;
                        read_state=RS_RepeatEndNumber;
                        break;
                    }
                    fprintf(stderr,"String '%s' was not initially defined from a Loop Start at line %lu char %lu.\n",str_name,line_num,char_num);
                    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                    this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                if(current_char==';'){
                    str_name=(char*)calloc(read_offset_i+1,sizeof(char));
                    EXIT_IF_NULL(str_name,char);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    const bool str_exists=(str_name!=SSManager_add_string(this->rim->ssm,&str_name));
                    if(str_exists){
                        command_array_add(this->cmd_arr,
                            (command_t){.type=CMD_RepeatEnd,.is_query=is_query,.print_cmd=print_cmd,
                                .cmd_u.repeat_end=(repeat_end_t){
                                    .cmd_index=repeat_id_manager_search_command_index(this->rim,str_name),
                                    .str_index=repeat_id_manager_search_string_index(this->rim,str_name),
                                    .counter_max=0
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }
                    fprintf(stderr,"String '%s' was not initially defined from a Loop Start at line %lu char %lu.\n",str_name,line_num,char_num);
                    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                    this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_RepeatEndNumber:
                if(isdigit(current_char)) break;
                else if(current_char==';'){
                    num_str=(char*)malloc(sizeof(char)*(read_offset_i+1));
                    EXIT_IF_NULL(num_str,char*);
                    strncpy(num_str,this->contents+this->token_i+read_i,read_offset_i);
                    num_str[read_offset_i]='\0';
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_RepeatEnd,.is_query=is_query,.print_cmd=print_cmd,
                            .cmd_u.repeat_end=(repeat_end_t){
                                .cmd_index=repeat_id_manager_search_command_index(this->rim,str_name),
                                .str_index=repeat_id_manager_search_string_index(this->rim,str_name),
                                .counter_max=strtol(num_str,NULL,10)
                            }
                        }
                    );
                    free(num_str);
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_Keys:
                if(char_is_x11_key(current_char)) break;
                else if(current_char=='='){
                    read_state=RS_KeyState;
                    added_keystate=false;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_KeyState:
                if(char_is_keystate(current_char)){
                    if(added_keystate){
                        fprintf(stderr,"Cannot add more than 1 keystate at line %lu char %lu.\n",line_num,char_num);
                        print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                        this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                        this->parse_error=true;
                        key_processed=true;
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
                    str_name=malloc(sizeof(char)*read_offset_i-1);//-2 to exclude RS_KeyState modifiers, but -1 because null terminator.
                    EXIT_IF_NULL(str_name,char*)
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i-2);
                    str_name[read_offset_i-2]='\0';\
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_KeyStroke,.is_query=is_query,.print_cmd=print_cmd,
                            .cmd_u.ks=(keystroke_t){
                                .key=str_name,//SSManager/keystroke_t owns char* key via command_array_add.
                                .key_state=input_state
                            }
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
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
                    read_state=RS_DelayNum;
                    break;
                }else if(isdigit(current_char)){
                    delay_mult=1;//Default microseconds.
                    read_state=RS_DelayNum;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_DelayNum:
                if(isdigit(current_char)) break;
                else if(current_char==';'){
                    num_str=(char*)malloc(sizeof(char)*(read_offset_i+1));
                    EXIT_IF_NULL(num_str,char*);
                    strncpy(num_str,this->contents+this->token_i+read_i,read_offset_i);
                    num_str[read_offset_i]='\0';
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_Delay,.is_query=is_query,.print_cmd=print_cmd,
                            .cmd_u.delay=strtol(num_str,NULL,10)*delay_mult
                        }
                    );
                    free(num_str);
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_MouseClickType:
                if(isdigit(current_char)&&!first_number){
                    num_str=(char*)calloc(sizeof(char),2);
                    EXIT_IF_NULL(num_str,char*)
                    num_str[0]=current_char;
                    parsed_num[0]=strtol(num_str,NULL,10);
                    free(num_str);
                    first_number=true;
                    break;
                }else if(current_char=='='&&first_number){
                    read_i+=2;//To read numbers.
                    read_offset_i=-1;
                    read_state=RS_MouseClickState;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_MouseClickState:
                if(char_is_keystate(current_char)){
                    if(added_keystate){
                        fprintf(stderr,"Cannot add more than 1 keystate at line %lu char %lu.\n",line_num,char_num);
                        print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                        this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                        this->parse_error=true;
                        key_processed=true;
                        break;
                    }
                    switch(current_char){
                        case 'D': case 'd': input_state=IS_Down; break;
                        case 'U': case 'u': input_state=IS_Up; break;
                        default: input_state=IS_Click;
                    }
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_MouseClick,.is_query=is_query,.print_cmd=print_cmd,
                            .cmd_u.mouse_click=(mouse_click_t){.mouse_state=input_state,
                                .mouse_type=parsed_num[0]
                            }
                        }
                    );
                    added_keystate=true;
                    break;
                }else if(added_keystate&&current_char==';'){
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_MoveMouse:
                if(isdigit(current_char)||current_char=='-') break;
                else if(current_char==','&&!first_number){
                    num_str=(char*)malloc(sizeof(char)*read_offset_i+1);
                    EXIT_IF_NULL(num_str,char*)
                    strncpy(num_str,this->contents+this->token_i+read_i,read_offset_i);
                    num_str[read_offset_i]='\0';
                    parsed_num[0]=strtol(num_str,NULL,10);
                    free(num_str);
                    read_i+=read_offset_i+1;//Read second string.
                    read_offset_i=-1;
                    first_number=true;
                    break;
                }else if(current_char==';'){
                    if(first_number){
                        num_str=(char*)malloc(sizeof(char)*read_offset_i+1);
                        EXIT_IF_NULL(num_str,char*)
                        strncpy(num_str,this->contents+this->token_i+read_i,read_offset_i);
                        num_str[read_offset_i]='\0';
                        parsed_num[1]=strtol(num_str,NULL,10);
                        free(num_str);
                        command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_MoveMouse,.is_query=is_query,.print_cmd=print_cmd,
                                .cmd_u.mouse_move=(mouse_move_t){
                                    .x=parsed_num[0],.y=parsed_num[1],.is_absolute=mouse_absolute
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }
                    fprintf(stderr,"2 numbers are needed (separated by comma) at line %lu char %lu.\n",line_num,char_num);
                    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                    this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_JumpTo:
                if(char_is_key(current_char)) break;
                if(current_char==';'){
                    str_name=(char*)malloc(sizeof(char)*read_offset_i+1);
                    EXIT_IF_NULL(str_name,char*)
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    str_name[read_offset_i]='\0';
                    int jid_cmd_i=jump_id_manager_search_command_index(this->jim,str_name);
                    if(jid_cmd_i==-1){
                        jump_id_manager_add_name(this->jim,str_name,JumpFromNotConnected,false);//-2 Because it's not a JumpFrom
                        command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_JumpTo,.is_query=is_query,.print_cmd=print_cmd,
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
                        (command_t){.type=CMD_JumpTo,.is_query=is_query,.print_cmd=print_cmd,
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
                    fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                    this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_JumpFrom:
                if(char_is_key(current_char)) break;
                if(current_char==';'){
                    str_name=(char*)malloc(sizeof(char)*read_offset_i+1);
                    EXIT_IF_NULL(str_name,char*)
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    str_name[read_offset_i]='\0';
                    int jid_cmd_i=jump_id_manager_search_command_index(this->jim,str_name);
                    int jid_str_i=jump_id_manager_search_string_index(this->jim,str_name);
                    int cmd_arr_count=command_array_count(this->cmd_arr);
                    if(jid_cmd_i==-1){
                        jump_id_manager_add_name(this->jim,str_name,cmd_arr_count,true);
                        command_array_add(this->cmd_arr,
                            (command_t){.type=CMD_JumpFrom,.is_query=is_query,.print_cmd=print_cmd,
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
                                (command_t){.type=CMD_JumpFrom,.is_query=is_query,.print_cmd=print_cmd,
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
                        fprintf(stderr,"Cannot add a second JumpFrom with string '%s' at line %lu char %lu state %s.\n",str_name,line_num,char_num,ReadStateStrings[read_state]);
                        free(str_name);
                        print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                        this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                        this->parse_error=true;
                        key_processed=true;
                        break;
                    }
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_Query:
                is_query=true;
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
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_QueryComparePixel:
                if(isdigit(current_char)) break;
                if(current_char==','){
                    if(parsed_num_i<3){
                        num_str=(char*)malloc(sizeof(char)*read_offset_i+1);
                        EXIT_IF_NULL(num_str,char*)
                        strncpy(num_str,this->contents+this->token_i+read_i,read_offset_i);
                        num_str[read_offset_i]='\0';
                        parsed_num[parsed_num_i]=strtol(num_str,NULL,10);
                        free(num_str);
                        if(parsed_num[parsed_num_i]>255){
                            fprintf(stderr,"Number should be between 0 and 255 at line %lu char %lu state %s.\n",line_num,char_num,ReadStateStrings[read_state]);
                            print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                            this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                            this->parse_error=true;
                            key_processed=true;
                            break;
                        }
                        parsed_num_i++;
                        read_i+=read_offset_i+1;//Read other strings.
                        read_offset_i=-1;
                        break;
                    }
                    fprintf(stderr,"There should only be 4 numbers separated by 3 ',' at line %lu char %lu state %s.\n",line_num,char_num,ReadStateStrings[read_state]);
                    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                    this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                if(current_char=='?'){
                    if(parsed_num_i==3){
                        num_str=(char*)malloc(sizeof(char)*read_offset_i+1);
                        EXIT_IF_NULL(num_str,char*)
                        strncpy(num_str,this->contents+this->token_i+read_i,read_offset_i);
                        num_str[read_offset_i]='\0';
                        parsed_num[parsed_num_i]=strtol(num_str,NULL,10);
                        free(num_str);
                        if(parsed_num[parsed_num_i]>255){
                            fprintf(stderr,"Number should be between 0 and 255 at line %lu char %lu state %s.\n",line_num,char_num,ReadStateStrings[read_state]);
                            print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                            this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                            this->parse_error=true;
                            key_processed=true;
                            break;
                        }
                        command_array_add(this->cmd_arr,
                            (command_t){.type=CMD_QueryComparePixel,.is_query=is_query,.print_cmd=print_cmd,
                                .cmd_u.pixel_compare=(pixel_compare_t){
                                    .r=parsed_num[0],.g=parsed_num[1],.b=parsed_num[2],.thr=parsed_num[3]
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }
                    fprintf(stderr,"There should only be 4 numbers separated by 3 ',' at line %lu char %lu state %s.\n",line_num,char_num,ReadStateStrings[read_state]);
                    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                    this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
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
                    read_state=RS_QueryCoordsVar;
                    break;
                }
                if(current_char=='<'){
                    cmp_flags|=CMP_LT;
                    if(this->contents[this->token_i+read_i+read_offset_i+1]=='='){
                        read_i++; cmp_flags|=CMP_W_EQ;
                    }
                    read_i++;
                    read_offset_i=-1;
                    read_state=RS_QueryCoordsVar;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_QueryCoordsVar:
                if(isdigit(current_char)) break;
                if(current_char=='?'){
                    num_str=(char*)malloc(sizeof(char)*read_offset_i+1);
                    EXIT_IF_NULL(num_str,char*)
                    strncpy(num_str,this->contents+this->token_i+read_i,read_offset_i);
                    num_str[read_offset_i]='\0';
                    parsed_num[0]=strtol(num_str,NULL,10);
                    free(num_str);
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_QueryCompareCoords,.is_query=is_query,.print_cmd=print_cmd,
                            .cmd_u.compare_coords=(compare_coords_t){
                                .cmp_flags=cmp_flags,.var=parsed_num[0]
                            }
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_QueryCoordsWithin:
                if(isdigit(current_char)) break;
                if(current_char==','){
                    if(parsed_num_i<3){
                        num_str=(char*)malloc(sizeof(char)*read_offset_i+1);
                        EXIT_IF_NULL(num_str,char*)
                        strncpy(num_str,this->contents+this->token_i+read_i,read_offset_i);
                        num_str[read_offset_i]='\0';
                        parsed_num[parsed_num_i]=strtol(num_str,NULL,10);
                        free(num_str);
                        parsed_num_i++;
                        read_i+=read_offset_i+1;//Read other strings.
                        read_offset_i=-1;
                        break;
                    }
                    fprintf(stderr,"There should only be 4 numbers separated by 3 ',' at line %lu char %lu state %s.\n",line_num,char_num,ReadStateStrings[read_state]);
                    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                    this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                if(current_char=='?'){
                    if(parsed_num_i==3){
                        num_str=(char*)malloc(sizeof(char)*read_offset_i+1);
                        EXIT_IF_NULL(num_str,char*)
                        strncpy(num_str,this->contents+this->token_i+read_i,read_offset_i);
                        num_str[read_offset_i]='\0';
                        parsed_num[parsed_num_i]=strtol(num_str,NULL,10);
                        free(num_str);
                        command_array_add(this->cmd_arr,
                            (command_t){.type=CMD_QueryCoordsWithin,.is_query=is_query,.print_cmd=print_cmd,
                                .cmd_u.coords_within=(coords_within_t){
                                    .xl=parsed_num[0],.yl=parsed_num[1],.xh=parsed_num[2],.yh=parsed_num[3]
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }
                    fprintf(stderr,"There should only be 4 numbers separated by 3 ',' at line %lu char %lu state %s.\n",line_num,char_num,ReadStateStrings[read_state]);
                    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                    this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Unexpected character '%c' at line %lu char %lu state %s.\n",current_char,line_num,char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->token_i+=error_move_offset(this->contents+this->token_i+read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_Count://Nothing (Shouldn't be used).
                break;
        }
        read_offset_i++;
    }while(!key_processed);
    this->token_i+=read_i+read_offset_i;
    char* end_p=this->contents+this->token_i-1;
    if(!this->parse_error){
        this->cmd_arr->cmds[this->cmd_arr->size-1].start_cmd_p=start_p;
        this->cmd_arr->cmds[this->cmd_arr->size-1].end_cmd_p=end_p;
    }
    return !this->parse_error;
}
void macro_buffer_str_id_check(macro_buffer_t* this){//Check if RepeatStart doesn't have a respective RepeatEnd, or if there are no JumpFroms.
    bool* id_check=calloc(this->rim->size,sizeof(bool));
    EXIT_IF_NULL(id_check,bool*)
    for(int i=0;i<this->rim->size;i++){
        for(int j=0;j<this->cmd_arr->size;j++){
            const command_t cmd=this->cmd_arr->cmds[j];
            if(cmd.type==CMD_RepeatEnd&&cmd.cmd_u.repeat_end.str_index==i){id_check[i]=true;}
        }
    }
    for(int i=0;i<this->rim->size;i++){
        if(!id_check[i]){
            fprintf(stderr,"RepeatEnd command missing for string '%s'\n",this->rim->names[i]);
            this->parse_error=true;
        }
    }
    free(id_check);
    id_check=calloc(this->jim->size,sizeof(bool));
    EXIT_IF_NULL(id_check,bool*)
    for(int i=0;i<this->jim->size;i++){
        for(int j=0;j<this->cmd_arr->size;j++){
            const command_t cmd=this->cmd_arr->cmds[j];
            if(cmd.type==CMD_JumpFrom&&cmd.cmd_u.jump_from.str_index==i){id_check[i]=true;}
        }
    }
    for(int i=0;i<this->jim->size;i++){
        if(!id_check[i]){
            fprintf(stderr,"JumpFrom command missing for string '%s'\n",this->jim->names[i]);
            this->parse_error=true;
        }
    }
    free(id_check);
    int check_i=this->cmd_arr->size-1;
    const command_t any_cmd=this->cmd_arr->cmds[check_i];
    if(any_cmd.type==CMD_JumpFrom){
        fprintf(stderr,"JumpFrom found without a command next to it. Error command found at end of file.\n");
        this->parse_error=true;
    }
    if(any_cmd.type==CMD_JumpTo&&any_cmd.cmd_u.jump_to.store_index){
        fprintf(stderr,"JumpTo with store_index enabled found without a command next to it. Error command found at end of file.\n");
        this->parse_error=true;
    }
    if(this->cmd_arr->cmds[check_i--].is_query){//Check twice.
        fprintf(stderr,"Queries should have at least 2 commands next to it. Error command found at end of file.\n");
        this->parse_error=true;
    }else if(this->cmd_arr->cmds[check_i].is_query){
        fprintf(stderr,"Queries should have at least 2 commands next to it. Error command found at end of file.\n");
        this->parse_error=true;
    }
}
void macro_buffer_free(macro_buffer_t* this){
    repeat_id_manager_free(this->rim);
    jump_id_manager_free(this->jim);
    free(this->contents);
    free(this);
}
repeat_id_manager_t* repeat_id_manager_new(void){
    repeat_id_manager_t* this=(repeat_id_manager_t*)(malloc(sizeof(repeat_id_manager_t)));
    EXIT_IF_NULL(this,repeat_id_manager_t);
    *this=(repeat_id_manager_t){.size=0,.names=NULL,.index=NULL,.ssm=SSManager_new()};
    return this;
}
void repeat_id_manager_add_name(repeat_id_manager_t* this, char* str_owned, int index){
    this->size++;
    if(this->names){
        this->names=(char**)realloc(this->names,sizeof(char*)*(this->size));
        this->index=(int*)realloc(this->index,sizeof(int)*(this->size));
    }else{
        this->names=(char**)(malloc(sizeof(char*)));
        this->index=(int*)(malloc(sizeof(int)));
    }
    EXIT_IF_NULL(this->names,char**);
    EXIT_IF_NULL(this->index,int*);
    bool is_unique=(str_owned==SSManager_add_string(this->ssm,&str_owned));
    if(is_unique){
        this->names[this->size-1]=str_owned;
        this->index[this->size-1]=index;
        return;
    }
    fprintf(stderr,"Repeat name '%s' has been used more than once.\n",str_owned);
    exit(EXIT_FAILURE);
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
    jump_id_manager_t* this=(jump_id_manager_t*)(malloc(sizeof(jump_id_manager_t)));
    EXIT_IF_NULL(this,jump_id_manager_t);
    *this=(jump_id_manager_t){.size=0,.names=NULL,.index=NULL,.jump_from_added=NULL,.ssm=SSManager_new()};
    return this;
}
void jump_id_manager_add_name(jump_id_manager_t* this, char* str_owned, int index, bool is_jump_from){
    this->size++;
    if(this->names){
        this->names=(char**)realloc(this->names,sizeof(char*)*(this->size));
        this->index=(int*)realloc(this->index,sizeof(int)*(this->size));
        this->jump_from_added=(bool*)realloc(this->jump_from_added,sizeof(bool)*(this->size));
    }else{
        this->names=(char**)(malloc(sizeof(char*)));
        this->index=(int*)(malloc(sizeof(int)));
        this->jump_from_added=(bool*)(malloc(sizeof(bool)));
    }
    EXIT_IF_NULL(this->names,char**)
    EXIT_IF_NULL(this->index,int*)
    EXIT_IF_NULL(this->jump_from_added,bool*)
    bool is_unique=(str_owned==SSManager_add_string(this->ssm,&str_owned));
    if(is_unique){
        this->names[this->size-1]=str_owned;
        this->index[this->size-1]=index;
        this->jump_from_added[this->size-1]=is_jump_from;
        return;
    }
    fprintf(stderr,"Jump name '%s' has been used more than once.\n",str_owned);
    exit(EXIT_FAILURE);
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
    command_array_t* this=(command_array_t*)(malloc(sizeof(command_array_t)));
    EXIT_IF_NULL(this,command_array_t);
    *this=(command_array_t){.size=0,.cmds=NULL,.SSM=SSManager_new()};
    return this;
}
void command_array_add(command_array_t* this, command_t cmd){
    this->size++;
    if(this->cmds) this->cmds=(command_t*)realloc(this->cmds,sizeof(command_t)*(this->size));
    else this->cmds=(command_t*)malloc(sizeof(command_t));
    EXIT_IF_NULL(this->cmds,command_t*);
    if(cmd.type==CMD_KeyStroke) SSManager_add_string(this->SSM,&cmd.cmd_u.ks.key);//Edit pointer for any shared strings first before placing in array.
    this->cmds[this->size-1]=cmd;
}
int command_array_count(const command_array_t* this){
    return this->size;
}
void command_array_print(const command_array_t* this){
    for(int i=0;i<this->size;i++){
        const command_union_t cmd=this->cmds[i].cmd_u;
        printf("Command Index: %d ",i);
        char* cmd_str=char_string_slice(this->cmds[i].start_cmd_p,this->cmds[i].end_cmd_p);
        printf("\tCommand String: '%s'\n\tCommandType: ",cmd_str);
        free(cmd_str);
        switch(this->cmds[i].type){
            case CMD_KeyStroke:
                printf("Key %s KeyState: %d\n",cmd.ks.key,cmd.ks.key_state);
                break;
            case CMD_Delay:
                printf("Delay %lu\n",cmd.delay);
                break;
            case CMD_RepeatStart:
                printf("RepeatStart Counter: %d str_i: %d\n",cmd.repeat_start.counter,cmd.repeat_start.str_index);
                break;
            case CMD_RepeatEnd:
                printf("RepeatEnd RepeatAtIndex: %d MaxCounter: %d str_i: %d\n",cmd.repeat_end.cmd_index,cmd.repeat_end.counter_max,cmd.repeat_start.str_index);
                break;
            case CMD_RepeatResetCounters:
                puts("RepeatResetCounters");
                break;
            case CMD_MouseClick:
                printf("MouseClick MouseType: %d MouseState: %d\n",cmd.mouse_click.mouse_type,cmd.mouse_click.mouse_state);
                break;
            case CMD_MoveMouse:
                printf("MoveMouse x: %d y: %d is_absolute: %d\n",cmd.mouse_move.x,cmd.mouse_move.y,cmd.mouse_move.is_absolute);
                break;
            case CMD_Exit:
                puts("ExitProgram");
                break;
            case CMD_Pass:
                puts("Pass");
                break;
            case CMD_JumpTo:
                printf("JumpTo cmd_i: %d str_i: %d store_i: %d\n",cmd.jump_to.cmd_index,cmd.jump_to.str_index,cmd.jump_to.store_index);
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
                printf("QueryComparePixel r: %d g: %d b: %d threshold: %d\n",cmd.pixel_compare.r,cmd.pixel_compare.g,cmd.pixel_compare.b,cmd.pixel_compare.thr);
                break;
            case CMD_QueryCompareCoords:
                ;const CompareCoords cc=cmd.compare_coords.cmp_flags;
                printf("QueryCompareCoords cmp_flags: '%c,%c%s' var:%d\n",(cc&CMP_Y)==CMP_Y?'y':'x',(cc&CMP_GT)==CMP_GT?'>':'<',(cc&CMP_W_EQ)==CMP_W_EQ?",=":"",cmd.compare_coords.var);
                break;
            case CMD_QueryCoordsWithin:
                printf("QueryCoordsWithin xl: %d yl: %d xh: %d yh: %d\n",cmd.coords_within.xl,cmd.coords_within.yl,cmd.coords_within.xh,cmd.coords_within.yh);
                break;
        }
    }
}
void command_array_free(command_array_t* this){
    SSManager_free(this->SSM);
    free(this->cmds);
    free(this);
}
