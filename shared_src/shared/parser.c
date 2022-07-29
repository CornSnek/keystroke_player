#include "parser.h"
#include "macros.h"
#include <string.h>
#include <ctype.h>
__ReadStateWithStringDef(__ReadStateEnums)
const int IndexNotFound=-1;
const int JumpFromNotConnected=-2;
int trim_whitespace(char** strptr){//For null-terminated strings only, and reedit pointer to resize for trimmed strings. Returns int to get total length of the trimmed string.
    int str_i=0;
    int whitespace_count=0;
    do{
        if(char_is_whitespace((*strptr)[str_i])) whitespace_count++;
        else (*strptr)[str_i-whitespace_count]=(*strptr)[str_i];//Copy the next non-whitespace character.
    }while((*strptr)[++str_i]);
    (*strptr)[str_i-whitespace_count]='\0';//Null terminate last character and reallocate as whitespace-trimmed string.
    *strptr=(char*)realloc(*strptr,(str_i-whitespace_count+1)*sizeof(char));
    EXIT_IF_NULL(*strptr,char*);
    return str_i-whitespace_count;
}

void replace_str(char** strptr_owner, const char* replace, const char* with){//Assume all null-terminated.
    char* new_strptr=(char*)(malloc(sizeof(char)*1));
    EXIT_IF_NULL(new_strptr,char*)
    int strptr_i=0;
    int new_strptr_i=0;
    const int replace_len=strlen(replace);
    const int with_len=strlen(with);
    char current_char;
    while((current_char=(*strptr_owner)[strptr_i])){
        if(current_char==*replace&&!strncmp((*strptr_owner+strptr_i),replace,replace_len)){//0 in strncmp for same string contents.
            new_strptr=(char*)(realloc(new_strptr,sizeof(char)*(new_strptr_i+1+with_len)));
            EXIT_IF_NULL(new_strptr,char*)
            for(int with_i=0;with_i<with_len;with_i++){
                new_strptr[new_strptr_i++]=with[with_i];
            }
            strptr_i+=replace_len;
            continue;
        }
        new_strptr[new_strptr_i++]=current_char;
        strptr_i++;
        new_strptr=(char*)(realloc(new_strptr,sizeof(char)*new_strptr_i+1));
        EXIT_IF_NULL(new_strptr,char*)
    }
    new_strptr[new_strptr_i]='\0';//Null-terminate.
    free(*strptr_owner);
    *strptr_owner=new_strptr;//Change freed pointer to new pointer.
}
macro_buffer_t* macro_buffer_new(char* str_owned, command_array_t* cmd_arr){
    macro_buffer_t* this=(macro_buffer_t*)(malloc(sizeof(macro_buffer_t)));
    EXIT_IF_NULL(this,macro_buffer_t);
    *this=(macro_buffer_t){.token_i=0,.line_num=1,.char_num=1,.str_size=strlen(str_owned),.contents=str_owned,.cmd_arr=cmd_arr,.rim=repeat_id_manager_new(),.jim=jump_id_manager_new(),.parse_error=false};
    return this;
}
void print_where_error_is(const char* contents,int begin_error,int end_error){
    char* str_to_print=(char*)malloc(sizeof(char)*(end_error+2)); //+2 to count a character and for '\0'.
    strncpy(str_to_print,contents+begin_error,end_error+1);
    str_to_print[end_error+1]='\0';
    printf("%s < Command where error occured.\n",str_to_print);
    free(str_to_print);
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
    CompareCoords cmp_flags=CMP_NULL;
    bool mouse_absolute;
    do{
        const char current_char=this->contents[this->token_i+read_i+read_offset_i];
        if(print_debug) printf("'%c' token_i:%d read_offset_i:%d read_i:%d State:%s\n",current_char,this->token_i,read_offset_i,read_i,ReadStateStrings[read_state]);
        switch(read_state){
            case RS_Start:
                if(!strncmp(this->contents+this->token_i+read_i+read_offset_i,"exit;",5)){
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_Exit,.is_query=is_query,
                            .cmd_u={{0}}
                        }
                    );
                    read_i+=4;//Exclude reading "xit;"
                    key_processed=true;
                    break;
                }
                if(!strncmp(this->contents+this->token_i+read_i+read_offset_i,"JT<",3)){
                    read_i+=3;
                    read_offset_i=-1;
                    read_state=RS_JumpTo;
                    break;
                }
                if(!strncmp(this->contents+this->token_i+read_i+read_offset_i,"JF>",3)){
                    read_i+=3;
                    read_offset_i=-1;
                    read_state=RS_JumpFrom;
                    break;
                }
                if(!strncmp(this->contents+this->token_i+read_i+read_offset_i,"mma=",4)){
                    read_i+=4;
                    read_offset_i=-1;
                    mouse_absolute=true;
                    read_state=RS_MouseMove;
                    break;
                }
                if(!strncmp(this->contents+this->token_i+read_i+read_offset_i,"mmr=",4)){
                    read_i+=4;
                    read_offset_i=-1;
                    mouse_absolute=false;
                    read_state=RS_MouseMove;
                    break;
                }
                if(char_is_key(current_char)){
                    if(current_char=='m'&&isdigit(this->contents[this->token_i+read_i+read_offset_i+1])){
                        read_i++;
                        read_offset_i=-1;
                        read_state=RS_MouseClickType;
                        break;
                    }
                    read_state=RS_KeyOrMouse;
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
                    case '\0':
                        key_processed=true;
                        break;
                    case '\n':
                        read_i++;
                        read_offset_i=-1;
                        this->line_num++;
                        this->char_num=0;//1 after loop repeats.
                        break;
                    case '#':
                        read_state=RS_Comments;
                        break;
                    case ' '://Fallthrough
                    case '\t'://Allow tabs and spaces before making comments.
                        read_i++;
                        read_offset_i=-1;
                        break;
                    case '?':
                        read_i++;
                        read_offset_i=-1;
                        read_state=RS_Query;
                        break;
                    default:
                        fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                        print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                        this->parse_error=true;
                        key_processed=true;
                        break;
                }
                break;
            case RS_Comments:
                if(current_char=='\n'){
                    read_i+=read_offset_i;
                    read_offset_i=-1;
                    this->line_num++;
                    this->char_num=0;//1 after loop repeats.
                    read_state=RS_Start;
                }else if(current_char=='\0') key_processed=true;
                break; //No need for errors here.
            case RS_RepeatStart:
                if(char_is_key(current_char)) break;
                if(current_char==';'){
                    str_name=(char*)calloc(read_offset_i+1,sizeof(char));
                    EXIT_IF_NULL(str_name,char);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    repeat_id_manager_add_name(this->rim,str_name,command_array_count(this->cmd_arr));
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_RepeatStart,.is_query=is_query,
                            .cmd_u.repeat_start=(repeat_start_t){
                                .counter=0,
                                .str_index=repeat_id_manager_search_string_index(this->rim,str_name)
                            }
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
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
                    fprintf(stderr,"String '%s' was not initially defined from a Loop Start at line %d char %d.\n",str_name,this->line_num,this->char_num);
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
                            (command_t){.type=CMD_RepeatEnd,.is_query=is_query,
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
                    fprintf(stderr,"String '%s' was not initially defined from a Loop Start at line %d char %d.\n",str_name,this->line_num,this->char_num);
                    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
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
                        (command_t){.type=CMD_RepeatEnd,.is_query=is_query,
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
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_KeyOrMouse:
                if(char_is_key(current_char)) break;
                else if(current_char=='='){
                    read_state=RS_KeyState;
                    added_keystate=false;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_KeyState:
                if(char_is_keystate(current_char)){
                    if(added_keystate){
                        fprintf(stderr,"Cannot add more than 1 keystate at line %d char %d.\n",this->line_num,this->char_num);
                        print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
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
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i-2);
                    str_name[read_offset_i-2]='\0';\
                    command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_KeyStroke,.is_query=is_query,
                            .cmd_u.ks=(keystroke_t){
                                .key=str_name,//SSManager/keystroke_t owns char* key via command_array_add.
                                .key_state=input_state
                            }
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
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
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
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
                        (command_t){.type=CMD_Delay,.is_query=is_query,
                            .cmd_u.delay=strtol(num_str,NULL,10)*delay_mult
                        }
                    );
                    free(num_str);
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_MouseClickType:
                if(isdigit(current_char)&&!first_number){
                    num_str=(char*)calloc(sizeof(char),2);
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
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_MouseClickState:
                if(char_is_keystate(current_char)){
                    if(added_keystate){
                        fprintf(stderr,"Cannot add more than 1 keystate at line %d char %d.\n",this->line_num,this->char_num);
                        print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
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
                        (command_t){.type=CMD_MouseClick,.is_query=is_query,
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
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_MouseMove:
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
                        (command_t){.type=CMD_MouseMove,.is_query=is_query,
                                .cmd_u.mouse_move=(mouse_move_t){
                                    .x=parsed_num[0],.y=parsed_num[1],.is_absolute=mouse_absolute
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }
                    fprintf(stderr,"2 numbers are needed (separated by comma) at line %d char %d.\n",this->line_num,this->char_num);
                    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_JumpTo:
                if(char_is_key(current_char)) break;
                if(current_char==';'){
                    str_name=(char*)malloc(sizeof(char)*read_offset_i+1);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    str_name[read_offset_i]='\0';
                    int jid_cmd_i=jump_id_manager_search_command_index(this->jim,str_name);
                    if(jid_cmd_i==-1){
                        jump_id_manager_add_name(this->jim,str_name,JumpFromNotConnected,false);//-2 Because it's not a JumpFrom
                        command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_JumpTo,.is_query=is_query,
                                .cmd_u.jump_to=(jump_to_t){
                                    .cmd_index=JumpFromNotConnected,//Will be edited from a CMD_JumpFrom later.
                                    .str_index=jump_id_manager_search_string_index(this->jim,str_name)
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }else{
                        command_array_add(this->cmd_arr,
                        (command_t){.type=CMD_JumpTo,.is_query=is_query,
                                .cmd_u.jump_to=(jump_to_t){
                                    .cmd_index=jid_cmd_i,
                                    .str_index=jump_id_manager_search_string_index(this->jim,str_name)
                                }
                            }
                        );
                        key_processed=true;
                        free(str_name);
                        break;
                    }
                    fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_JumpFrom:
                if(char_is_key(current_char)) break;
                if(current_char==';'){
                    str_name=(char*)malloc(sizeof(char)*read_offset_i+1);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_offset_i);
                    str_name[read_offset_i]='\0';
                    int jid_cmd_i=jump_id_manager_search_command_index(this->jim,str_name);
                    int jid_str_i=jump_id_manager_search_string_index(this->jim,str_name);
                    int cmd_arr_count=command_array_count(this->cmd_arr);
                    if(jid_cmd_i==-1){
                        jump_id_manager_add_name(this->jim,str_name,cmd_arr_count,true);
                        command_array_add(this->cmd_arr,
                            (command_t){.type=CMD_JumpFrom,.is_query=is_query,
                                .cmd_u.jump_from=(jump_from_t){
                                    .str_index=jump_id_manager_search_string_index(this->jim,str_name)
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }else{
                        bool unique=jump_id_manager_set_command_index_once(this->jim,jid_str_i,cmd_arr_count);
                        if(unique){//No Second RS_JumpFrom
                            command_array_add(this->cmd_arr,
                                (command_t){.type=CMD_JumpFrom,.is_query=is_query,
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
                            key_processed=true;
                            break;
                        }
                        fprintf(stderr,"Cannot add a second JumpFrom with string '%s' at line %d char %d state %s.\n",str_name,this->line_num,this->char_num,ReadStateStrings[read_state]);
                        free(str_name);
                        print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                        this->parse_error=true;
                        key_processed=true;
                        break;
                    }
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_Query:
                is_query=true;
                if(!strncmp(this->contents+this->token_i+read_i+read_offset_i,"pxc=",4)){
                    read_i+=4;
                    read_offset_i=-1;
                    read_state=RS_QueryComparePixel;
                    break;
                }
                if(!strncmp(this->contents+this->token_i+read_i+read_offset_i,"coords:",7)){
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
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
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
                            fprintf(stderr,"Number should be between 0 and 255 at %d char %d state %s.\n",this->line_num,this->char_num,ReadStateStrings[read_state]);
                            print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                            this->parse_error=true;
                            key_processed=true;
                            break;
                        }
                        parsed_num_i++;
                        read_i+=read_offset_i+1;//Read other strings.
                        read_offset_i=-1;
                        break;
                    }
                    fprintf(stderr,"There should only be 4 numbers separated by 3 ',' at %d char %d state %s.\n",this->line_num,this->char_num,ReadStateStrings[read_state]);
                    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
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
                            fprintf(stderr,"Number should be between 0 and 255 at %d char %d state %s.\n",this->line_num,this->char_num,ReadStateStrings[read_state]);
                            print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                            this->parse_error=true;
                            key_processed=true;
                            break;
                        }
                        command_array_add(this->cmd_arr,
                            (command_t){.type=CMD_QueryComparePixel,.is_query=is_query,
                                .cmd_u.pixel_compare=(pixel_compare_t){
                                    .r=parsed_num[0],.g=parsed_num[1],.b=parsed_num[2],.thr=parsed_num[3]
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }
                    fprintf(stderr,"There should only be 4 numbers separated by 3 ',' at %d char %d state %s.\n",this->line_num,this->char_num,ReadStateStrings[read_state]);
                    print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
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
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
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
                        (command_t){.type=CMD_QueryCompareCoords,.is_query=is_query,
                            .cmd_u.compare_coords=(compare_coords_t){
                                .cmp_flags=cmp_flags,.var=parsed_num[0]
                            }
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d state %s.\n",current_char,this->line_num,this->char_num,ReadStateStrings[read_state]);
                print_where_error_is(this->contents,this->token_i,read_i+read_offset_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_Count://Nothing (Shouldn't be used).
                break;
        }
        read_offset_i++;
        this->char_num++;
    }while(!key_processed);
    this->token_i+=read_i+read_offset_i;
    return !this->parse_error;
}
void macro_buffer_str_id_check(macro_buffer_t* this){//Check if RepeatStart doesn't have a respective RepeatEnd, or if there are no JumpFroms.
    bool* id_check=calloc(this->rim->size,sizeof(bool));
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
    const command_t is_jf_cmd=this->cmd_arr->cmds[check_i];
    if(is_jf_cmd.type==CMD_JumpFrom){
        fprintf(stderr,"JumpFrom found without command next to it. Error command found at end of file.\n");
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
        switch(this->cmds[i].type){
            case CMD_KeyStroke:
                printf("(%d) Key: %s KeyState: %d\n",i,cmd.ks.key,cmd.ks.key_state);
                break;
            case CMD_Delay:
                printf("(%d) Delay: %lu\n",i,cmd.delay);
                break;
            case CMD_RepeatStart:
                printf("(%d) RepeatStart: Counter: %d str_i: %d\n",i,cmd.repeat_start.counter,cmd.repeat_start.str_index);
                break;
            case CMD_RepeatEnd:
                printf("(%d) RepeatEnd: RepeatAtIndex: %d MaxCounter: %d str_i: %d\n",i,cmd.repeat_end.cmd_index,cmd.repeat_end.counter_max,cmd.repeat_start.str_index);
                break;
            case CMD_MouseClick:
                printf("(%d) MouseClick: MouseType: %d MouseState: %d\n",i,cmd.mouse_click.mouse_type,cmd.mouse_click.mouse_state);
                break;
            case CMD_MouseMove:
                printf("(%d) MouseMove: x: %d y: %d is_absolute: %d\n",i,cmd.mouse_move.x,cmd.mouse_move.y,cmd.mouse_move.is_absolute);
                break;
            case CMD_Exit:
                printf("(%d) ExitProgram\n",i);
                break;
            case CMD_JumpTo:
                printf("(%d) JumpTo cmd_i: %d str_i: %d\n",i,cmd.jump_to.cmd_index,cmd.jump_to.str_index);
                break;
            case CMD_JumpFrom:
                printf("(%d) JumpFrom str_i: %d\n",i,cmd.jump_from.str_index);
                break;
            case CMD_QueryComparePixel:
                printf("(%d) QueryComparePixel r: %d g: %d b: %d threshold: %d\n",i,cmd.pixel_compare.r,cmd.pixel_compare.g,cmd.pixel_compare.b,cmd.pixel_compare.thr);
                break;
            case CMD_QueryCompareCoords:
                ;const CompareCoords cc=cmd.compare_coords.cmp_flags;
                printf("(%d) QueryCompareCoords cmp_flags: '%c,%c%s' var:%d\n",i,(cc&CMP_Y)==CMP_Y?'y':'x',(cc&CMP_GT)==CMP_GT?'>':'<',(cc&CMP_W_EQ)==CMP_W_EQ?",=":"",cmd.compare_coords.var);
                break;
        }
    }
}
void command_array_free(command_array_t* this){
    SSManager_free(this->SSM);
    free(this->cmds);
    free(this);
}
bool char_is_key(char c){
    return isalnum(c)||(c=='_')||(c=='+');
}
bool char_is_keystate(char c){
    return (c=='u')||(c=='d')||(c=='c')||(c=='U')||(c=='D')||(c=='C');
}
bool char_is_whitespace(char c){
    return (c==' ')||(c=='\t')||(c=='\n')||(c=='\v')||(c=='\f')||(c=='\r');
}
bool char_is_delay(char c){
    return (c=='m')||(c=='u')||(c=='s')||(c=='M')||(c=='U')||(c=='S');
}
