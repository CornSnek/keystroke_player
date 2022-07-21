#include "parser.h"
#include "key_down_check.h"
#include <string.h>
#define __USE_XOPEN
#include <unistd.h>
#include <xdo.h>
#include <pthread.h>
int usleep(useconds_t usec);
bool fgets_change(char* str,int buffer_len){//Remove '\n' (if any) and check if string buffer is full (returns bool). 
    int real_len=strlen(str);
    bool is_full=(buffer_len-1==real_len);
    if(!is_full){
        str[real_len-1]='\0';
    }
    return is_full;
}
int parse_buffer();
bool run_program(command_array_t* cmd_arr);
#define INPUT_BUFFER_LEN 101
int main(int argc, char** argv){
    (void)argc;
    (void)argv;
    char input_str[INPUT_BUFFER_LEN]={0};
    while(true){
        printf("Type file name of autoclicker script to use, or type '\\q' to quit: ");
        fgets(input_str,INPUT_BUFFER_LEN,stdin);
        if(!strncmp(input_str,"\\q",2)) break;
        bool is_full=fgets_change(input_str,INPUT_BUFFER_LEN);
        printf("%s %d\n",input_str,is_full);
    }
    return parse_buffer();
}
int parse_buffer(){
    //char a_string_stack[]="(A;w+a=d.s1;w+a=u.s1;d=d.s2;d=u.0;(B;i=d.1000;i=u.1000;o=d.1000;o=u.1000;)B=5;)A=3";
    char a_string_stack[]="(A;\nm1=d.0;m1=u.0;m3=d.0;m3=u.s1;)A=10;";
    char* a_string_heap=(char*)malloc(sizeof(a_string_stack)/sizeof(char));
    strcpy(a_string_heap,a_string_stack);
    shared_string_manager* ssm=SSManager_new();
    command_array_t* cmd_arr=command_array_new(ssm);
    repeat_id_manager_t* rim=repeat_id_manager_new(ssm);
    macro_buffer_t* mb=macro_buffer_new(a_string_heap,ssm,cmd_arr,rim);
    while(macro_buffer_process_next(mb)){
        if(mb->parse_i>mb->size) break;
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
void* mouse_check_listener(void* srs_v){
    shared_rs* srs_p=(shared_rs*)srs_v;
    int mouse_x_before,mouse_y_before,mouse_x_after,mouse_y_after;
    pthread_mutex_lock(&input_mutex);
    mouse_x_before=srs_p->mouse.x; mouse_y_before=srs_p->mouse.y;//To use if mouse moved after autoclicker started.
    while(!srs_p->program_done){
        pthread_mutex_unlock(&input_mutex);
        usleep(10000);
        pthread_mutex_lock(&input_mutex);
        xdo_get_mouse_location(srs_p->xdo_obj,&mouse_x_after,&mouse_y_after,NULL);
        if(mouse_x_before!=mouse_x_after||mouse_y_before!=mouse_y_after){
            printf("Mouse moved. Stopping autoclicker.\n");
            srs_p->program_done=true;
        }
    }
    pthread_mutex_unlock(&input_mutex);
    pthread_exit(NULL);
}
bool run_program(command_array_t* cmd_arr){
    xdo_t* xdo_obj=xdo_new(NULL);
    Window focus_window;
    xdo_select_window_with_click(xdo_obj,&focus_window);
    xdo_raise_window(xdo_obj,focus_window);
    xdo_focus_window(xdo_obj,focus_window);
    int cmd_arr_len=command_array_count(cmd_arr);
    int cmd_arr_i=0;
    key_down_check_t* kdc=key_down_check_new();
    shared_rs srs=(shared_rs){.xdo_obj=xdo_obj,.program_done=false,.mouse={0}};
    usleep(1000000);
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
        switch(cmd_type){
            case(VT_KeyStroke):
                pthread_mutex_lock(&input_mutex);
                if(cmd_u.ks.key_state){
                    printf("Command %d/%d: Key down for %s\n",cmd_arr_i,cmd_arr_len-1,cmd_u.ks.key);
                    xdo_send_keysequence_window_down(xdo_obj,CURRENTWINDOW,cmd_u.ks.key,0);
                    key_down_check_add(kdc,cmd_u.ks.key);
                }else{
                    printf("Command %d/%d: Key up for %s\n",cmd_arr_i,cmd_arr_len-1,cmd_u.ks.key);
                    xdo_send_keysequence_window_up(xdo_obj,CURRENTWINDOW,cmd_u.ks.key,0);
                    key_down_check_remove(kdc,cmd_u.ks.key);
                }
                pthread_mutex_unlock(&input_mutex);
                break;
            case(VT_Delay):
                printf("Command %d/%d: Sleeping for %ld microseconds\n",cmd_arr_i,cmd_arr_len-1,cmd_u.delay);
                usleep(cmd_u.delay);
                break;
            case(VT_RepeatEnd):
                rst_counter=&(cmd_arr->cmds[cmd_u.repeat_end.index].cmd.repeat_start.counter);
                printf("Command %d/%d: Jump to Command #%d if Counter %d reaches %d\n",cmd_arr_i,cmd_arr_len-1,cmd_u.repeat_end.index+1,++(*rst_counter),cmd_u.repeat_end.counter_max);
                if(*rst_counter!=cmd_u.repeat_end.counter_max) cmd_arr_i=cmd_u.repeat_end.index;//Go back if not counter_max
                else *rst_counter=0;//Reset to loop again.
                break;
            case(VT_RepeatStart):
                printf("Command %d/%d: This is a loop counter (%d).\n",cmd_arr_i,cmd_arr_len-1,cmd_u.repeat_start.counter);
                break;
            case(VT_MouseClick):
                pthread_mutex_lock(&input_mutex);
                if(cmd_u.ks.key_state){
                    printf("Command %d/%d: Mouse down (%d).\n",cmd_arr_i,cmd_arr_len-1,cmd_u.mouse.mouse_type);
                    xdo_mouse_down(xdo_obj,CURRENTWINDOW,cmd_u.mouse.mouse_type);
                }else{
                    printf("Command %d/%d: Mouse up (%d).\n",cmd_arr_i,cmd_arr_len-1,cmd_u.mouse.mouse_type);
                    xdo_mouse_up(xdo_obj,CURRENTWINDOW,cmd_u.mouse.mouse_type);
                }
                pthread_mutex_unlock(&input_mutex);
                break;
            default:
                break;
        }
        pthread_mutex_lock(&input_mutex);
        if(++cmd_arr_i==cmd_arr_len){
            srs.program_done=true;//To end the input_t thread loop as well.
        }
    }
    pthread_mutex_unlock(&input_mutex);
    pthread_join(input_t,NULL);
    key_down_check_key_up(kdc,xdo_obj,focus_window);
    key_down_check_free(kdc);
    xdo_free(xdo_obj);
    return true;
}
