#include "macros.h"
#include "parser.h"
#include "key_down_check.h"
#include "variable_loader.h"
#include "rpn_evaluator.h"
#include "reserved_macro.h"
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
#include <poll.h>
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
    unsigned char rpn_decimals;
    bool rpn_stack_debug;
}Config;
const Config InitConfig={
    .init_delay=2000000
    ,.key_check_delay=1000
    ,.debug_print_type=DBP_None
    ,.rpn_decimals=5
    ,.rpn_stack_debug=false
};
inline static bool fgets_change(char* str,int buffer_len);
inline static bool write_to_config(const Config config);
inline static Config read_config_file(void);
inline static char* read_default_file(void);
inline static bool write_to_default_file(const char* path);
inline static void get_pixel_color(Display* d, int x, int y, XColor* color);
inline static ProgramStatus parse_file(const char* path, xdo_t* xdo_obj, Config config, bool and_run);
inline static void timespec_diff(const struct timespec* ts_begin,struct timespec* ts_end_clone,struct timespec* ts_diff);
inline static bool run_program(command_array_t* cmd_arr, const char* file_str, Config config, xdo_t* xdo_obj, VariableLoader_t* vl);
bool input_state_func(void* MS_v);
bool test_mouse_func(void* xdo_v);
inline static void clear_stdin();
typedef enum _MenuState{
    MS_Start,MS_EditConfig,MS_BuildFile,MS_RunFile,MS_MouseCoords,MS_RpnEvaluatorStart,MS_RpnEvaluator,MS_Done
}MenuState;
typedef struct ms_cont{
    MenuState* ms;
    MenuState v;
}ms_cont_t;
int main(void){
    if(access(CONFIG_FILE_F,F_OK)) if(!write_to_config(InitConfig)) return EXIT_FAILURE;
    xdo_t* xdo_obj=xdo_new(NULL);
    char* file_name_str=0;
    Config config;
    char input_str[INPUT_BUFFER_LEN+1];
    MenuState menu_state=MS_Start;
    bool also_run=false,do_rpn=false,rpn_see_stack=false,var_menu=false,add_var=false,remove_var=false,list_var=false;
    VariableLoader_t* vl=0;
    char* input_str_end=0,* var_str,* num_str;
    as_number_t an_output;
    as_number_opt_t an_opt_output;
    RPNValidStringE rpnvs_e;
    RPNEvaluatorInit();
    R_TS_Macro_Init();
    while(true){
        //printf(CLEAR_TERM);
        switch(menu_state){
            case MS_Start:
                also_run=false;
                printf(
                    "--------------------\n"
                    "Keystroke Player: Plays mouse/keyboard macro scripts and stops when the Escape key is pressed, or the script exits\n"
                    "Press c for Config\n"
                    "Press b to Build File\n"
                    "Press r to Build and Run File\n"
                    "Press t to Test coordinates of mouse and color\n"
                    "Press e to Test equations in Reverse Polish Notation (RPN)\n"
                    "Press q to Quit\n"
                    "Escape key toggles enabling/disabling keybinds outside of macro scripts\n\n"
                );
                keypress_loop(xdo_obj->xdpy,(callback_t[6]){{
                    .func=input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_Done},
                    .ks=XK_Q
                },{
                    .func=input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_EditConfig},
                    .ks=XK_C
                },{
                    .func=input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_BuildFile},
                    .ks=XK_B
                },{
                    .func=input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_RunFile},
                    .ks=XK_R
                },{
                    .func=input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_MouseCoords},
                    .ks=XK_T
                },{
                    .func=input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_RpnEvaluatorStart},
                    .ks=XK_E
                }},6);
                break;
            case MS_EditConfig:
                config=read_config_file();
                puts("Set value for init_delay (Microseconds before macro plays)");
                printf("Value right now is %lu (Enter nothing to skip): ",config.init_delay);
                clear_stdin();
                fgets(input_str,INPUT_BUFFER_LEN,stdin);
                if(input_str[0]!='\n') config.init_delay=strtol(input_str,NULL,10);
                while(true){
                    puts("Set value for key_check_delay (Microseconds to check key presses, up to 1 second or 1000000 microseconds)");
                    printf("Value right now is %lu (Enter nothing to skip): ",config.key_check_delay);
                    clear_stdin();
                    fgets(input_str,INPUT_BUFFER_LEN,stdin);
                    if(input_str[0]!='\n') config.key_check_delay=strtol(input_str,NULL,10);
                    if(config.key_check_delay<=1000000) break;
                }
                puts("Set value for decimals (0 to 255 number of decimals to show for a double)");
                printf("Value right now is %u (Enter nothing to skip):",config.rpn_decimals);
                clear_stdin();
                fgets(input_str,INPUT_BUFFER_LEN,stdin);
                if(input_str[0]!='\n') config.rpn_decimals=strtol(input_str,NULL,10);
                printf("The output is %u.\n",config.rpn_decimals);
                while(true){
                    puts("Set value for debug_commands (Prints debug commands when playing macro)");
                    puts("0 for no debug printing, 1 to print all commands (parsing and running program), 2 to print some command numbers and strings (clears terminal)");
                    printf("Value right now is %d (Enter 0/1/2 or nothing to skip): ",config.debug_print_type);
                    clear_stdin();
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
                while(true){
                    puts("Set value for rpn_stack_debug (Prints every operation of numbers and operators in the number stack)");
                    puts("0 to disable, 1 to enable.");
                    printf("Value right now is %d (Enter 0 or 1 or nothing to skip): ",config.rpn_stack_debug);
                    clear_stdin();
                    fgets(input_str,INPUT_BUFFER_LEN,stdin);
                    switch(input_str[0]){
                        case '0': config.rpn_stack_debug=false; goto rpn_stack_debug_valid;
                        case '1': config.rpn_stack_debug=true; goto rpn_stack_debug_valid;
                        case '\n': goto rpn_stack_debug_valid;
                        default: break;
                    }
                }
                rpn_stack_debug_valid:
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
                clear_stdin();
                fgets(input_str,INPUT_BUFFER_LEN,stdin);
                if(!strcmp(input_str,"c\n")){
                    free(file_name_str);
                    menu_state=MS_Start;
                    break;
                }
                if(input_str[0]!='\n'){
                    if(fgets_change(input_str,INPUT_BUFFER_LEN)) printf("Warning: String has been truncated to %d characters.\n",INPUT_BUFFER_LEN);
                }else{//Enter pressed.
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
                puts("Press y to build/run again or q to return to menu.\n");
                keypress_loop(xdo_obj->xdpy,(callback_t[2]){{
                    .func=input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_Start},
                    .ks=XK_Q
                },{
                    .func=CallbackEndLoop,
                    .arg=0,//Previous state used.
                    .ks=XK_Y
                }},2);
                break;
            case MS_MouseCoords:
                puts("Press t to test mouse/color coordinates. Press q to quit.\n");
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
                config=read_config_file();
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
                "Press q to quit.\n\n",rpn_see_stack?"on":"off");
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
                    .func=input_state_func,
                    .arg=&(ms_cont_t){.ms=&menu_state,.v=MS_Start},
                    .ks=XK_Q
                },{
                    .func=_boolean_edit_func,
                    .arg=&(_boolean_edit_t){.p=&list_var,.v=true},
                    .ks=XK_L
                }},5);
                if(do_rpn){
                    puts("Type RPN with values comma delimited tokens enclosed with parenthesis.");
                    puts("Example: (1,2,+,3,4,+,*) is valid.");
                    clear_stdin();
                    fgets(input_str,INPUT_BUFFER_LEN,stdin);
                    rpnvs_e=RPNEvaluatorEvaluate(input_str,vl,&an_output,rpn_see_stack,true,RPN_EVAL_START_B,RPN_EVAL_END_B,RPN_EVAL_SEP);
                    //while((clear_stdin=getchar())!='\n'&&clear_stdin!=EOF);
                    printf("Status: %d Type: %s Result: ",rpnvs_e,VLNumberTypeStr(an_output.type));
                    VLNumberPrintNumber(an_output,config.rpn_decimals);
                    puts("");
                }
                while(var_menu){
                    printf("--------------------\n"
                    "Press a to add variable name with number.\n"
                    "Press r to remove variable name.\n"
                    "Press q to return go back.\n\n");
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
                        .ks=XK_Q
                    }},3);
                    if(add_var){
                        input_str_end=0;
                        while(!input_str_end||input_str[0]=='\n'){
                            puts("Variable name to add:");
                            clear_stdin();
                            fgets(input_str,INPUT_BUFFER_LEN,stdin);
                            input_str_end=strchr(input_str,'\n');
                        }
                        var_str=char_string_slice(input_str,input_str_end-1);
                        input_str_end=0;
                        while(!input_str_end||input_str[0]=='\n'){
                            puts("Write value (Numbers can have (i)nt/(c)har/(l)ong/(d)ouble at the end to signify type. Default: (l)ong): ");
                            clear_stdin();
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
                            VLNumberPrintNumber(an_opt_output.v,config.rpn_decimals);
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
                            clear_stdin();
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
                    printf("Functions used (Updated since 9/17/2022):\n"
                    "abs,max(u),min(u),random_c,as_c,random_i,as_i,random_l,as_l,random_d,as_d\n"
                    "exp,exp2,log,log2,log10,pow,sqrt,cbrt,hypot,sin(d),cos(d),tan(d),asin(d),acos(d),atan(d),ceil,floor,round,trunc\n"
                    "+,++,-,-m,--,*,/,/u,%%,%%u,&,|,~,^,<<,<<u,>>,>>u,==,==u,!=,!=u,>,>u,<,<u,>=,>=u,<=,<=u,!,&&,||\n"
                    "Notes: Functions/operators are nearly similar to c. Int/char/long are all signed.\n"
                    "Unlike in C, ++ and -- doesn't increment/decrement the variable value after usage.\n"
                    "For comparisons with unsigned integers, append u to ==,!-,>,<,>=,<=,/, and %%\n"
                    "(Note: Not implemented for double types). -m is unary minus sign, trigonometric functions can use degrees\n"
                    "if appended with d (Ex: sind,atand...). minu and maxu compares unsigned integers.\n"
                    "random_(c/i/l) for random numbers of their respective types.\n"
                    "random_d just outputs a double from 0 to 1. Function as_(c/i/l/d) is used for type casting.\n"
                    "Dividing by 0 with /(u) or %%(u) doesn't abort the program, but terminates the macro.\n"
                    "Using a negative signed number for the second operation in >>(u) and <<(u) also terminates the macro.\n"
                    "Note that it does not abort the program in c, but mixed signedness is not implemented in this program.\n"
                    "Custom variables currently set: ");
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
    R_TS_Macro_Free();
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
    ts_macro_paster_t* mp=ts_macro_paster_new();
    R_TS_Macro_ResetState();
    MacroProcessStatus mps=file_contains_any_macros(file_str,MACROS_DEF_START_B,MACROS_DEF_END_B,MACRO_START_B);
    char* cmd_output;
    if(mps==MPS_NoMacros){
        cmd_output=file_str;
    }else if(mps==MPS_HasDefinitions){
        if(!ts_macro_paster_process_macros(mp,file_str,MACROS_DEF_START_B,MACROS_DEF_END_B,MACRO_START_B,MACRO_END_B,MACRO_DEF_SEP,MACRO_VAR_SEP)){
            ts_macro_paster_free(mp);
            free(file_str);
            return PS_MacroError;
        }
        if(!ts_macro_paster_expand_macros(mp,false,file_str,MACROS_DEF_END_B,MACRO_START_B,MACRO_END_B,MACRO_VAR_SEP,&cmd_output)){
            ts_macro_paster_free(mp);
            free(file_str);
            return PS_MacroError;
        }
        free(file_str);//Free since cmd_output is used instead.
    }else if(mps==MPS_HasReservedMacros){
        if(!ts_macro_paster_expand_macros(mp,true,file_str,MACROS_DEF_END_B,MACRO_START_B,MACRO_END_B,MACRO_VAR_SEP,&cmd_output)){
            ts_macro_paster_free(mp);
            free(file_str);
            return PS_MacroError;
        }
        free(file_str);//Free since cmd_output is used instead.
    }else{
        ts_macro_paster_free(mp);
        free(file_str);
        return PS_MacroError;
    }
    ts_macro_paster_free(mp);
    command_array_t* cmd_arr=command_array_new();
    macro_buffer_t* mb=macro_buffer_new(cmd_output,cmd_arr);
    while(true){
        macro_buffer_process_next(mb,config.debug_print_type==DBP_AllCommands,config.rpn_stack_debug);
        if(mb->token_i>mb->str_size) break;
    }
    if(!mb->parse_error){
        if(config.debug_print_type==DBP_AllCommands) command_array_print(cmd_arr,mb->vl,config.rpn_decimals); //Always print if no read errors after processing all commands.
        macro_buffer_extra_checks(mb,mb->vl);
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
    km_grabs_t* kmg;
    delay_ns_t key_check_delay;
    bool program_done;
    bool do_wait_cond;
    bool wait_cond_inv;
    bool wait_for_key;
    bool wait_for_button;
    int wait_button;
}shared_rs;
pthread_mutex_t input_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wait_cond=PTHREAD_COND_INITIALIZER;
void* keyboard_check_listener(shared_rs* srs_p){
    pthread_mutex_lock(&input_mutex);
    km_grabs_t* kmg=srs_p->kmg;
    Display* xdpy=srs_p->xdo_obj->xdpy;
    int scr=DefaultScreen(xdpy);
    XGrabKey(xdpy,XKeysymToKeycode(xdpy,XK_Escape),None,RootWindow(xdpy,scr),False,GrabModeAsync,GrabModeAsync);
    XFlush(xdpy);
    XEvent e={0};
    while(!srs_p->program_done){
        const delay_ns_t key_check_delay=srs_p->key_check_delay;
        pthread_mutex_unlock(&input_mutex);
        usleep(key_check_delay);
        pthread_mutex_lock(&input_mutex);
        while(XPending(xdpy)){ //XPending doesn't make XNextEvent block if 0 events.
            XNextEvent(xdpy,&e);
            if(e.type==KeyPress||e.type==KeyRelease){
                const KeyCode this_keycode=e.xkey.keycode;
                const bool is_pressed=(e.type==KeyPress);
                if(this_keycode==XKeysymToKeycode(xdpy,XK_Escape)){
                    puts("Escape key pressed. Stopping macro script.");
                    srs_p->program_done=true;
                    if(srs_p->do_wait_cond)//If exiting while wait condition was active in the main thread.
                        pthread_cond_signal(&wait_cond);
                    break;
                }
                for(int i=0;i<kmg->size;i++)
                    if(XKeysymToKeycode(xdpy,kmg->ks_arr[i].keysym)==this_keycode)
                        kmg->ks_pressed[i]=is_pressed;
            }
            if(e.type==ButtonPress||e.type==ButtonRelease)
                km_grabs_set_bpressed(srs_p->kmg,e.xbutton.button,(e.type==ButtonPress));
        }
        if(srs_p->wait_for_key&&(kmg->ks_pressed[kmg->size-1]^srs_p->wait_cond_inv))
            pthread_cond_signal(&wait_cond); //Last key would be from CMD_WaitUntilKey if do_wait_cond is set.
        if(srs_p->wait_for_button&&(km_grabs_get_bpressed(srs_p->kmg,srs_p->wait_button)^srs_p->wait_cond_inv))
            pthread_cond_signal(&wait_cond);
    }
    XUngrabKey(xdpy,XKeysymToKeycode(xdpy,XK_Escape),None,RootWindow(xdpy,scr));
    XFlush(xdpy);
    pthread_mutex_unlock(&input_mutex);
    pthread_exit(NULL);
}
//Get time elapsed since ts_begin after calling this function. Assuming ts_begin was before ts_end. Returns time to ts_diff (optional).
//ts_end_clone (optional) for another timespec_get from ts_end.
void timespec_diff(const struct timespec* ts_begin,struct timespec* ts_end_clone,struct timespec* ts_diff){
    struct timespec ts_end;
    timespec_get(&ts_end,TIME_UTC);
    if(ts_end_clone) *ts_end_clone=ts_end;
    if(!ts_diff) return;
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
    puts("Press s to execute the macro. Press q to cancel.\n");
    {
        bool start_program;
        keypress_loop(xdo_obj->xdpy,(callback_t[2]){{
            .func=_boolean_edit_func,
            .arg=&(_boolean_edit_t){.p=&start_program,.v=true},
            .ks=XK_S
        },{
            .func=_boolean_edit_func,
            .arg=&(_boolean_edit_t){.p=&start_program,.v=false},
            .ks=XK_Q
        }},2);
        if(!start_program){
            puts("Cancelling starting macro.");
            return false;
        }
    }
    int cmd_arr_len=command_array_count(cmd_arr),cmd_arr_i=0,stack_cmd_i;
    key_down_check_t* kdc=key_down_check_new();
    shared_rs srs=(shared_rs){.xdo_obj=xdo_obj,.program_done=false,.do_wait_cond=false,.wait_cond_inv=false,.wait_for_key=false,.wait_for_button=false,.key_check_delay=config.key_check_delay,.kmg=km_grabs_new(xdo_obj)};
    printf("Starting script in %ld microseconds (%f seconds)\n",config.init_delay,(float)config.init_delay/1000000);
    puts("Press Escape Key to stop the macro.");
    usleep(config.init_delay);
    pthread_t keyboard_input_t;
    int ret=pthread_mutex_init(&input_mutex,PTHREAD_MUTEX_TIMED_NP);
    if(ret){
        fprintf(stderr,ERR("Unable to create thread. Exiting program.\n"));
        key_down_check_free(kdc);
        xdo_free(xdo_obj);
        km_grabs_free(srs.kmg);
        return false;
    }
    ret=pthread_create(&keyboard_input_t,NULL,(void*(*)(void*))keyboard_check_listener,&srs);
    if(ret){
        fprintf(stderr,ERR("Unable to create thread. Exiting program.\n"));
        key_down_check_free(kdc);
        xdo_free(xdo_obj);
        km_grabs_free(srs.kmg);
        return false;
    }
    struct timespec ts_begin,ts_diff,ts_usleep_before,ts_usleep_before_adj,ts_rm_delay;
    timespec_get(&ts_begin,TIME_UTC);
    bool query_is_true,not_empty,no_error=true;
    typedef long nano_sec;
    nano_sec time_after_last_usleep;
    nano_sec real_delay=0;//Adjust delay depending on time after commands and after sleeping.
    delay_ns_t adj_usleep;
    as_number_t an_output[4]={0};
    XColor pc;
    coords_within_t coords_within;
    pixel_compare_t pixel_compare;
    int x_mouse,y_mouse,x_mouse_store=0,y_mouse_store=0;
    char LastKey[LAST_CMD_BUFFER_LEN+1]={0},LastJump[LAST_CMD_BUFFER_LEN+1]={0},LastRepeat[LAST_CMD_BUFFER_LEN+1]={0},LastQuery[LAST_CMD_BUFFER_LEN+1]={0};
    int LastKey_n=0,LastJump_n=0,LastRepeat_n=0,LastQuery_n=0;
    int LastCommands[LAST_COMMANDS_LEN]={0}; //Counting indices as +1 so that 0 means nothing to print.
    int LastCommands_i=0;
    timespec_get(&ts_usleep_before,TIME_UTC);
    RPNEvaluatorAssignVar("@ci_last",(as_number_t){.i=cmd_arr_len,.type=VLNT_Int});
    pthread_mutex_lock(&input_mutex);
    char* print_string_copy,** print_string_rpn_results;
    while(!srs.program_done){
        pthread_mutex_unlock(&input_mutex);
        timespec_diff(&ts_begin,NULL,&ts_diff);
        RPNEvaluatorAssignVar("@time_s",(as_number_t){.l=ts_diff.tv_sec,.type=VLNT_Long});
        RPNEvaluatorAssignVar("@time_ns",(as_number_t){.l=ts_diff.tv_nsec,.type=VLNT_Long});
        RPNEvaluatorAssignVar("@ci_prev",RPNEvaluatorReadVar("@ci_now").value);
        RPNEvaluatorAssignVar("@ci_now",(as_number_t){.i=cmd_arr_i+1,.type=VLNT_Int});
        query_is_true=false;
        command_t this_cmd=cmd_arr->cmds[cmd_arr_i];
        command_union_t cmd_u=cmd_arr->cmds[cmd_arr_i].cmd_u;
        CommandType cmd_type=cmd_arr->cmds[cmd_arr_i].type;
        int* rst_counter=0;
        if(config.debug_print_type>DBP_None||this_cmd.print_cmd){
            printf("%s[%ld.%09ld] Command #%d/%d%s",config.debug_print_type==DBP_CommandNumber?CLEAR_TERM:"",ts_diff.tv_sec,ts_diff.tv_nsec,cmd_arr_i+1,cmd_arr->size,config.debug_print_type==DBP_CommandNumber?"":"\n");
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
        #define RuntimeExitIfProcessVLFalse(F)\
        if(!F){\
            pthread_mutex_lock(&input_mutex);\
            srs.program_done=true;\
            no_error=false;\
            pthread_mutex_unlock(&input_mutex);\
            break;\
        } (void)0
        #define DoRuntimeError()\
        pthread_mutex_lock(&input_mutex);\
        srs.program_done=true;\
        pthread_mutex_unlock(&input_mutex);\
        no_error=false;\
        (void)0
        LastJump_n++;
        LastRepeat_n++;
        LastQuery_n++;
        LastKey_n++;
        switch(cmd_type){
            case CMD_KeyStroke:
                pthread_mutex_lock(&input_mutex);
                km_grabs_ktoggle(srs.kmg,false);//Don't send key pressed events to input thread.
                switch(cmd_u.auto_ks.key_state){
                    case IS_Down:
                        cmdprintf("Key down for %s\n",cmd_u.auto_ks.key);
                        xdo_send_keysequence_window_down(xdo_obj,CURRENTWINDOW,cmd_u.auto_ks.key,0);
                        key_down_check_add(kdc,cmd_u.auto_ks.key);
                        break;
                    case IS_Up:
                        cmdprintf("Key up for %s\n",cmd_u.auto_ks.key);
                        xdo_send_keysequence_window_up(xdo_obj,CURRENTWINDOW,cmd_u.auto_ks.key,0);
                        key_down_check_remove(kdc,cmd_u.auto_ks.key);
                        break;
                    case IS_Click:
                        cmdprintf("Key click for %s\n",cmd_u.auto_ks.key);
                        xdo_send_keysequence_window(xdo_obj,CURRENTWINDOW,cmd_u.auto_ks.key,0);
                }
                km_grabs_ktoggle(srs.kmg,true);
                pthread_mutex_unlock(&input_mutex);
                PrintLastCommand(LastKey);
                break;
            case CMD_Delay://Using timespec_get and timespec_diff (custom function) to try to get "precise delays"
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,cmd_u.delay.callback,&an_output[0]));
                an_output[0]=VLNumberCast(an_output[0],VLNT_Long);
                timespec_diff(&ts_usleep_before,&ts_usleep_before_adj,&ts_diff);
                time_after_last_usleep=ts_diff.tv_sec*NSEC_TO_SEC+ts_diff.tv_nsec;
                real_delay+=an_output[0].l*cmd_u.delay.delay_mult*1000-time_after_last_usleep;
                adj_usleep=real_delay>0?((delay_ns_t)(real_delay/1000+(real_delay%1000>499?1:0))):0u;//0 and rounded nanoseconds.
                cmdprintf("Sleeping for %ld microseconds (Adjusted to %ld due to commands) \n",an_output[0].l*cmd_u.delay.delay_mult,adj_usleep);
                {
                    int seconds=adj_usleep/MICSEC_TO_SEC;//Split into seconds so that it doesn't sleep when mouse moved over large seconds.
                    int left_over=adj_usleep%MICSEC_TO_SEC;
                    pthread_mutex_lock(&input_mutex);
                    while(seconds--&&!srs.program_done){
                        pthread_mutex_unlock(&input_mutex);
                        sleep(1);
                        pthread_mutex_lock(&input_mutex);
                    }
                    pthread_mutex_unlock(&input_mutex);
                    usleep(left_over);
                }
                timespec_diff(&ts_usleep_before_adj,&ts_usleep_before,&ts_diff);
                real_delay-=ts_diff.tv_sec*NSEC_TO_SEC+ts_diff.tv_nsec;
                break;
            case CMD_RepeatEnd:
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,cmd_u.repeat_end.counter,&an_output[0]));
                an_output[0]=VLNumberCast(an_output[0],VLNT_Int);
                if(an_output[0].i){//Max counter non-zero.
                    rst_counter=&(cmd_arr->cmds[cmd_u.repeat_end.cmd_index].cmd_u.repeat_start.counter);
                    ++(*rst_counter);
                    cmdprintf("Jump to Command #%d until Counter %d/%d String ID#%d\n",cmd_u.repeat_end.cmd_index+2,*rst_counter,an_output[0].i,cmd_u.repeat_end.str_index);
                    if(*rst_counter!=an_output[0].i) cmd_arr_i=cmd_u.repeat_end.cmd_index;//Go back if not counter_max
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
                pthread_mutex_lock(&input_mutex);
                km_grabs_btoggle(srs.kmg,false); //Disable mouse grabs.
                switch(cmd_u.mouse_click.mouse_state){
                    case IS_Down:
                        cmdprintf("Mouse down (%d).\n",cmd_u.mouse_click.mouse_type);
                        xdo_mouse_down(xdo_obj,CURRENTWINDOW,cmd_u.mouse_click.mouse_type);
                        break;
                    case IS_Up:
                        cmdprintf("Mouse up (%d).\n",cmd_u.mouse_click.mouse_type);
                        xdo_mouse_up(xdo_obj,CURRENTWINDOW,cmd_u.mouse_click.mouse_type);
                        break;
                    case IS_Click:
                        cmdprintf("Mouse click (%d).\n",cmd_u.mouse_click.mouse_type);
                        xdo_click_window(xdo_obj,CURRENTWINDOW,cmd_u.mouse_click.mouse_type);
                }
                km_grabs_btoggle(srs.kmg,true);
                pthread_mutex_unlock(&input_mutex);
                PrintLastCommand(LastKey);
                break;
            case CMD_MoveMouse:
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,cmd_u.mouse_move.x_cb,&an_output[0]));
                an_output[0]=VLNumberCast(an_output[0],VLNT_Int);
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,cmd_u.mouse_move.y_cb,&an_output[1]));
                an_output[1]=VLNumberCast(an_output[1],VLNT_Int);
                cmdprintf("Mouse move at (%d,%d) (%s).",an_output[0].i,an_output[1].i,cmd_u.mouse_move.is_absolute?"absolute":"relative");
                pthread_mutex_lock(&input_mutex);
                if(cmd_u.mouse_move.is_absolute) custom_xdo_move_mouse_absolute(xdo_obj,an_output[0].i,an_output[1].i);
                else xdo_move_mouse_relative(xdo_obj,an_output[0].i,an_output[1].i);
                if(config.debug_print_type==DBP_AllCommands||this_cmd.print_cmd){
                    xdo_get_mouse_location(xdo_obj,&x_mouse,&y_mouse,0);//To check mouse location.
                    if(cmd_u.mouse_move.is_absolute) puts("");
                    else printf(" Mouse now at (%d,%d).\n",x_mouse,y_mouse);
                } 
                pthread_mutex_unlock(&input_mutex);
                PrintLastCommand(LastKey);
                break;
            case CMD_Exit:
                cmdprintf("Exit command issued. Exiting program.\n");
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
            case CMD_JumpToIndex:
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,cmd_u.jump_to_index.jump_cb,&an_output[0]));
                an_output[0]=VLNumberCast(an_output[0],VLNT_Int);
                cmdprintf("Jump to Command Index (%s) by %d\n",cmd_u.jump_to_index.is_absolute?"absolute":"relative",an_output[0].i);
                an_output[0].i=cmd_u.jump_to_index.is_absolute?an_output[0].i:(cmd_arr_i+an_output[0].i+1);
                //Check bounds of index.
                if(an_output[0].i<1||an_output[0].i>cmd_arr_len){
                    fprintf(stderr,ERR("Command Index (%d) is out of bounds from 1 to %d. Exiting Program!\n"),an_output[0].i,cmd_arr_len);
                    DoRuntimeError();
                    break;
                }
                cmd_arr_i=an_output[0].i-2; //-2 because of cmd_arr_i++ and the program parses from 1 to cmd_arr_len. 
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
                    fprintf(stderr,ERR("Stack is empty! Aborting program."));
                    DoRuntimeError();
                    break;
                }
                PrintLastCommand(LastJump);
                break;
            case CMD_SaveMouseCoords:
                cmdprintf("Saving current mouse coordinates.");
                pthread_mutex_lock(&input_mutex);
                xdo_get_mouse_location(xdo_obj,&x_mouse_store,&y_mouse_store,0);
                pthread_mutex_unlock(&input_mutex);
                cmdprintf(" x: %d y: %d\n",x_mouse_store,y_mouse_store);
                RPNEvaluatorAssignVar("@mma_x",(as_number_t){.i=x_mouse_store,.type=VLNT_Int});
                RPNEvaluatorAssignVar("@mma_y",(as_number_t){.i=y_mouse_store,.type=VLNT_Int});
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
                pixel_compare=cmd_u.pixel_compare;
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,pixel_compare.r_cb,&an_output[0]));
                an_output[0]=VLNumberCast(an_output[0],VLNT_Char);
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,pixel_compare.g_cb,&an_output[1]));
                an_output[1]=VLNumberCast(an_output[1],VLNT_Char);
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,pixel_compare.b_cb,&an_output[2]));
                an_output[2]=VLNumberCast(an_output[2],VLNT_Char);
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,pixel_compare.thr_cb,&an_output[3]));
                an_output[3]=VLNumberCast(an_output[3],VLNT_Char);
                cmdprintf("If pixel at mouse matches r,g,b=%d,%d,%d with threshold of %d, jump by %d. Else, jump by %d.\n",an_output[0].i,an_output[1].i,an_output[2].i,an_output[3].i,this_cmd.query_details.jump_e,this_cmd.query_details.jump_ne);
                pthread_mutex_lock(&input_mutex);
                xdo_get_mouse_location(xdo_obj,&x_mouse,&y_mouse,0);
                get_pixel_color(xdo_obj->xdpy,x_mouse,y_mouse,&pc);
                pthread_mutex_unlock(&input_mutex);
                query_is_true=(abs((pc.red>>8)-an_output[0].c)<=an_output[3].i
                    &&abs((pc.green>>8)-an_output[1].c)<=an_output[3].i
                    &&abs((pc.blue>>8)-an_output[2].c)<=an_output[3].i
                );
                cmdprintf("Pixel does%s match (r,g,b=%d,%d,%d).\n",query_is_true?"":"n't",pc.red>>8,pc.green>>8,pc.blue>>8);
                PrintLastCommand(LastQuery);
                break;
            case CMD_QueryCompareCoords:
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,cmd_u.compare_coords.var_callback,&an_output[0]));
                an_output[0]=VLNumberCast(an_output[0],VLNT_Int);
                cmdprintf("If mouse coordinate %c%c%s%d, jump by %d. Else, jump by %d.\n",(cmd_u.compare_coords.cmp_flags&CMP_Y)==CMP_Y?'y':'x',(cmd_u.compare_coords.cmp_flags&CMP_GT)==CMP_GT?'>':'<',(cmd_u.compare_coords.cmp_flags&CMP_W_EQ)==CMP_W_EQ?"=":"",an_output[0].i,this_cmd.query_details.jump_e,this_cmd.query_details.jump_ne);
                pthread_mutex_lock(&input_mutex);
                xdo_get_mouse_location(xdo_obj,&x_mouse,&y_mouse,0);
                pthread_mutex_unlock(&input_mutex);
                const int mouse_compare=(cmd_u.compare_coords.cmp_flags&CMP_Y)==CMP_Y?y_mouse:x_mouse;
                if((cmd_u.compare_coords.cmp_flags&CMP_GT)==CMP_GT) query_is_true=(cmd_u.compare_coords.cmp_flags&CMP_W_EQ)==CMP_W_EQ?mouse_compare>=an_output[0].i:mouse_compare>an_output[0].i;
                else query_is_true=(cmd_u.compare_coords.cmp_flags&CMP_W_EQ)==CMP_W_EQ?mouse_compare<=an_output[0].i:mouse_compare<an_output[0].i;
                cmdprintf("Compare is %s.\n",query_is_true?"true":"false");
                PrintLastCommand(LastQuery);
                break;
            case CMD_QueryCoordsWithin:
                coords_within=cmd_u.coords_within;
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,coords_within.xl_cb,&an_output[0]));
                an_output[0]=VLNumberCast(an_output[0],VLNT_Int);
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,coords_within.yl_cb,&an_output[1]));
                an_output[1]=VLNumberCast(an_output[1],VLNT_Int);
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,coords_within.xh_cb,&an_output[2]));
                an_output[2]=VLNumberCast(an_output[2],VLNT_Int);
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,coords_within.yh_cb,&an_output[3]));
                an_output[3]=VLNumberCast(an_output[3],VLNT_Int);
                cmdprintf("If mouse is within Top Left x:%d y:%d Bottom Right x:%d y:%d, jump by %d. Else, jump by %d.\n",an_output[0].i,an_output[1].i,an_output[2].i,an_output[3].i,this_cmd.query_details.jump_e,this_cmd.query_details.jump_ne);
                pthread_mutex_lock(&input_mutex);
                xdo_get_mouse_location(xdo_obj,&x_mouse,&y_mouse,0);
                pthread_mutex_unlock(&input_mutex);
                query_is_true=x_mouse>=an_output[0].i&&x_mouse<=an_output[2].i&&y_mouse>=an_output[1].i&&y_mouse<=an_output[3].i;
                cmdprintf("It is %s within the box.\n",query_is_true?"":"not");
                PrintLastCommand(LastQuery);
                break;
            case CMD_QueryRPNEval:
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,cmd_u.rpn_eval,&an_output[0]));
                an_output[0]=VLNumberCast(an_output[0],VLNT_Int);
                cmdprintf("If RPN string '%s' is non-zero, jump by %d. Else, jump by %d.\n",VL_get_callback(vl,cmd_u.rpn_eval)->args.an_rpn.rpn_str,this_cmd.query_details.jump_e,this_cmd.query_details.jump_ne);
                query_is_true=an_output[0].i;
                cmdprintf("It is %szero.\n",query_is_true?"non-":"");
                PrintLastCommand(LastQuery);
                break;
            case CMD_QueryKeyPress:
                cmdprintf("If Key '%s' is pressed, jump by %d. Else, jump by %d.\n",cmd_u.qkey_pressed.key,this_cmd.query_details.jump_e,this_cmd.query_details.jump_ne);
                if(!km_grabs_kgrab_exist(srs.kmg,cmd_u.grab_key))
                    printf(ERR("Warning: GrabKey command has not been initialized for key '%s'.\n"),cmd_u.qkey_pressed.key);
                pthread_mutex_lock(&input_mutex);
                query_is_true=km_grabs_get_kpressed(srs.kmg,cmd_u.grab_key);
                pthread_mutex_unlock(&input_mutex);
                cmdprintf("It is %spressed.\n",query_is_true?"":"not ");
                PrintLastCommand(LastQuery);
                break;
            case CMD_QueryButtonPress:
                cmdprintf("If Button %d is pressed, jump by %d. Else, jump by %d.\n",cmd_u.qbutton_pressed.button,this_cmd.query_details.jump_e,this_cmd.query_details.jump_ne);
                if(!km_grabs_bgrab_exist(srs.kmg,cmd_u.grab_button.button))
                    printf(ERR("Warning: GrabButton command has not been initialized for button %d.\n"),cmd_u.qbutton_pressed.button);
                pthread_mutex_lock(&input_mutex);
                query_is_true=km_grabs_get_bpressed(srs.kmg,cmd_u.qbutton_pressed.button);
                pthread_mutex_unlock(&input_mutex);
                PrintLastCommand(LastQuery);
                break;
            case CMD_InitVar:
                cmdprintf("Initialized variable string name '%s' of type %s of value '",cmd_u.init_var.variable,VLNumberTypeStr(cmd_u.init_var.as_number.type));
                if((config.debug_print_type==DBP_AllCommands||this_cmd.print_cmd)){
                    VLNumberPrintNumber(cmd_u.init_var.as_number,config.rpn_decimals);
                    puts("'");
                }
                break;
            case CMD_EditVar:
                cmdprintf("Editing variable string name '%s' with RPN string '%s'\n",VL_get_callback(vl,cmd_u.edit_var)->args.rpn.variable,VL_get_callback(vl,cmd_u.edit_var)->args.rpn.rpn_str);
                RuntimeExitIfProcessVLFalse(ProcessVLCallback(vl,cmd_u.edit_var,0));
                if((config.debug_print_type==DBP_AllCommands||this_cmd.print_cmd)){
                    const char* var=VL_get_callback(vl,cmd_u.edit_var)->args.rpn.variable;
                    StringMapOpt_as_number_t an=StringMap_as_number_read(vl->sman,var);
                    printf("Value of '%s' is now: '",var);
                    VLNumberPrintNumber(an.value,config.rpn_decimals);
                    puts("'");
                }
                break;
            case CMD_WaitUntilKey:
                cmdprintf("Waiting until keypress '%s'%s\n",cmd_u.wait_until_key.key,cmd_u.wait_until_key.invert_press?" (Not Held Down)":"");
                pthread_mutex_lock(&input_mutex);
                if(km_grabs_kgrab_exist(srs.kmg,cmd_u.wait_until_key)){
                    pthread_mutex_unlock(&input_mutex);//DoRuntimeError macro will relock.
                    fprintf(stderr,ERR("Key '%s' has been previously grabbed by a GrabKey command. Exiting program!\n"),cmd_u.wait_until_key.key);
                    DoRuntimeError();
                    break;
                }
                km_grabs_kadd(srs.kmg,cmd_u.wait_until_key);
                srs.do_wait_cond=true;
                srs.wait_for_key=true;
                srs.wait_cond_inv=cmd_u.wait_until_key.invert_press;
                pthread_cond_wait(&wait_cond,&input_mutex);
                srs.do_wait_cond=false;
                srs.wait_for_key=false;
                km_grabs_kremove(srs.kmg,cmd_u.wait_until_key);
                pthread_mutex_unlock(&input_mutex);
                timespec_get(&ts_rm_delay,TIME_UTC);//To remove delay adjustments for CMD_Delay below.
                ts_usleep_before=ts_rm_delay;
                ts_usleep_before_adj=ts_rm_delay;
                break;
            case CMD_WaitUntilButton:
                cmdprintf("Waiting until mouse button '%d' (%s%s)\n",cmd_u.wait_until_button.button,cmd_u.wait_until_button.invert_press?"Not ":"",cmd_u.wait_until_button.held_down?"Held Down":"Clicked");
                pthread_mutex_lock(&input_mutex);
                if(km_grabs_bgrab_exist(srs.kmg,cmd_u.wait_until_button.button)){
                    pthread_mutex_unlock(&input_mutex);//DoRuntimeError macro will relock.
                    fprintf(stderr,ERR("Mouse button %d has been previously grabbed by a GrabButton command. Exiting program!\n"),cmd_u.wait_until_button.button);
                    DoRuntimeError();
                    break;
                }
                km_grabs_badd(srs.kmg,cmd_u.wait_until_button,cmd_u.wait_until_button.held_down);
                srs.wait_button=cmd_u.wait_until_button.button;
                srs.do_wait_cond=true;
                srs.wait_for_button=true;
                srs.wait_cond_inv=cmd_u.wait_until_button.invert_press;
                pthread_cond_wait(&wait_cond,&input_mutex);
                srs.do_wait_cond=false;
                srs.wait_for_button=false;
                km_grabs_bremove(srs.kmg,cmd_u.wait_until_button);
                pthread_mutex_unlock(&input_mutex);
                timespec_get(&ts_rm_delay,TIME_UTC);//To remove delay adjustments for CMD_Delay below.
                ts_usleep_before=ts_rm_delay;
                ts_usleep_before_adj=ts_rm_delay;
                break;
            case CMD_GrabKey:
                cmdprintf("Adding key grab for key '%s'\n",cmd_u.grab_key.key);
                pthread_mutex_lock(&input_mutex);
                km_grabs_kadd(srs.kmg,cmd_u.grab_key);
                pthread_mutex_unlock(&input_mutex);
                break;
            case CMD_UngrabKey:
                cmdprintf("Removing key grab for key '%s'\n",cmd_u.ungrab_key.key);
                pthread_mutex_lock(&input_mutex);
                km_grabs_kremove(srs.kmg,cmd_u.ungrab_key);
                pthread_mutex_unlock(&input_mutex);
                break;
            case CMD_GrabButton:
                cmdprintf("Adding button grab for button %d\n",cmd_u.grab_button.button);
                pthread_mutex_lock(&input_mutex);
                km_grabs_badd(srs.kmg,cmd_u.grab_button,false);
                pthread_mutex_unlock(&input_mutex);
                break;
            case CMD_UngrabButton:
                cmdprintf("Removing button grab for button %d\n",cmd_u.grab_button.button);
                pthread_mutex_lock(&input_mutex);
                km_grabs_bremove(srs.kmg,cmd_u.ungrab_button);
                pthread_mutex_unlock(&input_mutex);
                break;
            case CMD_UngrabKeyAll:
                cmdprintf("Removing all key grabs.\n");
                pthread_mutex_lock(&input_mutex);
                km_grabs_kremove_all(srs.kmg);
                pthread_mutex_unlock(&input_mutex);
                break;
            case CMD_UngrabButtonAll:
                cmdprintf("Removing all button grabs.\n");
                pthread_mutex_lock(&input_mutex);
                km_grabs_bremove_all(srs.kmg);
                pthread_mutex_unlock(&input_mutex);
                break;
            case CMD_PrintString:
                if(!cmd_u.print_string.newline)
                    cmdprintf(ERR("Warning: Debug printing should be off when using PrintString command.\n"));
                print_string_rpn_results=malloc(sizeof(char*)*cmd_u.print_string.rpn_strs_len);
                EXIT_IF_NULL(print_string_rpn_results,char*);
                for(int i=0;i<cmd_u.print_string.rpn_strs_len;i++){
                    RPNEvaluatorEvaluate(cmd_u.print_string.rpn_strs[i],vl,&an_output[0],config.rpn_stack_debug,true,RPN_EVAL_START_B,RPN_EVAL_END_B,RPN_EVAL_SEP);
                    print_string_rpn_results[i]=VLNumberGetNumberString(an_output[0],config.rpn_decimals);
                }
                print_string_copy=replace_str_array(cmd_u.print_string.str,"%_rpn",cmd_u.print_string.rpn_strs_len,(const char**)print_string_rpn_results);
                fputs(print_string_copy,stdout);//printf but without using format strings.
                fflush(stdout);
                for(int i=0;i<cmd_u.print_string.rpn_strs_len;i++)
                    free(print_string_rpn_results[i]);
                free(print_string_rpn_results);
                free(print_string_copy);
                break;
            case CMD_DebugConfig:
                cmdprintf("Changing debug config '%s' to value '%ld'\n",DebugConfigTypeString[cmd_u.debug_config.type],cmd_u.debug_config.value);
                switch(cmd_u.debug_config.type){
                    case DCT_DebugPrintType:
                        config.debug_print_type=cmd_u.debug_config.value;
                        break;
                    case DCT_RPNDecimals:
                        config.rpn_decimals=cmd_u.debug_config.value;
                        break;
                    case DCT_RPNStackDebug:
                        config.rpn_stack_debug=cmd_u.debug_config.value;
                }
                break;
        }
        if(this_cmd.subtype!=CMDST_Query) ++cmd_arr_i;
        else cmd_arr_i+=(query_is_true^this_cmd.query_details.invert)?this_cmd.query_details.jump_e:this_cmd.query_details.jump_ne;
        pthread_mutex_lock(&input_mutex);
        if(cmd_arr_i==cmd_arr_len) srs.program_done=true;//To end the mouse_input_t thread loop as well.
    }
    empty_cmd_index();
    timespec_diff(&ts_begin,NULL,&ts_diff);
    printf("%ld.%09ld seconds since macro script ran.\n",ts_diff.tv_sec,ts_diff.tv_nsec);
    pthread_mutex_unlock(&input_mutex);
    pthread_join(keyboard_input_t,NULL);
    km_grabs_free(srs.kmg);
    key_down_check_key_up(kdc,xdo_obj,CURRENTWINDOW);
    key_down_check_free(kdc);
    return no_error;
}
bool input_state_func(void* MS_v){//Sets menu_state variable in main(void) to v.
    *(((ms_cont_t*)MS_v)->ms)=((ms_cont_t*)MS_v)->v;
    return false;
}
bool test_mouse_func(void* xdo_v){
    int x_mouse,y_mouse;
    XColor pc;
    xdo_get_mouse_location((xdo_t*)xdo_v,&x_mouse,&y_mouse,0);
    printf("You clicked at x:%d y:%d",x_mouse,y_mouse);
    get_pixel_color(((xdo_t*)xdo_v)->xdpy,x_mouse,y_mouse,&pc);
    printf(" with r:%u g:%u b:%u\n",pc.red>>8,pc.green>>8,pc.blue>>8);//Truncate to byte instead.
    return true;
}
inline static void clear_stdin(){
    static char next_c;
    static struct pollfd pfd={.fd=STDIN_FILENO,.events=POLLIN};//Using poll(,,0) to read() without block.
    while(poll(&pfd,1,0)>0) read(STDIN_FILENO,&next_c,sizeof(next_c));
}
