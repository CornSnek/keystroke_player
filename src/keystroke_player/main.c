#include "parser.h"
#include "key_down_check.h"
#include <math.h>
#include <time.h>
#include <string.h>
#define __USE_XOPEN
#include <unistd.h>
int usleep(useconds_t usec);
#include <xdo.h>
#include <pthread.h>
#include <X11/Xutil.h>

#define NSEC_TO_SEC 1000000000
//101 to include '\0'.
#define INPUT_BUFFER_LEN 100

#define __S(s) #s
#define __XS(s) __S(s)
//Just to do get string of INPUT_BUFFER_LEN value
#define INPUT_BUFFER_LEN_STR __XS(INPUT_BUFFER_LEN)

#define CONFIG_FILE_F "./config.bin"
#define LAST_FILE_F "./last_file.conf"
typedef enum{
    PS_RunSuccess,PS_CompileSuccess,PS_ReadError,PS_ParseError,PS_ProgramError
}ProgramStatus;
typedef struct{
    delay_ns_t init_delay;
    delay_ns_t mouse_check_delay;
    bool print_commands;
}Config;
const Config InitConfig={
    .init_delay=2000000,.mouse_check_delay=100000,.print_commands=true
};
typedef struct mouse_c_s{
    int x;
    int y;
}mouse_c_t;
int* cmd_i_stack=0;//TODO: Add Command that pops Command Index to jump back to a JumpToStore like a function.
int cmd_i_size=0;
void store_cmd_index(int i);
bool pop_cmd_index(int* cmd_i);
bool fgets_change(char* str,int buffer_len);
bool write_to_config(const Config config);
Config read_config_file(void);
char* read_default_file(void);
bool write_to_default_file(const char* path);
void get_pixel_color(Display* d, int x, int y, XColor* color);
ProgramStatus parse_file(const char* path, xdo_t* xdo_obj, Config config, bool and_run);
void timespec_diff(const struct timespec* ts_begin,struct timespec* ts_end_clone,struct timespec* ts_diff);
bool run_program(command_array_t* cmd_arr, Config config, xdo_t* xdo_obj);
int main(void){
    if(access(CONFIG_FILE_F,F_OK)){
        printf("First time executing. Creating default binary config.bin file.\n");
        if(!write_to_config(InitConfig)) return EXIT_FAILURE;
    }
    xdo_t* xdo_obj=xdo_new(NULL);
    Window focus_window;//Will be ignored.
    int x_mouse,y_mouse;
    XColor pc;
    char* file_str=0;
    Config config;
    typedef enum{
        IS_Start,IS_EditConfig,IS_BuildFile,IS_RunFile,IS_MouseCoords
    } InputState;
    char input_str[INPUT_BUFFER_LEN+1];
    InputState input_state=IS_Start;
    bool also_run;
    while(true){
        switch(input_state){
            case IS_Start:
                also_run=false;
                printf("Type c for Config\nType b to Build File\nType r to Build and Run File\nType t to Test coordinates of mouse and color\nType q to Quit\nType to continue: ");
                fgets(input_str,INPUT_BUFFER_LEN+1,stdin);
                //fgets_change(input_str,INPUT_BUFFER_LEN);//Remove '\n' if any.
                if(!strcmp(input_str,"q\n")) goto done;
                if(!strcmp(input_str,"c\n")) input_state=IS_EditConfig;
                if(!strcmp(input_str,"b\n")) input_state=IS_BuildFile;
                if(!strcmp(input_str,"r\n")) input_state=IS_RunFile;
                if(!strcmp(input_str,"t\n")) input_state=IS_MouseCoords;
                break;
            case IS_EditConfig:
                config=read_config_file();
                printf("Set value for init_delay (Microseconds before macro plays)\n");
                printf("Value right now is %lu (Enter nothing to skip): ",config.init_delay);
                fgets(input_str,INPUT_BUFFER_LEN+1,stdin);
                if(input_str[0]!='\n') config.init_delay=strtol(input_str,NULL,10);
                printf("Set value for mouse_check_delay (Microseconds to check if mouse moved)\n");
                printf("Value right now is %lu (Enter nothing to skip): ",config.mouse_check_delay);
                fgets(input_str,INPUT_BUFFER_LEN+1,stdin);
                if(input_str[0]!='\n') config.mouse_check_delay=strtol(input_str,NULL,10);
                while(true){
                    printf("Set value for print_commands (Prints debug commands when playing macro)\n");
                    printf("Value right now is %d (Enter 0/1 or nothing to skip): ",config.print_commands);
                    fgets(input_str,INPUT_BUFFER_LEN+1,stdin);
                    if(input_str[0]=='0'){config.print_commands=false;break;}
                    else if(input_str[0]=='1'){config.print_commands=true;break;}
                    else if(input_str[0]=='\n') break;
                }
                if(write_to_config(config)) printf("Changes written to config.bin\n");
                else printf("Didn't write. An error has occured when writing.\n");
                input_state=IS_Start;
                break;
            case IS_RunFile:
                also_run=true;
                //Fallthrough
            case IS_BuildFile:
                file_str=read_default_file();
                printf("Set file path to open. Current file: %s\n",file_str?file_str:"(None)");
                printf("(Press enter to skip, type c to cancel.): ");
                fgets(input_str,INPUT_BUFFER_LEN+1,stdin);
                if(!strcmp(input_str,"c\n")){input_state=IS_Start; break;}
                if(input_str[0]!='\n'){
                    if(fgets_change(input_str,INPUT_BUFFER_LEN)) printf("Warning: String has been truncated to "INPUT_BUFFER_LEN_STR" characters.\n");
                }else{//Enter pressed.
                    if(!file_str){
                        printf("Needs a filepath (None given).\n");
                        break;
                    }
                    strncpy(input_str,file_str,INPUT_BUFFER_LEN); //To input_str stack to be read.
                    input_str[INPUT_BUFFER_LEN]='\0'; //Null terminate since strncpy doesn't guarantee.
                }
                printf("Opening file %s\n",input_str);
                free(file_str);//Free from read_default_file.
                ProgramStatus ps=parse_file(input_str,xdo_obj,read_config_file(),also_run);
                if(ps!=PS_ReadError){//Don't rewrite default path if non-existent.
                    write_to_default_file(input_str);
                }
                switch(ps){
                    case PS_RunSuccess: printf("Macro script ran successfully.\n"); break;
                    case PS_CompileSuccess: printf("Macro script compiled successfully.\n"); break;
                    case PS_ReadError: printf("Macro script failed (File non-existent or read error).\n"); break;
                    case PS_ParseError: printf("Macro script failed (File parsing errors).\n"); break;
                    case PS_ProgramError: printf("Macro script failed (Runtime program errors).\n"); break;
                }
                input_state=IS_Start;
                break;
            case IS_MouseCoords:
                printf("Click anywhere on the screen to see the color of a pixel and its mouse coordinates.\n");
                xdo_select_window_with_click(xdo_obj,&focus_window);
                xdo_get_mouse_location(xdo_obj,&x_mouse,&y_mouse,0);
                printf("You clicked at x:%d y:%d",x_mouse,y_mouse);
                get_pixel_color(xdo_obj->xdpy,x_mouse,y_mouse,&pc);
                printf(" with r:%u g:%u b:%u\n",pc.red>>8,pc.green>>8,pc.blue>>8);//Truncate to byte instead.
                input_state=IS_Start;
                break;
        }
        
    }
    done:
    xdo_free(xdo_obj);
    return 0;
}
void store_cmd_index(int i){
    cmd_i_size++;
    if(cmd_i_stack) cmd_i_stack=(int*)realloc(cmd_i_stack,sizeof(int)*cmd_i_size);
    else cmd_i_stack=(int*)malloc(sizeof(int));
    EXIT_IF_NULL(cmd_i_stack,int)
    cmd_i_stack[cmd_i_size-1]=i;
}
bool pop_cmd_index(int* cmd_i){
    if(cmd_i_stack){
        int popped_cmd_i=cmd_i_stack[--cmd_i_size];
        if(cmd_i_size){
            cmd_i_stack=(int*)realloc(cmd_i_stack,sizeof(int)*cmd_i_size);
            EXIT_IF_NULL(cmd_i_stack,int)
        }else{
            free(cmd_i_stack);
            cmd_i_stack=0;
        }
        *cmd_i=popped_cmd_i;
        return true;
    }else{
        *cmd_i=0;
        return false;
    }
}
bool fgets_change(char* str,int buffer_len){//Remove '\n' (if any) and check if string buffer is full (returns bool). 
    int real_len=strlen(str);
    bool is_full=(buffer_len-1==real_len);
    if(!is_full){
        str[real_len-1]='\0';
    }
    return is_full;
}
bool write_to_config(const Config config){
    FILE* f_obj;
    f_obj=fopen(CONFIG_FILE_F,"wb");
    if(!f_obj) return false;//No file pointer given.
    size_t wrote=fwrite(&config,sizeof(Config),1,f_obj);
    fclose(f_obj);
    return (bool)wrote;//Byte written or not.
}
Config read_config_file(void){
    FILE* f_obj;
    Config config;
    f_obj=fopen(CONFIG_FILE_F,"rb");
    if(!f_obj) return (Config){0};
    fread(&config,sizeof(Config),1,f_obj);
    fclose(f_obj);
    return config;
}
bool write_to_default_file(const char* path){
    FILE* f_obj;
    f_obj=fopen(LAST_FILE_F,"w");
    if(!f_obj) return false;
    size_t wrote=fwrite(path,strlen(path),1,f_obj);
    fclose(f_obj);
    return (bool)wrote;
}
/**nodiscard - Function gives pointer ownership.*/
char* read_default_file(void){
    FILE* f_obj;
    char* df_str;
    f_obj=fopen(LAST_FILE_F,"r");
    if(!f_obj) return 0;
    fseek(f_obj,0,SEEK_END);
    size_t str_len=ftell(f_obj);
    fseek(f_obj,0,SEEK_SET);
    df_str=malloc(sizeof(char)*(str_len+1));//To include '\0'
    fread(df_str,str_len,1,f_obj);
    df_str[str_len]='\0';
    fclose(f_obj);
    return df_str;
}
//Code from https://rosettacode.org/wiki/Color_of_a_screen_pixel#C, but uses <X11/Xutil.h> as the library including -lX11
void get_pixel_color(Display* d, int x, int y, XColor* color){
  XImage* image;
  image=XGetImage(d,RootWindow(d,DefaultScreen(d)),x,y,1,1,AllPlanes,XYPixmap);
  color->pixel=XGetPixel(image,0,0);
  XFree(image);
  XQueryColor(d,DefaultColormap(d,DefaultScreen(d)),color);
}
ProgramStatus parse_file(const char* path, xdo_t* xdo_obj, Config config, bool and_run){
    FILE* f_obj;
    char* file_str;
    f_obj=fopen(path,"r");
    if(!f_obj) return PS_ReadError;
    fseek(f_obj,0,SEEK_END);
    size_t str_len=ftell(f_obj);
    fseek(f_obj,0,SEEK_SET);
    file_str=malloc(sizeof(char)*(str_len+1));//To include '\0'
    fread(file_str,str_len,1,f_obj);
    file_str[str_len]='\0';
    fclose(f_obj);
    command_array_t* cmd_arr=command_array_new();
    macro_buffer_t* mb=macro_buffer_new(file_str,cmd_arr);
    while(macro_buffer_process_next(mb,config.print_commands)){
        if(mb->token_i>mb->str_size) break;
    }
    if(!mb->parse_error){
        if(config.print_commands) command_array_print(cmd_arr); //Always print if no read errors after processing all commands.
        macro_buffer_str_id_check(mb);
    }
    if(!mb->parse_error&&and_run){
        bool run_success=run_program(cmd_arr,config,xdo_obj);
        if(!run_success){
            macro_buffer_free(mb);
            command_array_free(cmd_arr);
            return PS_ProgramError;
        }
    }
    ProgramStatus ps=mb->parse_error?PS_ParseError:(and_run?PS_RunSuccess:PS_CompileSuccess);
    macro_buffer_free(mb); //Get parse_error bool before freeing.
    command_array_free(cmd_arr);
    return ps;
}
typedef struct{
    xdo_t* xdo_obj;
    bool program_done;
    delay_ns_t mouse_check_delay;
    mouse_c_t mouse_c;
}shared_rs;
pthread_mutex_t input_mutex;
void* mouse_check_listener(void* srs_v){
    shared_rs* srs_p=(shared_rs*)srs_v;
    int mouse_x_after,mouse_y_after;
    pthread_mutex_lock(&input_mutex);
    delay_ns_t mouse_check_delay=srs_p->mouse_check_delay;
    while(!srs_p->program_done){
        pthread_mutex_unlock(&input_mutex);
        usleep(mouse_check_delay);
        pthread_mutex_lock(&input_mutex);
        xdo_get_mouse_location(srs_p->xdo_obj,&mouse_x_after,&mouse_y_after,NULL);
        if(srs_p->mouse_c.x!=mouse_x_after||srs_p->mouse_c.y!=mouse_y_after){
            printf("Mouse moved. Stopping macro script.\n");
            srs_p->program_done=true;
        }
    }
    pthread_mutex_unlock(&input_mutex);
    pthread_exit(NULL);
}
//Get time elapsed since ts_begin after calling this function. Assuming ts_begin was before ts_end. Returns time to ts_diff.
//ts_end_clone for another timespec_get from ts_end.
void timespec_diff(const struct timespec* ts_begin,struct timespec* ts_end_clone,struct timespec* ts_diff){
    struct timespec ts_end;
    timespec_get(&ts_end,TIME_UTC);
    if(ts_end_clone) *ts_end_clone=ts_end;
    const long nsec_diff=ts_end.tv_nsec-ts_begin->tv_nsec;
    *ts_diff=(struct timespec){//Ternary operators: If ts_end has less nanoseconds, decrease by 1 sec and add NSEC_TO_SEC to nsec.
        .tv_sec=ts_end.tv_sec-ts_begin->tv_sec-(nsec_diff<0l?1l:0l),
        .tv_nsec=(nsec_diff<0l?NSEC_TO_SEC:0l)+nsec_diff,
    };
}
bool run_program(command_array_t* cmd_arr, Config config, xdo_t* xdo_obj){
    Window focus_window;//Will be ignored.
    xdo_select_window_with_click(xdo_obj,&focus_window);
    int cmd_arr_len=command_array_count(cmd_arr);
    int cmd_arr_i=0;
    key_down_check_t* kdc=key_down_check_new();
    shared_rs srs=(shared_rs){.xdo_obj=xdo_obj,.program_done=false,.mouse_c={0},.mouse_check_delay=config.mouse_check_delay};
    printf("Starting script in %ld microseconds (%f seconds)\n",config.init_delay,(float)config.init_delay/1000000);
    usleep(config.init_delay);
    printf("Running.\n");
    xdo_get_mouse_location(xdo_obj,&srs.mouse_c.x,&srs.mouse_c.y,NULL);
    pthread_t input_t;
    pthread_mutex_init(&input_mutex,NULL);
    int ret=pthread_create(&input_t,NULL,mouse_check_listener,&srs);
    if(ret){
        fprintf(stderr,"Unable to create thread. Exiting program.\n");
        key_down_check_free(kdc);
        xdo_free(xdo_obj);
        return false;
    }
    struct timespec ts_begin,ts_diff,ts_usleep_before,ts_usleep_before_adj;
    timespec_get(&ts_begin,TIME_UTC);
    pthread_mutex_lock(&input_mutex);
    bool print_commands=config.print_commands;
    typedef long nano_sec;
    nano_sec time_after_last_usleep;
    nano_sec real_delay=0;//Adjust delay depending on time after commands and after sleeping.
    delay_ns_t adj_usleep;
    XColor pc;
    int x_mouse,y_mouse,x_mouse_store=0,y_mouse_store=0;
    bool query_is_true;
    timespec_get(&ts_usleep_before,TIME_UTC);
    while(!srs.program_done){
        pthread_mutex_unlock(&input_mutex);
        query_is_true=false;
        command_t this_cmd=cmd_arr->cmds[cmd_arr_i];
        command_union_t cmd_u=cmd_arr->cmds[cmd_arr_i].cmd_u;
        CommandType cmd_type=cmd_arr->cmds[cmd_arr_i].type;
        int* rst_counter=0;
        if(print_commands){
            timespec_diff(&ts_begin,NULL,&ts_diff);
            printf("[%ld.%09ld] - Command %d/%d: ",ts_diff.tv_sec,ts_diff.tv_nsec,cmd_arr_i+1,cmd_arr_len);
        }
        switch(cmd_type){
            case CMD_KeyStroke:
                switch(cmd_u.ks.key_state){
                    case IS_Down:
                        if(print_commands) printf("Key down for %s\n",cmd_u.ks.key);
                        pthread_mutex_lock(&input_mutex);
                        xdo_send_keysequence_window_down(xdo_obj,CURRENTWINDOW,cmd_u.ks.key,0);
                        key_down_check_add(kdc,cmd_u.ks.key);
                        break;
                    case IS_Up:
                        if(print_commands) printf("Key up for %s\n",cmd_u.ks.key);
                        pthread_mutex_lock(&input_mutex);
                        xdo_send_keysequence_window_up(xdo_obj,CURRENTWINDOW,cmd_u.ks.key,0);
                        key_down_check_remove(kdc,cmd_u.ks.key);
                        break;
                    case IS_Click:
                        if(print_commands) printf("Key click for %s\n",cmd_u.ks.key);
                        pthread_mutex_lock(&input_mutex);
                        xdo_send_keysequence_window(xdo_obj,CURRENTWINDOW,cmd_u.ks.key,0);
                }
                pthread_mutex_unlock(&input_mutex);
                break;
            case CMD_Delay://Using timespec_get and timespec_diff (custom function) to try to get "precise delays"
                timespec_diff(&ts_usleep_before,&ts_usleep_before_adj,&ts_diff);
                time_after_last_usleep=ts_diff.tv_sec*NSEC_TO_SEC+ts_diff.tv_nsec;
                real_delay+=cmd_u.delay*1000-time_after_last_usleep;
                adj_usleep=real_delay>0?((delay_ns_t)(real_delay/1000+(real_delay%1000>499?1:0))):0u;//0 and rounded nanoseconds.
                if(print_commands) printf("Sleeping for %ld microseconds (Adjusted to %ld due to commands) \n",cmd_u.delay,adj_usleep);
                usleep(adj_usleep);
                timespec_diff(&ts_usleep_before_adj,&ts_usleep_before,&ts_diff);
                real_delay-=ts_diff.tv_sec*NSEC_TO_SEC+ts_diff.tv_nsec;
                break;
            case CMD_RepeatEnd:
                if(cmd_u.repeat_end.counter_max){//Max counter non-zero.
                    rst_counter=&(cmd_arr->cmds[cmd_u.repeat_end.cmd_index].cmd_u.repeat_start.counter);
                    if(print_commands) printf("Jump to Command #%d until Counter %d/%d String ID#%d\n",cmd_u.repeat_end.cmd_index+2,++(*rst_counter),cmd_u.repeat_end.counter_max,cmd_u.repeat_end.str_index);
                    if(*rst_counter!=cmd_u.repeat_end.counter_max) cmd_arr_i=cmd_u.repeat_end.cmd_index;//Go back if not counter_max
                    else *rst_counter=0;//Reset to loop again.
                }else{//Loop forever otherwise.
                    if(print_commands) printf("Jump to Command #%d (Loops forever) String ID#%d\n",cmd_u.repeat_end.cmd_index+2,cmd_u.repeat_end.str_index);
                    cmd_arr_i=cmd_u.repeat_end.cmd_index;
                }
                break;
            case CMD_RepeatResetCounters:
                if(print_commands) printf("Resetting RepeatStart counters back to 0.\n");
                for(int i=0;i<cmd_arr->size;i++){
                    command_t* cmd=cmd_arr->cmds+i;
                    if(cmd->type==CMD_RepeatStart) cmd->cmd_u.repeat_start.counter=0;
                }
                break;
            case CMD_RepeatStart:
                if(print_commands) printf("This is a loop counter (%d) String ID#%d\n",cmd_u.repeat_start.counter,cmd_u.repeat_start.str_index);
                break;
            case CMD_MouseClick:
                switch(cmd_u.mouse_click.mouse_state){
                    case IS_Down:
                        if(print_commands) printf("Mouse down (%d).\n",cmd_u.mouse_click.mouse_type);
                        pthread_mutex_lock(&input_mutex);
                        xdo_mouse_down(xdo_obj,CURRENTWINDOW,cmd_u.mouse_click.mouse_type);
                        break;
                    case IS_Up:
                        if(print_commands) printf("Mouse up (%d).\n",cmd_u.mouse_click.mouse_type);
                        pthread_mutex_lock(&input_mutex);
                        xdo_mouse_up(xdo_obj,CURRENTWINDOW,cmd_u.mouse_click.mouse_type);
                        break;
                    case IS_Click:
                        if(print_commands) printf("Mouse click (%d).\n",cmd_u.mouse_click.mouse_type);
                        pthread_mutex_lock(&input_mutex);
                        xdo_click_window(xdo_obj,CURRENTWINDOW,cmd_u.mouse_click.mouse_type);
                }
                pthread_mutex_unlock(&input_mutex);
                break;
            case CMD_MoveMouse:
                if(print_commands) printf("Mouse move at (%d,%d) (%s).",cmd_u.mouse_move.x,cmd_u.mouse_move.y,cmd_u.mouse_move.is_absolute?"absolute":"relative");
                pthread_mutex_lock(&input_mutex);
                if(cmd_u.mouse_move.is_absolute) xdo_move_mouse(xdo_obj,cmd_u.mouse_move.x,cmd_u.mouse_move.y,0);
                else xdo_move_mouse_relative(xdo_obj,cmd_u.mouse_move.x,cmd_u.mouse_move.y);
                xdo_get_mouse_location(xdo_obj,&srs.mouse_c.x,&srs.mouse_c.y,0);//Update mouse movement for input_t thread loop.
                if(print_commands){
                    if(cmd_u.mouse_move.is_absolute) printf("\n");
                    else printf(" Mouse now at (%d,%d).\n",srs.mouse_c.x,srs.mouse_c.y);
                } 
                pthread_mutex_unlock(&input_mutex);
                break;
            case CMD_Exit:
                if(print_commands) printf("Exit command issued. Exiting program now.\n");
                pthread_mutex_lock(&input_mutex);
                srs.program_done=true;
                pthread_mutex_unlock(&input_mutex);
                break;
            case CMD_Pass:
                if(print_commands) printf("Pass command (does nothing).\n");
                break;
            case CMD_JumpTo:
                if(print_commands) printf("Jump to Command #%d String ID#%d. %s\n",cmd_u.jump_to.cmd_index+2,cmd_u.jump_to.str_index,cmd_u.jump_to.store_index?"Will also store this command's index.":"");
                cmd_arr_i=cmd_u.jump_to.cmd_index;
                if(cmd_u.jump_to.store_index){
                    if(print_commands) printf(" Storing next command index to jump later (#%d)",cmd_arr_i+2);
                    store_cmd_index(cmd_arr_i);
                }
                break;
            case CMD_JumpFrom:
                if(print_commands) printf("Jump from command String ID#%d.\n",cmd_u.jump_from.str_index);
                break;
            case CMD_SaveMouseCoords:
                if(print_commands) printf("Saving current mouse coordinates.");
                pthread_mutex_lock(&input_mutex);
                xdo_get_mouse_location(xdo_obj,&x_mouse_store,&y_mouse_store,0);
                pthread_mutex_unlock(&input_mutex);
                if(print_commands) printf(" x: %d y: %d\n",x_mouse_store,y_mouse_store);
                break;
            case CMD_LoadMouseCoords:
                if(print_commands) printf("Moving to stored mouse coordinates x: %d y: %d\n",x_mouse_store,y_mouse_store);
                pthread_mutex_lock(&input_mutex);
                xdo_move_mouse(xdo_obj,x_mouse_store,y_mouse_store,0);
                xdo_get_mouse_location(xdo_obj,&srs.mouse_c.x,&srs.mouse_c.y,0);//Update mouse movement for input_t thread loop.
                pthread_mutex_unlock(&input_mutex);
                break;
            case CMD_QueryComparePixel:
                if(print_commands) printf("Don't skip next command if pixel at mouse matches r,g,b=%d,%d,%d with threshold of %d. ",cmd_u.pixel_compare.r,cmd_u.pixel_compare.g,cmd_u.pixel_compare.b,cmd_u.pixel_compare.thr);
                pthread_mutex_lock(&input_mutex);
                xdo_get_mouse_location(xdo_obj,&x_mouse,&y_mouse,0);
                get_pixel_color(xdo_obj->xdpy,x_mouse,y_mouse,&pc);
                pthread_mutex_unlock(&input_mutex);
                query_is_true=(abs((unsigned char)(pc.red>>8)-cmd_u.pixel_compare.r)<=cmd_u.pixel_compare.thr
                    &&abs((unsigned char)(pc.green>>8)-cmd_u.pixel_compare.g)<=cmd_u.pixel_compare.thr
                    &&abs((unsigned char)(pc.blue>>8)-cmd_u.pixel_compare.b)<=cmd_u.pixel_compare.thr
                );
                if(print_commands) printf("Pixel does%s match (r,g,b=%d,%d,%d).\n",query_is_true?"":"n't",pc.red>>8,pc.green>>8,pc.blue>>8);
                break;
            case CMD_QueryCompareCoords:
                ;const CompareCoords cc=cmd_u.compare_coords.cmp_flags;
                if(print_commands) printf("Don't skip next command if mouse coordinate %c%c%s%d. ",(cc&CMP_Y)==CMP_Y?'y':'x',(cc&CMP_GT)==CMP_GT?'>':'<',(cc&CMP_W_EQ)==CMP_W_EQ?"=":"",cmd_u.compare_coords.var);
                pthread_mutex_lock(&input_mutex);
                xdo_get_mouse_location(xdo_obj,&x_mouse,&y_mouse,0);
                pthread_mutex_unlock(&input_mutex);
                ;const int mouse_compare=(cc&CMP_Y)==CMP_Y?y_mouse:x_mouse;
                if((cc&CMP_GT)==CMP_GT) query_is_true=(cc&CMP_W_EQ)==CMP_W_EQ?mouse_compare>=cmd_u.compare_coords.var:mouse_compare>cmd_u.compare_coords.var;
                else query_is_true=(cc&CMP_W_EQ)==CMP_W_EQ?mouse_compare<=cmd_u.compare_coords.var:mouse_compare<cmd_u.compare_coords.var;
                if(print_commands) printf("Compare is %s.\n",query_is_true?"true":"false");
                break;
            case CMD_QueryCoordsWithin:
                ;const coords_within_t coords_within=cmd_u.coords_within;
                if(print_commands) printf("Don't skip next command if mouse is within Top Left x:%d y:%d Bottom Right x:%d y:%d. ",coords_within.xl,coords_within.yl,coords_within.xh,coords_within.yh);
                pthread_mutex_lock(&input_mutex);
                xdo_get_mouse_location(xdo_obj,&x_mouse,&y_mouse,0);
                pthread_mutex_unlock(&input_mutex);
                query_is_true=x_mouse>=coords_within.xl&&x_mouse<=coords_within.xh&&y_mouse>=coords_within.yl&&y_mouse<=coords_within.yh;
                if(print_commands) printf("It is %s within the box.\n",query_is_true?"":"not");
                break;
        }
        pthread_mutex_lock(&input_mutex);
        if(!this_cmd.is_query) ++cmd_arr_i;
        else{
            if(query_is_true){
                query_is_true=false;
                cmd_arr_i++;
            }else cmd_arr_i+=2;//Skip
        }
        if(cmd_arr_i==cmd_arr_len){
            srs.program_done=true;//To end the input_t thread loop as well.
        }
    }
    timespec_diff(&ts_begin,NULL,&ts_diff);
    printf("%ld.%09ld seconds since macro script ran.\n",ts_diff.tv_sec,ts_diff.tv_nsec);
    pthread_mutex_unlock(&input_mutex);
    pthread_join(input_t,NULL);
    key_down_check_key_up(kdc,xdo_obj,CURRENTWINDOW);
    key_down_check_free(kdc);
    return true;
}
