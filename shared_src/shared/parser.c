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
    return str_i-whitespace_count+1;
}

void replace_str(char** p_owner strptr_owner, const char* replace, const char* with){//Assume all null-terminated.
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
macro_buffer_t* macro_buffer_new(char* p_owner str_owned, shared_string_manager* ssm, command_array_t* cmd_arr, repeat_id_manager_t* rim){
    macro_buffer_t* this=(macro_buffer_t*)(malloc(sizeof(macro_buffer_t)));
    EXIT_IF_NULL(this,macro_buffer_t);
    int size=trim_whitespace(&str_owned);
    *this=(macro_buffer_t){.size=size,.parse_i=0,.contents=str_owned,.ssm=ssm,.cmd_arr=cmd_arr,.rim=rim,.parse_error=false};
    return this;
}
void add_read_token(macro_buffer_t* this,char** read_tokens,int* num_key_tokens,int* new_token_i,int* parse_i_offset){
    read_tokens[(*num_key_tokens)]=(char*)calloc((*parse_i_offset)-1,sizeof(char));//calloc to add null characters or 0. -1 to exclude RS_KeyState modifiers '|u', '|d'... etc.
    EXIT_IF_NULL(read_tokens[(*num_key_tokens)],char);
    strncpy(read_tokens[(*num_key_tokens)],this->contents+this->parse_i+(*new_token_i),(*parse_i_offset)-2); //-2 to exclude RS_KeyState modifiers.
    (*new_token_i)+=(*parse_i_offset)+1;//+1 to exclude reading comma or period.
    (*parse_i_offset)=-1;//To reset to 0 because ++ is added at end of loop.
}
bool macro_buffer_process_next(macro_buffer_t* this){//Returns bool if processed successfully or not.
    ReadState read_state=RS_Start;
    bool key_processed=false;
    char* repeat_name_str=NULL;
    char** read_tokens=(char**)malloc(sizeof(char*));
    EXIT_IF_NULL(read_tokens,char*);
    bool* keystate_tokens=(bool*)malloc(sizeof(bool));
    EXIT_IF_NULL(keystate_tokens,bool*);
    __uint64_t delay_mult; 
    char* num_str=(char*)malloc(sizeof(char)*2);
    EXIT_IF_NULL(num_str,char*);
    __uint64_t parsed_num=0;
    bool added_keystate=false;
    int num_key_tokens=0;
    int new_token_i=0; //To exclude reading certain non-alphanumeric characters
    int parse_i_offset=0;
    bool maybe_mouse=false;
    do{
        const char current_char=this->contents[this->parse_i+new_token_i+parse_i_offset];
        printf("'%c' %d %d %d %d\n",current_char,this->parse_i,parse_i_offset,new_token_i,(int)read_state);
        switch(read_state){
            case RS_Start:
                if(char_is_key(current_char)){
                    if(current_char=='m'||current_char=='M'){maybe_mouse=true;}
                    read_state=RS_KeyOrMouse;
                    break;
                }else if(current_char=='('){
                    new_token_i+=1;
                    parse_i_offset=-1;
                    read_state=RS_RepeatStart;
                    break;
                }else if(current_char==')'){
                    new_token_i+=1;
                    parse_i_offset=-1;
                    read_state=RS_RepeatEnd;
                    break;
                }else if(current_char=='\0'){
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"In state RS_Start, current character not allowed '%c'.\n",current_char);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_RepeatStart:
                if(char_is_key(current_char)) break;
                if(current_char==';'){
                    repeat_name_str=(char*)calloc(parse_i_offset+1,sizeof(char));
                    EXIT_IF_NULL(repeat_name_str,char);
                    strncpy(repeat_name_str,this->contents+this->parse_i+new_token_i,parse_i_offset);
                    repeat_id_add_name(this->rim,repeat_name_str,command_array_count(this->cmd_arr));
                    command_array_add(this->cmd_arr,
                        (command_t){.type=VT_RepeatStart,
                            .cmd.repeat_start=(repeat_start_t){
                                .counter=0
                            }
                        }
                    );
                    new_token_i+=parse_i_offset+1;
                    parse_i_offset=-1;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"In state RS_RepeatStart, current character not allowed '%c'.\n",current_char);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_RepeatEnd:
                if(char_is_key(current_char)) break;
                if(current_char=='='){
                    repeat_name_str=(char*)calloc(parse_i_offset+1,sizeof(char));
                    EXIT_IF_NULL(repeat_name_str,char);
                    strncpy(repeat_name_str,this->contents+this->parse_i+new_token_i,parse_i_offset);
                    const bool str_exists=(repeat_name_str!=SSManager_add_string(this->ssm,&repeat_name_str));
                    if(str_exists){
                        new_token_i+=parse_i_offset+1;
                        parse_i_offset=-1;
                        read_state=RS_RepeatEndNumber;
                        break;
                    }
                    fprintf(stderr,"String '%s' was not initially defined in a RS_RepeatStart.\n",repeat_name_str);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                if(current_char==';'||current_char=='\0'){
                    repeat_name_str=(char*)calloc(parse_i_offset+1,sizeof(char));
                    EXIT_IF_NULL(repeat_name_str,char);
                    strncpy(repeat_name_str,this->contents+this->parse_i+new_token_i,parse_i_offset);
                    const bool str_exists=(repeat_name_str!=SSManager_add_string(this->ssm,&repeat_name_str));
                    if(str_exists){
                        command_array_add(this->cmd_arr,
                            (command_t){.type=VT_RepeatEnd,
                                .cmd.repeat_end=(repeat_end_t){
                                    .index=repeat_id_search_index(this->rim,repeat_name_str),
                                    .counter_max=0
                                }
                            }
                        );
                        new_token_i+=parse_i_offset+1;
                        parse_i_offset=-1;
                        key_processed=true;
                        break;
                    }
                    fprintf(stderr,"String '%s' was not initially defined in a RS_RepeatStart.\n",repeat_name_str);
                    this->parse_error=true;
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"In state RS_RepeatEnd, current character not allowed '%c'.\n",current_char);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_RepeatEndNumber:
                if(isdigit(current_char)){
                    num_str[parse_i_offset]=current_char;
                    num_str=(char*)realloc(num_str,sizeof(char)*(parse_i_offset+2));
                    EXIT_IF_NULL(num_str,char*);
                    break;
                }else if(current_char==';'||current_char=='\0'){
                    num_str[parse_i_offset]='\0';
                    parsed_num=strtol(num_str,NULL,10);
                    command_array_add(this->cmd_arr,
                        (command_t){.type=VT_RepeatEnd,
                            .cmd.repeat_end=(repeat_end_t){
                                .index=repeat_id_search_index(this->rim,repeat_name_str),
                                .counter_max=parsed_num
                            }
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"In state RS_RepeatEndNumber, current character not allowed '%c'.\n",current_char);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_KeyOrMouse:
                if(maybe_mouse&&isdigit(current_char)){
                        new_token_i+=parse_i_offset;
                        parse_i_offset=-1;
                        read_state=RS_MouseType;
                        break;
                }
                if(char_is_key(current_char)) break;
                else if(current_char=='='){
                    read_state=RS_KeyState;
                    added_keystate=false;
                    break;
                }else if(current_char==','||current_char=='.'){
                    fprintf(stderr,"= should be added after key.\n");
                    exit(EXIT_FAILURE);
                }
                fprintf(stderr,"In state RS_KeyOrMouse, current character not allowed '%c'.\n",current_char);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_KeyState:
                if(char_is_keystate(current_char)){
                    if(added_keystate){
                        fprintf(stderr,"Cannot add more than 1 keystate.\n");
                        this->parse_error=true;
                        key_processed=true;
                        break;
                    }
                    if(current_char=='u'||current_char=='U') keystate_tokens[num_key_tokens]=false;
                    else keystate_tokens[num_key_tokens]=true;
                    added_keystate=true;
                    break;
                }else if(current_char==','){
                    add_read_token(this,read_tokens,&num_key_tokens,&new_token_i,&parse_i_offset);
                    read_tokens=(char**)realloc(read_tokens,sizeof(char*)*(++num_key_tokens+1));//+1 to expect another read_/keystate_ in RS_KeyOrMouse.
                    EXIT_IF_NULL(read_tokens,char*);
                    keystate_tokens=(bool*)realloc(keystate_tokens,sizeof(bool)*(num_key_tokens+1));
                    EXIT_IF_NULL(keystate_tokens,bool*);
                    read_state=RS_KeyOrMouse;
                    break;
                }else if(current_char=='.'){
                    add_read_token(this,read_tokens,&num_key_tokens,&new_token_i,&parse_i_offset);
                    ++num_key_tokens;
                    read_state=RS_Delay;
                    break;
                }
                fprintf(stderr,"In state RS_KeyState, current character not allowed '%c'.\n",current_char);
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
                    new_token_i+=1;
                    parse_i_offset=-1;
                    read_state=RS_DelayNum;
                    break;
                }else if(isdigit(current_char)){
                    delay_mult=1;//Default microseconds.
                    read_state=RS_DelayNum;
                    num_str[parse_i_offset]=current_char;
                    num_str=(char*)realloc(num_str,sizeof(char)*(parse_i_offset+2));
                    EXIT_IF_NULL(num_str,char*);
                    break;
                }
                fprintf(stderr,"In state RS_Delay, current character not allowed '%c'.\n",current_char);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_DelayNum:
                if(isdigit(current_char)){
                    num_str[parse_i_offset]=current_char;
                    num_str=(char*)realloc(num_str,sizeof(char)*(parse_i_offset+2));
                    EXIT_IF_NULL(num_str,char*);
                    break;
                }else if(current_char==';'||current_char=='\0'){
                    num_str[parse_i_offset]='\0';
                    parsed_num=strtol(num_str,NULL,10)*delay_mult;
                    for(int i=0;i<num_key_tokens;i++){
                        SSManager_add_string(this->ssm,&read_tokens[i]);
                        command_array_add(this->cmd_arr,
                            (command_t){.type=VT_KeyStroke,
                                .cmd.ks=(keystroke_t){
                                    .key=read_tokens[i],//keystroke_t owns char* key.
                                    .key_state=keystate_tokens[i]
                                }
                            }
                        );
                    }
                    if(parsed_num) command_array_add(this->cmd_arr,
                        (command_t){.type=VT_Delay,
                            .cmd.delay=parsed_num
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"In state RS_DelayNum, current character not allowed '%c'.\n",current_char);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_MouseType:
                if(isdigit(current_char)){
                    num_str[0]=current_char;
                    num_str[1]='\0';
                    break;
                }else if(current_char=='='){
                    new_token_i+=2;//To read numbers.
                    parse_i_offset=-1;
                    read_state=RS_MouseState;
                    break;
                }
                fprintf(stderr,"In state RS_MouseType, current character not allowed '%c'.\n",current_char);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_MouseState:
                if(char_is_keystate(current_char)){
                    if(added_keystate){
                        fprintf(stderr,"Cannot add more than 1 keystate.\n");
                        this->parse_error=true;
                        key_processed=true;
                        break;
                    }
                    if(current_char=='u'||current_char=='U') keystate_tokens[0]=false;//Using keystate since there's only 1 mouse click.
                    else keystate_tokens[0]=true;
                    added_keystate=true;
                    command_array_add(this->cmd_arr,
                        (command_t){.type=VT_MouseClick,
                            .cmd.mouse=(mouse_click_t){.mouse_state=keystate_tokens[0],
                                .mouse_type=strtol(num_str,NULL,10)
                            }
                        }
                    );
                    break;
                }else if(current_char=='.'){
                    new_token_i+=2;//To read numbers only.
                    parse_i_offset=-1;
                    read_state=RS_MouseDelay;
                    break;
                }
                fprintf(stderr,"In state RS_MouseType, current character not allowed '%c'.\n",current_char);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_MouseDelay:
                if(char_is_delay(current_char)){
                    switch(current_char){
                        case 's': case 'S': delay_mult=1000000; break;
                        case 'm': case 'M': delay_mult=1000; break;
                        case 'u': case 'U': delay_mult=1;
                    }
                    new_token_i+=1;//To read numbers only.
                    parse_i_offset=-1;
                    read_state=RS_MouseDelayNum;
                    break;
                }else if(isdigit(current_char)){
                    delay_mult=1;//Default microseconds.
                    read_state=RS_MouseDelayNum;
                    num_str[parse_i_offset]=current_char;
                    num_str=(char*)realloc(num_str,sizeof(char)*(parse_i_offset+2));
                    EXIT_IF_NULL(num_str,char*);
                    break;
                }
                fprintf(stderr,"In state RS_MouseDelay, current character not allowed '%c'.\n",current_char);
                this->parse_error=true;
                key_processed=true;
                break;
            case RS_MouseDelayNum:
                if(isdigit(current_char)){
                    num_str[parse_i_offset]=current_char;
                    num_str=(char*)realloc(num_str,sizeof(char)*(parse_i_offset+2));
                    EXIT_IF_NULL(num_str,char*);
                    break;
                }else if(current_char==';'||current_char=='\0'){
                    num_str[parse_i_offset]='\0';
                    parsed_num=strtol(num_str,NULL,10)*delay_mult;
                    if(parsed_num) command_array_add(this->cmd_arr,
                        (command_t){.type=VT_Delay,
                            .cmd.delay=parsed_num
                        }
                    );
                    key_processed=true;
                    break;
                }
                fprintf(stderr,"In state RS_MouseDelayNum, current character not allowed '%c'.\n",current_char);
                this->parse_error=true;
                key_processed=true;
                break;
        }
        parse_i_offset++;
    }while(!key_processed);
    this->parse_i+=new_token_i+parse_i_offset;
    free(read_tokens);//Free any arrays from parsing.
    free(keystate_tokens);
    free(num_str);
    return !this->parse_error;
}
void macro_buffer_free(macro_buffer_t* this){
    free(this->contents);
    free(this);
}
keystroke_t* keystroke_new(bool key_state, char* p_owner key_owned){//keystroke_t owns key string.
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
void repeat_id_add_name(repeat_id_manager_t* this, char* p_owner str_owned, int index){
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
int repeat_id_search_index(const repeat_id_manager_t* this,const char* search_str){
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
                printf("(%d) RepeatStart: Counter: %d\n",i,cmd.repeat_start.counter);
                break;
            case VT_RepeatEnd:
                printf("(%d) RepeatEnd: RepeatAtIndex: %d MaxCounter: %d\n",i,cmd.repeat_end.index,cmd.repeat_end.counter_max);
                break;
            default:
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
    return (c=='u')||(c=='d')||(c=='U')||(c=='D');
}
bool char_is_whitespace(char c){
    return (c==' ')||(c=='\t')||(c=='\n')||(c=='\v')||(c=='\f')||(c=='\r');
}
bool char_is_delay(char c){
    return (c=='m')||(c=='u')||(c=='s')||(c=='M')||(c=='U')||(c=='S');
}
