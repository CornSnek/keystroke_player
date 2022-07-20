#include "parser.h"
#include "key_down_check.h"
#include <string.h>
#define __USE_XOPEN
#include <unistd.h>
#include <xdo.h>
int usleep(useconds_t usec);
void run_program(command_array_t* cmd_arr);
int main(int argc, char** argv){
    (void)argc;
    (void)argv;
    printf("Making a keystroke player in c.\n");
    char a_string_stack[]="w=d,a=d,s=d.s1;d=d.s2;q=d.0";
    char* a_string_heap=(char*)malloc(sizeof(a_string_stack)/sizeof(char));
    strcpy(a_string_heap,a_string_stack);
    shared_string_manager* ssm=SSManager_new();
    command_array_t* cmd_arr=command_array_new(ssm);
    repeat_id_manager_t* rim=repeat_id_manager_new(ssm);
    macro_buffer_t* mb=macro_buffer_new(a_string_heap,ssm,cmd_arr,rim);
    while(macro_buffer_process_next(mb)){
        if(mb->parse_i==mb->size) break;
    }
    if(!mb->parse_error){
        run_program(cmd_arr);
    }
    repeat_id_manager_free(rim);
    command_array_free(cmd_arr);
    SSManager_free(ssm);
    bool parse_error=mb->parse_error;
    macro_buffer_free(mb);
    return parse_error;
}
void run_program(command_array_t* cmd_arr){
    xdo_t* xdo_obj=xdo_new(NULL);
    Window focus_window;
    xdo_select_window_with_click(xdo_obj,&focus_window);
    printf("%lu\n",(size_t)focus_window);
    xdo_raise_window(xdo_obj,focus_window);
    xdo_focus_window(xdo_obj,focus_window);
    Window new_window=0;
    int cmd_arr_len=command_array_count(cmd_arr);
    int cmd_arr_i=0;
    key_down_check_t* kdt=key_down_check_new();
    while(true){
        xdo_get_focused_window_sane(xdo_obj,&new_window);//Break if window changes.
        if(new_window!=focus_window){
            break;
        }
        command_t cmd=cmd_arr->cmds[cmd_arr_i];
        command_union_t cmd_u=cmd_arr->cmds[cmd_arr_i].cmd;
        CommandType cmd_type=cmd_arr->cmds[cmd_arr_i].type;
        switch(cmd_type){
            case(VT_KeyStroke):
                if(cmd_u.ks.key_state){
                    xdo_send_keysequence_window_down(xdo_obj,focus_window,cmd_u.ks.key,0);
                    key_down_check_add(kdt,cmd_u.ks.key);
                }
                else{
                    xdo_send_keysequence_window_up(xdo_obj,focus_window,cmd_u.ks.key,0);
                    key_down_check_remove(kdt,cmd_u.ks.key);
                }
                break;
            case(VT_Delay):
                usleep(cmd_u.delay);
                break;
            case(VT_RepeatEnd):
                printf("TODO\n");
                break;
            default:
                break;
        }
        if(++cmd_arr_i==cmd_arr_len) break;
    }
    if(new_window){//Disable keys if on new window.
        xdo_focus_window(xdo_obj,new_window);
        key_down_check_key_up(kdt,xdo_obj,new_window);
    }
    key_down_check_key_up(kdt,xdo_obj,focus_window);
    key_down_check_free(kdt);
    xdo_free(xdo_obj);
}
