#include "parser.h"
#include "key_down_check.h"
#include <string.h>
#define __USE_XOPEN
#include <unistd.h>
int usleep(useconds_t usec);
#include <xdo.h>
#include <pthread.h>
#include <X11/Xutil.h>

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
bool fgets_change(char* str,int buffer_len);
bool write_to_config(const Config config);
Config read_config_file();
char* read_default_file();
bool write_to_default_file(const char* path);
void get_pixel_color(Display* d, int x, int y, XColor* color);
ProgramStatus parse_file(const char* path, xdo_t* xdo_obj, Config config, bool and_run);
bool run_program(command_array_t* cmd_arr, Config config, xdo_t* xdo_obj);
int main(int argc, char** argv){
    (void)argc;
    (void)argv;
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
                printf("Type '\\c' for config, '\\b' to build file, '\\r' to build and run file, '\\m' to test coordinates of mouse and color, or '\\q' to quit: ");
                fgets(input_str,INPUT_BUFFER_LEN+1,stdin);
                //fgets_change(input_str,INPUT_BUFFER_LEN);//Remove '\n' if any.
                if(!strcmp(input_str,"\\q\n")) goto done;
                if(!strcmp(input_str,"\\c\n")) input_state=IS_EditConfig;
                if(!strcmp(input_str,"\\b\n")) input_state=IS_BuildFile;
                if(!strcmp(input_str,"\\r\n")) input_state=IS_RunFile;
                if(!strcmp(input_str,"\\m\n")) input_state=IS_MouseCoords;
                break;
            case IS_EditConfig:
                config=read_config_file();
                printf("Set value for init_delay (Microseconds before autoclicker plays)\n");
                printf("Value right now is %lu (Enter nothing to skip): ",config.init_delay);
                fgets(input_str,INPUT_BUFFER_LEN+1,stdin);
                if(input_str[0]!='\n') config.init_delay=strtol(input_str,NULL,10);
                printf("Set value for mouse_check_delay (Microseconds to check if mouse moved)\n");
                printf("Value right now is %lu (Enter nothing to skip): ",config.mouse_check_delay);
                fgets(input_str,INPUT_BUFFER_LEN+1,stdin);
                if(input_str[0]!='\n') config.mouse_check_delay=strtol(input_str,NULL,10);
                while(true){
                    printf("Set value for print_commands (Prints debug commands when playing autoclicker)\n");
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
                printf("(Press enter to skip, type \\c to cancel.): ");
                fgets(input_str,INPUT_BUFFER_LEN+1,stdin);
                if(!strcmp(input_str,"\\c\n")){input_state=IS_Start; break;}
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
                    case PS_RunSuccess: printf("Autoclicker script ran successfully.\n"); break;
                    case PS_CompileSuccess: printf("Autoclicker script compiled successfully.\n"); break;
                    case PS_ReadError: printf("Autoclicker script failed (File non-existent or read error).\n"); break;
                    case PS_ParseError: printf("Autoclicker script failed (File parsing errors).\n"); break;
                    case PS_ProgramError: printf("Autoclicker script failed (Running program errors).\n"); break;
                }
                input_state=IS_Start;
                break;
            case IS_MouseCoords:
                printf("After pressing ENTER and waiting 1 second, click anywhere on the screen.\n");
                fgets(input_str,INPUT_BUFFER_LEN+1,stdin);
                usleep(1000000);
                xdo_select_window_with_click(xdo_obj,&focus_window);
                xdo_get_mouse_location(xdo_obj,&x_mouse,&y_mouse,0);
                printf("You clicked at x:%d, y:%d\n",x_mouse,y_mouse);
                get_pixel_color(xdo_obj->xdpy,x_mouse,y_mouse,&pc);
                printf ("r:%u g:%u b:%u\n",pc.red>>8,pc.green>>8,pc.blue>>8);//Truncate to byte instead.
                input_state=IS_Start;
                break;
        }
        
    }
    done:
    xdo_free(xdo_obj);
    return 0;
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
Config read_config_file(){
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
/**nodiscard - Function is pointer owner.*/
char* read_default_file(){
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
    repeat_id_manager_t* rim=repeat_id_manager_new();
    macro_buffer_t* mb=macro_buffer_new(file_str,cmd_arr,rim);
    while(macro_buffer_process_next(mb,config.print_commands)){
        if(mb->token_i>mb->size) break;
    }
    if(!mb->parse_error&&and_run){
        bool run_success=run_program(cmd_arr,config,xdo_obj);
        if(!run_success){
            repeat_id_manager_free(rim);
            command_array_free(cmd_arr);
            macro_buffer_free(mb);
            return PS_ProgramError;
        }
    }
    repeat_id_manager_free(rim);
    command_array_free(cmd_arr);
    ProgramStatus ps=mb->parse_error?PS_ParseError:(and_run?PS_RunSuccess:PS_CompileSuccess);
    macro_buffer_free(mb); //Get parse_error bool before freeing.
    return ps;
}
typedef struct{
    xdo_t* xdo_obj;
    bool program_done;
    delay_ns_t mouse_check_delay;
    struct{
        int x;
        int y;
    }mouse;
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
        if(srs_p->mouse.x!=mouse_x_after||srs_p->mouse.y!=mouse_y_after){
            printf("Mouse moved. Stopping autoclicker.\n");
            srs_p->program_done=true;
        }
    }
    pthread_mutex_unlock(&input_mutex);
    pthread_exit(NULL);
}
bool run_program(command_array_t* cmd_arr, Config config, xdo_t* xdo_obj){
    Window focus_window;//Will be ignored.
    if(config.print_commands) command_array_print(cmd_arr);
    xdo_select_window_with_click(xdo_obj,&focus_window);
    int cmd_arr_len=command_array_count(cmd_arr);
    int cmd_arr_i=0;
    key_down_check_t* kdc=key_down_check_new();
    shared_rs srs=(shared_rs){.xdo_obj=xdo_obj,.program_done=false,.mouse={0},.mouse_check_delay=config.mouse_check_delay};
    printf("Starting script in %ld microseconds (%f seconds)\n",config.init_delay,(float)config.init_delay/1000000);
    usleep(config.init_delay);
    printf("Running.\n");
    xdo_get_mouse_location(xdo_obj,&srs.mouse.x,&srs.mouse.y,NULL);
    pthread_t input_t;
    pthread_mutex_init(&input_mutex,NULL);
    int ret=pthread_create(&input_t,NULL,mouse_check_listener,&srs);
    if(ret){
        fprintf(stderr,"Unable to create thread. Exiting program.\n");
        key_down_check_free(kdc);
        xdo_free(xdo_obj);
        return false;
    }
    pthread_mutex_lock(&input_mutex);
    while(!srs.program_done){
        pthread_mutex_unlock(&input_mutex);
        command_union_t cmd_u=cmd_arr->cmds[cmd_arr_i].cmd;
        CommandType cmd_type=cmd_arr->cmds[cmd_arr_i].type;
        int* rst_counter=0;
        bool print_commands=config.print_commands;
        switch(cmd_type){
            case(VT_KeyStroke):
                pthread_mutex_lock(&input_mutex);
                switch(cmd_u.ks.key_state){
                    case IS_Down:
                        if(print_commands) printf("Command %d/%d: Key down for %s\n",cmd_arr_i+1,cmd_arr_len,cmd_u.ks.key);
                        xdo_send_keysequence_window_down(xdo_obj,CURRENTWINDOW,cmd_u.ks.key,0);
                        key_down_check_add(kdc,cmd_u.ks.key);
                        break;
                    case IS_Up:
                        if(print_commands) printf("Command %d/%d: Key up for %s\n",cmd_arr_i+1,cmd_arr_len,cmd_u.ks.key);
                        xdo_send_keysequence_window_up(xdo_obj,CURRENTWINDOW,cmd_u.ks.key,0);
                        key_down_check_remove(kdc,cmd_u.ks.key);
                        break;
                    case IS_Click:
                        if(print_commands) printf("Command %d/%d: Key click for %s\n",cmd_arr_i+1,cmd_arr_len,cmd_u.ks.key);
                        xdo_send_keysequence_window(xdo_obj,CURRENTWINDOW,cmd_u.ks.key,0);
                }
                pthread_mutex_unlock(&input_mutex);
                break;
            case(VT_Delay):
                if(print_commands) printf("Command %d/%d: Sleeping for %ld microseconds\n",cmd_arr_i+1,cmd_arr_len,cmd_u.delay);
                usleep(cmd_u.delay);
                break;
            case(VT_RepeatEnd):
                if(cmd_u.repeat_end.counter_max){//Counter has been set.
                    rst_counter=&(cmd_arr->cmds[cmd_u.repeat_end.index].cmd.repeat_start);
                    if(print_commands) printf("Command %d/%d: Jump to Command #%d until counter %d reaches %d\n",cmd_arr_i+1,cmd_arr_len,cmd_u.repeat_end.index+2,++(*rst_counter),cmd_u.repeat_end.counter_max);
                    if(*rst_counter!=cmd_u.repeat_end.counter_max) cmd_arr_i=cmd_u.repeat_end.index;//Go back if not counter_max
                    else *rst_counter=0;//Reset to loop again.
                }else{//Loop forever otherwise.
                    if(print_commands) printf("Command %d/%d: Jump to Command #%d (Repeats forever)\n",cmd_arr_i+1,cmd_arr_len,cmd_u.repeat_end.index+2);
                    cmd_arr_i=cmd_u.repeat_end.index;
                }
                break;
            case(VT_RepeatStart):
                //if(print_commands) printf("Command %d/%d: This is a loop counter (%d).\n",cmd_arr_i+1,cmd_arr_len,cmd_u.repeat_start.counter);
                break;
            case(VT_MouseClick):
                pthread_mutex_lock(&input_mutex);
                switch(cmd_u.mouse_click.mouse_state){
                    case IS_Down:
                        if(print_commands) printf("Command %d/%d: Mouse down (%d).\n",cmd_arr_i+1,cmd_arr_len,cmd_u.mouse_click.mouse_type);
                        xdo_mouse_down(xdo_obj,CURRENTWINDOW,cmd_u.mouse_click.mouse_type);
                        break;
                    case IS_Up:
                        if(print_commands) printf("Command %d/%d: Mouse up (%d).\n",cmd_arr_i+1,cmd_arr_len,cmd_u.mouse_click.mouse_type);
                        xdo_mouse_up(xdo_obj,CURRENTWINDOW,cmd_u.mouse_click.mouse_type);
                        break;
                    case IS_Click:
                        if(print_commands) printf("Command %d/%d: Mouse click (%d).\n",cmd_arr_i+1,cmd_arr_len,cmd_u.mouse_click.mouse_type);
                        xdo_click_window(xdo_obj,CURRENTWINDOW,cmd_u.mouse_click.mouse_type);
                }
                pthread_mutex_unlock(&input_mutex);
                break;
            case(VT_MouseMove):
                pthread_mutex_lock(&input_mutex);
                if(print_commands) printf("Command %d/%d: Mouse move at (%d,%d).\n",cmd_arr_i+1,cmd_arr_len,cmd_u.mouse_move.x,cmd_u.mouse_move.y);
                srs.mouse.x=cmd_u.mouse_move.x;
                srs.mouse.y=cmd_u.mouse_move.y;//Update mouse movement for input_t thread loop.
                xdo_move_mouse(xdo_obj,cmd_u.mouse_move.x,cmd_u.mouse_move.y,0);
                pthread_mutex_unlock(&input_mutex);
                break;
            case(VT_Exit):
                pthread_mutex_lock(&input_mutex);
                if(print_commands) printf("Command %d/%d: Exit command issued. Exiting program now.\n",cmd_arr_i+1,cmd_arr_len);
                srs.program_done=true;
                pthread_mutex_unlock(&input_mutex);
        }
        pthread_mutex_lock(&input_mutex);
        if(++cmd_arr_i==cmd_arr_len){
            srs.program_done=true;//To end the input_t thread loop as well.
        }
    }
    pthread_mutex_unlock(&input_mutex);
    pthread_join(input_t,NULL);
    key_down_check_key_up(kdc,xdo_obj,CURRENTWINDOW);
    key_down_check_free(kdc);
    return true;
}
