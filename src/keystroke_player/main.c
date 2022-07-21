#include "parser.h"
#include "key_down_check.h"
#include <string.h>
#define __USE_XOPEN
#include <unistd.h>
#include <xdo.h>
#include <pthread.h>
int usleep(useconds_t usec);
void run_program(command_array_t* cmd_arr);
int main(int argc, char** argv){
    (void)argc;
    (void)argv;
    //char a_string_stack[]="(A;w+a=d.s1;w+a=u.s1;d=d.s2;d=u.0;(B;i=d.1000;i=u.1000;o=d.1000;o=u.1000;)B=5;)A=3";
    char a_string_stack[]="(A;m5=u.m200;)A=10";
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
typedef struct{
    xdo_t* xdo_obj;
    bool program_done;
    struct{
        int x;
        int y;
    }mouse;
}shared_rs;
pthread_mutex_t input_mutex;
void* close_listener(void* srs_p){
    shared_rs* srs=(shared_rs*)srs_p;
    int mouse_x_before,mouse_y_before,mouse_x_after,mouse_y_after;
    pthread_mutex_lock(&input_mutex);
    mouse_x_before=srs->mouse.x; mouse_y_before=srs->mouse.y;//To use if mouse moved after autoclicker started.
    while(!srs->program_done){
        pthread_mutex_unlock(&input_mutex);
        usleep(100000);
        pthread_mutex_lock(&input_mutex);
        xdo_get_mouse_location(srs->xdo_obj,&mouse_x_after,&mouse_y_after,NULL);
        if(mouse_x_before!=mouse_x_after||mouse_y_before!=mouse_y_after){
            printf("Mouse moved. Stopping autoclicker.\n");
            srs->program_done=true;
        }
    }
    pthread_mutex_unlock(&input_mutex);
    pthread_exit(NULL);
}
void run_program(command_array_t* cmd_arr){
    xdo_t* xdo_obj=xdo_new(NULL);
    Window focus_window;
    xdo_select_window_with_click(xdo_obj,&focus_window);
    printf("%lu\n",(size_t)focus_window);
    xdo_raise_window(xdo_obj,focus_window);
    xdo_focus_window(xdo_obj,focus_window);
    int cmd_arr_len=command_array_count(cmd_arr);
    int cmd_arr_i=0;
    key_down_check_t* kdt=key_down_check_new();
    shared_rs srs=(shared_rs){.xdo_obj=xdo_obj,.program_done=false,.mouse={0}};
    xdo_get_mouse_location(xdo_obj,&srs.mouse.x,&srs.mouse.y,NULL);
    pthread_t input_f;
    pthread_mutex_init(&input_mutex,NULL);
    pthread_create(&input_f,NULL,close_listener,&srs);
    pthread_mutex_lock(&input_mutex);
    while(!srs.program_done){
        pthread_mutex_unlock(&input_mutex);
        command_union_t cmd_u=cmd_arr->cmds[cmd_arr_i].cmd;
        CommandType cmd_type=cmd_arr->cmds[cmd_arr_i].type;
        int* rst_counter=0;
        switch(cmd_type){
            case(VT_KeyStroke):
                pthread_mutex_lock(&input_mutex);
                if(cmd_u.ks.key_state){
                    printf("Command #%d: Key down for %s\n",cmd_arr_i,cmd_u.ks.key);
                    xdo_send_keysequence_window_down(xdo_obj,CURRENTWINDOW,cmd_u.ks.key,0);
                    key_down_check_add(kdt,cmd_u.ks.key);
                }
                else{
                    printf("Command #%d: Key up for %s\n",cmd_arr_i,cmd_u.ks.key);
                    xdo_send_keysequence_window_up(xdo_obj,CURRENTWINDOW,cmd_u.ks.key,0);
                    key_down_check_remove(kdt,cmd_u.ks.key);
                }
                pthread_mutex_unlock(&input_mutex);
                break;
            case(VT_Delay):
                printf("Command #%d: Sleeping for %ld microseconds\n",cmd_arr_i,cmd_u.delay);
                usleep(cmd_u.delay);
                break;
            case(VT_RepeatEnd):
                rst_counter=&(cmd_arr->cmds[cmd_u.repeat_end.index].cmd.repeat_start.counter);
                printf("Command #%d: Jump to Command #%d if Counter %d reaches %d\n",cmd_arr_i,cmd_u.repeat_end.index,++(*rst_counter),cmd_u.repeat_end.counter_max);
                if(*rst_counter!=cmd_u.repeat_end.counter_max) cmd_arr_i=cmd_u.repeat_end.index-1;//Go back if not counter_max
                else *rst_counter=0;//Reset to 0.
                break;
            case(VT_RepeatStart):
                printf("Command #%d: This is a loop counter (%d).\n",cmd_arr_i,cmd_u.repeat_start.counter);
                break;
            default:
                break;
        }
        pthread_mutex_lock(&input_mutex);
        if(++cmd_arr_i==cmd_arr_len) break;
    }
    srs.program_done=true;//If not done yet by other thread.
    pthread_mutex_unlock(&input_mutex);
    pthread_join(input_f,NULL);
    key_down_check_key_up(kdt,xdo_obj,focus_window);
    key_down_check_free(kdt);
    xdo_free(xdo_obj);
}
