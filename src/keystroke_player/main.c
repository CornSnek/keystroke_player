#include "macros.h"
#include "parser.h"
#include "key_down_check.h"
#include "variable_loader.h"
#include "rpn_evaluator.h"
#include <math.h>
#include <time.h>
#include <string.h>
#define __USE_XOPEN
#include <unistd.h>
int usleep(useconds_t usec);
#include <xdo.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <assert.h>
typedef enum _ProgramStatus{
    PS_RunSuccess,PS_CompileSuccess,PS_MacroError,PS_ReadError,PS_ParseError,PS_ProgramError
}ProgramStatus;
typedef enum _DebugPrintType{
    DBP_None,DBP_AllCommands,DBP_CommandNumber
}DebugPrintType;
typedef struct{
    delay_ns_t init_delay;
    delay_ns_t key_check_delay;
    DebugPrintType debug_print_type;
}Config;
const Config InitConfig={
    .init_delay=2000000,.key_check_delay=100000,.debug_print_type=DBP_AllCommands
};
bool fgets_change(char* str,int buffer_len);
bool write_to_config(const Config config);
Config read_config_file(void);
char* read_default_file(void);
bool write_to_default_file(const char* path);
void get_pixel_color(Display* d, int x, int y, XColor* color);
ProgramStatus parse_file(const char* path, xdo_t* xdo_obj, Config config, bool and_run);
void timespec_diff(const struct timespec* ts_begin,struct timespec* ts_end_clone,struct timespec* ts_diff);
bool run_program(command_array_t* cmd_arr, const char* file_str, Config config, xdo_t* xdo_obj, VariableLoader_t* vl);
bool test_mouse_func(void* xdo_v){
    int x_mouse,y_mouse;
    XColor pc;
    xdo_get_mouse_location((xdo_t*)xdo_v,&x_mouse,&y_mouse,0);
    printf("You clicked at x:%d y:%d",x_mouse,y_mouse);
    get_pixel_color(((xdo_t*)xdo_v)->xdpy,x_mouse,y_mouse,&pc);
    printf(" with r:%u g:%u b:%u\n",pc.red>>8,pc.green>>8,pc.blue>>8);//Truncate to byte instead.
    return true;
}
typedef enum _MenuState{
    MS_Start,MS_EditConfig,MS_BuildFile,MS_RunFile,MS_MouseCoords,MS_RpnEvaluatorStart,MS_RpnEvaluator,MS_Done
}MenuState;
typedef struct ms_cont{
    MenuState* ms;
    MenuState v;
}ms_cont_t;
bool _input_state_func(void* MS_v){//Sets menu_state variable in main(void) to v.
    *(((ms_cont_t*)MS_v)->ms)=((ms_cont_t*)MS_v)->v;
    return false;
}
int main(void){
    if(access(CONFIG_FILE_F,F_OK)) if(!write_to_config(InitConfig)) return EXIT_FAILURE;
    xdo_t* xdo_obj=xdo_new(NULL);
    char* file_name_str=0;
    Config config;
    char input_str[INPUT_BUFFER_LEN+1];
    MenuState menu_state=MS_Start;
    bool also_run=false,do_rpn=false,rpn_see_stack=false,var_menu=false,add_var=false,remove_var=false,list_var=false;
    VariableLoader_t* vl;
    char* input_str_end=0,* var_str,* num_str;
    as_number_t an_output;
    as_number_opt_t an_opt_output;
    RPNValidStringE rpnvs_e;
    RPNEvaluatorInit();
    while(true){
        //printf(CLEAR_TERM);
        switch(menu_state){
            case MS_Start:
                also_run=false;
                printf(
                    "--------------------\n"
                    "Keystroke Player: Plays mouse/keyboard macro scripts and stops when the q key is pressed, or the script exits\n"
                    "Press c for Config\n"
                    "Press b to Build File\n"
                    "Press r to Build and Run File\n"
                    "Press t to Test coordinates of mouse and color\n"
                    "Press e to Test equations in Reverse Polish Notation (RPN)\n"
                    "Press q to Quit\n"
                    "Escape key toggles enabling/disabling keybinds outside of macro scripts\n"
                );
                keypress_loop(xdo_obj->xdpy,(callback_t[6]){{
                    .func=_input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_Done},
                    .ks=XK_Q
                },{
                    .func=_input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_EditConfig},
                    .ks=XK_C
                },{
                    .func=_input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_BuildFile},
                    .ks=XK_B
                },{
                    .func=_input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_RunFile},
                    .ks=XK_R
                },{
                    .func=_input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_MouseCoords},
                    .ks=XK_T
                },{
                    .func=_input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_RpnEvaluatorStart},
                    .ks=XK_E
                }},6);
                break;
            case MS_EditConfig:
                config=read_config_file();
                puts("Set value for init_delay (Microseconds before macro plays)");
                printf("Value right now is %lu (Enter nothing to skip): ",config.init_delay);
                fgets(input_str,INPUT_BUFFER_LEN,stdin);
                if(input_str[0]!='\n') config.init_delay=strtol(input_str,NULL,10);
                puts("Set value for key_check_delay (Microseconds to check if q key is pressed)");
                printf("Value right now is %lu (Enter nothing to skip): ",config.key_check_delay);
                fgets(input_str,INPUT_BUFFER_LEN,stdin);
                if(input_str[0]!='\n') config.key_check_delay=strtol(input_str,NULL,10);
                while(true){
                    puts("Set value for debug_commands (Prints debug commands when playing macro)");
                    printf("0 for no debug printing, 1 to print all commands (parsing and running program), 2 to print some command numbers and strings (clears terminal)\n");
                    printf("Value right now is %d (Enter 0/1/2 or nothing to skip): ",config.debug_print_type);
                    fgets(input_str,INPUT_BUFFER_LEN,stdin);
                    switch(input_str[0]){
                        case '0': config.debug_print_type=DBP_None; goto debug_print_type_valid;
                        case '1': config.debug_print_type=DBP_AllCommands; goto debug_print_type_valid;
                        case '2': config.debug_print_type=DBP_CommandNumber; goto debug_print_type_valid;
                        case '\n': goto debug_print_type_valid;
                        default: break;
                    }
                }
                debug_print_type_valid:
                if(write_to_config(config)) puts("Changes written to config.bin");
                else puts("Didn't write. An error has occured when writing.");
                //while((clear_stdin=getchar())!='\n'&&clear_stdin!=EOF);
                menu_state=MS_Start;
                break;
            case MS_RunFile:
                also_run=true;
                //Fallthrough
            case MS_BuildFile:
                file_name_str=read_default_file();
                printf("Set file path to open. Current file: %s\n",file_name_str?file_name_str:"(None)");
                printf("(Press enter to skip, type c to cancel.): ");
                fgets(input_str,INPUT_BUFFER_LEN,stdin);
                if(!strcmp(input_str,"c\n")){
                    free(file_name_str);
                    menu_state=MS_Start;
                    break;
                }
                if(input_str[0]=='\n'){//Enter pressed.
                    if(!file_name_str){
                        puts("Needs a filepath (None given).");
                        break;
                    }
                    strncpy(input_str,file_name_str,INPUT_BUFFER_LEN); //To input_str stack to be read.
                    input_str[INPUT_BUFFER_LEN]='\0'; //Null terminate since strncpy doesn't guarantee.
                }
                printf("Opening file %s\n",input_str);
                free(file_name_str);
                ProgramStatus ps=parse_file(input_str,xdo_obj,read_config_file(),also_run);
                if(ps!=PS_ReadError){//Don't rewrite default path if non-existent.
                    write_to_default_file(input_str);
                }
                switch(ps){
                    case PS_RunSuccess: puts("Macro script ran successfully."); break;
                    case PS_CompileSuccess: puts("Macro script compiled successfully."); break;
                    case PS_ReadError: puts("Macro script failed (File non-existent or read error)."); break;
                    case PS_ParseError: puts("Macro script failed (File parsing errors)."); break;
                    case PS_ProgramError: puts("Macro script failed (Runtime program errors)."); break;
                    case PS_MacroError: puts("Macro script failed (Macro expansion errors or cancelled).");
                }
                puts("Press y to build/run again or q to return to menu.");
                keypress_loop(xdo_obj->xdpy,(callback_t[2]){{
                    .func=_input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_Start},
                    .ks=XK_Q
                },{
                    .func=CallbackEndLoop,
                    .arg=0,//Previous state used.
                    .ks=XK_Y
                }},2);
                break;
            case MS_MouseCoords:
                puts("Press t to test mouse/color coordinates. Press q to quit.");
                keypress_loop(xdo_obj->xdpy,(callback_t[2]){{
                    .func=test_mouse_func,
                    .arg=xdo_obj,
                    .ks=XK_T
                },{
                    .func=CallbackEndLoop,
                    .arg=0,
                    .ks=XK_Q
                }},2);
                menu_state=MS_Start;
                break;
            case MS_RpnEvaluatorStart://There's no enter/exit states. Initialize Variable Loader and set menu_state to below.
                vl=VL_new(200);
                menu_state=MS_RpnEvaluator;
                rpn_see_stack=false;//fallthrough
            case MS_RpnEvaluator:
                do_rpn=false;
                printf("--------------------\n"
                "Press r to type equations in RPN notation.\n"
                "Press a to add custom variables.\n"
                "Press s to see the number stack when processing RPN notation. Currently set to %s.\n"
                "Press l to see the list of variable/functions used.\n"
                "Press q to quit.\n",rpn_see_stack?"on":"off");
                keypress_loop(xdo_obj->xdpy,(callback_t[5]){{
                    .func=_boolean_edit_func,
                    .arg=&(_boolean_edit_t){.p=&do_rpn,.v=true},
                    .ks=XK_R
                },{
                    .func=_boolean_edit_func,
                    .arg=&(_boolean_edit_t){.p=&var_menu,.v=true},
                    .ks=XK_A
                },{
                    .func=_boolean_edit_func,
                    .arg=&(_boolean_edit_t){.p=&rpn_see_stack,.v=!rpn_see_stack},
                    .ks=XK_S
                },{
                    .func=_input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_Start},
                    .ks=XK_Q
                },{
                    .func=_boolean_edit_func,
                    .arg=&(_boolean_edit_t){.p=&list_var,.v=true},
                    .ks=XK_L
                }},5);
                if(do_rpn){
                    puts("Type RPN with values comma delimited starting and ending with parenthesis.");
                    puts("Example: (1,2,+) is valid.");
                    fgets(input_str,INPUT_BUFFER_LEN,stdin);
                    rpnvs_e=RPNEvaluatorGetNumber(input_str,vl,&an_output,rpn_see_stack,RPN_EVAL_START_B,RPN_EVAL_END_B,RPN_EVAL_SEP);
                    //while((clear_stdin=getchar())!='\n'&&clear_stdin!=EOF);
                    printf("Status: %d Type: %s Result: ",rpnvs_e,VLNumberTypeStr(an_output.type));
                    VLNumberPrintNumber(an_output);
                    puts("");
                }
                while(var_menu){
                    printf("--------------------\n"
                    "Press a to add variable name with number.\n"
                    "Press r to remove variable name.\n"
                    "Press d to return go back.\n");
                    keypress_loop(xdo_obj->xdpy,(callback_t[3]){{
                        .func=_boolean_edit_func,
                        .arg=&(_boolean_edit_t){.p=&add_var,.v=true},
                        .ks=XK_A
                    },{
                        .func=_boolean_edit_func,
                        .arg=&(_boolean_edit_t){.p=&remove_var,.v=true},
                        .ks=XK_R
                    },{
                        .func=_boolean_edit_func,
                        .arg=&(_boolean_edit_t){.p=&var_menu,.v=false},
                        .ks=XK_D
                    }},3);
                    if(add_var){
                        input_str_end=0;
                        while(!input_str_end||input_str[0]=='\n'){
                            puts("Variable name to add:");
                            fgets(input_str,INPUT_BUFFER_LEN,stdin);
                            input_str_end=strchr(input_str,'\n');
                        }
                        var_str=char_string_slice(input_str,input_str_end-1);
                        input_str_end=0;
                        while(!input_str_end||input_str[0]=='\n'){
                            puts("Write value (Numbers can have (i)nt/(c)har/(l)ong/(d)ouble at the end to signify type. Default: (l)ong): ");
                            fgets(input_str,INPUT_BUFFER_LEN,stdin);
                            input_str_end=strchr(input_str,'\n');
                        }
                        num_str=char_string_slice(input_str,input_str_end-1);
                        if((an_opt_output=String_to_as_number_t(num_str)).exists){
                            switch(an_opt_output.v.type){
                                case VLNT_Char: VL_add_as_char(vl,&var_str,an_opt_output.v.c); break;
                                case VLNT_Int: VL_add_as_int(vl,&var_str,an_opt_output.v.i); break;
                                case VLNT_Long: VL_add_as_long(vl,&var_str,an_opt_output.v.l); break;
                                case VLNT_Double: VL_add_as_double(vl,&var_str,an_opt_output.v.d); break;
                                default: break;//Should not exist.
                            }
                            printf("Added number ");
                            VLNumberPrintNumber(an_opt_output.v);
                            printf(" to variable '%s'.\n",var_str);
                        }else{
                            printf("Invalid number parse. Did not add variable '%s'.\n",var_str);
                            free(var_str);
                        }
                        free(num_str);
                        add_var=false;
                    }
                    if(remove_var){
                        input_str_end=0;
                        while(!input_str_end||input_str[0]=='\n'){
                            puts("Variable name to remove:");
                            fgets(input_str,INPUT_BUFFER_LEN,stdin);
                            input_str_end=strchr(input_str,'\n');
                        }
                        var_str=char_string_slice(input_str,input_str_end-1);
                        printf("Removing variable %s.\n",var_str);
                        StringMap_as_number_erase_own(vl->sman,var_str);
                        remove_var=false;
                    }
                }
                if(list_var){
                    printf("Functions used (Updated since 8/31/2022):\n"
                    "abs,max,min,random_c,as_c,random_i,as_i,random_l,as_l,random_d,as_d\n"
                    "exp,exp2,log,log2,log10,pow,sqrt,cbrt,hypot,sin,cos,tan,ceil,floor,round,trunc\n"
                    "+,++,-,-m,--,*,/,%%,&,|,~,^,<<,>>,==,==u,!=,!=u,>,>u,<,<u,>=,>=u,<=,<=u,!,&&,||\n"
                    "Notes: Functions are nearly similar to c. Int/char/long are all signed.\n"
                    "For comparisons with unsigned integers, append u to ==,!-,>,<,>=, and <=\n"
                    "(Note: Not implemented for double types). -m is unary minus sign, cos/sin/tan uses degrees,\n"
                    "random_(c/i/l) for random numbers of their respective types.\n"
                    "random_d just inputs a number from 0 to 1. Function as_(c/i/l/d) is used for type casting.\n"
                    "Dividing by 0 with / or %% doesn't abort the program but fails any program/macro/rpn.\n"
                    "Variables currently set: ");
                    for(size_t i=0;i<vl->sman->MaxSize;i++) if(vl->sman->keys[i]) printf("%s, ",vl->sman->keys[i]);
                    puts("");
                    list_var=false;
                }
                if(menu_state==MS_Start) VL_free(vl);
                break;
            case MS_Done:
                goto done;
        }
    }
    done:
    RPNEvaluatorFree();
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
    rewind(f_obj);
    df_str=malloc(sizeof(char)*(str_len+1));//To include '\0'
    EXIT_IF_NULL(df_str,char*);
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
  XDestroyImage(image);
  XQueryColor(d,DefaultColormap(d,DefaultScreen(d)),color);
}
ProgramStatus parse_file(const char* path, xdo_t* xdo_obj, Config config, bool and_run){
    FILE* f_obj;
    char* file_str;
    f_obj=fopen(path,"r");
    if(!f_obj) return PS_ReadError;
    fseek(f_obj,0,SEEK_END);
    size_t str_len=ftell(f_obj);
    rewind(f_obj);
    file_str=malloc(sizeof(char)*(str_len+1));//To include '\0'
    EXIT_IF_NULL(file_str,char*);
    fread(file_str,str_len,1,f_obj);
    file_str[str_len]='\0';
    fclose(f_obj);
    trim_comments(&file_str);//So that the program doesn't process commented macros.
    macro_paster_t* mp=macro_paster_new();
    MacroProcessStatus mps=file_contains_macro_definitions(file_str,MACROS_DEF_START_B,MACROS_DEF_END_B);
    char* cmd_output;
    if(mps==MPS_NoMacros){
        cmd_output=file_str;
    }else if(mps==MPS_HasDefinitions){
        if(!macro_paster_process_macros(mp,file_str,MACROS_DEF_START_B,MACROS_DEF_END_B,MACRO_START_B,MACRO_END_B,MACRO_DEF_SEP,MACRO_VAR_SEP)){
            macro_paster_free(mp);
            free(file_str);
            return PS_MacroError;
        }
        if(!macro_paster_expand_macros(mp,file_str,MACROS_DEF_END_B,MACRO_START_B,MACRO_END_B,MACRO_VAR_SEP,&cmd_output)){
            macro_paster_free(mp);
            free(file_str);
            return PS_MacroError;
        }
        free(file_str);//Free since cmd_output is used instead.
    }else{//TODO: Built-in macros
        macro_paster_free(mp);
        free(file_str);
        return PS_MacroError;
    }
    macro_paster_free(mp);
    command_array_t* cmd_arr=command_array_new();
    macro_buffer_t* mb=macro_buffer_new(cmd_output,cmd_arr);
    while(true){
        macro_buffer_process_next(mb,config.debug_print_type==DBP_AllCommands);
        if(mb->token_i>mb->str_size) break;
    }
    if(!mb->parse_error){
        if(config.debug_print_type==DBP_AllCommands) command_array_print(cmd_arr,mb->vl); //Always print if no read errors after processing all commands.
        macro_buffer_str_id_check(mb);
    }
    if(!mb->parse_error&&and_run){
        bool run_success=run_program(cmd_arr,cmd_output,config,xdo_obj,mb->vl);
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
    delay_ns_t key_check_delay;
}shared_rs;
pthread_mutex_t input_mutex=PTHREAD_MUTEX_INITIALIZER;
void* keyboard_check_listener(void* srs_v){
    shared_rs* const srs_p=(shared_rs*)srs_v;
    pthread_mutex_lock(&input_mutex);
    delay_ns_t key_check_delay=srs_p->key_check_delay;
    Display* xdpy=((shared_rs*)srs_v)->xdo_obj->xdpy;
    int scr=DefaultScreen(xdpy);
    XGrabKey(xdpy,XKeysymToKeycode(xdpy,XK_Q),None,RootWindow(xdpy,scr),False,GrabModeAsync,GrabModeAsync);
    XFlush(xdpy);
    XEvent e={0};
    while(!srs_p->program_done){
        pthread_mutex_unlock(&input_mutex);
        usleep(key_check_delay);
        pthread_mutex_lock(&input_mutex);
        while(XPending(xdpy)){ //XPending doesn't make XNextEvent block if 0 events.
            XNextEvent(xdpy,&e); 
            if(e.type==KeyPress&&e.xkey.keycode==XKeysymToKeycode(xdpy,XK_Q)){
                puts("q key pressed. Stopping macro script.");
                srs_p->program_done=true;
            }
        }
    }
    XUngrabKey(xdpy,XKeysymToKeycode(xdpy,XK_Q),None,RootWindow(xdpy,scr));
    XFlush(xdpy);
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
int* cmd_i_stack=0;
int cmd_i_size=0;
void store_cmd_index(int i){
    cmd_i_size++;
    if(cmd_i_stack) cmd_i_stack=realloc(cmd_i_stack,sizeof(int)*cmd_i_size);
    else cmd_i_stack=malloc(sizeof(int));
    EXIT_IF_NULL(cmd_i_stack,int);
    cmd_i_stack[cmd_i_size-1]=i;
}
bool pop_cmd_index(int* cmd_i){
    if(cmd_i_stack){
        int popped_cmd_i=cmd_i_stack[--cmd_i_size];
        if(cmd_i_size){
            cmd_i_stack=realloc(cmd_i_stack,sizeof(int)*cmd_i_size);
            EXIT_IF_NULL(cmd_i_stack,int);
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
void empty_cmd_index(void){//If macro ends while having stack.
    free(cmd_i_stack);
    cmd_i_stack=0;
    cmd_i_size=0;
}
#include <X11/extensions/XTest.h>
//Based from xdo.c file using xdo_move_mouse_relative's functions, as XWarpPointer makes the mouse not click properly.
int custom_xdo_move_mouse_absolute(const xdo_t *xdo,int x,int y){
    int ret,last_mouse_x,last_mouse_y;
    xdo_get_mouse_location(xdo,&last_mouse_x,&last_mouse_y,0);
    ret=XTestFakeRelativeMotionEvent(xdo->xdpy,x-last_mouse_x,y-last_mouse_y,CurrentTime);
    XFlush(xdo->xdpy);
    return ret==0;
}
bool run_program(command_array_t* cmd_arr, const char* file_str, Config config, xdo_t* xdo_obj, VariableLoader_t* vl){
    puts("Press s to execute the macro. Press c to cancel.");
    {
        bool start_program;
        keypress_loop(xdo_obj->xdpy,(callback_t[2]){{
            .func=_boolean_edit_func,
            .arg=&(_boolean_edit_t){.p=&start_program,.v=true},
            .ks=XK_S
        },{
            .func=_boolean_edit_func,
            .arg=&(_boolean_edit_t){.p=&start_program,.v=false},
            .ks=XK_C
        }},2);
        if(!start_program){
            puts("Cancelling starting macro.");
            return false;
        }
    }
    int cmd_arr_len=command_array_count(cmd_arr),cmd_arr_i=0,stack_cmd_i;
    key_down_check_t* kdc=key_down_check_new();
    shared_rs srs=(shared_rs){.xdo_obj=xdo_obj,.program_done=false,.key_check_delay=config.key_check_delay};
    printf("Starting script in %ld microseconds (%f seconds)\n",config.init_delay,(float)config.init_delay/1000000);
    usleep(config.init_delay);
    puts("Running. Press q to stop the macro.");
    pthread_t keyboard_input_t;
    int ret=pthread_mutex_init(&input_mutex,PTHREAD_MUTEX_TIMED_NP);
    if(ret){
        fprintf(stderr,"Unable to create thread. Exiting program.\n");
        key_down_check_free(kdc);
        xdo_free(xdo_obj);
        return false;
    }
    ret=pthread_create(&keyboard_input_t,NULL,keyboard_check_listener,&srs);
    if(ret){
        fprintf(stderr,"Unable to create thread. Exiting program.\n");
        key_down_check_free(kdc);
        xdo_free(xdo_obj);
        return false;
    }
    struct timespec ts_begin,ts_diff,ts_usleep_before,ts_usleep_before_adj;
    timespec_get(&ts_begin,TIME_UTC);
    pthread_mutex_lock(&input_mutex);
    bool query_is_true,not_empty,vlsuccess,no_error=true;
    typedef long nano_sec;
    nano_sec time_after_last_usleep;
    nano_sec real_delay=0;//Adjust delay depending on time after commands and after sleeping.
    delay_ns_t adj_usleep;
    as_number_t an_output={0};
    XColor pc;
    coords_within_t coords_within;
    int x_mouse,y_mouse,x_mouse_store=0,y_mouse_store=0;
    char LastKey[LAST_CMD_BUFFER_LEN+1]={0},LastJump[LAST_CMD_BUFFER_LEN+1]={0},LastRepeat[LAST_CMD_BUFFER_LEN+1]={0},LastQuery[LAST_CMD_BUFFER_LEN+1]={0};
    int LastKey_n=0,LastJump_n=0,LastRepeat_n=0,LastQuery_n=0;
    int LastCommands[LAST_COMMANDS_LEN]={0}; //Counting indices as +1 so that 0 means nothing to print.
    int LastCommands_i=0;
    timespec_get(&ts_usleep_before,TIME_UTC);
    while(!srs.program_done){
        pthread_mutex_unlock(&input_mutex);
        query_is_true=false;
        command_t this_cmd=cmd_arr->cmds[cmd_arr_i];
        command_union_t cmd_u=cmd_arr->cmds[cmd_arr_i].cmd_u;
        CommandType cmd_type=cmd_arr->cmds[cmd_arr_i].type;
        int* rst_counter=0;
        if(config.debug_print_type>DBP_None||this_cmd.print_cmd){
            timespec_diff(&ts_begin,NULL,&ts_diff);
            printf("%s[%ld.%09ld] Command #%d/%d%s",config.debug_print_type==DBP_CommandNumber?CLEAR_TERM:"",ts_diff.tv_sec,ts_diff.tv_nsec,cmd_arr_i+1,cmd_arr->size,config.debug_print_type==DBP_CommandNumber?"":" ");
        }
        if(config.debug_print_type==DBP_CommandNumber){
            LastCommands[LastCommands_i]=cmd_arr_i+1;
            LastCommands_i=(LastCommands_i+1)%LAST_COMMANDS_LEN;
            const command_t cmd=cmd_arr->cmds[cmd_arr_i];
            const size_t cmd_off_b=cmd.start_cmd_p-file_str,cmd_off_e=cmd.end_cmd_p-file_str;
            size_t line_n,column_n;
            get_line_column_positions_p1(file_str,cmd.start_cmd_p,&line_n,&column_n);
            printf(" Line: %lu Column: %lu\n",line_n,column_n);
            #define CMD_HIGHLIGHT "\x1B[47;30;1m"
            char* str_high=print_string_highlight(file_str,file_str+cmd_off_b,file_str+cmd_off_e,CMD_HIGHLIGHT,"\x1B[0m");
            char* str_rv=string_read_view(str_high,str_high+cmd_off_b,4);
            printf("%s\x1B[H\x1B[%dB",str_rv,10);
            free(str_rv);
            free(str_high);
            printf("%7d %-30s '%s'\n%7d %-30s '%s'\n%7d %-30s '%s'\n"
                ,LastJump_n,"commands since last Jump:",LastJump,LastRepeat_n,"commands since last Repeat:",LastRepeat,LastQuery_n,"commands since last Query:",LastQuery);
            printf("%38s '%s'\n","Last Key/Mouse command used:",LastKey);
            printf("Last %d commands (Counting in ranges):\t",LAST_COMMANDS_LEN);
            int contiguous_start_i=0; //To print command# of contiguous range (Ex: 15,16,17,18 as 15-18)
            int contiguous_end_i=0;
            int contiguous_count=0;
            for(int i=LAST_COMMANDS_LEN;i>0;i--){
                const int cmd_i=LastCommands[(LAST_COMMANDS_LEN+LastCommands_i-1+i)%LAST_COMMANDS_LEN];
                if(contiguous_start_i-cmd_i==contiguous_count){
                    contiguous_end_i=cmd_i;
                    contiguous_count++;
                }else{
                    if(contiguous_start_i){
                        if(contiguous_start_i!=contiguous_end_i) printf("%4d-%-4d",contiguous_end_i,contiguous_start_i);
                        else if(contiguous_start_i) printf("%4d     ",contiguous_start_i);
                    }
                    contiguous_start_i=contiguous_end_i=cmd_i;
                    contiguous_count=1;
                }
            }
            if(contiguous_start_i){//Twice because the last range after the for loop doesn't appear.
                if(contiguous_start_i!=contiguous_end_i) printf("%4d-%-4d",contiguous_end_i,contiguous_start_i);
                else if(contiguous_start_i) printf("%4d     ",contiguous_start_i);
            }
            puts("");
        }
        #define cmdprintf(...) if((config.debug_print_type==DBP_AllCommands||this_cmd.print_cmd)) printf(__VA_ARGS__)
        #define PrintLastCommand(ToArray) if(config.debug_print_type==DBP_CommandNumber){ToArray##_n=0;memset(ToArray,0,LAST_CMD_BUFFER_LEN);strncpy(ToArray,this_cmd.start_cmd_p,(this_cmd.end_cmd_p-this_cmd.start_cmd_p+1<LAST_CMD_BUFFER_LEN)?(this_cmd.end_cmd_p-this_cmd.start_cmd_p+1):LAST_CMD_BUFFER_LEN);}
        LastJump_n++;
        LastRepeat_n++;
        LastQuery_n++;
        LastKey_n++;
        switch(cmd_type){
            case CMD_KeyStroke:
                switch(cmd_u.ks.key_state){
                    case IS_Down:
                        cmdprintf("Key down for %s\n",cmd_u.ks.key);
                        pthread_mutex_lock(&input_mutex);
                        xdo_send_keysequence_window_down(xdo_obj,CURRENTWINDOW,cmd_u.ks.key,0);
                        key_down_check_add(kdc,cmd_u.ks.key);
                        break;
                    case IS_Up:
                        cmdprintf("Key up for %s\n",cmd_u.ks.key);
                        pthread_mutex_lock(&input_mutex);
                        xdo_send_keysequence_window_up(xdo_obj,CURRENTWINDOW,cmd_u.ks.key,0);
                        key_down_check_remove(kdc,cmd_u.ks.key);
                        break;
                    case IS_Click:
                        cmdprintf("Key click for %s\n",cmd_u.ks.key);
                        pthread_mutex_lock(&input_mutex);
                        xdo_send_keysequence_window(xdo_obj,CURRENTWINDOW,cmd_u.ks.key,0);
                }
                pthread_mutex_unlock(&input_mutex);
                PrintLastCommand(LastKey);
                break;
            case CMD_Delay://Using timespec_get and timespec_diff (custom function) to try to get "precise delays"
                timespec_diff(&ts_usleep_before,&ts_usleep_before_adj,&ts_diff);
                time_after_last_usleep=ts_diff.tv_sec*NSEC_TO_SEC+ts_diff.tv_nsec;
                vlsuccess=ProcessVLCallback(vl,cmd_u.delay,&an_output);
                if(!vlsuccess){
                    //TODO
                }
                real_delay+=an_output.l*1000-time_after_last_usleep;
                adj_usleep=real_delay>0?((delay_ns_t)(real_delay/1000+(real_delay%1000>499?1:0))):0u;//0 and rounded nanoseconds.
                cmdprintf("Sleeping for %ld microseconds (Adjusted to %ld due to commands) \n",an_output.l,adj_usleep);
                {
                    int seconds=adj_usleep/MICSEC_TO_SEC;//Split into seconds so that it doesn't sleep when mouse moved over large seconds.
                    int left_over=adj_usleep%MICSEC_TO_SEC;
                    pthread_mutex_lock(&input_mutex);
                    while(seconds--&&!srs.program_done){
                        pthread_mutex_unlock(&input_mutex);
                        usleep(MICSEC_TO_SEC);
                        pthread_mutex_lock(&input_mutex);
                    }
                    pthread_mutex_unlock(&input_mutex);
                    usleep(left_over);
                }
                timespec_diff(&ts_usleep_before_adj,&ts_usleep_before,&ts_diff);
                real_delay-=ts_diff.tv_sec*NSEC_TO_SEC+ts_diff.tv_nsec;
                break;
            case CMD_RepeatEnd:
                if(cmd_u.repeat_end.counter_max){//Max counter non-zero.
                    rst_counter=&(cmd_arr->cmds[cmd_u.repeat_end.cmd_index].cmd_u.repeat_start.counter);
                    ++(*rst_counter);
                    cmdprintf("Jump to Command #%d until Counter %d/%d String ID#%d\n",cmd_u.repeat_end.cmd_index+2,*rst_counter,cmd_u.repeat_end.counter_max,cmd_u.repeat_end.str_index);
                    if(*rst_counter!=cmd_u.repeat_end.counter_max) cmd_arr_i=cmd_u.repeat_end.cmd_index;//Go back if not counter_max
                    else *rst_counter=0;//Reset to loop again.
                }else{//Loop forever otherwise.
                    cmdprintf("Jump to Command #%d (Loops forever) String ID#%d\n",cmd_u.repeat_end.cmd_index+2,cmd_u.repeat_end.str_index);
                    cmd_arr_i=cmd_u.repeat_end.cmd_index;
                }
                PrintLastCommand(LastRepeat);
                break;
            case CMD_RepeatResetCounters:
                cmdprintf("Resetting RepeatStart counters back to 0.\n");
                for(int i=0;i<cmd_arr->size;i++){
                    command_t* cmd=cmd_arr->cmds+i;
                    if(cmd->type==CMD_RepeatStart) cmd->cmd_u.repeat_start.counter=0;
                }
                break;
            case CMD_RepeatStart:
                cmdprintf("This is a loop counter (%d) String ID#%d\n",cmd_u.repeat_start.counter,cmd_u.repeat_start.str_index);
                break;
            case CMD_MouseClick:
                switch(cmd_u.mouse_click.mouse_state){
                    case IS_Down:
                        pthread_mutex_lock(&input_mutex);
                        cmdprintf("Mouse down (%d).\n",cmd_u.mouse_click.mouse_type);
                        xdo_mouse_down(xdo_obj,CURRENTWINDOW,cmd_u.mouse_click.mouse_type);
                        break;
                    case IS_Up:
                        pthread_mutex_lock(&input_mutex);
                        cmdprintf("Mouse up (%d).\n",cmd_u.mouse_click.mouse_type);
                        xdo_mouse_up(xdo_obj,CURRENTWINDOW,cmd_u.mouse_click.mouse_type);
                        break;
                    case IS_Click:
                        pthread_mutex_lock(&input_mutex);
                        cmdprintf("Mouse click (%d).\n",cmd_u.mouse_click.mouse_type);
                        xdo_click_window(xdo_obj,CURRENTWINDOW,cmd_u.mouse_click.mouse_type);
                }
                pthread_mutex_unlock(&input_mutex);
                PrintLastCommand(LastKey);
                break;
            case CMD_MoveMouse:
                cmdprintf("Mouse move at (%d,%d) (%s).",cmd_u.mouse_move.x,cmd_u.mouse_move.y,cmd_u.mouse_move.is_absolute?"absolute":"relative");
                pthread_mutex_lock(&input_mutex);
                if(cmd_u.mouse_move.is_absolute) custom_xdo_move_mouse_absolute(xdo_obj,cmd_u.mouse_move.x,cmd_u.mouse_move.y);
                else xdo_move_mouse_relative(xdo_obj,cmd_u.mouse_move.x,cmd_u.mouse_move.y);
                if(config.debug_print_type==DBP_AllCommands||this_cmd.print_cmd){
                    xdo_get_mouse_location(xdo_obj,&x_mouse,&y_mouse,0);//To check mouse location.
                    if(cmd_u.mouse_move.is_absolute) puts("");
                    else printf(" Mouse now at (%d,%d).\n",x_mouse,y_mouse);
                } 
                pthread_mutex_unlock(&input_mutex);
                PrintLastCommand(LastKey);
                break;
            case CMD_Exit:
                cmdprintf("Exit command issued. Exiting program now.\n");
                pthread_mutex_lock(&input_mutex);
                srs.program_done=true;
                pthread_mutex_unlock(&input_mutex);
                break;
            case CMD_Pass:
                cmdprintf("Pass command (does nothing).\n");
                break;
            case CMD_JumpTo:
                cmdprintf("Jump to Command #%d String ID#%d. %s",cmd_u.jump_to.cmd_index+2,cmd_u.jump_to.str_index,cmd_u.jump_to.store_index?"Will also store this command's index.":"\n");
                if(cmd_u.jump_to.store_index){
                    cmdprintf(" Storing command index to jump to later (#%d)\n",cmd_arr_i+2);
                    store_cmd_index(cmd_arr_i);
                }
                cmd_arr_i=cmd_u.jump_to.cmd_index;
                PrintLastCommand(LastJump);
                break;
            case CMD_JumpFrom:
                cmdprintf("Jump from command String ID#%d.\n",cmd_u.jump_from.str_index);
                break;
            case CMD_JumpBack:
                not_empty=pop_cmd_index(&stack_cmd_i);
                if(not_empty) cmd_arr_i=stack_cmd_i;
                if(config.debug_print_type==DBP_AllCommands||this_cmd.print_cmd){
                    printf("Jump to Command stored from stack. ");
                    if(not_empty) printf("Jumping back to command index #%d\n",stack_cmd_i+2);
                }
                if(!not_empty){
                    puts("Stack is empty! Aborting program.");
                    pthread_mutex_lock(&input_mutex);
                    srs.program_done=true;
                    no_error=false;
                    pthread_mutex_unlock(&input_mutex);
                }
                PrintLastCommand(LastJump);
                break;
            case CMD_SaveMouseCoords:
                cmdprintf("Saving current mouse coordinates.");
                pthread_mutex_lock(&input_mutex);
                xdo_get_mouse_location(xdo_obj,&x_mouse_store,&y_mouse_store,0);
                pthread_mutex_unlock(&input_mutex);
                cmdprintf(" x: %d y: %d\n",x_mouse_store,y_mouse_store);
                PrintLastCommand(LastKey);
                break;
            case CMD_LoadMouseCoords:
                cmdprintf("Moving to stored mouse coordinates x: %d y: %d\n",x_mouse_store,y_mouse_store);
                pthread_mutex_lock(&input_mutex);
                custom_xdo_move_mouse_absolute(xdo_obj,x_mouse_store,y_mouse_store);
                pthread_mutex_unlock(&input_mutex);
                PrintLastCommand(LastKey);
                break;
            case CMD_QueryComparePixel:
                cmdprintf("Don't skip next command if pixel at mouse matches r,g,b=%d,%d,%d with threshold of %d. ",cmd_u.pixel_compare.r,cmd_u.pixel_compare.g,cmd_u.pixel_compare.b,cmd_u.pixel_compare.thr);
                pthread_mutex_lock(&input_mutex);
                xdo_get_mouse_location(xdo_obj,&x_mouse,&y_mouse,0);
                get_pixel_color(xdo_obj->xdpy,x_mouse,y_mouse,&pc);
                pthread_mutex_unlock(&input_mutex);
                query_is_true=(abs((unsigned char)(pc.red>>8)-cmd_u.pixel_compare.r)<=cmd_u.pixel_compare.thr
                    &&abs((unsigned char)(pc.green>>8)-cmd_u.pixel_compare.g)<=cmd_u.pixel_compare.thr
                    &&abs((unsigned char)(pc.blue>>8)-cmd_u.pixel_compare.b)<=cmd_u.pixel_compare.thr
                );
                cmdprintf("Pixel does%s match (r,g,b=%d,%d,%d).\n",query_is_true?"":"n't",pc.red>>8,pc.green>>8,pc.blue>>8);
                PrintLastCommand(LastQuery);
                break;
            case CMD_QueryCompareCoords:
                ;const CompareCoords cc=cmd_u.compare_coords.cmp_flags;
                cmdprintf("Don't skip next command if mouse coordinate %c%c%s%d. ",(cc&CMP_Y)==CMP_Y?'y':'x',(cc&CMP_GT)==CMP_GT?'>':'<',(cc&CMP_W_EQ)==CMP_W_EQ?"=":"",cmd_u.compare_coords.var);
                pthread_mutex_lock(&input_mutex);
                xdo_get_mouse_location(xdo_obj,&x_mouse,&y_mouse,0);
                pthread_mutex_unlock(&input_mutex);
                ;const int mouse_compare=(cc&CMP_Y)==CMP_Y?y_mouse:x_mouse;
                if((cc&CMP_GT)==CMP_GT) query_is_true=(cc&CMP_W_EQ)==CMP_W_EQ?mouse_compare>=cmd_u.compare_coords.var:mouse_compare>cmd_u.compare_coords.var;
                else query_is_true=(cc&CMP_W_EQ)==CMP_W_EQ?mouse_compare<=cmd_u.compare_coords.var:mouse_compare<cmd_u.compare_coords.var;
                cmdprintf("Compare is %s.\n",query_is_true?"true":"false");
                PrintLastCommand(LastQuery);
                break;
            case CMD_QueryCoordsWithin:
                coords_within=cmd_u.coords_within;
                cmdprintf("Don't skip next command if mouse is within Top Left x:%d y:%d Bottom Right x:%d y:%d. ",coords_within.xl,coords_within.yl,coords_within.xh,coords_within.yh);
                pthread_mutex_lock(&input_mutex);
                xdo_get_mouse_location(xdo_obj,&x_mouse,&y_mouse,0);
                pthread_mutex_unlock(&input_mutex);
                query_is_true=x_mouse>=coords_within.xl&&x_mouse<=coords_within.xh&&y_mouse>=coords_within.yl&&y_mouse<=coords_within.yh;
                cmdprintf("It is %s within the box.\n",query_is_true?"":"not");
                PrintLastCommand(LastQuery);
                break;
            case CMD_InitVar:
                cmdprintf("Initialized variable string name '%s' of type %s of value ",cmd_u.init_var.variable,VLNumberTypeStr(cmd_u.init_var.as_number.type));
                if((config.debug_print_type==DBP_AllCommands||this_cmd.print_cmd)){
                    VLNumberPrintNumber(cmd_u.init_var.as_number);
                    puts("'");
                }
                break;
        }
        pthread_mutex_lock(&input_mutex);
        if(this_cmd.subtype!=CMDST_Query) ++cmd_arr_i;
        else{
            if(query_is_true){
                query_is_true=false;
                cmd_arr_i++;
            }else cmd_arr_i+=2;//Skip
        }
        if(cmd_arr_i==cmd_arr_len){
            srs.program_done=true;//To end the mouse_input_t thread loop as well.
        }
    }
    empty_cmd_index();
    timespec_diff(&ts_begin,NULL,&ts_diff);
    printf("%ld.%09ld seconds since macro script ran.\n",ts_diff.tv_sec,ts_diff.tv_nsec);
    pthread_mutex_unlock(&input_mutex);
    pthread_join(keyboard_input_t,NULL);
    key_down_check_key_up(kdc,xdo_obj,CURRENTWINDOW);
    key_down_check_free(kdc);
    return no_error;
}
