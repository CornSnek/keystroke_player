#include "parser.h"
#include "macros.h"
#include <string.h>
#include <ctype.h>
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

void replace_str(char**  strptr_owner, const char* replace, const char* with){//Assume all null-terminated.
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
macro_buffer_t* macro_buffer_new(char*  str_owned, shared_string_manager* ssm, command_array_t* cmd_arr, repeat_id_manager_t* rim){
    macro_buffer_t* this=(macro_buffer_t*)(malloc(sizeof(macro_buffer_t)));
    EXIT_IF_NULL(this,macro_buffer_t);
    *this=(macro_buffer_t){.token_i=0,.line_num=1,.char_num=1,.size=strlen(str_owned),.contents=str_owned,.ssm=ssm,.cmd_arr=cmd_arr,.rim=rim,.parse_error=false};
    return this;
}
void print_where_error_is(const char* contents,int begin_error,int error_offset){
    char* str_to_print=(char*)malloc(sizeof(char)*(error_offset+2)); //+1 to count a character and +1 for '\0'.
    strncpy(str_to_print,contents+begin_error,error_offset+1);
    str_to_print[error_offset+1]='\0';
    printf("%s < Command where error occured.\n",str_to_print);
    free(str_to_print);
}
bool macro_buffer_process_next(macro_buffer_t* this){//Returns bool if processed successfully or not.
    ReadState read_state=RS_Start;
    bool key_processed=false;
    InputState input_state;
    char* str_name=0;
    __uint64_t delay_mult; 
    char* num_str=(char*)malloc(sizeof(char)*2);
    EXIT_IF_NULL(num_str,char*);
    __uint64_t parsed_num=0;
    __uint64_t parsed_num2=0;
    bool added_keystate=false;
    int read_i=0; //Index to read.
    int read_end_i=0; //Last character to read.
    bool maybe_mouse=false;
    bool mm_first_number=false;
    do{
        const char current_char=this->contents[this->token_i+read_i+read_end_i];
        printf("'%c' token_i:%d read_end_i:%d read_i:%d State:%d Length:%d\n",current_char,this->token_i,read_end_i,read_i,(int)read_state,this->size);
        switch(read_state){
            case RS_Start:
                if(char_is_key(current_char)){
                    if(current_char=='m'||current_char=='M'){maybe_mouse=true;}
                    read_state=RS_KeyOrMouse;
                    break;
                }
                switch(current_char){
                    case '.':
                        read_i+=1;
                        read_end_i=-1;
                        read_state=RS_Delay;
                        break;
                    case '(':
                        read_i+=1;
                        read_end_i=-1;
                        read_state=RS_RepeatStart;
                        break;
                    case ')':
                        read_i+=1;
                        read_end_i=-1;
                        read_state=RS_RepeatEnd;
                        break;
                    case '\0':
                        key_processed=true;
                        break;
                    case '\n':
                        read_i+=1;
                        read_end_i=-1;
                        this->line_num++;
                        this->char_num=0;//1 after loop repeats.
                        break;
                    case '#':
                        read_state=RS_Comments;
                        break;
                    case ' '://Fallthrough
                    case '\t'://Allow tabs and spaces before making comments.
                        read_i+=1;
                        read_end_i=-1;
                        break;
                    default:
                        fprintf(stderr,"Current character not allowed '%c' at line %d char %d.\n",current_char,this->line_num,this->char_num);
                        print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
                        this->parse_error=true;
                        key_processed=true;
                        break;
                }
                break;
            case RS_Comments:
                if(current_char=='\n'){
                    read_i+=read_end_i;
                    read_end_i=-1;
                    this->line_num++;
                    this->char_num=0;//1 after loop repeats.
                    read_state=RS_Start;
                }else if(current_char=='\0') key_processed=true;
                break; //No need for errors here.
            case RS_RepeatStart:
                if(char_is_key(current_char)) break;
                if(current_char==';'){
                    str_name=(char*)calloc(read_end_i+1,sizeof(char));
                    EXIT_IF_NULL(str_name,char);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_end_i);
                    repeat_id_manager_add_name(this->rim,str_name,command_array_count(this->cmd_arr));
                    command_array_add(this->cmd_arr,
                        (command_t){.type=VT_RepeatStart,
                            .cmd.repeat_start=0
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d.\n",current_char,this->line_num,this->char_num);
                print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_RepeatEnd:
                if(char_is_key(current_char)) break;
                if(current_char=='='){
                    str_name=(char*)calloc(read_end_i+1,sizeof(char));
                    EXIT_IF_NULL(str_name,char);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_end_i);
                    const bool str_exists=(str_name!=SSManager_add_string(this->ssm,&str_name));
                    if(str_exists){
                        read_i+=read_end_i+1;
                        read_end_i=-1;
                        read_state=RS_RepeatEndNumber;
                        break;
                    }
                    fprintf(stderr,"String '%s' was not initially defined from a Loop Start at line %d char %d.\n",str_name,this->line_num,this->char_num);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                if(current_char==';'){
                    str_name=(char*)calloc(read_end_i+1,sizeof(char));
                    EXIT_IF_NULL(str_name,char);
                    strncpy(str_name,this->contents+this->token_i+read_i,read_end_i);
                    const bool str_exists=(str_name!=SSManager_add_string(this->ssm,&str_name));
                    if(str_exists){
                        command_array_add(this->cmd_arr,
                            (command_t){.type=VT_RepeatEnd,
                                .cmd.repeat_end=(repeat_end_t){
                                    .index=repeat_id_manager_search_index(this->rim,str_name),
                                    .counter_max=0
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }
                    fprintf(stderr,"String '%s' was not initially defined from a Loop Start at line %d char %d.\n",str_name,this->line_num,this->char_num);
                    print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d.\n",current_char,this->line_num,this->char_num);
                print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_RepeatEndNumber:
                if(isdigit(current_char)){
                    num_str[read_end_i]=current_char;
                    num_str=(char*)realloc(num_str,sizeof(char)*(read_end_i+2));
                    EXIT_IF_NULL(num_str,char*);
                    break;
                }else if(current_char==';'){
                    num_str[read_end_i]='\0';
                    parsed_num=strtol(num_str,NULL,10);
                    command_array_add(this->cmd_arr,
                        (command_t){.type=VT_RepeatEnd,
                            .cmd.repeat_end=(repeat_end_t){
                                .index=repeat_id_manager_search_index(this->rim,str_name),
                                .counter_max=parsed_num
                            }
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d.\n",current_char,this->line_num,this->char_num);
                print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_KeyOrMouse:
                if(maybe_mouse){
                    if(isdigit(current_char)){
                        read_i+=read_end_i;//No +1 to reread digit.
                        read_end_i=-1;
                        read_state=RS_MouseType;
                        break;
                    }else if(true){
                        read_i+=read_end_i+1;
                        read_end_i=-1;
                        read_state=RS_MouseMove;
                        break;
                    }
                }
                if(char_is_key(current_char)) break;
                else if(current_char=='='){
                    read_state=RS_KeyState;
                    added_keystate=false;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d.\n",current_char,this->line_num,this->char_num);
                print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_KeyState:
                if(char_is_keystate(current_char)){
                    if(added_keystate){
                        fprintf(stderr,"Cannot add more than 1 keystate at line %d char %d.\n",this->line_num,this->char_num);
                        print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
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
                }else if(current_char==';'){
                    str_name=malloc(sizeof(char)*read_end_i-1);//-2 to exclude RS_KeyState modifiers, but -1 because null terminator.
                    strncpy(str_name,this->contents+this->token_i+read_i,read_end_i-2);
                    str_name[read_end_i-2]='\0';
                    SSManager_add_string(this->ssm,&str_name);
                    command_array_add(this->cmd_arr,
                        (command_t){.type=VT_KeyStroke,
                            .cmd.ks=(keystroke_t){
                                .key=str_name,//SSManager/keystroke_t owns char* key.
                                .key_state=input_state
                            }
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d.\n",current_char,this->line_num,this->char_num);
                print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
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
                    read_i+=1;
                    read_end_i=-1;
                    read_state=RS_DelayNum;
                    break;
                }else if(isdigit(current_char)){
                    delay_mult=1;//Default microseconds.
                    read_state=RS_DelayNum;
                    num_str[read_end_i]=current_char;
                    num_str=(char*)realloc(num_str,sizeof(char)*(read_end_i+2));
                    EXIT_IF_NULL(num_str,char*);
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d.\n",current_char,this->line_num,this->char_num);
                print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_DelayNum:
                if(isdigit(current_char)){
                    num_str[read_end_i]=current_char;
                    num_str=(char*)realloc(num_str,sizeof(char)*(read_end_i+2));
                    EXIT_IF_NULL(num_str,char*);
                    break;
                }else if(current_char==';'){
                    num_str[read_end_i]='\0';
                    parsed_num=strtol(num_str,NULL,10)*delay_mult;
                    if(parsed_num) command_array_add(this->cmd_arr,
                        (command_t){.type=VT_Delay,
                            .cmd.delay=parsed_num
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d.\n",current_char,this->line_num,this->char_num);
                print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_MouseType:
                if(isdigit(current_char)){
                    num_str[0]=current_char;
                    num_str[1]='\0';
                    break;
                }else if(current_char=='='){
                    read_i+=2;//To read numbers.
                    read_end_i=-1;
                    read_state=RS_MouseState;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d.\n",current_char,this->line_num,this->char_num);
                print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_MouseState:
                if(char_is_keystate(current_char)){
                    if(added_keystate){
                        fprintf(stderr,"Cannot add more than 1 keystate at line %d char %d.\n",this->line_num,this->char_num);
                        print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
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
                    command_array_add(this->cmd_arr,
                        (command_t){.type=VT_MouseClick,
                            .cmd.mouse_click=(mouse_click_t){.mouse_state=input_state,
                                .mouse_type=strtol(num_str,NULL,10)
                            }
                        }
                    );
                    break;
                }else if(current_char==';'){
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d.\n",current_char,this->line_num,this->char_num);
                print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_MouseMove:
                if(isdigit(current_char)) break;
                else if(current_char==','&&!mm_first_number){
                    num_str=realloc(num_str,sizeof(char)*read_end_i+1);
                    EXIT_IF_NULL(num_str,char*)
                    strncpy(num_str,this->contents+this->token_i+read_i,read_end_i);
                    num_str[read_end_i]='\0';
                    parsed_num=strtol(num_str,NULL,10);
                    read_i+=read_end_i+1;//Read second string.
                    read_end_i=-1;
                    mm_first_number=true;
                    break;
                }else if(current_char==';'){
                    if(mm_first_number){
                        num_str=realloc(num_str,sizeof(char)*read_end_i+1);
                        EXIT_IF_NULL(num_str,char*)
                        strncpy(num_str,this->contents+this->token_i+read_i,read_end_i);
                        num_str[read_end_i]='\0';
                        parsed_num2=strtol(num_str,NULL,10);
                        command_array_add(this->cmd_arr,
                        (command_t){.type=VT_MouseMove,
                                .cmd.mouse_move=(mouse_move_t){
                                    .x=parsed_num,.y=parsed_num2
                                }
                            }
                        );
                        key_processed=true;
                        break;
                    }
                    fprintf(stderr,"2 numbers are needed (separated by comma) at line %d char %d.\n",this->line_num,this->char_num);
                    print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"Current character not allowed '%c' at line %d char %d.\n",current_char,this->line_num,this->char_num);
                print_where_error_is(this->contents,this->token_i,read_i+read_end_i);
                this->parse_error=true;
                key_processed=true;
                break;
        }
        read_end_i++;
        this->char_num++;
    }while(!key_processed);
    this->token_i+=read_i+read_end_i;
    free(num_str);//Free any arrays from parsing.
    return !this->parse_error;
}
void macro_buffer_free(macro_buffer_t* this){
    free(this->contents);
    free(this);
}
keystroke_t* keystroke_new(bool key_state, char* key_owned){//keystroke_t owns key string.
    keystroke_t* this=(keystroke_t*)(malloc(sizeof(keystroke_t)));
    EXIT_IF_NULL(this,macro_buffer_t);
    *this=(keystroke_t){.key_state=key_state,.key=key_owned};
    return this;
}
void keystroke_free(keystroke_t* this){
    free(this->key);
    free(this);
}
repeat_id_manager_t* repeat_id_manager_new(shared_string_manager* ssm){
    repeat_id_manager_t* this=(repeat_id_manager_t*)(malloc(sizeof(repeat_id_manager_t)));
    EXIT_IF_NULL(this,repeat_id_manager_t);
    *this=(repeat_id_manager_t){.size=0,.names=NULL,.index=NULL,.ssm=ssm};
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
int repeat_id_manager_search_index(const repeat_id_manager_t* this,const char* search_str){
    for(int i=0;i<this->size;i++){
        if(strcmp(search_str,this->names[i])==0){
            return this->index[i];
        }
    }
    return -1;
}
void repeat_id_manager_free(repeat_id_manager_t* this){
    free(this->names);
    free(this->index);
    free(this);
}
command_array_t* command_array_new(shared_string_manager* ssm){
    command_array_t* this=(command_array_t*)(malloc(sizeof(command_array_t)));
    EXIT_IF_NULL(this,command_array_t);
    *this=(command_array_t){.size=0,.cmds=NULL,.SSM=ssm};
    return this;
}
void command_array_add(command_array_t* this, command_t cmd_arr){
    this->size++;
    if(this->cmds) this->cmds=(command_t*)realloc(this->cmds,sizeof(command_t)*(this->size));
    else this->cmds=(command_t*)malloc(sizeof(command_t));
    EXIT_IF_NULL(this->cmds,command_t*);
    this->cmds[this->size-1]=cmd_arr;
}
int command_array_count(const command_array_t* this){
    return this->size;
}
void command_array_print(const command_array_t* this){
    for(int i=0;i<this->size;i++){
        const command_union_t cmd=this->cmds[i].cmd;
        switch(this->cmds[i].type){
            case VT_KeyStroke:
                printf("(%d) Key: %s KeyState: %d\n",i,cmd.ks.key,cmd.ks.key_state);
                break;
            case VT_Delay:
                printf("(%d) Delay: %lu\n",i,cmd.delay);
                break;
            case VT_RepeatStart:
                printf("(%d) RepeatStart: Counter: %d\n",i,cmd.repeat_start);
                break;
            case VT_RepeatEnd:
                printf("(%d) RepeatEnd: RepeatAtIndex: %d MaxCounter: %d\n",i,cmd.repeat_end.index,cmd.repeat_end.counter_max);
                break;
            case VT_MouseClick:
                printf("(%d) MouseClick: MouseType: %d MouseState: %d\n",i,cmd.mouse_click.mouse_type,cmd.mouse_click.mouse_state);
                break;
            case VT_MouseMove:
                printf("(%d) MouseMove: x: %d y: %d\n",i,cmd.mouse_move.x,cmd.mouse_move.y);
                break;
        }
    }
}
void command_array_free(command_array_t* this){
    for(int i=0;i<this->size;i++){
        const command_union_t cmd=this->cmds[i].cmd;
        switch(this->cmds[i].type){
            case VT_KeyStroke:
                SSManager_free_string(this->SSM,cmd.ks.key);
                break;
            default:
                break;
        }
    }
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
