#include <key_down_check.h>
#include <X11/Xutil.h>
key_down_check_t* key_down_check_new(void){
    key_down_check_t* this=(malloc(sizeof(key_down_check_t)));
    EXIT_IF_NULL(this,key_down_check_t*);
    this->keys=malloc(sizeof(const char*)*1);//No need to malloc if 0. Just realloc.
    EXIT_IF_NULL(this->keys,const char*);
    this->len=0;
    return this;
}
bool key_down_check_add(key_down_check_t* this,const char* add_key){
    for(int i=0;i<this->len;i++){
        if(!strcmp(this->keys[i],add_key)) return false;//Don't add duplicate keys.
    }
    this->keys[this->len++]=add_key;
    this->keys=(const char**)(realloc(this->keys,sizeof(const char*)*(this->len+1)));
    EXIT_IF_NULL(this->keys,const char*);
    return true;
}
bool key_down_check_remove(key_down_check_t* this,const char* rem_key){
    for(int i=0;i<this->len;i++){
        if(!strcmp(this->keys[i],rem_key)){//If found, just move the last key in the array where the deleted_key is, and NULL last.
            this->keys[i]=this->keys[this->len-1];
            this->keys[(this->len--)-1]=NULL;
            this->keys=(const char**)(realloc(this->keys,sizeof(const char*)*(this->len+1)));
            EXIT_IF_NULL(this->keys,const char*);
            return true;
        }
    }
    return false;
}
void key_down_check_print(const key_down_check_t* this){
    printf("Keys pressed: ");
    for(int i=0;i<this->len;i++){
        printf("%s ",this->keys[i]);
    }
    puts("");
}
void key_down_check_key_up(key_down_check_t* this,xdo_t* xdo_obj,Window window){
    for(int i=0;i<this->len;i++){
        xdo_send_keysequence_window_up(xdo_obj,window,this->keys[i],0);
    }
}
void key_down_check_free(key_down_check_t* this){
    free(this->keys);
    free(this);
}
bool _KeybindDisable=false;
//Removes keybinds except for escape key.
void _GlobalKeybindDisable(Display* xdpy,int scr,bool d,const callback_t* cb_list,size_t cb_list_len){
    _KeybindDisable=d;
    puts(_KeybindDisable?"Escape key pressed. Keybinds are currently disabled. To reenable, press Escape key again.":"Escape key pressed. Keybinds are currently enabled.");
    for(size_t i=0;i<cb_list_len;i++) d?XUngrabKey(xdpy,XKeysymToKeycode(xdpy,cb_list[i].ks),None,RootWindow(xdpy,scr)):XGrabKey(xdpy,XKeysymToKeycode(xdpy,cb_list[i].ks),None,RootWindow(xdpy,scr),False,GrabModeAsync,GrabModeAsync);
}
void wait_for_keypress(Display* xdpy,KeySym ks){
    const callback_t to_disable={.ks=ks};
    int scr=DefaultScreen(xdpy);
    XEvent e={0};
    bool finished=false;
    XGrabKey(xdpy,XKeysymToKeycode(xdpy,ks),None,RootWindow(xdpy,scr),False,GrabModeAsync,GrabModeAsync);
    XGrabKey(xdpy,XKeysymToKeycode(xdpy,XK_Escape),None,RootWindow(xdpy,scr),False,GrabModeAsync,GrabModeAsync);
    XFlush(xdpy);
    while(!finished){
        usleep(100000);
        while(XPending(xdpy)){
            XNextEvent(xdpy,&e);
            if(!_KeybindDisable){
                if(e.type==KeyPress&&e.xkey.keycode==XKeysymToKeycode(xdpy,ks)) finished=true;
                else if(e.type==KeyPress&&e.xkey.keycode==XKeysymToKeycode(xdpy,XK_Escape)) _GlobalKeybindDisable(xdpy,scr,true,&to_disable,1);
            }else if(e.type==KeyPress&&e.xkey.keycode==XKeysymToKeycode(xdpy,XK_Escape)) _GlobalKeybindDisable(xdpy,scr,false,&to_disable,1);
        }
    }
    XUngrabKey(xdpy,XKeysymToKeycode(xdpy,ks),None,RootWindow(xdpy,scr));
    XUngrabKey(xdpy,XKeysymToKeycode(xdpy,XK_Escape),None,RootWindow(xdpy,scr));
    XFlush(xdpy);
}
void keypress_loop(Display* xdpy,const callback_t* cb_list,size_t cb_list_len){
    int scr=DefaultScreen(xdpy);
    XEvent e={0};
    bool keep_looping=true;
    for(size_t i=0;i<cb_list_len;i++) XGrabKey(xdpy,XKeysymToKeycode(xdpy,cb_list[i].ks),None,RootWindow(xdpy,scr),False,GrabModeAsync,GrabModeAsync);
    XGrabKey(xdpy,XKeysymToKeycode(xdpy,XK_Escape),None,RootWindow(xdpy,scr),False,GrabModeAsync,GrabModeAsync);
    XFlush(xdpy);
    while(keep_looping){
        usleep(100000);
        while(XPending(xdpy)){
            XNextEvent(xdpy,&e);
            if(e.type==KeyPress){
                if(!_KeybindDisable){
                    if(e.type==KeyPress&&e.xkey.keycode==XKeysymToKeycode(xdpy,XK_Escape)){
                        _GlobalKeybindDisable(xdpy,scr,true,cb_list,cb_list_len);
                        break;
                    } 
                    for(size_t i=0;i<cb_list_len;i++){
                        if(e.xkey.keycode==XKeysymToKeycode(xdpy,cb_list[i].ks)){
                            keep_looping&=cb_list[i].func(cb_list[i].arg);//Keep looping until one is false.
                            break;
                        }
                    }
                }else if(e.type==KeyPress&&e.xkey.keycode==XKeysymToKeycode(xdpy,XK_Escape)) _GlobalKeybindDisable(xdpy,scr,false,cb_list,cb_list_len);
            }
        }
    }
    for(size_t i=0;i<cb_list_len;i++) XUngrabKey(xdpy,XKeysymToKeycode(xdpy,cb_list[i].ks),None,RootWindow(xdpy,scr));
    XUngrabKey(xdpy,XKeysymToKeycode(xdpy,XK_Escape),None,RootWindow(xdpy,scr));
    XFlush(xdpy);
}
bool CallbackEndLoop(void* v){
    (void)v;
    return false;
}
bool _boolean_edit_func(void* b_v){
    *(((_boolean_edit_t*)b_v)->p)=((_boolean_edit_t*)b_v)->v;
    return false;
}
key_grabs_t* key_grabs_new(xdo_t* xdo_obj){
    key_grabs_t* this=malloc(sizeof(key_grabs_t));
    EXIT_IF_NULL(this,key_grabs_t*);
    *this=(key_grabs_t){.xdo_obj=xdo_obj,.ks_arr=0,.ks_pressed=0,.size=0};
    return this;
}
void key_grabs_add(key_grabs_t* this,keystroke_t ks_add){
    for(int i=0;i<this->size;i++)//Skip duplicates with same key keysym.
        if(this->ks_arr[i].keysym==ks_add.keysym) return;
    this->ks_arr=this->size++?realloc(this->ks_arr,sizeof(keystroke_t)*sizeof(this->size)):malloc(sizeof(keystroke_t));
    this->ks_pressed=this->size?realloc(this->ks_pressed,sizeof(bool)*sizeof(this->size)):malloc(sizeof(bool));
    EXIT_IF_NULL(this->ks_arr,keystroke_t*);
    EXIT_IF_NULL(this->ks_pressed,bool*);
    this->ks_arr[this->size-1]=ks_add;
    this->ks_pressed[this->size-1]=false;
    XGrabKey(this->xdo_obj->xdpy,XKeysymToKeycode(this->xdo_obj->xdpy,ks_add.keysym),None,RootWindow(this->xdo_obj->xdpy,DefaultScreen(this->xdo_obj->xdpy)),False,GrabModeAsync,GrabModeAsync);
    XFlush(this->xdo_obj->xdpy);
}
bool key_grabs_grab_exist(const key_grabs_t* this,keystroke_t ks){
    for(int i=0;i<this->size;i++)
        if(this->ks_arr[i].keysym==ks.keysym) return true;
    return false;
}
void key_grabs_set_pressed(key_grabs_t* this,keystroke_t ks,bool pressed){
    for(int i=0;i<this->size;i++){
        if(this->ks_arr[i].keysym==ks.keysym){
            this->ks_pressed[i]=pressed;
            break;
        }
    }
}
kg_bool_t key_grabs_get_pressed(const key_grabs_t* this,keystroke_t ks){
    for(int i=0;i<this->size;i++){
        if(this->ks_arr[i].keysym==ks.keysym){
            return (kg_bool_t){.exist=true,.pressed=this->ks_pressed[i]};
        }
    }
    return (kg_bool_t){0};
}
void key_grabs_remove(key_grabs_t* this,keystroke_t ks_rm){
    for(int i=0;i<this->size;i++){
        if(this->ks_arr[i].keysym==ks_rm.keysym){
            XUngrabKey(this->xdo_obj->xdpy,XKeysymToKeycode(this->xdo_obj->xdpy,ks_rm.keysym),None,RootWindow(this->xdo_obj->xdpy,DefaultScreen(this->xdo_obj->xdpy)));
            this->ks_arr[i]=this->ks_arr[--this->size];//Move the last to the deleted index.
            this->ks_pressed[i]=this->ks_pressed[this->size];
            if(this->size){
                this->ks_arr=realloc(this->ks_arr,sizeof(keystroke_t)*sizeof(this->size));
                this->ks_pressed=realloc(this->ks_pressed,sizeof(bool)*sizeof(this->size));
                EXIT_IF_NULL(this->ks_arr,keystroke_t*);
                EXIT_IF_NULL(this->ks_pressed,bool*);
            }else{
                free(this->ks_arr);
                this->ks_arr=0;
                free(this->ks_pressed);
                this->ks_pressed=0;
            }
        }
    }
}
void key_grabs_remove_all(key_grabs_t* this){
    for(int i=0;i<this->size;i++)
        XUngrabKey(this->xdo_obj->xdpy,XKeysymToKeycode(this->xdo_obj->xdpy,this->ks_arr[i].keysym),None,RootWindow(this->xdo_obj->xdpy,DefaultScreen(this->xdo_obj->xdpy)));
    free(this->ks_arr);
    free(this->ks_pressed);
    this->size=0;
    this->ks_arr=0;
}
void key_grabs_free(key_grabs_t* this){
    for(int i=0;i<this->size;i++)
        XUngrabKey(this->xdo_obj->xdpy,XKeysymToKeycode(this->xdo_obj->xdpy,this->ks_arr[i].keysym),None,RootWindow(this->xdo_obj->xdpy,DefaultScreen(this->xdo_obj->xdpy)));
    free(this->ks_arr);
    free(this->ks_pressed);
    free(this);
}